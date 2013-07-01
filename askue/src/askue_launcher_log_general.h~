#ifndef ASKUE_LAUNCHER_LOG_GENERAL_H_
#define ASKUE_LAUNCHER_LOG_GENERAL_H_

#include <libaskue.h>
#include <stdarg.h>

typedef bool_t ( *_log_func_t ) ( const char* );

/*
 * Оформить сообщение для записи
 */
char* _log_get_msg ( const char *_format, va_list _args );

/*
 * Функции записи в лог
 */
bool_t _log_ ( _log_func_t _log_func, const char *_format, va_list _args );

#endif /* ASKUE_LAUNCHER_LOG_GENERAL_H_ */
