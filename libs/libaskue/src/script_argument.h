#ifndef ASKUE_SCRIPT_ARG_H_
#define ASKUE_SCRIPT_ARG_H_

typedef struct _port_arg_t
{
    const char *File;
    const char *Speed;
    const char *DBits;
    const char *SBits;
    const char *Parity;
} port_arg_t;

typedef struct _device_arg_t
{
    const char *Name;
    const char *Timeout;
} device_arg_t;

typedef struct _journal_arg_t
{
    const char *File;
    const char *Flashback;
} journal_arg_t;

typedef struct _log_arg_t
{
    const char *File;
    // char *Mode; == 'a' ВСЕГДА
} log_arg_t;
 
typedef struct _parametr_arg_t
{
    const char *Value;
} parametr_arg_t;

typedef struct _script_arg_t
{
    port_arg_t *Port;
    device_arg_t *Device;
    journal_arg_t *Journal;
    log_arg_t *Log;
    parametr_arg_t *Parametr;
} script_arg_t;

// создать набор аргументов скрипта
void script_arg_init ( script_arg_t *SArg );

// удалить набор аргументов скрипта
void script_arg_destroy ( script_arg_t *SArg );

#endif /* ASKUE_SCRIPT_ARG_H_ */
