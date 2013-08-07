#include <libaskue.h>
#include <stdio.h>
#include <stdlib.h>

script_env_t *Env;
script_arg_t *Arg;
script_stream_t *Stream;

/* обработчик сигнала на завершение */
static
void __signal_handler ( int S )
{
    if ( S == SIGUSR2 )
        write_msg ( Env->Log, "Скрипт", "ОК", "Пришёл сигнал SIGUSR2. Выполняется завершение скрипта." );
    else
        write_msg ( Env->Log, "Скрипт", "ОК", "Пришёл другой сигнал. Выполняется завершение скрипта." );
        
    exit ( EXIT_BYSIGNAL );
}

/* освобождение памяти при завершении */
static
void __script_end ( void )
{
    script_env_destroy ( Env, NULL );
    script_env_delete ( Env );
    script_arg_delete ( Arg );
    script_stream_delete ( Stream );
}

void script_start ( void )
{
    script_arg_new ( &Arg );
    script_env_new ( &Env );
    script_stream_new ( &Stream );
}

/* тело скрипта */
static
int script_body ( void )
{
    write_msg ( Stream->Log, "Скрипт", "ОК", "Hello, World!" );
    
    return 0;
}

int main ( int argc, char **argv )
{
    // обработчик завершения скрипта
    atexit ( __script_end );
    // обработчик сигнала от родителя
    signal ( SIGUSR2, __signal_handler );
    
    // инициализация аргументов
    if ( script_arg_init ( Arg, argc, argv ) != 0 )
    {
        exit ( EXIT_FAILURE );
    }
    // инициализация окружения
    if ( script_env_init ( Env, Arg, NULL ) != 0 )
    {
        exit ( EXIT_FAILURE );
    }
    // инициализация потока ввода-вывода
    Stream->Log = Env->Log;
    Stream->Flag = Env->Flag;
    
    /* тело скрипта */
    if ( script_body () != 0 )
    {
        exit ( EXIT_FAILURE );
    }
    
    return 0;
}

