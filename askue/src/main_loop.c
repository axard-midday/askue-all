#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "main_loop.h"
#include "write_msg.h"
// проверка последнего аргумента в списке

static
int is_last_script_argument ( script_argument_t *Args )
{
    return ( Args->Arg == NULL ) && ( Args->Value == NULL );
}

// начальные установки аргументов скрипта
static
void script_argument_init ( script_argument_t *Args, askue_cfg_t *ACfg )
{
    // настройки порта
    Args[ SCRIPT_ARGUMENT_PORT_FILE ].Value = ACfg->Port->File;
    Args[ SCRIPT_ARGUMENT_PORT_PARITY ].Value = ACfg->Port->Parity;
    Args[ SCRIPT_ARGUMENT_PORT_DBITS ].Value = ACfg->Port->DBits;
    Args[ SCRIPT_ARGUMENT_PORT_SBITS ].Value = ACfg->Port->SBits;
    Args[ SCRIPT_ARGUMENT_PORT_SPEED ].Value = ACfg->Port->Speed;
    
    // настройки журнала
    Args[ SCRIPT_ARGUMENT_JOURNAL_FILE ].Value = ACfg->Journal->File;
    Args[ SCRIPT_ARGUMENT_JOURNAL_FLASHBACK ].Value = ACfg->Journal->Flashback;
}

// Выполнение скрипта
static
void exec_script ( const char *ScriptName, char **Argv )
{
    if ( execvp ( ScriptName, ( char * const []) Argv ) == -1 )
    {
        write_msg ( pLog, "Цикл опроса", "FAIL", "Ошибка execvp" );
        exit ( EXIT_FAILURE );
    }
}

// результат выполнения
static
void get_script_result ( pid_t pid, const char *ScriptName )
{
    int status;
    char Buffer[ 256 ];
    if ( wait ( &status ) != pid )
    {
        snprintf ( Buffer, 256, "wait(): %s (%d)", strerror ( errno ), errno );
        write_msg ( pLog, "Цикл опроса", "ERROR", Buffer );
    }
    
    if ( WIFEXITED ( status ) ) // успешное завершение, т.е. через exit или return
    {
        int code = WEXITSTATUS ( status );
        if ( code != EXIT_SUCCESS )
        {
            snprintf ( Buffer, 256, "Скрипт завершён с кодом: %d", code );
            write_msg ( pLog, "Цикл опроса", "ERROR", Buffer );
        }
    }
    else if ( WIFSIGNALED ( status ) )
    {
        int sig = WTERMSIG ( status );
        snprintf ( Buffer, 256, "Скрипт завершён из-за сигнала: %d", sig );
        write_msg ( pLog, "Цикл опроса", "ERROR", Buffer );
    }
    
}

// Запуск скрипта
static
int run_script ( char *ScriptName, char **Argv )
{
    pid_t pid = fork();
    
    if ( pid == -1 )
    {
        char Buffer[ 256 ];
        snprintf ( Buffer, 256, "fork(): %s (%d)", strerror ( errno ), errno );
        write_msg ( "Цикл опроса", "FAIL", "fork()" );
        return -1;
    }
    else if ( pid == 0 ) // потомок
    {
        exec_script ( ScriptName, Argv );
    }
    else // родитель
    {
        return get_script_result ( pid );
    }
}

// установка аргументов
static
int init_argv ( char **Argv, const script_argument_t *ScriptArgv )
{
    int stat = 0;
    
    for ( size_t i = 0, j = 1 ; is_last_script_argument ( &( ScriptArgv[ i ] ) ) && !stat; i++ )
    {
        if ( ScriptArgv[ i ].Value != NULL )
        {
            stat = asprintf ( &( Argv[ j ], "--%s=%s", ScriptArgv[ i ].Arg, ScriptArgv[ i ].Value );
            if ( stat == -1 )
            {
                write_msg ( pLog, "Цикл опроса", "FAIL", "asprintf()" );
            }
            else
            {
                j++;
            }
        }
    }
    
    return stat;
}

// добавить имя скрипта
static
int reinit_argv ( char **Argv, const script

void destroy_argv ( char **Argv )
{
    for ( int i = 0; i < 11; i++ )
    {
        if ( Argv[ i ] != NULL )
            free ( Argv[ i ] );
    }
}

// опрос устройства
static
int run_inquiry ( const char **ScriptNamev, const script_argument_t *ScriptArgv, FILE *pLog )
{
    char *Argv[ 11 ] = { [ 0 ... 10 ] = NULL };
    if ( init_argv ( Argv, ScriptArgv ) )
    {
        write_msg ( pLog, "Цикл опроса", "FAIL", "init_argv()" );
        return -1;
    }
    
    int Result = 0;
    for ( size_t i = 0; ScriptNamev[ i ] != NULL && !Result; i++ )
    {
        if ( reinit_argv ( Argv, ScriptNamev[ i ] ) )
        {
            write_msg ( pLog, "Цикл опроса", "FAIL", "reinit_argv()" );
            Result = -1;
        }
        else if ( run_script ( ScriptNamev[ i ], Argv, pLog ) )
        {
            write_msg ( pLog, "Цикл опроса", "FAIL", "run_script()" );
            Result = -1;
        }
    }
    
    if ( !Result )
        destroy_argv ( Argv );
    
    return Result;
}


// основной цикл программы
int main_loop ( const askue_cfg_t *ACfg )
{
    device_cfg_t **DeviceList = ACfg->DeviceList;
    device_cfg_t *BaseModem = NULL;
    
    script_argument_t ScriptArg[] =
    {
        { "port_file", NULL },
        { "port_parity", NULL },
        { "port_dbits", NULL },
        { "port_sbits", NULL },
        { "port_speed", NULL },
        { "device", NULL },
        { "parametr", NULL },
        { "timeout", NULL },
        { "journal_file", NULL },
        { "journal_flashback", NULL },
        SCRIPT_ARGUMENT_END
    };
    
    FILE *pLog = fopen ( ACfg->Log->File, "a" );
    if ( pLog == NULL )
    {
        return -1;
    }
    
    
    script_argument_init ( ScriptArg, ACfg );
    
    if ( DeviceList[ 0 ]->Class == Askue_Modem )
    {
        BaseModem = DeviceList[ 0 ]
    }
    
    int IsSuccess = 0;
    while ( !IsSuccess ) 
        for ( size_t i = ( ( BaseModem) ? 1 : 0 ); DeviceList[ i ] != NULL && !IsSuccess; i++ )
        {
            ScriptArg[ SCRIPT_ARGUMENT_DEVICE ] = DeviceList[ i ]->Id;
            char TimeoutString[ 32 ];
            ScriptArg[ SCRIPT_ARGUMENT_TIMEOUT ] = TimeoutString;
            
            if ( snprintf ( TimeoutString, 32, "%ld", DeviceList[ i ]->Timeout ) <= 0 )
            {
                write_msg ( pLog, "Цикл опроса", "FAIL", "snprintf()" );
                IsSuccess = -1;
            }
            else 
            {
                // установка параметра
                if ( DeviceList[ i ]->Class == AskueModem )
                {
                    ScriptArg[ SCRIPT_ARGUMENT_PARAMETR ].Value = BaseModem->Id;
                }
                else
                {
                    ScriptArg[ SCRIPT_ARGUMENT_PARAMETR ].Value = NULL;
                }
                
                IsSuccess = run_inquiry ( DeviceList[ i ]->Type->Script, ScriptArg, pLog );
            }
        }
}
