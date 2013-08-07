#ifndef ASKUE_SCRIPT_STREAM_H_
#define ASKUE_SCRIPT_STREAM_H_

#include <stdint.h>
#include <stdio.h>

// поток ( в частности файловый или
// доступа к базе данных )
typedef struct
{
    void *Stream;
    FILE *Log;
    uint32_t Flag;
} script_stream_t;

// выделить память под поток
void script_stream_new ( script_stream_t **ScriptStream );

// освободить память выделенную под поток
void script_stream_delete ( script_stream_t *ScriptStream );

#endif /* ASKUE_STREAM_H_ */
