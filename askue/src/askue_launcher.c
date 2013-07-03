#define _GNU_SOURCE
#define __USE_BSD

#include "askue_launcher_macro.h"

#include <libconfig.h>
#include <libaskue.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h> 
#include <sys/wait.h>

#include "askue_launcher_opt.h"
#include "askue_launcher_global.h"
#include "askue_launcher_db.h"
#include "askue_launcher_net_cfg.h"


//#define DEBUG

/**********************************************/
/*
 * Типы данных
 */
/**********************************************/

/**********************************************/
/*
 * Глобальные переменные
 */
/**********************************************/

/**********************************************/
/*
 * Прототипы
 */
/**********************************************/


/*
 * Разбор установленных флагов
 */
void askue_flag_parse ( void );

/*
 * Конфигурирование аскуэ
 */
void askue_config ( sqlite3 *db );

bool_t askue_monitor ( sqlite3* );

int main ( int argc, char **argv )
{
	int result = 0;

	if ( askue_db_open () )
	{	
		askue_config ( askue_db_access () );
		
		if ( askue_argv_parse ( argc, argv ) == TRUE )
		{
			askue_flag_parse (); // возможная точка выхода
			
			result = askue_monitor ( askue_db_access () );
		}
	}
	
	askue_db_close ();
	askue_launcher_close_plugins ();
		
	return result;
}


/*
 * Конфигурирование аскуэ
 */
void askue_config ( sqlite3 *db )
{
	askue_network_config ( db ); 
	askue_scripts_config ( db );
	askue_launcher_flags_to_default (); // начальная установка флагов
	askue_launcher_log_line_default_amount (); // кол-во линий в логе по умолчанию
	askue_launcher_init_plugins ( db );
}

/*
 * Разбор установленных флагов
 */
void askue_flag_parse ( void )
{
	if ( TESTBIT ( Askue_Launcher_Flags, Exit_Flag ) )
	{
		exit ( 0 ); // выход если стоит флаг выхода
	}
}



/*
bool_t exec_cmd ( const char *_cmd )
{
	char cmd[ strlen ( _cmd ) + strlen ( "/sbin/" ) + 1 ];

	int cmd_status = 0;
	bool_t result = TRUE;
	
	if ( _cmd != NULL && strlen ( _cmd ) > 0 )
	{
		sprintf ( cmd, "/sbin/%s", _cmd );
		
		if ( TESTBIT ( Askue_Launcher_Flags, Debug_Flag ) )
        		log_write ( stderr, "Старт выполнения команды '%s'\n", cmd );
		
		pid_t pid = fork ();
		
		if  ( pid < (pid_t)0 ) //ошибка
		{
			log_write ( stderr, "Ошибка fork(): %s ( %d )\n", strerror ( errno ), errno );
			result = FALSE;
		}
		else if ( pid == (pid_t)0 ) //потомок
		{
			char *const cmd_argv[] = { ( char *const )cmd, 
						   ( TESTBIT ( Askue_Launcher_Flags, Terminal_Flag ) ) ? "debug" : NULL,
						   NULL };
			
			if ( execv ( cmd, cmd_argv ) < 0 )
			{		
				log_write ( stderr, "Ошибка execv(): %s ( %d )\n", strerror ( errno ), errno );
				
				exit ( -1 );
			}
		}
		else //родитель
		{		
			// получаем статус завершение
           		 wait( &cmd_status );
           		  
           		 if ( cmd_status )
           		 {		
				log_write ( stderr, "Команда '%s' завершена с кодом %d\n", cmd, cmd_status );
			 }
			 else if ( TESTBIT ( Askue_Launcher_Flags, Debug_Flag ) )
			 {
			        log_write ( stderr, "Команда '%s' завершена успешно\n", cmd );
			 }
           		 
           		 askue_launcher_log_cut();
           		 
           		 result = TRUE;
		}
		
		
	}
	else if ( _cmd != NULL && strlen ( _cmd ) == 0 ) 
	{
		log_write ( stderr, "Пустая строка вместо команды\n" );
	}
	
	return result;
}
*/










