#define SCRIPT_NAME "4ta_profile"

#include <stdio.h>
#include <string.h>
#include <libaskue.h>
#include <sqlite3.h>
#include <libconfig.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <byteswap.h>
#include <time.h>

#include "cli.h"

#define SQL_FindLostRecord \
"SELECT time_tbl.time, date_tbl.date FROM time_tbl, date_tbl \
        WHERE\
                ( ( NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl \
                        WHERE time_tbl.time = reg_tbl.time\
                                AND reg_tbl.date = date_tbl.date\
                                AND reg_tbl.cnt = %d\
                                AND reg_tbl.type = 'p+' )\
                  OR NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl \
                        WHERE time_tbl.time = reg_tbl.time \
                                AND reg_tbl.date = date_tbl.date\
                                AND reg_tbl.cnt = %d\
                                AND reg_tbl.type = 'p-' )\
		  OR NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl \
                        WHERE time_tbl.time = reg_tbl.time \
                                AND reg_tbl.date = date_tbl.date\
                                AND reg_tbl.cnt = %d\
                                AND reg_tbl.type = 'q+' )\
                  OR NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl \
                        WHERE time_tbl.time = reg_tbl.time \
                                AND reg_tbl.date = date_tbl.date\
                                AND reg_tbl.cnt = %d\
                                AND reg_tbl.type = 'q-' ) )\
	          AND time_tbl.time <= '23:30:00' \
	          AND date_tbl.date < ( SELECT DATE ( 'now' ) )\
	          AND date_tbl.date > ( SELECT DATE ( 'now', '-31 day' ) ) )\
	       OR\
	       ( ( NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl \
                        WHERE time_tbl.time = reg_tbl.time \
                                AND reg_tbl.date = date_tbl.date\
                                AND reg_tbl.cnt = %d\
                                AND reg_tbl.type = 'p+' )\
                  OR NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl \
                        WHERE time_tbl.time = reg_tbl.time \
                                AND reg_tbl.date = date_tbl.date\
                                AND reg_tbl.cnt = %d\
                                AND reg_tbl.type = 'p-' )\
		  OR NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl \
                        WHERE time_tbl.time = reg_tbl.time\
                                AND reg_tbl.date = date_tbl.date\
                                AND reg_tbl.cnt = %d\
                                AND reg_tbl.type = 'q+' )\
                  OR NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl \
                        WHERE time_tbl.time = reg_tbl.time\
                                AND reg_tbl.date = date_tbl.date\
                                AND reg_tbl.cnt = %d\
                                AND reg_tbl.type = 'q-' ) )\
	         AND time_tbl.time < ( SELECT TIME ( 'now', '-1 hour' ) ) \
	         AND date_tbl.date = ( SELECT DATE ( 'now' ) ) )\
	ORDER BY date_tbl.date DESC, time_tbl.time DESC;"
    
typedef struct
{
	sqlite3 *DB;		    // --db={ПУТЬ}
	int RS232_FD;		    // --port="{ПУТЬ} {ФОРМУЛА КОНФИГА}"
	FILE *Log;		        // --log={ПУТЬ}
	uint8_t Device;	    // --device={НОМЕР УСТРОЙСТВА}
	long int Timeout;	    // --timeout={ТАЙМАУТ СОЕДИНЕНИЯ}
	uint8_t DebugFlag;		// --debug 
    power_profile_t *PowerProfile;
} script_cfg_t;
    

// запись в лог
static
void __log_write ( FILE *F, const char *format, ... )
{
   	time_t t = time ( NULL );
	char asctime_str[ DATE_TIME_STRING_BUF + 1 ];
	int len = strftime ( asctime_str, DATE_TIME_STRING_BUF + 1, "%Y-%m-%d %H:%M:%S", localtime ( &t ) );

	if ( len != DATE_TIME_STRING_BUF )
	{
		return;
	}
	
	fprintf ( F, "%s ", asctime_str );

    va_list vl;
    va_start ( vl, format );
    vfprintf ( F, format, vl );
    va_end ( vl );
}
    
    
    
