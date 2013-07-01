#ifndef ASKUE_LAUNCHER_LOG_ERROR_H_
#define ASKUE_LAUNCHER_LOG_ERROR_H_

#include <stdarg.h>
#include <libaskue.h>

/*
 * Запись лога ошибок
 * Запись ведётся в файл
 */ 
void askue_log_error ( const char *format, ... );

#endif /* ASKUE_LAUNCHER_LOG_ERROR_H_ */
