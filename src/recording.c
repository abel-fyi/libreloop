// SPDX-License-Identifier: GPL-3.0-only
#include "recording.h"
#include "audio.h"
#include "arrangement.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

static unsigned recording_read(void *context,int take,float *stereo,unsigned frames) {
    (void)context; return audio_record_read(take,stereo,frames);
}
int recording_refresh(RecordingSession *session,Project *project) {
    int ok=1;
    for(int i=0;i<session->count;i++) {
        RecordingTake *take=&session->takes[i];
        if(recording_writer_failed(take->writer)) ok=0;
        if(take->clip<0 || project->clips[take->lane][take->clip]!=PATTERNS+take->channel+1) {
            take->clip=-1;
            for(int l=0;l<LANES && take->clip<0;l++) for(int b=0;b<CLIPS;b++) if(project->clips[l][b]==PATTERNS+take->channel+1) { take->lane=l; take->clip=b; break; }
        }
        unsigned frames=recording_writer_frames(take->writer),old=take->sample.frames;
        if(frames>old) {
            Sample mapped;
            if(!sample_map_recording(take->path,frames,&mapped)) { ok=0; continue; }
            sample_free(take->sample); take->sample=mapped;
            if(!waveform_append(&take->wave,take->sample,old)) ok=0;
            project->audio_seconds[take->channel]=frames/(float)RATE;
            if(take->clip>=0) project->clip_steps[take->lane][take->clip]=project->audio_seconds[take->channel];
        }
    }
    return ok;
}
int recording_prepare(RecordingSession *session,Project *next,const int buses[],int count,float start,const char *directory) {
    if(session->active || session->count || !buses || next->channel_count<0 || next->channel_count>CHANNELS ||
       count<1 || count>CHANNELS-next->channel_count || !isfinite(start) || start<0) return 0;
    for(int i=0;i<count;i++) if(buses[i]<0 || buses[i]>next->insert_count) return 0;
    session->count=0;
    for(int i=0;i<count;i++) {
        int lane=0;
        for(;lane<LANES;lane++) { int occupied=0; for(int b=0;b<CLIPS;b++) occupied|=next->clips[lane][b]!=0; if(!occupied) break; }
        if(lane==LANES) goto failed;
        RecordingTake *take=&session->takes[i]; memset(take,0,sizeof *take);
        take->channel=next->channel_count++; take->lane=lane; take->bus=buses[i];
        session->count=i+1;
        take->writer=recording_writer_open(directory); if(!take->writer) goto failed;
        if(strlen(recording_writer_path(take->writer))>=sizeof next->paths[0]) goto failed;
        snprintf(take->path,sizeof take->path,"%s",recording_writer_path(take->writer));
        int c=take->channel; next->channel_audio[c]=1; next->audio_seconds[c]=.001f;
        next->volume[c]=1; next->pan[c]=next->channel_pitch[c]=0; next->pitch_range[c]=2; next->mute[c]=0;
        next->route[c]=0; next->sampler[c]=(Sampler){.time=1,.length=1};
        snprintf(next->paths[c],sizeof next->paths[c],"%s",take->path);
        snprintf(next->channel_names[c],PATTERN_NAME,"%s take",buses[i]?next->insert_names[buses[i]-1]:"Master");
        unsigned colors[CHANNELS]; int ncolors=0;
        for(int j=0;j<c;j++) if(next->channel_audio[j]) colors[ncolors++]=next->channel_colors[j];
        next->channel_colors[c]=next_source_color(colors,ncolors);
        take->clip=arrangement_place(next,lane,start/STEPS,PATTERNS+c,.001f*next->bpm/15);
        if(take->clip<0) goto failed;
        next->clip_steps[lane][take->clip]=0;
    }
    return 1;
failed:
    recording_cancel(session); return 0;
}
int recording_start_workers(RecordingSession *session) {
    for(int i=0;i<session->count;i++) if(!recording_writer_start(session->takes[i].writer,recording_read,NULL,i)) return 0;
    return 1;
}
int recording_finish_writers(RecordingSession *session,Project *project) {
    audio_record_end(); int ok=!audio_record_failed();
    for(int i=0;i<session->count;i++) if(!recording_writer_finish(session->takes[i].writer)) ok=0;
    if(!recording_refresh(session,project)) ok=0;
    session->active=0; return ok;
}
void recording_cancel(RecordingSession *session) {
    for(int i=0;i<session->count;i++) {
        RecordingTake *take=&session->takes[i]; recording_writer_free(take->writer,1);
        sample_free(take->sample); free(take->wave.tree); memset(take,0,sizeof *take);
    }
    session->count=0;
}
