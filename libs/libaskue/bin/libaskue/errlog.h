#ifndef ASKUE_ERRORLOG_H_
#define ASKUE_ERRORLOG_H_

#include <stdarg.h>

typedef void ( *askue_errlog_method_f ) ( int exit_status,
                                            const char *fmt,
                                            ... );
                                  
void errlog_method ( int exit_status, const char *fmt, ... )
    __attribute__ ((__format__ (__printf__, 2, 3)));

#endif /* ASKUE_ERRORLOG_H_ */
