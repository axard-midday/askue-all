#ifndef ASKUE_LIB_TM_plugin_H_
#define ASKUE_LIB_TM_plugin_H_

#include "askue_container.h"
#include "uint8_array.h"
#include <stdarg.h>

/******************************************************************************/
			/*
			 * Тело скрипта
			 */
/******************************************************************************/

// доступ к порту ввода вывода
typedef uint8_array_t* ( *askue_ioport_method_f ) ( const uint8_array_t *, long int );

// обработчик обращений к базе
typedef int ( *askue_db_callback_f ) ( void*, int, char **, char ** );

// доступ к базе данных
typedef bool_t ( *askue_db_method_f ) ( const char *, askue_db_callback_f, void * );

// доступ к логу событий.
typedef void ( *askue_log_method_f ) ( const char *fmt, ... );

// интерфейсы предоставляемые плагину со стороны аскуэ
typedef struct
{
        askue_ioport_method_f   io_method;
        askue_db_method_f       db_method;
        askue_log_method_f      log_method;
} askue_plugin_interface_t;

// аргументы предоставляемые плагину
typedef struct
{
     const char *address;
     const char *timeout;
     const char *parametr;
} askue_plugin_argument_vector_t;

// плагин
typedef void ( *askue_plugin_f ) ( const askue_plugin_interface_t *interface,
                                     const askue_plugin_argument_vector_t *argv );

// сформировать массив байт для отправки
typedef uint8_array_t* askue_plugin_get_request_f ( const char*, const char*, char ** );

// проверить контрольную сумму
typedef bool_t askue_plugin_valid_checksum_f ( const uint8_array_t*, char** );

// проверить содержимое
typedef bool_t askue_plugin_valid_content_f ( const uint8_array_t*, const uint8_array_t*, char** );

// получить результат
typedef bool_t askue_plugin_get_result_f ( const uint8_array_t*, askue_container_t* );

// получение пароля
// реализуется в самом плагине
/*
typedef char ( *askue_plugin_get_password_f ) ( askue_db_method_f, 
                               const askue_plugin_argument_vector_t * );
*/

typedef bool_t ( *askue_plugin_method_f ) ( askue_ioport_method_f, 
                                                    const askue_plugin_argument_vector_t *,
                                                    askue_plugin_get_request_f, 
                                                    askue_plugin_valid_checksum_f,
                                                    askue_plugin_valid_content_f,
                                                    askue_plugin_get_result_f,
                                                    askue_container_t *output,
                                                    char **error_string );

// процедура обмены через порт ввода-вывода
bool_t askue_plugin_method ( askue_ioport_method_f, 
                           const askue_plugin_argument_vector_t *,
                           askue_plugin_get_request_f __get_request, 
		           askue_plugin_valid_checksum_f __valid_checksum,
		           askue_plugin_valid_content_f __valid_content,
		           askue_plugin_get_result_f __get_result,
		           askue_container_t *output,
		           char **error_string );
		  
// процедура сохранения данных
// реализуется в самом плагине
/*
typedef void ( *askue_plugin_save_result ) ( askue_db_method_f, 
                          const askue_plugin_argument_vector_t *, 
                          const askue_container_t *input );
*/


#endif /* ASKUE_LIB_TM_plugin_H_ */

