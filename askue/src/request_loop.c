#define IsLast( __Unit__ ) ({ (__Unit__).Last; })

askue_module_enviroment_t getEnviroment ( askue_cfg_t *Cfg )
{
    askue_module_enviroment_t Env;
    Env.Buffer = Cfg->Buffer;
    Env.LogFile = Cfg->LogFile;
    Env.DB = Cfg->DB;
    Env.PortFd = Cfg->RS232;
    return Env;
}

void log_callback ( FILE *Log, char *Buffer, size_t BufferLen, const char *Format, ... )
{
    va_list vArgs;
    va_start ( vArgs, Format );
    vsnprintf ( Buffer, BufferLen, Format, vArgs );
    fputs ( Buffer, Log );
}

int io_callback ( int Fd, const uint8_array_t *Out, uint8_array_t **In, long int StartTimeout, long int StopTimeout )
{
    int result = rs232_write ( Fd, Out );
    
    if ( result != Out->len )
    {
        return EE_WRITE;
    }
    
    uint8_array_t *_In = rs232_read_v2 ( Fd, StartTimeout, StopTimeout );
    
    if ( _In )
        *In = _In;
    else
    {
        
    }
}

int askue_request_loop ( askue_cfg_t *Cfg )
{
	askue_net_t *Net = &(Cfg->Net);
    
    askue_module_callbacks_t Callbacks;
    Callbacks.LogCallback = ...;
    Callbacks.DBCallback = ...;
    Callbacks.JnlCallback = ...;
    Callbacks.IOCallback = ...;
    // инициализация callback
    askue_module_enviroment_t Enviroment;
    Enviroment = getEnviroment ( Cfg );
    // инициализация окружения 
    
    bool_t localNet = true;
    
    while ( Net )
    {
            if ( Net->Device != NULL )
            {
                for ( size_t i = 0; IsLast( Net->Device[ i ] ); i++ )
                {
                    device_request_session ( Net->Device[ i ], Callbacks, Enviroment );
                }
            }
            
            if ( Net->SubNet != NULL )
            {
                if ( Net->Gate 
            }
            
            if ( Net->Gate != NULL && Net->SubNet )
            {
                for ( size_t i = 0; IsLast( Net->Gate[ i ] ); i++ )
                {
                    gate_request_session ( Net->Gate[ i ], Net->Gate[ i ], Callbacks, Enviroment );
                }
            }
    }
    
	return ;
}

int device_request_session ( const askue_device_t *Device,
                             const askue_module_callbacks_t *Callbacks,
                             const askue_module_enviroment_t *Enviroment )
{
    askue_module_t *Module = Device->Module;
    int result;
    
    for ( size_t j = 0; Module->Action[ i ] != NULL && Ok(result); i++ )
    {
        askue_module_method_t Method;
        if ( askue_module_method_exist ( Module, Module->Action[ i ], &Method ) )
        {
            askue_module_argv_t Argv = get_Argv ( Device );
            
            result = askue_module_method_exec ( Method, Callbacks, Enviroment, &Argv );
        }
    }
    
    return result;
}


int gate_request_session ( const askue_gate_t *Gate,
                           const askue_module_callbacks_t *Callbacks,
                           const askue_module_enviroment_t *Enviroment )
{
    askue_module_t *Module = Device->Module;
    int result;
    
    for ( size_t j = 0; Module->Action[ i ] != NULL && Ok(result); i++ )
    {
        askue_module_method_t Method;
        if ( askue_module_method_exist ( Module, Module->Action[ i ], &Method ) )
        {
            askue_module_argv_t Argv = get_Argv ( Gate );
            
            result = askue_module_method_exec ( Method, Callbacks, Enviroment, &Argv );
        }
    }
    
    return result;
}





























    
