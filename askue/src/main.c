#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include "config.h"
#include "monitor.h"
#include "write_msg.h"
#include "journal.h"
#include "macro.h"
#include "log.h"

typedef enum _askue_cfg_flag_t
{
    Askue_ReConfigure,
    Askue_Configure
} askue_cfg_flag_t;

static
int askue_configure ( askue_cfg_t *Cfg, FILE **Log, askue_cfg_flag_t CfgFlag )
{
    if ( CfgFlag == Askue_ReConfigure )
    {
        askue_config_destroy ( Cfg );
        askue_log_close ( Log );
    }
        
    askue_config_init ( Cfg );
    
    if ( !askue_config_read ( Cfg ) &&
         !askue_journal_init ( Cfg ) &&
         !askue_log_open ( Log, Cfg ) )
    {
        return 0;
    }
    else
    {
        askue_config_destroy ( Cfg );
        return -1;
    }
}

static
loop_state_t main_loop ( askue_cfg_t *Cfg, FILE *Log, const sigset_t *SignalSet )
{
    loop_state_t LS = LoopOk;
    
    while ( LS != LoopError && LS != LoopExit )
    {
        LS = run_monitor_loop ( Log, Cfg, SignalSet );
        if ( LS == LoopReconfig )
            LS = ( askue_configure ( Cfg, &Log, Askue_ReConfigure ) ) ? LoopError : LoopOk;
    }
    
	return LS;
}

static
void signal_set_init ( sigset_t *sigset )
{
    // настраиваем сигналы которые будем обрабатывать
    sigemptyset(sigset);
    // сигнал остановки процесса пользователем
    sigaddset(sigset, SIGQUIT);
    // сигнал для остановки процесса пользователем с терминала
    sigaddset(sigset, SIGINT);
    // сигнал запроса завершения процесса
    sigaddset(sigset, SIGTERM);
    // сигнал посылаемый при изменении статуса дочернего процесса
    sigaddset(sigset, SIGCHLD); 
    // пользовательский сигнал который мы будем использовать для обновления конфига
    sigaddset(sigset, SIGUSR1); 
    sigprocmask(SIG_BLOCK, sigset, NULL);
}

int main ( int argc, char **argv )
{
    FILE *Log;
    askue_cfg_t Cfg;
    char Buffer[ 256 ];
    if ( !askue_configure ( &Cfg, &Log, Askue_Configure ) )
    {
        pid_t pid = fork ();
        if ( pid < 0 )
        {
            if ( snprintf ( Buffer, 256, "fork(): %s (%d)", strerror ( errno ), errno ) != -1 )
                write_msg ( stdout, "Запуск скрипта", "FAIL", Buffer );
            exit ( EXIT_FAILURE ); 
        }
        else if ( !pid )
        {
            umask ( 0 );
            
            if ( setsid ( ) &&
                 snprintf ( Buffer, 256, "setsid(): %s (%d)", strerror ( errno ), errno ) != -1)
            {
                write_log ( Log, "Запуск в фоне", "FAIL", Buffer );
            }
            if ( chdir ( "/" ) &&
                 snprintf ( Buffer, 256, "chdir(): %s (%d)", strerror ( errno ), errno ) != -1 )
            {
                write_msg ( stdout, "Запуск скрипта", "FAIL", Buffer );
            }
            
            sigset_t SignalSet;
            signal_set_init ( &SignalSet );
            
            if ( main_loop ( &Cfg, Log, ( const sigset_t* ) &SignalSet ) == LoopExit )
            {
                exit ( EXIT_SUCCESS );
            }
            else
            {
                exit ( EXIT_FAILURE );
            }
        }
        else
        {
            exit ( EXIT_SUCCESS );
        }
    }
    else
    {
        exit ( EXIT_FAILURE );
    }
}

