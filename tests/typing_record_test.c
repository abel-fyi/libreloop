// SPDX-License-Identifier: GPL-3.0-only
#include "midi_session.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project p;static MidiTake take;static MidiSession s;static int released=-1,downs;
static void key(void *ctx,int slot,int channel,int pitch,int down){(void)ctx;(void)slot;(void)pitch;if(down)downs++;else released=channel;}
static void velocity(void *ctx,int slot,int c,int pitch,int value){(void)ctx;(void)slot;(void)c;(void)pitch;(void)value;}
static void frame(AppInput *input,int down,int focused,double time) {
    input_begin(input,(uint64_t)(time*1e9));input->focused=focused;
    input_event(input,&(InputEvent){.kind=INPUT_KEY,.data.key={'Q',down?APP_PRESS:APP_RELEASE,0}});
    midi_session_typing_at(&s,input,0,0,time);
}
int main(void) {
    project_new(&p);p.bpm=120;p.channel_count=2;
    s=(MidiSession){.project=&p,.take=&take,.key=key,.key_velocity=velocity,.record_mask=MIDI_RECORD_NOTES,.typing_keys=1,.focused_editor=2,.piano_channel=1};
    AppInput input;input_init(&input);
    CHECK(midi_take_begin(&take,&p,1,1,0,10));int pattern=take.pattern;
    midi_session_typing_begin(&s,10);frame(&input,1,1,10.25);
    CHECK(p.notes[pattern][1][0].pitch==60 && p.notes[pattern][1][0].start==2 && downs==1);
    midi_session_event(&s,(MidiEvent){0x90,60,77,10.25}); /* Same pitch, independent MIDI hold. */
    frame(&input,1,1,10.4);CHECK(downs==1);
    midi_session_event(&s,(MidiEvent){0x80,60,0,10.5});s.channel=0;s.piano_channel=0;
    frame(&input,0,1,10.75);CHECK(released==1);
    CHECK(p.notes[pattern][1][0].length==4 && p.notes[pattern][1][1].length==2);
    midi_session_typing_end(&s,10.75);midi_take_finish(&take,&p,10.75);
    /* Held before Record/count-in gets a fresh note at time zero. */
    s.piano_channel=1;frame(&input,1,1,11);
    CHECK(midi_take_begin(&take,&p,1,1,0,12));pattern=take.pattern;
    midi_session_typing_begin(&s,12);CHECK(p.notes[pattern][1][0].start==0);
    frame(&input,1,0,12.5);CHECK(p.notes[pattern][1][0].length==4 && !s.typing_held[17]);
    midi_session_typing_end(&s,12.5);midi_take_finish(&take,&p,12.5);
    frame(&input,0,1,13);frame(&input,1,1,13.1);
    CHECK(midi_take_begin(&take,&p,1,1,0,14));pattern=take.pattern;midi_session_typing_begin(&s,14);
    midi_session_typing_end(&s,14.5);midi_take_finish(&take,&p,14.5);frame(&input,0,1,15);
    CHECK(p.notes[pattern][1][0].length==4); /* Stop finalizes a still-held key. */
    puts("Typing recording: timestamps, held keys, focus loss, target routing and simultaneous MIDI passed.");return 0;
}
