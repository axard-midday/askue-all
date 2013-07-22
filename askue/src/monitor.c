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
#include <signal.h>

#include "config.h"
#include "main_loop.h"
#include "write_msg.h"
#include "script_option.h"
#include "device_loop.h"
#include "macro.h"
#include "log.h"


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

// предустановка опций скрипта
static
void __script_option_preset ( script_option_t *ScriptOption, const askue_cfg_t *ACfg )
{
    script_option_init ( ScriptOption );
    script_option_set ( ScriptOption, SA_PORT_DBITS, ACfg->Port->DBits );
    script_option_set ( ScriptOption, SA_PORT_SBITS, ACfg->Port->SBits );
    script_option_set ( ScriptOption, SA_PORT_FILE, ACfg->Port->File );
    script_option_set ( ScriptOption, SA_PORT_PARITY, ACfg->Port->Parity );
    script_option_set ( ScriptOption, SA_PORT_SPEED, ACfg->Port->Speed );
    script_option_set ( ScriptOption, SA_JOURNAL_FILE, ACfg->Journal->File );
    script_option_set ( ScriptOption, SA_JOURNAL_FLASHBACK, ACfg->Journal->Flashback );
    script_option_set ( ScriptOption, SA_LOG_FILE, ACfg->Log->File );
}

typedef struct
{
    const gate_cfg_t *LocalGate;
    const gate_cfg_t **RemoteGateList;
    const device_cfg_t **DeviceList;
    const sigset_t *SignalSet;
} inquire_input_t;

typedef struct
{
    script_option_t *ScriptOption;
    loop_state_t *LoopStatus;
    FILE *Log;
} inquire_var_t;

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
        write_log ( Log, "Опрос", "FAIL", Buffer );
        
        return LoopError;
    }
    else if ( WaitpidReturn == 0 )
    {
        write_log ( Log, "Опрос", "FAIL", "Ложный сигнал SIGCHLD" );
        return LoopError;
    }
    
    if ( WIFEXITED ( status ) ) // успешное завершение, т.е. через exit или return
    {
        int code = WEXITSTATUS ( status );
        if ( code != EXIT_SUCCESS )
        {
            if ( snprintf ( Buffer, 256, "Скрипт '%s' завершён с кодом: %d", ScriptName, code ) > 0 ) 
                write_log ( Log, "Опрос", "FAIL", Buffer );
            else
                return LoopError;
        }
    }
    else if ( WIFSIGNALED ( status ) ) // завершение по внешнему сигналу
    {
        int sig = WTERMSIG ( status );
        if ( snprintf ( Buffer, 256, "Скрипт '%s' завершён по сигналу: %d", ScriptName, sig ) > 0 )
            write_log ( Log, "Опрос", "FAIL", Buffer );
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
            write_log ( Log, "Сигнал", "ERROR", Buffer );
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
                write_log ( Log, "Сигнал", "OK", "Выполнить переконфигурацию" );
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
    /*
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
    */
    write_log ( Log, "Опрос", "OK", "Запуск скрипта:" );
    for ( size_t i = 0; i < SA_AMOUNT; i++ )
    {
        if ( TESTBIT ( ScriptOptions->Flag, i ) )
            printf ( "%s ", ScriptOptions->Value[ i ] );
    }
    printf ( "\n" );
    sleep ( 5 );
    
    
    return LS;
}

// условия переходя к следующему скрипту
static
int __condition_2 ( const inquire_var_t *_Var, const device_cfg_t *Device, size_t i )
{
    return ( Device->Type->Script[ i ] != NULL ) &&
            ( *(_Var->LoopStatus) == LoopOk );
}

// перебор скриптов
static
void __foreach_script ( inquire_var_t *_Var, const device_cfg_t *Device, const sigset_t *SignalSet )
{
    script_option_unset ( _Var->ScriptOption, SA_PARAMETR );
    script_option_set ( _Var->ScriptOption, SA_TIMEOUT, Device->Timeout );
    script_option_set ( _Var->ScriptOption, SA_DEVICE, Device->Name );
    
    for ( size_t i = 0; __condition_2 ( _Var, Device, i ); i++ )
    {
        if ( Device->Type->Script[ i ]->Parametr != NULL )
            script_option_set ( _Var->ScriptOption, SA_PARAMETR, Device->Type->Script[ i ]->Parametr );
            
        *(_Var->LoopStatus) =  __run_script ( _Var->Log, Device->Type->Script[ i ]->Name, 
                               _Var->ScriptOption, SignalSet );
    }
}

// условие перехода к следубщему устройству
static
int __condition_1 ( const inquire_var_t *_Var, const inquire_input_t *_Input, size_t i )
{
    return ( _Input->DeviceList[ i ] != NULL ) &&
            ( *(_Var->LoopStatus) == LoopOk );
}

// открыть соединение
static
int __init_connection ( const gate_cfg_t *LocalGate, const gate_cfg_t *RemoteGate )
{
    return 1;
}

// цикл опроса устройст
static
int __foreach_device ( inquire_var_t *_Var, const inquire_input_t *_Input )
{
    for ( size_t i = 0; __condition_1 ( _Var, _Input, i ); i++ )
    {
        int Connection = 0;
        
        if ( _Input->DeviceList[ i ]->Segment == Askue_Remote )
        {
            const gate_cfg_t *RemoteGate = __find_remote_gate ( _Input->RemoteGateList,
                                                                  _Input->DeviceList[ i ]->Id );
            if ( RemoteGate != NULL )
            {
                Connection = __init_connection ( _Input->LocalGate, RemoteGate );
            }
        }
        else
        {
            Connection = 1;
        }
        
        if ( Connection )
            __foreach_script ( _Var, _Input->DeviceList[ i ], _Input->SignalSet );
    }
    
    return *( _Var->LoopStatus ) == LoopOk;
}

// цикл сбора показаний
loop_state_t run_monitor_loop ( FILE *Log, const askue_cfg_t *ACfg, const sigset_t *SignalSet )
{
    loop_state_t LoopStatus;
    
    script_option_t ScriptOption;
    __script_option_preset ( &ScriptOption, ACfg );
    // входы монитора
    inquire_input_t Input;
    Input.LocalGate = ( const gate_cfg_t* ) ACfg->LocalGate;
    Input.RemoteGateList = ( const gate_cfg_t** ) ACfg->RemoteGateList;
    Input.DeviceList = ( const device_cfg_t** ) ACfg->DeviceList;
    Input.SignalSet = SignalSet;
    // переменные монитора
    inquire_var_t MVar;
    MVar.Log = Log;
    MVar.LoopStatus = &LoopStatus;
    MVar.ScriptOption = &ScriptOption;
    
    while ( __foreach_device ( &MVar, &Input ) );
    
    __log_loop_state ( Log, LoopStatus );
    
    return LoopStatus;
}




