#include <stdio.h>

#include "config.h"
#include "sarg.h"
#include "macro.h"

static
const char *SA_Template[] =
{
    "%s",
    "--port_file=%s", "--port_parity=%s", "--port_dbits=%s", "--port_sbits=%s", "--port_speed=%s",
    "--device=%s", "--parametr=%s", "--timeout=%ld", 
    "--journal_file=%s", "--journal_flashback=%ld",
    "--log_file=%s",
    "--protocol",
    "--verbose"
};

int script_argument_set ( script_argument_vector_t *sargv, int _sa, ... )
{
    va_list Smth;
    va_start ( Smth, _sa );
    
    if ( _sa >= SA_FIRST &&
         _sa < SA_LAST )
    {
        SETBIT ( sargv->Flag, _sa );
    }
    else
    {
        va_end ( Smth );
        return -1;
    }

    int Result;
    if ( _sa == SA_PROTOCOL ||
         _sa == SA_VERBOSE )
    {
        Result = ( snprintf ( sargv->Value[ _sa ], SA_LENGTH, "%s", SA_Template[ _sa ] ) > 0 ) ? 0 : -1;
    }
    else
    {
        Result = ( vsnprintf ( sargv->Value[ _sa ], SA_LENGTH, SA_Template[ _sa ], Smth ) > 0 ) ? 0 : -1;
    }
    va_end ( Smth );
    return Result;
}

void script_argument_unset ( script_argument_vector_t *sargv, int _sa )
{
    UNSETBIT ( sargv->Flag, _sa );
}


void script_argument_init ( script_argument_vector_t *sargv, const askue_cfg_t *ACfg, int _flag )
{
    switch ( _flag )
    {
        case SA_PRESET_DEVICE:
        
            sargv->Flag = 0;
        
            script_argument_set ( sargv, SA_PORT_DBITS, ACfg->Port->DBits );
            script_argument_set ( sargv, SA_PORT_SBITS, ACfg->Port->SBits );
            script_argument_set ( sargv, SA_PORT_FILE, ACfg->Port->File );
            script_argument_set ( sargv, SA_PORT_PARITY, ACfg->Port->Parity );
            script_argument_set ( sargv, SA_PORT_SPEED, ACfg->Port->Speed );
            script_argument_set ( sargv, SA_JOURNAL_FILE, ACfg->Journal->File );
            script_argument_set ( sargv, SA_JOURNAL_FLASHBACK, ACfg->Journal->Flashback );
            script_argument_set ( sargv, SA_LOG_FILE, ACfg->Log->File );
            
            if ( TESTBIT ( ACfg->Flag, ASKUE_FLAG_PROTOCOL ) )
                script_argument_set ( sargv, SA_PROTOCOL );
                
            if ( TESTBIT ( ACfg->Flag, ASKUE_FLAG_VERBOSE ) )
                script_argument_set ( sargv, SA_VERBOSE );
        
            break;
            
        case SA_PRESET_REPORT:
        
            sargv->Flag = 0;
            
            script_argument_set ( sargv, SA_JOURNAL_FILE, ACfg->Journal->File );
            script_argument_set ( sargv, SA_JOURNAL_FLASHBACK, ACfg->Journal->Flashback );
            script_argument_set ( sargv, SA_LOG_FILE, ACfg->Log->File );
            
            if ( TESTBIT ( ACfg->Flag, ASKUE_FLAG_VERBOSE ) )
                script_argument_set ( sargv, SA_VERBOSE );
        
            break;
            
        case SA_PRESET_CLEAR:
            
            sargv->Flag = 0;
            
            break;
    }
}
