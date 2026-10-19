// SPDX-License-Identifier: GPL-3.0-only
#include "file_io.h"
#include "engine.h"
#include "atomic_file.h"
#include "project_format.h"
#include "playback_plan.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "portable.h"
#include <stddef.h>
#include <errno.h>
#include <float.h>
#include <ctype.h>

static int project_valid(const Project *p) {
    for(int c=0;c<CHANNELS;c++)if(!voice_settings_valid(p->voice[c]) || !arp_valid(p->arp[c]))return 0;
    if(!isfinite(p->bpm) || p->bpm<30 || p->bpm>300 || p->pattern_count<1 || p->pattern_count>PATTERNS) return 0;
    if(!isfinite(p->master_pan) || fabsf(p->master_pan)>1 || !isfinite(p->master_pitch) || fabsf(p->master_pitch)>12) return 0;
    if(!automation_valid(p) || p->midi_binding_count<0 || p->midi_binding_count>MIDI_BINDINGS) return 0;
    for(int i=0;i<p->midi_binding_count;i++){MidiBinding b=p->midi_bindings[i];if(b.channel>15 || b.controller>119 || !parameter_descriptor(b.target.parameter) || b.target.owner>INSERTS || b.target.slot>=EFFECT_SLOTS)return 0;}
    for(int c=0;c<CHANNELS;c++) if(!instrument_descriptor(p->instrument[c]) || !device_descriptor(DEVICE_FM)->valid(&p->fm[c]) || (p->instrument[c]==INSTRUMENT_FM && p->channel_audio[c])) return 0;
    for(int bus=0;bus<=INSERTS;bus++) for(int slot=0;slot<EFFECT_SLOTS;slot++)
        if((p->effect_type[bus][slot]!=EFFECT_EMPTY && !effect_descriptor(p->effect_type[bus][slot])) || !device_descriptor(DEVICE_CHORUS)->valid(&p->chorus[bus][slot]) || !device_descriptor(DEVICE_EQ)->valid(&p->eq[bus][slot]) || !delay_valid(p->delay[bus][slot]) || !tube_valid(p->tube[bus][slot]) || !reverb_valid(p->reverb[bus][slot]) || !dynamics_valid(p->dynamics[bus][slot]) || !flanger_valid(p->flanger[bus][slot]) || !phaser_valid(p->phaser[bus][slot])) return 0;
    if(p->channel_count<0 || p->channel_count>CHANNELS || p->insert_count<0 || p->insert_count>INSERTS) return 0;
    for(int c=0;c<CHANNELS;c++) if(!p->channel_names[c][0] || strchr(p->channel_names[c],'\n') || strchr(p->channel_names[c],'\r')) return 0;
    for(int c=0;c<CHANNELS;c++) if(strchr(p->paths[c],'\n') || strchr(p->paths[c],'\r')) return 0;
    for(int pat=0;pat<PATTERNS;pat++) if(!p->pattern_names[pat][0] || strchr(p->pattern_names[pat],'\n') || strchr(p->pattern_names[pat],'\r')) return 0;
    for(int c=0;c<CHANNELS;c++) if(!device_descriptor(DEVICE_SAMPLER)->valid(&p->sampler[c])) return 0;
    if(!isfinite(p->master_width) || p->master_width<0 || p->master_width>2 || p->master_mute>1) return 0;
    for(int i=0;i<INSERTS;i++) if(!isfinite(p->insert_width[i]) || p->insert_width[i]<0 || p->insert_width[i]>2) return 0;
    for(int c=0;c<CHANNELS;c++) if(p->mute[c]>3 || !isfinite(p->volume[c]) || p->volume[c]<0 || p->volume[c]>VOLUME_KNOB_MAX) return 0;
    for(int l=0;l<LANES;l++) if(p->lane_mute[l]>3) return 0;
    if(!isfinite(p->swing) || p->swing<0 || p->swing>1) return 0;
    if(!isfinite(p->master) || p->master<0 || p->master>MIXER_GAIN_MAX) return 0;
    for(int i=0;i<INSERTS;i++) if(!isfinite(p->insert_volume[i]) || p->insert_volume[i]<0 || p->insert_volume[i]>MIXER_GAIN_MAX) return 0;
    for(int id=0;id<=INSERTS;id++) for(int io=0;io<2;io++) if(ll_strnlen(p->audio_io[id][io],128)==128 || strchr(p->audio_io[id][io],'\n') || strchr(p->audio_io[id][io],'\r')) return 0;
    for(int id=0;id<=INSERTS;id++) for(int slot=0;slot<10;slot++) if(!isfinite(p->effect_mix[id][slot]) || p->effect_mix[id][slot]<0 || p->effect_mix[id][slot]>1 || p->effect_bypass[id][slot]>1) return 0;
    for(int l=0;l<LANES;l++) if(!p->track_names[l][0] || ll_strnlen(p->track_names[l],PATTERN_NAME)==PATTERN_NAME || strchr(p->track_names[l],'\n') || strchr(p->track_names[l],'\r')) return 0;
    for(int i=0;i<INSERTS;i++) if(!p->insert_names[i][0] || ll_strnlen(p->insert_names[i],PATTERN_NAME)==PATTERN_NAME || strchr(p->insert_names[i],'\n') || strchr(p->insert_names[i],'\r')) return 0;
    for(int i=0;i<PATTERNS;i++) if(p->pattern_colors[i]>0xffffff) return 0;
    for(int c=0;c<CHANNELS;c++) if(p->channel_colors[c]>0xffffff) return 0;
    for(int c=0;c<CHANNELS;c++) if(p->channel_audio[c]>1 || !isfinite(p->audio_seconds[c]) || p->audio_seconds[c]<0 || p->audio_seconds[c]>SAMPLE_MAX_FRAMES*4.0/RATE) return 0;
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(p->clips[l][b]>PATTERNS && p->clips[l][b]<=AUTOMATION_SOURCE) {
        int c=p->clips[l][b]-PATTERNS-1;
        if(!channel_has_audio(p,c)) return 0;
    }
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(p->clips[l][b]>AUTOMATION_SOURCE && p->clips[l][b]>AUTOMATION_SOURCE+p->automation_count) return 0;
    for(int c=0;c<CHANNELS;c++) if(!isfinite(p->channel_pitch[c]) || fabsf(p->channel_pitch[c])>1 || !isfinite(p->pitch_range[c]) || p->pitch_range[c]<1 || p->pitch_range[c]>48 || p->pitch_range[c]!=roundf(p->pitch_range[c])) return 0;
    for(int c=0;c<CHANNELS;c++) if(!isfinite(p->pan[c]) || fabsf(p->pan[c])>1 || p->route[c]>p->insert_count) return 0;
    for(int i=0;i<INSERTS;i++) if(!isfinite(p->insert_pan[i]) || fabsf(p->insert_pan[i])>1 || p->insert_mute[i]>3 || (p->insert_output[i]>p->insert_count && p->insert_output[i]!=255)) return 0;
    if(!insert_routes_valid(p))return 0;
    for(int pat=0;pat<PATTERNS;pat++) {
        if(!isfinite(p->pattern_steps[pat]) || p->pattern_steps[pat]<STEPS || p->pattern_steps[pat]>1e15f) return 0;
        for(int c=0;c<CHANNELS;c++) for(int n=0;n<NOTES;n++) {
            Note v=p->notes[pat][c][n];
            if(v.pitch>127 || v.velocity>127 || !isfinite(v.start) || !isfinite(v.length) || v.start<0 || v.start>=1e15f || v.length<0 || v.length>1e15f-v.start) return 0;
        }
    }
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) {
        unsigned source=p->clips[l][b];
        if(source>SOURCES || (source && source<=PATTERNS && source>(unsigned)p->pattern_count)) return 0;
        if(!isfinite(p->clip_starts[l][b]) || p->clip_starts[l][b]<0 || p->clip_starts[l][b]>=1e15f ||
           !isfinite(p->clip_steps[l][b]) || p->clip_steps[l][b]<0 || p->clip_steps[l][b]>1e15f ||
           !isfinite(p->clip_offsets[l][b]) || p->clip_offsets[l][b]<0 || p->clip_offsets[l][b]>1e15f) return 0;
    }
    return 1;
}

_Static_assert(sizeof(float)==4 && sizeof(int)==4 && sizeof(unsigned)==4,
               "LLP scalar mappings require 32-bit float/int/unsigned");
typedef enum { F32,I32,U32,U8,STRING } FieldType;
typedef struct { const char *name; FieldType type; size_t offset,count,inner,outer_stride,inner_stride,width; } Field;
#define FLAT(m,t,k) {#m,k,offsetof(Project,m),sizeof(((Project *)0)->m)/sizeof(t),1,sizeof(t),0,sizeof(t)}
#define SCALAR(m,t,k) {#m,k,offsetof(Project,m),1,1,sizeof(t),0,sizeof(t)}
#define TEXT(m,n,w) {#m,STRING,offsetof(Project,m),n,1,w,0,w}
#define MEMBER(m,t,f,k,n) {#m "." #f,k,offsetof(Project,m)+offsetof(t,f),n,1,sizeof(t),0,sizeof(((t *)0)->f)}
#define SUB(m,t,s,u,f,k,n) {#m "." #s "." #f,k,offsetof(Project,m)+offsetof(t,s)+offsetof(u,f),n,1,sizeof(t),0,sizeof(((u *)0)->f)}
#define ANALOG_NEW(member,name) {name,F32,offsetof(Project,fm)+offsetof(FMSettings,analog)+offsetof(AnalogSettings,member),CHANNELS,1,sizeof(FMSettings),0,sizeof(float)}
#define ANALOG_ARRAY(member,name) {name,F32,offsetof(Project,fm)+offsetof(FMSettings,analog)+offsetof(AnalogSettings,member),CHANNELS*3,3,sizeof(FMSettings),sizeof(float),sizeof(float)}
/* Stable file schema; offsets/strides map logical values without saving padding. */
static const Field fields[]={
    SCALAR(bpm,float,F32),
    SCALAR(master,float,F32),
    SCALAR(master_pitch,float,F32),
    SCALAR(master_width,float,F32),
    {"x-master-pan",F32,offsetof(Project,master_pan),1,1,sizeof(float),0,sizeof(float)},
    SCALAR(swing,float,F32),
    SCALAR(pattern_count,int,I32),
    SCALAR(channel_count,int,I32),
    SCALAR(insert_count,int,I32),
    SCALAR(automation_count,int,I32),
    SCALAR(midi_binding_count,int,I32),
    SCALAR(master_mute,uint8_t,U8),
    FLAT(volume,float,F32),
    FLAT(pan,float,F32),
    FLAT(channel_pitch,float,F32),
    FLAT(pitch_range,float,F32),
    {"x-voice.limit",F32,offsetof(Project,voice)+offsetof(VoiceSettings,limit),CHANNELS,1,sizeof(VoiceSettings),0,sizeof(float)},
    {"x-voice.mono",F32,offsetof(Project,voice)+offsetof(VoiceSettings,mono),CHANNELS,1,sizeof(VoiceSettings),0,sizeof(float)},
    {"x-voice.legato",F32,offsetof(Project,voice)+offsetof(VoiceSettings,legato),CHANNELS,1,sizeof(VoiceSettings),0,sizeof(float)},
    {"x-voice.glide",F32,offsetof(Project,voice)+offsetof(VoiceSettings,glide),CHANNELS,1,sizeof(VoiceSettings),0,sizeof(float)},
    {"x-arp.direction",F32,offsetof(Project,arp)+offsetof(ArpSettings,direction),CHANNELS,1,sizeof(ArpSettings),0,sizeof(float)},
    {"x-arp.time",F32,offsetof(Project,arp)+offsetof(ArpSettings,time),CHANNELS,1,sizeof(ArpSettings),0,sizeof(float)},
    {"x-arp.gate",F32,offsetof(Project,arp)+offsetof(ArpSettings,gate),CHANNELS,1,sizeof(ArpSettings),0,sizeof(float)},
    {"x-arp.range",F32,offsetof(Project,arp)+offsetof(ArpSettings,range),CHANNELS,1,sizeof(ArpSettings),0,sizeof(float)},
    {"x-arp.repeat",F32,offsetof(Project,arp)+offsetof(ArpSettings,repeat),CHANNELS,1,sizeof(ArpSettings),0,sizeof(float)},
    {"x-arp.chord",F32,offsetof(Project,arp)+offsetof(ArpSettings,chord),CHANNELS,1,sizeof(ArpSettings),0,sizeof(float)},
    {"x-arp.slide",F32,offsetof(Project,arp)+offsetof(ArpSettings,slide),CHANNELS,1,sizeof(ArpSettings),0,sizeof(float)},
    FLAT(clip_steps,float,F32),
    FLAT(clip_starts,float,F32),
    FLAT(clip_offsets,float,F32),
    FLAT(audio_seconds,float,F32),
    FLAT(pattern_steps,float,F32),
    FLAT(insert_volume,float,F32),
    FLAT(insert_pan,float,F32),
    FLAT(insert_width,float,F32),
    FLAT(effect_mix,float,F32),
    FLAT(mute,uint8_t,U8),
    FLAT(clips,uint8_t,U8),
    FLAT(channel_audio,uint8_t,U8),
    FLAT(route,uint8_t,U8),
    FLAT(insert_mute,uint8_t,U8),
    FLAT(insert_output,uint8_t,U8),
    {"x-insert-sends",U8,offsetof(Project,insert_sends),INSERTS*(INSERTS+1),1,1,0,1},
    FLAT(effect_type,uint8_t,U8),
    FLAT(effect_bypass,uint8_t,U8),
    FLAT(lane_mute,uint8_t,U8),
    FLAT(instrument,uint8_t,U8),
    FLAT(pattern_colors,uint32_t,U32),
    FLAT(channel_colors,uint32_t,U32),
    TEXT(paths,CHANNELS,1024),
    TEXT(channel_names,CHANNELS,PATTERN_NAME),
    TEXT(pattern_names,PATTERNS,PATTERN_NAME),
    TEXT(audio_io,(INSERTS+1)*2,128),
    TEXT(track_names,LANES,PATTERN_NAME),
    TEXT(insert_names,INSERTS,PATTERN_NAME),
    MEMBER(notes,Note,pitch,U8,PATTERNS*CHANNELS*NOTES),
    MEMBER(notes,Note,velocity,U8,PATTERNS*CHANNELS*NOTES),
    MEMBER(notes,Note,start,F32,PATTERNS*CHANNELS*NOTES),
    MEMBER(notes,Note,length,F32,PATTERNS*CHANNELS*NOTES),
    MEMBER(sampler,Sampler,pitch,F32,CHANNELS),
    MEMBER(sampler,Sampler,time,F32,CHANNELS),
    MEMBER(sampler,Sampler,start,F32,CHANNELS),
    MEMBER(sampler,Sampler,length,F32,CHANNELS),
    MEMBER(sampler,Sampler,trim,F32,CHANNELS),
    MEMBER(sampler,Sampler,fit_bpm,F32,CHANNELS),
    MEMBER(sampler,Sampler,flags,U8,CHANNELS),
    MEMBER(sampler,Sampler,stretch,U8,CHANNELS),
    MEMBER(fm,FMSettings,ratio,F32,CHANNELS),
    MEMBER(fm,FMSettings,depth,F32,CHANNELS),
    MEMBER(fm,FMSettings,attack,F32,CHANNELS),
    MEMBER(fm,FMSettings,decay,F32,CHANNELS),
    MEMBER(fm,FMSettings,sustain,F32,CHANNELS),
    MEMBER(fm,FMSettings,release,F32,CHANNELS),
    MEMBER(fm,FMSettings,mod_decay,F32,CHANNELS),
    MEMBER(fm,FMSettings,mod_sustain,F32,CHANNELS),
    MEMBER(fm,FMSettings,velocity,F32,CHANNELS),
    MEMBER(fm,FMSettings,lfo_rate,F32,CHANNELS),
    MEMBER(fm,FMSettings,vibrato,F32,CHANNELS),
    MEMBER(fm,FMSettings,tremolo,F32,CHANNELS),
    MEMBER(fm,FMSettings,carrier_ratio,F32,CHANNELS),
    MEMBER(fm,FMSettings,carrier_detune,F32,CHANNELS),
    MEMBER(fm,FMSettings,body_detune,F32,CHANNELS),
    MEMBER(fm,FMSettings,mod_attack,F32,CHANNELS),
    MEMBER(fm,FMSettings,mod_release,F32,CHANNELS),
    MEMBER(fm,FMSettings,attack_ratio,F32,CHANNELS),
    MEMBER(fm,FMSettings,attack_detune,F32,CHANNELS),
    MEMBER(fm,FMSettings,attack_depth,F32,CHANNELS),
    MEMBER(fm,FMSettings,attack_attack,F32,CHANNELS),
    MEMBER(fm,FMSettings,attack_decay,F32,CHANNELS),
    MEMBER(fm,FMSettings,attack_sustain,F32,CHANNELS),
    MEMBER(fm,FMSettings,attack_release,F32,CHANNELS),
    MEMBER(fm,FMSettings,routing,F32,CHANNELS),
    MEMBER(fm,FMSettings,lfo_shape,F32,CHANNELS),
    MEMBER(fm,FMSettings,lfo_fade,F32,CHANNELS),
    MEMBER(fm,FMSettings,body_pitch,F32,CHANNELS),
    MEMBER(fm,FMSettings,engine,F32,CHANNELS),
    ANALOG_NEW(model,"x-analog.model"),
    ANALOG_ARRAY(osc_wave,"x-analog.wave"),ANALOG_ARRAY(osc_tune,"x-analog.tune"),
    ANALOG_ARRAY(osc_fine,"x-analog.fine"),ANALOG_ARRAY(osc_level,"x-analog.level"),
    ANALOG_NEW(drive,"x-analog.drive"),ANALOG_NEW(drift,"x-analog.drift"),ANALOG_NEW(keytrack,"x-analog.keytrack"),
    SUB(fm,FMSettings,analog,AnalogSettings,wave,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,detune,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,mix,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,sub,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,noise,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,pulse,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,pwm,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,cutoff,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,resonance,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,filter_env,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,chorus,F32,CHANNELS),
    {"fm.dx7.value",F32,offsetof(Project,fm)+offsetof(FMSettings,dx7)+offsetof(DX7Settings,value),CHANNELS*DX7_PARAMETERS,DX7_PARAMETERS,sizeof(FMSettings),sizeof(float),sizeof(float)},
    {"x-phaser.settings",F32,offsetof(Project,phaser),(INSERTS+1)*EFFECT_SLOTS*6,6,sizeof(PhaserSettings),sizeof(float),sizeof(float)},
    {"x-flanger.settings",F32,offsetof(Project,flanger),(INSERTS+1)*EFFECT_SLOTS*6,6,sizeof(FlangerSettings),sizeof(float),sizeof(float)},
    {"x-dynamics.settings",F32,offsetof(Project,dynamics),(INSERTS+1)*EFFECT_SLOTS*(sizeof(DynamicsSettings)/sizeof(float)),sizeof(DynamicsSettings)/sizeof(float),sizeof(DynamicsSettings),sizeof(float),sizeof(float)},
    {"x-reverb.size",F32,offsetof(Project,reverb)+offsetof(ReverbSettings,size),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(ReverbSettings),0,sizeof(float)},
    {"x-reverb.decay",F32,offsetof(Project,reverb)+offsetof(ReverbSettings,decay),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(ReverbSettings),0,sizeof(float)},
    {"x-reverb.damping",F32,offsetof(Project,reverb)+offsetof(ReverbSettings,damping),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(ReverbSettings),0,sizeof(float)},
    {"x-reverb.predelay",F32,offsetof(Project,reverb)+offsetof(ReverbSettings,predelay),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(ReverbSettings),0,sizeof(float)},
    {"x-reverb.width",F32,offsetof(Project,reverb)+offsetof(ReverbSettings,width),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(ReverbSettings),0,sizeof(float)},
    {"x-reverb.mod",F32,offsetof(Project,reverb)+offsetof(ReverbSettings,mod),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(ReverbSettings),0,sizeof(float)},
    {"x-reverb.gate",F32,offsetof(Project,reverb)+offsetof(ReverbSettings,gate),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(ReverbSettings),0,sizeof(float)},
    {"x-tube.drive",F32,offsetof(Project,tube)+offsetof(TubeSettings,drive),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(TubeSettings),0,sizeof(float)},
    {"x-tube.tone",F32,offsetof(Project,tube)+offsetof(TubeSettings,tone),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(TubeSettings),0,sizeof(float)},
    {"x-tube.output",F32,offsetof(Project,tube)+offsetof(TubeSettings,output),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(TubeSettings),0,sizeof(float)},
    {"x-tube.natural",F32,offsetof(Project,tube)+offsetof(TubeSettings,natural),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(TubeSettings),0,sizeof(float)},
    {"x-delay.input_pan",F32,offsetof(Project,delay)+offsetof(DelaySettings,input_pan),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(DelaySettings),0,sizeof(((DelaySettings *)0)->input_pan)},
    {"x-delay.input_gain",F32,offsetof(Project,delay)+offsetof(DelaySettings,input_gain),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(DelaySettings),0,sizeof(((DelaySettings *)0)->input_gain)},
    {"x-delay.steps",F32,offsetof(Project,delay)+offsetof(DelaySettings,steps),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(DelaySettings),0,sizeof(((DelaySettings *)0)->steps)},
    {"x-delay.offset",F32,offsetof(Project,delay)+offsetof(DelaySettings,offset),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(DelaySettings),0,sizeof(((DelaySettings *)0)->offset)},
    {"x-delay.dry",F32,offsetof(Project,delay)+offsetof(DelaySettings,dry),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(DelaySettings),0,sizeof(((DelaySettings *)0)->dry)},
    {"x-delay.mode",U32,offsetof(Project,delay)+offsetof(DelaySettings,mode),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(DelaySettings),0,sizeof(((DelaySettings *)0)->mode)},
    {"x-delay.model",U32,offsetof(Project,delay)+offsetof(DelaySettings,model),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(DelaySettings),0,sizeof(((DelaySettings *)0)->model)},
    {"x-delay.time_ms",F32,offsetof(Project,delay)+offsetof(DelaySettings,time_ms),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(DelaySettings),0,sizeof(((DelaySettings *)0)->time_ms)},
    {"x-delay.feedback",F32,offsetof(Project,delay)+offsetof(DelaySettings,feedback),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(DelaySettings),0,sizeof(((DelaySettings *)0)->feedback)},
    {"x-delay.cutoff",F32,offsetof(Project,delay)+offsetof(DelaySettings,cutoff),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(DelaySettings),0,sizeof(((DelaySettings *)0)->cutoff)},
    {"x-delay.stereo",F32,offsetof(Project,delay)+offsetof(DelaySettings,stereo),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(DelaySettings),0,sizeof(((DelaySettings *)0)->stereo)},
    {"x-delay.output",F32,offsetof(Project,delay)+offsetof(DelaySettings,output),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(DelaySettings),0,sizeof(((DelaySettings *)0)->output)},
    {"x-delay.sync",U32,offsetof(Project,delay)+offsetof(DelaySettings,sync),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(DelaySettings),0,sizeof(((DelaySettings *)0)->sync)},
    {"x-delay.division",U32,offsetof(Project,delay)+offsetof(DelaySettings,division),(INSERTS+1)*EFFECT_SLOTS,1,sizeof(DelaySettings),0,sizeof(((DelaySettings *)0)->division)},
    MEMBER(chorus,ChorusSettings,rate,F32,(INSERTS+1)*EFFECT_SLOTS),
    MEMBER(chorus,ChorusSettings,depth,F32,(INSERTS+1)*EFFECT_SLOTS),
    {"eq.bands.frequency",F32,offsetof(Project,eq)+offsetof(EQSettings,bands)+offsetof(EQBand,frequency),(INSERTS+1)*EFFECT_SLOTS*EQ_BANDS,EQ_BANDS,sizeof(EQSettings),sizeof(EQBand),sizeof(((EQBand *)0)->frequency)},
    {"eq.bands.gain",F32,offsetof(Project,eq)+offsetof(EQSettings,bands)+offsetof(EQBand,gain),(INSERTS+1)*EFFECT_SLOTS*EQ_BANDS,EQ_BANDS,sizeof(EQSettings),sizeof(EQBand),sizeof(((EQBand *)0)->gain)},
    {"eq.bands.q",F32,offsetof(Project,eq)+offsetof(EQSettings,bands)+offsetof(EQBand,q),(INSERTS+1)*EFFECT_SLOTS*EQ_BANDS,EQ_BANDS,sizeof(EQSettings),sizeof(EQBand),sizeof(((EQBand *)0)->q)},
    {"eq.bands.shape",U32,offsetof(Project,eq)+offsetof(EQSettings,bands)+offsetof(EQBand,shape),(INSERTS+1)*EFFECT_SLOTS*EQ_BANDS,EQ_BANDS,sizeof(EQSettings),sizeof(EQBand),sizeof(((EQBand *)0)->shape)},
    SUB(automations,Automation,target,ParameterTarget,parameter,U32,AUTOMATIONS),
    SUB(automations,Automation,target,ParameterTarget,owner,U32,AUTOMATIONS),
    SUB(automations,Automation,target,ParameterTarget,slot,U32,AUTOMATIONS),
    MEMBER(automations,Automation,steps,F32,AUTOMATIONS),
    MEMBER(automations,Automation,count,I32,AUTOMATIONS),
    MEMBER(automations,Automation,color,U32,AUTOMATIONS),
    {"automations.name",STRING,offsetof(Project,automations)+offsetof(Automation,name),AUTOMATIONS,1,sizeof(Automation),0,48},
    {"automations.points.step",F32,offsetof(Project,automations)+offsetof(Automation,points)+offsetof(AutomationPoint,step),AUTOMATIONS*AUTOMATION_POINTS,AUTOMATION_POINTS,sizeof(Automation),sizeof(AutomationPoint),sizeof(float)},
    {"automations.points.value",F32,offsetof(Project,automations)+offsetof(Automation,points)+offsetof(AutomationPoint,value),AUTOMATIONS*AUTOMATION_POINTS,AUTOMATION_POINTS,sizeof(Automation),sizeof(AutomationPoint),sizeof(float)},
    MEMBER(midi_bindings,MidiBinding,channel,U32,MIDI_BINDINGS),
    MEMBER(midi_bindings,MidiBinding,controller,U32,MIDI_BINDINGS),
    SUB(midi_bindings,MidiBinding,target,ParameterTarget,parameter,U32,MIDI_BINDINGS),
    SUB(midi_bindings,MidiBinding,target,ParameterTarget,owner,U32,MIDI_BINDINGS),
    SUB(midi_bindings,MidiBinding,target,ParameterTarget,slot,U32,MIDI_BINDINGS),
};
#undef FLAT
#undef SCALAR
#undef TEXT
#undef MEMBER
#undef SUB
#define FIELD_COUNT (sizeof fields/sizeof *fields)
#define MAX_FIELDS (FIELD_COUNT+64)
static const char *type_names[]={"f32","i32","u32","u8","str"};
static unsigned char *address(Project *p,const Field *f,size_t i) {
    return (unsigned char *)p+f->offset+(i/f->inner)*f->outer_stride+(i%f->inner)*f->inner_stride;
}
static int structure_valid(const Project *p) {
    if(!p) return 0;
    for(size_t f=0;f<FIELD_COUNT;f++)
        for(size_t i=0;i<fields[f].count;i++) {
            const char *s=(const char *)address((Project *)p,&fields[f],i);
            if(fields[f].type==STRING && (!memchr(s,0,fields[f].width) || strchr(s,'\n') || strchr(s,'\r'))) return 0;
            if(fields[f].type==F32) { float v; memcpy(&v,s,sizeof v); if(!isfinite(v)) return 0; }
        }
    return project_valid(p);
}
static int equal_value(const unsigned char *a,const unsigned char *b,const Field *f) {
    return f->type==STRING?!strcmp((const char *)a,(const char *)b):!memcmp(a,b,f->width);
}
/* Version 1 reserves eight pattern IDs; preserve its compact encoding whenever
   the extended pattern slots still have their default state. */
static size_t pattern_field_count(const Field *f,int patterns){
    if(!strcmp(f->name,"pattern_steps") || !strcmp(f->name,"pattern_names") || !strcmp(f->name,"pattern_colors"))return patterns;
    if(!strncmp(f->name,"notes.",6))return (size_t)patterns*CHANNELS*NOTES;
    return f->count;
}
static int legacy_patterns(const Project *p){
    if(p->pattern_count>LEGACY_PATTERNS)return 0;
    for(int pat=LEGACY_PATTERNS;pat<PATTERNS;pat++){
        char name[PATTERN_NAME];snprintf(name,sizeof name,"Pattern %d",pat+1);
        if(p->pattern_steps[pat]!=STEPS || p->pattern_colors[pat]!=pattern_palette[pat%COLOR_COUNT] || strcmp(p->pattern_names[pat],name))return 0;
        for(int c=0;c<CHANNELS;c++)for(int n=0;n<NOTES;n++){Note v=p->notes[pat][c][n];if(v.pitch || v.velocity || v.start || v.length)return 0;}
    }
    return 1;
}
int project_save(const char *path,const Project *p) {
    if(!structure_valid(p)) return 0;
    AtomicFile out; if(!atomic_file_open(&out,path)) return 0;
    FILE *file=out.file;
    int version=legacy_patterns(p)?1:PROJECT_FORMAT_VERSION;
    fprintf(file,PROJECT_FORMAT_MAGIC " %d\n",version);
    for(size_t n=0;n<FIELD_COUNT;n++) {
        Field stored=fields[n];if(version==1)stored.count=pattern_field_count(&stored,LEGACY_PATTERNS);
        const Field *f=&stored;
        if(!strncmp(f->name,"x-analog.",9) || !strncmp(f->name,"x-voice.",8)){
            int any=0;for(size_t i=0;i<f->count;i++){float value;memcpy(&value,address((Project *)p,f,i),sizeof value);any|=value!=0;}
            if(!any)continue;
        }
        if(!strncmp(f->name,"x-arp.",6)){ArpSettings defaults=arp_default();int any=0;for(int c=0;c<CHANNELS;c++)any|=memcmp(&p->arp[c],&defaults,sizeof defaults)!=0;if(!any)continue;}
        if(!strcmp(f->name,"x-master-pan") && p->master_pan==0)continue;
        if(!strncmp(f->name,"x-phaser.",9)){int any=0;for(int bus=0;bus<=INSERTS;bus++)for(int slot=0;slot<EFFECT_SLOTS;slot++)any|=p->effect_type[bus][slot]==EFFECT_PHASER;if(!any)continue;}
        if(!strncmp(f->name,"x-flanger.",10)){int any=0;for(int bus=0;bus<=INSERTS;bus++)for(int slot=0;slot<EFFECT_SLOTS;slot++)any|=p->effect_type[bus][slot]==EFFECT_FLANGER;if(!any)continue;}
        if(!strncmp(f->name,"x-dynamics.",11)){int any=0;for(int bus=0;bus<=INSERTS;bus++)for(int slot=0;slot<EFFECT_SLOTS;slot++)any|=p->effect_type[bus][slot]==EFFECT_DYNAMICS;if(!any)continue;}
        if(!strncmp(f->name,"x-reverb.",9)) {
            int any=0;for(int bus=0;bus<=INSERTS;bus++)for(int slot=0;slot<EFFECT_SLOTS;slot++)any|=p->effect_type[bus][slot]==EFFECT_REVERB;
            if(!any)continue;
        }
        if(!strncmp(f->name,"x-tube.",7)) {
            int any=0;for(int bus=0;bus<=INSERTS;bus++)for(int slot=0;slot<EFFECT_SLOTS;slot++)any|=p->effect_type[bus][slot]==EFFECT_TUBE;
            if(!any)continue;
        }
        if(!strncmp(f->name,"x-delay.",8)) {
            int any=0;for(int bus=0;bus<=INSERTS;bus++)for(int slot=0;slot<EFFECT_SLOTS;slot++)any|=p->effect_type[bus][slot]==EFFECT_DELAY;
            if(!any)continue;
        }
        if(!strcmp(f->name,"x-insert-sends")) {
            int any=0;for(int source=0;source<INSERTS;source++)for(int dest=0;dest<=INSERTS;dest++)any|=p->insert_sends[source][dest];
            if(!any)continue;
        }
        fprintf(file,"%s %s %zu\n",f->name,type_names[f->type],f->count);
        if(version==1 && !strcmp(f->name,"clips")){
            uint8_t encoded[LANES*CLIPS];
            for(size_t i=0;i<f->count;i++){uint8_t v=((const uint8_t *)p->clips)[i];encoded[i]=v>PATTERNS?v-(PATTERNS-LEGACY_PATTERNS):v;}
            for(size_t i=0;i<f->count;){size_t run=1;while(i+run<f->count && encoded[i+run]==encoded[i])run++;fprintf(file,"%zu %u\n",run,encoded[i]);i+=run;}
            continue;
        }
        for(size_t i=0;i<f->count;) {
            const unsigned char *value=address((Project *)p,f,i); size_t run=1;
            while(i+run<f->count && equal_value(value,address((Project *)p,f,i+run),f)) run++;
            fprintf(file,"%zu ",run);
            if(f->type==STRING) {
                size_t length=strlen((const char *)value);
                fprintf(file,"%zu:",length); fwrite(value,1,length,file); fputc('\n',file);
            } else if(f->type==F32) { float v; memcpy(&v,value,sizeof v); fprintf(file,"%.9g\n",v); }
            else if(f->type==I32) { int v; memcpy(&v,value,sizeof v); fprintf(file,"%d\n",v); }
            else if(f->type==U32) { unsigned v; memcpy(&v,value,sizeof v); fprintf(file,"%u\n",v); }
            else fprintf(file,"%u\n",(unsigned)*value);
            i+=run;
        }
    }
    fputs("end\n",file);
    return atomic_file_commit(&out);
}
static int line(FILE *f,char *out,size_t size) {
    size_t used=0; int ch;
    while((ch=fgetc(f))!=EOF && ch!='\n') {
        if(!ch || used+1>=size) return 0;
        out[used++]=(char)ch;
    }
    if(ch!='\n') return 0;
    if(used && out[used-1]=='\r') used--;
    out[used]=0; return 1;
}
static int blank(const char *s) { while(*s && isspace((unsigned char)*s)) s++; return !*s; }
/* Parse bounded counts without scanf's undefined behavior on integer overflow. */
static int count_token(const char *text,size_t *value,size_t limit) {
    if(!isdigit((unsigned char)*text)) return 0;
    char *end; errno=0; unsigned long long n=strtoull(text,&end,10);
    if(errno || !blank(end) || n>limit) return 0;
    *value=(size_t)n; return 1;
}
static int number_prefix(FILE *file,size_t *value,int delimiter,size_t limit) {
    size_t n=0; int ch=fgetc(file),digits=0;
    while(ch>='0' && ch<='9') {
        unsigned d=ch-'0'; if(n>limit/10 || (n==limit/10 && d>limit%10)) return 0;
        n=n*10+d; digits++; ch=fgetc(file);
    }
    if(!digits || ch!=delimiter) return 0;
    *value=n; return 1;
}
static int eol(FILE *f) { int ch=fgetc(f); return ch=='\n' || (ch=='\r' && fgetc(f)=='\n'); }
static int read_values(FILE *file,Project *p,const Field *field,FieldType type,size_t count) {
    size_t used=0;
    while(used<count) {
        size_t run; if(!number_prefix(file,&run,' ',count-used) || !run) return 0;
        unsigned char value[1024]={0};
        if(type==STRING) {
            size_t length;
            if(!number_prefix(file,&length,':',sizeof value-1) || (field && length>=field->width) ||
               fread(value,1,length,file)!=length || !eol(file) || memchr(value,0,length) ||
               memchr(value,'\n',length) || memchr(value,'\r',length)) return 0;
        } else {
            char text[128],*end; if(!line(file,text,sizeof text) || !text[0]) return 0;
            errno=0;
            if(type==F32) {
                /* Parse directly to binary32: %.9g of FLT_MAX can be a little
                   above its exact double value, while still rounding to it. */
                float v=strtof(text,&end);
                if(end==text || !blank(end) || !isfinite(v) || (errno && (errno!=ERANGE || v==0))) return 0;
                memcpy(value,&v,sizeof v);
            } else if(type==I32) {
                long long v=strtoll(text,&end,10); if(end==text || errno || !blank(end) || v<INT32_MIN || v>INT32_MAX) return 0;
                int i=(int)v; memcpy(value,&i,sizeof i);
            } else {
                if(text[0]=='-') return 0;
                unsigned long long v=strtoull(text,&end,10);
                if(end==text || errno || !blank(end) || v>(type==U8?UINT8_MAX:UINT32_MAX)) return 0;
                if(type==U8) *value=(uint8_t)v; else { unsigned u=(unsigned)v; memcpy(value,&u,sizeof u); }
            }
        }
        if(field) for(size_t i=0;i<run;i++) memcpy(address(p,field,used+i),value,field->width);
        used+=run;
    }
    return 1;
}
int project_load(const char *path,Project *p) {
    FILE *file=platform_file_open(path,"rb"); if(!file) return 0;
    if(fseek(file,0,SEEK_END) || ftell(file)<0 || ftell(file)>32*1024*1024 || fseek(file,0,SEEK_SET)) { fclose(file); return 0; }
    char text[256],magic[64]; size_t version; int consumed=0;
    int ok=p && line(file,text,sizeof text) && sscanf(text,"%63s %n",magic,&consumed)==1 &&
        !strcmp(magic,PROJECT_FORMAT_MAGIC) && count_token(text+consumed,&version,PROJECT_FORMAT_VERSION) && version>=1;
    Project *next=ok?calloc(1,sizeof *next):NULL;
    if(next)for(int c=0;c<CHANNELS;c++)next->arp[c]=arp_default();
    if(next)for(int bus=0;bus<=INSERTS;bus++)for(int slot=0;slot<EFFECT_SLOTS;slot++){next->delay[bus][slot]=delay_default();next->tube[bus][slot]=tube_default();next->reverb[bus][slot]=reverb_default();next->dynamics[bus][slot]=dynamics_default();next->flanger[bus][slot]=flanger_default();next->phaser[bus][slot]=phaser_default();}
    if(next)for(int pat=0;pat<PATTERNS;pat++){
        next->pattern_steps[pat]=STEPS;next->pattern_colors[pat]=pattern_palette[pat%COLOR_COUNT];
        snprintf(next->pattern_names[pat],PATTERN_NAME,"Pattern %d",pat+1);
    }
    char (*names)[64]=ok?calloc(MAX_FIELDS,sizeof *names):NULL;
    if(ok && (!next || !names)) ok=0;
    unsigned char seen[FIELD_COUNT]={0}; size_t records=0; int ended=0;
    while(ok && line(file,text,sizeof text)) {
        if(!strcmp(text,"end")) { ended=1; break; }
        char name[64],kind[8]; size_t count; consumed=0;
        ok=records<MAX_FIELDS && sscanf(text,"%63s %7s %n",name,kind,&consumed)==2 &&
            count_token(text+consumed,&count,1048576) && count>0;
        if(!ok) break;
        for(size_t i=0;ok && i<records;i++) if(!strcmp(names[i],name)) ok=0;
        if(!ok) break;
        strcpy(names[records++],name);
        int type=-1; for(int t=0;t<=STRING;t++) if(!strcmp(kind,type_names[t])) type=t;
        const Field *field=NULL;
        for(size_t i=0;i<FIELD_COUNT;i++) if(!strcmp(name,fields[i].name)) { field=&fields[i]; seen[i]=1; break; }
        ok=type>=0 && (field?field->type==(FieldType)type && pattern_field_count(field,version==1?LEGACY_PATTERNS:PATTERNS)==count:!strncmp(name,"x-",2));
        if(ok) ok=read_values(file,next,field,(FieldType)type,count);
    }
    int delay_model_present=0;
    for(size_t i=0;i<FIELD_COUNT;i++)if(!strcmp(fields[i].name,"x-delay.model"))delay_model_present=seen[i];
    if(next && !delay_model_present)for(int bus=0;bus<=INSERTS;bus++)for(int slot=0;slot<EFFECT_SLOTS;slot++)if(next->effect_type[bus][slot]==EFFECT_DELAY) {
        DelaySettings *d=&next->delay[bus][slot];
        d->input_pan=d->input_gain=d->steps=d->offset=d->dry=0;d->mode=d->model=0;
    }
    ok=ok && ended && !ferror(file);
    for(size_t i=0;ok && i<FIELD_COUNT;i++) if(!seen[i] && strncmp(fields[i].name,"x-",2)) ok=0;
    for(int ch;ok && (ch=fgetc(file))!=EOF;) if(!isspace((unsigned char)ch)) ok=0;
    if(ok)for(int bus=0;bus<=INSERTS;bus++)for(int slot=0;slot<EFFECT_SLOTS;slot++){
        DynamicsSettings *s=&next->dynamics[bus][slot];
        if(s->lowcut>80 && s->lowcut<=200)s->lowcut=80;
        if(isfinite(s->low) && s->lowcut>=0 && s->lowcut<=80)s->low=fmaxf(s->low,s->lowcut);
    }
    if(ok && version==1){
        if(next->pattern_count>LEGACY_PATTERNS)ok=0;
        for(int l=0;l<LANES;l++)for(int b=0;b<CLIPS;b++){
            unsigned v=next->clips[l][b];if(v>LEGACY_PATTERNS+CHANNELS+AUTOMATIONS)ok=0;
            else if(v>LEGACY_PATTERNS)next->clips[l][b]=v+(PATTERNS-LEGACY_PATTERNS);
        }
    }
    ok=ok && !ferror(file) && structure_valid(next);
    if(ok) *p=*next;
    free(next); free(names); fclose(file); return ok;
}


static void le(FILE *f,uint32_t x,int n) { for(int i=0;i<n;i++) fputc((x>>(i*8))&255,f); }
int export_wav(const char *path,const Project *pr,const Sample s[CHANNELS]) {
    double total=ceil(song_steps(pr)*RATE*60.0/pr->bpm/4);
    if(total<0 || total>(UINT32_MAX-36)/4 || !isfinite(total)) return 0;
    uint32_t frames=(uint32_t)total;
    AtomicFile output; if(!atomic_file_open(&output,path)) return 0;
    FILE *f=output.file;
    fwrite("RIFF",1,4,f); le(f,36+frames*4,4); fwrite("WAVEfmt ",1,8,f); le(f,16,4); le(f,1,2); le(f,2,2); le(f,RATE,4); le(f,RATE*4,4); le(f,4,2); le(f,16,2); fwrite("data",1,4,f); le(f,frames*4,4);
    Player player; player_reset(&player); player.song=1; float block[1024];
    player.effects=effects_create(INSERTS+1);
    if(!player.effects || !effects_prepare(player.effects,&pr->effect_type[0][0],(INSERTS+1)*EFFECT_SLOTS)) { effects_free(player.effects); atomic_file_abort(&output); return 0; }
    PlaybackPlan plan; playback_plan_prepare(&plan,pr,1,NULL,1); player.plan=&plan;
    for(uint32_t i=0;i<frames;) {
        unsigned n=frames-i>512?512:frames-i; render(&player,pr,s,block,n);
        for(unsigned j=0;j<n*2;j++) {
            float x=isfinite(block[j])?fmaxf(-1,fminf(1,block[j])):0;
            le(f,(uint16_t)(int16_t)(x*32767),2);
        }
        i+=n;
    }
    effects_free(player.effects);
    return atomic_file_commit(&output);
}
