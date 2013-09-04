#ifndef ASKUE_VDEVICE_H_
#define ASKUE_VDEVICE_H_

#include <stdio.h>
#include <stdint.h>

struct vdevice_s;

typedef struct vdevice_s vdevice_t;

#define _ASKUE_VDEVICE  \
    askue_port_t *Port; \
    FILE *Log;          \
    long int Timeout;   \
    uint64_t Name;      \
    int Verbose;        \
    int Debug;          \
    int Protocol;

typedef int vdevice_method_f ( vdevice_t *VDev, void *Data );

#endif /* ASKUE_VDEVICE_H_ */

