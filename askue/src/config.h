#ifndef ASKUE_CONFIG_H_
#define ASKUE_CONFIG_H_

#include <libconfig.h>
#include <stdint.h>

typedef struct _device_cfg_t
{
    char                        *Name;
    char                        *Type;
    uint32_t                    Timeout;
    long int                    Id;
} device_cfg_t;

typedef struct _script_cfg_t
{
    char                        *Name;
    char                        *Parametr;
} script_cfg_t;

typedef struct _comm_cfg_t
{
    device_cfg_t                *Device;
    script_cfg_t                *Script;
} comm_cfg_t;

typedef struct _task_cfg_t
{
    char                        *Target;
    script_cfg_t               *Script;
} task_cfg_t;

typedef struct _port_cfg_t
{
    char                        *File;
    char                        *Speed;
    char                        *DBits;
    char                        *SBits;
    char                        *Parity;
} port_cfg_t;

typedef struct _log_cfg_t
{
    char                        *File;
    char                        *Mode;
    size_t                      Lines;
} log_cfg_t;

typedef struct _journal_cfg_t
{
    char                        *File;
    size_t                      Size;
    size_t                      Flashback;
} journal_cfg_t;

typedef struct _askue_cfg_t
{
    device_cfg_t              *Device;
    comm_cfg_t                *Comm;
    task_cfg_t                *Task;
    port_cfg_t                *Port;
    log_cfg_t                 *Log;
    journal_cfg_t             *Journal;
    long int                 *Network; 
    uint32_t                 Flag;  
} askue_cfg_t;


// инициализироать переменную конфигурации
void askue_config_init ( askue_cfg_t *ACfg );

// прочитать конфигурацию из файла
int askue_config_read ( askue_cfg_t *ACfg );

// удалить память выделенную под конфигурацию
void askue_config_destroy ( askue_cfg_t *ACfg );

#endif /* ASKUE_CONFIG_H_ */
