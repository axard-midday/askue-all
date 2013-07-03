#include <libconfig.h>
#include <sqlite3.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <sys/wait.h>

/**********************************************
                    Макросы
**********************************************/
#define Ok( __MYFUNC__ ) ((__MYFUNC__) == EE_SUCCESS )

#define CfgBufferSize 2048

#define ModuleNameSize 64

#define ActionleNameSize 64

#define DeviceIdSize 32
#define DeviceTypeSize 32

#define GateIdSize 32
#define GateTypeSize 32

/* удалить */
#define EE_SUCCESS EXIT_SUCCESS
#define EE_UNKNOWN EXIT_FAILURE
#define EE_MALLOC 2
#define EE_REALLOC 3
#define EE_LOG_FOPEN 4
#define EE_PRINTF 5 // ошибка какой-то из функций серии printf
#define EE_FORK 6
#define EE_SETSID 7

/**********************************************
                    Типы данных
**********************************************/

typedef struct
{
    bool_t Last;
    Name[ ActionleNameSize ];
    char **argv; // NULL - окончание
} askue_action_t;

typedef askue_action_t* askue_action_vector_t;

typedef struct
{
    bool_t Last;
    char Name[ ModuleNameSize ];
    void *Func;
    askue_action_vector_t Action; // NULL - окончание 
} askue_module_t;

typedef askue_module_t* askue_module_vector_t;

typedef struct
{
    bool_t Last;
    char Type[ DeviceTypeSize ]; // тип прибора
    char ID[ DeviceIdSize ]; // номер прибора
    long int Timeout;
    askue_module_t *Module; // указатель на конкретный модуль для работы с устройством
} askue_device_t;

typedef struct
{
    bool_t Last;
    char Type[ GateTypeSize ]; // тип прибора
    char ID[ GateIdSize ]; // номер прибора
    long int Timeout;
    askue_module_t *Module; // указатель на конкретный модуль для работы с устройством
} askue_gate_t;

typedef askue_device_t* askue_device_vector_t;
typedef askue_gate_t* askue_gate_vector_t;

struct _askue_net_t;

typedef struct _askue_net_t askue_net_t;
typedef struct _askue_net_t* askue_net_vector_t;

struct _askue_net_t
{
    bool_t Last;
    askue_net_vector_t SubNet;
    askue_gate_vector_t Gate; 
    askue_device_vector_t Device; 
};

// askue_cfg_t - конфигурация программы
typedef struct
{
    char Buffer[ CfgBufferSize ];
    int ErrorCode;
    FILE *LogFile;
    int RS232;
    sqlite3 *DB;
    askue_module_vector_t Module; // NULL-окончание
    askue_net_t Net; // NULL-окончание
} askue_cfg_t;

#define InitAskueCfg \
({\
    askue_cfg_t _0_;\
    do {\
        bzero ( _0_.Buffer, CfgBufferSize );\
        _0_.ErrorCode = EE_SUCCESS;\
        _0_.LogFile = NULL;\
        _0_.RS232 = -1;\
        _0_.DB = NULL;\
        _0_.Module = NULL;\
        _0_.Net.Gate = NULL;\
        _0_.Net.Device = NULL;\
        _0_.Net.SubNet = NULL;\
    } while ( 0 );\
    _0_;\
})
/**********************************************
             Глобальные переменные
**********************************************/

askue_cfg_t AskueCfg;

/**********************************************
                    Прототипы
**********************************************/

int start_NewProcess ( askue_cfg_t *AskueCfg );
int set_Independent ( askue_cfg_t *AskueCfg );

// удалить
int askue_read_arguments ( int _1, char **_2, askue_cfg_t *_3 ) { return 0; }
int askue_read_config ( askue_cfg_t *_3 ) { return 0; }
int askue_open_db ( askue_cfg_t *_3 ) { return 0; }
int askue_init_db ( askue_cfg_t *_3 ) { return 0; }
int askue_open_log ( askue_cfg_t *_3 ) { return 0; }
int askue_request_loop ( askue_cfg_t *_3 ) { return 0; }
void stdError ( const char *_3 ) { ; }

/**********************************************
            Точка входа в программу
**********************************************/
int main ( int argc, char **argv )
{
    AskueCfg = InitAskueCfg;
    
    if ( Ok(askue_read_arguments ( argc, argv, &AskueCfg )) &&
         Ok(askue_read_config ( &AskueCfg )) && 
         Ok(start_NewProcess ( &AskueCfg)) &&
         Ok(set_Independent ( &AskueCfg )) &&
         Ok(askue_open_db ( &AskueCfg )) &&
         Ok(askue_init_db ( &AskueCfg )) &&
         Ok(askue_open_log ( &AskueCfg )) )
    {
        while ( Ok(askue_request_loop ( &AskueCfg )) );
        
        stdError ( AskueCfg.Buffer );
        return AskueCfg.ErrorCode;
    }
    else
    {
        stdError ( AskueCfg.Buffer );
        return AskueCfg.ErrorCode;
    }
}



/**********************************************
                Логика функций
**********************************************/

// Отделиться в независимый от терминала процесс
int start_NewProcess ( askue_cfg_t *AskueCfg )
{
    pid_t pid = fork();
    if ( pid < 0 )
    {
        strcpy ( AskueCfg->Buffer, "start_SignalMonitor(): Ошибка fork()\n" );
        AskueCfg->ErrorCode = EE_FORK;
        return EE_FORK;
    }
    else if ( pid > 0 )
    {
        exit ( EE_SUCCESS );
    }
    else 
    {
        AskueCfg->ErrorCode = EE_SUCCESS;
    }
    
    return EE_SUCCESS;
}
     
// стать главным в своей группе
int set_Independent ( askue_cfg_t *AskueCfg )
{
    umask( 0 );
    pid_t sid = setsid();
    if ( sid < 0 )
    {
        strcpy ( AskueCfg->Buffer, "start_SignalMonitor(): Ошибка setsid()\n" );
        AskueCfg->ErrorCode = EE_SETSID;
        return EE_SETSID;
    }
    chdir( "/" );
    AskueCfg->ErrorCode = EE_SUCCESS;
    return EE_SUCCESS;
}












