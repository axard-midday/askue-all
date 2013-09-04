#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libconfig.h>
#include <libaskue.h>
#include <ctype.h>

#include "config.h"

// результат выполнения - ошибка
#ifndef ASKUE_ERROR
    #define ASKUE_ERROR -1
#endif 

// результат выполнения - успех
#ifndef ASKUE_SUCCESS
    #define ASKUE_SUCCESS 0
#endif

#ifndef _ASKUE_TBUFLEN
    #define _ASKUE_TBUFLEN 512
#endif

#define askue_config_fail( msg ) write_msg ( stderr, "Конфигурация", "FAIL", msg );
#define askue_config_error( msg ) write_msg ( stderr, "Конфигурация", "ERROR", msg );
#define askue_config_ok( msg ) write_msg ( stdout, "Конфигурация", "OK", msg );

/**********************************************************************/

// получить описание журнала
static
const config_setting_t* _get_journal ( const config_t *cfg )
{
    // корневая метка конфигурации журнала
    config_setting_t *setting = config_lookup ( cfg, "Journal" ); // поиск сети
    if ( setting == NULL )
    {
        // сообщение об ошибке
        askue_config_fail ( "В конфигурации отсутствует запись 'Journal'" );
        return ( const config_setting_t* ) NULL;
    }
    return ( const config_setting_t* ) setting;
}

// получить имя файла журнала ( базы данных )
static
const char* _get_journal_file ( const config_setting_t* setting )
{
    const char *JnlFile;
    if ( config_setting_lookup_string ( setting, "file", &(JnlFile) ) != CONFIG_TRUE )
    {
        // сообщение об ошибке
        askue_config_fail ( "Запись 'Journal' не полная" );
        return ( const char* ) NULL;
    }
    return JnlFile;
}

// получить размер журнала
static
const char* _get_journal_size ( const config_setting_t* setting, int Verbose )
{
    const char *JnlSize;
    if ( config_setting_lookup_string ( setting, "size", &(JnlSize) ) != CONFIG_TRUE )
    {
        // доп.сообщение
        if ( Verbose )
        {
            askue_config_error ( "Отсутствует запись 'Journal.size'" );
            askue_config_ok ( "Установка значения по умолчанию 'Journal.size = 3'" );
        }
        return ( const char* ) NULL;
    }
    return JnlSize;
}

// установить значение размера журнала
static
size_t _set_journal_size ( const config_setting_t* setting, int Verbose )
{
    const char *JnlSize = _get_journal_size ( setting, Verbose );
    if ( JnlSize == NULL)
    {
        // значение по умолчанию
        return ( size_t ) 3;
    }
    else
    {
        // значение из конфига
        return (size_t) strtoul ( JnlSize, NULL, 10 );
    }
}

// прочитать кол-во флешбеков
static
const char* _get_journal_flashback ( const config_setting_t* setting, int Verbose )
{
    const char *JnlFlashback;
    if ( config_setting_lookup_string ( setting, "flashback", &(JnlFlashback) ) != CONFIG_TRUE )
    {
        // доп.сообщение
        if ( Verbose )
        {
            askue_config_error ( "Отсутствует запись 'Journal.flashback'" );
            askue_config_ok ( "Установка значения по умолчанию 'Journal.flashback = 0'" );
        }
        return ( const char* ) NULL;
    }
    else
    {
        return JnlFlashback;
    }
}

// установить кол-во флешбеков
static
size_t _set_journal_flashback ( const config_setting_t* setting, int Verbose )
{
    const char *JnlFlashback = _get_journal_flashback ( setting, Verbose );
    if ( JnlFlashback == NULL )
    {
        // значение по умолчанию
        return ( size_t ) 0;
    }
    else
    {
        // значение из конфига
        return (size_t) strtoul ( JnlFlashback, NULL, 10 );
    }
}

// чтение конфигурации базы
static
int __config_read_journal ( const config_t *cfg, askue_cfg_t *ACfg )
{
    // флаг дополнительных сообщений
    int Verbose = TESTBIT ( ACfg->Flag, ASKUE_FLAG_VERBOSE );
    // данные о журнале
    const config_setting_t *Jnl = _get_journal ( cfg );
    if ( Jnl == NULL ) return ASKUE_ERROR;
    // файл журнала
    const char *JnlFile = _get_journal_file ( Jnl );
    if ( JnlFile == NULL ) return ASKUE_ERROR;
    // память под журнал
    ACfg->Journal = mymalloc ( sizeof ( journal_cfg_t ) );
    // размер в днях 
    ACfg->Journal->Size = _set_journal_size ( Jnl, Verbose );
    // флешбек в днях
    ACfg->Journal->Flashback = _set_journal_flashback ( Jnl, Verbose );
    // значение из конфига
    ACfg->Journal->File = mystrdup ( JnlFile );
    // доп.сообщение
    if ( Verbose )
        askue_config_ok ( "Настройки журнала успешно считаны." );
     
    return ASKUE_SUCCESS;
}

/**********************************************************************/

// поиск описания в конфиге лога
static
const config_setting_t* _get_log ( const config_t *cfg )
{
    config_setting_t *setting = config_lookup ( cfg, "Log" ); // поиск сети
    if ( setting == NULL )
    {
        // сообщение об ошибке
        askue_config_fail ( "В конфигурации отсутствует запись 'Log'" );
        return ( const config_setting_t* ) NULL;
    }
    return ( const config_setting_t* ) setting;
}

// получить параметры лога
static
int _get_log_params ( const config_setting_t *setting, const char **File, const char **Lines, const char **Mode, int Verbose )
{
    if ( config_setting_lookup_string ( setting, "file", File ) == CONFIG_FALSE )
    {
        askue_config_fail ( "Отсутствует запись 'Log.file'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( setting, "mode", Mode ) == CONFIG_FALSE )
    {
        askue_config_fail ( "Отсутствует запись 'Log.Mode'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( setting, "lines", Lines ) == CONFIG_FALSE )
    {
        askue_config_error ( "Отсутствует запись 'Log.lines'." );
        if ( Verbose ) 
            askue_config_ok ( "Установка значения по умолчанию 'Log.lines = 1000'" );
        *Lines = NULL;
        return ASKUE_SUCCESS;
    }
    else
    {
        return ASKUE_SUCCESS;
    }
}

// установить параметры
static
void _set_log_params ( askue_cfg_t *ACfg, const char *File, const char *Lines, const char *Mode )
{
    ACfg->Log->File = mystrdup ( File );
    ACfg->Log->Lines = ( size_t ) ( ( Lines != NULL ) ? strtoul ( Lines, NULL, 10 ) : 1000 );
    ACfg->Log->Mode = mystrdup ( Mode );
}

// чтение конфигурации лога
static
int __config_read_log ( const config_t *cfg, askue_cfg_t *ACfg )
{
    // флаг дополнительных сообщений
    int Verbose = TESTBIT ( ACfg->Flag, ASKUE_FLAG_VERBOSE );
    // корневая структура конфигурации лога
    const config_setting_t *Log = _get_log ( cfg );
    if ( Log == NULL ) return ASKUE_ERROR;
    // поиск параметров лога
    const char *File, *Lines, *Mode;
    if ( _get_log_params ( Log, &File, &Lines, &Mode, Verbose ) == ASKUE_ERROR )
    {
        return ASKUE_ERROR;
    }
    // выделить память 
    ACfg->Log = mymalloc ( sizeof ( log_cfg_t ) );
    // скопировать данные из текстового конфига
    _set_log_params ( ACfg, File, Lines, Mode );
    // дополнительное сообщение
    if ( Verbose )
        askue_config_ok ( "Настройки лога успешно считаны." );
     
    return ASKUE_SUCCESS;
}

/**********************************************************************/

// поиск описания порта в конфигурации
static
const config_setting_t* _get_port ( const config_t *cfg )
{
    config_setting_t *setting = config_lookup ( cfg, "Port" );
    if ( setting == NULL )
    {
        // сообщение об ошибке
        askue_config_fail ( "В конфигурации отсутствует запись 'Port'." );
        return ( const config_setting_t* ) NULL;
    }
    return ( const config_setting_t* ) setting;
}

// поиск параметров в конфиге
static 
int _get_port_params ( const config_setting_t *port_setting, const char **File, 
                                                              const char **DBits, 
                                                              const char **SBits, 
                                                              const char **Parity,
                                                              const char **Speed )
{
    if ( config_setting_lookup_string ( port_setting, "file", File ) == CONFIG_FALSE )
    {
        askue_config_fail ( "Отсутствует запись 'Port.file'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( port_setting, "dbits", DBits ) == CONFIG_FALSE )
    {
        askue_config_fail ( "Отсутствует запись 'Port.databits'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( port_setting, "sbits", SBits ) == CONFIG_FALSE )
    {
        askue_config_fail ( "Отсутствует запись 'Port.stopbits'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( port_setting, "parity", Parity ) == CONFIG_FALSE )
    {
        askue_config_fail ( "Отсутствует запись 'Port.parity'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( port_setting, "speed", Speed ) == CONFIG_FALSE )
    {
        askue_config_fail ( "Отсутствует запись 'Port.speed'." );
        return ASKUE_ERROR;
    }
    else
    {
        return ASKUE_SUCCESS;
    }
}

// установить параметры
static
void _set_port_params ( askue_cfg_t *ACfg, const char *File,
                                             const char *DBits,
                                             const char *SBits,
                                             const char *Parity,
                                             const char *Speed )
{
    // Заполнить память данными
    ACfg->Port->DBits = mystrdup ( DBits );
    ACfg->Port->SBits = mystrdup ( SBits );
    ACfg->Port->Speed = mystrdup ( Speed );
    ACfg->Port->Parity = mystrdup ( Parity );
    ACfg->Port->File = mystrdup ( File );
}

// чтение конфигурации порта
static
int __config_read_port ( const config_t *cfg, askue_cfg_t *ACfg )
{
    // флаг дополнительных сообщений
    int Verbose = TESTBIT ( ACfg->Flag, ASKUE_FLAG_VERBOSE );
    // поиск корневой структуры конфигурации порта
    const config_setting_t *Port = _get_port ( cfg );
    if ( Port == NULL ) return ASKUE_ERROR;
    // параметры порта
    const char *File, *DBits, *SBits, *Parity, *Speed;
    // поиск параметров порта
    if ( _get_port_params ( Port, &File, &DBits, &SBits, &Parity, &Speed ) == ASKUE_ERROR )
        return ASKUE_ERROR;
    // выделить память
    ACfg->Port = mymalloc ( sizeof ( port_cfg_t ) );
    // заполнить параметры
    _set_port_params ( ACfg, File, DBits, SBits, Parity, Speed );
    
    // доп. сообщение 
    if ( Verbose )
        askue_config_ok ( "Настройки порта успешно считаны." );
     
    return ASKUE_SUCCESS;
}

/**********************************************************************/

// поиск параметров конкретного устройства
static 
int _get_the_device_param ( const config_setting_t *next_device, long int *Id,
                                                                  const char **Name,
                                                                  const char **Type,
                                                                  const char **Timeout, int Verbose )
{
    if ( config_setting_lookup_int ( next_device, "id", Id ) == CONFIG_FALSE )
    {
        askue_config_fail ( "Отсутствует запись 'Devices.id'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( next_device, "name", Name ) == CONFIG_FALSE )
    {
        askue_config_fail ( "Отсутствует запись 'Devices.name'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( next_device, "type", Type ) == CONFIG_FALSE )
    {
        askue_config_fail ( "Отсутствует запись 'Devices.type'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( next_device, "timeout", Timeout ) == CONFIG_FALSE )
    {
        askue_config_error ( "Отсутствует запись 'Devices.timeout'." );
        if ( Verbose ) 
            askue_config_ok ( "Установка значения по умолчанию 'Devices.timeout = 5000'" );
        *Timeout = NULL;
        return ASKUE_SUCCESS;
    }
    else
    {
        return ASKUE_SUCCESS;
    }
}
            
// установка параметров устройства
static
void _set_the_device_param ( device_cfg_t *Device, const char *Name, 
                                                     const char *Type,
                                                     const char *Timeout,
                                                     long int Id, )
{
    Device->Name = mystrdup ( Name );
    Device->Type = mystrdup ( Type );
    Device->Timeout = ( Timeout != NULL ) ? ( uint32_t ) strtoul ( Timeout, NULL, 10 ) : 5000;
    Device->Id = Id;
}
                                                
// разбор описания устройства
static
int __config_read_the_device ( const config_setting_t *next_device, askue_cfg_t *ACfg, size_t DeviceAmount )
{
    // поиск обязательных параметров: id, name, type
    const char *Name, *Type, *Timeout;
    long int Id;
    if ( _get_the_device_param ( next_device, &Id, &Name, &Type, &Timeout, TESTBIT ( ACfg->Flag, ASKUE_FLAG_VERBOSE ) ) == ASKUE_ERROR )
        return ASKUE_ERROR;
    // определить id
    if ( Id > DeviceAmount - 1 ) // id выходит за границы
    {
        // сообщение об ошибке
        askue_config_fail ( "Запись 'Device.id' выходит за границы ( кол-во элементов записи 'Device' )." );
        return ASKUE_ERROR;
    }
    // копировать данные в конфигурацию
    _set_the_device_param ( ACfg->Device + Id, Name, Type, Timeout, Id );
    
    return ASKUE_SUCCESS;
}

// поиск описания списка устройств
static
const config_setting_t* _get_device_list ( const config_t *cfg )
{
    config_setting_t *setting = config_lookup ( cfg, "Devices" );
    if ( setting == NULL )
    {
        // сообщение об ошибке
        askue_config_fail ( "В конфигурации отсутствует запись 'Devices'." );
        return ( const config_setting_t* ) NULL;
    }
    else
    {
        return ( const config_setting_t* ) setting;
    }
}

// инициализировать конкретное устройство
static
void __config_init_the_device ( device_cfg_t *TheDevice )
{
    TheDevice->Name = NULL;
    TheDevice->Type = NULL;
    TheDevice->Timeout = 0;
    TheDevice->Id = 0;
}

// инициализировать устройства
static
void __config_init_device ( askue_cfg_t *ACfg, size_t DeviceAmount )
{
    for ( size_t i = 0; i < DeviceAmount; i++ )
    {
        __config_init_the_device ( ACfg->Device + i );
    }
}

// чтение конфигурации списка устройств
static
int __config_read_device ( const config_t *cfg, askue_cfg_t *ACfg )
{
    // корневая структура списка устройств
    const config_setting_t *DeviceList = _get_device_list ( cfg );
    if ( DeviceList == NULL ) return ASKUE_ERROR;
    // кол-во устройств в списке
    size_t Amount = config_setting_length ( DeviceList ) + 2;
    // выделить память
    ACfg->Device = ( device_cfg_t* ) mymalloc ( sizeof ( device_cfg_t ) * ( Amount ) );
    
    __config_init_device ( ACfg, Amount );
    
    // Заполнить память
    // Нулевое устройство - это АСКУЭ
    ACfg->Device->Name = mystrdup ( "АСКУЭ" );
    ACfg->Device->Type = mystrdup ( "Сервер задач" );
    ACfg->Device->Timeout = 0;
    ACfg->Device->Id = 0;
    // остальные устройства
    int Result = ASKUE_SUCCESS; // результат работы
    for ( size_t i = 0; i < Amount && Result != ASKUE_ERROR; i++ )
    {
        // описание устройства
        config_setting_t *next_device = config_setting_get_elem ( DeviceList, i );
        // разбор описания устройства
        Result = __config_read_the_device ( next_device, ACfg, Amount );
    }
    return Result;
}

/**********************************************************************/

// поиск описания сети
static
const config_setting_t* _get_network ( const config_t *cfg )
{
    config_setting_t *Network = config_lookup ( cfg, "Network" );
    if ( Network == NULL )
    {
        // сообщение об ошибке
        askue_config_fail ( "В конфигурации отсутствует запись 'Network'." );
        return ( const config_setting_t* ) Network;
    }
    else
    {
        return ( const config_setting_t* ) NULL;
    }
}

// читать структу сети
static
int __config_read_network ( const config_t *cfg, askue_cfg_t *ACfg )
{
    // корневая структура описывающая сеть
    const config_setting_t* Network = _get_network ( cfg );
    if ( Network == NULL ) return ASKUE_ERROR;
    // размер сети
    size_t Size = config_setting_length ( Network );
    ACfg->NetworkSize = Size + 1;
    // выделить память
    ACfg->Network = mymalloc ( ( Size + 1 ) * sizeof ( uint32_t ) );
    // перебор всех рёбер графа
    for ( size_t i = 0; i < Size; i++ )
    {
        // получить ребро графа
        config_setting_t *edge = config_setting_get_elem ( Network, i );
        // id устройства через которое подключаемся
        int Base = config_setting_get_int_elem ( edge, 0 );
        // id устройства к которому подключаемся
        int Remote = config_setting_get_int_elem ( edge, 1 );
        // запись структуры в конфиг
        ACfg->Network[ Remote ] = ( uint32_t ) Base;
    }
    return ASKUE_SUCCESS;
}

/**********************************************************************/

// разделение скрипта на имя и параметр
static
void _get_script ( char **ScriptName, char **ScriptParametr, const char *script )
{
    const char *Parametr = strchr ( script, ':' );
    
    if ( Parametr != NULL )
    {
        size_t len = ( size_t ) ( ( const char* ) Parametr - ( const char* ) script );
        *ScriptName = mystrndup ( script, len );
        
        Parametr++;
        while ( *Parametr != '\0' && isspace ( *Parametr ) )
            Parametr++;
        *ScriptParametr = mystrdup ( Parametr );
    }
    else
    {
        *ScriptParametr = NULL;
        *ScriptName = mystrdup ( script );
    }
}

// чтение скрипта и его параметра
static
script_cfg_t __config_script ( const char *script )
{
    char *ScriptName, *ScriptParametr;
    _get_script ( &ScriptName, &ScriptParametr, script );
    return ( script_cfg_t ) { ScriptName, ScriptParametr };
}

/**********************************************************************/

// поиск параметров коммуникации
static
int _get_the_comm_params ( const config_setting_t *TheComm, long int *Id,
                                                             const char **Script )
{
    if ( config_setting_lookup_int ( TheComm, "id", Id ) == CONFIG_FALSE )
    {
        // сообщение об ошибке
        askue_config_fail ( "Отсутствует запись 'Communication.id'." );
        return ASKUE_ERROR;
    }
    else if ( config_setting_lookup_string ( TheComm, "script", Script ) == CONFIG_FALSE )
    {
        // сообщение об ошибке
        askue_config_fail ( "Отсутствует запись 'Communication.script'." );
        return ASKUE_ERROR;
    }
    else
    {
        return ASKUE_SUCCESS;
    }
}

// прочитать способ коммуникации
static
int __config_read_the_comm ( const config_setting_t *TheComm, askue_cfg_t *ACfg, size_t Index )
{
    const char *Script;
    long int Id;
    if ( _get_the_comm_params ( TheComm, &Id, &Script ) == ASKUE_ERROR ) return ASKUE_ERROR;
    
    // определить id
    // присвоить коммуникации описание устройства
    comm_cfg_t *Comm = ACfg->Comm + Index;
    Comm->Device = ACfg->Device + Id;
    // прочитать скрипт для выполнения соединения
    Comm->Script = mymalloc ( sizeof ( script_cfg_t ) );
    *( Comm->Script ) = __config_script ( Script );
    
    return ASKUE_SUCCESS;
}

// поиск списка коммуникаций
static
const config_setting_t* _get_comm ( const config_t *cfg )
{
    config_setting_t *Communication = config_lookup ( cfg, "Communication" );
    if ( Communication == NULL )
    {
        // сообщение об ошибке
        askue_config_fail ( "В конфигурации отсутствует запись 'Communication'." );
        return ( const config_setting_t* ) NULL;
    }
    return ( const config_setting_t* ) Communication;
}

// конфигурировать следующую коммуникацию
static
int __config_next_comm ( const config_setting_t *Comm, askue_cfg_t *ACfg, size_t offset )
{
    config_setting_t *TheComm = config_setting_get_elem ( Comm, offset );
    return __config_read_the_comm ( TheComm, ACfg, offset );
}

// конфигурирование всех коммуникаций
static 
int __config_each_comm ( const config_setting_t *Comm, askue_cfg_t *ACfg )
{
    int Result = ASKUE_SUCCESS;
    for ( size_t i = 0; i < ACfg->CommAmount && Result != ASKUE_ERROR; i++ )
    {
        Result = __config_next_comm ( Comm, ACfg, i );
    }
    return Result;
}

// инициализировать память конкретной конфигурации
static
void __config_init_the_comm ( comm_cfg_t *TheComm )
{
    TheComm->Device = NULL;
    TheComm->Script = NULL;
}

// инициализировать память, выделенную под коммуникации
static
void __config_init_comm ( askue_cfg_t *Cfg )
{
    for ( size_t i = 0; i < Cfg->CommAmount; i++ )
    {
        __config_init_the_comm ( Cfg->Comm + i );
    }
}

// читать структу коммуникаций
static
int __config_read_comm ( const config_t *cfg, askue_cfg_t *ACfg )
{
    // корневая структура описывающая коммуникации
    const config_setting_t *Communication = _get_comm ( cfg );
    if ( Communication == NULL ) return ASKUE_ERROR;
    
    // кол-во способов коммуникации
    size_t Amount = config_setting_length ( Communication );
    ACfg->CommAmount = Amount;
    // выделить память
    ACfg->Comm = mymalloc ( ( Amount ) * sizeof ( comm_cfg_t ) );
    // инициализировать память
    __config_init_comm ( ACfg );
    // прочитать коммуникации
    return __config_each_comm ( Communication, ACfg );
}

/**********************************************************************/

static
void __config_init_the_script ( script_cfg_t *Script )
{
    Script->Name = NULL;
    Script->Parametr = NULL;
}

static
void __config_init_script ( target_cfg_t *CurTarget )
{
    for ( size_t i = 0; i < CurTarget->ScriptAmount; i++ )
    {
        __config_init_the_script ( CurTarget->Script + i );
    }
}

// прочитать цель задачи
static
void __config_target ( target_cfg_t *CurTarget, const config_setting_t *TheTarget )
{
    // имя
    const char *Name = config_setting_name ( TheTarget );
    CurTarget->Type = mystrdup ( Name );
    // кол-во скриптов
    CurTarget->ScriptAmount = config_setting_length ( TheTarget );
    // выделить память
    CurTarget->Script = mymalloc ( sizeof ( script_cfg_t ) * CurTarget->ScriptAmount );
    __config_init_script ( CurTarget );
    // перебор скриптов
    for ( size_t i = 0; i < CurTarget->ScriptAmount; i++ )
    {
        const char *script = config_setting_get_string_elem ( TheTarget, i );
        char *ScriptName, *ScriptParametr;
        _get_script ( &ScriptName, &ScriptParametr, script );
        ( CurTarget->Script + i )->Name = ScriptName;
        ( CurTarget->Script + i )->Parametr = ScriptParametr;
    }
}

static 
void __config_init_the_target ( target_cfg_t *TheTarget )
{
    TheTarget->Type = NULL;
    TheTarget->Script = NULL;
    TheTarget->ScriptAmount = 0;
}

static
void __config_init_target ( task_cfg_t *TheTask )
{
    for ( size_t i = 0; i < TheTask->TargetAmount; i++ )
    {
        __config_init_the_target ( TheTask->Target + i );
    }
}

// прочитать способ коммуникации
static
int __config_read_the_task ( const config_setting_t *TheTask, askue_cfg_t *ACfg, size_t Index )
{
    // кол-во целей
    size_t Amount = config_setting_length ( TheTask );
    // выделить память
    task_cfg_t *curTask = ACfg->Task + Index;
    curTask->TargetAmount = Amount;
    curTask->Target = mymalloc ( sizeof ( target_cfg_t ) * Amount );
    __config_init_target ( curTask );
    // перебор целей
    for ( size_t i = 0; i < Amount; i++ )
    {
        target_cfg_t *curTarget = curTask->Target + i;
        config_setting_t *TheTarget = config_setting_get_elem ( TheTask, i );
        __config_target ( curTarget, TheTarget );
    }
}

// Найти описание списка задач
static
const config_setting_t* _get_task ( const config_t *cfg )
{
    config_setting_t *Task = config_lookup ( cfg, "Task" );
    if ( Task == NULL )
    {
        // сообщение об ошибке
        askue_config_fail ( "В конфигурации отсутствует запись 'Task'." );
        return ( const config_setting_t* ) NULL;
    }
    return ( const config_setting_t* ) Task;
}

// конфигурирование следующей задачи
static
int __config_next_task ( const config_setting_t* Task, askue_cfg_t *ACfg, size_t offset )
{
    config_setting_t *TheTask = config_setting_get_elem ( Task, offset );
    return __config_read_the_task ( TheTask, ACfg, offset );
}

// конфигурирование всех задач
static
int __config_each_task ( const config_setting_t* Task, askue_cfg_t *ACfg, size_t TaskAmount )
{
    int Result = ASKUE_SUCCESS;
    for ( size_t i = 0; ( i < TaskAmount ) && ( Result != ASKUE_ERROR ); i++ )
    {
        Result = __config_next_task ( Task, ACfg, i );
    }
    return Result;
}

static
void __config_init_the_task ( task_cfg_t *TheTask )
{
    TheTask->Target = NULL;
    TheTask->Script = NULL;
}

// инициализировать память под задачи
static
void __config_init_task ( askue_cfg_t *ACfg, size_t TaskAmount ) 
{
    for ( size_t i = 0; i < TaskAmount; i++ )
    {
        __config_init_the_task ( ACfg->Task + i );
    }
}

// читать структу задач
static
int __config_read_task ( const config_t *cfg, askue_cfg_t *ACfg )
{
    // корневая структура описывающая задачи
    const config_setting_t* Task = _get_task ( cfg );
    // кол-во способов коммуникации
    size_t Amount = config_setting_length ( Task ) + 1;
    // выделить память
    ACfg->Task = mymalloc ( ( Amount ) * sizeof ( task_cfg_t ) );
    __config_init_task ( ACfg, Amount );
    // прочитать задачи
    return __config_each_task ( Task, ACfg, Amount );
}

/**********************************************************************/

// сообщение об ошибке чтения конфига
static
void _say_ConfigReadFail ( char *Buffer, const config_t *cfg )
{
    if ( snprintf ( Buffer, _ASKUE_TBUFLEN, "%s at line %d.", config_error_text ( cfg ), config_error_line ( cfg ) ) > 0 )
        askue_config_fail ( Buffer );
}

// сообщение об успешном чтении конфига
static
int _say_ConfigReadOk ( char *Buffer )
{
    if ( snprintf ( Buffer, _ASKUE_TBUFLEN, "Файл '%s' - прочитан.", ASKUE_FILE_CONFIG ) > 0 )
    {
        askue_config_ok ( Buffer );
        return ASKUE_SUCCESS;
    }
    else
    {
        return ASKUE_ERROR;
    }
}

// читать все части конфига
static 
int __config_read ( const config_t *cfg, askue_cfg_t *ACfg )
{
    return __config_read_journal ( cfg, ACfg ) == 0 &&
            __config_read_log ( cfg, ACfg ) == 0 &&
            __config_read_port ( cfg, ACfg ) == 0 && 
            __config_read_device ( cfg, ACfg ) == 0 &&
            __config_read_network ( cfg, ACfg ) == 0 &&
            __config_read_comm ( cfg, ACfg ) == 0 &&
            __config_read_task ( cfg, ACfg ) == 0;
}

/* Точка чтения конфигурации */
int askue_config_read ( askue_cfg_t *ACfg )
{
    config_t cfg; // конфиг из файла
	config_init ( &cfg ); // выделить память под переменную с конфигурацией
    
    // текстовый буфер
    char Buffer[ _ASKUE_TBUFLEN ];
    memset ( Buffer, '\0', _ASKUE_TBUFLEN );
    // флаг дополнительных сообщений
    int Verbose = TESTBIT ( ACfg->Flag, ASKUE_FLAG_VERBOSE );
    // читать конфиг из файла
	if ( config_read_file ( &cfg, "./askue.cfg.new" ) != CONFIG_TRUE ) // открыть и прочитать файл
	{
        _say_ConfigReadFail ( Buffer, &cfg );
        config_destroy ( &cfg );
        return ASKUE_ERROR;
    }
    // дополнительное сообщение
    if ( Verbose && ( _say_ConfigReadOk ( Buffer ) == ASKUE_ERROR ) )
    {
        config_destroy ( &cfg );
        return ASKUE_ERROR;
    }
    // результат работы
    int Result;
    if ( __config_read ( &cfg, ACfg ) )
    {
        if ( Verbose ) write_msg ( stderr, "Конфигурация", "OK", "Считывание завершено без ошибок." );
        Result = ASKUE_SUCCESS;
    }
    else
    {
        Result = ASKUE_ERROR;
    }
   
    config_destroy ( &cfg );
    return Result;
}

/**********************************************************************/

/* Инициализация памяти конфигурации */
void askue_config_init ( askue_cfg_t *Cfg )
{
    Cfg->Device = NULL;
    Cfg->DeviceAmount = 0;
    Cfg->Task = NULL;
    Cfg->TaskAmount = 0;
    Cfg->Comm = NULL;
    Cfg->CommAmount = 0;
    Cfg->Port = NULL;
    Cfg->Log = NULL;
    Cfg->Journal = NULL;
    Cfg->Network = NULL;
    Cfg->NetworkSize = 0;
    Cfg->Flag = 0;
}

// Удаление данных о порте
static
void __destroy_port ( askue_cfg_t *ACfg )
{
    if ( ACfg->Port )
    {
        myfree ( ACfg->Port->DBits );
        myfree ( ACfg->Port->SBits );
        myfree ( ACfg->Port->Speed );
        myfree ( ACfg->Port->File );
        myfree ( ACfg->Port->Parity );
        free ( ACfg->Port );
    }
}

// Удаление данных о логе
static
void __destroy_log ( askue_cfg_t *ACfg )
{
    if ( ACfg->Log )
    {
        myfree ( ACfg->Log->File );
        myfree ( ACfg->Log->Mode );
        free ( ACfg->Log );
    }
}

// Удаление данных о базе
static
void __destroy_journal ( askue_cfg_t *ACfg )
{
    if ( ACfg->Journal )
    {
        myfree ( ACfg->Journal->File );
        myfree ( ACfg->Journal );
    }
}

// удалить информацию об устройстве
static
void __destroy_device_info ( device_cfg_t *TheDevice )
{
    myfree ( TheDevice->Name );
    myfree ( TheDevice->Type );
}

// удалить информацию о списке устройств
static
void __destroy_device ( askue_cfg_t *ACfg )
{
    for ( size_t i = 0; i < ACfg->DeviceAmount; i++ )
    {
        __destroy_device_info ( ACfg->Device + i );
    }
    
    myfree ( ACfg->Device );
}

// удалить инфу о скрипте
static 
void __destroy_script_info ( script_cfg_t *TheScript )
{
    myfree ( TheScript->Name );
    myfree ( TheScript->Parametr );
}

// удалить информацию о коммуникации
static
void __destroy_comm_info ( comm_cfg_t *TheComm )
{
    __destroy_script_info ( TheComm->Script );
    myfree ( TheComm->Script );
}

// удалить информацию о списке коммуникаций
static
void __destroy_comm ( askue_cfg_t *ACfg )
{
    for ( size_t i = 0; i < ACfg->CommAmount; i++ )
    {
        __destroy_comm_info ( ACfg->Comm + i );
    }
    
    myfree ( ACfg->Comm );
}

// удалить информацию о целе
static
void __destroy_target_info ( target_cfg_t *TheTarget )
{
    myfree ( TheTarget->Type );
    
    for ( int i = 0; i < TheTarget->ScriptAmount; i++ )
    {
        __destroy_script_info ( TheTarget->Script + i );
    }
    myfree ( TheTarget->Script );
}

// удалить информацию о задаче
static
void __destroy_task_info ( task_cfg_t *TheTask )
{    
    for ( size_t i = 0; i < TheTask->TargetAmount; i++ )
    {
        __destroy_target_info ( TheTask->Target + i );
    }
        
    myfree ( TheTask->Target );
}

// удалить информацию о списке задач
static
void __destroy_task ( askue_cfg_t *ACfg )
{
    for ( size_t i = 0; i < ACfg->TaskAmount; i++ )
    {
        __destroy_task_info ( ACfg->Task + i ); 
    }
    
    myfree ( ACfg->Task );
}

// удалить сеть
static
void __destroy_network ( askue_cfg_t *Cfg )
{
    myfree ( Cfg->Network );
}

/* Деинициализация памяти конфигурации */
void askue_config_destroy ( askue_cfg_t *Cfg )
{
    __destroy_port ( Cfg );
    askue_config_ok ( "__destroy_port() - done" );
    __destroy_log ( Cfg );
    askue_config_ok ( "__destroy_log() - done" );
    __destroy_journal ( Cfg );
    askue_config_ok ( "__destroy_journal() - done" );
    __destroy_device ( Cfg );
    askue_config_ok ( "__destroy_device() - done" );
    __destroy_comm ( Cfg );
    askue_config_ok ( "__destroy_comm() - done" );
    __destroy_task ( Cfg );
    askue_config_ok ( "__destroy_task() - done" );
    __destroy_network ( Cfg );
    askue_config_ok ( "__destroy_network() - done" );
}

#undef askue_config_fail
#undef askue_config_error
#undef askue_config_ok





