#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libconfig.h>
#include <libaskue.h>

#include "config.h"

/*                  Функции точки сбора конфигурации                  */

// Инициализация порта
static
void __init_port ( askue_cfg_t *ACfg )
{
    ACfg->Port = mymalloc ( sizeof ( port_cfg_t ) );
    ACfg->Port->File = NULL;
    ACfg->Port->DBits = NULL;
    ACfg->Port->Parity = NULL;
    ACfg->Port->SBits = NULL;
    ACfg->Port->Speed = NULL;
}

// Инициализация лога
static
void __init_log ( askue_cfg_t *ACfg )
{
    ACfg->Log = mymalloc ( sizeof ( log_cfg_t ) );
    ACfg->Log->File = NULL;
    ACfg->Log->Mode = NULL;
    ACfg->Log->Lines = 0;
}

// Инициализация базы данных
static
void __init_db ( askue_cfg_t *ACfg )
{
    ACfg->Journal = mymalloc ( sizeof ( journal_cfg_t ) );
    ACfg->Journal->File = NULL;
}

// Инициализация списка устройств
static
void __init_device_list ( askue_cfg_t *ACfg )
{
    ACfg->DeviceList = NULL;
}

// Инициализация списка гейтов
static
void __init_gate_list ( askue_cfg_t *ACfg )
{
    ACfg->RemoteGateList = NULL;
    ACfg->LocalGate = NULL;
}


// Инициализация списка сценариев
static
void __init_script_list ( askue_cfg_t *ACfg )
{
    ACfg->TypeList = NULL;
}

// Инициализация списка отчётов
static
void __init_report_list ( askue_cfg_t *ACfg )
{
    ACfg->ReportList = NULL;
}

/*                    Точка сбора конфигурации                        */
void askue_config_init ( askue_cfg_t *ACfg )
{
    ACfg->Flag = 0;
    __init_port ( ACfg );
    __init_log ( ACfg );
    __init_db ( ACfg );
    __init_gate_list ( ACfg );
    __init_device_list ( ACfg );
    __init_script_list ( ACfg );
    __init_report_list ( ACfg );
}

/*                Функции точки разбора конфигурации                  */

// Удаление данных о порте
static
void __destroy_port ( askue_cfg_t *ACfg )
{
    myfree ( ACfg->Port->DBits );
    myfree ( ACfg->Port->SBits );
    myfree ( ACfg->Port->Speed );
    myfree ( ACfg->Port->File );
    myfree ( ACfg->Port->Parity );
    myfree ( ACfg->Port );
}

// Удаление данных о логе
static
void __destroy_log ( askue_cfg_t *ACfg )
{
    myfree ( ACfg->Log->File );
    myfree ( ACfg->Log->Mode );
    myfree ( ACfg->Log );
}

// Удаление данных о базе
static
void __destroy_db ( askue_cfg_t *ACfg )
{
    myfree ( ACfg->Journal->File );
    myfree ( ACfg->Journal );
}

// Удаление данных о списке устройств
static
void __destroy_device_list ( askue_cfg_t *ACfg )
{
    if ( ACfg->DeviceList != NULL )
    {
        for ( size_t i = 0; ACfg->DeviceList[ i ] != NULL; i++ )
        {
            myfree ( ACfg->DeviceList[ i ]->Name );
            myfree ( ACfg->DeviceList[ i ] );
        } 
        
        myfree ( ACfg->DeviceList );
    }
}

// Удаление данных о списке гейтов
static
void __destroy_gate_list ( askue_cfg_t *ACfg )
{
    //write_msg ( stderr, "Деструктор: __destroy_gate_list", "OK", "Point 1" );
    if ( ACfg->RemoteGateList != NULL )
    {
        for ( size_t i = 0; ACfg->RemoteGateList[ i ] != NULL; i++ )
        {
            //write_msg ( stderr, "Деструктор: __destroy_gate_list", "OK", "ACfg->RemoteGateList[ i ]->Device->Name" );
            myfree ( ACfg->RemoteGateList[ i ]->Device->Name );
            //write_msg ( stderr, "Деструктор: __destroy_gate_list", "OK", "ACfg->RemoteGateList[ i ]->Device" );
            myfree ( ACfg->RemoteGateList[ i ]->Device );
            //write_msg ( stderr, "Деструктор: __destroy_gate_list", "OK", "ACfg->RemoteGateList[ i ]" );
            myfree ( ACfg->RemoteGateList[ i ] );
        } 
        
        myfree ( ACfg->RemoteGateList );
    }
    //write_msg ( stderr, "Деструктор: __destroy_gate_list", "OK", "Point 2" );
    if ( ACfg->LocalGate != NULL )
    {
        myfree ( ACfg->LocalGate->Device->Name );
        myfree ( ACfg->LocalGate->Device );
        myfree ( ACfg->LocalGate );
    } 
}

// Удаление данных о списке скриптов
static
void __destroy_type_list ( askue_cfg_t *ACfg )
{
    if ( ACfg->TypeList != NULL )
    {
        for ( size_t i = 0; ACfg->TypeList[ i ] != NULL; i++ )
        {
            myfree ( ACfg->TypeList[ i ]->Name );
            for ( size_t j = 0; ACfg->TypeList[ i ]->Script[ j ] != NULL; j++ )
            {
                myfree ( ACfg->TypeList[ i ]->Script[ j ] );
            }
            myfree ( ACfg->TypeList[ i ]->Script );
            myfree ( ACfg->TypeList[ i ] );
        } 
        
        myfree ( ACfg->TypeList );
    }
}

// удалит память выделенную под отчёты
static 
void __destroy_report_list ( askue_cfg_t *ACfg )
{
    if ( ACfg->ReportList != NULL )
    {
        for ( size_t i = 0; ACfg->ReportList[ i ] != NULL; i++ )
        {
            myfree ( ACfg->ReportList[ i ]->Name );
            myfree ( ACfg->ReportList[ i ] );
        }
        
        myfree ( ACfg->ReportList );
    }
}

/*                      Точка разбора конфигурации                    */
void askue_config_destroy ( askue_cfg_t *ACfg )
{
    __destroy_port ( ACfg );
    //write_msg ( stderr, "Деструктор конфигурации", "OK", "__destroy_port() - done" );
    __destroy_log ( ACfg );
    //write_msg ( stderr, "Деструктор конфигурации", "OK", "__destroy_log() - done" );
    __destroy_db ( ACfg );
    //write_msg ( stderr, "Деструктор конфигурации", "OK", "__destroy_db() - done" );
    __destroy_device_list ( ACfg );
    //write_msg ( stderr, "Деструктор конфигурации", "OK", "__destroy_device_list() - done" );
    __destroy_gate_list ( ACfg );
    //write_msg ( stderr, "Деструктор конфигурации", "OK", "__destroy_gate_list() - done" );
    __destroy_type_list ( ACfg );
    //write_msg ( stderr, "Деструктор конфигурации", "OK", "__destroy_type_list() - done" );
    __destroy_report_list ( ACfg );
    //write_msg ( stderr, "Деструктор конфигурации", "OK", "__destroy_report_list() - done" );
}

/*                Функции точки чтения конфигурации                   */

// чтение конфигурации порта
static
int __config_read_port ( config_t *cfg, askue_cfg_t *ACfg )
{
     config_setting_t *port_setting = config_lookup ( cfg, "Port" ); // поиск сети
     if ( port_setting == NULL )
     {
         write_msg ( stderr, "Конфигурация", "FAIL", "В конфигурации отсутствует запись 'Port'" );
         return -1;
     }
     
     const char *PortSettings[ 5 ];
     #define PortFile PortSettings[ 0 ]
     #define PortDBits PortSettings[ 1 ]
     #define PortSBits PortSettings[ 2 ]
     #define PortParity PortSettings[ 3 ]
     #define PortSpeed PortSettings[ 4 ]
     
     if ( !( config_setting_lookup_string ( port_setting, "file", &(PortFile) ) == CONFIG_TRUE &&
             config_setting_lookup_string ( port_setting, "dbits", &(PortDBits) ) == CONFIG_TRUE &&
             config_setting_lookup_string ( port_setting, "sbits", &(PortSBits) ) == CONFIG_TRUE &&
             config_setting_lookup_string ( port_setting, "parity", &(PortParity) ) == CONFIG_TRUE &&
             config_setting_lookup_string ( port_setting, "speed", &(PortSpeed) ) == CONFIG_TRUE ) )
     {
         write_msg ( stderr, "Конфигурация", "FAIL", "Запись 'Port' не полная" );
         return -1;
     }
     
     ACfg->Port = mymalloc ( sizeof ( port_cfg_t ) );
     ACfg->Port->DBits = mystrdup ( PortDBits );
     ACfg->Port->SBits = mystrdup ( PortSBits );
     ACfg->Port->Speed = mystrdup ( PortSpeed );
     ACfg->Port->Parity = mystrdup ( PortParity );
     ACfg->Port->File = mystrdup ( PortFile );
     
     verbose_msg ( ACfg->Flag, stdout, "Конфигурация", "OK", "Настройки порта успешно считаны." );
     
     return 0;
     
     #undef PortFile
     #undef PortDBits
     #undef PortSBits
     #undef PortParity
     #undef PortSpeed
        
}

// чтение конфигурации лога
static
int __config_read_log ( config_t *cfg, askue_cfg_t *ACfg )
{
     config_setting_t *setting = config_lookup ( cfg, "Log" ); // поиск сети
     if ( setting == NULL )
     {
         write_msg ( stderr, "Конфигурация", "FAIL", "В конфигурации отсутствует запись 'Log'" );
         return -1;
     }
    
     const char *LogFile;
     const char *LogLines;
     const char *LogMode;
     if ( !( config_setting_lookup_string ( setting, "file", &(LogFile) ) == CONFIG_TRUE &&
             config_setting_lookup_string ( setting, "lines", &(LogLines) ) == CONFIG_TRUE &&
             config_setting_lookup_string ( setting, "mode", &(LogMode) ) == CONFIG_TRUE ) )
     {
         write_msg ( stderr, "Конфигурация", "FAIL", "Запись 'Log' не полная" );
         return -1;
     }
     
     ACfg->Log = mymalloc ( sizeof ( log_cfg_t ) );
     ACfg->Log->File = mystrdup ( LogFile );
     ACfg->Log->Lines = strtol ( LogLines, NULL, 10 );
     ACfg->Log->Mode = mystrdup ( LogMode );
     
     verbose_msg ( ACfg->Flag, stdout, "Конфигурация", "OK", "Настройки лога успешно считаны." );
     
     return 0;
}

// чтение конфигурации базы
static
int __config_read_db ( config_t *cfg, askue_cfg_t *ACfg )
{
     config_setting_t *setting = config_lookup ( cfg, "Journal" ); // поиск сети
     if ( setting == NULL )
     {
         write_msg ( stderr, "Конфигурация", "FAIL", "В конфигурации отсутствует запись 'Journal'" );
         return -1;
     }
     
     const char *JnlFile;
     if ( config_setting_lookup_string ( setting, "file", &(JnlFile) ) != CONFIG_TRUE )
     {
         write_msg ( stderr, "Конфигурация", "FAIL", "Запись 'Journal' не полная" );
         return -1;
     }
     
     const char *JnlSize;
     if ( config_setting_lookup_string ( setting, "size", &(JnlSize) ) != CONFIG_TRUE )
     {
         verbose_msg ( ACfg->Flag, stderr, "Конфигурация", "ERROR", "Отсутствует запись 'Journal.size'" );
         verbose_msg ( ACfg->Flag, stderr, "Конфигурация", "OK", "Установка значения по умолчанию 'Journal.size = 3'" );
         ACfg->Journal->Size = 3;
     }
     else
     {
         ACfg->Journal->Size = (size_t) strtol ( JnlSize, NULL, 10 );
     }
     
     const char *JnlFlashback;
     if ( config_setting_lookup_string ( setting, "flashback", &(JnlFlashback) ) != CONFIG_TRUE )
     {
         verbose_msg ( ACfg->Flag, stderr, "Конфигурация", "ERROR", "Отсутствует запись 'Journal.flashback'" );
         verbose_msg ( ACfg->Flag, stderr, "Конфигурация", "OK", "Установка значения по умолчанию 'Journal.flashback = 0'" );
         ACfg->Journal->Flashback = 0;
     }
     else
     {
         ACfg->Journal->Flashback = (size_t) strtol ( JnlFlashback, NULL, 10 );
     }
     
     ACfg->Journal = mymalloc ( sizeof ( journal_cfg_t ) );
     ACfg->Journal->File = mystrdup ( JnlFile );
     
     verbose_msg ( ACfg->Flag, stderr, "Конфигурация", "OK", "Настройки журнала успешно считаны." );
     
     return 0;
}

// чтение скрипта и его параметра
static
void __config_script ( char **ScriptName, char **ScriptParametr, const char *script )
{
    const char *Parametr = strchr ( script, ':' );
    
    if ( Parametr != NULL )
    {
        size_t len = (size_t)( (const char*) Parametr - (const char*)script );
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
void __config_report ( char **ReportName, char **ReportParametr, const char *report )
{
    const char *Parametr = strchr ( report, ':' );
    
    if ( Parametr != NULL )
    {
        size_t len = (size_t)( (const char*) Parametr - (const char*)report );
        *ReportName = mystrndup ( report, len );
        
        Parametr++;
        while ( *Parametr != '\0' && isspace ( *Parametr ) )
            Parametr++;
        *ReportParametr = mystrdup ( Parametr );
    }
    else
    {
        *ReportParametr = NULL;
        *ReportName = mystrdup ( report );
    }
}

// чтение конфигурации одного типа
static
void __config_read_type ( config_setting_t *setting, askue_cfg_t *ACfg, size_t Number )
{
    ACfg->TypeList[ Number ] = mymalloc ( sizeof ( type_cfg_t ) );
    
    size_t ScriptAmount = (size_t) config_setting_length ( setting );
    ACfg->TypeList[ Number ]->Script = mymalloc ( sizeof ( script_cfg_t* ) * ( ScriptAmount + 1 ) );
    for ( size_t i = 0; i < ScriptAmount + 1; i++ )
    {
        ACfg->TypeList[ Number ]->Script[ i ] = NULL;
    } 
    const char *type = config_setting_name ( setting );
     ACfg->TypeList[ Number ]->Name = mystrdup ( type );
     
    for ( size_t i = 0; i < ScriptAmount; i++ )
    {
        
        const char *script = config_setting_get_string_elem ( setting, i );
        //ACfg->TypeList[ Number ]->Script[ i ] = mystrdup ( script );
        ACfg->TypeList[ Number ]->Script[ i ] = mymalloc ( sizeof ( script_cfg_t ) );
        char *ScriptName, *ScriptParametr;
        __config_script ( &ScriptName, &ScriptParametr, script );
        ACfg->TypeList[ Number ]->Script[ i ]->Name = ScriptName;
        ACfg->TypeList[ Number ]->Script[ i ]->Parametr  =ScriptParametr;
    }
}

// чтение конфигурации типов устройств и их обработчиков
static 
int __config_read_type_list ( config_t *cfg, askue_cfg_t *ACfg )
{
     config_setting_t *setting = config_lookup ( cfg, "ScriptList" ); // поиск сети
     if ( setting == NULL )
     {
         write_msg ( stderr, "Конфигурация", "FAIL", "В конфигурации отсутствует запись 'ScriptList'" );
         return -1;
     }
     
     size_t TypeAmount = (size_t) config_setting_length ( setting );
     ACfg->TypeList = mymalloc ( sizeof ( type_cfg_t* ) * ( TypeAmount + 1 ) );
     for ( size_t i = 0; i < TypeAmount + 1; i++ )
     {
         ACfg->TypeList[ i ] = NULL;
     }
     
     for ( size_t i = 0; i < TypeAmount; i++ )
     {
         config_setting_t *subsetting = config_setting_get_elem ( setting, i );
         __config_read_type ( subsetting, ACfg, i );
     }
     
     verbose_msg ( ACfg->Flag, stdout, "Конфигурация", "OK", "Список типов успешно считан." );
     
     return 0;
}

// поиск типа устройства
static
type_cfg_t* __find_device_type ( askue_cfg_t *ACfg, const char *type )
{
    type_cfg_t *Result = NULL; 
    for ( size_t i = 0; ACfg->TypeList[ i ] != NULL && Result == NULL; i++ )
    {
        if ( !strcmp ( ACfg->TypeList[ i ]->Name, type ) )
            Result = ACfg->TypeList[ i ];
    }
    return Result;
}



// расшифровка класса устройств
static
device_class_t __what_class ( const char *DeviceClass )
{
    if ( !strcmp ( DeviceClass, "counter" ) )
    {
        return Askue_Counter;
    }
    else if ( !strcmp ( DeviceClass, "modem" ) )
    {
        return Askue_Modem;
    }
    else
    {
        return Askue_NoClass;
    }
}

// получить сегмент
static
device_segment_t __what_segment ( const char *DeviceSegment )
{
    if ( !strcmp ( DeviceSegment, "remote" ) )
    {
        return Askue_Remote;
    }
    else if ( !strcmp ( DeviceSegment, "local" ) )
    {
        return Askue_Local;
    }
    else
    {
        return Askue_NoSegment;
    }
}

// чтение конфигурации одного устройства
static
int __config_read_device ( config_setting_t *setting, askue_cfg_t *ACfg, device_cfg_t *Device )
{    
    const char *DeviceSetting[ 6 ];
    
    #define DeviceClass DeviceSetting[ 0 ]
    #define DeviceType DeviceSetting[ 1 ]
    #define DeviceName DeviceSetting[ 2 ]
    #define DeviceTimeout DeviceSetting[ 3 ]
    #define DeviceSegment DeviceSetting[ 4 ]
    #define DeviceId DeviceSetting[ 5 ]
    
    if ( !( config_setting_lookup_string ( setting, "class", &(DeviceClass) ) == CONFIG_TRUE && 
            config_setting_lookup_string ( setting, "type", &(DeviceType) ) == CONFIG_TRUE &&  
            config_setting_lookup_string ( setting, "name", &(DeviceName) ) == CONFIG_TRUE ) )
    {
         write_msg ( stderr, "Конфигурация", "FAIL", "Запись 'DeviceList' не полная" );
         return -1;
    }
    // класс
    Device->Class = __what_class ( DeviceClass );
    if ( Device->Class == Askue_NoClass )
    {
         write_msg ( stderr, "Конфигурация", "FAIL", "Не верный класс устройства" );
         return -1;
    }
    // имя
    Device->Name = mystrdup( DeviceName );
    // тип
    Device->Type = __find_device_type ( ACfg, DeviceType );
    if ( Device->Type == NULL )
    {
        write_msg ( stderr, "Конфигурация", "FAIL", "Нет скриптов для данного типа" );
        return -1;
    }
    
    // id
    if ( config_setting_lookup_string ( setting, "id", &(DeviceId) ) == CONFIG_TRUE )
    {
        Device->Id = strtol ( DeviceId, NULL, 10 );
    }
    // таймаут
    if ( config_setting_lookup_string ( setting, "timeout", &(DeviceTimeout) ) == CONFIG_TRUE )
    {
        Device->Timeout = strtol ( DeviceTimeout, NULL, 10 );
    }
    else if ( Device->Class == Askue_Counter )
    {
        write_msg ( stderr, "Конфигурация", "FAIL", "Нет таймаута ожидания для счётчика" );
        return -1;
    }
    // сегмент
    if ( config_setting_lookup_string ( setting, "segment", &(DeviceSegment) ) == CONFIG_TRUE )
    {
        Device->Segment = __what_segment ( DeviceSegment );
        if ( Device->Segment == Askue_NoSegment )
        {
             write_msg ( stderr, "Конфигурация", "FAIL", "Не указан сегмент сети" );
             return -1;
        }
    }
    else
    {
        Device->Segment = Askue_Local;
    }
    
    #undef DeviceClass
    #undef DeviceType
    #undef DeviceName
    #undef DeviceTimeout
    #undef DeviceSegment
    #undef DeviceId
    
    return 0;
}

// чтение конфигурации списка устройств
static
int __config_read_device_list ( config_t *cfg, askue_cfg_t *ACfg )
{
     config_setting_t *setting = config_lookup ( cfg, "DeviceList" ); // поиск сети
     if ( setting == NULL )
     {
         write_msg ( stderr, "Конфигурация", "FAIL", "В конфигурации отсутствует запись 'DeviceList'" );
         return -1;
     }
     
     size_t DeviceAmount = (size_t) config_setting_length ( setting );
     ACfg->DeviceList = mymalloc ( sizeof( device_cfg_t* ) * ( DeviceAmount + 1 ) );
     for ( size_t i = 0; i < DeviceAmount + 1; i++ )
     {
         ACfg->DeviceList[ i ] = NULL;
     }
     int Result = 0;
     for ( size_t i = 0; i < DeviceAmount && !Result; i++ )
     {
         config_setting_t *subsetting = config_setting_get_elem ( setting, i );
         ACfg->DeviceList[ i ] = mymalloc ( sizeof ( device_cfg_t ) );
         Result = __config_read_device ( subsetting, ACfg, ACfg->DeviceList[ i ] );
     }
     if ( Result ) return -1;
     
     verbose_msg ( ACfg->Flag, stdout, "Конфигурация", "OK", "Список устройств успешно cчитан." );
     
     return 0;
}

// чтение конфигурации отчётов
int __config_read_report_list ( config_t *cfg, askue_cfg_t *ACfg )
{
     config_setting_t *setting = config_lookup ( cfg, "ReportList" ); // поиск сети
     if ( setting == NULL )
     {
         write_msg ( stderr, "Конфигурация", "FAIL", "В конфигурации отсутствует запись 'ReportList'" );
         return -1;
     }
     
     size_t ReportAmount = (size_t) config_setting_length ( setting );
     ACfg->ReportList = mymalloc ( sizeof( report_cfg_t* ) * ( ReportAmount + 1 ) );
     for ( size_t i = 0; i < ReportAmount + 1; i++ )
     {
         ACfg->ReportList[ i ] = NULL;
     }
     for ( size_t i = 0; i < ReportAmount; i++ )
     {
         ACfg->ReportList[ i ] = mymalloc ( sizeof( report_cfg_t ) );
         const char *Report = config_setting_get_string_elem ( setting, i );
         __config_report ( &(ACfg->ReportList[ i ]->Name), 
                           &(ACfg->ReportList[ i ]->Parametr), 
                           Report );
     }
     
     verbose_msg ( ACfg->Flag, stdout, "Конфигурация", "OK", "Список отчётов успешно считан." );
     
     return 0;
}

// чтение конфигурации базового модема
int __config_read_local_gate ( config_t *cfg, askue_cfg_t *ACfg )
{
     config_setting_t *setting = config_lookup ( cfg, "LocalGate" ); // поиск сети
     if ( setting == NULL )
     {
         verbose_msg ( ACfg->Flag, stdout, "Конфигурация", "ERROR", "В конфигурации отсутствует запись 'LocalGate'" );
         return 0;
     }
     
     ACfg->LocalGate = mymalloc ( sizeof( gate_cfg_t ) );
     ACfg->LocalGate->Device = mymalloc ( sizeof ( device_cfg_t ) );
     int Result = __config_read_device ( setting, ACfg, ACfg->LocalGate->Device );
     if ( Result ) 
     {
        return -1;
     }
     else
     {
         verbose_msg ( ACfg->Flag, stdout, "Конфигурация", "OK", "Конфигурация базового модема успешно считана." );
         return 0;
     }
}


// чтение конфигурации модемов
int __config_read_remote_gate_list ( config_t *cfg, askue_cfg_t *ACfg )
{
     config_setting_t *setting = config_lookup ( cfg, "RemoteGateList" ); // поиск сети
     if ( setting == NULL )
     {
         write_msg ( stderr, "Конфигурация", "ERROR", "В конфигурации отсутствует запись 'RemoteGateList'" );
         return 0;
     }
     
     size_t GateAmount = (size_t) config_setting_length ( setting );
     ACfg->RemoteGateList = mymalloc ( sizeof( gate_cfg_t* ) * ( GateAmount + 1 ) );
     for ( size_t i = 0; i < GateAmount + 1; i++ )
     {
         ACfg->RemoteGateList[ i ] = NULL;
     }
     int Result = 0;
     for ( size_t i = 0; i < GateAmount && !Result; i++ )
     {
         config_setting_t *subsetting = config_setting_get_elem ( setting, i );
         ACfg->RemoteGateList[ i ] = mymalloc ( sizeof( gate_cfg_t ) );
         ACfg->RemoteGateList[ i ]->Device = mymalloc ( sizeof ( device_cfg_t ) );
         Result = __config_read_device ( subsetting, ACfg, ACfg->RemoteGateList[ i ]->Device );
         ACfg->RemoteGateList[ i ]->Device->Timeout = ACfg->LocalGate->Device->Timeout;
     }
     if ( Result ) return -1;
     
     verbose_msg ( ACfg->Flag, stdout, "Конфигурация", "OK", "Список удалённых модемов успешно cчитан." );
     
     return 0;
}

/*                      Точка чтения конфигурации                     */
int askue_config_read ( askue_cfg_t *ACfg )
{
    int Result = -1; 
    
    config_t cfg;
	config_init ( &cfg ); // выделить память под переменную с конфигурацией
    
    char Buffer[ 256 ];
    snprintf ( Buffer, 256, "Открываем файл: '%s'", ASKUE_FILE_CONFIG );
    verbose_msg ( ACfg->Flag, stderr, "Конфигурация", "OK", Buffer );
    
	if ( config_read_file ( &cfg, ASKUE_FILE_CONFIG ) == CONFIG_TRUE ) // открыть и прочитать файл
	{
        verbose_msg ( ACfg->Flag, stderr, "Конфигурация", "OK", "Файл '/etc/askue/askue.cfg' успешно открыт." );
        // порт
        if ( __config_read_db ( &cfg, ACfg ) == 0 &&
             __config_read_log ( &cfg, ACfg ) == 0 &&
             __config_read_port ( &cfg, ACfg ) == 0 && 
             __config_read_type_list ( &cfg, ACfg ) == 0 &&
             __config_read_report_list ( &cfg, ACfg ) == 0 &&
             __config_read_local_gate ( &cfg, ACfg ) == 0 &&
             __config_read_remote_gate_list ( &cfg, ACfg ) == 0 &&
             __config_read_device_list ( &cfg, ACfg ) == 0 )
        {
            verbose_msg ( ACfg->Flag, stderr, "Конфигурация", "OK", "Считывание завершено без ошибок." );
            Result = 0;
        }
    }
    else
    {
        snprintf ( Buffer, 256, "%s at line %d.", config_error_text ( &cfg ), config_error_line ( &cfg ) );
        verbose_msg ( ACfg->Flag, stderr, "Конфигурация", "FAIL", Buffer );
    }
    config_destroy ( &cfg );
    return Result;
}

