#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void errlog_method ( int exit_status, const char *fmt, ... )
{
    va_list vaList;
    va_start ( vaList, fmt );
    
    FILE *logFile = fopen ( LOG_FILE, "a" );
    if ( logFile == NULL )
    {
        exit ( EE_LOG_FOPEN );
    }
    
    if ( vfprintf ( logFile, fmt, vaList ) <= 0 )
    {
        fclose ( logFile );
        exit ( EE_PRINTF );
    }
    
    fclose ( logFile );
    
    if ( exit_status != 0 )
    {
        exit ( exit_status );
    }
}
