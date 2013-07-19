#ifndef ASKUE_SCRIPT_OPTION_H_
#define ASKUE_SCRIPT_OPTION_H_

#include <stdarg.h>
#include <stdint.h>

#define SA_PORT_FILE 0
#define SA_PORT_PARITY 1
#define SA_PORT_DBITS 2
#define SA_PORT_SBITS 3
#define SA_PORT_SPEED 4
#define SA_DEVICE 5
#define SA_PARAMETR 6
#define SA_TIMEOUT 7
#define SA_JOURNAL_FILE 8
#define SA_JOURNAL_FLASHBACK 9
#define SA_LOG_FILE 10

#define SA_AMOUNT 11

#define SA_LENGTH 256

typedef struct
{
    char Value[ SA_AMOUNT ][ SA_LENGTH ];
    uint32_t Flag;
} script_option_t;

// установит опцию скрипта
int script_option_set ( script_option_t *SOpt, int SOptNumber, ... );

// снять опцию скрипта
void script_option_unset ( script_option_t *SOpt, int SOptNumber );

// инициализировать опции
void script_option_init ( script_option_t *SOpt );

#endif /* ASKUE_SCRIPT_OPTION_H_ */
