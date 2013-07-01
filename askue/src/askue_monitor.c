#include <sqlite3.h>
#include <libaskue.h>

#include "askue_launcher_db.h"
#include "askue_launcher_log_error.h"
#include "askue_plugin.h"
#include "askue_launcher_macro.h"

/*
 * Проверка удалённости сегмента
 */
static bool_t __is_remote ( const char *segment )
{
	return segment[ 0 ] == '1';
}

/*
 * Обработка скриптов устройства
 */
#define askue_sql_GET_SCRIPTS "SELECT script_tbl.script, script_tbl.parametr FROM script_tbl WHERE script_tbl.type = '%s';"
static bool_t askue_exec_script ( sqlite3 *db, const char *unit, const char *type, const char *timeout )
{
	int __callback ( void *ptr, int len, char **vals, char **cols )
	{
		const char **params = ptr;
		
		#define UNIT 		params[ 0 ]
		#define TIMEOUT 	params[ 1 ]
		#define TYPE 		params[ 2 ]
		#define SCRIPT 		vals[ 0 ]
		#define PARAMETR	vals[ 1 ]
		
		void *plugin = askue_launcher_get_plugin ( TYPE );
		
		return printf ( ">> >> >> %s ( %s | %s msec ) : %s = %s\n", UNIT, TYPE, TIMEOUT, SCRIPT, ( ( PARAMETR ) ?: "(null)" ) ) <= 0;
		
		//script_f script = askue_script_access ( TYPE, SCRIPT );
		
		//askue_container_t param = 
		
		#undef PARAMETR
		#undef SCRIPT
		#undef UNIT
		#undef TYPE
		#undef TIMEOUT
	}

	char *sql = sqlite3_mprintf ( askue_sql_GET_SCRIPTS, type );
	
	if ( sql )
	{
		const char *params[ 3 ] =
		{
			unit, timeout, type
		};
	
		return sqlite3_exec_decor ( db, sql, __callback, params );
	}
	else
	{
		askue_log_error ( "askue: askue_exec_script(): sqlite3_mprintf()\n" );
	}
}
#undef askue_sql_GET_SCRIPTS

#define askue_sql_GET_COUNTERS "SELECT cnt_tbl.cnt, cnt_tbl.type, cnt_tbl.timeout, cnt_tbl.modem, modem_tbl.remote_flag, modem_tbl.type FROM cnt_tbl, modem_tbl WHERE cnt_tbl.modem = modem_tbl.modem;"
bool_t askue_monitor ( sqlite3 *db )
{		
	int __callback ( void *ptr, int len, char **vals, char **cols )
	{
		#define CNT 		vals[ 0 ]
		#define CNT_TYPE 	vals[ 1 ]
		#define CNT_TIMEOUT	vals[ 2 ]
		#define MODEM 		vals[ 3 ]
		#define REMOTE_FLAG 	vals[ 4 ]
		#define MODEM_TYPE 	vals[ 5 ]

		printf ( ">> | %s | %s | %s | %s | %s | %s |\n", CNT,
									     CNT_TYPE,
									     CNT_TIMEOUT,
									     MODEM,
									     REMOTE_FLAG,
									     MODEM_TYPE );
		printf ( ">> >> remote = %d\n", __is_remote ( REMOTE_FLAG ) );
		
		if ( __is_remote ( REMOTE_FLAG ) )
		{
			return !( askue_exec_script ( ( sqlite3* ) ptr, MODEM, MODEM_TYPE, "3000" ) && 
				   askue_exec_script ( ( sqlite3* ) ptr, CNT, CNT_TYPE, CNT_TIMEOUT ) );
		}
		else
		{
			return !askue_exec_script ( ( sqlite3* ) ptr, CNT, CNT_TYPE, CNT_TIMEOUT );
		}

		
		
	
		
		#undef CNT
		#undef CNT_TYPE
		#undef CNT_TIMEOUT
		#undef MODEM
		#undef REMOTE_FLAG
		#undef MODEM_TYPE
	}
     
	return sqlite3_exec_decor ( db, askue_sql_GET_COUNTERS, __callback, db );
}
#undef askue_sql_GET_COUNTERS







































