#ifndef SCRIPT_ARGUMENT_H_
#define SCRIPT_ARGUMENT_H_


#include <stdarg.h>
#include <stdint.h>

#include "config.h"

#define SA_NAME 0
#define SA_PORT_FILE SA_NAME + 1
#define SA_PORT_PARITY SA_NAME + 2
#define SA_PORT_DBITS SA_NAME + 3
#define SA_PORT_SBITS SA_NAME + 4
#define SA_PORT_SPEED SA_NAME + 5
#define SA_DEVICE SA_NAME + 6
#define SA_PARAMETR SA_NAME + 7
#define SA_TIMEOUT SA_NAME + 8
#define SA_JOURNAL_FILE SA_NAME + 9
#define SA_JOURNAL_FLASHBACK SA_NAME + 10
#define SA_LOG_FILE SA_NAME + 11
#define SA_PROTOCOL SA_NAME + 12
#define SA_VERBOSE SA_NAME + 13

#define SA_LAST SA_VERBOSE + 1
#define SA_FIRST SA_NAME

#define SA_AMOUNT SA_LAST

#define SA_LENGTH 256

#define SA_PRESET_CLEAR 0
#define SA_PRESET_DEVICE 1
#define SA_PRESET_REPORT 2

typedef struct
{
    char Value[ SA_AMOUNT ][ SA_LENGTH ];
    uint32_t Flag;
} script_argument_vector_t;

// установить опцию
int script_argument_set ( script_argument_vector_t *sargv, int _sa, ... );

// снять флаг опции
void script_argument_unset ( script_argument_vector_t *sargv, int _sa );

// инициализация скрипта
void script_argument_init ( script_argument_vector_t *sargv, const askue_cfg_t *Cfg, int _flag );

#endif /* SCRIPT_ARGUMENT_H_ */
