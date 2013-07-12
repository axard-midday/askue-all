#ifndef ASKUE_CONFIG_H_
#define ASKUE_CONFIG_H_

#include <libconfig.h>

#define ASKUE_CONFIG_FILE "askue.cfg"

/*
 * Типы можно было бы сделать и более сжато,
 * но при таком построении будет легче
 * осуществлять нововведения
 */

typedef enum
{
    Askue_NoClass,
    Askue_Counter,
    Askue_Modem
} device_class_t;

typedef struct
{
    char *File;
    char *Speed;
    char *DBits;
    char *SBits;
    char *Parity;
} port_cfg_t;

typedef struct
{
    char *File;
    size_t Lines;
} log_cfg_t;

typedef struct 
{
    char *File;
} db_cfg_t;

typedef struct
{
    char *Name;
    char **Script;
} type_cfg_t;

typedef struct
{
    char *Id;
    long int Timeout;
    type_cfg_t *Type;
} device_cfg_t;

typedef struct
{
    char *Name;
} report_cfg_t;

typedef struct
{
    port_cfg_t *Port;
    log_cfg_t *Log;
    db_cfg_t *DB;
    device_cfg_t **DeviceList;
    type_cfg_t **TypeList;
    report_cfg_t **ReportList;
} askue_cfg_t;

// инициализироать переменную конфигурации
void askue_config_init ( askue_cfg_t *ACfg );

// прочитать конфигурацию из файла
int askue_config_read ( askue_cfg_t *ACfg );

// удалить память выделенную под конфигурацию
void askue_config_destroy ( askue_cfg_t *ACfg );

#endif /* ASKUE_CONFIG_H_ */