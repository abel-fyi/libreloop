// SPDX-License-Identifier: GPL-3.0-only
/* Frozen app_midi.c routing and typing branches; native device management is
   separate. Audio commands run on the single UI producer through callbacks. */
#include "midi_session.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
int midi_session_target(const MidiSession *s) { return s->take->active?s->take->channel:s->recording_audio?s->midi_record_target:s->focused_editor==2?s->piano_channel:s->focused_editor==4?s->instrument_channel:s->channel; }

void midi_session_panic(MidiSession *s,double time) {
    for(int i=0;i<32;i++)if(s->midi_keys[i].used) {s->key_velocity(s->context,64+i,s->midi_keys[i].target,s->midi_keys[i].pitch,0);if(s->keyboard_notes[s->midi_keys[i].target][s->midi_keys[i].pitch])s->keyboard_notes[s->midi_keys[i].target][s->midi_keys[i].pitch]--;if(s->take->active && (s->record_mask&MIDI_RECORD_NOTES))midi_take_note(s->take,s->project,s->midi_keys[i].mchannel,s->midi_keys[i].pitch,0,time);s->midi_keys[i].used=0;}
    memset(s->midi_sustain,0,sizeof s->midi_sustain);
}

void midi_session_release(MidiSession *s,int slot,double time) {
    if(!s->midi_keys[slot].used)return;
    int target=s->midi_keys[slot].target,pitch=s->midi_keys[slot].pitch;
    s->key_velocity(s->context,64+slot,target,pitch,0);if(s->keyboard_notes[target][pitch])s->keyboard_notes[target][pitch]--;
    if(s->take->active && (s->record_mask&MIDI_RECORD_NOTES))midi_take_note(s->take,s->project,s->midi_keys[slot].mchannel,pitch,0,time);
    s->midi_keys[slot].used=0;
}

void midi_session_control(MidiSession *s,ParameterTarget target,float normalized,double time) {
    float value,lo,hi;if(!parameter_info(s->project,target,&value,&lo,&hi))return;
    const ParameterDescriptor *d=parameter_descriptor(target.parameter);
    if(target.parameter==PARAM_MASTER_VOLUME || target.parameter==PARAM_INSERT_VOLUME)normalized=fader_gain(normalized)/hi;
    if(d && d->kind==PARAMETER_LOGARITHMIC && lo>0)normalized=(lo*powf(hi/lo,normalized)-lo)/(hi-lo);
    if(target.parameter>=PARAM_DX7_FIRST && target.parameter<PARAM_DX7_FIRST+126 &&
       (target.parameter-PARAM_DX7_FIRST)%21==18 && s->project->fm[target.owner].dx7.value[target.parameter-PARAM_DX7_FIRST-1])normalized=roundf(normalized*3)/31;
    if(s->take->active && (s->record_mask&MIDI_RECORD_AUTOMATION))midi_take_control(s->take,s->project,target,normalized,time);
    parameter_write(s->project,target,normalized);
}

void midi_session_event(MidiSession *s,MidiEvent e) {
    do {
        int type=e.status&0xf0,mchannel=e.status&15;
        if(type==0x90 || type==0x80) {
            int slot=-1;for(int i=0;i<32;i++)if(s->midi_keys[i].used && s->midi_keys[i].mchannel==mchannel && s->midi_keys[i].pitch==e.data1){slot=i;break;}
            if(type==0x80 || !e.data2) {if(slot>=0){s->midi_keys[slot].pressed=0;if(!s->midi_sustain[mchannel])midi_session_release(s,slot,e.time);}continue;}
            if(slot>=0)midi_session_release(s,slot,e.time);
            if(slot<0)for(int i=0;i<32;i++)if(!s->midi_keys[i].used){slot=i;break;}
            if(slot<0){slot=0;midi_session_release(s,slot,e.time);}
            int target=midi_session_target(s);if(target<0 || target>=s->project->channel_count)continue;
            s->midi_keys[slot]=(MidiHeld){1,1,mchannel,e.data1,target,e.data2};
            s->key_velocity(s->context,64+slot,target,e.data1,e.data2);s->keyboard_notes[target][e.data1]++;
            if(s->take->active && (s->record_mask&MIDI_RECORD_NOTES))midi_take_note(s->take,s->project,mchannel,e.data1,e.data2,e.time);
        } else if(type==0xb0) {
            if(e.data1>=120){for(int i=0;i<32;i++)if(s->midi_keys[i].used && s->midi_keys[i].mchannel==mchannel)midi_session_release(s,i,e.time);s->midi_sustain[mchannel]=0;continue;}
            if(e.data1==64){s->midi_sustain[mchannel]=e.data2>=64;if(!s->midi_sustain[mchannel])for(int i=0;i<32;i++)if(s->midi_keys[i].used && s->midi_keys[i].mchannel==mchannel && !s->midi_keys[i].pressed)midi_session_release(s,i,e.time);}
            if(s->midi_learning){int ok=midi_bind(s->project,mchannel,e.data1,s->midi_learn_target);s->midi_learning=0;snprintf(s->status,sizeof s->status,ok?"MIDI controller %d linked; mapping saved with project":"MIDI link could not be added",e.data1);}
            for(int i=0;i<s->project->midi_binding_count;i++){MidiBinding b=s->project->midi_bindings[i];if(b.channel==(unsigned)mchannel && b.controller==e.data1)midi_session_control(s,b.target,e.data2/127.f,e.time);}
        } else if(type==0xe0) {
            int target=midi_session_target(s);int value=e.data1|(e.data2<<7);
            float pitch=value>=8192?(value-8192)/8191.f:(value-8192)/8192.f;
            midi_session_control(s,(ParameterTarget){PARAM_CHANNEL_PITCH,target,0},(pitch+1)*.5f,e.time);
        }
    } while(0);
}
int midi_session_update(MidiSession *s,double time,int playing,float *control_drag) {
    if(s->take->active) {
        s->project->bpm=s->take->bpm;midi_take_update(s->take,s->project,time);
        if((s->record_mask&MIDI_RECORD_AUTOMATION) && control_drag){ParameterTarget target;float value,lo,hi;
            if(parameter_from_pointer(s->project,control_drag,&target) && parameter_info(s->project,target,&value,&lo,&hi))midi_take_control(s->take,s->project,target,(value-lo)/(hi-lo),time);}
        if(s->take->failed || !playing)return 1;
    }
    return 0;
}
void midi_session_typing(MidiSession *s,const AppInput *input,int blocked,int command) {
    /* GLFW key tokens describe physical US positions, including Spanish - and +. */
    static const int keys[]={'Z','S','X','D','C','V','G','B','H','N','J','M',',','L','.',';','/',
        'Q','2','W','3','E','R','5','T','6','Y','7','U','I','9','O','0','P','[','=',']'};
    _Static_assert(sizeof keys/sizeof *keys==sizeof s->typing_held,"Original physical typing map and held state must agree");
    unsigned char *held=s->typing_held;
    int enabled=s->typing_keys && !blocked && !s->browser_focus && input->focused && !command && !input_key_down(input,APP_KEY_LEFT_ALT) && !input_key_down(input,APP_KEY_RIGHT_ALT);
    for(unsigned i=0;i<sizeof keys/sizeof *keys;i++) {
        int pitch=i<17?48+i:60+i-17;
        if(held[i] && (!enabled || !input_key_down(input,keys[i]))) {
            if(s->keyboard_notes[held[i]-1][pitch]) s->keyboard_notes[held[i]-1][pitch]--;
            s->key(s->context,i,s->channel,pitch,0); held[i]=0;
        }
        if(enabled && !held[i] && input_key_pressed(input,keys[i])) {
            s->key(s->context,i,s->channel,pitch,1); held[i]=s->channel+1; s->keyboard_notes[s->channel][pitch]++;
        }
    }
}

/* Production typing path records into independent held slots. The original
   audition-only entry point above remains available to frozen replay fixtures. */
static int typing_pitch(int i){return i<17?48+i:60+i-17;}
void midi_session_typing_begin(MidiSession *s,double time) {
    s->typing_recording=1;
    for(int i=0;i<37;i++) {
        s->typing_slots[i]=-1;
        if(s->typing_held[i] && s->typing_held[i]-1==s->take->channel && (s->record_mask&MIDI_RECORD_NOTES))
            midi_take_key_note(s->take,s->project,&s->typing_slots[i],typing_pitch(i),100,time);
    }
}
void midi_session_typing_end(MidiSession *s,double time) {
    if(!s->typing_recording)return;
    for(int i=0;i<37;i++)if(s->typing_slots[i]>=0)midi_take_key_note(s->take,s->project,&s->typing_slots[i],typing_pitch(i),0,time);
    s->typing_recording=0;
}
void midi_session_typing_at(MidiSession *s,const AppInput *input,int blocked,int command,double time) {
    static const int keys[]={'Z','S','X','D','C','V','G','B','H','N','J','M',',','L','.',';','/',
        'Q','2','W','3','E','R','5','T','6','Y','7','U','I','9','O','0','P','[','=',']'};
    int enabled=s->typing_keys && !blocked && !s->browser_focus && input->focused && !command && !input_key_down(input,APP_KEY_LEFT_ALT) && !input_key_down(input,APP_KEY_RIGHT_ALT);
    int record=s->typing_recording && s->take->active && (s->record_mask&MIDI_RECORD_NOTES);
    for(int i=0;i<37;i++) {
        int pitch=typing_pitch(i),target=s->typing_held[i]-1;
        if(target>=0 && (!enabled || !input_key_down(input,keys[i]))) {
            if(s->keyboard_notes[target][pitch])s->keyboard_notes[target][pitch]--;
            s->key(s->context,i,target,pitch,0);s->typing_held[i]=0;
            if(record && s->typing_slots[i]>=0)midi_take_key_note(s->take,s->project,&s->typing_slots[i],pitch,0,time);
        }
        if(enabled && !s->typing_held[i] && input_key_pressed(input,keys[i])) {
            target=midi_session_target(s);if(target<0 || target>=s->project->channel_count)continue;
            s->key(s->context,i,target,pitch,1);s->typing_held[i]=target+1;s->keyboard_notes[target][pitch]++;
            if(record)midi_take_key_note(s->take,s->project,&s->typing_slots[i],pitch,100,time);
        }
        if(record && s->typing_slots[i]>=0)midi_take_key_note(s->take,s->project,&s->typing_slots[i],pitch,-1,time);
    }
}
