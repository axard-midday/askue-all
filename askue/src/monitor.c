#include <libaskue.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

#include "sarg.h"
#include "log.h"
#include "workspace.h"
#include "config.h"

// найти модем с таким же id
const gate_cfg_t* __find_remote_gate ( const gate_cfg_t ** GateList, long int Id )
{
    const gate_cfg_t *Gate = NULL;
    for ( size_t i = 0; GateList[ i ] != NULL && Gate == NULL; i++ )
    {
        if ( GateList[ i ]->Device->Id == Id )
            Gate = GateList[ i ];
    }
    return Gate;
}


// Выполнение скрипта
static
void __exec_script ( askue_workspace_t *WS )
{
    char *Argv[ SA_AMOUNT ];
    
    // обнуление
    for ( int i = 0; i < SA_AMOUNT; i++ ) Argv[ i ] = NULL;
    
    // установка аргументов
    for ( int i = SA_FIRST, j = 0; i < SA_LAST && j < SA_LAST; i++ )
    {
        if ( TESTBIT ( WS->ScriptArgV->Flag, i ) )
        {
            Argv[ j ] = WS->ScriptArgV->Value[ i ];
            j++;
        }
    }
    
    // не факт что на лог стоит COE
    //fclose ( WS->Log );
    Argv[ 0 ] = "/home/axard/workspace/Repos/askue-repo/askue/src/test_script";
    // выполнить скрипт
    if ( execvp ( Argv[ 0 ], ( char * const * ) Argv ) )
    {
        text_buffer_write ( WS->Buffer, "execvp(): %s (%d)", strerror ( errno ), errno );
        write_msg ( WS->Log, "Скрипт", "FAIL", WS->Buffer->Text );
        //kill ( 0, SIGCHLD );
        exit ( EXIT_FAILURE );
    }
}

// проверить результат выполнения скрипта
static
int __wait_script_result ( askue_workspace_t *WS, pid_t pid )
{
    int status;
    pid_t WaitpidReturn = waitpid ( pid, &status, WNOHANG );
    if ( WaitpidReturn == -1 )
    {
        text_buffer_write ( WS->Buffer, "waitpid(): %s (%d)", strerror ( errno ), errno );
        write_msg ( WS->Log, "АСКУЭ", "FAIL", WS->Buffer->Text );
        WS->Loop = LoopError;
        return -1;
    }
    else if ( WaitpidReturn == 0 )
    {
        write_msg ( WS->Log, "АСКУЭ", "FAIL", "Ложный сигнал SIGCHLD" );
        //if ( kill ( pid, SIGTERM ) ) write_msg ( WS->Log, "Сигнал", "ERROR", "kill()" );
        WS->Loop = LoopError;
        return 0;
    }
    
    if ( WIFEXITED ( status ) ) // успешное завершение, т.е. через exit или return
    {
        int code = WEXITSTATUS ( status );
        if ( code != EXIT_SUCCESS )
        {
            text_buffer_write ( WS->Buffer, "Скрипт '%s' завершён с кодом: %d", WS->ScriptArgV->Value[ 0 ], code );
            write_msg ( WS->Log, "АСКУЭ", "ERROR", WS->Buffer->Text );
        }
    }
    else if ( WIFSIGNALED ( status ) ) // завершение по внешнему сигналу
    {
        int sig = WTERMSIG ( status );
        text_buffer_write ( WS->Buffer, "Скрипт '%s' завершён по сигналу: %d", WS->ScriptArgV->Value[ 0 ], sig );
        write_msg ( WS->Log, "АСКУЭ", "ERROR", WS->Buffer->Text );
    }
    
    WS->Loop = LoopOk;
    return 0;
}

int __wait_signal ( askue_workspace_t *WS, uint32_t Flag, pid_t pid )
{
    siginfo_t SignalInfo;
    if ( sigwaitinfo ( WS->SignalSet, &SignalInfo ) == -1 )
    {
        // сообщение об ошибке
        text_buffer_write ( WS->Buffer, "sigwaitinfo(): %s (%d)", strerror ( errno ), errno );
        write_msg ( WS->Log, "Сигнал", "ERROR", WS->Buffer->Text );
        WS->Loop = LoopError;
        return -1;
    }
    else
    {
        int Result;
        switch ( SignalInfo.si_signo )
        {
            case SIGCHLD:
            
                Result = __wait_script_result ( WS, pid );
                break;
                
            case SIGUSR1:
            
                if ( kill ( pid, SIGUSR2 ) )
                    write_msg ( WS->Log, "Сигнал", "ERROR", "kill()" );
                if ( TESTBIT ( Flag, ASKUE_FLAG_VERBOSE ) )
                    write_msg ( WS->Log, "Сигнал", "OK", "Выполнить переконфигурацию" );
                WS->Loop = LoopReconfig;
                Result = __wait_script_result ( WS, pid );
                break;
                
            case SIGTERM:
            
                if ( kill ( pid, SIGUSR2 ) )
                    write_msg ( WS->Log, "Сигнал", "ERROR", "kill()" );
                if ( TESTBIT ( Flag, ASKUE_FLAG_VERBOSE ) )
                    write_msg ( WS->Log, "Сигнал", "OK", "Завершить работу АСКУЭ" );
                WS->Loop = LoopExit;
                Result = -1;
                break;
                
            default:
            
                if ( kill ( pid, SIGUSR2 ) )
                    write_msg ( WS->Log, "Сигнал", "ERROR", "kill()" );
                WS->Loop = LoopError;
                Result = -1;
                break;
        }
        return Result;
    }
}

// обработать сигнал
int __wait_signal_loop ( askue_workspace_t *WS, uint32_t Flag, pid_t pid )
{
    int Result;
    while ( ( ( Result = __wait_signal ( WS, Flag, pid ) ) == 0 ) &&
             ( WS->Loop == LoopError ) )
    {
        WS->Loop = LoopOk;
    }
    return Result;
}

// сообщить о pid процесса-потомка
static
void __verbose_say_about_child ( askue_workspace_t *WS, uint32_t Flag, pid_t pid )
{
    if ( TESTBIT ( Flag, ASKUE_FLAG_VERBOSE ) )
    {
        text_buffer_write ( WS->Buffer, "Pid потомка: %ld", pid );
        write_msg ( WS->Log, "АСКУЭ", "OK", WS->Buffer->Text );
    }
}

// сообщение об ошибке при запуске скрипта
static
void __run_script_error ( askue_workspace_t *WS )
{ 
    text_buffer_write ( WS->Buffer, "fork(): %s (%d)", strerror ( errno ), errno );
    write_msg ( WS->Log, "АСКУЭ", "FAIL", WS->Buffer->Text );
}

static 
void __verbose_say_about_script ( askue_workspace_t *WS, uint32_t Flag )
{
    if ( TESTBIT ( Flag, ASKUE_FLAG_VERBOSE ) )
    {
        text_buffer_write ( WS->Buffer, "Запуск скрипта '%s' с параметрами:", WS->ScriptArgV->Value[ 0 ] );
        
        for ( int i = SA_FIRST + 1; i < SA_LAST; i++ )
        {
            if ( TESTBIT ( WS->ScriptArgV->Flag, i ) )
            {
                text_buffer_append ( WS->Buffer, " %s", WS->ScriptArgV->Value[ i ] );
            }
        }
        
        write_msg ( WS->Log, "АСКУЭ", "OK", WS->Buffer->Text );
    }
}

// запуск скрипты
static
int __run_script ( askue_workspace_t *WS, uint32_t Flag )
{
    int Result;
    __verbose_say_about_script ( WS, Flag );
    
    pid_t ScriptPid = fork ();
    if ( ScriptPid < 0 )
    {
        __run_script_error ( WS );
        Result = -1;
    }
    else if ( ScriptPid == 0 )
    {
        __exec_script ( WS );
    }
    else
    {
        //__verbose_say_about_child ( WS, Flag, ScriptPid ); 
        Result = __wait_signal_loop ( WS, Flag, ScriptPid );
    }
    
    return Result;
}

// проверка окончания списка скриптов
static
int __is_last_script ( const askue_workspace_t *WS, const script_cfg_t **SList, size_t si )
{
    return ( SList[ si ] == NULL ) &&
            ( WS->Loop == LoopOk );
}

// перебор скриптов
static
int __foreach_script ( askue_workspace_t *WS, const script_cfg_t **SList, uint32_t Flag )
{
    int Result = 0;
    
    for ( size_t i = 0; !__is_last_script ( WS, SList, i ) && Result == 0; i++ )
    {
        // имя
        script_argument_set ( WS->ScriptArgV, SA_NAME, SList[ i ]->Name );
        
        // параметр
        if ( SList[ i ]->Parametr != NULL )
            script_argument_set ( WS->ScriptArgV, SA_PARAMETR, SList[ i ]->Parametr );
        else
            script_argument_unset ( WS->ScriptArgV, SA_PARAMETR );
            
        Result =  __run_script ( WS, Flag );
    }
    
    return Result;
}


// проверка окончания списка устройств
static
int __is_last_device ( const askue_workspace_t *WS, const askue_cfg_t *Cfg, size_t i )
{
    return ( Cfg->DeviceList[ i ] == NULL ) &&
            ( WS->Loop == LoopOk );
}

// напечатать об опрашиваемом устройстве
static
void __verbose_say_about_device ( const askue_workspace_t *WS, const askue_cfg_t *Cfg, const char *device )
{
    if ( TESTBIT ( Cfg->Flag, ASKUE_FLAG_VERBOSE ) )
    {
        text_buffer_write ( WS->Buffer, "Опрос устройства: '%s'", device );
        write_msg ( WS->Log, "АСКУЭ", "OK", WS->Buffer->Text );
    }
}

// перебор всех устройств
static
int __foreach_device ( askue_workspace_t *WS, const askue_cfg_t *Cfg )
{
    int Result = 0;
    const gate_cfg_t *LastConnectedGate = NULL;
    
    script_argument_init ( WS->ScriptArgV, Cfg, SA_PRESET_DEVICE );
    
    for ( size_t i = 0; !__is_last_device ( WS, Cfg, i) && Result == 0; i++ )
    {        
        script_cfg_t **ScriptList;
        if ( Cfg->DeviceList[ i ]->Segment == Askue_Remote )
        {
            gate_cfg_t **RemoteGateList = Cfg->RemoteGateList;
            const gate_cfg_t *RemoteGate = __find_remote_gate ( ( const gate_cfg_t ** )RemoteGateList,
                                                                 Cfg->DeviceList[ i ]->Id );
                                                                  
            if ( LastConnectedGate != RemoteGate )
            {
                LastConnectedGate = RemoteGate;
                
                if ( RemoteGate != NULL )
                {
                    __verbose_say_about_device ( WS, Cfg, RemoteGate->Device->Name );
                    
                    script_argument_set ( WS->ScriptArgV, SA_DEVICE, RemoteGate->Device->Name );
                    script_argument_set ( WS->ScriptArgV, SA_TIMEOUT, RemoteGate->Device->Timeout );
                    
                    ScriptList = RemoteGate->Device->Type->Script;
                    
                    Result = __foreach_script ( WS, ( const script_cfg_t ** ) ScriptList, Cfg->Flag );
                }
            }
        }
        ScriptList = Cfg->DeviceList[ i ]->Type->Script;
        __verbose_say_about_device ( WS, Cfg, Cfg->DeviceList[ i ]->Name );
        script_argument_set ( WS->ScriptArgV, SA_DEVICE, Cfg->DeviceList[ i ]->Name );
        script_argument_set ( WS->ScriptArgV, SA_TIMEOUT, Cfg->DeviceList[ i ]->Timeout );
        Result = __foreach_script ( WS, ( const script_cfg_t ** ) ScriptList, Cfg->Flag );
    }
        
    return Result;
}

// цикл опроса устройств
static
int __run_device_loop ( askue_workspace_t *WS, const askue_cfg_t *Cfg )
{
    verbose_msg ( Cfg->Flag, WS->Log, "АСКУЭ", "OK", "Старт опроса устройств." );
    return __foreach_device ( WS, Cfg );
}

static
int __is_last_report ( askue_workspace_t *WS, const askue_cfg_t *Cfg, size_t ri )
{
    return ( Cfg->ReportList[ ri ] != NULL ) &&
            ( WS->Loop == LoopOk );
}

// создание отчётов
static
int __foreach_report ( askue_workspace_t *WS, const askue_cfg_t *Cfg )
{
    int Result = 0;
    // начальные настройки ( будут для всех скриптов формирующих отчёты )
    script_argument_init ( WS->ScriptArgV, Cfg, SA_PRESET_REPORT );
    for ( size_t i = 0; __is_last_report ( WS, Cfg, i ) && Result == 0; i++ )
    {
        // установить параметр
        if ( Cfg->ReportList[ i ]->Parametr != NULL )
        {
            script_argument_set ( WS->ScriptArgV, SA_PARAMETR, Cfg->ReportList[ i ]->Parametr );
        }
        else
        {
            script_argument_unset ( WS->ScriptArgV, SA_PARAMETR ); 
        }
        // установить имя
        script_argument_set ( WS->ScriptArgV, SA_NAME, Cfg->ReportList[ i ]->Name );
        
        Result = __run_script ( WS, Cfg->Flag );
    }

    return Result;
}

// цикл создания отчётов
static
int __run_report_loop ( askue_workspace_t *WS, const askue_cfg_t *Cfg )
{
    verbose_msg ( Cfg->Flag, WS->Log, "АСКУЭ", "OK", "Старт создания отчётов." );
    
    return __foreach_report ( WS, Cfg );
}


// условия работы цикла мониторинга
static
int __condition_monitor_loop ( askue_workspace_t *WS, const askue_cfg_t *Cfg )
{
    return ( __run_device_loop ( WS, Cfg ) == 0 ) && 
            ( __run_report_loop ( WS, Cfg ) == 0 );
}

// условие разрыва цикла мониторинга
static
int __condition_break_monitor_loop ( askue_workspace_t *WS, const askue_cfg_t *Cfg )
{
    if ( TESTBIT ( Cfg->Flag, ASKUE_FLAG_CYCLE ) )
    {
        WS->Loop = LoopBreak;
        return 0;
    }
    else
        return 1;
}

// вывод в лог сообщения об ошибке
static
void __say_break_reason ( const askue_workspace_t *WorkSpace )
{
    if ( WorkSpace->Loop == LoopError )
    {
        write_msg ( WorkSpace->Log, "АСКУЭ", "FAIL", "Цикл опроса прерван в связи с ошибкой." );
    }    
    else if ( WorkSpace->Loop == LoopExit ) 
    {
        write_msg ( WorkSpace->Log, "АСКУЭ", "OK", "Цикл опроса прерван в связи с сигналом завершения." );
    }
    else if ( WorkSpace->Loop == LoopReconfig )
    {
        write_msg ( WorkSpace->Log, "АСКУЭ", "OK", "Цикл опроса прерван в связи с сигналом переконфигурации." );
    }
    else if ( WorkSpace->Loop == LoopBreak )
    {
        write_msg ( WorkSpace->Log, "АСКУЭ", "OK", "Цикл опроса прерван в связи с завершением итерации цикла." );
    }
}

// цикл сбора показаний
// 0 - нормально завершение ( переконфигурация, сигнал )
// -1 - ошибка
int run_monitor_loop ( askue_workspace_t *WS, const askue_cfg_t *Cfg )
{
    int Result = 0;
    
    while ( __condition_monitor_loop ( WS, Cfg ) &&
             __condition_break_monitor_loop ( WS, Cfg ) &&
             !askue_log_cut ( &(WS->Log), Cfg ) );
    
    __say_break_reason ( WS );
    
    return ( WS->Loop == LoopError ) ? -1 : 0;
}
