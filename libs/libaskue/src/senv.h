#ifndef
#define

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

#endif /* */
