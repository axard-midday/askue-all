#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include "config.h"
#include "monitor.h"
#include "write_msg.h"
#include "journal.h"
#include "macro.h"
#include "log.h"
#include "cli.h"

typedef enum _askue_cfg_flag_t
{
    Askue_ReConfigure,
    Askue_Configure
} askue_cfg_flag_t;

/*                         Чтение конфигурации                        */
static
int askue_configure ( askue_cfg_t *Cfg, FILE **Log, askue_cfg_flag_t CfgFlag )
{
    if ( CfgFlag == Askue_ReConfigure )
    {
        askue_config_destroy ( Cfg );
        askue_config_init ( Cfg );
        askue_log_close ( Log );
    }
        
    verbose_msg ( Cfg->Flag, stdout, "Конфигурация", "OK", "Инициализация" );
    
    if ( !askue_config_read ( Cfg ) &&
         !askue_journal_init ( Cfg ) &&
         !askue_log_open ( Log, Cfg ) )
    {
        return 0;
    }
    else
    {
        askue_config_destroy ( Cfg );
        return -1;
    }
}

/*                          Основной цикл программы                   */
static
loop_state_t main_loop ( askue_cfg_t *Cfg, FILE **Log, const sigset_t *SignalSet )
{
    loop_state_t LS = LoopOk;
    
    verbose_msg ( Cfg->Flag, Log, "Демон", "OK", "Старт опроса." );
    while ( LS != LoopError && LS != LoopExit )
    {
        LS = run_monitor_loop ( *Log, Cfg, SignalSet );
        if ( LS == LoopReconfig )
            LS = ( askue_configure ( Cfg, Log, Askue_ReConfigure ) ) ? LoopError : LoopOk;
            
        if ( TESTBIT ( Cfg->Flag, ASKUE_FLAG_CYCLE ) )
            break;
    }
    
	return LS;
}

/*                   Установка отслеживаемых сигналов                 */
static
void signal_set_init ( sigset_t *sigset )
{
    // настраиваем сигналы которые будем обрабатывать
    sigemptyset(sigset);
    // сигнал остановки процесса пользователем
    sigaddset(sigset, SIGQUIT);
    // сигнал для остановки процесса пользователем с терминала
    sigaddset(sigset, SIGINT);
    // сигнал запроса завершения процесса
    sigaddset(sigset, SIGTERM);
    // сигнал посылаемый при изменении статуса дочернего процесса
    sigaddset(sigset, SIGCHLD); 
    // пользовательский сигнал который мы будем использовать для обновления конфига
    sigaddset(sigset, SIGUSR1); 
    sigprocmask(SIG_BLOCK, sigset, NULL);
}

/*                      Стать независимым от родителя                 */
static
int set_independent ( askue_cfg_t *Cfg, FILE *Log )
{
    char Buffer[ 256 ];
    umask ( 0 );
    
    if ( setsid ( ) != -1 &&
         snprintf ( Buffer, 256, "setsid(): %s (%d)", strerror ( errno ), errno ) != -1)
    {
        write_log ( Log, "Демон", "FAIL", Buffer );
        return -1
    }
    else
    {
        verbose_msg ( Cfg->Flag, Log, "Демон", "OK", "Новый сеанс создан." );
    }
    
    if ( chdir ( "/" ) != -1 &&
         snprintf ( Buffer, 256, "chdir(): %s (%d)", strerror ( errno ), errno ) != -1 )
    {
        write_log ( Log, "Демон", "FAIL", Buffer );
        return -1;
    }
    else
    {
        verbose_msg ( Cfg->Flag, Log, "Демон", "OK", "Каталог сменён." );
    }
    
    return 0;
}

/*                      Запуск аскуе                                  */
int run_askue ( askue_cfg_t *Cfg, FILE **Log )
{
    if ( set_independent ( *Log ) )
    {
        return -1;
    }
            
    sigset_t SignalSet;
    signal_set_init ( &SignalSet );
            
    loop_state_t ls = main_loop ( Cfg, Log, ( const sigset_t* ) &SignalSet );
    
    return ( ls == LoopExit ) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*      Отчёт об ошибках разбора аргументов командной строки          */
static
void say_cli_error ( cli_result_t CliResult )
{
    switch ( CliResult )
    {
        case CLI_ERROR_ARG: 
            write_msg ( stdout, "CLI", "FAIL", "Неизвестный аргумент." );
            break;
        case CLI_ERROR_HANDLER: 
            write_msg ( stdout, "CLI", "FAIL", "Ошибка обработчика аргумента." );
            break;
        case CLI_ERROR_NF_OPT:
            write_msg ( stdout, "CLI", "FAIL", "Не найден аргумент." );
            break;
        case CLI_ERROR_NOARG:
            write_msg ( stdout, "CLI", "FAIL", "Нет требуемого аргумента." );
            break;
        case CLI_ERROR_OPTYPE:
            write_msg ( stdout, "CLI", "FAIL", "Невозможно рзобрать тип опции." );
            break;
        default:
            break;
    }
}

/*                   Разбор опций командной строки                    */
static
int askue_cli_parse ( cli_option_t *vOption, int argc, char **argv )
{
    cli_result_t CliResult = cli_parse ( vOption, argc, argv );
    if ( CliResult != CLI_SUCCESS )
    {
        say_cli_error ( CliResult );
        return -1;
    }
    else
    {
        return 0;
    }
}

/*                  Обработчики аргументов командной строки           */
// многословный режим
static
int __cli_verbose ( void *ptr, int *flag, const char *arg )
{
    SETBIT ( *( uint32_t* )ptr, ASKUE_FLAG_VERBOSE );
    return 0;
}
// прогнать один круг
static
int __cli_cycle ( void *ptr, int *flag, const char *arg )
{
    SETBIT ( *( uint32_t* )ptr, ASKUE_FLAG_CYCLE );
    return 0;
}
// печатать протокол обмена
static
int __cli_protocol ( void *ptr, int *flag, const char *arg )
{
    SETBIT ( *( uint32_t* )ptr, ASKUE_FLAG_PROTOCOL );
    return 0;
} 

/*                      Точка входа в программу                       */
int main ( int argc, char **argv )
{
    FILE *Log;
    askue_cfg_t Cfg;
    askue_config_init ( &Cfg );

    write_msg ( stdout, "АСКУЭ", "OK", "Старт" );
 
    cli_option_t vOption[] =
    {
        { "verbose", 'v', CLI_NO_ARG, __cli_verbose, &( Cfg.Flag ), NULL },
        { "cycle", 'c', CLI_NO_ARG, __cli_cycle, &( Cfg.Flag ), NULL },
        { "protocol", 'p', CLI_NO_ARG, __cli_protocol, &( Cfg.Flag ), NULL },
        CLI_LAST_OPTION
    };
    
    if ( !askue_cli_parse ( vOption, argc, argv ) &&
         !askue_configure ( &Cfg, &Log, Askue_Configure ) )
    {
        
        char Buffer[ 256 ];
        pid_t pid = fork ();
        if ( pid < 0 )
        {
            if ( snprintf ( Buffer, 256, "fork(): %s (%d)", strerror ( errno ), errno ) != -1 )
                verbose_msg ( Cfg.Flag, stdout, "Демон", "FAIL", Buffer );
            exit ( EXIT_FAILURE ); 
        }
        else if ( !pid )
        {
            int exit_status = run_askue ( &Cfg, &Log );
            askue_log_close ( &Log );
            askue_config_destroy ( &Cfg );
            exit ( exit_status );
        }
        else
        {
            exit ( EXIT_SUCCESS );
        }
        
    }
    else
    {
        exit ( EXIT_FAILURE );
    }
}

