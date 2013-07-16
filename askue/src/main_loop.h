#ifndef MAIN_LOOP_H_
#define MAIN_LOOP_H_

typedef struct
{
    const char *Arg;
    const char *Value;
} script_argument_t;

#define SCRIPT_ARGUMENT_END ( (script_argument_t) { NULL, NULL} )

#define SCRIPT_ARGUMENT_PORT_FILE 0
#define SCRIPT_ARGUMENT_PORT_PARITY 1
#define SCRIPT_ARGUMENT_PORT_DBITS 2
#define SCRIPT_ARGUMENT_PORT_SBITS 3
#define SCRIPT_ARGUMENT_PORT_SPEED 4
#define SCRIPT_ARGUMENT_DEVICE 5
#define SCRIPT_ARGUMENT_PARAMETR 6
#define SCRIPT_ARGUMENT_TIMEOUT 7
#define SCRIPT_ARGUMENT_JOURNAL_FILE 8
#define SCRIPT_ARGUMENT_JOURNAL_FLASHBACK 9

// основной цикл программы
void main_loop ( const askue_cfg_t *ACfg );

#endif /* MAIN_LOOP_H_ */
