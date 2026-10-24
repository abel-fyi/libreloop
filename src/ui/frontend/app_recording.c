// SPDX-License-Identifier: GPL-3.0-only
#include "file_io.h"
#include "ui_internal.h"

static void recording_begin_now(int target,float origin);

void recording_finish(void) {
    ui.record_enabled=0;
    if(ui.record_count_in.active){ui.record_count_in.active=0;snprintf(ui.status,sizeof ui.status,"Count-in cancelled; no take created");return;}

    if(ui.midi_take.active) {
        int failed=ui.midi_take.failed;midi_typing_record_end(midi_input_time());audio_record_mode(0);midi_take_finish(&ui.midi_take,&ui.project,midi_input_time());midi_panic();
        ui.pattern=fmaxf(0,fminf(ui.pattern,ui.project.pattern_count-1));
        snprintf(ui.status,sizeof ui.status,failed?"MIDI recording stopped at the note or track limit; captured take kept":ui.midi_take.in_place?"Pattern take saved":"MIDI take saved in the Playlist");
    }
    if(!ui.recording_ui.active) return;
    int ok=recording_finish_writers(&ui.recording_ui,&ui.project);
    for(int i=0;i<ui.recording_ui.count;i++) {
        RecordingTake *take=&ui.recording_ui.takes[i]; int c=take->channel;
        unsigned committed=recording_writer_frames(take->writer);
        recording_writer_free(take->writer,0); take->writer=NULL;
        if(!take->sample.frames && committed) {
            /* A mapping failure must never delete successfully written audio. */
            ui.project.audio_seconds[c]=committed/(float)RATE;
            if(take->clip>=0) ui.project.clip_steps[take->lane][take->clip]=0;
            snprintf(ui.project.paths[c],sizeof ui.project.paths[c],"%s",take->path); ok=0;
        } else if(!take->sample.frames) {
            if(take->clip>=0) ui.project.clips[take->lane][take->clip]=0;
            ui.project.audio_seconds[c]=0;
            snprintf(ui.project.paths[c],sizeof ui.project.paths[c],"%s",SAMPLE_EMPTY); platform_file_remove(take->path);
            sample_free(take->sample);
        } else {
            if(take->clip>=0) ui.project.clip_steps[take->lane][take->clip]=0;
            Sample processed;
            if(!sample_process(take->sample,ui.project.sampler[c],&processed)) {
                processed=take->sample; take->sample=(Sample){0}; ok=0;
            }
            ui.originals[c]=take->sample; ui.samples[c]=processed; audio_sample(c,processed);
            ui.sampler_applied[c]=ui.project.sampler[c]; ui.sample_generation[c]++; ui.sampler_generation[c]=ui.sample_generation[c];
            ui.project.audio_seconds[c]=processed.frames/(float)RATE;
            snprintf(ui.project.paths[c],sizeof ui.project.paths[c],"%s",take->path);
        }
        free(take->wave.tree); take->wave=(Waveform){0}; take->sample=(Sample){0};
    }
    snprintf(ui.status,sizeof ui.status,ok?"Recording saved: %d audio clips in recordings/":"Recording stopped after a device, buffer, memory or disk error; captured audio kept in recordings/",ui.recording_ui.count);
    ui.recording_ui.count=0;
}

void recording_poll(void) {
    if(ui.record_count_in.active) {
        if(!record_count_in_remaining(&ui.record_count_in,midi_input_time())) {
            ui.record_count_in.active=0;recording_begin_now(ui.record_count_target,ui.record_count_origin);
        }
        return;
    }
    if(ui.midi_take.active && !ui.playing){recording_finish();return;}
    if(ui.recording_ui.active && (!recording_refresh(&ui.recording_ui,&ui.project) || audio_record_failed() || !ui.playing)) recording_finish();
}

void recording_start_audio(float start) {
    if(ui.recording_ui.active) { recording_finish(); return; }
    int buses[CHANNELS],count=0;
    for(int id=0;id<=ui.project.insert_count;id++) if(ui.record_armed[id]) {
        if(count>=CHANNELS-ui.project.channel_count) { snprintf(ui.status,sizeof ui.status,"Not enough free Channel Rack slots for all armed tracks."); return; }
        buses[count++]=id;
    }
    if(!count) { snprintf(ui.status,sizeof ui.status,"Arm a mixer track with its red recording button first."); return; }
    if(!sampler_flush()) { snprintf(ui.status,sizeof ui.status,"Wait for sample processing before recording."); return; }
    if(host_make_directory("recordings")!=0) { snprintf(ui.status,sizeof ui.status,"Cannot create recordings/ in the working directory."); return; }
    char directory[LL_PATH_CAPACITY]; if(!host_realpath("recordings",directory)) { snprintf(ui.status,sizeof ui.status,"Cannot open recordings/ in the working directory."); return; }
    Project *next=malloc(sizeof *next);
    if(!next) goto failed;
    *next=ui.project;
    if(!recording_prepare(&ui.recording_ui,next,buses,count,start,directory)) goto failed;
    char error[256]={0};
    /* Record the selected transport; Song takes run freely, Pattern keeps looping. */
    for(int i=0;i<count;i++) audio_sample(ui.recording_ui.takes[i].channel,(Sample){0});
    if(!audio_record_start(next,buses,count,start,ui.output_volume,ui.song,ui.pattern,ui.record_loop[0],ui.record_loop[1],error)) {
        audio_channels(&ui.project,ui.samples);
        audio_update(&ui.project,ui.playing,ui.song,ui.pattern,1,ui.output_volume,playback_start(),playback_loop()[0],playback_loop()[1]);
        snprintf(ui.status,sizeof ui.status,"%s",error); goto cleanup;
    }
    if(!recording_start_workers(&ui.recording_ui)) {
        audio_record_end(); audio_channels(&ui.project,ui.samples);
        audio_update(&ui.project,ui.playing,ui.song,ui.pattern,1,ui.output_volume,playback_start(),playback_loop()[0],playback_loop()[1]);
        snprintf(ui.status,sizeof ui.status,"Could not start the recording writer."); goto cleanup;
    }
    for(int i=0;i<count;i++) {
        int c=ui.recording_ui.takes[i].channel; sample_free(ui.samples[c]); sample_free(ui.originals[c]);
        ui.samples[c]=ui.originals[c]=(Sample){0}; ui.sampler_applied[c]=next->sampler[c];
    }
    ui.project=*next; free(next); ui.recording_ui.active=1; ui.recording_ui.bpm=ui.project.bpm; ui.recording_ui.start=start;
    ui.playing=1; ui.reset=0;
    if(ui.song){ui.playlist_start=start/STEPS;memset(ui.song_loop,0,sizeof ui.song_loop);}
    memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected);
    for(int i=0;i<count;i++) { RecordingTake *take=&ui.recording_ui.takes[i]; ui.arrangement.selected[take->lane][take->clip]=1; }
    ui.picker_tab=1; ui.picker_scroll=0; ui.channel=ui.instrument_channel=ui.recording_ui.takes[0].channel;
    ui.arrangement.source_pattern=-1; ui.arrangement.source_offset=0;
    if(ui.song)windows_focus(&ui.windows,1);
    ui.browser_focus=0;
    snprintf(ui.status,sizeof ui.status,"Recording %d armed mixer tracks; Stop or Record finishes the takes.",count); return;
failed:
    free(next);
    snprintf(ui.status,sizeof ui.status,"Cannot prepare recording: free Playlist tracks, memory or writable disk space required.");
cleanup:
    recording_cancel(&ui.recording_ui);
}

static int recording_ready(void) {
    if(!ui.record_mask){snprintf(ui.status,sizeof ui.status,"Enable Audio, Notes or Automation recording");return 0;}
    if((ui.record_mask&RECORD_NOTES) && !ui.midi_connected && !ui.typing_keys){snprintf(ui.status,sizeof ui.status,"Choose a MIDI input in View > MIDI / Recording");open_popup(13);return 0;}
    return 1;
}
void recording_start(void) {
    if(ui.record_enabled || recording_active()){recording_finish();snprintf(ui.status,sizeof ui.status,"Recording disarmed");return;}
    if(!ui.record_mask){snprintf(ui.status,sizeof ui.status,"Enable Audio, Notes or Automation recording");return;}
    ui.record_enabled=1;
    snprintf(ui.status,sizeof ui.status,ui.playing?"Recording armed; stop, then Play to begin":"Recording armed; Play starts the take; right-click Record for options");
}
int recording_play(void) {
    if(!ui.record_enabled || recording_active())return 0;
    midi_poll();if(!recording_ready())return 0;
    int target=midi_target();
    float origin=ui.playing?audio_visual_position()/(RATE*15/ui.project.bpm):ui.song?ui.playlist_start*STEPS:ui.piano_start[ui.pattern];
    if(!ui.song) {
        float *range=ui.pattern_loop[ui.pattern];
        ui.record_loop[0]=range[1]>range[0]?range[0]:0;
        ui.record_loop[1]=range[1]>range[0]?range[1]:pattern_playback_steps(&ui.project,ui.pattern);
        if(origin<ui.record_loop[0] || origin>=ui.record_loop[1])origin=ui.record_loop[0];
    }else ui.record_loop[0]=ui.record_loop[1]=0;
    ui.midi_record_target=target;
    if(ui.record_count_enabled) {
        int steps=isfinite(ui.record_count_steps)?(int)fmaxf(1,fminf(4096,roundf(ui.record_count_steps))):16;
        ui.record_count_steps=steps;ui.record_count_target=target;ui.record_count_origin=origin;
        record_count_in_begin(&ui.record_count_in,steps,ui.project.bpm,midi_input_time());
        ui.playing=0;ui.reset=1;ui.browser_focus=0;snprintf(ui.status,sizeof ui.status,"Count-in: %d sixteenth-note steps; Stop or Record cancels",steps);return 1;
    }
    recording_begin_now(target,origin);return recording_active();
}
static void recording_begin_now(int target,float origin) {
    if(!recording_ready())return;
    ui.midi_record_target=target;
    if(!history_checkpoint())return;
    if(ui.record_mask&(RECORD_NOTES|RECORD_AUTOMATION)) {
        int ok=ui.song?midi_take_begin(&ui.midi_take,&ui.project,target,!!(ui.record_mask&RECORD_NOTES),origin,midi_input_time()):
            midi_take_begin_pattern(&ui.midi_take,&ui.project,target,(ui.record_mask&RECORD_NOTES)?ui.pattern:-1,ui.record_new_pattern,origin,ui.record_loop[0],ui.record_loop[1],midi_input_time());
        if(!ok){snprintf(ui.status,sizeof ui.status,ui.song?"MIDI recording needs a free pattern and Playlist track":"Cannot start Pattern take: check pattern capacity and loop range");return;}
        if(!ui.song && ui.midi_take.pattern>=0)ui.pattern=ui.midi_take.pattern;
    }
    int armed=0;for(int i=0;i<=ui.project.insert_count;i++)armed+=ui.record_armed[i]!=0;
    if((ui.record_mask&RECORD_AUDIO) && armed){if(!ui.playing && ui.song)ui.playlist_start=origin/STEPS;recording_start_audio(origin);if(!ui.recording_ui.active){
        if(ui.midi_take.active)midi_take_finish(&ui.midi_take,&ui.project,midi_input_time());
        if(!ui.song){ui.pattern=fminf(ui.pattern,ui.project.pattern_count-1);ui.reset=1;}
        return;
    }}
    if(!ui.recording_ui.active && !ui.midi_take.active){snprintf(ui.status,sizeof ui.status,"Arm a mixer track to record audio");return;}
    if(ui.midi_take.active){
        audio_record_mode(1);double now=midi_input_time();
        if(ui.recording_ui.active)ui.midi_take.time=ui.song?now-fmax(0,audio_visual_position()/(RATE*15/ui.project.bpm)-origin)*15/ui.project.bpm:now;
        if(ui.song)ui.playlist_start=origin/STEPS;
        if(!ui.playing){ui.playing=1;ui.reset=1;}
        else if(!ui.recording_ui.active)ui.reset=0;
        if(!ui.recording_ui.active){audio_update(&ui.project,1,ui.song,ui.pattern,ui.reset,ui.output_volume,origin,ui.record_loop[0],ui.record_loop[1]);ui.reset=0;}
        if(ui.midi_take.pattern>=0){ui.pattern=ui.midi_take.pattern;ui.piano_channel=target;ui.piano_channels[ui.pattern][target]=1;}
        midi_typing_record_begin(ui.midi_take.time);
        for(int i=0;i<32;i++)if(ui.midi_keys[i].used && ui.midi_keys[i].target==target && (ui.record_mask&RECORD_NOTES))midi_take_note(&ui.midi_take,&ui.project,ui.midi_keys[i].mchannel,ui.midi_keys[i].pitch,ui.midi_keys[i].velocity,ui.midi_take.time);
        if(ui.song)memset(ui.song_loop,0,sizeof ui.song_loop);
        windows_focus(&ui.windows,2);ui.browser_focus=0;
        snprintf(ui.status,sizeof ui.status,"Recording MIDI to %s; Stop finishes the take",ui.project.channel_names[target]);
    }
}


int recording_active(void) {return ui.record_count_in.active || ui.recording_ui.active || ui.midi_take.active;}

int recording_source(int source) {
    if(ui.recording_ui.active) for(int i=0;i<ui.recording_ui.count;i++) if(source==PATTERNS+ui.recording_ui.takes[i].channel) return 1;
    return 0;
}
