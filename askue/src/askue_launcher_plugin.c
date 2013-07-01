#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include <errno.h>
#include <libaskue.h>
#include <dlfcn.h>

#include "askue_launcher_db.h"

struct __plugin_t
{
	char *name;
	void *file;
};

typedef struct __plugin_t plugin_t;

/*
 * Набор подключенных плагинов
 */
struct 
{
	size_t amount;
	plugin_t *item;
} Plugin_Set = { .amount = 0, .item = NULL };

/*
 * Закрыть плагины
 */
void askue_plugin_close ( void )
{
	if ( Plugin_Set.item )
	{
		for ( size_t i = 0; i < Plugin_Set.amount && Plugin_Set.item[ i ].name; i++ )
		{
			free ( Plugin_Set.item[ i ].name );
			//dlclose ( Plugin_Set.item[ i ].file );
		}
	}
}


/*
 * Привести к строчным буквам
 */
static void str_to_lowercase ( char *str )
{
	for ( size_t i = 0; str[ i ] != '\0'; i++ )
	{
		str[ i ] = tolower( str[ i ] );
	}
}

/*
 * Полное имя библиотеки
 */
static char* __get_full_name ( const char *name )
{
	char *result;
	
	char _name[ strlen ( name ) + 1 ];

	str_to_lowercase ( strcpy ( _name, name ) );
	
	if ( asprintf ( &result, "libaskue-%s.so", _name ) >= 0 )
	{
		return result;
	}
	else
	{
		askue_launcher_close_plugins ();
		error ( EXIT_FAILURE, errno, "askue: __get_full_name(): asprintf():" );
	}
}

/*
 * Новый плагин
 */
static plugin_t __new_plugin ( const char *name, const void *file )
{
	plugin_t p = { 
			.name = strdup ( name ), 
			.file = NULL
		     };
		     
	if ( !( p.name ) )
	{
		askue_launcher_close_plugins ();
		error ( EXIT_FAILURE, errno, "askue: __new_plugin(): strdup():" );	
	}
	
	p.file = file;
	
	return p;
}

/*
 * Открыть библиотеку плагина
 */
static void* __open_plugin_lib ( const char *name )
{
	char *__full_name = __get_full_name ( name );	
	
	void *lib = dlopen ( __full_name, RTLD_LAZY);

	if ( !lib )
	{
		askue_launcher_close_plugins ();
		error ( EXIT_FAILURE, errno, "askue: __open_plugin_lib(): dlopen(): %s", dlerror() );		
	}
	
	free ( __full_name );
	
	return lib;
}

/*
 * Открыть плагин
 */
static plugin_t open_plugin ( const char *name )
{
	return __new_plugin ( name, __open_plugin_lib ( name ) );
}

/*
 * Кол-во плагинов
 */
#define askue_sql_GET_PLUGINS_AMOUNT "SELECT COUNT( DISTINCT type ) FROM script_tbl;"
static size_t __get_plugins_amount ( sqlite3 *db )
{
	int __callback ( void *ptr, int len, char **vals, char **cols )
	{
		if ( sscanf ( vals[ 0 ], "%u", ( size_t* ) ptr ) != 1 )
		{
			errno ( EXIT_FAILURE, errno, "askue: __get_plugins_amount(): __callback(): sscanf():" );
		} 
		else
		{
			return 0;
		}
	}

	size_t amount;
	
	sqlite3_exec_decor ( db, askue_sql_GET_PLUGINS_AMOUNT, __callback, &amount );
	
	return amount;
}
#undef askue_sql_GET_PLUGINS_AMOUNT

/*
 * Узнать кол-во зарегистрированных плагинов ( типов устройств )
 * Выделить память под каждый плагин
 */
static void __init_plugins ( sqlite3 *db )
{
	Plugin_Set.amount = __get_plugins_amount ( db );
	
	Plugin_Set.item = ( plugin_t* ) askue_malloc ( sizeof ( plugin_t ) * Plugin_Set.amount );
	
	for ( size_t i = 0; i < Plugin_Set.amount; i++ )
	{
		Plugin_Set.item[ i ].name = Plugin_Set.item[ i ].file = NULL;
	}
}

/*
 * Получить имена типов
 */
#define askue_sql_GET_PLUGINS "SELECT DISTINCT type FROM script_tbl;"
void askue_plugin_init ( sqlite3 *db )
{
	__init_plugins ( db );
	
	size_t N = 0;

	int __callback ( void *ptr, int len, char **vals, char **cols )
	{
		Plugin_Set.item[ N ] = open_plugin ( vals[ 0 ] );
		
		N++;
		
		return 0;
	}

	sqlite3_exec_decor ( db, askue_sql_GET_PLUGINS, __callback, NULL );
}
#undef askue_sql_GET_PLUGINS

/*
 * Получить ссылку на плагин по его имени
 */
void* askue_plugin_invoke ( const char *name )
{
	size_t i;

	for ( i = 0; i < Plugin_Set.amount && strcmp ( Plugin_Set.item[ i ].name, name ); i++ );
	
	return Plugin_Set.item[ i ].file;
	
}





