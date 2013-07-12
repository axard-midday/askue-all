#include <stdio.h>
#include <time.h>

#define TIME_STRBUF 19

void write_msg ( FILE *output, const char *Hdr, const char *St, const char *Msg )
{
    time_t t = time ( NULL );
	char asctime_str[ TIME_STRBUF + 1 ];
	int len = strftime ( asctime_str, TIME_STRBUF + 1, "%Y-%m-%d %H:%M:%S", localtime ( &t ) );

	if ( len != TIME_STRBUF )
	{
		return;
	}
	
    if ( Msg )
        fprintf ( output, "[ %s | %s | %s ]: %s\n", asctime_str, Hdr, St, Msg );
    else
        fprintf ( output, "[ %s | %s | %s ]\n", asctime_str, Hdr, St );
}

#undef TIME_STRBUF
