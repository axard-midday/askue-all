#ifndef ASKUE_LAUNCHER_FUNC_H_
#define ASKUE_LAUNCHER_FUNC_H_

#include <libaskue.h>

/*
 * Установить флаг режима отладки
 */
bool_t argv_function_debug ( const char *arg_value );

/*
 * Вывод краткой справки в терминал
 */
bool_t argv_function_help ( const char *arg_value );

/*
 * Установить флаг режима вывода на экран терминала связи
 */
bool_t argv_function_terminal ( const char *arg_value );

/*
 * Установить флаг режима ограничения лога
 */
bool_t argv_function_loglines ( const char *arg_value );

/*
 * Создание отчётов
 */
bool_t argv_function_report ( const char *arg_value );

#endif /* ASKUE_LAUNCHER_OPT_H_ */