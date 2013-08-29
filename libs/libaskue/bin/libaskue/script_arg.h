#ifndef LIBASKUE_SCRIPT_ARG_H_
#define LIBASKUE_SCRIPT_ARG_H_

#include "cli.h"
#include <stdarg.h>

#define SARG_PORT                   1
#define SARG_LOG                    2
#define SARG_JNL                    3
#define SARG_DEVICE                 4
#define SARG_TIMEOUT                5
#define SARG_PARAMETR               6
#define SARG_FLAG                   7



/*
 * 
 * name: cli_parse_script_arg
 * @param
 *  void * - параметр ( изменяемый )
 *  int - имя аргумента
 *  int - кол-во аргументов в массиве
 *  char ** - массив аргументов
 *  ... - при указании определённых sa_name, требуется функция cli_handler_f
 *        К ним относятся SARG_DEVICE, SARG_PARAMETR  
 * @return
 *  cli_result_t - результат разбора массива аргументов
 * 
 * Инициализация аргумента скрипта. Аргумент указывается в первом параметре.
 * Инициализатор во втором. Если инициализатор == NULL, используется библиотечный.
 * Если библиотечный отсутствует, то возврат -1.
 */
cli_result_t cli_parse_script_arg ( void *sa, int sa_name, int argc, char **argv, ... );


#endif /* LIBASKUE_SCRIPT_ARG_H_ */
