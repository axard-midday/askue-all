
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


// Выполнение скрипта
static
void __exec_script ( askue_workspace_t *WS, const script_cfg_t *Script )
{
    char *Argv[ SA_AMOUNT ];
    
    // обнуление
    for ( int i = 0; i < SA_AMOUNT; i++ ) Argv[ i ] = NULL;
    
    // установка аргументов
    for ( int i = SA_FIRST, j = 0; i < SA_LAST && j < SA_LAST; i++ )
    {
        if ( WS->ScriptArgV->Flag && i )
        {
            Argv[ j ] = WS->ScriptArgV->Value[ i ];
        }
    }
    
    // не факт что на лог стоит COE
    fclose ( WS->Log );
    
    // выполнить скрипт
    if ( execvp ( WS->ScriptArgV[ 0 ], ( char * const * ) Argv ) )
    {
        exit ( EXIT_FAILURE );
    }
}

// проверить результат выполнения скрипта
static
int __wait_script_result ( askue_workspace_t *WS, const char *ScriptName, pid_t pid )
{
    int status;
    pid_t WaitpidReturn = waitpid ( pid, &status, WNOHANG );
    if ( WaitpidReturn == -1 )
    {
        text_buffer_write ( WS->Buffer, "waitpid(): %s (%d)", strerror ( errno ), errno );
        write_msg ( WS->Log, "Демон", "FAIL", WS->Buffer );
        WS->Loop = LoopError;
        return -1;
    }
    else if ( WaitpidReturn == 0 )
    {
        write_msg ( WS->Log, "Демон", "FAIL", "Ложный сигнал SIGCHLD" );
        WS->Loop = LoopError;
        return -1;
    }
    
    if ( WIFEXITED ( status ) ) // успешное завершение, т.е. через exit или return
    {
        int code = WEXITSTATUS ( status );
        if ( code != EXIT_SUCCESS )
        {
            text_buffer_write ( WS->Buffer, "Скрипт '%s' завершён с кодом: %d", ScriptName, code );
            write_msg ( WS->Log, "Демон", "FAIL", WS->Buffer );
            WS->Loop = LoopError;
            return -1;
        }
    }
    else if ( WIFSIGNALED ( status ) ) // завершение по внешнему сигналу
    {
        int sig = WTERMSIG ( status );
        text_buffer_write ( WS->Buffer, "Скрипт '%s' завершён по сигналу: %d", ScriptName, sig );
        write_msg ( WS->Log, "Демон", "FAIL", WS->Buffer );
        WS->Loop = LoopError;
        return -1;
    }
    
    WS->Loop = LoopOk;
    return 0;
}

// обработать сигнал
int __wait_signal ( askue_workspace_t *WS, const script_cfg_t *Script, uint32_t Flag, pid_t pid )
{
    siginfo_t SignalInfo;
    
    if ( sigwaitinfo ( WS->SignalSet, &SignalInfo ) == -1 )
    {
        // сообщение об ошибке
        text_buffer_write ( WS->Buffer, "sigwaitinfo(): %s (%d)", strerror ( errno ), errno );
        write_msg ( WS->Log, "Сигнал", "ERROR", WS->Buffer );
        WS->Loop = LoopError;
        return -1;
    }
    else
    {
        int Result;
        switch ( SignalInfo.si_signo )
        {
            case SIGCHLD:
            
                Result = __wait_script_result ( WS, Script->Name, pid );
                break;
                
            case SIGUSR1:
            
                if ( kill ( pid, SIGUSR2 ) )
                    write_msg ( WS->Log, "Сигнал", "ERROR", "kill()" );
                if ( TESTBIT ( Flag, ASKUE_FLAG_VERBOSE ) )
                    write_msg ( WS->Log, "Сигнал", "OK", "Выполнить переконфигурацию" );
                WS->Loop = LoopReconfig;
                Result = -1;
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

// сообщить о pid процесса-потомка
static
void __verbose_say_about_child ( askue_workspace_t *WS, uint32_t Flag, pid_t pid )
{
    if ( TESTBIT ( Flag, ASKUE_FLAG_VERBOSE ) )
    {
        text_buffer_write ( WS->Buffer, "Pid потомка: %ld", pid );
        write_msg ( WS->Log, "Демон", "OK", WS->Buffer );
    }
}

// сообщение об ошибке при запуске скрипта
static
void __run_script_error ( askue_workspace_t *WS );
{ 
    text_buffer_write ( WS->Buffer, "fork(): %s (%d)", strerror ( errno ), errno );
    write_msg ( WS->Log, "Демон", "FAIL", WS->Buffer );
}

// запуск скрипты
static
int __run_script ( askue_workspace_t *WS, const script_cfg_t *Script, uint32_t Flag )
{
    int Result;
    pid_t ScriptPid = fork ();
    if ( ScriptPid < 0 )
    {
        __run_script_error ( WS );
        Result = -1;
    }
    else if ( ScriptPid == 0 )
    {
        __exec_script ( WS, Script );
    }
    else
    {
        __verbose_say_about_child ( WS, Flag, ScriptPid ); 
        Result = __wait_signal ( WS, Script, Flag ScriptPid );
    }
    
    return Result;
}

// проверка окончания списка скриптов
static
int __is_last_script ( const askue_workspace_t *WS, const script_cfg_t **SList, size_t si )
{
    return ( SList[ si ] != NULL ) &&
            ( WS->Loop == LoopOk );
}

// перебор скриптов
static
int __foreach_script ( askue_workspace_t *WS, const script_cfg_t **SList, uint32_t Flag )
{
    int Result = 0;
    for ( size_t i = 0; __is_last_script ( WS, SList, i ) && Result == 0; i++ )
    {
        script_argument_set ( WS->ScriptArgV, SA_NAME, SList[ i ]->Name );
        
        if ( SList[ i ]->Parametr != NULL )
            script_argument_set ( WS->ScriptArgV, SA_PARAMETR, SList[ i ]->Parametr );
        else
            script_argument_unset ( WS->ScriptArgV, SA_PARAMETR );
            
        Result =  __run_script ( WS, SList[ i ], Flag );
    }
}


// проверка окончания списка устройств
static
int __is_last_device ( const askue_workspace_t *WS, const askue_cfg_t *Cfg, size_t i )
{
    return ( Cfg->DeviceList[ i ] != NULL ) &&
            ( WS->Loop == LoopOk );
}

// напечатать об опрашиваемом устройстве
static
void __verbose_say_about_device ( const askue_workspace_t *WS, const askue_cfg_t *Cfg, const char *device )
{
    if ( TESTBIT ( Cfg->Flag, ASKUE_FLAG_VERBOSE ) )
    {
        text_buffer_write ( WS->Buffer, "Опрос устройства: '%s'", device );
        write_msg ( WS->Log, "Демон", "OK", WS->Buffer );
    }
}

// перебор всех устройств
static
int __foreach_device ( askue_workspace_t *WS, const askue_cfg_t *Cfg )
{
    int Result = 0;
    const gate_cfg_t *LastConnectedGate = NULL;
    
    for ( size_t i = 0; __is_last_device ( WS, Cfg, i) && Result == 0; i++ )
    {        
        
        if ( Cfg->DeviceList[ i ]->Segment == Askue_Remote )
        {
            const gate_cfg_t *RemoteGate = __find_remote_gate ( Cfg->RemoteGateList,
                                                                 Cfg->DeviceList[ i ]->Id );
                                                                  
            if ( LastConnectedGate != RemoteGate )
            {
                LastConnectedGate = RemoteGate;
                
                if ( RemoteGate != NULL )
                {
                    __verbose_say_about_device ( WS, Cfg, RemoteGate->Device->Name );
                    
                    script_argument_init ( WS->ScriptArgV, Cfg, SA_PRESET_DEVICE );
                    script_argument_set ( WS->ScriptArgV, SA_DEVICE, RemoteGateList->Device->Name );
                    script_argument_set ( WS->ScriptArgV, SA_TIMEOUT, RemoteGateList->Device->Timeout );
                    
                    script_cfg_t **ScriptList = RemoteGate->Device->Type->Script;
                    
                    Result = __foreach_script ( WS, ScriptList, Cfg->Flag );
                }
            }
        }
        
        __verbose_say_about_device ( WS, Cfg, Cfg->DeviceList[ i ]->Name );
        Result = __foreach_script ( WS, Cfg, i );
    }
        
    return *( _Var->LoopStatus ) == LoopOk;
}

// цикл опроса устройств
static
int __run_device_loop ( askue_workspace_t *WS, const askue_cfg_t *Cfg )
{
    verbose_msg ( Cfg->Flag, Log, "Демон", "OK", "Старт опроса устройств." );
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
    script_argument_init ( WS->ScriptArgV, Cfg, SA_PRESET_REPORT );
    for ( size_t i = 0; __is_last_report ( WS, Cfg, i ) && Result == 0; i++ )
    {
        // установить параметр
        if ( Cfg->ReportList[ i ]->Parametr != NULL )
        {
            script_argument_set ( WS->ScriptArgV, SA_PARAMETR, Cfg->ReportList[ i ]->Parametr );
        }
        
        Result = __run_script ( WS, (script_cfg_t*) ( Cfg->ReportList[ i ] ), Cfg->Flag );
    }

    return Result;
}

// цикл создания отчётов
static
int __run_report_loop ( askue_workspace_t *WS, const askue_cfg_t *Cfg )
{
    verbose_msg ( ACfg->Flag, Log, "Демон", "OK", "Старт создания отчётов." );
    
    return __foreach_report ( WS, Cfg );
}


// условия работы цикла мониторинга
static
int __condition_monitor_loop ( askue_workspace_t WS, const askue_cfg_t *Cfg )
{
    return ( __run_device_loop ( WS, Cfg ) == 0 ) && 
            ( __run_report_loop ( WS, Cfg ) == 0 );
}

// условие разрыва цикла мониторинга
static
int __condition_break_monitor_loop ( askue_workspace_t WS, const askue_cfg_t *Cfg )
{
    if ( TESTBIT ( Cfg->Flag, ASKUE_FLAG_CYCLE ) )
    {
        *( WS->Loop ) = LoopBreak;
        return 0;
    }
    else
        return 1;
}

// вывод в лог сообщения об ошибке
static
void __say_break_reson ( const askue_workspace_t *WorkSpace )
{
    if ( WorkSpace->Loop == LoopError )
    {
        write_msg ( WorkSpace->Log, "Демон", "FAIL", "Цикл опроса прерван в связи с ошибкой." );
    }    
    else if ( WorkSpace->Loop == LoopExit ) 
    {
        write_msg ( WorkSpace->Log, "Демон", "OK", "Цикл опроса прерван в связи с сигналом завершения." );
    }
    else if ( WorkSpace->Loop == LoopReconfig )
    {
        write_msg ( WorkSpace->Log, "Демон", "OK", "Цикл опроса прерван в связи с сигналом переконфигурации." );
    }
    else if ( WorkSpace->Loop == LoopBreak )
    {
        write_msg ( WorkSpace->Log, "Демон", "OK", "Цикл опроса прерван в связи с завершением цикла опроса." );
    }
}

// цикл сбора показаний
// 0 - нормально завершение ( переконфигурация, сигнал )
// -1 - ошибка
int run_monitor_loop ( askue_workspace_t WS, const askue_cfg_t *Cfg )
{
    int Result = 0;
    
    while ( __condition_monitor_loop ( WS, ACfg ) &&
             __condition_break_monitor_loop ( WS, ACfg ) );
    
    __say_break_reason ( WS );
    
    return LoopStatus;
}
