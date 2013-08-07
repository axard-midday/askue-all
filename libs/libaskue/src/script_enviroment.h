#ifndef ASKUE_SCRIPT_ENVIRPMENT_H_
#define ASKUE_SCRIPT_ENVIRPMENT_H_

#include <stdlib.h>
#include <stdint.h>
#include <sqlite3.h>

#include "port.h"
#include "script_argument.h"

typedef struct _script_env_t
{
    askue_port_t *Port;
    FILE *Log;
    sqlite3 *Journal;
    uint32_t Flashback;
    uint32_t Device;
    uint32_t Timeout;
    void *Parametr;
    uint32_t Flag;
} script_env_t;

// выделить память под окружение
void script_env_new ( script_env_t **Env );

// инициализация окружения
int script_env_init ( script_env_t *Env, const script_arg_t *Arg, void* ( get_param ) ( const char * ) );

// удаление окружения
void script_env_destroy ( script_env_t *Env, void (*destroy_param) (void*) );

// очистить память выделенную под окружение
void script_env_delete ( script_env_t *Env );

#endif /* ASKUE_SCRIPT_ENVIRPMENT_H_ */
