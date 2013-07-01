#ifndef ASKUE_LAUNCHER_LOG_EVENT_H_
#define ASKUE_LAUNCHER_LOG_EVENT_H_

#include <stdarg.h>
#include <libaskue.h>

/*
 * Запись лога событий
 * Запись ведётся в базу
 */ 
bool_t askue_log_event ( const char *format, ... );

#endif /* ASKUE_LAUNCHER_LOG_EVENT_H_ */
