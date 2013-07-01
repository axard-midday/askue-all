#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "askue_launcher_log_general.h"

/*
 * Оформить сообщение для записи
 */
char* _log_get_msg ( const char *_format, va_list _args )
{
	char *_msg;
		
	if ( vasprintf ( &_msg, _format, _args ) > 0 )
	{
		return _msg;
	}
	else
	{	
		return NULL;
	}
}

/*
 * Функции записи в лог
 */
bool_t _log_ ( _log_func_t _log_func, const char *_format, va_list _args )
{
	char *msg = _log_get_msg ( _format, _args ); // оформить сообщение для записи в базу
	
	/***************************************/
	/*
	 * запись в базу
	 */	
	bool_t status; 
	 
	if ( msg != NULL )
	{
		status = _log_func ( msg );

		free ( msg );
	}
	else
	{
		status = FALSE;
	}
	
	return status;
}

