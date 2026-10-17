// SPDX-License-Identifier: GPL-3.0-only
/* Frozen app_project.c sampler_worker/update/queue/flush, with borrowed app
   state and a concrete native thread/audio-publication boundary. */
#include "sampler_service.h"
#include <string.h>
static void duration(Project *p,int c,Sample processed,Sample original) {
    if(original.frames || !p->paths[c][0] || !strcmp(p->paths[c],SAMPLE_EMPTY))p->audio_seconds[c]=processed.frames/(float)RATE;
}
static void publish(SamplerService *s,int c,Sample sample){if(s->bindings.publish)s->bindings.publish(s->bindings.publish_context,c,sample);}
static void *worker(void *context) {
    SamplerService *s=context;s->job.ok=sample_process(s->job.input,s->job.settings,&s->job.result);
    atomic_store(&s->job.done,1);if(s->bindings.wake)s->bindings.wake(s->bindings.wake_context);return NULL;
}
void sampler_service_init(SamplerService *s,SamplerBindings bindings){memset(s,0,sizeof *s);s->bindings=bindings;atomic_init(&s->job.done,0);}
void sampler_service_update(SamplerService *s) {
    SamplerBindings b=s->bindings;Project *p=b.project;
    if(s->job.busy && atomic_load(&s->job.done)) {
        platform_thread_join(s->job.thread);s->job.thread=NULL;int c=s->job.channel;
        if(s->job.epoch==*b.epoch && c<p->channel_count && s->job.generation==b.generation[c] && sampler_processing_equal(s->job.settings,p->sampler[c])) {
            if(s->job.ok) {
                Sample old=b.processed[c];publish(s,c,s->job.result);b.processed[c]=s->job.result;s->job.result=(Sample){0};sample_free(old);
            }else p->sampler[c]=b.applied[c];
            duration(p,c,b.processed[c],b.originals[c]);b.applied[c]=p->sampler[c];b.applied_generation[c]=b.generation[c];
        }
        sample_free(s->job.input);sample_free(s->job.result);s->job.busy=0;
    }
}
void sampler_service_cancel(SamplerService *s) {
    if(s->job.busy){platform_thread_join(s->job.thread);s->job.thread=NULL;sample_free(s->job.input);sample_free(s->job.result);s->job.busy=0;}
}
int sampler_service_flush(SamplerService *s) {
    SamplerBindings b=s->bindings;Project *p=b.project;sampler_service_cancel(s);
    for(int c=0;c<p->channel_count;c++)if(b.applied_generation[c]!=b.generation[c] || !sampler_processing_equal(b.applied[c],p->sampler[c])) {
        Sample next;if(!sample_process(b.originals[c],p->sampler[c],&next))return 0;
        Sample old=b.processed[c];publish(s,c,next);b.processed[c]=next;sample_free(old);
        duration(p,c,b.processed[c],b.originals[c]);b.applied[c]=p->sampler[c];b.applied_generation[c]=b.generation[c];
    }
    return 1;
}
void sampler_service_queue(SamplerService *s,int recording,int busy) {
    if(recording)return;sampler_service_update(s);if(s->job.busy || busy)return;
    SamplerBindings b=s->bindings;Project *p=b.project;
    for(int c=0;c<p->channel_count;c++)if(b.applied_generation[c]!=b.generation[c] || !sampler_processing_equal(b.applied[c],p->sampler[c])) {
        Sample copy={0};sample_clone(b.originals[c],&copy);if(b.originals[c].frames && !copy.data)return;
        s->job.input=copy;s->job.result=(Sample){0};s->job.channel=c;s->job.settings=p->sampler[c];s->job.epoch=*b.epoch;s->job.generation=b.generation[c];atomic_store(&s->job.done,0);
        s->job.thread=platform_thread_start("sampler-processing",worker,s);
        if(!s->job.thread){sample_free(copy);return;}s->job.busy=1;return;
    }
}
