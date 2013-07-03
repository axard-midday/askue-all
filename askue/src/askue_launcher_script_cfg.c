#include <stdio.h>
#include <sqlite3.h>
#include <libconfig.h>
#include <string.h>
#include <errno.h>
#include <libaskue.h>

#include "askue_launcher_db.h"
#include "askue_launcher_macro.h"
#include "askue_launcher_log_error.h"

#define script_tbl_insert_2 "INSERT INTO script_tbl ( type, script, parametr ) VALUES ( '%s', ( SELECT TRIM ( '%.*s', ' ' ) ), ( SELECT TRIM ( '%s', ' ' ) ) );"
#define script_tbl_insert_1 "INSERT INTO script_tbl ( type, script, parametr ) VALUES ( '%s', ( SELECT TRIM ( '%s', ' ' ) ), '' );"
static char* __get_script_tbl_insert ( const char *type, const char *script )
{
	char *parametr = strchr ( script, '=' );
	
	return ( parametr ) ? sqlite3_mprintf ( script_tbl_insert_2, type, strlen ( parametr ), script, parametr + 1 )
			     : sqlite3_mprintf ( script_tbl_insert_1, type, script );
		
}
#undef script_tbl_insert_2
#undef script_tbl_insert_1

static bool_t askue_scripts_add_script ( sqlite3 *db, const char *type, const char *script )
{
	char *sql = __get_script_tbl_insert ( type, script );
	
	if ( sql )
	{
		bool_t status = sqlite3_exec_decor ( db, sql, NULL, NULL );
		sqlite3_free ( sql );
		return status;
	}
	else
	{
		askue_log_error ( "askue: askue_scripts_add_script(): sqlite3_mprintf()\n" );
	}
}


static bool_t askue_scripts_config_branch ( sqlite3 *db, const config_setting_t *scripts )
{
	bool_t status = TRUE;

	const char *type = config_setting_name ( scripts ); // имя типа
	
	size_t length = config_setting_length ( scripts ); // кол-во элементов
	
	for ( size_t i = 0; i < length && status; i++ ) // перебор элементов
	{
		status = askue_scripts_add_script ( db, type, config_setting_get_string_elem ( scripts, i ) );
	}
	
	return status;
}

/*
 * Занесение данных из конфига в таблицу базы
 */
int askue_scripts_config ( sqlite3 *db )
{
	bool_t status = FALSE;
	
        config_t cfg;

	config_init ( &cfg ); // выделить память под переменную с конфигурацией
	
	if ( config_read_file ( &cfg, ASKUE_NET_FILE ) == CONFIG_TRUE ) // открыть и прочитать файл
	{
	        config_setting_t *scripts = config_lookup ( &cfg, "Scripts" ); // поиск сети
	        
	        if ( scripts )
	        {
	        	if ( ( status = sqlite3_exec_decor ( db, "BEGIN TRANSACTION;", NULL, NULL ) ) ) // обёртка из транзакций
	        	{
				size_t length = config_setting_length ( scripts ); // длина сети
	
				for ( size_t i = 0; i < length && status; i++ ) //пройтись по ветвям сети
				{ 
					status = askue_scripts_config_branch ( db, config_setting_get_elem ( scripts, i ) );
				}
				
				sqlite3_exec_decor ( db, "END TRANSACTION;", NULL, NULL );
			}
	        }
	        else
	        {
	        	askue_log_error ( "Отсутствует поле 'Scripts' в конфигурации\n" );
	        }      
	}
	else
	{
	        askue_log_error ( "Ошибка конфигурации: %s ( %d )\n", config_error_text ( &cfg ), config_error_line ( &cfg ) );
	}
	
	config_destroy ( &cfg );
	 
	return status;
}