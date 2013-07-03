#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <libaskue.h>

#include "askue_launcher_log_general.h"
#include "askue_launcher_global.h"
#include "askue_launcher_macro.h"


/*
 * Открыть файл лога
 * Возможная точка выхода
 */
static FILE* _log_open ( const char *file_name )
{
	FILE* f = fopen ( file_name, "a+" );
	
	if ( f == NULL )
	{
		printf ( "E1: %s - %s ( %d )\n", file_name, strerror ( errno ), errno );
		exit ( 1 );
	}
	
	return f;
}

/*
 * Получить время записи лога
 */
static const char* _log_time ( void )
{
	static char buf[ DATE_STRING_BUF + TIME_STRING_BUF + 2 ];
	
	time_t t = time ( NULL );
	
	strftime ( buf, DATE_STRING_BUF + TIME_STRING_BUF + 2, "%Y-%m-%d %H:%M:%S", localtime ( &t ) );
	
	return buf;
}

/*
 * Запись сообщения в базу
 */
static bool_t _log_write_msg_to_file ( const char *_msg )
{
	FILE *file = _log_open ( ASKUE_LOG_FILE ); // получить доступ к файлу лога
		
	bool_t status = fprintf ( file, "%s: %s\n", _log_time (), _msg ) > 0;
	
	fclose ( file );
	
	return status;
}

/*
 * Обрезать лог по кол-ву записей
 */
static bool_t _log_cut ( const char *file_name )
{
	if ( TESTBIT ( Askue_Launcher_Flags, Log_Cut ) ) // проверка разрешения обрезки
	{
		char *cmd = NULL;
	
		bool_t status = FALSE;
	
		// строка с командой
		if ( asprintf ( &cmd, "/sbin/askue_launcher_log_cut %s %u", file_name, Askue_Launcher_Log_Line_Amount ) > 0 )
		{
			status = !system ( cmd ); // выполнить команду
			
			free ( cmd );
		}
		else
		{
			_log_write_msg_to_file ( "Ошибка asprintf () в функции '_log_cut'\n" );
		}
		
		return status;
	}
	else
	{
		return TRUE;
	}
}

/*
 * Запись лога ошибок
 * Запись ведётся в файл
 */ 
bool_t askue_log_error ( const char *format, ... )
{
	// Обработка аргументов
	va_list args;
	
	va_start ( args, format );
	
	// Запись лога
	bool_t status = _log_ ( _log_write_msg_to_file, format, args ); // оформить сообщение для записи в базу
	
	va_end ( args );
	
	return status;
}

