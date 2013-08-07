
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <libaskue.h>

#include "config.h"
#include "monitor.h"
#include "journal.h"
#include "log.h"
#include "text_buffer.h"

/*                         Чтение конфигурации                        */
static
int askue_configure ( askue_workspace_t *WS, askue_cfg_t *Cfg )
{
    uint32_t Flag = Cfg->Flag;
    
    askue_config_destroy ( Cfg );
    askue_config_init ( Cfg );
    askue_log_close ( &( WS->Log ) );
        
    Cfg->Flag = Flag;
    
    verbose_msg ( Cfg->Flag, stdout, "Конфигурация", "OK", "Инициализация" );
    
    if ( !askue_config_read ( Cfg ) &&
         !askue_journal_init ( Cfg ) &&
         !askue_log_open ( &( WS->Log ), Cfg ) )
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
int main_loop ( askue_workspace_t *WS, askue_cfg_t *Cfg )
{
    verbose_msg ( Cfg->Flag, WS->Log, "АСКУЭ", "OK", "Старт опроса." );
    
    while ( WS->Loop == LoopOk )
    {
        if ( run_monitor_loop ( WS, Cfg ) == -1 )
            return -1;
            
        if ( WS->Loop == LoopReconfig )
        {
            if ( askue_configure ( WS, Cfg ) == -1 )
            {
                WS->Loop = LoopError;
            }
            else
            {
                WS->Loop = LoopOk;
            }  
        }
    }
    
	return ( WS->Loop == LoopError ) ? -1 : 0;
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
int set_independent ( askue_workspace_t *WS, askue_cfg_t *Cfg )
{
    umask ( 0 );
    
    if ( setsid () == -1 )
    {
        text_buffer_write ( WS->Buffer, "setsid(): %s (%d)", strerror ( errno ), errno );
        write_msg ( WS->Log, "АСКУЭ", "FAIL", WS->Buffer->Text );
        return -1;
    }
    else
    {
        verbose_msg ( Cfg->Flag, WS->Log, "АСКУЭ", "OK", "Новый сеанс создан." );
    }
    
    if ( chdir ( "/home/axard/" ) == -1 )
    {
        text_buffer_write ( WS->Buffer, "chdir(): %s (%d)", strerror ( errno ), errno );
        write_log ( WS->Log, "АСКУЭ", "FAIL", WS->Buffer->Text );
        return -1;
    }
    else
    {
        verbose_msg ( Cfg->Flag, WS->Log, "АСКУЭ", "OK", "Каталог изменён." );
    }
    
    return 0;
}

/*                      Запуск аскуе                                  */
int run_askue ( askue_workspace_t *WS, askue_cfg_t *Cfg )
{
    if ( set_independent ( WS, Cfg ) )
    {
        return -1;
    }
    return main_loop ( WS, Cfg );
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

/*                      Запись pid-файла                              */
int create_pid_file ( pid_t pid )
{
    FILE *pidFile = fopen ( ASKUE_FILE_PID, "w" );
    
    char Buffer[ 256 ];
    if ( pidFile == NULL )
    {
        snprintf ( Buffer, 256, "Ошибка создания pid-файла: %s ( %d )", strerror ( errno ), errno );
        write_msg ( stdout, "АСКУЭ", "FAIL", Buffer );
        return -1;
    }
    
    fprintf ( pidFile, "%ld\n", ( long int ) pid );
    
    fclose ( pidFile );
    
    snprintf ( Buffer, 256, "Старт. Процесс с pid = %ld", ( long int ) pid );
    write_msg ( stdout, "АСКУЭ", "OK", Buffer );
    
    return 0;
}

/*                      Точка входа в программу                       */
int main ( int argc, char **argv )
{
    
    askue_cfg_t Cfg;
    askue_config_init ( &Cfg );
    
    text_buffer_t Buffer;
    text_buffer_init ( &Buffer, 512 );
    
    sigset_t SignalSet;
    signal_set_init ( &SignalSet );
    
    script_argument_vector_t SArgV;
    script_argument_init ( &SArgV, NULL, SA_PRESET_CLEAR );
    
    askue_workspace_t WS;
    WS.Log = NULL;
    WS.SignalSet = &SignalSet;
    WS.ScriptArgV = &SArgV;
    WS.Loop = LoopOk;
    WS.Buffer = &Buffer;
 
    cli_option_t vOption[] =
    {
        { "verbose", 'v', CLI_NO_ARG, __cli_verbose, &( Cfg.Flag ), NULL },
        { "cycle", 'c', CLI_NO_ARG, __cli_cycle, &( Cfg.Flag ), NULL },
        { "protocol", 'p', CLI_NO_ARG, __cli_protocol, &( Cfg.Flag ), NULL },
        CLI_LAST_OPTION
    };
    
    if ( !askue_cli_parse ( vOption, argc, argv ) &&
         !askue_configure ( &WS, &Cfg ) )
    {
        char Buffer[ 256 ];
        pid_t pid = fork ();
        if ( pid < 0 )
        {
            text_buffer_write ( WS.Buffer, "pid() = %s ( %d )", strerror ( errno ), errno );
            verbose_msg ( Cfg.Flag, stdout, "АСКУЭ", "FAIL", WS.Buffer->Text );
            exit ( EXIT_FAILURE ); 
        }
        else if ( !pid )
        {
            int exit_status = run_askue ( &WS, &Cfg );
            
            askue_config_destroy ( &Cfg );
            

            if ( unlink ( ASKUE_FILE_PID ) == -1 )
            {
                text_buffer_write ( WS.Buffer, "Ошибка удаления pid-файла: %s ( %d )", strerror ( errno ), errno );
                write_msg ( WS.Log, "АСКУЭ", "FAIL", WS.Buffer->Text );
            }
            
            text_buffer_destroy ( WS.Buffer );
            askue_log_close ( &( WS.Log ) );
            
            exit ( exit_status );
        }
        else
        {
            create_pid_file ( pid );
            
            exit ( EXIT_SUCCESS );
        }
        
    }
    else
    {
        exit ( EXIT_FAILURE );
    }
}

