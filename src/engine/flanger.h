// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_FLANGER_H
#define LIBRELOOP_FLANGER_H
#include <stddef.h>
typedef struct {float rate,delay,depth,feedback,stereo,invert;} FlangerSettings;
typedef struct Flanger Flanger;
FlangerSettings flanger_default(void);
FlangerSettings flanger_factory(unsigned index);
const char *flanger_factory_name(unsigned index);
int flanger_valid(FlangerSettings settings);
const float *flanger_parameter(const FlangerSettings *settings,unsigned offset);
Flanger *flanger_create(void);
void flanger_free(Flanger *flanger);
void flanger_reset(Flanger *flanger);
int flanger_audible(const Flanger *flanger);
size_t flanger_storage_bytes(void);
void flanger_process(Flanger *flanger,FlangerSettings settings,float wet,float stereo[2]);
#endif
