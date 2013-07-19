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

typedef enum
{
    Askue_NoSegment,
    Askue_Remote,
    Askue_Local
} device_segment_t;

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
    char *Mode;
    size_t Lines;
} log_cfg_t;

typedef struct 
{
    char *File;
    size_t Size;
    size_t Flashback;
} journal_cfg_t;

typedef struct
{
    char *Name;
    char **Script;
} type_cfg_t;

typedef struct
{
    char *Name;
    long int Id;
    long int Timeout;
    device_class_t Class;
    device_segment_t Segment;
    type_cfg_t *Type;
} device_cfg_t;

typedef struct
{
    char *Name;
} report_cfg_t;

typedef struct
{
    long int Id;
    device_cfg_t *Device;
} gate_cfg_t;

typedef struct
{
    port_cfg_t *Port;
    log_cfg_t *Log;
    journal_cfg_t *Journal;
    gate_cfg_t **GateList;
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
