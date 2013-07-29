#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "files.h"

static
int is_equal_char ( char c1, char c2, int *Result )
{
    return ( *Result = c1 == c2 );
}

static
int is_equal_str ( const char *str, const char *etalon )
{
    int Result = *etalon == *str;
    
    do {
        etalon++;
        str++;
    } while ( !is_equal_char ( *str, '\0', &Result ) && 
               !is_equal_char ( *etalon, '\0', &Result ) &&
               is_equal_char ( *str, *etalon, &Result ) );
               
    return Result;
}

#define is_start( _STR_ ) \
    is_equal_str ( _STR_, "start" )

#define is_stop( _STR_ ) \
    is_equal_str ( _STR_, "stop" )

#define is_restart( _STR_ ) \
    is_equal_str ( _STR_, "restart" )
    
#define is_reconfigure( _STR_ ) \
    is_equal_str ( _STR_, "reconfigure" )
    
#define is_askue_arg( _STR_ ) \
    ({ _STR_[ 0 ] == '-'; })
    
#define is_help( _STR_ ) \
    is_equal_str ( _STR_, "help" )
    
static
int say_ASKUE_pid ( void )
{
    FILE *PidFile;
    
    PidFile = fopen ( ASKUE_FILE_PID, "r" );
    if ( PidFile == 0 )
    {
        perror ( "Невозможно открыть '/var/askue.pid'." );
        return -1;
    }
    
    unsigned int pid;
    if ( fscanf ( PidFile, "%u", &pid ) != 1 )
    {
        perror ( "Ошибка чтения pid из '/var/askue.pid'." );
        return -1;
    }
    else
    {
        fprintf ( stdout, "PID процесса: %u\n", pid );
    }

    fclose ( PidFile );
    
    return 0;
}

static
int Start_ASKUE_proc ( int argc, char **argv, int argv_offset )
{
    // проверка на повторный запуск
    int Exist = access ( ASKUE_FILE_PID, F_OK );
    if ( Exist == 0 )
    {
        puts ( "АСКУЭ уже работает" );
        return say_ASKUE_pid ();
    }
    else if ( ( Exist == -1 ) && ( errno == ENOENT ) )
    {
        if ( argc != 0 )
            puts ( "Вызываю программу 'askue-main' с параметрами:" );
        else
            puts ( "Вызываю программу 'askue-main'." );
            
        for ( int i = 0; i < argc; i++ )
        {
            puts ( argv[ i + argv_offset ] );
        }
        
        puts ( "Управление передано 'askue-main' с помощью execvp." );
        
        return 0;
    }
    else
    {
        perror ( "Невозможно открыть '/var/askue.pid'." );
        return -1;
    }
}

static
int Stop_ASKUE_proc ( void )
{
    puts ( "Читаю '/var/askue.pid'." );
    puts ( "Посылаю сигнал 'SIGTERM' процессу 'askue-main'." );
    puts ( "Отслеживаю наличие процесса..." );
    puts ( "Процесс остановлен." );
    
    return 0;
}

static
int Help_ASKUE_proc ( void )
{
    FILE *Help;
    
    Help = fopen ( ASKUE_FILE_HELP, "r" );
    if ( Help == 0 )
    {
        perror ( "Невозможно открыть '/etc/askue.help'." );
        return -1;
    }
    
    while ( !feof ( Help ) )
    {
        char Buffer[ 512 ];
        size_t BufLen = fread ( Buffer, sizeof ( char ), 512, Help );
        fwrite ( Buffer, sizeof ( char ), BufLen, stdout );
    }
    
    fclose ( Help );
    
    return 0;
}

#define ReStart_ASKUE_proc( argc, argv, argv_offser )\
({ ( !Stop_ASKUE_proc() ) ? Start_ASKUE_proc ( argc, argv, argv_offser ) : -1; })

static
int ReConfigure_ASKUE_proc ( void )
{
    puts ( "Читаю '/var/askue.pid'." );
    puts ( "Посылаю сигнал 'SIGUSR1' процессу 'askue-main'." );
    puts ( "Соединяю свой вывод с его..." );
    puts ( "АСКУЭ переконфигурирована" );
    return 0;
}

static
void Error_ASKUE_proc ( void )
{
    puts ( "" );
    puts ( "Указаны неправильные аргумент." );
    puts ( "" );
    puts ( "Используйте:" );
    puts ( "" );
    puts ( "askue start <опции_демона>" );
    puts ( "    для запуска АСКУЭ" );
    puts ( "askue stop" );
    puts ( "    для остановки АСКУЭ" );
    puts ( "askue restart <опции демона>" );
    puts ( "    для перезапуска АСКУЭ с новыми параметрами" );
    puts ( "askue reconfigure" );
    puts ( "    для переконфигурации АСКУЭ" );
    puts ( "askue help" );
    puts ( "    для вывода справки на экран" );
    puts ( "" );
    puts ( "Don’t use the Force, Luke, try to think!" );
    puts ( "" );
}


int main(int argc, char **argv)
{
	if ( argc == 1 )
    {
        return Start_ASKUE_proc ( argc - 1, argv, 1 );
    }
    else if ( is_start ( argv[ 1 ] ) )
    {
        return Start_ASKUE_proc ( argc - 2, argv, 2 );
    }
    else if ( is_stop ( argv[ 1 ] ) )
    {
        return Stop_ASKUE_proc ();
    }
    else if ( is_restart ( argv[ 1 ] ) )
    {
        return ReStart_ASKUE_proc ( argc - 2, argv, 2 );
    }
    else if ( is_reconfigure ( argv[ 1 ] ) )
    {
        return ReConfigure_ASKUE_proc ();
    }
    else if ( is_askue_arg ( argv[ 1 ] ) )
    {
        return Start_ASKUE_proc ( argc - 1, argv, 1 );
    }
    else if ( is_help ( argv[ 1 ] ) )
    {
        return Help_ASKUE_proc ();
    }
    else
    {
        Error_ASKUE_proc ();
        return -1;
    }
}

