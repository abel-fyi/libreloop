// SPDX-License-Identifier: GPL-3.0-only
#ifndef PRESET_H
#define PRESET_H
#include "device.h"
#include "sampler.h"
#include "fm_synth.h"
#include "chorus.h"
#include "equalizer.h"
#include "delay.h"
#include "tube.h"
#include "reverb.h"
#include "dynamics.h"
enum { PRESET_SAMPLER=DEVICE_SAMPLER, PRESET_FM=DEVICE_FM, PRESET_CHORUS=DEVICE_CHORUS, PRESET_EQ=DEVICE_EQ, PRESET_DELAY=DEVICE_DELAY, PRESET_TUBE=DEVICE_TUBE, PRESET_REVERB=DEVICE_REVERB, PRESET_DYNAMICS=DEVICE_DYNAMICS };
typedef struct {
    int kind;
    Sampler sampler;
    FMSettings fm;
    ChorusSettings chorus;
    EQSettings eq;
    DelaySettings delay;
    TubeSettings tube;
    ReverbSettings reverb;
    DynamicsSettings dynamics;
    float mix;
    char sample_path[1024]; /* Original audio reference, not embedded PCM. */
} DevicePreset;
int preset_save(const char *path,const DevicePreset *preset);
/* Validates before publishing; resolves sample references relative to the preset. */
int preset_load(const char *path,DevicePreset *preset);
#endif
