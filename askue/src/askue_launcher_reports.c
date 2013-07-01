#define _GNU_SOURCE

#include <stdio.h>
#include <sqlite3.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <libaskue.h>

#include "askue_launcher_macro.h"
#include "askue_launcher_report_DFL_DTL.h"
#include "askue_launcher_report_TMTA.h"
#include "askue_launcher_global.h"
#include "askue_launcher_log_error.h"
/*
 * тип дня недели
 */
typedef enum
{
	weekday_Mon = 1,
	weekday_Tue = 2,
	weekday_Wed = 3,
	weekday_Thu = 4,
	weekday_Fri = 5,
	weekday_Sat = 6,
	weekday_Sun = 7
} weekday_t;

/*
 * Тип функции создания отчёта
 */
typedef bool_t ( *askue_launcher_report_func_t ) ( sqlite3*, const char*, FILE* );

/*
 * Тип самого отчёта
 */
typedef struct _askue_launcher_report
{ 
	const char *name;
	askue_launcher_report_func_t get_report;
} askue_launcher_report_t;

/*
 * Список отчётов
 */
static askue_launcher_report_t Askue_Launcher_Reports[] = 
{
	{ "counterTMTA", askue_launcher_report_TMTA },
	{ "dfl", askue_launcher_report_DFL },
	{ "dtl", askue_launcher_report_DTL },
	{ NULL, NULL }
};
 
/*
 * Проверка конца массива отчётов
 */
static bool_t askue_launcher_report_is_end ( size_t index )
{
	return ( Askue_Launcher_Reports[ index ]. name = NULL ) ? TRUE : FALSE;
}
 
/*
 * 			Функции отчётов
 */

/*
 * Закрыть файл отчёта
 */
#define close_report( _REPORT_ ) fclose ( _REPORT_ )

/*
 * Открыть файл отчёта
 */
static FILE* open_report ( const char *report_name ) 
{
	FILE *report = fopen ( report_name, "w" ); // открыть файл
	
	if ( report == NULL ) // ошибка fopen ()
	{
		askue_log_error ( "Ошибка fopen () = %s ( %d )\n", strerror ( errno ), errno );
	}
		
	return report;	
}



/*
 * Добавить строку с датой
 */
static bool_t add_date_to_report ( FILE *report )
{
	time_t t = time ( NULL );
	
	struct tm *tm_ptr = localtime ( &t );
	
	char date[ DATE_STRING_BUF + 1 ];
	
	strftime ( date, DATE_STRING_BUF + 1,  "%Y-%m-%d", tm_ptr );
	
	return ( fprintf ( report, "%s\n", date ) > 0 ) ? TRUE : FALSE ;
}




/*
 * Получить дату по дню недели
 */
static char* get_date_according_weekday ( weekday_t weekday )
{
	/* 
 	 * Получить текущий день недели
 	 */
	weekday_t get_weekday_now ( void )
	{
		time_t t = time ( NULL );
	
		struct tm *tm_ptr = localtime ( &t );
	
		if ( tm_ptr -> tm_wday )
		{
			return ( weekday_t ) tm_ptr -> tm_wday;
		}
		else
			return weekday_Sun;	
	}

	// 
	char *offset = NULL;
	
	// оформить строку смещения
	if ( asprintf ( &offset, "%d day", ( int )( weekday - get_weekday_now () ) ) > 0 )
	{
		// получить дату
		char *date = ( char* ) malloc ( sizeof ( char ) * ( DATE_STRING_BUF + 1 ) );
		
		if ( date != NULL )
		{
			sprintf_datetime ( date, DATE_STRING_BUF + 1, "%Y-%m-%d", ( const char *[] ){ offset, NULL } );
		}
		
		free ( offset );
		
		return date;
	}
	else
	{
		askue_log_error ( "Ошибка asprintf () = %s ( %d )\n", strerror ( errno ), errno );
		
		return NULL;
	}
}

/*
 * Получить полное имя файла отчёта согласно дню недели
 */
static char* get_report_name_according_date ( const char *name, const char *date ) 
{
	char *full_name;
	
	if ( asprintf ( &full_name, "/mnt/base/reports/%s.%s", name, date ) > 0 )
	{
		return full_name;
	}
	else // ошибка asprintf ()
	{
		askue_log_error ( "Ошибка asprintf () = %s ( %d )\n", strerror ( errno ), errno );
	
		return NULL;
	}
}

/*
 * Получить полное имя файла отчёта
 */
static char* get_report_name_according_weekday ( const char *name, weekday_t weekday ) 
{
	char *full_name;
	
	if ( asprintf ( &full_name, "/mnt/base/reports/%s%d", name, weekday ) > 0 )
	{
		return full_name;
	}
	else // ошибка asprintf ()
	{
		askue_log_error ( "Ошибка asprintf () = %s ( %d )\n", strerror ( errno ), errno );
	
		return NULL;
	}
}

/*
 * Запаковать отчёт
 */
static bool_t askue_launcher_gzip_report ( const char *file_name )
{
	char *command = NULL;
			
	// заполнить шаблон	
	if ( asprintf ( &command, "gzip -c %s > %s.gz", file_name, file_name ) > 0 )
	{
		bool_t status;
	
		if ( TESTBIT ( Askue_Launcher_Flags, Debug_Flag ) )
			askue_log_event ( stderr, "Файл '%s' - сжат\n", file_name );
		
		status = ( system ( command ) ) ? ({
						     askue_log_error ( "Ошибка выполнения команды: %s\n", command );
						     FALSE; 
						  }) : TRUE;
			
		free ( command );
		
		return status;
	}
	else
	{
		askue_log_error ( "Ошибка функции asprintf ()\n" );
		
		return FALSE;
	}
}

/*
 * Запись отчётов за Пн, Вт, Ср, Чт, Пт, Сб, Вс
 */
bool_t askue_launcher_write_reports_according_weekday ( sqlite3 *SQLite3 )
{
	bool_t status = TRUE;
	
	// перебор дней недели
	for ( weekday_t weekday = weekday_Mon; weekday <= weekday_Sun && status; weekday++ )
	{
		// получить дату исходя из дня недели
		char *date = get_date_according_weekday ( weekday ); 
	
		if ( ( status = date != NULL ) )
		{
			// перебор отчётов
			for ( size_t i = 0; !askue_launcher_report_is_end( i ) && status; i++ )
			{
				// получить имя отчёта
				char *report_name = get_report_name_according_weekday ( Askue_Launcher_Reports[ i ].name, weekday );
				
				if ( ( status = report_name != NULL ) )
				{
					// открыть отчёт
					FILE *report = open_report ( report_name );
				
					if ( ( status = report != NULL ) )
					{
						// добавить дату в отчёт и записать в него выборку показаний
						status = ( add_date_to_report ( report ) ) ? 
								( Askue_Launcher_Reports[ i ].get_report ( SQLite3, date, report ) && askue_launcher_gzip_report ( report_name ) ) : FALSE;
				
						close_report ( report );
					}
					
					free ( report_name );
				}	
			}
			
			free ( date );
		}
		
	
	}		
	return status;
}

/*
 * Запись отчётов за конкретную дату
 */
bool_t askue_launcher_write_reports_according_date ( sqlite3 *SQLite3, const char *date )
{

	bool_t status = TRUE;
	
	// перебор отчётов
	for ( size_t i = 0; !askue_launcher_report_is_end( i ) && status; i++ )
	{
		// получить имя отчёта
		char *report_name = get_report_name_according_date ( Askue_Launcher_Reports[ i ].name, date );
			
		if ( ( status = report_name != NULL ) )
		{
			// открыть отчёт
			FILE *report = open_report ( report_name );
			
			if ( ( status = report != NULL ) )
			{
				// добавить дату в отчёт и записать в него выборку показаний
				status = ( add_date_to_report ( report ) ) ? 
						( Askue_Launcher_Reports[ i ].get_report ( SQLite3, date, report ) && askue_launcher_gzip_report ( report_name ) ) : FALSE;
			
				close_report ( report );
			}	
			
			free ( report_name );
		}
			
	}	
		
	return status;
}

