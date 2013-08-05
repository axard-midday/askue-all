#ifndef SCRIPT_ENVIRPMENT_H_
#define SCRIPT_ENVIRPMENT_H_

#include <stdlib.h>
#include <stdint.h>

#include "port.h"

typedef struct _script_enviroment_t
{
    askue_port_t *Port;
    FILE *Log;
    sqlite3 *Journal;
    uint32_t Flashback;
    uint32_t Device;
    uint32_t Timeout;
    void *Parametr;
    uint32_t Flag;
} script_enviroment_t;

// инициализация окружения
int script_env_init ( script_enviroment_t *Env, int argc, char **argv, void* ( get_param ) ( const char * ) );

// удаление окружения
void script_env_destroy ( script_enviroment_t *Env );

#endif /* SCRIPT_ENVIRPMENT_H_ */
