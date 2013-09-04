#include <stdarg.h>
#include <stdlib.h>

#include "journal.h"
#include "cli.h"
#include "macro.h"
#include "port.h"
#include "rs232.h"
#include "script_arg.h"


// установка флага пояснений
static
int parse4flag_verbose ( void *ptr, const char *value )
{
    //return SETBIT ( ( *( uint32_t* )ptr ), ASKUE_FLAG_VERBOSE ) == 0;
    *( uint32_t* )ptr = 1;
    return 0;
}

// установка флага протокола
static
int parse4flag_protocol ( void *ptr, const char *value )
{
    //return SETBIT ( ( *( uint32_t* )ptr ), ASKUE_FLAG_PROTOCOL ) == 0;
    *( uint32_t* )ptr = 1;
    return 0;
}

// установка флага отладки
static
int parse4flag_debug ( void *ptr, const char *value )
{
    //return SETBIT ( ( *( uint32_t* )ptr ), ASKUE_FLAG_DEBUG ) == 0;
    *( uint32_t* )ptr = 1;
    return 0;
}

// разбор флагов
static
cli_result_t parse4flag ( void *Flags, int argc, char **argv )
{
    cli_arg_t FlagArgs[] = 
    {
        { "verbose", 0, CLI_OPTIONAL_ARG, CLI_NO_VAL, parse4flag_verbose, Flags },
        { "protocol", 0, CLI_OPTIONAL_ARG, CLI_NO_VAL, parse4flag_protocol, Flags },
        { "debug", 0, CLI_OPTIONAL_ARG, CLI_NO_VAL, parse4flag_debug, Flags },
        CLI_LAST_ARG
    };
    
    return cli_parse ( FlagArgs, argc, argv );
}

// имя устройства
static 
cli_result_t parse4device ( void *Device, int argc, char **argv, cli_handler_f Handler )
{
    cli_arg_t DeviceArgs[] = 
    {
        { "device", 0, CLI_OPTIONAL_ARG, CLI_REQUIRED_VAL, Handler, Device },
        CLI_LAST_ARG
    };
    
    return cli_parse ( DeviceArgs, argc, argv );
}

// перевод из строкового вида в текстовый
int __parse4timeout ( void *Timeout, const char *value )
{
    return ( *( long int* ) Timeout = strtol ( value, NULL, 10 ) ) == 0;
}

// таймаут
static
cli_result_t parse4timeout ( void *Timeout, int argc, char **argv )
{
    cli_arg_t TimeoutArgs[] = 
    {
        { "timeout", 0, CLI_OPTIONAL_ARG, CLI_REQUIRED_VAL, __parse4timeout, Timeout },
        CLI_LAST_ARG
    };
    
    return cli_parse ( TimeoutArgs, argc, argv );
}

// имя устройства
static 
cli_result_t parse4parametr ( void *Parametr, int argc, char **argv, cli_handler_f Handler )
{
    cli_arg_t ParametrArgs[] = 
    {
        { "parametr", 0, CLI_OPTIONAL_ARG, CLI_REQUIRED_VAL, Handler, Parametr },
        CLI_LAST_ARG
    };
    
    return cli_parse ( ParametrArgs, argc, argv );
}

// открыть файл лога на добавление
static
int __parse4log ( void *Log, const char *value )
{
    *( FILE** ) Log = fopen ( value, "a" );
    return ( *( FILE** ) Log ) == NULL;
}

// лог
cli_result_t parse4log ( void *Log, int argc, char **argv )
{
    cli_arg_t LogArgs[] = 
    {
        { "log", 0, CLI_OPTIONAL_ARG, CLI_REQUIRED_VAL, __parse4log, Log },
        CLI_LAST_ARG
    };
    
    return cli_parse ( LogArgs, argc, argv );
}

int __parse4journal_file ( void *Journal, const char *value )
{
    sqlite3 *db;
    if ( sqlite3_open ( value, &db ) != SQLITE_OK )
    {
        return -1;
    }
    
    ( *( askue_jnl_t** ) Journal )->File = db;
    
    return 0;
}

int __parse4journal_flashback ( void *Journal, const char *value )
{
    return ( ( *( askue_jnl_t** ) Journal )->Flashback = ( size_t ) strtol ( value, NULL, 10 ) ) == 0;
}


// журнал
cli_result_t parse4journal ( void *Journal, int argc, char **argv )
{
    cli_arg_t JournalArgs[] = 
    {
        { "journal_file", 0, CLI_OPTIONAL_ARG, CLI_REQUIRED_VAL, __parse4journal_file, Journal },
        { "journal_flashback", 0, CLI_OPTIONAL_ARG, CLI_REQUIRED_VAL, __parse4journal_flashback, Journal },
        CLI_LAST_ARG
    };
    
    return cli_parse ( JournalArgs, argc, argv );
}

// получить стрококвое значение параметра
int __parse4port_strp ( void *strp, const char *value )
{
    *( const char ** )strp = value;
    return *value != '\0'; // проверка на пустой параметр
}

// порт
cli_result_t parse4port ( void *Port, int argc, char **argv )
{
    const char *PortCfg[ 5 ];
    #define File ( PortCfg[ 0 ] )
    #define Parity ( PortCfg[ 1 ] )
    #define Speed ( PortCfg[ 2 ] )
    #define DBits ( PortCfg[ 3 ] )
    #define SBits ( PortCfg[ 4 ] )
    
    // настроить порт
    cli_arg_t JournalArgs[] = 
    {
        { "port_file", 0, CLI_OPTIONAL_ARG, CLI_REQUIRED_VAL, __parse4port_strp, &File },
        { "port_parity", 0, CLI_OPTIONAL_ARG, CLI_REQUIRED_VAL, __parse4port_strp, &Parity },
        { "port_speed", 0, CLI_OPTIONAL_ARG, CLI_REQUIRED_VAL, __parse4port_strp, &Speed },
        { "port_sbits", 0, CLI_OPTIONAL_ARG, CLI_REQUIRED_VAL, __parse4port_strp, &DBits },
        { "port_dbits", 0, CLI_OPTIONAL_ARG, CLI_REQUIRED_VAL, __parse4port_strp, &SBits },
        CLI_LAST_ARG
    };
    
    cli_result_t Result = cli_parse ( JournalArgs, argc, argv );
    if ( Result != CLI_SUCCESS )
    {
        return Result;
    }
    
    if ( rs232_init ( ( *( askue_port_t** ) Port )->RS232, &( ( *( askue_port_t** ) Port )->Termios ) ) )
    {
        return CLI_ERROR_HANDLER;
    }
    
    rs232_set_speed ( &( ( *( askue_port_t** ) Port )->Termios ), Speed );
    rs232_set_databits ( &( ( *( askue_port_t** ) Port )->Termios ), DBits );
    rs232_set_stopbits ( &( ( *( askue_port_t** ) Port )->Termios ), SBits );
    rs232_set_parity ( &( ( *( askue_port_t** ) Port )->Termios ), Parity );
    
    if ( rs232_apply ( ( *( askue_port_t** ) Port )->RS232, &( ( *( askue_port_t** ) Port )->Termios ) ) )
    {
        return CLI_ERROR_HANDLER;
    }
    
    if ( port_init ( *( askue_port_t** ) Port, File, Speed, DBits, SBits, Parity ) == -1 )
    {
        return CLI_ERROR_HANDLER;
    }

    return Result;
    
    #undef File
    #undef Parity
    #undef Speed
    #undef DBits
    #undef SBits
}

/*
 * 
 * name: cli_parse_script_arg
 * @param
 *  void * - параметр ( изменяемый )
 *  cli_handler_f - инициализатор параметра
 *  int - имя аргумента
 *  int - кол-во аргументов в массиве
 *  char ** - массив аргументов
 * @return
 *  cli_result_t - результат разбора массива аргументов
 * 
 * Инициализация аргумента скрипта. Аргумент указывается в первом параметре.
 * Инициализатор во втором. Если инициализатор == NULL, используется библиотечный.
 * Если библиотечный отсутствует, то возврат -1.
 */
cli_result_t cli_parse_script_arg ( void *sa, int sa_name, int argc, char **argv, ... )
{
    cli_result_t Result;
    
    switch ( sa_name )
    {
        case SARG_FLAG:
            Result = parse4flag ( sa, argc, argv );
            break;
        case SARG_DEVICE:
            {
                va_list va;
                va_start ( va, argv );
                cli_handler_f _handler = va_arg ( va, cli_handler_f );
                Result = parse4device ( sa, argc, argv, _handler );
                va_end ( va );
            }
            break;
        case SARG_PARAMETR:
            {
                va_list va;
                va_start ( va, argv );
                cli_handler_f _handler = va_arg ( va, cli_handler_f );
                Result = parse4parametr ( sa, argc, argv, _handler );
                va_end ( va );
            }
            break;
        case SARG_LOG:
            Result = parse4log ( sa, argc, argv );
            break;
        case SARG_TIMEOUT:
            Result = parse4timeout ( sa, argc, argv );
            break;
        case SARG_JNL:
            Result = parse4journal ( sa, argc, argv );
            break;
        case SARG_PORT:
            Result = parse4port ( sa, argc, argv );
            break;
        
        default:
            break;
    }
    
    return Result;
}
