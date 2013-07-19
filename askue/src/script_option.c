#include <stdio.h>
#include <string.h>
#include "script_option.h"
#include "macro.h"

// инициализировать опции
void script_option_init ( script_option_t *SOpt )
{
    memset ( SOpt->Value[ SA_PORT_FILE ], '\0', SA_LENGTH );
    memset ( SOpt->Value[ SA_PORT_PARITY ], '\0', SA_LENGTH );
    memset ( SOpt->Value[ SA_PORT_DBITS ], '\0', SA_LENGTH );
    memset ( SOpt->Value[ SA_PORT_SBITS ], '\0', SA_LENGTH );
    memset ( SOpt->Value[ SA_PORT_SPEED ], '\0', SA_LENGTH );
    memset ( SOpt->Value[ SA_DEVICE ], '\0', SA_LENGTH );
    memset ( SOpt->Value[ SA_PARAMETR ], '\0', SA_LENGTH );
    memset ( SOpt->Value[ SA_TIMEOUT ], '\0', SA_LENGTH );
    memset ( SOpt->Value[ SA_JOURNAL_FILE ], '\0', SA_LENGTH );
    memset ( SOpt->Value[ SA_JOURNAL_FLASHBACK ], '\0', SA_LENGTH );
    memset ( SOpt->Value[ SA_LOG_FILE ], '\0', SA_LENGTH );
    
    SOpt->Flag = 0;
}

// установить опцию скрипта
int script_option_set ( script_option_t *SOpt, int SOptNumber, ... )
{
    const char *SA_Template[] =
    {
        "--port_file=%s", "--port_parity=%s", "--port_dbits=%s", "--port_sbits=%s", "--port_speed=%s",
        "--device=%s", "--parametr=%s", "--timeout=%d", 
        "--journal_file=%s", "--journal_flashback=%d",
        "--log_file=%s",
    };
    
    va_list Smth;
    va_start ( Smth, SOptNumber );
    
    if ( SOptNumber >= SA_PORT_FILE &&
         SOptNumber <= SA_LOG_FILE )
    {
        SETBIT ( SOpt->Flag, SOptNumber );
    }
    else
    {
        va_end ( Smth );
        return -1;
    }
    
    int Result = ( vsnprintf ( SOpt->Value[ SOptNumber ], SA_LENGTH, SA_Template[ SOptNumber ], Smth ) > 0 ) ? 0 : -1;
    va_end ( Smth );
    return Result;
}

// снять опцию скрипта
void script_option_unset ( script_option_t *SOpt, int SOptNumber )
{
    UNSETBIT ( SOpt->Flag, SOptNumber ); 
}


