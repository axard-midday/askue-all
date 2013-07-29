#ifndef ASKUE_WRITE_MSG_H_
#define ASKUE_WRITE_MSG_H_

#include "macro.h"
// запись строки сообщения в файл
void write_msg ( FILE *output, const char *Hdr, const char *St, const char *Msg );

#define verbose_msg( _FLAG_, _FILE_, _HDR_, _ST_, _MSG_ ) \
({  \
    if ( TESTBIT ( _FLAG_, ASKUE_FLAG_VERBOSE ) ) \
        write_msg ( _FILE_, _HDR_, _ST_, _MSG_ ); \
})

#endif /* ASKUE_WRITE_MSG_H_ */
