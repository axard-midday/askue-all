#define IsLast( __Unit__ ) ({ (__Unit__).Last; })

int askue_request_loop ( askue_cfg_t *Cfg )
{
	askue_net_t *Net = &(Cfg->Net);
    
    askue_module_callbacks_t Callbacks;
    // инициализация callback
    askue_module_enviroment_t Enviroment;
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





























    
