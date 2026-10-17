// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_SAMPLER_SERVICE_H
#define LIBRELOOP_SAMPLER_SERVICE_H
#include "engine.h"
#include "thread.h"
#include <stdatomic.h>
typedef struct {
    Project *project;Sample *processed,*originals;Sampler *applied;
    unsigned *generation,*applied_generation,*epoch;
    void (*publish)(void *context,int channel,Sample sample);void *publish_context;
    void (*wake)(void *context);void *wake_context;
} SamplerBindings;
typedef struct {
    SamplerBindings bindings;
    struct {
        PlatformThread *thread;atomic_int done;int busy,channel,ok;
        unsigned generation,epoch;Sampler settings;Sample input,result;
    } job;
} SamplerService;
void sampler_service_init(SamplerService *service,SamplerBindings bindings);
void sampler_service_update(SamplerService *service);
/* recording skips even result adoption; editing_busy means the original
   control drag/Stretch gesture/number-popup5 gate. Exactly one worker. */
void sampler_service_queue(SamplerService *service,int recording,int editing_busy);
int sampler_service_flush(SamplerService *service);
void sampler_service_cancel(SamplerService *service);
#endif
