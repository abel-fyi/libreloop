// SPDX-License-Identifier: GPL-3.0-only
#include "midi_session.h"
#include <stdlib.h>
#include <string.h>
#include "midi_sequence.h"
static Project project;static MidiTake take;static MidiSession state;
/* Keep the frozen Song replay's byte layout independent of new take fields. */
typedef struct {
    int active,pattern,channel,lane,clip,failed;
    float origin,bpm; double time;
    unsigned char lane_mute[AUTOMATIONS+1];
    Note previous_notes[CHANNELS][NOTES];
    char previous_name[PATTERN_NAME];
    float previous_steps,previous_start,previous_cap,previous_offset;
    int held[16][128],curves[AUTOMATIONS],curve_lane[AUTOMATIONS],curve_clip[AUTOMATIONS],count;
} OriginalMidiTake;
static void capture_take(FILE *file) {
    OriginalMidiTake original;memset(&original,0,sizeof original);
#define COPY(field) memcpy(&original.field,&take.field,sizeof original.field)
    COPY(active);COPY(pattern);COPY(channel);COPY(lane);COPY(clip);COPY(failed);
    COPY(origin);COPY(bpm);COPY(time);COPY(lane_mute);COPY(previous_notes);
    COPY(previous_name);COPY(previous_steps);COPY(previous_start);COPY(previous_cap);
    COPY(previous_offset);COPY(held);COPY(curves);COPY(curve_lane);COPY(curve_clip);COPY(count);
#undef COPY
    fwrite(&original,sizeof original,1,file);
}
static FILE *commands;
static void key_velocity(void *ctx,int slot,int channel,int pitch,int velocity){(void)ctx;fprintf(commands,"v %d %d %d %d\n",slot,channel,pitch,velocity);}
static void key(void *ctx,int slot,int channel,int pitch,int down){(void)ctx;fprintf(commands,"k %d %d %d %d\n",slot,channel,pitch,down);}
int main(int argc,char **argv) {
    if(argc!=2)return 1;int mode=atoi(argv[1]);if(mode<0 || mode>7)return 1;
    midi_sequence_project(&project);state.project=&project;state.take=&take;
    state.key_velocity=key_velocity;state.key=key;state.record_mask=7;state.typing_keys=1;
    state.midi_learn_target=(ParameterTarget){PARAM_CHANNEL_PAN,1,0};state.midi_learning=mode==2;
    if(mode==3 || mode==4)if(!midi_take_begin(&take,&project,0,mode==3,0,1))return 3;
    if(mode==7){state.recording_audio=1;state.midi_record_target=2;}
    commands=fopen("commands.txt","wb");FILE *trace=fopen("state.bin","wb");if(!commands || !trace)return 2;
    AppInput input;input_init(&input);int finish=0;
    for(int frame=0;frame<midi_sequence_count(mode);frame++) {
        MidiEvent e=midi_sequence_event(mode,frame);
        if(mode==1)state.channel=frame%3;
        if(mode==6 && frame>=2){state.focused_editor=2;state.piano_channel=5;}
        if(e.status)midi_session_event(&state,e);
        if(mode==5){input_begin(&input,frame*1000000);input.focused=frame!=7;state.channel=frame>=2?1:0;
            const int codes[]={90,83,81,50,91,61,93};
            for(unsigned i=0;i<sizeof codes/sizeof *codes;i++){input.keys[codes[i]]=frame<5;input.previous_keys[codes[i]]=frame>0 && frame<=5;}
            midi_session_typing(&state,&input,frame==3,frame==8);}
        if(frame==midi_sequence_count(mode)-1)midi_session_panic(&state,e.time);
        if(midi_session_update(&state,e.time,frame<10,mode==4?&project.master:NULL))finish++;
        int ints[]={state.midi_learning,finish};fwrite(ints,sizeof ints,1,trace);
        fwrite(state.midi_keys,sizeof state.midi_keys,1,trace);fwrite(state.midi_sustain,sizeof state.midi_sustain,1,trace);fwrite(state.keyboard_notes,sizeof state.keyboard_notes,1,trace);fwrite(state.status,sizeof state.status,1,trace);capture_take(trace);midi_sequence_values(trace,&project);
    }
    fclose(trace);fclose(commands);return project_save("expected.llp",&project)?0:2;
}
