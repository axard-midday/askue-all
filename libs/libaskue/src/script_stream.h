#ifndef ASKUE_SCRIPT_STREAM_H_
#define ASKUE_SCRIPT_STREAM_H_

#include <stdint.h>
#include <stdio.h>

typedef struct
{
    void *Stream;
    FILE *Log;
    uint32_t Flag;
} script_stream_t;

#endif /* ASKUE_STREAM_H_ */
