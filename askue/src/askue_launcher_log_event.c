#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "askue_launcher_db.h"
#include "askue_launcher_log_general.h"
#include "askue_launcher_log_error.h"


/*
 * Запись сообщения в базу
 */
static bool_t _log_write_msg_to_db ( const char *_msg )
{
	sqlite3 *db = askue_db_access (); // получить доступ к базе
		
	char *sql = sqlite3_mprintf ( "INSERT INTO log_tbl ( date, time, msg ) \
					VALUES ( ( SELECT DATE ( 'now' ) ), ( SELECT TIME ( 'now' ) ), '%s' );", _msg );
	
	bool_t status;
		
	if ( sql != NULL ) 
	{
		status = sqlite3_exec_decor ( db, sql, NULL, NULL );
		
		sqlite3_free ( sql );
	}
	else
	{
		askue_log_error ( "Ошибка sqlite3_mprintf ():\n" );
	
		status = FALSE;
	}
	
	return status;
}

/*
 * Запись лога событий
 * Запись ведётся в базу
 */ 
bool_t askue_log_event ( const char *format, ... )
{
	// Обработка аргументов
	va_list args;
	
	va_start ( args, format );
	
	// Запись лога
	bool_t status = _log_ ( _log_write_msg_to_db, format, args ); // оформить сообщение для записи в базу
	
	va_end ( args );
	
	return status;
}
