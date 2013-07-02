#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libaskue.h>

#include "askue_launcher_global.h"
#include "askue_launcher_macro.h"
#include "askue_launcher_reports.h"
#include "askue_launcher_db.h"
#include "askue_launcher_log_error.h"

/*
 * Установить флаг режима отладки
 */
bool_t argv_function_debug ( const char *arg_value )
{
	SETBIT ( Askue_Launcher_Flags, Debug_Flag );

	return TRUE;
}

/*
 * Установить флаг режима вывода на экран терминала связи
 */
bool_t argv_function_terminal ( const char *arg_value )
{
	SETBIT ( Askue_Launcher_Flags, Terminal_Flag );

	return TRUE;
}

/*
 * Установить кол-во строк для ограничения лога
 */
bool_t argv_function_loglines ( const char *arg_value )
{	
	if ( arg_value != NULL )
		sscanf ( arg_value, "%u", &Askue_Launcher_Log_Line_Amount );
		
	SETBIT ( Askue_Launcher_Flags, Log_Cut );

	return TRUE;
}

/*
 * Вывод краткой справки в терминал
 */
bool_t argv_function_help ( const char *arg_value )
{
	SETBIT ( Askue_Launcher_Flags, Exit_Flag );

	FILE *help_file; // файл справки
	
	if ( ( help_file = fopen ( ASKUE_HELP_FILE, "r" ) ) != NULL ) // открыть файл справки
	{
		int ch;
		
		while ( ( ch = fgetc ( help_file ) ) != EOF )	putchar ( ch ); // вывести на экран
		
		fflush ( help_file );
		
		fclose ( help_file ); // закрыть файл	
		
		return TRUE;
	}
	else
	{
		askue_log_error ( "Ошибка открытия файла: %s ( %d )\n", strerror ( errno ), errno );
		
		return FALSE;
	}
}

/*
 * Создание отчётов
 */
bool_t argv_function_report ( const char *arg_value )
{
	sqlite3 *db = askue_db_access ();

	SETBIT ( Askue_Launcher_Flags, Exit_Flag );
	
	bool_t status = ( arg_value != NULL ) ? askue_launcher_write_reports_according_date ( db, arg_value ) :
						askue_launcher_write_reports_according_weekday ( db );	
	
	return status; 
}





