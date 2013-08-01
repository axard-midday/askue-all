/*
 * script_template.c
 * 
 * Copyright 2013 axard <axard@axard-desktop>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sqlite3.h>
#include <signal.h>

#define EXIT_BYSIGNAL EXIT_SUCCESS + 2

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
} device_arg_t;

typedef struct _journal_arg_t
{
    char *File;
    char *Flashback;
} journal_arg_t;

typedef struct _log_arg_t
{
    char *File;
    // char *Mode; == 'a' ВСЕГДА
} log_arg_t;
 
typedef struct _parametr_arg_t
{
    char *Value;
} parametr_arg_t;

typedef struct _script_arg_t
{
    port_arg_t Port;
    device_arg_t Device;
    journal_arg_t Journal;
    log_arg_t Log;
    parametr_arg_t Parametr;
} script_arg_t;

typedef struct _script_cfg_t
{
    FILE *Port;
    FILE *Log;
    FILE *Protocol;
    sqlite3 *Journal;
    size_t Flashback;
    long long int Device;
    long int Timeout;
    void *Parametr;
    uint32_t Flag;
} script_cfg_t;

script_cfg_t Cfg;


#define StartScript( _ARGC_, _ARGV_ ) int main ( int _ARGC_, char **_ARGV_ ) {
#define EndScript( _E_ ) exit ( _E_ ); return 0; }

#define InitScriptCfg( _CFG_ ) \
do{ \
    _CFG_.Port = NULL; \
    _CFG_.Log = NULL; \
    _CFG_.Flashback = 0; \
    _CFG_.Journal = NULL; \
    _CFG_.Device = 0; \
    _CFG_.Timeout = 0; \
    _CFG_.Parametr = NULL; \
    _CFG_.Flag = 0; \
}while(0)

#define InitScriptArg( _ARG_ ) \
do{ \
    _ARG_.Port.DBits = NULL; _ARG_.Port.File = NULL; _ARG_.Port.Parity = NULL; _ARG_.Port.SBits = NULL; _ARG_.Port.Speed = NULL; \
    _ARG_.Device.Name = NULL; _ARG_.Device.Timeout = NULL; \
    _ARG_.Journal.Flashback = NULL; _ARG_.Journal.File = NULL; \
    _ARG_.Log.File = NULL; \
    _ARG_.Parametr.Value = NULL; \
}while(0)

#define DestroyScriptCfg( _CFG_ ) \
do{ \
    fclose ( _CFG_.Log ); \
    sqlite3_close ( _CFG_.Journal ); \
    fclose ( _CFG_.Port ); \
    if ( _CFG_.Parametr != NULL ) free ( _CFG_.Parametr ); \
}while(0)

#define BreakScript( _E_ ) \
do{ \
    DestroyScriptCfg ( Cfg ); \
    exit ( _E_ ); \
}while(0)

#define InitScriptSignal( _SH_ ) \
do{ \
    if ( signal ( SIGUSR2, _SH_ ) == SIG_ERR ) \
    { BreakScript ( EXIT_FAILURE ); } \
}while(0)

void SignalHandler ( int s )
{
    DestroyScriptCfg ( Cfg );
    exit ( EXIT_BYSIGNAL );
}

/*
 * ConfigScript ( Cfg, argc, argv )
 * 1. script_arg_t Arg;
 * 2. InitScriptArg ( Arg );
 * 3. cli_option_t ScriptCliOption[] = { ... };
 * 4. parse_cli ( ScriptCliOption, argc, argv );
 * 5. открыть файл Cfg.Port;
 * 6. открыть файл Cfg.Log;
 * 7. открыть журнал ( базу данных ) Cfg.Journal;
 * 8. считать кол-во флешбеков Cfg.Flashback;
 * 9. считать номер устройства Cfg.Device;
 * 10. считать таймаут ожидания Cfg.Timeout;
 * 11. считать параметр ( с выделением памяти ) Cfg.Parametr;
 * 12. считать флаги Cfg.Flag
 */


StartScript ( argc, argv )

    InitScriptCfg ( Cfg );
    
    // ConfigScript ( Cfg, argc, argv, ParametrDecode );
    
    /*
     * тело скрипта
     * Здесь используются библиотечные функции обмена с портом
     * port_write ( const script_cfg_t *Cfg, const uint8_array_t *u8a );
     * port_read ( const script_cfg_t *Cfg, uint8_array_t *u8a );
     * Эти функции сбрасывают данные из u8a в лог при наличии флага --protocol в следующем виде:
     *  При чтении:
     *   [ <time> | Script | OK ]: In: <принятые байты>\n
     *  При записи:
     *   [ <time> | Script | OK ]: Out: <переданные байты>\n
     * решение о формате вывода принятых байт программа принимает сама, на основании проверки 
     * всего потока на вхождение в множество символов ASCII ( без учёта формата первого и последнего символа )
     * Первый и последний символ печатаются исходя из выбранного режима.
     * 
     * Так же используется функция save_registration ( const script_cfg_t *Cfg, const char *RegType, ... )
     * В качестве дополнительного аргумента может приниматься double или int.
     * При указании ключа --verbose в протокол и в лог происходя записи следующего вида
     *   [ <time> | Script | OK ]: <RegType>: <Value>\n
     * 
     * Проверка по CRC описана в библиотеке. 
     * Функции парсинга сообщения прописаны в скрипте. 
     * Разбор ошибок протокола прописан в скрипте.
     * 
     * Сообщения об ошибках в функциях генерятся внутри библиотеки и записываются в лог, если он указан.
     */
    
    printf ( "i = %i\n", sizeof ( int ) );
    
    // DestroyScriptCfg ( Cfg );
    InitScriptSignal ( SignalHandler );

EndScript ( EXIT_SUCCESS )

