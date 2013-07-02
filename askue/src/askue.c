#define _GNU_SOURCE
#define __USE_BSD

#include "askue_launcher_macro.h"

#include <libconfig.h>
#include <libaskue.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h> 
#include <sys/wait.h>

#include "askue_launcher_opt.h"
#include "askue_launcher_global.h"
#include "askue_launcher_db.h"
#include "askue_launcher_net_cfg.h"

/**********************************************
                    Макросы
**********************************************/
#define Ok( __MYFUNC__ ) {((__MYFUNC__) == EE_SUCCESS )}

#define CfgBufferSize
/**********************************************
                    Типы данных
**********************************************/
// askue_cfg_t - конфигурация программы
typedef struct
{
    char Buffer[ CfgBufferSize ];
    int ErrorCode;
    FILE *LogFile;
    int RS232;
    sqlite3 *DB;
}
/**********************************************
             Глобальные переменные
**********************************************/

/**********************************************
                    Прототипы
**********************************************/

// запуск функции мониторинга сигналов из-вне
int start_SignalMonitor ( askue_cfg_t *AskueCfg )


/**********************************************
            Точка входа в программу
**********************************************/
int main ( int argc, char **argv )
{
    askue_cfg_t AskueCfg = INIT_AskueCfg;
    
    if ( Ok(askue_read_arguments ( argc, argv, &AskueCfg )) &&
         Ok(askue_read_config ( argc, argv, &AskueCfg )) )
    {
        if ( Ok(start_SignalMonitor ( &AskueCfg )) &&
             Ok(set_as_Parent ( &AskueCfg)) &&
             Ok(start_AskueServer ( &AskueCfg)) ) 
        {
            stdError ( AskueCfg.Buffer );
            return AskueCfg.ErrorCode;
        }
        
        fclose ( stdout );
        fclose ( stdin );
        fclose ( stderr );
        
        // логика работы
        // Возможно нормальное завершение по сигналу из-вне
        return monitoring_Signal ();
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
int start_SignalMonitor ( askue_cfg_t *AskueCfg )
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
        return EE_SUCCESS;
    }
}
     
int set_as_Parent ( askue_cfg_t *AskueCfg )
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

int monitoring_Signal ( void )
{ return EE_SUCCESS; }

int start_AskueServer ( askue_cfg_t *Cfg )
{
    if ( Ok(askue_open_db ( Cfg )) &&
         Ok(askue_open_log ( Cfg )) )
    {
        
    }   
}














