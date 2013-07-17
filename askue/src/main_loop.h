#ifndef MAIN_LOOP_H_
#define MAIN_LOOP_H_

typedef struct
{
    const char *Option;
    const char *Value;
} script_argument_t;

#define SA_END ( (script_argument_t) { NULL, NULL} )

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

// основной цикл программы
void main_loop ( const askue_cfg_t *ACfg );

#endif /* MAIN_LOOP_H_ */
