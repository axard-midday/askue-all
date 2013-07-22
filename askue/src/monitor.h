#ifndef ASKUE_MONITOR_H_
#define ASKUE_MONITOR_H_

#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

#include "config.h"

typedef enum _loop_state_t
{
    LoopOk,
    LoopError,
    LoopReconfig,
    LoopExit,
    LoopContinue
} loop_state_t;

// цикл сбора показаний
loop_state_t run_monitor_loop ( FILE *Log, const askue_cfg_t *ACfg, const sigset_t *SignalSet );

#endif /* ASKUE_MONITOR_H_ */
