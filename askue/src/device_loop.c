#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

#include "config.h"
#include "main_loop.h"
#include "write_msg.h"
#include "script_option.h"
#include "device_loop.h"
#include "macro.h"
#include "log.h"

// Выполнение скрипта
static
void __exec_script ( FILE *Log, const char *ScriptName, const script_option_t *ScriptOptions )
{
    const char *Argv[ SA_AMOUNT + 1 ];
    
    for ( size_t i = 0; i < SA_AMOUNT + 1; i++ )
        Argv[ i ] = NULL;
    
    int j = 0;
    for ( int i = SA_PORT_FILE; i < SA_AMOUNT; i++ )
        if ( TESTBIT ( ScriptOptions->Flag, i ) )
        {
            Argv[ j ] = ( const char* ) ScriptOptions->Value[ i ];
            j++;
        }
    
    // fclose ( Log )
    askue_log_close ( &Log );

    if ( execvp ( ScriptName, ( char * const * ) Argv ) )
    {
        exit ( EXIT_FAILURE );
    }
}

// проверить результат выполнения скрипта
static
loop_state_t __wait_script_result ( FILE *Log, pid_t pid, const char *ScriptName )
{
    int status;
    char Buffer[ 256 ];
    
    pid_t WaitpidReturn = waitpid ( pid, &status, WNOHANG );
    if ( WaitpidReturn == -1 )
    {
        snprintf ( Buffer, 256, "wait(): %s (%d)", strerror ( errno ), errno );
        write_msg ( Log, "Опрос", "FAIL", Buffer );
        
        return LoopError;
    }
    else if ( WaitpidReturn == 0 )
    {
        write_msg ( Log, "Опрос", "FAIL", "Ложный сигнал SIGCHLD" );
        return LoopError;
    }
    
    if ( WIFEXITED ( status ) ) // успешное завершение, т.е. через exit или return
    {
        int code = WEXITSTATUS ( status );
        if ( code != EXIT_SUCCESS )
        {
            if ( snprintf ( Buffer, 256, "Скрипт '%s' завершён с кодом: %d", ScriptName, code ) > 0 ) 
                write_msg ( Log, "Опрос", "FAIL", Buffer );
            else
                return LoopError;
        }
    }
    else if ( WIFSIGNALED ( status ) ) // завершение по внешнему сигналу
    {
        int sig = WTERMSIG ( status );
        if ( snprintf ( Buffer, 256, "Скрипт '%s' завершён по сигналу: %d", ScriptName, sig ) > 0 )
            write_msg ( Log, "Опрос", "FAIL", Buffer );
        else
            return LoopError;
    }
    
    return LoopOk;
}

// обработать сигнал
loop_state_t __wait_signal ( FILE *Log, pid_t pid, const char *ScriptName, const sigset_t *SignalSet )
{
    siginfo_t SignalInfo;
    
    if ( sigwaitinfo ( SignalSet, &SignalInfo ) )
    {
        if ( errno == EAGAIN )
        {
            return LoopContinue;
        }
        else
        {
            // сообщение об ошибке
            char Buffer[ 256 ];
            snprintf ( Buffer, 256, "wait(): %s (%d)", strerror ( errno ), errno );
            write_msg ( Log, "Сигнал", "ERROR", Buffer );
            return LoopError;
        }
    }
    else
    {
        loop_state_t Result;
        switch ( SignalInfo.si_signo )
        {
            case SIGCHLD:
                Result = __wait_script_result ( Log, pid, ScriptName );
                break;
            case SIGHUP:
                write_msg ( Log, "Сигнал", "OK", "Выполнить переконфигурацию" );
                Result = LoopReconfig;
                break;
            default:
                Result = LoopExit;
                break;
        }
        return Result;
    }
}

// ожидание результата
static
loop_state_t __wait_result ( FILE *Log, pid_t pid, const char *ScriptName, const sigset_t *SignalSet )
{
    loop_state_t LS = LoopContinue;
    while ( ( LS = __wait_signal ( Log, pid, ScriptName, SignalSet ) ) == LoopContinue );
    return LS;
}


// вывод в лог сообщения об ошибке
static
void __log_loop_state ( FILE *Log, loop_state_t LS )
{
    if ( LS == LoopError )
    {
        write_log ( Log, "Опрос", "FAIL", "Цикл опроса прерван в связи с ошибкой" );
    }    
    else if ( LS == LoopExit ) 
    {
        write_log ( Log, "Опрос", "OK", "Цикл опроса прерван в связи с сигналом завершения" );
    }
    else if ( LS == LoopReconfig )
    {
        write_log ( Log, "Опрос", "OK", "Цикл опроса прерван в связи с сигналом переконфигурации" );
    }
}

// ошибка запуска скрипта на уровне форка
static
void __run_script_error ( FILE *Log )
{ 
    char Buffer[ 256 ];
    snprintf ( Buffer, 256, "fork(): %s (%d)", strerror ( errno ), errno );
    write_log ( Log, "Запуск скрипта", "FAIL", "fork()" );
}


// запуск скрипты
static
loop_state_t __run_script ( FILE *Log, const char *Script, const script_option_t *ScriptOptions, const sigset_t *SignalSet )
{
    loop_state_t LS = LoopOk;
    
    pid_t ScriptPid = fork ();
    if ( ScriptPid < 0 )
    {
        __run_script_error ( Log );
        LS = LoopError;
    }
    else if ( ScriptPid == 0 )
    {
        __exec_script ( Log, Script, ScriptOptions );
    }
    else
    {
        LS = __wait_result ( Log, ScriptPid, Script, SignalSet );
    }
    
    return LS;
}

// перебор доступных скриптов
static
loop_state_t __script_loop ( FILE *Log, const type_cfg_t *Type, const script_option_t *ScriptOptions, const sigset_t *SignalSet )
{
    loop_state_t LS = LoopOk;
    
    for ( size_t i = 0; LS == LoopOk && Type->Script[ i ] != NULL; i++ )
    {
        LS = __run_script ( Log, Type->Script[ i ], ScriptOptions, SignalSet );
    }
    
    return LS;
}

// найти локальный модем
const gate_cfg_t* __find_local_gate ( const gate_cfg_t **GateList )
{
    const gate_cfg_t *Gate = NULL;
    for ( size_t i = 0; GateList[ i ] != NULL && Gate == NULL; i++ )
    {
        if ( GateList[ i ]->Device->Segment == Askue_Local )
            Gate = GateList[ i ];
    }
    return Gate;
}

// найти модем с таким же id
const gate_cfg_t* __find_remote_gate ( const gate_cfg_t **GateList, long int Id )
{
    const gate_cfg_t *Gate = NULL;
    for ( size_t i = 0; GateList[ i ] != NULL && Gate == NULL; i++ )
    {
        if ( GateList[ i ]->Device->Id == Id )
            Gate = GateList[ i ];
    }
    return Gate;
}

// основной цикл программы
loop_state_t device_loop ( FILE *Log, const askue_cfg_t *ACfg, const sigset_t *SignalSet )
{
    loop_state_t LS = LoopOk;
    const device_cfg_t **DL = ( const device_cfg_t ** ) ACfg->DeviceList;
    const gate_cfg_t *LocalGate = __find_local_gate ( ACfg->GateList );
    const gate_cfg_t *CurrentGate = NULL;
    
    script_option_t ScriptOption;
    script_option_init ( &ScriptOption );
    script_option_set ( &ScriptOption, SA_PORT_DBITS, ACfg->Port->DBits );
    script_option_set ( &ScriptOption, SA_PORT_SBITS, ACfg->Port->SBits );
    script_option_set ( &ScriptOption, SA_PORT_FILE, ACfg->Port->File );
    script_option_set ( &ScriptOption, SA_PORT_PARITY, ACfg->Port->Parity );
    script_option_set ( &ScriptOption, SA_PORT_SPEED, ACfg->Port->Speed );
    script_option_set ( &ScriptOption, SA_JOURNAL_FILE, ACfg->Journal->File );
    script_option_set ( &ScriptOption, SA_JOURNAL_FLASHBACK, ACfg->Journal->Flashback );
    script_option_set ( &ScriptOption, SA_LOG_FILE, ACfg->Log->File );
    
    size_t i = 0;
    while ( LS == LoopOk )
    {
        int IsConnect = 1;
        
        if ( DL[ i ] == NULL )
        {
            i = 0;
        }
        
        if ( DL[ i ]->Segment == Askue_Remote )
        {
            const gate_cfg_t *RemoteGate = __find_remote_gate ( ACfg->GateList, DL[ i ]->Id );
            
            if ( CurrentGate != RemoteGate )
            {
                CurrentGate = RemoteGate;
                
                script_option_set ( &ScriptOption, SA_PARAMETR, RemoteGate->Device->Name );
                script_option_set ( &ScriptOption, SA_TIMEOUT, LocalGate->Device->Timeout );
                script_option_set ( &ScriptOption, SA_DEVICE, LocalGate->Device->Name );
                
                LS = __gate_open ( Log, LocalGate->Device->Type, ScriptOption, SignalSet );
                if ( LS == LoopOk )
                    IsConnect = 1;
            }  
        }
        else
        {
            CurrentGate = NULL;
        }
        
        // обработка скриптов устройства
        if ( IsConnect )
        {
            script_option_unset ( &ScriptOption, SA_PARAMETR );
            script_option_set ( &ScriptOption, SA_TIMEOUT, DL[ i ]->Timeout );
            script_option_set ( &ScriptOption, SA_DEVICE, DL[ i ]->Name );
            LS = __script_loop ( Log, DL[ i ]->Type, ScriptOption, SignalSet );
        }
        
        i++;
    }
    
    __log_loop_state ( Log, LS );
    
    return LS;
}


