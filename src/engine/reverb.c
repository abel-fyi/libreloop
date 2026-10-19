// SPDX-License-Identifier: GPL-3.0-only
#include "reverb.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define RATE 48000.f
#define LINES 8
#define LINE_FRAMES 8192
#define DIFF_FRAMES 1024
#define PRE_FRAMES 24576
#define PI 3.14159265358979323846
struct Reverb {
    struct {float line[LINES][LINE_FRAMES],diff[2][6][DIFF_FRAMES],tank[LINES][2][DIFF_FRAMES],pre[PRE_FRAMES][2];} pcm;
    unsigned cursor,valid,pre_cursor,pre_valid,diff_cursor[2][6],diff_valid[2][6],tank_cursor[LINES][2],tank_valid[LINES][2],tick,gate_remaining;
    float decay,damping,width,mod,wet,gate_level,low[LINES],gain[LINES],gain_step[LINES],coefficient,coefficient_step,coefficient_target;
    float last_input[2],highpass[2];
    double sine[LINES],cosine[LINES],rotate_sine[LINES],rotate_cosine[LINES];
    double old_size,new_size,old_pre,new_pre,transition;
    int ready;
};
static const unsigned lengths[LINES]={1493,1601,1747,1867,1999,2137,2281,2437};
static const unsigned diff_lengths[2][6]={{143,211,347,557,421,683},{163,239,373,601,449,709}};
static const unsigned tank_lengths[LINES][2]={{103,37},{173,61},{263,89},{337,127},{419,163},{503,211},{631,277},{751,359}};
static const float rates[LINES]={.071f,.083f,.097f,.113f,.131f,.151f,.173f,.197f};
static const int inject[2][LINES]={{1,1,1,1,-1,-1,-1,-1},{1,-1,1,-1,1,-1,1,-1}};
static const int pickup[2][LINES]={{1,1,-1,1,-1,1,-1,-1},{1,-1,-1,-1,1,1,1,-1}};
ReverbSettings reverb_default(void) {return (ReverbSettings){.55f,2.4f,18000,18,1,.3f,0};}
ReverbSettings reverb_factory(unsigned index) {
    switch(index) {
    case 1:return (ReverbSettings){.38f,4,20000,8,1.3f,.2f,260};
    case 2:return (ReverbSettings){.18f,.65f,12000,5,.9f,.15f,0};
    case 3:return (ReverbSettings){.85f,8,14000,35,1.4f,.45f,0};
    default:return reverb_default();
    }
}
const char *reverb_factory_name(unsigned index) {
    static const char *names[]={"Bright Hall","Bright Snare Gate","Small Room","Wide Space"};
    return index<REVERB_FACTORY_COUNT?names[index]:"Bright Hall";
}
float reverb_factory_mix(unsigned index) {return index==1?.5f:index==2?.2f:index==3?.3f:.25f;}
int reverb_valid(ReverbSettings s) {
    return isfinite(s.size) && s.size>=0 && s.size<=1 && isfinite(s.decay) && s.decay>=.15f && s.decay<=20 &&
        isfinite(s.damping) && s.damping>=500 && s.damping<=20000 && isfinite(s.predelay) && s.predelay>=0 && s.predelay<=250 &&
        isfinite(s.width) && s.width>=0 && s.width<=2 && isfinite(s.mod) && s.mod>=0 && s.mod<=1 &&
        isfinite(s.gate) && s.gate>=0 && s.gate<=1000;
}
size_t reverb_storage_bytes(void) {return sizeof(Reverb);}
Reverb *reverb_create(void) {
    Reverb *r=malloc(sizeof *r);
    if(r) {volatile unsigned char *p=(volatile unsigned char *)r;for(size_t i=0;i<sizeof *r;i+=4096)p[i]=0;reverb_reset(r);}
    return r;
}
void reverb_free(Reverb *r) {free(r);}
void reverb_reset(Reverb *r) {if(r)memset((char *)r+sizeof r->pcm,0,sizeof *r-sizeof r->pcm);}
int reverb_audible(const Reverb *r) {return r && r->ready && r->wet>.00001f;}
static float flush(float x) {return fabsf(x)<1e-20f?0:x;}
static float pre_sample(const Reverb *r,unsigned back,unsigned side,float current) {
    if(!back)return current;if(back>r->pre_valid)return 0;
    unsigned at=(r->pre_cursor+PRE_FRAMES-back)%PRE_FRAMES;return r->pcm.pre[at][side];
}
static float pre_read(const Reverb *r,float delay,unsigned side,float current) {
    unsigned whole=(unsigned)delay;float fraction=delay-whole;
    float a=pre_sample(r,whole,side,current),b=pre_sample(r,whole+1,side,current);return a+(b-a)*fraction;
}
static float line_sample(const Reverb *r,unsigned line,unsigned index) {
    unsigned age=(r->cursor+LINE_FRAMES-index)%LINE_FRAMES;
    return !age || age>r->valid?0:r->pcm.line[line][index];
}
static float line_read(const Reverb *r,unsigned line,double delay) {
    if(delay>r->valid+2)return 0;
    double at=r->cursor-delay;if(at<0)at+=LINE_FRAMES;
    unsigned first=(unsigned)at;float t=at-first;
    /* Central four-point Lagrange interpolation keeps the upper tail brighter
       than a linear interpolator while retaining bounded fractional positions. */
    float a=line_sample(r,line,(first+LINE_FRAMES-1)%LINE_FRAMES),b=line_sample(r,line,first);
    float c=line_sample(r,line,(first+1)%LINE_FRAMES),d=line_sample(r,line,(first+2)%LINE_FRAMES);
    return -a*t*(t-1)*(t-2)/6+b*(t+1)*(t-1)*(t-2)/2-c*(t+1)*t*(t-2)/2+d*(t+1)*t*(t-1)/6;
}
static float diffuse(Reverb *r,unsigned side,unsigned stage,float input) {
    unsigned cursor=r->diff_cursor[side][stage],length=diff_lengths[side][stage];
    unsigned read=(cursor+DIFF_FRAMES-length)%DIFF_FRAMES;
    float delayed=r->diff_valid[side][stage]>=length?r->pcm.diff[side][stage][read]:0;
    float g=stage<4?.72f:.6f,output=delayed-g*input;
    r->pcm.diff[side][stage][cursor]=flush(input+g*output);
    r->diff_cursor[side][stage]=(cursor+1)%DIFF_FRAMES;
    if(r->diff_valid[side][stage]<DIFF_FRAMES)r->diff_valid[side][stage]++;
    return output;
}
/* Lossless scattering inside the feedback loop increases echo density on every
   return, rather than merely smoothing the input/output of a sparse tank. */
static float tank_diffuse(Reverb *r,unsigned line,unsigned stage,float input) {
    unsigned cursor=r->tank_cursor[line][stage],length=tank_lengths[line][stage];
    unsigned at=(cursor+DIFF_FRAMES-length)%DIFF_FRAMES;
    float delayed=r->tank_valid[line][stage]>=length?r->pcm.tank[line][stage][at]:0;
    float g=stage?.45f:.55f,output=delayed-g*input;
    r->pcm.tank[line][stage][cursor]=flush(input+g*output);
    r->tank_cursor[line][stage]=(cursor+1)%DIFF_FRAMES;
    if(r->tank_valid[line][stage]<DIFF_FRAMES)r->tank_valid[line][stage]++;
    return output;
}
static float loop_length(unsigned line,float size) {
    return lengths[line]*size+tank_lengths[line][0]+tank_lengths[line][1];
}
static float damping_coefficient(float hz) {return hz>=19999?1:1-expf(-2*PI*hz/RATE);}
void reverb_process(Reverb *r,ReverbSettings s,float wet,float stereo[2]) {
    if(!r)return;
    const float slew=1.f/(RATE*.02f);float size=.5f+1.5f*s.size,pre=s.predelay*48;
    wet=fmaxf(0,fminf(1,wet));
    if(!r->ready) {
        r->decay=s.decay;r->damping=s.damping;r->width=s.width;r->mod=s.mod;r->wet=wet;r->gate_level=s.gate?0:1;
        r->old_size=r->new_size=size;r->old_pre=r->new_pre=pre;r->transition=1;r->coefficient=damping_coefficient(s.damping);
        for(unsigned i=0;i<LINES;i++) {
            r->gain[i]=powf(10,-3.f*loop_length(i,size)/(RATE*s.decay));
            r->sine[i]=sin(2*PI*i/LINES);r->cosine[i]=cos(2*PI*i/LINES);
            r->rotate_sine[i]=sin(2*PI*rates[i]/RATE);r->rotate_cosine[i]=cos(2*PI*rates[i]/RATE);
        }
        r->ready=1;
    }
    r->decay+=(s.decay-r->decay)*slew;r->damping+=(s.damping-r->damping)*slew;
    r->width+=(s.width-r->width)*slew;r->mod+=(s.mod-r->mod)*slew;r->wet+=(wet-r->wet)*slew;
    if(r->transition>=1 && (fabs(size-r->new_size)>1e-5 || fabs(pre-r->new_pre)>.01)) {
        r->old_size=r->new_size;r->old_pre=r->new_pre;r->new_size=size;r->new_pre=pre;r->transition=0;
    }
    if(!(r->tick++%32)) {
        float scale=r->old_size+(r->new_size-r->old_size)*r->transition;
        for(unsigned i=0;i<LINES;i++)r->gain_step[i]=(powf(10,-3.f*loop_length(i,scale)/(RATE*r->decay))-r->gain[i])/32;
        r->coefficient_target=damping_coefficient(r->damping);
        r->coefficient_step=(r->coefficient_target-r->coefficient)/32;
    }
    r->coefficient+=r->coefficient_step;if(!(r->tick%32))r->coefficient=r->coefficient_target;
    float source[2];
    for(unsigned side=0;side<2;side++){source[side]=flush(stereo[side]-r->last_input[side]+.99738543f*r->highpass[side]);r->last_input[side]=stereo[side];r->highpass[side]=source[side];}
    float input[2],values[LINES],mixed[LINES];
    for(unsigned side=0;side<2;side++) {
        float a=pre_read(r,r->old_pre,side,source[side]),b=pre_read(r,r->new_pre,side,source[side]);
        input[side]=a+(b-a)*(float)r->transition;
        for(unsigned stage=0;stage<4;stage++)input[side]=diffuse(r,side,stage,input[side]);
    }
    for(unsigned i=0;i<LINES;i++) {
        double modulation=r->sine[i]*12*r->mod;
        float a=line_read(r,i,lengths[i]*r->old_size+modulation),b=line_read(r,i,lengths[i]*r->new_size+modulation);
        float sample=a+(b-a)*(float)r->transition;
        r->low[i]=r->coefficient==1?sample:flush(r->low[i]+(sample-r->low[i])*r->coefficient);values[i]=mixed[i]=r->low[i];
        double sine=r->sine[i]*r->rotate_cosine[i]+r->cosine[i]*r->rotate_sine[i];
        r->cosine[i]=r->cosine[i]*r->rotate_cosine[i]-r->sine[i]*r->rotate_sine[i];r->sine[i]=sine;
        if(!(r->tick%4096)){double norm=1/hypot(r->sine[i],r->cosine[i]);r->sine[i]*=norm;r->cosine[i]*=norm;}
    }
    /* Normalized Walsh-Hadamard mixing preserves feedback-vector energy. */
    for(unsigned width=1;width<LINES;width*=2)for(unsigned block=0;block<LINES;block+=width*2)for(unsigned i=0;i<width;i++) {
        float a=mixed[block+i],b=mixed[block+i+width];mixed[block+i]=a+b;mixed[block+i+width]=a-b;
    }
    for(unsigned i=0;i<LINES;i++) {
        r->gain[i]+=r->gain_step[i];
        float scattered=tank_diffuse(r,i,1,tank_diffuse(r,i,0,mixed[i]*.3535533905932738f));
        r->pcm.line[i][r->cursor]=flush(scattered*r->gain[i]+.18f*(input[0]*inject[0][i]+input[1]*inject[1][i]));
    }
    /* A centered, already-diffused onset replaces separate wall-reflection
       taps. Stereo spread develops in the tank without an ear-leading echo. */
    float onset=(input[0]+input[1])*.12f;
    float predelay=r->old_pre+(r->new_pre-r->old_pre)*r->transition;
    float output[2]={0};
    for(unsigned side=0;side<2;side++) {
        for(unsigned i=0;i<LINES;i++)output[side]+=values[i]*pickup[side][i]*.3535533905932738f;
        output[side]=diffuse(r,side,5,diffuse(r,side,4,output[side]))+onset;
    }
    /* Gate hold follows the input, with a smooth finite release on the wet path. */
    if(s.gate && fmaxf(fabsf(source[0]),fabsf(source[1]))>.0039810717f)r->gate_remaining=(unsigned)(s.gate*48+predelay);
    float gate=1;
    if(s.gate) {float t=fminf(1,r->gate_remaining/480.f);gate=t*t*(3-2*t);if(r->gate_remaining)r->gate_remaining--;}
    r->gate_level=gate>r->gate_level?fminf(gate,r->gate_level+1.f/96):fmaxf(gate,r->gate_level-1.f/480);
    float mid=(output[0]+output[1])*.5f,side=(output[0]-output[1])*.5f*r->width;
    r->pcm.pre[r->pre_cursor][0]=source[0];r->pcm.pre[r->pre_cursor][1]=source[1];
    stereo[0]=stereo[0]*(1-r->wet)+(mid+side)*r->gate_level*r->wet;
    stereo[1]=stereo[1]*(1-r->wet)+(mid-side)*r->gate_level*r->wet;
    r->pre_cursor=(r->pre_cursor+1)%PRE_FRAMES;if(r->pre_valid<PRE_FRAMES)r->pre_valid++;
    r->cursor=(r->cursor+1)%LINE_FRAMES;if(r->valid<LINE_FRAMES)r->valid++;
    if(r->transition<1)r->transition=fmin(1,r->transition+slew);
}
