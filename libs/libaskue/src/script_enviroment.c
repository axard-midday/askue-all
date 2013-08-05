#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cli.h"
#include "macro.h"
#include "port.h"
#include "my.h"
#include "sqlite3.h"
#include "script_enviroment.h"

/*          ***             */
typedef struct _port_arg_t
{
    char *File;
    char *Speed;
    char *DBits;
    char *SBits;
    char *Parity;
} port_arg_t;

typedef struct _device_arg_t
{
    char *Name;
    char *Timeout;
    char *Parametr;
} device_arg_t;

typedef struct _journal_arg_t
{
    char *File;
    char *Flashback;
} journal_arg_t;

typedef struct _log_arg_t
{
    char *File;
    // char *Mode; == 'a' ВСЕГДА
} log_arg_t;

typedef struct _script_arg_t
{
    port_arg_t Port;
    journal_arg_t Journal;
    log_arg_t Log;
    device_arg_t Device;
} script_arg_t;
/*          ***             */

/*  обработчики опций   */
// копирование указателя на аргумент
static
int __hndl_1 ( void *ptr, int *flag, const char *arg )
{
    *( const char** )ptr = arg;
    return 0;
}

// установка флага ведения протокола
static
int __hndl_protocol ( void *ptr, int *flag, const char *arg )
{
    SETBIT ( ( *( uint32_t* ) ptr ), ASKUE_FLAG_PROTOCOL );
    return 0;
}

// установка флага "многословности"
static
int __hndl_verbose ( void *ptr, int *flag, const char *arg )
{
    SETBIT ( ( *( uint32_t* ) ptr ), ASKUE_FLAG_VERBOSE );
    return 0;
}

// прединициализация окружения
static 
void __first_init ( script_enviroment_t *Env )
{
    Env->Port = mymalloc ( sizeof ( askue_port_t ) );
    Env->Port->RS232 = -1;
    Env->Port->In = NULL;
    Env->Port->Out = NULL;
    Env->Log = NULL;
    Env->Journal = NULL;
    Env->Parametr = NULL;
    Env->Device = 0;
    Env->Timeout = 0;
    Env->Flashback = 0;
    Env->Flashback = 0;
}

// инициализировать набор аргументов
static 
int __arg_init ( script_arg_t *Arg, script_enviroment_t *Env, int argc, char **argv )
{
    cli_option_t CliOpt[] =
    {
        { "port_file", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Port.File), NULL },
        { "port_dbits", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Port.DBits), NULL },
        { "port_sbits", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Port.SBits), NULL },
        { "port_parity", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Port.Parity), NULL },
        { "port_speed", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Port.Speed), NULL },
        { "device", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Device.Name), NULL },
        { "timeout", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Device.Timeout), NULL },
        { "parametr", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Device.Parametr), NULL },
        { "log_file", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Log.File), NULL },
        { "journal_file", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Journal.File), NULL },
        { "journal_flashback", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Journal.Flashback), NULL },
        { "protocol", 0, CLI_REQUIRED_ARG, __hndl_protocol, &(Env->Flag), NULL },
        { "verbose", 0, CLI_REQUIRED_ARG, __hndl_verbose, &(Env->Flag), NULL },
        CLI_LAST_OPTION
    };
    
    if ( cli_parse ( CliOpt, argc, argv ) != CLI_SUCCESS )
        return -1;
    else
        return 0;
}

// инициализировать порт
static
int __port_init ( script_enviroment_t *Env, const script_arg_t *Arg )
{
    // настроить порт
    askue_port_cfg_t PortCfg;
    PortCfg.File = Arg->Port.File;
    PortCfg.Parity = Arg->Port.Parity;
    PortCfg.Speed = Arg->Port.Speed;
    PortCfg.DBits = Arg->Port.DBits;
    PortCfg.SBits = Arg->Port.SBits;
    if ( port_init ( Env->Port, &PortCfg ) == -1 )
    {
        //myfree ( Env->Port );
        //Env->Port = NULL;
        return -1;
    }
    else
    {
        return 0;
    }
}

// инициализировать лог
static
int __log_init ( script_enviroment_t *Env, const script_arg_t *Arg )
{
    Env->Log = fopen ( Arg->Log.File, "a" );
    if ( Env->Log == NULL )
    {
        //port_destroy ( Env->Port );
        //myfree ( Env->Port );
        //Env->Port = NULL;
        return -1;
    }
    else
    {
        return 0;
    }
}

// инициализировать журнал
static
int __journal_init ( script_enviroment_t *Env, const script_arg_t *Arg )
{
    if ( sqlite3_open ( Arg->Journal.File, &(Env->Journal) ) != SQLITE_OK )
    {
        //port_destroy ( Env->Port );
        //myfree ( Env->Port );
        //Env->Port = NULL;
        return -1;
    }
    else
    {
        return 0;
    }
}

static
void __integer_params_init ( script_enviroment_t *Env, const script_arg_t *Arg )
{
    // получить номер устройства
    Env->Device = ( uint32_t ) strtoul ( Arg->Device.Name, NULL, 10 );
    
    // получить таймаут
    Env->Timeout = ( uint32_t ) strtoul ( Arg->Device.Timeout, NULL, 10 );
    
    // получить кол-во флешбеков
    Env->Flashback = ( uint32_t ) strtoul ( Arg->Journal.Flashback, NULL, 10 );
}

// инициализация окружения
int script_env_init ( script_enviroment_t *Env, int argc, char **argv, void* ( get_param ) ( const char * ) )
{
    // подготовка окружения
    __first_init ( Env );
    
    // разбор опций на аргументы
    script_arg_t Arg;
    
    if ( __arg_init ( &Arg, Env, argc, argv ) )
        return -1;
        
    // настроить порт
    if ( __port_init ( Env, &Arg ) )
        return -1;
    
    // настроить лог
    if ( __log_init ( Env, &Arg ) )
        return -1;
    
    // открыть базу данных
    if ( __journal_init ( Env, &Arg ) )
        return -1;
        
    __integer_params_init ( Env, &Arg );
    
    // получить параметр
    if ( get_param != NULL )
        Env->Parametr = get_param ( Arg.Device.Parametr );
    
    return 0;
}

// удаление окружения
void script_env_destroy ( script_enviroment_t *Env )
{
    if ( Env->Journal != NULL )
        sqlite3_close ( Env->Journal );
        
    if ( Env->Log != NULL )
        fclose ( Env->Log );
        
    if ( Env->Port != NULL )
    {
        port_destroy ( Env->Port );
        myfree ( Env->Port );
    }
    
    myfree ( Env->Parametr );
}















