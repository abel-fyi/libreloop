// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_REVERB_H
#define LIBRELOOP_REVERB_H
#include <stddef.h>
/* Eight-line stereo FDN at 48 kHz. Delay storage is prepared off audio. */
typedef struct {float size,decay,damping,predelay,width,mod,gate;} ReverbSettings;
typedef struct Reverb Reverb;
#define REVERB_FACTORY_COUNT 4
ReverbSettings reverb_default(void);
ReverbSettings reverb_factory(unsigned index);
const char *reverb_factory_name(unsigned index);
float reverb_factory_mix(unsigned index);
int reverb_valid(ReverbSettings settings);
Reverb *reverb_create(void);
void reverb_free(Reverb *reverb);
void reverb_reset(Reverb *reverb);
size_t reverb_storage_bytes(void);
int reverb_audible(const Reverb *reverb);
void reverb_process(Reverb *reverb,ReverbSettings settings,float wet,float stereo[2]);
#endif
