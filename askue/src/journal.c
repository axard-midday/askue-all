#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <libaskue.h>

#include "config.h"



// функция инициализации
typedef int ( *__init_func_f ) ( sqlite3*, void* );

// обёртка для запроса к базе без выполнения обработчика результатов
static
int sqlite3_exec_simple_decore ( sqlite3 *DB, const char *Request )
{
    char *ErrorStr;
    int st = sqlite3_exec ( DB, Request, NULL, NULL, &ErrorStr );
    
    if ( st != SQLITE_OK && st != SQLITE_CONSTRAINT )
    {
        write_msg ( stderr, "Журнал", "FAIL", ErrorStr );
        write_msg ( stderr, "-||-", "FAIL", "В запросе:" );
        write_msg ( stderr, "-||-", "FAIL", Request );
        
        sqlite3_free ( ErrorStr );
        
        return ASKUE_ERROR;
    }
    else
    {
        return ASKUE_SUCCESS;
    }
}

// создать таблицу если отсутствует
static 
int __init_tbl ( sqlite3 *DB, const char *TblReq, const char *IdReq, __init_func_f init_func, void *init_arg )
{
    // таблица
    // Индекс
    // Инициализация ( если есть )
    
    if ( init_func != NULL )
    {
        if ( !sqlite3_exec_simple_decore ( DB, TblReq ) &&
             !sqlite3_exec_simple_decore ( DB, IdReq ) &&
             !init_func ( DB, init_arg ) )
        {
            return ASKUE_SUCCESS;
        }
        else
        {
            return ASKUE_ERROR;
        }
    }
    else
    {
        if ( !sqlite3_exec_simple_decore ( DB, TblReq ) &&
             !sqlite3_exec_simple_decore ( DB, IdReq ) )
        {
            return ASKUE_SUCCESS;
        }
        else
        {
            return ASKUE_ERROR;
        }
    }
}

/*
 * Запросы на создание таблиц
 */
// таблица показаний
#define SQL_MAKE_REG_TBL "CREATE TABLE IF NOT EXISTS reg_tbl ( cnt integer, value integer, type text, date text, time text );"

/*
 * Запросы на создание индексов
 */
// индекс таблицы показаний
#define SQL_MAKE_REG_ID "CREATE UNIQUE INDEX IF NOT EXISTS reg_id ON reg_tbl ( cnt, type, date, time );"

/*                Точка первоначальной настройки базы                 */
int askue_journal_init ( askue_cfg_t *ACfg, FILE *Log, int Verbose )
{
    sqlite3 *DB;
    if ( sqlite3_open ( ACfg->Journal->File, &DB ) != SQLITE_OK )
    {
        char Buffer[ _ASKUE_TBUFLEN ];
        snprintf ( Buffer, 256, "Попытка открытия: %s", sqlite3_errmsg ( DB ) );
        write_msg ( Log, "Журнал", "FAIL", Buffer );
        sqlite3_close ( DB );
        return ASKUE_ERROR;
    }
    
    int Result;
    if ( !__init_tbl ( DB, SQL_MAKE_REG_TBL, SQL_MAKE_REG_ID, NULL, NULL ) )
    {
        if ( Verbose )
            write_msg ( Log, "Журнал", "OK", "Инициализация успешно завершена." );
        Result = ASKUE_SUCCESS;
    }
    else
    {
        if ( Verbose )
            write_msg ( Log, "Журнал", "FAIL", "Инициализация остановлена в связи с ошибкой." );
        Result = ASKUE_ERROR;
    }
    
    sqlite3_close ( DB );
    return Result;
}

#undef SQL_MAKE_CNT_ID

#undef SQL_MAKE_CNT_TBL

#define SQL_STIFLE_JOURNAL "DELETE FROM reg_tbl WHERE date < ( SELECT DATE ( 'now', '-%u day' ) );"

int askue_journal_stifle ( journal_cfg_t *JCfg, FILE *Log, int Verbose )
{
    // текстовый буфер
    char Buffer[ _ASKUE_TBUFLEN ];
    // база данных
    sqlite3 *DB;
    // открыть
    if ( sqlite3_open ( ACfg->Journal->File, &DB ) != SQLITE_OK )
    {
        // сообщение об ошибке
        snprintf ( Buffer, 256, "Попытка открытия: %s", sqlite3_errmsg ( DB ) );
        write_msg ( Log, "Журнал", "FAIL", Buffer );
        // закрыть базу
        sqlite3_close ( DB );
        // ошибка
        return ASKUE_ERROR;
    }
    // ошибка
    int Result = 0;
    // сформировать запрос
    if ( snprintf ( Buffer, _ASKUE_TBUFLEN, SQL_STIFLE_JOURNAL, JCfg->Size ) > 0 )
    {
        // выполнить запрос
        Result = sqlite3_exec_simple_decore ( DB, SQL_STIFLE_JOURNAL );
    }
    else
    {
        write_msg ( Log, "Журнал", "FAIL", "Ошибка snprintf()." );
        Result = ASKUE_ERROR;
    }
    // закрыть базу
    sqlite3_close ( DB );
    // доп. сообщение
    if ( Verbose && ( Result == ASKUE_SUCCESS ) )
        write_msg ( Log, "Журнал", "OK", "Журнал успешно сжат." );
    // результат
    return Result;
}

#undef SQL_STIFLE_JOURNAL
