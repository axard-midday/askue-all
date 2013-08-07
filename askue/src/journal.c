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
        
        return -1;
    }
    else
    {
        return 0;
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
            return 0;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        if ( !sqlite3_exec_simple_decore ( DB, TblReq ) &&
             !sqlite3_exec_simple_decore ( DB, IdReq ) )
        {
            return 0;
        }
        else
        {
            return -1;
        }
    }
}

#define SQL_INS_LOG_TBL \
"INSERT INTO log_tbl ( date, time, msg ) VALUES ( ( SELECT DATE ( 'now' ) ), ( SELECT TIME ( 'now' ) ), '%s' );"
#define SQL_CUT_LOG_TBL \
"DELETE FROM log_tbl WHERE date < ( SELECT DATE ( 'NOW', '-%u day' ) );"

// настройка таблицы лога
static
int init_log_tbl ( sqlite3 *DB, void *arg )
{
    askue_cfg_t *ACfg = ( askue_cfg_t* ) arg;
    char *Sql = sqlite3_mprintf ( SQL_CUT_LOG_TBL, ACfg->Journal->Size );
    int ExecStatus;
    if ( Sql != NULL )
    {
        ExecStatus = sqlite3_exec_simple_decore ( DB, Sql );
        sqlite3_free ( Sql );
    }
    else
    {
        write_msg ( stderr, "Журнал", "FAIL", "Таблица лога: ошибка sqlite3_mprintf()" );
        return -1;
    }
    
    if ( ExecStatus ) return ExecStatus;
    
    const char *Msg = "Запуск АСКУЭ";
    Sql = sqlite3_mprintf ( SQL_INS_LOG_TBL, Msg );
    if ( Sql != NULL )
    {
        ExecStatus = sqlite3_exec_simple_decore ( DB, Sql );
        sqlite3_free ( Sql );
    }
    else
    {
        write_msg ( stderr, "Журнал", "FAIL", "Таблица лога: ошибка sqlite3_mprintf()" );
        return -1;
    }
    
    return ExecStatus;
}
#undef SQL_INS_LOG_TBL
#undef SQL_CUT_LOG_TBL


#define SQL_INS_CNT_TBL \
"INSERT INTO cnt_tbl ( cnt, type ) VALUES ( %u, '%s' );"
// настройка таблицы счётчиков
static
int init_cnt_tbl ( sqlite3 *DB, void *arg )
{
    askue_cfg_t *ACfg = ( askue_cfg_t* ) arg;
    device_cfg_t **Device = ACfg->DeviceList;
    
    if ( !sqlite3_exec_simple_decore ( DB, "BEGIN TRANSACTION;" ) )
    {
        int Result = 0;
        for ( int i = 0; Device[ i ] != NULL && !Result; i++ )
        {
            if ( Device[ i ]->Class == Askue_Counter )
            {
                char *Sql = sqlite3_mprintf ( SQL_INS_CNT_TBL, (size_t) strtol ( Device[ i ]->Name, NULL, 10 ), Device[ i ]->Type->Name );
                if ( Sql == NULL )
                {
                    Result = -1;
                }
                else
                {
                    Result = sqlite3_exec_simple_decore ( DB, Sql );
                }
            }
        }
        
        Result = sqlite3_exec_simple_decore ( DB, "END TRANSACTION;" );
        
        return Result;
    }
    else
    {
        return -1;
    }
}
#undef SQL_INS_CNT_TBL

/*
 * Запросы на создание таблиц
 */
// таблица показаний
#define SQL_CRT_REG_TBL "CREATE TABLE IF NOT EXISTS reg_tbl ( cnt integer, value integer, type text, date text, time text );"
// таблица счётчиков
#define SQL_CRT_CNT_TBL "CREATE TABLE IF NOT EXISTS cnt_tbl ( cnt integer, type text );"
// таблица лога событий
#define SQL_CRT_LOG_TBL "CREATE TABLE IF NOT EXISTS log_tbl ( date text, time text, msg text );"

/*
 * Запросы на создание индексов
 */
// индекс таблицы показаний
#define SQL_CRT_REG_ID "CREATE UNIQUE INDEX IF NOT EXISTS reg_id ON reg_tbl ( cnt, type, date, time );"
// индекс таблицы счётчиков
#define SQL_CRT_CNT_ID "CREATE UNIQUE INDEX IF NOT EXISTS cnt_id ON cnt_tbl ( cnt, type );"
// индекс для таблица лога событий
#define SQL_CRT_LOG_ID "CREATE UNIQUE INDEX IF NOT EXISTS log_id ON log_tbl ( date, time, msg );"

/*                Точка первоначальной настройки базы                 */
int askue_journal_init ( askue_cfg_t *ACfg )
{
    sqlite3 *DB;
    if ( sqlite3_open ( ACfg->Journal->File, &DB ) != SQLITE_OK )
    {
        char Buffer[ 256 ];
        snprintf ( Buffer, 256, "Попытка открытия: %s", sqlite3_errmsg ( DB ) );
        write_msg ( stderr, "Журнал", "FAIL", Buffer );
        sqlite3_close ( DB );
        return -1;
    }
    
    int Result;
    if ( !__init_tbl ( DB, SQL_CRT_REG_TBL, SQL_CRT_REG_ID, NULL, NULL ) &&
         !__init_tbl ( DB, SQL_CRT_LOG_TBL, SQL_CRT_LOG_ID, init_log_tbl, ACfg ) &&
         !__init_tbl ( DB, SQL_CRT_CNT_TBL, SQL_CRT_CNT_ID, init_cnt_tbl, ACfg ) )
    {
        verbose_msg ( ACfg->Flag, stdout, "Журнал", "OK", "Инициализация успешно завершена." );
        Result = 0;
    }
    else
    {
        verbose_msg ( ACfg->Flag, stdout, "Журнал", "FAIL", "Инициализация остановлена в связи с ошибкой." );
        Result = -1;
    }
    
    sqlite3_close ( DB );
    return Result;
}

#undef SQL_CRT_CNT_ID
#undef SQL_CRT_LOG_ID
#undef SQL_CRT_REG_ID

#undef SQL_CRT_CNT_TBL
#undef SQL_CRT_LOG_TBL
#undef SQL_CRT_REG_TBL
