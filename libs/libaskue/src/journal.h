#ifndef LIBASKUE_JOURNAL_H
#define LIBASKUE_JOURNAL_H

#include <sqlite3.h>
#include <stdlib.h>
#include <stdint.h>

#include "macro.h"

#define JNL_TYPE_FIELD 32

#ifndef _ASKUE_TBUFLEN
    #define _ASKUE_TBUFLEN 512
#endif

typedef struct askue_jnl_rec_s
{
    double      Value;      // данные ( показания )
    uint64_t    Device;     // часть серийника устройства используемая в работе
    char        Type[ JNL_TYPE_FIELD + 1 ];      // тип данных
    char        Date[ DATE_STRBUF + 1 ];      // дата соответствующая фиксации показаний
    char        Time[ TIME_STRBUF + 1 ];      // время соответствующее фиксации показаний
} askue_jnl_rec_t;

typedef int jnl_callback_f ( void *, int, char **, char ** );

typedef struct askue_jnl_s
{
    sqlite3 *File;
    size_t Flashback;
    char SQL[ _ASKUE_TBUFLEN ];
    char Error[ _ASKUE_TBUFLEN ];
    
    int ( *Open ) ( struct askue_jnl_s *, const char * );
    int ( *Close ) ( struct askue_jnl_s * );
    int ( *ExecSQL ) ( struct askue_jnl_s *, jnl_callback_f *, void * );
    void ( *Refresh ) ( struct askue_jnl_s * );
} askue_jnl_t;

askue_jnl_t askue_jnl_init ( void );

#endif /* LIBASKUE_JOURNAL_H */
