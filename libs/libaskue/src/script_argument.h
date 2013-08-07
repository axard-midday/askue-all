#ifndef ASKUE_SCRIPT_ARGUMENT_H_
#define ASKUE_SCRIPT_ARGUMENT_H_

#include <stdint.h>

typedef struct _port_arg_t
{
    char *File;
    char *Speed;
    char *DBits;
    char *SBits;
    char *Parity;
} port_arg_t;

typedef struct _device_arg_t
{
    char *Name;
    char *Timeout;
    char *Parametr;
} device_arg_t;

typedef struct _journal_arg_t
{
    char *File;
    char *Flashback;
} journal_arg_t;

typedef struct _log_arg_t
{
    char *File;
} log_arg_t;

typedef struct _script_arg_t
{
    port_arg_t *Port;
    journal_arg_t *Journal;
    log_arg_t *Log;
    device_arg_t *Device;
    uint32_t Flag;
} script_arg_t;

int script_arg_init ( script_arg_t *Arg, int argc, char **argv );

void script_arg_delete ( script_arg_t *Arg );

void script_arg_new ( script_arg_t **Arg );

#endif /* ASKUE_REGISTRATION_H_ */
