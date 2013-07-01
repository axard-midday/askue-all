#include <stdio.h>
#include <sqlite3.h>
#include <libaskue.h>

#include "askue_launcher_db.h"

/*
 * Создать временную таблицу под конкретный тип показаний за определённую дату
 * для отчёта DFL
 */
bool_t create_dfl_tmp_tbl ( sqlite3 *db, const char *date, const char *type )
{
	bool_t status = FALSE;

	char *sql = sqlite3_mprintf ( "create temporary table tmp_tbl_%s as select reg_tbl.cnt, reg_tbl.value, reg_tbl.time from reg_tbl, cnt_tbl where reg_tbl.date = '%s' and reg_tbl.type = '%s' and ( reg_tbl.cnt = cnt_tbl.cnt and cnt_tbl.type = 'MKS' );", type, date, type );

	if ( sql != NULL )
	{
		status = sqlite3_exec_decor ( db, sql, NULL, NULL );
			
		sqlite3_free ( sql );
	}
	
	return status;
}

/*
 * Создать временную таблицу под конкретный тип показаний за определённую дату
 * для отчёта DTL
 */
bool_t create_dtl_tmp_tbl ( sqlite3 *db, const char *date, const char *type )
{
	bool_t status = FALSE;

	char *sql = sqlite3_mprintf ( "create temporary table tmp_tbl_%s as select reg_tbl.cnt, reg_tbl.value, reg_tbl.time from reg_tbl, cnt_tbl where reg_tbl.date = '%s' and reg_tbl.type = '%s' and ( reg_tbl.cnt = cnt_tbl.cnt and cnt_tbl.type = 'MKS' ) and reg_tbl.time = '00:00:00';", type, date, type );

	if ( sql != NULL )
	{
		status = sqlite3_exec_decor ( db, sql, NULL, NULL );
			
		sqlite3_free ( sql );
	}
	
	return status;
}

/*
 * Удалить временную таблицу, созданную под конкретный тип
 */
bool_t drop_tmp_tbl ( sqlite3 *db, const char *type )
{
	bool_t status = FALSE;

	char *sql = sqlite3_mprintf ( "DROP TABLE tmp_tbl_%s", type );

	if ( sql != NULL )
	{
		status = sqlite3_exec_decor ( db, sql, NULL, NULL );
			
		sqlite3_free ( sql );
	}
	
	return status;
} 

/*
 * СФормировать отчёт DFL
 */
bool_t askue_launcher_report_DFL ( sqlite3 *db, const char *date, FILE *report )
{
	/* обработчик записи отчёта в файл */
	int _write_report_DFL ( void* ptr, int amount, char **values, char **cols )
	{
		return fprintf ( ( FILE* ) ptr, "%s;%s;%s;%s;%s;%s\n", 
				  values[ 0 ], values[ 1 ], values[ 2 ], values[ 3 ], values[ 4 ], values[ 5 ] ) <= 0;
	}
	
	bool_t status = FALSE;

	if ( create_dfl_tmp_tbl ( db, date, "ch0" ) &&
	     create_dfl_tmp_tbl ( db, date, "ch1" ) &&
	     create_dfl_tmp_tbl ( db, date, "ch2" ) &&
	     create_dfl_tmp_tbl ( db, date, "ch3" ) )
	{
	
		const char *sql = "SELECT tmp_tbl_ch0.cnt, tmp_tbl_ch0.value, tmp_tbl_ch1.value, tmp_tbl_ch2.value, tmp_tbl_ch3.value, SUBSTR ( tmp_tbl_ch0.time, 1, 5 ) FROM tmp_tbl_ch0, tmp_tbl_ch1, tmp_tbl_ch2, tmp_tbl_ch3 WHERE tmp_tbl_ch0.cnt = tmp_tbl_ch1.cnt AND tmp_tbl_ch1.cnt = tmp_tbl_ch2.cnt AND tmp_tbl_ch2.cnt = tmp_tbl_ch3.cnt AND tmp_tbl_ch0.time = tmp_tbl_ch1.time AND tmp_tbl_ch1.time = tmp_tbl_ch2.time AND tmp_tbl_ch2.time = tmp_tbl_ch3.time;";
	
		status = sqlite3_exec_decor ( db, sql, _write_report_DFL, report );
	
		drop_tmp_tbl ( db, "ch0" );
		drop_tmp_tbl ( db, "ch1" );
		drop_tmp_tbl ( db, "ch2" );
		drop_tmp_tbl ( db, "ch3" );		
	}
	
	return status;
}

/*
 * СФормировать отчёт DTL
 */
bool_t askue_launcher_report_DTL ( sqlite3 *db, const char *date, FILE *report )
{
	/* обработчик записи отчёта в файл */
	int _write_report_DTL ( void* ptr, int amount, char **values, char **cols )
	{
		return fprintf ( ( FILE* ) ptr, "%s;%s;%s;%s;%s;%s\n", 
				  values[ 0 ], values[ 1 ], values[ 2 ], values[ 3 ], values[ 4 ], values[ 5 ] ) <= 0;
	}
	
	bool_t status = FALSE;

	if ( create_dtl_tmp_tbl ( db, date, "ch0" ) &&
	     create_dtl_tmp_tbl ( db, date, "ch1" ) &&
	     create_dtl_tmp_tbl ( db, date, "ch2" ) &&
	     create_dtl_tmp_tbl ( db, date, "ch3" ) )
	{
	
		const char *sql = "SELECT tmp_tbl_ch0.cnt, tmp_tbl_ch0.value, tmp_tbl_ch1.value, tmp_tbl_ch2.value, tmp_tbl_ch3.value, SUBSTR ( tmp_tbl_ch0.time, 1, 5 ) FROM tmp_tbl_ch0, tmp_tbl_ch1, tmp_tbl_ch2, tmp_tbl_ch3 WHERE tmp_tbl_ch0.cnt = tmp_tbl_ch1.cnt AND tmp_tbl_ch1.cnt = tmp_tbl_ch2.cnt AND tmp_tbl_ch2.cnt = tmp_tbl_ch3.cnt AND tmp_tbl_ch0.time = tmp_tbl_ch1.time AND tmp_tbl_ch1.time = tmp_tbl_ch2.time AND tmp_tbl_ch2.time = tmp_tbl_ch3.time;";
	
		status = sqlite3_exec_decor ( db, sql, _write_report_DTL, report );
	
		drop_tmp_tbl ( db, "ch0" );
		drop_tmp_tbl ( db, "ch1" );
		drop_tmp_tbl ( db, "ch2" );
		drop_tmp_tbl ( db, "ch3" );		
	}
	
	return status;
}












