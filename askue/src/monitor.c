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
#include "write_msg.h"
#include "script_option.h"
#include "macro.h"
#include "monitor.h"
#include "log.h"


typedef struct _inquire_input_t
{
    const gate_cfg_t *LocalGate;
    const gate_cfg_t **RemoteGateList;
    const device_cfg_t **DeviceList;
    const sigset_t *SignalSet;
} inquire_input_t;

typedef struct _inquire_var_t
{
    script_option_t *ScriptOption;
    loop_state_t *LoopStatus;
    FILE *Log;
} inquire_var_t;

typedef enum _sa_preset_t
{
    SA_PRESET_DEVICE,
    SA_PRESET_REPORT
} sa_preset_t;

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
void __script_option_preset ( script_option_t *ScriptOption, const askue_cfg_t *ACfg, sa_preset_t Preset )
{
    switch ( Preset )
    {
        case SA_PRESET_DEVICE:
            script_option_init ( ScriptOption );
            script_option_set ( ScriptOption, SA_PORT_DBITS, ACfg->Port->DBits );
            script_option_set ( ScriptOption, SA_PORT_SBITS, ACfg->Port->SBits );
            script_option_set ( ScriptOption, SA_PORT_FILE, ACfg->Port->File );
            script_option_set ( ScriptOption, SA_PORT_PARITY, ACfg->Port->Parity );
            script_option_set ( ScriptOption, SA_PORT_SPEED, ACfg->Port->Speed );
            script_option_set ( ScriptOption, SA_JOURNAL_FILE, ACfg->Journal->File );
            script_option_set ( ScriptOption, SA_JOURNAL_FLASHBACK, ACfg->Journal->Flashback );
            script_option_set ( ScriptOption, SA_LOG_FILE, ACfg->Log->File );
            break;
        case SA_PRESET_REPORT:
            // убрать ненужные опции
            script_option_unset ( ScriptOption, SA_TIMEOUT );
            script_option_unset ( ScriptOption, SA_DEVICE );
            script_option_unset ( ScriptOption, SA_PORT_FILE );
            script_option_unset ( ScriptOption, SA_PORT_PARITY );
            script_option_unset ( ScriptOption, SA_PORT_DBITS );
            script_option_unset ( ScriptOption, SA_PORT_SBITS );
            script_option_unset ( ScriptOption, SA_PORT_SPEED );
            break;
        default:
            break;
    }
}


// Выполнение скрипта
static
void __exec_script ( FILE *Log, const char *ScriptName, const script_option_t *ScriptOption )
{
    const char *Argv[ SA_AMOUNT + 1 ];
    
    for ( size_t i = 0; i < SA_AMOUNT + 1; i++ )
        Argv[ i ] = NULL;
    
    int j = 0;
    for ( int i = SA_PORT_FILE; i < SA_AMOUNT; i++ )
        if ( TESTBIT ( ScriptOption->Flag, i ) )
        {
            Argv[ j ] = ( const char* ) ScriptOption->Value[ i ];
            j++;
        }
    
    // fclose ( Log )
    askue_log_close ( &Log );
    //atexit ( script_exit_func );
    
    //write_log ( Log, "Скрипт", "ОК", "Скрипт начал работу" );
    //sleep ( 10 );
    //write_log ( Log, "Скрипт", "ОК", "Скрипт закончил работу" );
    //exit ( EXIT_SUCCESS );
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
        snprintf ( Buffer, 256, "waitpid(): %s (%d)", strerror ( errno ), errno );
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
    
    int s = sigwaitinfo ( SignalSet, &SignalInfo );
    if ( s == -1 )
    {
        // сообщение об ошибке
        char Buffer[ 256 ];
        snprintf ( Buffer, 256, "sigwaitinfo(): %s (%d)", strerror ( errno ), errno );
        write_log ( Log, "Сигнал", "ERROR", Buffer );
        return LoopError;
    }
    else
    {
        loop_state_t Result;
        
        switch ( SignalInfo.si_signo )
        {
            case SIGCHLD:
                Result = __wait_script_result ( Log, pid, ScriptName );
                break;
            case SIGUSR1:
                if ( kill ( pid, SIGUSR2 ) )
                    write_log ( Log, "Сигнал", "ERROR", "kill()" );
                write_log ( Log, "Сигнал", "OK", "Выполнить переконфигурацию" );
                Result = LoopReconfig;
                break;
            default:
                if ( kill ( pid, SIGUSR2 ) )
                    write_log ( Log, "Сигнал", "ERROR", "kill()" );
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
loop_state_t __run_script ( FILE *Log, const char *Script, const script_option_t *ScriptOption, const sigset_t *SignalSet )
{
    //write_log ( Log, "Опрос", "TEST", "\n" );
    //char Buffer[ 256 ];
    //snprintf ( Buffer, 256, "Запуск скрипта: %s", Script );
    //write_log ( Log, "Опрос", "TEST", Buffer );
    //snprintf ( Buffer, 256, "С опциями: " );
    //for ( size_t i = 0; i < SA_AMOUNT; i++ )
    //{
    //    if ( TESTBIT ( ScriptOption->Flag, i ) )
    //    {
    //        strcat ( Buffer, ScriptOption->Value[ i ] );
    //        strcat ( Buffer, " " );
    //    }
    //}
    //write_log ( Log, "Опрос", "TEST", Buffer );
    
    loop_state_t LS = LoopOk;
    pid_t ScriptPid = fork ();
    if ( ScriptPid < 0 )
    {
        __run_script_error ( Log );
        LS = LoopError;
    }
    else if ( ScriptPid == 0 )
    {
        __exec_script ( Log, Script, ScriptOption );
        //const char *Argv[] = { "test_script", NULL };
        //execvp ( "./test_script", ( char * const * ) Argv );
    }
    else
    {
        //snprintf ( Buffer, 256, "Pid потомка: %ld", ( long int )ScriptPid );
        //write_log ( Log, "Опрос", "TEST", Buffer );
        //LS = __wait_result ( Log, ScriptPid, Script, SignalSet );
        LS = __wait_signal ( Log, ScriptPid, Script, SignalSet );
    }
    
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
    script_option_set ( _Var->ScriptOption, SA_TIMEOUT, Device->Timeout );
    script_option_set ( _Var->ScriptOption, SA_DEVICE, Device->Name );
    
    for ( size_t i = 0; __condition_2 ( _Var, Device, i ); i++ )
    {
        if ( Device->Type->Script[ i ]->Parametr != NULL )
            script_option_set ( _Var->ScriptOption, SA_PARAMETR, Device->Type->Script[ i ]->Parametr );
        else
            script_option_unset ( _Var->ScriptOption, SA_PARAMETR );
            
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

// цикл опроса устройст
static
int __foreach_device ( inquire_var_t *_Var, const inquire_input_t *_Input )
{
    const gate_cfg_t *LastConnectedGate = NULL;
    
    for ( size_t i = 0; __condition_1 ( _Var, _Input, i ); i++ )
    {        
        char Buffer[ 256 ];
        if ( _Input->DeviceList[ i ]->Segment == Askue_Remote )
        {
            const gate_cfg_t *RemoteGate = __find_remote_gate ( _Input->RemoteGateList,
                                                                  _Input->DeviceList[ i ]->Id );
                                                                  
            if ( LastConnectedGate != RemoteGate )
            {
                //memset ( Buffer, '\0', 26 );
                //memset ( Buffer, '_', 25 );
                //write_log ( _Var->Log, "Опрос", "TEST", Buffer );
                LastConnectedGate = RemoteGate;
                //snprintf ( Buffer, 256, "Подключение через базовый '%s' к модему '%s'", 
                //            _Input->LocalGate->Device->Name, 
                //            RemoteGate->Device->Name );
                //write_log ( _Var->Log, "Опрос", "TEST", Buffer );
                if ( RemoteGate != NULL )
                {
                    __foreach_script ( _Var, RemoteGate->Device, _Input->SignalSet );
                }
            }
        }
        
        //memset ( Buffer, '\0', 26 );
        //memset ( Buffer, '_', 25 );
        //write_log ( _Var->Log, "Опрос", "TEST", Buffer );
        //snprintf ( Buffer, 256, "Устройство №%s типа '%s'", _Input->DeviceList[ i ]->Name, _Input->DeviceList[ i ]->Type->Name );
        //write_log ( _Var->Log, "Опрос", "TEST", Buffer );
        __foreach_script ( _Var, _Input->DeviceList[ i ], _Input->SignalSet );
    }
    
    //if ( *( _Var->LoopStatus ) == LoopOk )
    //    *( _Var->LoopStatus ) = LoopExit;
    return *( _Var->LoopStatus ) == LoopOk;
}

// создание отчётов
static
int __foreach_report ( inquire_var_t *_Var, const report_cfg_t **ReportList, const sigset_t *SignalSet )
{
    for ( size_t i = 0; ReportList[ i ] != NULL; i++ )
    {
        if ( ReportList[ i ]->Parametr != NULL )
        {
            script_option_set ( _Var->ScriptOption, SA_PARAMETR, ReportList[ i ]->Parametr );
        }
        *( _Var->LoopStatus ) = __run_script ( _Var->Log, ReportList[ i ]->Name, _Var->ScriptOption, SignalSet );
    }
    
    return *( _Var->LoopStatus ) == LoopOk;
}

// цикл опроса устройств
static
int __run_device_loop ( loop_state_t *LoopStatus, FILE *Log, const askue_cfg_t *ACfg, const sigset_t *SignalSet )
{
    //loop_state_t LoopStatus = LoopOk;
    
    script_option_t ScriptOption;
    __script_option_preset ( &ScriptOption, ACfg, SA_PRESET_DEVICE );
    // входы монитора
    inquire_input_t Input;
    Input.LocalGate = ( const gate_cfg_t* ) ACfg->LocalGate;
    //write_log ( Log, "Input.LocalGate.Device.Name", "?", Input.LocalGate->Device->Name );
    Input.RemoteGateList = ( const gate_cfg_t** ) ACfg->RemoteGateList;
    Input.DeviceList = ( const device_cfg_t** ) ACfg->DeviceList;
    Input.SignalSet = SignalSet;
    // переменные монитора
    inquire_var_t MVar;
    MVar.Log = Log;
    MVar.LoopStatus = LoopStatus;
    MVar.ScriptOption = &ScriptOption;
    
    return __foreach_device ( &MVar, &Input );
}

// цикл создания отчётов
static
int __run_report_loop ( loop_state_t *LoopStatus, FILE *Log, const askue_cfg_t *ACfg, const sigset_t *SignalSet )
{
    script_option_t ScriptOption;
    __script_option_preset ( &ScriptOption, ACfg, SA_PRESET_REPORT );
    // переменные монитора
    inquire_var_t MVar;
    MVar.Log = Log;
    MVar.LoopStatus = LoopStatus;
    MVar.ScriptOption = &ScriptOption;
    
    return __foreach_report ( &MVar, ( const report_cfg_t ** ) ACfg->ReportList, SignalSet );
}


// цикл сбора показаний
loop_state_t run_monitor_loop ( FILE *Log, const askue_cfg_t *ACfg, const sigset_t *SignalSet )
{
    loop_state_t LoopStatus = LoopOk;
    /*
    script_option_t ScriptOption;
    __script_option_preset ( &ScriptOption, ACfg );
    // входы монитора
    inquire_input_t Input;
    Input.LocalGate = ( const gate_cfg_t* ) ACfg->LocalGate;
    //write_log ( Log, "Input.LocalGate.Device.Name", "?", Input.LocalGate->Device->Name );
    Input.RemoteGateList = ( const gate_cfg_t** ) ACfg->RemoteGateList;
    Input.DeviceList = ( const device_cfg_t** ) ACfg->DeviceList;
    Input.SignalSet = SignalSet;
    // переменные монитора
    inquire_var_t MVar;
    MVar.Log = Log;
    MVar.LoopStatus = &LoopStatus;
    MVar.ScriptOption = &ScriptOption;
    
    while ( __foreach_device ( &MVar, &Input ) )
    {
        __script_option_preset ( &ScriptOption, ACfg, SA_REPORT_PRESET );
        __foreach_report ( &MVar, ACfg->ReportList, SignalSet ) );
        __script_option_preset ( &ScriptOption, ACfg, SA_SCRIPT_PRESET );
    }
    
    __log_loop_state ( Log, LoopStatus );
    */
    while ( __run_device_loop ( &LoopStatus, Log, ACfg, SignalSet ) &&
             __run_report_loop ( &LoopStatus, Log, ACfg, SignalSet ) );
    
    __log_loop_state ( Log, LoopStatus );
    
    return LoopStatus;
}




