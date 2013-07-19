#ifndef ASKUE_DEVICE_LOOP_H_
#define ASKUE_DEVICE_LOOP_H_

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

// основной цикл программы
loop_state_t device_loop ( FILE *Log, const askue_cfg_t *ACfg, const sigset_t *SignalSet );

#endif /* ASKUE_DEVICE_LOOP_H_ */
