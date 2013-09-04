#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <stdint.h>
#include <string.h>

#include "journal.h"
#include "my.h"

// открыть журнал
static
int _jnl_open ( askue_jnl_t *Jnl, const char *Base )
{
    if ( sqlite3_open ( Base, &(Jnl->File) ) != SQLITE_OK )
    {
        snprintf ( Jnl->Error, _ASKUE_TBUFLEN, "Ошибка при открытии журнала: %s.", sqlite3_errmsg ( Jnl->File ) );
        return -1;
    }
    return 0;
}

// закрыть журнал
static
int _jnl_close ( askue_jnl_t *Jnl )
{
    if ( sqlite3_close ( Jnl->File ) == SQLITE_BUSY )
    {
        snprintf ( Jnl->Error, _ASKUE_TBUFLEN, "База данных занята." );
        return -1;
    }
    return 0;
}

// выполнить запрос
static
int _jnl_exec_sql ( struct askue_jnl_s *Jnl, jnl_callback_f *Callback, void *Data )
{
    char *_Error;
    if ( sqlite3_exec ( Jnl->File, Jnl->SQL, Callback, Data, &_Error ) != SQLITE_OK )
    {
        snprintf ( Jnl->Error, _ASKUE_TBUFLEN, "Ошибка при выполнении SQL-запроса: %s.", _Error );
        sqlite3_free ( _Error );
        return -1;
    }
    return 0;
} 

// очистить буфер ошибок и буфер запросов
static
void _jnl_refresh ( struct askue_jnl_s *Jnl )
{
    memset ( Jnl->SQL, '\0', _ASKUE_TBUFLEN );
    memset ( Jnl->Error, '\0', _ASKUE_TBUFLEN );
}

askue_jnl_t askue_jnl_init ( void )
{
    return ( askue_jnl_t ) 
    {
        .File = NULL,
        .Flashback = 0,
        .SQL[ 0 ... _ASKUE_TBUFLEN - 1 ] = '\0',
        .Error[ 0 ... _ASKUE_TBUFLEN - 1 ] = '\0',
        .Open = _jnl_open,
        .Close = _jnl_close,
        .Refresh = _jnl_refresh,
        .ExecSQL = _jnl_exec_sql,
    };
}

















