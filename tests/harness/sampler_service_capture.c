// SPDX-License-Identifier: GPL-3.0-only
#include "sampler_service.h"
#include "frame_wait.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static Project project;static Sample originals[CHANNELS],processed[CHANNELS];static Sampler applied[CHANNELS];static unsigned generation[CHANNELS],applied_generation[CHANNELS],epoch;
static SamplerService service;static FILE *out,*pcm;static atomic_int wakes;static int publications;
static void wake(void *unused){(void)unused;atomic_fetch_add(&wakes,1);}
static void publish(void *unused,int channel,Sample sample){(void)unused;fprintf(out,"publish %d %u %d\n",channel,sample.frames,sample.channels);fwrite(sample.data,sizeof(float),(size_t)sample.frames*sample_channels(sample),pcm);publications++;}
static void snapshot(const char *name){Sampler s=project.sampler[0],a=applied[0];fprintf(out,"%s %d %d %d %d %u %u %u %u %.9g %.9g %u %.9g %.9g %u %.9g\n",name,service.job.busy,atomic_load(&service.job.done),atomic_load(&wakes),publications,epoch,generation[0],applied_generation[0],processed[0].frames,s.pitch,s.time,s.flags,a.pitch,a.time,a.flags,project.audio_seconds[0]);}
#define SETTINGS project.sampler[0]
#define EPOCH epoch
#define GENERATION generation[0]
#define QUEUE(r,b) sampler_service_queue(&service,r,b)
#define DONE() atomic_load(&service.job.done)
#define UPDATE() sampler_service_update(&service)
#define FLUSH() sampler_service_flush(&service)
#define SNAP snapshot

static void wait_done(void){while(!DONE())platform_frame_wait(.001);}
static void run(void){
    SETTINGS.pitch=3.75f;SETTINGS.flags=SAMPLE_REVERSE;QUEUE(1,0);SNAP("recording-gate");QUEUE(0,1);SNAP("editing-gate");
    QUEUE(0,0);wait_done();UPDATE();SNAP("adopt");
    SETTINGS.flags=SAMPLE_NORMALIZE;QUEUE(0,0);EPOCH++;wait_done();UPDATE();SNAP("stale-epoch");
    SETTINGS.pitch=-5.25f;QUEUE(0,0);GENERATION++;wait_done();UPDATE();SNAP("stale-generation");
    SETTINGS.pitch=7;QUEUE(0,0);if(!FLUSH())exit(3);SNAP("flush-reprocess");
}

int main(void){project_new(&project);project.channel_count=1;originals[0]=(Sample){.data=malloc(64*sizeof(float)),.frames=64,.channels=1};for(int i=0;i<64;i++)originals[0].data[i]=(i-32)/64.f;applied[0]=project.sampler[0];generation[0]=applied_generation[0]=1;if(!sample_process(originals[0],project.sampler[0],&processed[0]))return 2;sampler_service_init(&service,(SamplerBindings){.project=&project,.processed=processed,.originals=originals,.applied=applied,.generation=generation,.applied_generation=applied_generation,.epoch=&epoch,.publish=publish,.wake=wake});out=fopen("service.txt","wb");pcm=fopen("published.f32","wb");if(!out || !pcm)return 2;run();sampler_service_cancel(&service);fclose(out);fclose(pcm);sample_free(originals[0]);sample_free(processed[0]);return 0;}
