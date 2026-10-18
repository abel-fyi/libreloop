// SPDX-License-Identifier: GPL-3.0-only
#define main old_piano_capture_main
#define LIBRELOOP_CAPTURE_REAL_HISTORY
#include "frontend_piano_capture.c"
#undef main
#include "../../src/ui/frontend/app_project.c"
static int test_capture;
static FILE *selections;
int captured(void){return test_capture;}
int recording_active(void){return ui.recording_ui.active || ui.midi_take.active;}
void audio_channels(const Project *project,const Sample samples[CHANNELS]){(void)project;(void)samples;}
int host_thread_join(PlatformThread *thread,void **result){(void)thread;(void)result;abort();return 0;}
#include "clipboard_history_sequence.h"
int main(void) {
    project_new(&ui.project);ui.project.insert_count=INSERTS; /* Frozen reference project geometry. */ui.project.channel_count=2;ui.project.pattern_count=2;
    ui.project.notes[0][0][0]=(Note){60,90,0,1};ui.project.notes[0][0][1]=(Note){64,110,2,2};
    ui.project.clips[0][0]=1;ui.project.clip_steps[0][0]=16;ui.project.clip_starts[0][0]=0;
    ui.project.channel_audio[1]=1;ui.project.audio_seconds[1]=2;snprintf(ui.project.paths[1],sizeof ui.project.paths[1],"missing.wav");
    windows_init(&ui.windows,1200,675);windows_focus(&ui.windows,2);ui.piano_top=72;ui.piano_zoom=1;
    ui.selected_track=-1;ui.arrangement.source_pattern=0;ui.arrangement.source_steps=8;
    for(int c=0;c<CHANNELS;c++)ui.sampler_applied[c]=ui.project.sampler[c];
    trace=fopen("history.txt","w");selections=fopen("selections.bin","wb");if(!trace || !selections)return 1;
    sequence();fclose(trace);fclose(selections);history_clear(&ui.edit_history);return 0;
}
