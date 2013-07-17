#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libconfig.h>

#include "config.h"
#include "write_msg.h"
#include "my.h"

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
    __init_port ( ACfg );
    __init_log ( ACfg );
    __init_db ( ACfg );
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
            myfree ( ACfg->DeviceList[ i ]->Id );
            myfree ( ACfg->DeviceList[ i ] );
        } 
        
        myfree ( ACfg->DeviceList );
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
    __destroy_log ( ACfg );
    __destroy_db ( ACfg );
    __destroy_device_list ( ACfg );
    __destroy_type_list ( ACfg );
    __destroy_report_list ( ACfg );
}

/*                Функции точки чтения конфигурации                   */

// чтение конфигурации порта
static
int __config_read_port ( config_t *cfg, askue_cfg_t *ACfg )
{
     config_setting_t *port_setting = config_lookup ( cfg, "Port" ); // поиск сети
     if ( port_setting == NULL )
     {
         write_msg ( stderr, "Чтение конфигурации", "FAIL", "В конфигурации отсутствует запись 'Port'" );
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
         write_msg ( stderr, "Чтение конфигурации", "FAIL", "Запись 'Port' не полная" );
         return -1;
     }
     
     ACfg->Port = mymalloc ( sizeof ( port_cfg_t ) );
     ACfg->Port->DBits = mystrdup ( PortDBits );
     ACfg->Port->SBits = mystrdup ( PortSBits );
     ACfg->Port->Speed = mystrdup ( PortSpeed );
     ACfg->Port->Parity = mystrdup ( PortParity );
     ACfg->Port->File = mystrdup ( PortFile );
     
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
         write_msg ( stderr, "Чтение конфигурации", "FAIL", "В конфигурации отсутствует запись 'Log'" );
         return -1;
     }
    
     const char *LogFile;
     const char *LogLines;
     const char *LogMode;
     if ( !( config_setting_lookup_string ( setting, "file", &(LogFile) ) == CONFIG_TRUE &&
             config_setting_lookup_string ( setting, "lines", &(LogLines) ) == CONFIG_TRUE &&
             config_setting_lookup_string ( setting, "mode", &(LogMode) ) == CONFIG_TRUE ) )
     {
         write_msg ( stderr, "Чтение конфигурации", "FAIL", "Запись 'Log' не полная" );
         return -1;
     }
     
     ACfg->Log = mymalloc ( sizeof ( log_cfg_t ) );
     ACfg->Log->File = mystrdup ( LogFile );
     ACfg->Log->Lines = strtol ( LogLines, NULL, 10 );
     ACfg->Log->Mode = mystrdup ( LogMode );
     
     return 0;
}

// чтение конфигурации базы
static
int __config_read_db ( config_t *cfg, askue_cfg_t *ACfg )
{
     config_setting_t *setting = config_lookup ( cfg, "Journal" ); // поиск сети
     if ( setting == NULL )
     {
         write_msg ( stderr, "Чтение конфигурации", "FAIL", "В конфигурации отсутствует запись 'Journal'" );
         return -1;
     }
     
     const char *JnlFile;
     if ( config_setting_lookup_string ( setting, "file", &(JnlFile) ) != CONFIG_TRUE )
     {
         write_msg ( stderr, "Чтение конфигурации", "FAIL", "Запись 'Journal' не полная" );
         return -1;
     }
     
     const char *JnlSize;
     if ( config_setting_lookup_string ( setting, "size", &(JnlSize) ) != CONFIG_TRUE )
     {
         write_msg ( stderr, "Чтение конфигурации", "FAIL", "Отсутствует запись 'Journal.size'" );
         write_msg ( stderr, "Чтение конфигурации", "OK", "Установка значения по умолчанию 'Journal.size = 3'" );
         ACfg->Journal->Size = 3;
     }
     else
     {
         ACfg->Journal->Size = (size_t) strtol ( JnlSize, NULL, 10 );
     }
     
     const char *JnlFlashback;
     if ( config_setting_lookup_string ( setting, "flashback", &(JnlFlashback) ) != CONFIG_TRUE )
     {
         write_msg ( stderr, "Чтение конфигурации", "FAIL", "Отсутствует запись 'Journal.flashback'" );
         write_msg ( stderr, "Чтение конфигурации", "OK", "Установка значения по умолчанию 'Journal.flashback = 0'" );
         ACfg->Journal->Flashback = 0;
     }
     else
     {
         ACfg->Journal->Flashback = (size_t) strtol ( JnlFlashback, NULL, 10 );
     }
     
     ACfg->Journal = mymalloc ( sizeof ( journal_cfg_t ) );
     ACfg->Journal->File = mystrdup ( JnlFile );
     
     return 0;
}

// чтение конфигурации одного типа
static
void __config_read_type ( config_setting_t *setting, askue_cfg_t *ACfg, size_t Number )
{
    ACfg->TypeList[ Number ] = mymalloc ( sizeof ( type_cfg_t ) );
    
    size_t ScriptAmount = (size_t) config_setting_length ( setting );
    ACfg->TypeList[ Number ]->Script = mymalloc ( sizeof ( char* ) * ( ScriptAmount + 1 ) );
    
    const char *type = config_setting_name ( setting );
     ACfg->TypeList[ Number ]->Name = mystrdup ( type );
     
    for ( size_t i = 0; i < ScriptAmount; i++ )
    {
        const char *script = config_setting_get_string_elem ( setting, i );
        ACfg->TypeList[ Number ]->Script[ i ] = mystrdup ( script );
    }
    
    ACfg->TypeList[ Number ]->Script[ ScriptAmount ] = NULL;
}

// чтение конфигурации типов устройств и их обработчиков
static 
int __config_read_type_list ( config_t *cfg, askue_cfg_t *ACfg )
{
     config_setting_t *setting = config_lookup ( cfg, "ScriptList" ); // поиск сети
     if ( setting == NULL )
     {
         write_msg ( stderr, "Чтение конфигурации", "FAIL", "В конфигурации отсутствует запись 'ScriptList'" );
         return -1;
     }
     
     size_t TypeAmount = (size_t) config_setting_length ( setting );
     ACfg->TypeList = mymalloc ( sizeof ( type_cfg_t* ) * ( TypeAmount + 1 ) );
     
     for ( size_t i = 0; i < TypeAmount; i++ )
     {
         config_setting_t *subsetting = config_setting_get_elem ( setting, i );
         __config_read_type ( subsetting, ACfg, i );
     }
     
     ACfg->TypeList[ TypeAmount ] = NULL;
     
     return 0;
}

// чтение конфигурации счётчика
static
int __config_read_counter ( config_setting_t *setting, askue_cfg_t *ACfg, size_t Number )
{
    const char *CounterTimeout;
    if ( config_setting_lookup_string ( setting, "timeout", &CounterTimeout ) != CONFIG_TRUE )
    {
        return -1;
    }
    
    ACfg->DeviceList[ Number ]->Class = Askue_Counter;
    ACfg->DeviceList[ Number ]->Timeout = strtol ( CounterTimeout, NULL, 10 );
    
    return 0;
}

// чтение конфигурации модема
static
int __config_read_modem ( config_setting_t *setting, askue_cfg_t *ACfg, size_t Number )
{
    const char *ModemSegment;
    if ( config_setting_lookup_string ( setting, "segment", &ModemSegment ) != CONFIG_TRUE )
    {
        return -1;
    }
    
    if ( !strcmp ( ModemSegment, "remote" ) )
    {
        ACfg->DeviceList[ Number ]->Timeout = 0;
        ACfg->DeviceList[ Number ]->Class = Askue_Modem;
        return 0;
    }
    else if ( !strcmp ( ModemSegment, "local" ) )
    {
        const char *ModemTimeout;
        if ( config_setting_lookup_string ( setting, "timeout", &ModemTimeout ) != CONFIG_TRUE )
        {
            return -1;
        }
        
        ACfg->DeviceList[ Number ]->Class = Askue_Modem;
        ACfg->DeviceList[ Number ]->Timeout = strtol ( ModemTimeout, NULL, 10 );
        
        return 0;
    }
    else
    {
        return -1;
    }
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
device_class_t what_device_class ( const char *DeviceClass )
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

// чтение конфигурации одного устройства
static
int __config_read_device ( config_setting_t *setting, askue_cfg_t *ACfg, size_t Number )
{
    ACfg->DeviceList[ Number ] = mymalloc ( sizeof ( device_cfg_t ) );
    
    const char *DeviceSetting[ 5 ];
    
    #define DeviceClass DeviceSetting[ 0 ]
    #define DeviceType DeviceSetting[ 1 ]
    #define DeviceId DeviceSetting[ 2 ]
    #define DeviceTimeout DeviceSetting[ 3 ]
    #define DeviceFlag DeviceSetting[ 4 ]
    
    if ( !( config_setting_lookup_string ( setting, "class", &(DeviceClass) ) == CONFIG_TRUE && 
            config_setting_lookup_string ( setting, "type", &(DeviceType) ) == CONFIG_TRUE && 
            config_setting_lookup_string ( setting, "id", &(DeviceId) ) == CONFIG_TRUE ) )
    {
         write_msg ( stderr, "Чтение конфигурации", "FAIL", "Запись 'DeviceList' не полная" );
         return -1;
    }
    
    int Result = 0;
    switch ( what_device_class ( DeviceClass ) )
    {
        case Askue_Modem: 
            Result = __config_read_modem ( setting,ACfg, Number );
            break;
        case Askue_Counter:
            Result = __config_read_counter ( setting, ACfg, Number );
            break;
        default:
            Result = -1;
            break;
    }
    if ( Result )
    {
         write_msg ( stderr, "Чтение конфигурации", "FAIL", "Запись 'DeviceList' не полная" );
         return -1;
    }
    
    ACfg->DeviceList[ Number ]->Id = mystrdup( DeviceId );
    ACfg->DeviceList[ Number ]->Type = __find_device_type ( ACfg, DeviceType );
    if ( ACfg->DeviceList[ Number ]->Type == NULL )
    {
         write_msg ( stderr, "Чтение конфигурации", "FAIL", "Запись 'DeviceList' не полная" );
         return -1;
    }
    
    #undef DeviceClass
    #undef DeviceType
    #undef DeviceId
    #undef DeviceTimeout
    #undef DeviceFlag
    
    return 0;
}

// чтение конфигурации списка устройств
static
int __config_read_device_list ( config_t *cfg, askue_cfg_t *ACfg )
{
     config_setting_t *setting = config_lookup ( cfg, "DeviceList" ); // поиск сети
     if ( setting == NULL )
     {
         write_msg ( stderr, "Чтение конфигурации", "FAIL", "В конфигурации отсутствует запись 'DeviceList'" );
         return -1;
     }
     
     size_t DeviceAmount = (size_t) config_setting_length ( setting );
     ACfg->DeviceList = mymalloc ( sizeof( device_cfg_t* ) * ( DeviceAmount + 1 ) );
     
     int Result = 0;
     for ( size_t i = 0; i < DeviceAmount && !Result; i++ )
     {
         config_setting_t *subsetting = config_setting_get_elem ( setting, i );
         Result = __config_read_device ( subsetting, ACfg, i );
     }
     if ( Result ) return -1;
     ACfg->DeviceList[ DeviceAmount ] = NULL;
     
     return 0;
}

// чтение конфигурации отчётов
int __config_read_report_list ( config_t *cfg, askue_cfg_t *ACfg )
{
     config_setting_t *setting = config_lookup ( cfg, "ReportList" ); // поиск сети
     if ( setting == NULL )
     {
         write_msg ( stderr, "Чтение конфигурации", "FAIL", "В конфигурации отсутствует запись 'ReportList'" );
         return -1;
     }
     
     size_t ReportAmount = (size_t) config_setting_length ( setting );
     ACfg->ReportList = mymalloc ( sizeof( report_cfg_t* ) * ( ReportAmount + 1 ) );
     
     for ( size_t i = 0; i < ReportAmount; i++ )
     {
         ACfg->ReportList[ i ] = mymalloc ( sizeof( report_cfg_t ) );
         const char *ReportName = config_setting_get_string_elem ( setting, i );
         ACfg->ReportList[ i ]->Name = mystrdup ( ReportName );
     }
     
     ACfg->ReportList[ ReportAmount ] = NULL;
     
     return 0;
}

/*                      Точка чтения конфигурации                     */
int askue_config_read ( askue_cfg_t *ACfg )
{
    int Result = -1; 
    config_t cfg;
	config_init ( &cfg ); // выделить память под переменную с конфигурацией
	if ( config_read_file ( &cfg, ASKUE_CONFIG_FILE ) == CONFIG_TRUE ) // открыть и прочитать файл
	{
        // порт
        if ( __config_read_db ( &cfg, ACfg ) == 0 &&
             __config_read_log ( &cfg, ACfg ) == 0 &&
             __config_read_port ( &cfg, ACfg ) == 0 && 
             __config_read_type_list ( &cfg, ACfg ) == 0 &&
             __config_read_report_list ( &cfg, ACfg ) == 0 &&
             __config_read_device_list ( &cfg, ACfg ) == 0 )
        {
            Result = 0;
        }
    }
    else
    {
        char Buffer[ 256 ];
        snprintf ( Buffer, 256, "%s at line %d", config_error_text ( &cfg ), config_error_line ( &cfg ) );
        write_msg ( stderr, "Открытие файла конфигурации", "FAIL", Buffer );
    }
    config_destroy ( &cfg );
    return Result;
}

