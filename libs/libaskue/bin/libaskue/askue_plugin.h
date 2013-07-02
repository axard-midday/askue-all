#ifndef ASKUE_PLUGIN_H_
#define ASKUE_PLUGIN_H_

#include "bool.h"
#include "uint8_array.h"
#include "errlog.h"
#include <stdarg.h>

/******************************************************************************/
			/*
			 * Тело скрипта
			 */
/******************************************************************************/

// доступ к порту ввода вывода
typedef uint8_array_t* ( *askue_ioport_method_f ) ( const uint8_array_t *, 
                                                          long int );

// обработчик обращений к базе
typedef int ( *askue_db_callback_f ) ( void*, int, char **, char ** );

// доступ к базе данных
typedef bool_t ( *askue_db_method_f ) ( const char *, 
                                            askue_db_callback_f, 
                                            void * );

// доступ к журналу
typedef bool_t ( *askue_journal_method_f ) ( const char *fmt, ... );

// интерфейсы предоставляемые плагину со стороны аскуэ
typedef struct
{
        askue_ioport_method_f       io_method;
        askue_db_method_f            db_method;
        askue_journal_method_f      jnl_method;
        askue_errlog_method_f       log_method;
} askue_plugin_interface_t;

// плагин
typedef void ( *askue_plugin_f ) ( const askue_plugin_interface_t *interface,
                                      int pargc,
                                      const char * const * pargv );

// сформировать массив байт для отправки
typedef uint8_array_t* askue_plugin_get_request_f ( const char*, const void* );

// проверить контрольную сумму
typedef bool_t askue_plugin_valid_checksum_f ( const uint8_array_t*, 
                                                    askue_journal_method_f );

// проверить содержимое
typedef bool_t askue_plugin_valid_content_f ( const uint8_array_t*, 
                                                   const uint8_array_t*, 
                                                   askue_journal_method_f );

// получить результат
typedef void askue_plugin_get_result_f ( const uint8_array_t*, void* );

// болванка для конструирования функций обмена
typedef bool_t ( *askue_plugin_method_f ) ( askue_ioport_method_f, 
                                                askue_journal_method_f,
                                                askue_plugin_get_request_f, 
                                                askue_plugin_valid_checksum_f,
                                                askue_plugin_valid_content_f,
                                                askue_plugin_get_result_f,
                                                const char *address,
                                                long int timeout,
                                                const void *parametr,
                                                void *output );

// процедура обмены через порт ввода-вывода
bool_t askue_plugin_method ( askue_ioport_method_f, 
                             askue_journal_method_f,
                             askue_plugin_get_request_f, 
                             askue_plugin_valid_checksum_f,
                             askue_plugin_valid_content_f,
                             askue_plugin_get_result_f,
                             const char *address,
                             long int timeout,
                             const void *parametr,
                             void *output );


#endif /* ASKUE_PLUGIN_H_ */

