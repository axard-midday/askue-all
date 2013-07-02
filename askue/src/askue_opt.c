#include <stdio.h>
#include <getopt.h>
#include <libaskue.h>

#include "askue_launcher_argv_func.h"
#include "askue_launcher_global.h"
#include "askue_launcher_log_error.h"

// список длинных опций
static struct option long_options[] = 
{
	{ "debug", no_argument, NULL, 'd' },
	{ "help", no_argument, NULL, 'h' },
	{ "terminal", no_argument, NULL, 't' },
	{ "loglines", required_argument, NULL, 'l' },
	{ "report", optional_argument, NULL, 'r' },
	{ NULL, no_argument, NULL, 0 }
};

// тип, описывающий функцию опбрабатывающий конкретную опцию командной строки
typedef bool_t ( *argv_func_t ) ( const char *arg_value );

// массив функций обработчиков
static argv_func_t argv_functions[] = 
{ 
	argv_function_debug, 
	argv_function_help,
	argv_function_terminal,
	argv_function_loglines,
	argv_function_report
};

/*
 * Старт функции обработчика
 */
static bool_t argv_start_func ( int opt_code, int longopt_index, const char *arg_value )
{
	if ( opt_code != '?' ) // какая-то из опций указанных в long_options
	{
		return argv_functions[ longopt_index ] ( arg_value ); // выполнить обработчик
	}
	else
	{
		argv_function_help ( NULL );
		// вывод справки на экран
		return FALSE;
	}
}


/*
 * Разбор аргументов CLI ( Command Line Interface )
 */
bool_t askue_argv_parse ( int argc, char **argv )
{
	bool_t result = TRUE;

	int longopt_index = 0;

	int opt_code = getopt_long ( argc, argv, "", long_options, &longopt_index ); // разбор первого встреченного параметра

	while ( opt_code != -1 && result ) // если нашли что-то
	{
		if ( ( result = argv_start_func ( opt_code, longopt_index, optarg ) ) == FALSE ) // запуск функции-обработчика для опции
		{
			askue_log_error ( "Ошибка разбора опций\n" ); // произошла ошибка
		}
		else
		{
			// нормальное завершение
			opt_code = getopt_long ( argc, argv, "", long_options, &longopt_index ); // разбор следующего параметра
		}
	
	}
	
	return result;
}

