// SPDX-License-Identifier: GPL-3.0-only
#include "../src/ui/frontend/ui_internal.h"
#include <errno.h>
UiState ui;static MidiSession session;
#define RECORDING_FIXTURE_TRANSPORT
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
int audio_transport_stop(void){return 0;}
int audio_stop(void){return 0;}
#define LIBRELOOP_LOGO ""
#define LIBRELOOP_SAMPLES ""
#define LIBRELOOP_FONT ""
#include "../src/ui/frontend/ui_shell.c"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project before;
static void idle(void) {recording_finish();ui.playing=0;ui.reset=0;}
int main(void) {
    project_new(&ui.project);ui.project.bpm=120;ui.typing_keys=1;ui.record_mask=RECORD_NOTES;ui.song=1;
    windows_init(&ui.windows,1448,1086);studio_workspace_init(&ui.windows,1448,1086,238);
    before=ui.project;int old_updates=updates;
    recording_start();CHECK(ui.record_enabled && !ui.playing && !recording_active());
    CHECK(updates==old_updates && !memcmp(&before,&ui.project,sizeof before));
    fixture_time=12;recording_poll();CHECK(!recording_active() && !ui.record_count_in.active);
    recording_start();CHECK(!ui.record_enabled && !memcmp(&before,&ui.project,sizeof before));
    fixture_time=10;recording_start();transport_toggle();
    CHECK(ui.midi_take.active && ui.playing && ui.song && !ui.midi_connected && !popup);
    int pattern=ui.midi_take.pattern;AppInput input;input_init(&input);input.focused=1;
    fixture_time=10.25;input.keys['Q']=1;midi_session_typing_at(&session,&input,0,0,fixture_time);
    fixture_time=10.75;input.previous_keys['Q']=1;input.keys['Q']=0;midi_session_typing_at(&session,&input,0,0,fixture_time);
    CHECK(ui.project.notes[pattern][0][0].pitch==60 && ui.project.notes[pattern][0][0].start==2);
    transport_stop();CHECK(!ui.midi_take.active && !ui.playing && ui.record_enabled && ui.project.notes[pattern][0][0].length==4);
    idle();ui.record_count_enabled=1;ui.record_count_steps=4;fixture_time=20;before=ui.project;
    recording_start();CHECK(!ui.record_count_in.active);transport_toggle();
    CHECK(ui.record_count_in.active && !ui.midi_take.active && !ui.playing && recording_active());
    CHECK(!memcmp(&before,&ui.project,sizeof before));fixture_time=20.499;recording_poll();CHECK(!ui.midi_take.active);
    transport_toggle();CHECK(!recording_active() && !ui.playing && ui.record_enabled && !memcmp(&before,&ui.project,sizeof before));
    fixture_time=21;transport_toggle();fixture_time=21.5;recording_poll();
    CHECK(!ui.record_count_in.active && ui.midi_take.active && ui.midi_take.time==21.5 && ui.playing);
    fixture_time=21.7;transport_stop();CHECK(!ui.midi_take.active);
    idle();ui.record_count_enabled=0;ui.song=0;ui.pattern=0;ui.record_new_pattern=0;project_new(&ui.project);
    CHECK(note_add(&ui.project,0,0,0,48,1));before=ui.project;
    fixture_time=30;recording_start();CHECK(!ui.song && !ui.playing);transport_toggle();
    CHECK(ui.midi_take.active && ui.midi_take.in_place && ui.pattern==0 && !ui.song && last_song==0 && last_loop_end==16);
    CHECK(midi_take_note(&ui.midi_take,&ui.project,0,60,100,30.25));
    CHECK(midi_take_note(&ui.midi_take,&ui.project,0,60,0,30.75));
    fixture_time=30.75;transport_stop();
    CHECK(ui.project.pattern_count==1 && !ui.project.clips[0][0] && !memcmp(&ui.project.notes[0][0][0],&before.notes[0][0][0],sizeof(Note)));
    CHECK(ui.project.notes[0][0][1].start==2 && ui.project.notes[0][0][1].length==4);
    fixture_time=31;transport_toggle();
    CHECK(midi_take_note(&ui.midi_take,&ui.project,0,62,100,32.875));
    CHECK(midi_take_note(&ui.midi_take,&ui.project,0,62,0,33.125));
    CHECK(ui.project.notes[0][0][2].start==15 && ui.project.notes[0][0][2].length==1);
    CHECK(midi_take_note(&ui.midi_take,&ui.project,0,64,100,33.25));
    CHECK(midi_take_note(&ui.midi_take,&ui.project,0,64,0,33.5));
    fixture_time=33.5;transport_stop();CHECK(ui.project.notes[0][0][3].start==2 && ui.project.pattern_steps[0]==16);
    idle();ui.record_new_pattern=1;before=ui.project;fixture_time=40;recording_start();transport_toggle();
    CHECK(!ui.song && ui.midi_take.created_pattern && ui.pattern==1 && ui.project.pattern_count==2 && !ui.project.clips[0][0]);
    CHECK(midi_take_note(&ui.midi_take,&ui.project,0,67,100,40.25));
    fixture_time=40.5;transport_stop();CHECK(!memcmp(ui.project.notes[0],before.notes[0],sizeof before.notes[0]));
    CHECK(ui.project.notes[1][0][0].length==2);
    idle();before=ui.project;fixture_time=41;recording_start();transport_toggle();fixture_time=41.1;transport_stop();
    CHECK(ui.project.pattern_count==before.pattern_count && ui.pattern<ui.project.pattern_count);
    idle();ui.song=0;ui.pattern=0;ui.record_mask=RECORD_NOTES|RECORD_AUDIO;ui.record_armed[0]=1;ui.record_count_enabled=1;
    ui.piano_start[0]=4;fixture_time=50;recording_start();transport_toggle();CHECK(ui.record_count_origin==4 && !ui.song);
    fixture_time=50.5;recording_poll();
    CHECK(ui.recording_ui.active && ui.midi_take.active && !ui.song && ui.recording_ui.start==4 && last_song==0 && last_loop_end==16);
    fixture_time=50.7;transport_stop();CHECK(!ui.recording_ui.active && !ui.midi_take.active && !ui.song);
    for(int c=0;c<CHANNELS;c++){sample_free(ui.samples[c]);sample_free(ui.originals[c]);ui.samples[c]=ui.originals[c]=(Sample){0};}
    remove("recordings/take-0.wav");remove("recordings");
    idle();ui.record_mask=RECORD_NOTES;ui.record_count_enabled=0;ui.typing_keys=0;popup=0;
    recording_start();CHECK(ui.record_enabled && !popup && !ui.playing);transport_toggle();CHECK(!ui.midi_take.active && !ui.playing && popup==13);
    puts("Record arms; Play starts; Song and Pattern takes, overdub/new pattern, loop bounds, audio and count-in passed.");return 0;
}
