/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2012-2013 Google Inc.
 * Copyright 2017 Pascal Gauthier.
 * C adaptation for LibreLoop, 2026. Derived from Dexed's MSFA Modern engine.
 * Licensed under Apache License 2.0; see docs/licenses/msfa-Apache-2.0.txt.
 * Modified: fixed 48 kHz, bounded instance state, unsigned phase arithmetic,
 * no JUCE/MIDI/portamento dependencies, editable native voice parameters.
 */
#include "dx7.h"
#include "sample.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include "dx7_tables.h"
#include "dx7_factory.h"
#define Q24 16777216
#define PI 3.14159265358979323846
static int32_t sine_table[2048],exponent_table[2048],frequency_table[1025];
static ParameterDescriptor descriptors[DX7_PARAMETERS];
static char parameter_names[DX7_PARAMETERS][48];
static pthread_once_t initialized=PTHREAD_ONCE_INIT;
static int min_int(int a,int b) { return a<b?a:b; }
static int max_int(int a,int b) { return a>b?a:b; }
static int parameter_max(int i) {
    if(i>=DX7_NATIVE_PARAMETERS) return 100;
    static const int operator_max[21]={99,99,99,99,99,99,99,99,99,99,99,3,3,7,3,7,99,1,31,99,14};
    static const int global_max[19]={99,99,99,99,99,99,99,99,31,7,1,99,99,99,99,1,5,7,48};
    return i<126?operator_max[i%21]:global_max[i-126];
}
static void prepare(void) {
    double dphase=2*PI/1024; int32_t c=floor(cos(dphase)*(1<<30)+.5),s=floor(sin(dphase)*(1<<30)+.5),u=1<<30,v=0;
    for(int i=0;i<512;i++) {
        sine_table[2*i+1]=(v+32)>>6; sine_table[2*(i+512)+1]=-((v+32)>>6);
        int32_t t=((int64_t)u*s+(int64_t)v*c+(1<<29))>>30;
        u=((int64_t)u*c-(int64_t)v*s+(1<<29))>>30; v=t;
    }
    for(int i=0;i<1023;i++) sine_table[2*i]=sine_table[2*i+3]-sine_table[2*i+1];
    sine_table[2046]=-sine_table[2047];
    double y=1<<30,inc=exp2(1./1024);
    for(int i=0;i<1024;i++) { exponent_table[2*i+1]=floor(y+.5); y*=inc; }
    for(int i=0;i<1023;i++) exponent_table[2*i]=exponent_table[2*i+3]-exponent_table[2*i+1];
    exponent_table[2046]=(int64_t)(1ULL<<31)-exponent_table[2047];
    y=(1ULL<<44)/(double)RATE;
    for(int i=0;i<=1024;i++) { frequency_table[i]=floor(y+.5); y*=inc; }
    static const char *op_names[]={"Rate 1","Rate 2","Rate 3","Release rate","Level 1","Level 2","Sustain level","Release level","Breakpoint","Left depth","Right depth","Left curve","Right curve","Rate scaling","Amp sensitivity","Velocity","Output","Frequency mode","Coarse","Fine","Detune"};
    static const char *global_names[]={"Pitch rate 1","Pitch rate 2","Pitch rate 3","Pitch release rate","Pitch level 1","Pitch level 2","Pitch sustain","Pitch release","Algorithm","Feedback","Oscillator sync","LFO speed","LFO delay","Pitch modulation","Amplitude modulation","LFO sync","LFO waveform","Pitch sensitivity","Transpose"};
    for(int i=0;i<DX7_PARAMETERS;i++) {
        if(i>=DX7_NATIVE_PARAMETERS) {
            snprintf(parameter_names[i],sizeof parameter_names[i],"Operator %d key tracking",6-(i-DX7_NATIVE_PARAMETERS));
            descriptors[i]=(ParameterDescriptor){PARAM_DX7_FIRST+i,parameter_names[i],0,100,100,PARAMETER_CONTINUOUS};
            continue;
        }
        if(i<126) snprintf(parameter_names[i],sizeof parameter_names[i],"Operator %d %s",6-i/21,op_names[i%21]);
        else snprintf(parameter_names[i],sizeof parameter_names[i],"DX7 %s",global_names[i-126]);
        descriptors[i]=(ParameterDescriptor){PARAM_DX7_FIRST+i,parameter_names[i],0,parameter_max(i),factory[0][i],PARAMETER_INTEGER};
    }
}
void dx7_init(void) { pthread_once(&initialized,prepare); }
const ParameterDescriptor *dx7_parameter_descriptor(unsigned index) { return index<DX7_PARAMETERS?&descriptors[index]:NULL; }
const char *dx7_factory_name(int preset) { static const char *names[]={"DX7 E.PIANO 1","DX7 BASS 1","DX7 MARIMBA","DX7 TUB BELLS"}; return preset>=0 && preset<4?names[preset]:"MK80-inspired EP"; }
DX7Settings dx7_factory(int preset) {
    DX7Settings s; int index=preset>=0 && preset<4?preset:0;
    for(int i=0;i<DX7_NATIVE_PARAMETERS;i++) s.value[i]=factory[index][i];
    dx7_legacy_tracking(&s);
    if(preset<0) { s.value[4*21+16]=67; s.value[2*21+16]=92; s.value[4*21+1]=56; s.value[5*21+1]=28; }
    return s;
}
void dx7_legacy_tracking(DX7Settings *s) {
    for(int op=0;op<6;op++) s->value[DX7_NATIVE_PARAMETERS+op]=s->value[op*21+17]?0:100;
}
int dx7_valid(DX7Settings s) {
    for(int i=0;i<DX7_PARAMETERS;i++) if(!isfinite(s.value[i]) || s.value[i]<0 || s.value[i]>parameter_max(i) || (i<DX7_NATIVE_PARAMETERS && s.value[i]!=roundf(s.value[i]))) return 0;
    return 1;
}
static int32_t sine(uint32_t phase) {
    unsigned low=phase&16383,index=(phase>>13)&2046;
    return sine_table[index+1]+(((int64_t)sine_table[index]*low)>>14);
}
static int32_t exponential(int32_t x) {
    unsigned index=((uint32_t)x>>13)&2046,low=(uint32_t)x&16383;
    int64_t y=exponent_table[index+1]+(((int64_t)exponent_table[index]*low)>>14); int shift=6-(x>>24);
    return shift>=0?(int32_t)(y>>min_int(shift,62)):(int32_t)(y<<min_int(-shift,1));
}
static uint32_t frequency_lookup(int32_t x) {
    int index=((uint32_t)x&0xffffff)>>14,shift=20-(x>>24); unsigned low=(uint32_t)x&16383;
    uint64_t y=frequency_table[index]+(((int64_t)(frequency_table[index+1]-frequency_table[index])*low)>>14);
    return shift>=0?(uint32_t)(y>>min_int(shift,63)):(uint32_t)(y<<min_int(-shift,3));
}
static int scale_level(int level) { static const int lut[]={0,5,9,13,17,20,23,25,27,29,31,33,35,37,39,41,42,43,45,46}; return level>=20?28+level:lut[level]; }
static void envelope_advance(DX7Envelope *e,int stage) {
    e->stage=stage; e->hold=0;
    if(stage>=4) return;
    int level=e->levels[stage];
    int actual=max_int(16,((scale_level(level)>>1)*64)+e->outlevel-4256);
    e->target=actual*65536; e->rising=e->target>e->level;
    int rate=min_int(63,((e->rates[stage]*41)>>6)+e->rate_scale);
    e->increment=((int64_t)((4+(rate&3))<<(8+(rate>>2)))*(uint32_t)(44100.0/RATE*Q24))>>24;
    if(e->target==e->level || (!stage && !level)) {
        int r=min_int(99,e->rates[stage]+e->rate_scale),hold=r<77?statics[r]:20*(99-r);
        if(r<77 && !stage && !level) hold/=20;
        e->hold=((int64_t)hold*(uint32_t)(44100.0/RATE*Q24))>>24;
    }
}
static int32_t envelope_sample(DX7Envelope *e) {
    if(e->hold) { e->hold-=DX7_BLOCK; if(e->hold<=0) { e->hold=0; envelope_advance(e,e->stage+1); } }
    if(e->stage<3 || (e->stage<4 && !e->down)) {
        if(e->hold) return e->level;
        if(e->rising) {
            e->level=max_int(e->level,1716*65536);
            e->level+=((17*Q24-e->level)>>24)*e->increment;
            if(e->level>=e->target) { e->level=e->target; envelope_advance(e,e->stage+1); }
        } else {
            e->level-=e->increment;
            if(e->level<=e->target) { e->level=e->target; envelope_advance(e,e->stage+1); }
        }
    }
    return e->level;
}
static int scale_curve(int group,int depth,int curve) {
    int value=curve==0 || curve==3?(group*depth*329)>>12:(exp_scale_data[min_int(group,32)]*depth*329)>>15;
    return curve<2?-value:value;
}
static int32_t oscillator_pitch(int note,const uint8_t *p) {
    if(p[17]) { int32_t x=(4458616*((p[18]&3)*100+p[19]))>>3; return x+(p[20]>7?13457*(p[20]-7):0); }
    int32_t x=50857777+note*(Q24/12);
    double detune=.0209*exp(-.396*(x/(double)Q24))/7;
    x+=(int32_t)(detune*x*((int)p[20]-7)); x+=coarsemul[p[18]&31];
    if(p[19]) x+=(int32_t)floor(24204406.323123*log(1+.01*p[19])+.5);
    return x;
}
static void pitch_advance(DX7Envelope *e,int stage) {
    e->stage=stage; if(stage>=4) return;
    e->target=pitchenv_tab[e->levels[stage]]*524288; e->rising=e->target>e->level;
    e->increment=pitchenv_rate[e->rates[stage]]*(int)(DX7_BLOCK*(double)Q24/(21.3*RATE)+.5);
}
static int32_t pitch_sample(DX7Envelope *e) {
    if(e->stage<3 || (e->stage<4 && !e->down)) {
        e->level+=e->rising?e->increment:-e->increment;
        if(e->rising?e->level>=e->target:e->level<=e->target) { e->level=e->target; pitch_advance(e,e->stage+1); }
    }
    return e->level;
}
static void setup_operator(DX7Voice *v,int op,int reset) {
    const uint8_t *p=v->patch+op*21; DX7Envelope *e=&v->envelope[op];
    int previous_outlevel=e->outlevel;
    int offset=v->midi-p[8]-17;
    int scaling=offset>=0?scale_curve((offset+1)/3,p[10],p[12]):scale_curve(-(offset-1)/3,p[9],p[11]);
    e->outlevel=max_int(0,min_int(127,scale_level(p[16])+scaling)*32+(((p[15]*((int)velocity_data[v->velocity>>1]-239)+7)>>3)*16));
    e->rate_scale=(p[13]*min_int(31,max_int(0,v->midi/3-7)))>>3;
    for(int i=0;i<4;i++) { e->rates[i]=p[i]; e->levels[i]=p[i+4]; }
    if(reset) { e->level=0; e->down=1; envelope_advance(e,0); }
    else if(e->stage<4) {
        int peak=max_int(16,63*64+e->outlevel-4256)*65536;
        e->level=min_int(peak,max_int(16*65536,e->level+(e->outlevel-previous_outlevel)*65536));
        if(e->stage==3 && e->down) e->level=max_int(16,((scale_level(e->levels[2])>>1)*64)+e->outlevel-4256)*65536;
        envelope_advance(e,e->stage);
    }
    int32_t anchor=oscillator_pitch(60,p);
    double tracking=v->key_tracking[op]*.01;
    int32_t delta=p[17]?(v->midi-60)*(Q24/12):oscillator_pitch(v->midi,p)-anchor;
    v->base_pitch[op]=anchor+(int32_t)llround(delta*tracking);
}
static void setup_lfo(DX7Voice *v,int reset) {
    v->lfo_step=lfoSource[v->patch[137]]*(4437500000.0*DX7_BLOCK/RATE);
    int a=99-v->patch[138]; uint32_t unit=DX7_BLOCK*25190424.0/RATE+.5;
    if(a==99) v->delay_inc[0]=v->delay_inc[1]=UINT32_MAX;
    else { a=(16+(a&15))<<(1+(a>>4)); v->delay_inc[0]=unit*a; a=max_int(0x80,a&0xff80); v->delay_inc[1]=unit*a; }
    if(reset) { v->lfo_phase=v->patch[141]?(1U<<31)-1:0; v->delay_state=0; }
}
void dx7_note_on(DX7Voice *v,double hz,float velocity,DX7Settings s) {
    memset(v,0,sizeof *v); for(int i=0;i<DX7_NATIVE_PARAMETERS;i++) v->patch[i]=(uint8_t)roundf(s.value[i]);
    for(int op=0;op<6;op++) v->key_tracking[op]=s.value[DX7_NATIVE_PARAMETERS+op];
    v->base_midi=(int)llround(69+12*log2(hz/440)); v->midi=v->base_midi+v->patch[144]-24;
    v->velocity=max_int(1,min_int(127,(int)roundf(velocity*127))); v->note_frequency=hz; v->pitch_ratio=1; v->down=1; v->cursor=DX7_BLOCK; v->algorithm=v->patch[134];
    for(int op=0;op<6;op++) setup_operator(v,op,1);
    for(int i=0;i<4;i++) { v->pitch.rates[i]=v->patch[126+i]; v->pitch.levels[i]=v->patch[130+i]; }
    v->pitch.level=pitchenv_tab[v->pitch.levels[3]]*524288; v->pitch.down=1; pitch_advance(&v->pitch,0); setup_lfo(v,1);
}
void dx7_note_off(DX7Voice *v) {
    if(!v->down) return; v->down=0;
    for(int i=0;i<6;i++) { v->envelope[i].down=0; envelope_advance(&v->envelope[i],3); }
    v->pitch.down=0; pitch_advance(&v->pitch,3);
}
int dx7_active(const DX7Voice *v) {
    for(int i=0;i<6;i++) if((algorithms[v->algorithm][i]&4) && (v->envelope[i].stage<4 || v->envelope[i].levels[3]>0)) return 1;
    return 0;
}
static int32_t lfo_sample(DX7Voice *v) {
    v->lfo_phase+=v->lfo_step; uint32_t phase=v->lfo_phase; int32_t x;
    switch(v->patch[142]) {
    case 0: x=(int32_t)(phase>>7); x^=-(int32_t)(phase>>31); return x&(Q24-1);
    case 1: return (~phase^(1U<<31))>>8;
    case 2: return (phase^(1U<<31))>>8;
    case 3: return ((~phase)>>7)&Q24;
    case 4: return (1<<23)+(sine(phase>>8)>>1);
    default: if(phase<v->lfo_step) v->random=(v->random*179+17)&255; return ((v->random^128)+1)*65536;
    }
}
static int32_t lfo_delay(DX7Voice *v) {
    uint64_t next=(uint64_t)v->delay_state+v->delay_inc[v->delay_state<(1U<<31)?0:1];
    if(next>UINT32_MAX) return Q24;
    v->delay_state=next; return next<(1U<<31)?0:(next>>7)&(Q24-1);
}
static void render_block(DX7Voice *v,const DX7Settings *s,double pitch) {
    int transposed=v->patch[144]!=(int)roundf(s->value[144]);
    v->midi=v->base_midi+(int)roundf(s->value[144])-24;
    for(int op=0;op<6;op++) {
        int changed=transposed; for(int j=0;j<21;j++) { uint8_t value=roundf(s->value[op*21+j]); changed|=v->patch[op*21+j]!=value; v->patch[op*21+j]=value; }
        changed|=v->key_tracking[op]!=s->value[DX7_NATIVE_PARAMETERS+op];
        v->key_tracking[op]=s->value[DX7_NATIVE_PARAMETERS+op];
        if(changed) setup_operator(v,op,0);
    }
    int pitch_changed=0;
    for(int i=126;i<DX7_NATIVE_PARAMETERS;i++) { int value=roundf(s->value[i]); if(i<134) pitch_changed|=value!=v->patch[i]; v->patch[i]=value; }
    if(pitch_changed) {
        for(int i=0;i<4;i++) { v->pitch.rates[i]=v->patch[126+i]; v->pitch.levels[i]=v->patch[130+i]; }
        pitch_advance(&v->pitch,v->pitch.stage);
    }
    v->algorithm=v->patch[134]; setup_lfo(v,0);
    static const int sensitivity[]={0,10,20,33,55,92,153,255},amp_sensitivity[]={0,4342338,7171437,Q24};
    int32_t lfo=lfo_sample(v),delay=lfo_delay(v),sens=sensitivity[v->patch[143]]*(lfo-(1<<23));
    uint32_t depth=((v->patch[139]*165)>>6)*(uint32_t)delay;
    int32_t modulation=pitch_sample(&v->pitch)+(int32_t)(((int64_t)depth*sens)>>39);
    v->pitch_ratio+=(pitch-v->pitch_ratio)*(1-exp(-DX7_BLOCK/(RATE*.02)));
    int32_t bend=llround(log2(fmax(1e-6,v->pitch_ratio))*Q24);
    int32_t amplitude=(int64_t)(((v->patch[140]*165)>>6)*(int64_t)delay)>>8;
    amplitude=((int64_t)amplitude*(Q24-lfo))>>24;
    memset(v->output,0,sizeof v->output); int contents[3]={1,0,0};
    for(int op=0;op<6;op++) {
        const uint8_t *p=v->patch+op*21; int flags=algorithms[v->algorithm][op],in=(flags>>4)&3,out=flags&3,add=flags&4;
        int32_t level=envelope_sample(&v->envelope[op]);
        if(p[14]) {
            uint32_t am=((int64_t)amplitude*amp_sensitivity[p[14]])>>24;
            uint32_t pt=exp(am/262144.f*.07f+12.2f);
            level-=(int64_t)level*pt/16777216;
        }
        int32_t gain0=v->gain[op],gain1=exponential(level-14*Q24); v->gain[op]=gain1;
        uint32_t freq=frequency_lookup(v->base_pitch[op]+bend+(p[17]?0:modulation)); v->frequency[op]=freq;
        if(gain0>=1120 || gain1>=1120) {
            if(!contents[out]) add=0;
            int32_t *target=out?v->bus[out-1]:v->output;
            int32_t increment=(gain1-gain0+32)>>6,gain=gain0; uint32_t phase=v->phase[op];
            int feedback=(flags&0xc0)==0xc0 && v->patch[135] && (!in || !contents[in]);
            for(int i=0;i<DX7_BLOCK;i++) {
                gain+=increment; int32_t input=in && contents[in]?v->bus[in-1][i]:0;
                if(feedback) input=((int64_t)v->feedback[0]+v->feedback[1])>>(9-v->patch[135]);
                int32_t sample=((int64_t)sine(phase+(uint32_t)input)*gain)>>24;
                if(feedback) { v->feedback[0]=v->feedback[1]; v->feedback[1]=sample; }
                target[i]=add?target[i]+sample:sample; phase+=freq;
            }
            contents[out]=1;
        } else if(!add) contents[out]=0;
        v->phase[op]+=freq*DX7_BLOCK;
    }
    v->cursor=0;
}
float dx7_sample(DX7Voice *v,const DX7Settings *s,double pitch) {
    if(v->cursor==DX7_BLOCK) render_block(v,s,pitch);
    return v->output[v->cursor++]/(float)Q24*.08f;
}
