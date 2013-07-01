#include <stdio.h>
#include <sqlite3.h>
#include <libaskue.h>

#include "askue_launcher_db.h"

/*
 * Запись запись отчёта counterТМТА
 */
bool_t askue_launcher_report_TMTA ( sqlite3 *db, const char *date, FILE *report )
{
	/* обработчик записи отчёта в файл */
	int _write_report_TMTA ( void* ptr, int amount, char **values, char **cols )
	{
		return fprintf ( ( FILE* ) ptr, "%s;%s;%s;%s\n", values[ 0 ], values[ 1 ], values[ 2 ], values[ 3 ] ) <= 0;
	}
	
	/* Запись файла "counterTMTA" */
	
	bool_t status = FALSE;
	
	if ( report != NULL )
	{
		char *sql = sqlite3_mprintf ( "SELECT cnt, value, type, SUBSTR ( time, 1, 5 ) FROM reg_tbl WHERE ( reg_tbl.cnt = cnt_tbl.cnt and cnt_tbl.type = 'TM' or cnt_tbl.type = 'TA' ) and reg_tbl.date = '%s';", date );
		
		if ( sql != NULL )
		{
			status = sqlite3_exec_decor ( db, sql, _write_report_TMTA, report );
			
			sqlite3_free ( sql );
		}
	}
	
	return status;
}
