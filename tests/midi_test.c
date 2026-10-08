// SPDX-License-Identifier: GPL-3.0-only
#include "midi.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"Failed %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project p,q;static MidiTake take;static MidiEvent events[16];static int count;
static void emit(void *context,MidiEvent event){(void)context;events[count++]=event;}
int main(void){
 MidiParser parser={0};unsigned char a[]={0x91,60},b[]={100,0xf8,64,90,0x91,60,0,0xf0,2,3,0xf7,0xb2,1,127};
 midi_parse(&parser,a,sizeof a,1,emit,NULL);midi_parse(&parser,b,sizeof b,2,emit,NULL);
 CHECK(count==4 && events[0].status==0x91 && events[0].data1==60 && events[0].data2==100 && events[2].data2==0 && events[3].status==0xb2);
 project_new(&p);CHECK(midi_take_begin(&take,&p,0,1,32,10));CHECK(p.pattern_count==2 && p.clips[0][0]==2 && p.lane_mute[0]==1);
 CHECK(midi_take_note(&take,&p,0,60,110,10.25));CHECK(midi_take_note(&take,&p,0,64,80,10.25));
 midi_take_update(&take,&p,10.5);CHECK(p.notes[1][0][0].start==2 && p.notes[1][0][0].length==2);
 CHECK(midi_take_note(&take,&p,0,60,0,10.75));CHECK(p.notes[1][0][0].length==4 && p.notes[1][0][0].velocity==110);
 ParameterTarget target={PARAM_CHANNEL_PAN,0,0};CHECK(midi_bind(&p,2,10,target));CHECK(parameter_write(&p,target,.75f) && p.pan[0]==.5f);
 CHECK(midi_take_control(&take,&p,target,.25f,10.5));CHECK(midi_take_control(&take,&p,target,.75f,10.75));
 midi_take_finish(&take,&p,11);CHECK(!take.active && !p.lane_mute[0] && p.notes[1][0][1].length==6);
 CHECK(p.automation_count==1 && p.automations[0].count==4 && automation_valid(&p));
 CHECK(project_save("midi.hbt",&p) && project_load("midi.hbt",&q));CHECK(q.midi_binding_count==1 && q.midi_bindings[0].controller==10 && q.notes[1][0][0].velocity==110);remove("midi.hbt");
 midi_unbind(&p,target);CHECK(p.midi_binding_count==0);
 project_new(&p);q=p;CHECK(midi_take_begin(&take,&p,0,1,0,10));midi_take_finish(&take,&p,10.1);CHECK(p.pattern_count==1 && !p.clips[0][0] && !memcmp(&p,&q,sizeof p));
 project_new(&p);CHECK(midi_take_begin(&take,&p,0,0,0,10));
 for(int i=0;i<300;i++)CHECK(midi_take_control(&take,&p,target,(i%17)/16.f,10+i*.02));
 midi_take_finish(&take,&p,16);CHECK(automation_valid(&p) && p.automations[0].count<=AUTOMATION_POINTS);
 project_new(&p);p.channel_count=3;CHECK(midi_bind(&p,0,1,(ParameterTarget){PARAM_CHANNEL_VOLUME,2,0}));
 CHECK(midi_bind(&p,0,2,(ParameterTarget){PARAM_CHANNEL_PAN,1,0}));CHECK(channel_delete(&p,1));
 CHECK(p.midi_binding_count==1 && p.midi_bindings[0].target.owner==1);
 puts("MIDI parser, polyphonic recording, gates, automation, bindings and persistence passed.");return 0;
}
