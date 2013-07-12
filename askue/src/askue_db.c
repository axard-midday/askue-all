#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <libaskue.h>

#include "askue_launcher_macro.h"
#include "askue_launcher_log_error.h"

bool_t sqlite3_exec_decor ( sqlite3 *db, char *sql, int ( *callback ) ( void*, int, char **, char ** ), void *ptr )
{
	char *sqlite3_emsg = NULL;

        int sqlite3_exec_status = sqlite3_exec ( db, sql, callback, ptr, &sqlite3_emsg ); // выполнить запрос

        if ( sqlite3_exec_status != SQLITE_OK && sqlite3_exec_status != SQLITE_CONSTRAINT ) // если обнаружена ошибка
	{
	        error ( 0, 0, "askue: sqlite3_exec_decor(): sqlite3_exec(): %s\n", sqlite3_emsg );
	        			
	        sqlite3_free ( sqlite3_emsg );
	
	        return FALSE;
	}
	else
	{
	
	        return TRUE;
	}
} 

/*
 * Подготовка базы путём добавления запроса в неё
 */
static bool_t db_add_tbl ( sqlite3 *db, const char *_tbl_req, const char *_id_req )
{
        char *sqlite3_emsg = NULL;

        if ( sqlite3_exec_decor ( db, _tbl_req, NULL, NULL ) ) // запрос на создание таблицы
	{
	        return ( _id_req != NULL ) ? sqlite3_exec_decor ( db, _id_req, NULL, NULL ) : FALSE; // запрос на создание индекса
	} 
	else
	{
		return FALSE;
	}
}

/*
 * подготовить таблицу с возможными временами
 */
#define time_tbl_insert "INSERT INTO time_tbl ( time ) VALUES ( ( SELECT TIME ( 'now', 'start of day', '+%u minute' ) ) );"
static int time_tbl_prepare ( sqlite3 *db )
{
        bool_t status = TRUE;

        for ( size_t i = 0; i < 48 && status; i++ ) // перебор всех получасий
        {
        	
                // формирование запроса на добавление времени в таблицу маску
                char *sql = sqlite3_mprintf ( time_tbl_insert, i * 30 );
		
                bool_t status = sqlite3_exec_decor ( db, sql, NULL, NULL ); // выполнить запрос

	        sqlite3_free ( sql );     
	}
	
	return status;
}
#undef time_tbl_insert


/*
 * Запросы на создание таблиц
 */
// таблица показаний
#define askue_sql_CREATE_REG_TBL "CREATE TABLE IF NOT EXISTS reg_tbl ( cnt integer, value integer, type text, date text, time text );"

// таблица счётчиков
#define askue_sql_CREATE_CNT_TBL "CREATE TABLE IF NOT EXISTS cnt_tbl ( cnt integer, type text, timeout integer, modem text );"

// таблица-маска времени
#define askue_sql_CREATE_TIME_TBL "CREATE TABLE IF NOT EXISTS time_tbl ( time text );"

// таблица модемов
#define askue_sql_CREATE_MODEM_TBL "CREATE TABLE IF NOT EXISTS modem_tbl ( modem text, type text, remote_flag integer );"

// таблица лога событий
#define askue_sql_CREATE_LOG_TBL "CREATE TABLE IF NOT EXISTS log_tbl ( date text, time text, msg text );"

// таблица скриптов
#define askue_sql_CREATE_SCRIPT_TBL "CREATE TABLE IF NOT EXISTS script_tbl ( type text, script text, parametr text )"


/*
 * Запросы на создание индексов
 */
// индекс таблицы показаний
#define askue_sql_CREATE_REG_ID "CREATE UNIQUE INDEX IF NOT EXISTS reg_id ON reg_tbl ( cnt, type, date, time );"

// индекс таблицы счётчиков
#define askue_sql_CREATE_CNT_ID "CREATE UNIQUE INDEX IF NOT EXISTS cnt_id ON cnt_tbl ( cnt, modem, type );"

// индекс таблицы-маски времени
#define askue_sql_CREATE_TIME_ID "CREATE UNIQUE INDEX IF NOT EXISTS time_id ON time_tbl ( time );"

// индекс для таблицы модемов
#define askue_sql_CREATE_MODEM_ID "CREATE UNIQUE INDEX IF NOT EXISTS modem_id ON modem_tbl ( modem );"

// индекс для таблица лога событий
#define askue_sql_CREATE_LOG_ID "CREATE UNIQUE INDEX IF NOT EXISTS log_id ON log_tbl ( date, time, msg );"

// индекс таблицы скриптов
#define askue_sql_CREATE_SCRIPT_ID "CREATE UNIQUE INDEX IF NOT EXISTS script_id ON script_tbl ( script, type );"

/*
 * Начальная настройка базы
 */
bool_t askue_db_init ( sqlite3 *db )
{
	// проверка таблиц и индексов
        if ( db_add_tbl ( db, askue_sql_CREATE_REG_TBL, askue_sql_CREATE_REG_ID ) &&
             db_add_tbl ( db, askue_sql_CREATE_CNT_TBL, askue_sql_CREATE_CNT_ID ) && 
             db_add_tbl ( db, askue_sql_CREATE_TIME_TBL, askue_sql_CREATE_TIME_ID ) &&
             db_add_tbl ( db, askue_sql_CREATE_MODEM_TBL, askue_sql_CREATE_MODEM_ID ) &&
             db_add_tbl ( db, askue_sql_CREATE_LOG_TBL, askue_sql_CREATE_LOG_ID ) &&
             db_add_tbl ( db, askue_sql_CREATE_SCRIPT_TBL, askue_sql_CREATE_SCRIPT_ID ) )
        {
        	return time_tbl_prepare ( db );
        }
        else
        {
        	return FALSE;
        }
     
}

#undef askue_sql_CREATE_REG_TBL
#undef askue_sql_CREATE_CNT_TBL
#undef askue_sql_CREATE_TIME_TBL
#undef askue_sql_CREATE_MODEM_TBL
#undef askue_sql_CREATE_LOG_TBL
#undef askue_sql_CREATE_SCRIPT_TBL

#undef askue_sql_CREATE_REG_ID
#undef askue_sql_CREATE_CNT_ID
#undef askue_sql_CREATE_TIME_ID
#undef askue_sql_CREATE_MODEM_ID
#undef askue_sql_CREATE_LOG_ID
#undef askue_sql_CREATE_SCRIPT_ID

/*
 * Очистить базу от лишних записей
 */
#define reg_tbl_delete "DELETE FROM reg_tbl WHERE date < ( SELECT DATE ( 'now', '-62 day' ) );"
bool_t askue_db_cut ( sqlite3 *db )
{
	return ( sqlite3_exec_decor ( db, reg_tbl_delete, NULL, NULL ) ) ?
			sqlite3_exec_decor ( db, "VACUUM;", NULL, NULL ) : FALSE;
}
#undef reg_tbl_delete

/*
 * Открыть базу
 */
bool_t askue_db_open ( void )
{
	if ( sqlite3_open ( ASKUE_DB_FILE, &Askue_Launcher_DB ) == SQLITE_OK )
	{
		if ( sqlite3_exec_decor ( Askue_Launcher_DB, "BEGIN TRANSACTION;", NULL, NULL ) )	
			return askue_db_init ( Askue_Launcher_DB ) && sqlite3_exec_decor ( Askue_Launcher_DB, "END TRANSACTION;", NULL, NULL );
	}
	else
	{
		error ( 0, 0, "Ошибка sqlite: %s ( %d )\n", sqlite3_errmsg ( Askue_Launcher_DB ), sqlite3_errcode ( Askue_Launcher_DB ) );
		return FALSE;
	}
}

/*
 * Закрыть базу
 */
void askue_db_close ( void )
{
	sqlite3_close ( Askue_Launcher_DB );
}

/*
 * Доступ к базе
 */
sqlite3* askue_db_access ( void )
{
	return Askue_Launcher_DB;
}











