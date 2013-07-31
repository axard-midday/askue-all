#ifndef ASKUE_WORKSPACE_H_
#define ASKUE_WORKSPACE_H_

#include <stdio.h>
#include <signal.h>

#include "sarg.h"
#include "text_buffer.h"

typedef struct
{
    FILE *Log;
    sigset_t *SignalSet;
    text_buffer_t *Buffer;
    script_argument_vector_t *ScriptArgV;
    int Loop;
} askue_workspace_t;

#define LoopOk 0
#define LoopError 1
#define LoopReconfig 2
#define LoopBreak 3
#define LoopExit 4

#endif /* ASKUE_WORKSPACE_H_ */
