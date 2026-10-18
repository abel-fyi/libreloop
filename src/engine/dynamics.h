// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_DYNAMICS_H
#define LIBRELOOP_DYNAMICS_H
#include <stddef.h>
#define DYNAMICS_POINTS 8
#define DYNAMICS_HISTORY 256
#define DYNAMICS_PARAMETERS 52
/* All saved members are floats; field order is part of the project format. */
typedef struct {float x,y,bend,shape;} DynamicsPoint;
typedef struct {
    float pre,post,attack,release,release2,sustain,width,mode,solo,rms,ceiling,saturation,count;
    DynamicsPoint points[DYNAMICS_POINTS];
} DynamicsBand;
typedef struct {float low,high,lowcut,blend;DynamicsBand band[4];} DynamicsSettings;
typedef struct Dynamics Dynamics;
typedef struct {unsigned serial,count;float input[4][DYNAMICS_HISTORY],output[4][DYNAMICS_HISTORY],gain[4][DYNAMICS_HISTORY];} DynamicsMonitor;
DynamicsSettings dynamics_default(void);
DynamicsSettings dynamics_factory(unsigned index);
const char *dynamics_factory_name(unsigned index);
int dynamics_valid(DynamicsSettings settings);
float dynamics_curve(const DynamicsBand *band,float input_db);
const float *dynamics_parameter(const DynamicsSettings *settings,unsigned offset);
Dynamics *dynamics_create(void);
void dynamics_free(Dynamics *d);
void dynamics_reset(Dynamics *d);
void dynamics_process(Dynamics *d,const DynamicsSettings *settings,float wet,float stereo[2]);
void dynamics_monitor(const Dynamics *d,DynamicsMonitor *monitor);
size_t dynamics_storage_bytes(void);
#endif
