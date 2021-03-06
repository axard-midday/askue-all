#ifndef _POSIX_SOURCE
    #define _POSIX_SOURCE
#endif

#ifndef _POSIX_C_SOURCE
    #define _POSIX_C_SOURCE 201309L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <libaskue/write_msg.h>
#include <libaskue/macro.h>

#include "config.h"
#include "log.h"
#include "journal.h"
#include "tbuffer.h"
#include "ecode.h"

// число аргументов для скрипта
#ifndef ASKUE_SCRIPT_ARGV_SIZE
    #define ASKUE_SCRIPT_ARGV_SIZE 15
#endif

/* Типы данных */

typedef struct script_argv_s
{
    const char *Name;
    const char *Port_File;
    const char *Port_Speed;
    const char *Port_DBits;
    const char *Port_SBits;
    const char *Port_Parity;
    const char *Device;
    const char *Parametr;
    const char *Argument;
    long int *Timeout;
    const char *Journal_File;
    size_t Journal_Flashback;
    const char *Log_File;
    int Verbose;
    int Protocol;
} script_argv_t;

/* Глобальные переменные */
static FILE                    *_gLog_;
static askue_cfg_t             *_gCfg_;
script_argv_t                  *_gArgv_;
sigset_t                       *_gSignalSet_;
//char                            _gBuffer_[ _ASKUE_TBUFLEN ];

/* Функции */

/* установка строковых параметров */
int set_strparam ( char *arg, const char *tmpl, const char *Param, int *i )
{
    if ( Param )
    {
        if ( snprintf ( arg, _ASKUE_TBUFLEN, tmpl, Param ) < 0 )
        {
             return ASKUE_ERROR;
        }
        else
        {
            ( *i ) ++; // сдвиг на следующую ячейку памяти
            return ASKUE_SUCCESS;
        }
    }
    else
    {
         
        return ASKUE_SUCCESS;
    }
}

/* установка параметров long int */
int set_lintparam ( char *arg, const char *tmpl, long int Param, int *i )
{
    if ( snprintf ( arg, _ASKUE_TBUFLEN, tmpl, Param ) < 0 )
    {
         return ASKUE_ERROR;
    }
    else
    {
        ( *i ) ++; // сдвиг на следующую ячейку памяти
        return ASKUE_SUCCESS;
    }
}

/* установка аргумента отвечающего за отображение доп. сообщений */
int set_verbose ( char *arg, int Verbose, int *i )
{
    if ( Verbose )
    {
        if ( snprintf ( arg, _ASKUE_TBUFLEN, "%s", "--verbose" ) < 0 )
        {
            return ASKUE_ERROR;
        }
        else
        {
            ( *i ) ++; // сдвиг на следующую ячейку памяти
            return ASKUE_SUCCESS;
        }
    }
    else
    {
        return ASKUE_SUCCESS;
    }
}

/* установка аргумента отвечающего за отображение протокола */
int set_protocol ( char *arg, int Protocol, int *i )
{
    if ( Protocol )
    {
        if ( snprintf ( arg, _ASKUE_TBUFLEN, "%s", "--protocol" ) < 0 )
        {
            return ASKUE_ERROR;
        }
        else
        {
            ( *i ) ++; // сдвиг на следующую ячейку памяти
            return ASKUE_SUCCESS;
        }
    }
    else
    {
        return ASKUE_SUCCESS;
    }
}


/* проверить результат выполнения скрипта */
static
int wait_script_result ( const char *Name, pid_t pid )
{
    int status;
    pid_t WaitpidReturn = waitpid ( pid, &status, WNOHANG );
    if ( WaitpidReturn == -1 )
    {
        if ( snprintf( _gBuffer_, _ASKUE_TBUFLEN, "waitpid(): %s (%d)", strerror ( errno ), errno ) > 0 )
            write_msg ( _gLog_, "АСКУЭ", "FAIL", _gBuffer_ );
        return ASKUE_ERROR;
    }
    else if ( WaitpidReturn == 0 )
    {
        write_msg ( _gLog_, "АСКУЭ", "FAIL", "Ложный сигнал SIGCHLD" );
        return ASKUE_SUCCESS;
    }
    
    if ( WIFEXITED ( status ) ) // успешное завершение, т.е. через exit или return
    {
        int code = WEXITSTATUS ( status );
        if ( code != EXIT_SUCCESS )
        {
            if ( snprintf ( _gBuffer_, _ASKUE_TBUFLEN, "Скрипт '%s' завершён с кодом: %d", Name, code ) > 0 )
                write_msg ( _gLog_, "АСКУЭ", "ERROR", _gBuffer_ );
        }
    }
    else if ( WIFSIGNALED ( status ) ) // завершение по внешнему сигналу
    {
        if ( snprintf ( _gBuffer_, _ASKUE_TBUFLEN, "Скрипт '%s' завершён по сигналу: %d", Name, WTERMSIG ( status ) ) > 0 )
            write_msg ( _gLog_, "АСКУЭ", "ERROR", _gBuffer_ );
    }
    
    return ASKUE_SUCCESS;
}

/* Ожидание сигналов извне или от дочернего процесса */
int wait_signal ( const char *Name, pid_t pid, int Verbose )
{
    siginfo_t SignalInfo;
    if ( sigwaitinfo ( _gSignalSet_, &SignalInfo ) == -1 ) // ожидание сигнала
    {
        // сообщение об ошибке
        return ASKUE_ERROR;
    }
    else
    {
        int Result;
        switch ( SignalInfo.si_signo ) // перебор возможных сигналов
        {
            case SIGCHLD: // завершить потомка
            
                Result = wait_script_result ( Name, pid );
                break;
                
            case SIGUSR1: // прочитать заново конфигурацию
            
                // сигнал завершения потомку
                if ( kill ( pid, SIGUSR2 ) )
                    write_msg ( _gLog_, "Сигнал", "ERROR", "kill()" );
                // Доп. сообщение
                if ( Verbose )
                    write_msg ( _gLog_, "Сигнал", "OK", "Выполнить переконфигурацию" );
                // завершение потомка
                Result = wait_script_result ( Name, pid );
                break;
                
            default:
            
                // сигнал завершения потомку
                if ( kill ( pid, SIGUSR2 ) )
                    write_msg ( _gLog_, "Сигнал", "ERROR", "kill()" );
                // Доп. сообщение
                if ( Verbose )
                    write_msg ( _gLog_, "Сигнал", "OK", "Завершить работу АСКУЭ" );
                Result = ASKUE_EXIT;
                break;
        }
        return Result;
    }
}

/* выполнения скрипта с определёнными аргументами */
int exec_script ( const script_argv_t *Argv )
{
    // список параметров
    static char arg[ ASKUE_SCRIPT_ARGV_SIZE ][ _ASKUE_TBUFLEN ];
    
    // запись значения аргументов
    int i = 0;
    int Result = ( set_strparam ( arg[ i ], "%s", Argv->Name, &i ) ) ?:
                 ( set_strparam ( arg[ i ], "--port_file=%s", Argv->Port_File, &i ) ) ?: 
                 ( set_strparam ( arg[ i ], "--port_speed=%s", Argv->Port_Speed, &i ) ) ?:
                 ( set_strparam ( arg[ i ], "--port_dbits=%s", Argv->Port_DBits, &i ) ) ?:
                 ( set_strparam ( arg[ i ], "--port_sbits=%s", Argv->Port_SBits, &i ) ) ?:
                 ( set_strparam ( arg[ i ], "--port_parity=%s", Argv->Port_Parity, &i ) ) ?:
                 ( set_strparam ( arg[ i ], "--device=%s", Argv->Device, &i ) ) ?:
                 ( set_strparam ( arg[ i ], "--parametr=%s", Argv->Parametr, &i ) ) ?:
                 ( set_strparam ( arg[ i ], "--argument=%s", Argv->Argument, &i ) ) ?:
                 ( set_lintparam ( arg[ i ], "--timeout=%ld", Argv->Timeout, &i ) ) ?:
                 ( set_strparam ( arg[ i ], "--journal_file=%s", Argv->Journal_File, &i ) ) ?:
                 ( set_strparam ( arg[ i ], "--journal_flashback=%u", Argv->Journal_Flashback, &i ) ) ?:
                 ( set_strparam ( arg[ i ], "--log_file=%s", Argv->Log_File, &i ) ) ?:
                 ( set_protocol ( arg[ i ], Argv->Protocol, &i ) ) ?:
                 ( set_verbose ( arg[ i ], Argv->Verbose, &i ) ) ?: ASKUE_SUCCESS;
    
    // null-оканчивающийся массив со значениями аргументов             
    const char *_arg[ i + 1 ] = { [ 0 ... i ] = NULL };
    // создание ссылок на память
    for ( int j = 0; j < i; j++ ) _arg[ j ] = arg[ j ];
    
    // выполнить скрипт
    if ( execvp ( _arg[ 0 ], ( char * const * ) _arg ) )
    {
        // ошибка
        if ( snprintf ( _gBuffer_, _ASKUE_TBUFLEN, "Ошибка exec_script().execvp(): %s ( %d ).", strerror ( errno ), errno ) > 0 )
            write_msg ( _gLog_, "АСКУЭ", "FAIL", _gBuffer_ );
        return ASKUE_ERROR;
    }
}

/* запуск скрипта в отдельном процессе */
int run_script ( pid_t *ScriptPid, const script_argv_t *Argv )
{
    char Buffer[ _ASKUE_TBUFLEN ];
    // pid ещё понадобится
    ( *ScriptPid ) = fork ();  // запуск отдельного процесса
    if ( ScriptPid < 0 )
    {
        // текст ошибки
        if ( snprintf ( Buffer, _ASKUE_TBUFLEN, "Ошибка run_script().fork():", strerror ( errrno ), errno ) > 0 )
            write_msg ( _gLog_, "АСКУЭ", "FAIL", Buffer );
        // ошибка
        return ASKUE_ERROR;
    }
    else if ( ScriptPid == 0 )
    {
        return exec_script ( Argv ); // запуск скрипта
    }
    else
    {
        return ASKUE_SUCCESS;
    }
}

// символы равны
int is_eqch ( int _1, int _2 )
{
    return _1 == _2;
}

// это конец строки
int is_endstr ( int ch )
{
    return ch == '\0';
}

// сравнить две строки
// если их длины не равны, то они получатся не равными
int is_eqstr ( const char *s1, const char *s2 )
{
    int R, End;
    do {
        End = is_endstr ( *s1 ) || is_endstr ( *s2 ); // конец строки
        R = is_eqch ( *s1, *s2 );   // сравнить два символа
    } while ( R && !End );
    
    return R;
}



/* следующее устройство соответствующее цели */
const device_cfg_t* next_target_device ( const device_cfg_t *Device, const char *Target )
{
    // результат
    const device_cfg_t *Result = NULL;
    // перерор до конца
    for ( const device_cfg_t *TheDevice = Device; !is_last_device ( TheDevice ) && ( Result == NULL ); TheDevice ++ )
    {
        // сравнить строку с название типа устройства и целью задачи
        if ( is_eqstr ( TheDevice->Type, Target ) )
        {
            // нашли результат
            Result = TheDevice;
        }
    }
    // вернуть результат
    return Result;
}

/* найти коммуникацию по индексу */
const comm_cfg_t* find_comm ( const comm_cfg_t *Comm, int Id )
{
    const comm_cfg_t *Result = NULL;
    for ( const comm_cfg_t *TheComm = Comm; !is_last_comm ( TheComm ) && ( Result == NULL ); TheComm ++ )
    {
        if ( Comm->Device->Id == Id ) // проверка по индексу
        {
            Result = Comm;  // нашли результат
        }
    }
    return Result;
}



/* найти устройство по индексу */
const device_cfg_t* find_device ( const device_cfg_t *Device, int Id )
{
    const device_cfg_t *Result = NULL;
    for ( const device_cfg_t *TheDevice = Device; !is_last_device ( TheDevice ) && ( Result == NULL ); TheDevice ++ )
    {
        if ( Device->Id == Id ) // проверить по индексу
        {
            Result = Device;    // нашли результат
        }
    }
    return Result;
}

/* соединение через модем */
int askue_connect ( const comm_cfg_t *TheCommB, const device_cfg_t *TheCommR, int Verbose )
{
    // аргументы нужные скрипту
    script_argv_t Argv = ( *_gArgv_ );
    Argv.Name = TheCommB->Script->Name;
    Argv.Device = TheCommB->Device->Name;
    Argv.Timeout = TheCommB->Device->Timeout;
    Argv.Parametr = TheCommB->Script->Parametr;
    Argv.Argument = TheCommR->Name;
    // отслеживание ошибки запуска
    pid_t ScriptPid;
    if ( run_script ( &Argv, &ScriptPid ) == ASKUE_ERROR )
    {
        // ошибка
        return -1;
    }
    // ожидание сигнала от потомка и всяких других.
    return wait_signal ( Argv->Name, ScriptPid, Verbose );
}

// сообщить информацию о задаче 
int say_task_info ( const task_cfg_t *Task )
{
    char Buffer[ _ASKUE_TBUFLEN ];
    if ( snprintf ( Buffer, _ASKUE_TBUFLEN, "Обрабатывается задача: '%s'.", Task->Target ) > 0 )
    {
        write_msg ( _gLog_, "АСКУЭ", "INFO", Buffer );
        return ASKUE_SUCCESS;
    }
    else
    {
        return ASKUE_ERROR;
    }
}

// сообщить информацию об устройстве
void say_task_info ( const device_cfg_t *Device )
{
    char Buffer[ _ASKUE_TBUFLEN ];
    if ( snprintf ( Buffer, _ASKUE_TBUFLEN, "Обрабатывается устройство: '%s'.", Device->Name ) > 0 )
    {
        write_msg ( _gLog_, "АСКУЭ", "INFO", Buffer );
        return ASKUE_SUCCESS;
    }
    else
    {
        return ASKUE_ERROR;
    }
}

// сообщить информацию об установке соединения
void say_comm_info ( const device_cfg_t *DeviceB, const device_cfg_t *DeviceR )
{
    char Buffer[ _ASKUE_TBUFLEN ];
    const char *Tmp = "Установка канала связи: '%s: %s' ---> '%s: %s'.";
    if ( snprintf ( Buffer, _ASKUE_TBUFLEN, Tmp, DeviceB->Type, DeviceB->Name, DeviceR->Type, DeviceR->Name ) > 0 )
    {
        write_msg ( _gLog_, "АСКУЭ", "INFO", Buffer );
        return ASKUE_SUCCESS;
    }
    else
    {
        return ASKUE_ERROR;
    }
}


// выполнение скриптов
int do_scripts ( const script_cfg_t *Script, const device_cfg_t *Device )
{
    // аргументы нужные скрипту
    script_argv_t Argv = ( *_gArgv_ );
    // имя устройства и таймаут не изменяться для всего списка устройств
    Argv.Device = Device->Name;
    Argv.Timeout = Device->Timeout;
    int Result = ASKUE_SUCCESS;
    // перебор скриптов
    for ( const script_cfg_t *TheScript = Script; !is_last_script ( TheScript ) && ( Result == ASKUE_SUCCESS ); TheScript++ )
    {
        // pid процесса скрипта
        pid_t ScriptPid;
        // имя скрипта
        Argv.Name = TheScript->Name;
        // Параметр скрипта
        Argv.Parametr = TheScript->Parametr;
        // аргумент скрипта
        Argv.Argument = NULL;
        // запуск скрипта
        if ( run_script ( &ScriptPid, &Argv ) == ASKUE_ERROR )
        {
            Result = ASKUE_ERROR;
            break;
        }
        Result = wait_signal ( TheScript->Name, ScriptPid );
    }
    
    return Result;
}

/* рабочий процесс выполнения задач */
int task_workflow ( const task_cfg_t *Task, 
                     const comm_cfg_t *Comm, 
                     const device_cfg_t *Device,
                     const int *Network,
                     int Verbose )
{
    // результат выполнения
    int Result = ASKUE_SUCCESS;
    // перебор задач
    for ( const task_cfg_t *TheTask = Task; is_last_task ( TheTask ) && ( Result == ASKUE_SUCCESS ); TheTask++ )
    {
        // Доп. сообщение о задаче
        if ( Verbose && ( say_task_info ( TheTask ) == ASKUE_ERROR ) ) return ASKUE_ERROR;
        
        int NoTargetDevice = 1;
        
        for ( const device_cfg_t *TheDevice = next_target_device ( Device, TheTask->Target )
              ( TheDevice != NULL ) && !is_last_device ( TheDevice ) && ( Result == ASKUE_SUCCESS ); 
              TheDevice = next_target_device ( TheDevice, TheTask->Target ) )
        {
            // целевые устройства есть
            NoTargetDevice = 0; 
            // Доп. сообщение об устройстве
            if ( Verbose && ( say_device_info ( TheDevice ) == ASKUE_ERROR ) ) return ASKUE_ERROR;
            // если устройство удалено от компьютера
            if ( Network[ TheDevice->Id ] != 0 )
            {
                // найти соотвестствующее средство коммуникаци
                // далее модема
                int CommR = Network[ TheDevice->Id ]; // модем с той стороны
                int CommB = Network[ CommR ]; // модем с этой стороны
                // найти описание удалённого и местного модемов
                const comm_cfg_t *TheCommB = find_comm ( Comm, CommB );
                const device_cfg_t *TheCommR = find_device ( Device, CommR );
                // Доп. сообщение о подключении к устройству
                if ( Verbose && ( say_comm_info ( TheCommB->Device, TheCommR ) == ASKUE_ERROR ) ) return ASKUE_ERROR;
                // установить соединение
                int Connect = askue_connect ( TheCommB, TheCommR );
                // отслеживание ошибок
                if ( Connect == ASKUE_ERROR )
                {
                    Result = ASKUE_ERROR;
                }
                else if ( Connect == 0 ) // отслеживаем отсутствие соединения
                {
                    if ( Verbose ) write_msg ( _gLog_, "АСКУЭ", "INFO", "Ошибка соединения." );
                    break; // к другой задаче
                }
                else if ( Verbose )
                {
                    write_msg ( _gLog_, "АСКУЭ", "INFO", "Соединение установлено." );
                }
            }
            // выполнение скриптов задачи
            Result = ( Result == ASKUE_SUCCESS ) ? do_scripts ( Task->Script, TheDevice ) : ASKUE_ERROR;
        }
        
        if ( NoTargetDevice )
            write_msg ( _gLog_, "АСКУЭ", "ERROR", "Нет целевых устройств." );
    }
    // конец
    return Result;
}

/* произвести полное изменение конфигурации */
int full_reconfigure ( void )
{
    if ( reconfigure () == ASKUE_ERROR ) // прочтение конфига
    {
        return ASKUE_ERROR;
    }
    else if ( reopen_log () == ASKUE_ERROR ) // открыть лог с новыми настройками
    {
        return ASKUE_ERROR;
    }
    else if ( askue_journal_init ( _gCfg_, _gLog_, TESTBIT ( _gCfg_->Flag, ASKUE_FLAG_VERBOSE ) ) == ASKUE_ERROR ) // перенастроить журнал
    {
        return ASKUE_ERROR;
    }
    else
    {
        reinit_script_argv (); // перенастроить аргументы
        return ASKUE_SUCCESS;
    }
}

/* рабочий процесс программы */
int workflow ( void )
{
    int R;
    do {
        // выполнить задачи
        R = task_workflow ( _gCfg_->Task, 
                            _gCfg_->Comm, 
                            _gCfg_->Device, 
                            _gCfg_->Network,
                            TESTBIT ( _gCfg_->Flag, ASKUE_FLAG_VERBOSE ) );
        // отслеживание запрос на переконфигурацию
        if ( R == ASKUE_RECONF ) 
        {
            // выполнить переконфигурацию
            R = full_reconfigure ();
        }
        // отслеживаем ошибку
        if ( R == ASKUE_ERROR )
        {
            // ошибка
            break;
        }
        
        // обрезать лог
        R = askue_log_stifle ( &_gLog_, _gCfg_ );
        if ( R == ASKUE_ERROR )
        {
            // ошибка
            break;
        }
        
        // ужать журнал
        R = askue_journal_stifle ( _gCfg_->Journal, _gLog_, TESTBIT ( _gCfg_->Flag, ASKUE_FLAG_VERBOSE ) );
        if ( R == ASKUE_ERROR )
        {
            // ошибка
            break;
        }
        
    } while ( R == ASKUE_SUCCESS );
    
    return R;
}

/* Отчёт об ошибках возникших в результате
   разбора аргументов командной строки */
static
void say_cli_error ( cli_result_t CliResult )
{
    switch ( CliResult )
    {
        case CLI_ERROR_NOVAL: 
            write_msg ( stderr, "CLI", "FAIL", "Нет требуемого за аргументом значения." );
            break;
        case CLI_ERROR_HANDLER: 
            write_msg ( stderr, "CLI", "FAIL", "Ошибка обработчика аргумента." );
            break;
        case CLI_ERROR_NEEDARG:
            write_msg ( stderr, "CLI", "FAIL", "Ошибка с отсутствием в аргументах программы обязательных аргументов." );
            break;
        case CLI_ERROR_ARGTYPE:
            write_msg ( stderr, "CLI", "FAIL", "Ошибка определения аргумента ( он не начинается с символа '-' или '-' )." );
            break;
        default:
            break;
    }
}

// обработчик устанавливающий флаг вывода дополнительных сообщения
static
int __cli_verbose ( void *ptr, const char *arg )
{
    SETBIT ( ( *( uint32_t* ) ptr ), ASKUE_FLAG_VERBOSE );
    return 0;
}

// обработчик устанавливающий флаг одного прохода
static
int __cli_cycle ( void *ptr, const char *arg )
{
    SETBIT ( ( *( uint32_t* ) ptr ), ASKUE_FLAG_CYCLE );
    return 0;
}

// обработчик устанавливающий флаг вывода протокола
static
int __cli_protocol ( void *ptr, const char *arg )
{
    SETBIT ( ( *( uint32_t* ) ptr ), ASKUE_FLAG_PROTOCOL );
    return 0;
}

/* Разбор аргументов командной строки.
   Будут установлены флаги. */
static
int program_args_to_config ( askue_cfg_t *cfg, int argc, char **argv )
{
    // Возможные аргументы для разбора
    cli_arg_t Args[] =
    {
        { "verbose",  'v', CLI_OPTIONAL_ARG, CLI_NO_VAL, __cli_verbose,  &( cfg->Flag ) },
        { "cycle",    'c', CLI_OPTIONAL_ARG, CLI_NO_VAL, __cli_cycle,    &( cfg->Flag ) },
        { "protocol", 'p', CLI_OPTIONAL_ARG, CLI_NO_VAL, __cli_protocol, &( cfg->Flag ) },
        CLI_LAST_ARG
    };
    // разбор аргументов
    cli_result_t CliResult = cli_parse ( Args, argc, argv );
    if ( CliResult != CLI_SUCCESS )
    {
        say_cli_error ( CliResult ); // сообщение об ошибке
        return ASKUE_ERROR;
    }
    else
    {
        return ASKUE_SUCCESS;
    }
}

/* Открыть лог в первый раз ( при старте программы ) */
static
int open_log ( FILE **pLog, const askue_cfg_t *cfg, int Verbose )
{
    // буфер текстовых сообщений 
    char Buffer[ _ASKUE_TBUFLEN ];
    // открыть лог
    if ( askue_log_open ( pLog, cfg ) == ASKUE_ERROR )
    {
        // сообщение об ошибке
        if ( snprintf ( Buffer, _ASKUE_TBUFLEN, "Ошибка открытия лога: %d - %s", errno, strerror ( errno ) ) > 0 )
            write_msg ( stderr, "Лог", "FAIL", Buffer );
        return ASKUE_ERROR;
    }
    else if ( Verbose ) // вывести на экран дополнительные сообщения
    {
        // сообщение
        if ( snprintf ( Buffer, _ASKUE_TBUFLEN, "Лог открыт по адресу: %s", cfg->Log->File ) > 0 )
        {
            write_msg ( stdout, "Лог", "INFO", Buffer );
            write_msg ( *pLog, "Лог", "INFO", Buffer );
            return ASKUE_SUCCESS;
        }
        else
        {
            return ASKUE_ERROR;
        }
    }
    else
    {
        return ASKUE_SUCCESS;
    }
}



/* Направить стандартные каналы ввода вывода
   в /dev/null */
static
int stdstream_to_null ( FILE *Log )
{
    char Buffer[ _ASKUE_TBUFLEN ];
    if ( freopen ( "/dev/null", "r", stdin ) == NULL )
    {
        if ( snprintf ( Buffer, _ASKUE_TBUFLEN, "Ошибка перенаправления stdin: %d - %s", errno, strerror ( errno ) ) > 0 )
            write_msg ( Log, "Стандартные каналы", "FAIL", Buffer );
        return ASKUE_ERROR;
    }
    else if ( freopen ( "/dev/null", "w", stdout ) == NULL )
    {
        if ( snprintf ( Buffer, _ASKUE_TBUFLEN, "Ошибка перенаправления stdout: %d - %s", errno, strerror ( errno ) ) > 0 )
            write_msg ( Log, "Стандартные каналы", "FAIL", Buffer );
        return ASKUE_ERROR;
    }
    else if ( freopen ( "/dev/null", "w", stderr ) == NULL )
    {
        if ( snprintf ( Buffer, _ASKUE_TBUFLEN, "Ошибка перенаправления stderr: %d - %s", errno, strerror ( errno ) ) > 0 )
            write_msg ( Log, "Стандартные каналы", "FAIL", Buffer );
        return ASKUE_ERROR;
    }
    else
    {
        return ASKUE_SUCCESS;
    }
    
}

/* Отделить от управляющего терминала. 
   И стать самостоятельной программой в фоне.
   Т.е. демоном */
static
int daemonize_askue ( FILE *Log, int Verbose )
{
    // буфер текстовых сообщений
    char Buffer[ _ASKUE_TBUFLEN ];
    // разрешить выставлять все права на файлы
    umask ( 0 );
    // создать новую сессию, если вызывающий процесс
    // не является лидером в группе
    if ( setsid () == -1 )
    {
        if ( snprintf ( Buffer, _ASKUE_TBUFLEN, "setsid(): %s (%d)", strerror ( errno ), errno ) > 0 )
            write_msg ( Log, "АСКУЭ", "FAIL", Buffer );
            
        return ASKUE_ERROR;
    }
    else if ( Verbose )
    {
        write_msg (Log, "АСКУЭ", "INFO", "Новая сессия создан." );
    }
    // сменить рабочий каталог
    if ( chdir ( "/" ) == -1 )
    {
        if ( snprintf ( Buffer, _ASKUE_TBUFLEN, "chdir(): %s (%d)", strerror ( errno ), errno ) > 0 )
            write_msg ( Log, "АСКУЭ", "FAIL", Buffer );

        return ASKUE_ERROR;
    }
    else if ( Verbose )
    {
        write_msg ( Log, "АСКУЭ", "INFO", "Рабочий каталог изменён." );
    }
    
    return ASKUE_SUCCESS;
}

/* Запись pid-файла */
static
int create_pid_file ( pid_t pid )
{
    // буфер текстовых сообщений
    char Buffer[ _ASKUE_TBUFLEN ];
    FILE *pidFile = fopen ( ASKUE_FILE_PID, "w" ); // открыть на запись
    if ( pidFile == NULL )
    {
        // сообщение об ошибке
        if ( snprintf ( Buffer, 256, "Ошибка создания pid-файла: %s ( %d )", strerror ( errno ), errno ) > 0 )
            write_msg ( stdout, "АСКУЭ", "FAIL", Buffer );
        return ASKUE_ERROR;
    }
    // запись значения
    if ( fprintf ( pidFile, "%ld\n", ( long int ) pid ) == -1 )
    {
        fclose ( pidFile );
        return ASKUE_ERROR;
    }
    
    return ASKUE_SUCCESS;
}

/* Удалить pid-файл */
static
void delete_pid_file ( void )
{
    unlink ( ASKUE_FILE_PID );
}

/* Отделить от терминала */
static
int fork_from_terminal ( FILE *Log, int Verbose )
{
    // буфер текстовых сообщений
    char Buffer[ _ASKUE_TBUFLEN ];
    // разделяемся
    pid_t Pid = fork ();
    if ( Pid < 0 )
    {
        // сообщение об ошибке
        if ( snprintf ( Buffer, _ASKUE_TBUFLEN, "fork(): %s (%d)", strerror ( errno ), errno ) > 0 )
            write_msg ( Log, "Запуск АСКУЭ", "FAIL", Buffer );

        return ASKUE_ERROR;
    }
    else if ( Pid > 0 )
    {
        // создать файл содержащий pid
        if ( create_pid_file ( Pid ) == ASKUE_ERROR )
        {
            // убить потомка
            kill ( Pid, SIGKILL );
            return ASKUE_ERROR; // выйти с ошибкой
        }
        else if ( Verbose )
        {
            // дополнительное сообщение
            if ( snprintf ( Buffer, _ASKUE_TBUFLEN, "АСКУЭ запущенс pid = %ld", ( long int ) Pid ) > 0 )
            {
                write_msg ( Log, "Запуск АСКУЭ", "INFO", Buffer );
            }
            else
            {
                // убить потомка
                kill ( Pid, SIGKILL );
                return ASKUE_ERROR;
            }
        }
        
        exit ( EXIT_SUCCESS ); // завершить родительский процесс
    }
    else
    {
        atexit ( delete_pid_file ); // указать на удаление pid-файла при выходе
        return ASKUE_SUCCESS;
    }
}

// Закрыть лог по выходу
static
void close_log ( void )
{
    askue_log_close ( &_gLog_ ); // закрыть лог
}

// удалить конфиг
static
void destroy_config ( void )
{
    askue_config_destroy ( _gCfg_ );
}

// Переконфигурация
static
int reconfigure ( void )
{
    // сохранить флаги
    int FlagStorage = _gCfg_->Flag;
    // удалить старую конфигурацию
    askue_config_destroy ( _gCfg_ );
    // инициализировать память
    askue_config_init ( _gCfg_ );
    // возврат флагов на место
    _gCfg_->Flag = FlagStorage;
    // прочитать файл конфигурации
    return askue_config_read ( _gCfg_, _gLog_ );
}

// переоткрыть лога
static
int reopen_log ( void )
{
    write_msg ( _gLog_, "Лог", "INFO", "Лог будет закрыт." );
    // закрыть старый поток лога
    askue_log_close ( &_gLog_ );
    // открыть новый поток лога
    if ( askue_log_open ( &_gLog_, _gCfg_ ) == ASKUE_ERROR )
    {
        return ASKUE_ERROR;
    }
    else if ( TESTBIT ( _gCfg_->Flag, ASKUE_FLAG_VERBOSE ) ) // вывести в лог дополнительные сообщения
    {
        // сообщение
        if ( snprintf ( _gBuffer_, _ASKUE_TBUFLEN, "Лог открыт по адресу: %s", _gCfg_->Log->File ) > 0 )
        {
            write_msg ( _gLog_, "Лог", "INFO", _gBuffer_ );
            return ASKUE_SUCCESS;
        }
        else
        {
            return ASKUE_ERROR;
        }
    }
    else
    {
        return ASKUE_SUCCESS;
    }
}

// установить значения постоянных аргументов скрипта
static
script_argv_t init_script_argv ( askue_cfg_t *Cfg )
{
    return ( script_argv_t )
    {
        // постоянные
        .Port_File = Cfg->Port->File,
        .Port_DBits = Cfg->Port->DBits,
        .Port_SBits = Cfg->Port->SBits,
        .Port_Parity = Cfg->Port->Parity,
        .Port_Speed = Cfg->Port->Speed,
        .Journal_File = Cfg->Journal->File,
        .Journal_Flashback = Cfg->Journal->Flashback,
        .Log_File = Cfg->Log->File,
        .Verbose = TESTBIT ( Cfg->Flag->ASKUE_FLAG_VERBOSE ),
        .Protocol = TESTBIT ( Cfg->Flag->ASKUE_FLAG_DEBUG ),
        // переменные
        .Timeout = 0,
        .Parametr = NULL,
        .Name = NULL,
        .Argument = NULL,
        .Device = NULL
    };
}

// переинициализация аргументов
void reinit_script_argv ( void )
{
    ( *_gArgv_ ) = init_script_argv ( _gCfg_ );
}

// установка сигналов для отслеживания
int _sigset_init ( sigset_t *SignalSet )
{
    return ( sigemptyset ( SignalSet ) ) ? ASKUE_ERROR :
            ( sigaddset ( SignalSet, SIGCHLD ) ) ? ASKUE_ERROR : // сигнал от потомка
            ( sigaddset ( SignalSet, SIGUSR1 ) ) ? ASKUE_ERROR : // сигнал переконфигурирования
            // Сигналы прерываний
            ( sigaddset ( SignalSet, SIGINT ) ) ? ASKUE_ERROR :
            ( sigaddset ( SignalSet, SIGQUIT ) ) ? ASKUE_ERROR :
            ( sigaddset ( SignalSet, SIGTERM ) ) ? ASKUE_ERROR :
            // Установка сигналов на блокировку
            ( sigprocmask ( SIG_BLOCK, SignalSet, NULL ) ) ? ASKUE_ERROR : ASKUE_SUCCESS;
}

// установка сигналов для отслеживания с ловлей ошибки установки
int sigset_init ( sigset_t *SignalSet )
{
    if ( _sigset_init ( SignalSet ) == ASKUE_ERROR )
    {
        // сообщение об ошибке
        write_msg ( stderr, "Сигналы", "OK", "Ошибка установки маски сигналов." );
        return ASKUE_ERROR;
    }
    else
    {
        return ASKUE_SUCCESS;
    }
}

int main ( int argc, char **argv )
{
    // Выделить память. 
    askue_cfg_t             Cfg;
    sigset_t               SignalSet;

    // Инициализировать память.
    // установить отслеживаемые сигналы
    if ( sigset_init ( &SignalSet ) == ASKUE_ERROR ) exit ( EXIT_FAILURE );
    _gSignalSet_ = &SignalSet; // установить глобальную ссылку на переменную    
    // разбор конфигурации
    askue_config_init ( &Cfg ); // конфигурация
    _gCfg_ = &Cfg; // установить как глобальную    
    
    // указать высвыбождение памяти при выходе
    atexit ( destroy_config ); 
    
    // Разбор аргументов командной строки.
    if ( program_args_to_config ( &Cfg, argc, argv ) == ASKUE_ERROR ) exit ( EXIT_FAILURE );
    
    // Прочитать конфиг
    if ( askue_config_read ( &Cfg, stderr ) == ASKUE_ERROR ) exit ( EXIT_FAILURE );
    
    // настроить журнал 
    if ( askue_journal_init ( Cfg.Journal, stdout, TESTBIT ( Cfg.Flag, ASKUE_FLAG_VERBOSE ) ) == ASKUE_ERROR ) exit( EXIT_FAILURE );
    
    // Открыть лог.   
    FILE *Log;
    if ( open_log ( &Log, Cfg, TESTBIT ( Cfg.Flag, ASKUE_FLAG_VERBOSE ) ) == ASKUE_ERROR ) exit ( EXIT_FAILURE );
    _gLog_ = Log; // установить как глобальную переменную
    atexit ( close_log ); // указать закрытие лога при выходе
    
    // Направить std-каналы на NULL.
    if ( stdstream_to_null ( Log ) == ASKUE_ERROR ) exit ( EXIT_FAILURE );
    
    // Создать основу для аргументов передаваемых скрипту.
    // Многие из них просто не будут изменяться. Такие как лог-файл и т.д.
    script_argv_t Argv = init_script_argv ( &Cfg );
    // установить как глобальную переменную
    _gArgv_ = &Argv;
    
    // Отделить от терминала
    if ( fork_from_terminal ( Log, TESTBIT ( Cfg.Flag, ASKUE_FLAG_VERBOSE ) ) == ASKUE_ERROR ) exit ( EXIT_FAILURE );
    
    // Стать самостоятельной программой в фоне.
    // Т.е. демоном
    if ( daemonize_askue ( Log, TESTBIT ( Cfg.Flag, ASKUE_FLAG_VERBOSE ) ) == ASKUE_ERROR ) exit ( EXIT_FAILURE );
    
    // Запустить рабочий процесс ( Опрос, отчёты, обрезка лога, обрезка журнала пока нет ошибок. )
    // И завершить его
    return ( workflow () == ASKUE_ERROR ) ? EXIT_FAILURE : EXIT_SUCCESS;
}














