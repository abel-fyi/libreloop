// SPDX-License-Identifier: GPL-3.0-only
#ifndef DEVICE_H
#define DEVICE_H
#include <stddef.h>
#include "parameter.h"
/* Stable saved IDs. Built-in registration is independent of Project and UI. */
enum { INSTRUMENT_SAMPLER,INSTRUMENT_FM };
enum { EFFECT_EMPTY,EFFECT_CHORUS,EFFECT_EQ };
enum { DEVICE_SAMPLER=1,DEVICE_FM,DEVICE_CHORUS,DEVICE_EQ };
typedef struct { unsigned first,last; } DeviceParameterRange;
typedef struct {
    const char *name;
    size_t settings_size;
    void (*defaults)(void *settings);
    int (*valid)(const void *settings);
    const float *(*parameter)(const void *settings,unsigned id);
    DeviceParameterRange parameters[2];
} DeviceDescriptor;
const DeviceDescriptor *device_descriptor(unsigned kind);
const DeviceDescriptor *instrument_descriptor(unsigned type);
const DeviceDescriptor *effect_descriptor(unsigned type);
int device_has_parameter(const DeviceDescriptor *device,unsigned id);
const float *device_parameter(const DeviceDescriptor *device,const void *settings,unsigned id);
#endif
