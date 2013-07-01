#ifndef ASKUE_ERROR_H_
#define ASKUE_ERROR_H_

#include <stdarg.h>

typedef void ( *askue_errlog_method_f ) ( int exit_status,
                                            const char *fmt,
                                            ... );
                                  
void errlog_method ( int exit_status, const char *fmt, ... )
    __attribute__ ((__format__ (__printf__, 3, 4)));

/* 
 * Коды ошибок:
 *  EE_{NAME} статусы выхода
 */
#define EE_SUCCESS EXIT_SUCCESS
#define EE_UNKNOWN EXIT_FAILURE
#define EE_MALLOC 2
#define EE_REALLOC 3
#define EE_LOG_FOPEN 4
#define EE_PRINTF 5 // ошибка какой-то из функций серии printf

#endif /* ASKUE_ERROR_H_ */
