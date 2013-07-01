#include "bool.h"
#include "askue_plugin.h"

/*
 * Тело исполнения любого скрипта
 */
bool_t askue_plugin_method ( askue_ioport_method_f _io_method, 
                             askue_errlog_method_f _errlog_method,
                             askue_journal_method_f _jnl_method,
                             askue_plugin_get_request_f _get_request, 
                             askue_plugin_valid_checksum_f _valid_checksum,
                             askue_plugin_valid_content_f _valid_content,
                             askue_plugin_get_result_f _get_result,
                             const char *address,
                             long int timeout,
                             const void *parametr,
                             void *output );
{
	bool_t result;

	uint8_array_t *request = __get_request ( address, 
	                                         parametr ); // получить запрос

	uint8_array_t *answer = io_method ( request, timeout ); // получить ответ
		
	// проверки
	if ( !( answer -> len ) )
	{
	        _jnl_method ( "%s - %s", address, "Истекло время ожидания ответа" );
	}
	else if ( __valid_checksum && !__valid_checksum ( answer, _jnl_method ) )
	{
	        result = FALSE;	
	}
	else if ( __valid_content && !__valid_content ( request, answer, _jnl_method ) )
	{
	        result = FALSE;	
	}
	else 
	{
	        if ( __get_result ) __get_result ( answer, output );
	        result = TRUE; // выделить результат
	}
                
    uint8_array_delete ( answer );		
	uint8_array_delete ( request );
	
	return result;
}



