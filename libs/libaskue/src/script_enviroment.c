#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

#include "cli.h"
#include "macro.h"
#include "port.h"
#include "my.h"
#include "script_enviroment.h"
#include "script_argument.h"


// прединициализация окружения
static 
void __first_init ( script_enviroment_t *Env )
{
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

// инициализировать порт
static
int __port_init ( script_enviroment_t *Env, const script_arg_t *Arg )
{
    // настроить порт
    askue_port_cfg_t PortCfg;
    PortCfg.File = Arg->Port->File;
    PortCfg.Parity = Arg->Port->Parity;
    PortCfg.Speed = Arg->Port->Speed;
    PortCfg.DBits = Arg->Port->DBits;
    PortCfg.SBits = Arg->Port->SBits;
    if ( port_init ( Env->Port, &PortCfg ) == -1 )
    {
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
    Env->Log = fopen ( Arg->Log->File, "a" );
    if ( Env->Log == NULL )
    {
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
    if ( sqlite3_open ( Arg->Journal->File, &(Env->Journal) ) != SQLITE_OK )
    {
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
    Env->Device = ( uint32_t ) strtoul ( Arg->Device->Name, NULL, 10 );
    
    // получить таймаут
    Env->Timeout = ( uint32_t ) strtoul ( Arg->Device->Timeout, NULL, 10 );
    
    // получить кол-во флешбеков
    Env->Flashback = ( uint32_t ) strtoul ( Arg->Journal->Flashback, NULL, 10 );
}

void script_env_new ( script_env_t **Env )
{
    (*Env) = mymalloc ( sizeof ( script_env_t ) );
    (*Env)->Port = mymalloc ( sizeof ( askue_port_t ) );
}

// инициализация окружения
int script_env_init ( script_env_t *Env, const script_arg_t *Arg, void* ( get_param ) ( const char * ) )
{
    // подготовка окружения
    __first_init ( Env );
        
    // настроить порт
    if ( __port_init ( Env, Arg ) )
        return -1;
    
    // настроить лог
    if ( __log_init ( Env, Arg ) )
        return -1;
    
    // открыть базу данных
    if ( __journal_init ( Env, Arg ) )
        return -1;
        
    __integer_params_init ( Env, Arg );
    
    // получить параметр
    if ( get_param != NULL )
        Env->Parametr = get_param ( Arg->Device->Parametr );
    
    return 0;
}

// удаление окружения
void script_env_destroy ( script_env_t *Env, void (*destroy_param) (void*) )
{
    if ( Env->Journal != NULL )
        sqlite3_close ( Env->Journal );
        
    if ( Env->Log != NULL )
        fclose ( Env->Log );
        
    if ( Env->Port != NULL )
        port_destroy ( Env->Port );
    
    if ( destroy_param != NULL ) destroy_param ( Env->Parametr ); 
}

void script_env_delete ( script_env_t *Env )
{
    myfree ( Env->Port );
    myfree( Env );
}













