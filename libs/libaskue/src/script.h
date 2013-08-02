#ifndef ASKUE_SCRIPT_WORKSPACE_H_
#define ASKUE_SCRIPT_WORKSPACE_H_

// начало скрипта
#define BeginScript( _ENVIROMENT_, _ARGC_, _ARGV_, _GET_PARAM_ ) \
script_enviroment_t _ENVIROMENT_; \
void __script_SIGUSR2_handler ( int __S ) \
{ \
    script_env_destroy ( &_ENVIROMENT_ ); \
    exit ( EXIT_BYSIGNAL ); \
} \
int __script_body__ ( script_enviroment_t * ); \
int main ( int _ARGC_, char **_ARGV_ ) \
{ \
    signal ( SIGUSR2, __script_SIGUSR2_handler ); \
    if ( _ARGC_ != 
    if ( script_env_init ( &_ENVIROMENT_, _ARGC_, _ARGV_, _GET_PARAM_ ) ) exit ( EXIT_FAILURE );
    
// конец скрипта
#define EndScript( _ENVIROMENT_ ) \
    script_env_destroy ( _ENVIROMENT_ );\
    exit ( _$result_ ); \
    return 0; \
}

// исполнение тела скрипта
#define ScriptBody( _ENVIROMENT_ )\
    int _$result_ = __script_body__ ( &_ENVIROMENT_ );
    
// Начало тела скрипта
#define BeginScriptBody( _ENVIROMENT_ )\
int __script_body__ ( script_enviroment_t *_ENVIROMENT_ ) {
    
// конец тела скрипта
#define EndScriptBody( _INT_RESULT_ ) return _INT_RESULT_; }


    

#endif /* ASKUE_SCRIPT_WORKSPACE_H_ */
