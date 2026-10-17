// SPDX-License-Identifier: GPL-3.0-only
#include "../src/ui/frontend/ui_internal.h"
#include <errno.h>
UiState ui;static MidiSession session;
#define RECORDING_FIXTURE_TYPING
#include "harness/recording_fixture_services.h"
static void key(void *ctx,int slot,int c,int pitch,int down){(void)ctx;(void)slot;(void)c;(void)pitch;(void)down;}
void midi_typing_record_begin(double time) {
    session.project=&ui.project;session.take=&ui.midi_take;session.record_mask=ui.record_mask;
    session.typing_keys=ui.typing_keys;session.key=key;
    midi_session_typing_begin(&session,time);
}
void midi_typing_record_end(double time){midi_session_typing_end(&session,time);}
int host_make_directory(const char *p){return directory_fixture(p);}
char *host_realpath(const char *p,char *out){return canonical_fixture(p,out);}
#include "../src/platform/recording.c"
#include "../src/ui/frontend/app_recording.c"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project before;
int main(void) {
    project_new(&ui.project);ui.project.bpm=120;ui.typing_keys=1;ui.record_mask=RECORD_NOTES;
    windows_init(&ui.windows,1448,1086);studio_workspace_init(&ui.windows,1448,1086,238);
    recording_start();CHECK(ui.midi_take.active && !ui.midi_connected && !popup);
    int pattern=ui.midi_take.pattern;AppInput input;input_init(&input);input.focused=1;
    fixture_time=10.25;input.keys['Q']=1;midi_session_typing_at(&session,&input,0,0,fixture_time);
    fixture_time=10.75;input.previous_keys['Q']=1;input.keys['Q']=0;midi_session_typing_at(&session,&input,0,0,fixture_time);
    CHECK(ui.project.notes[pattern][0][0].pitch==60 && ui.project.notes[pattern][0][0].start==2);
    recording_finish();CHECK(!ui.midi_take.active && ui.project.notes[pattern][0][0].length==4);
    ui.playing=0;ui.record_count_enabled=1;ui.record_count_steps=4;fixture_time=20;before=ui.project;
    recording_start();CHECK(ui.record_count_in.active && !ui.midi_take.active && !ui.playing && recording_active());
    CHECK(!memcmp(&before,&ui.project,sizeof before));fixture_time=20.499;recording_poll();CHECK(!ui.midi_take.active);
    recording_finish();CHECK(!recording_active() && !memcmp(&before,&ui.project,sizeof before));
    fixture_time=21;recording_start();fixture_time=21.5;recording_poll();
    CHECK(!ui.record_count_in.active && ui.midi_take.active && ui.midi_take.time==21.5 && ui.playing);
    fixture_time=21.7;recording_finish();CHECK(!ui.midi_take.active);
    ui.playing=ui.song=1;visual_frame=RATE*2;fixture_time=22;before=ui.project;
    recording_start();CHECK(ui.record_count_in.active && !ui.playing && ui.record_count_origin==16);
    recording_finish();CHECK(!memcmp(&before,&ui.project,sizeof before));
    ui.record_mask=RECORD_NOTES|RECORD_AUDIO;ui.record_armed[0]=1;ui.playing=ui.song=1;ui.playlist_start=.5f;fixture_time=23;
    recording_start();fixture_time=23.5;recording_poll();
    CHECK(ui.recording_ui.active && ui.midi_take.active && ui.recording_ui.start==16 && ui.midi_take.origin==16 && ui.playlist_start==1);
    fixture_time=23.7;recording_finish();
    for(int c=0;c<CHANNELS;c++){sample_free(ui.samples[c]);sample_free(ui.originals[c]);ui.samples[c]=ui.originals[c]=(Sample){0};}
    remove("recordings/take-0.wav");remove("recordings");ui.record_mask=RECORD_NOTES;
    ui.record_count_enabled=0;ui.typing_keys=0;popup=0;recording_start();CHECK(!ui.midi_take.active && popup==13);
    puts("Actual recording entry: laptop-only source, deferred take creation, exact count-in boundary and cancellation passed.");return 0;
}
