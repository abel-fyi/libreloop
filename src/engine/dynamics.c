// SPDX-License-Identifier: GPL-3.0-only
#include "dynamics.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#define RATE 48000.f
#define PI 3.14159265358979323846
_Static_assert(sizeof(DynamicsSettings)==184*sizeof(float),"Dynamics saved float layout");
_Static_assert(ATOMIC_INT_LOCK_FREE==2,"Dynamics meters need lock-free atomics");
typedef struct {double a[3],b[3],target_a[3],target_b[3],z[2][2];float frequency;} Biquad;
struct Dynamics {
    Biquad split[2][2][2],align[2][2],cut[4];
    float cut_mix,rms[4],envelope[4],gain[4],slow_gain[4],width[4],pre[4],post[4],wet;
    float attack[4],release[4],release2[4],average[4],sat[4],ceil[4],target_pre[4],target_post[4];
    float peak_in[4],peak_out[4],peak_gain[4];
    DynamicsPoint curve_cache[4][DYNAMICS_POINTS];
    float curve_count[4];unsigned curve_slew[4];
    unsigned tick,ready;
    _Atomic unsigned serial,history[4][DYNAMICS_HISTORY];
};
static float clamp(float x,float a,float b){return fmaxf(a,fminf(b,x));}
static float db(float x){return 20*log10f(fmaxf(x,1e-12f));}
static float amp(float x){return powf(10,x*.05f);}
static float time_coefficient(float ms){return ms<=0?0:expf(-1/(RATE*ms*.001f));}
DynamicsSettings dynamics_default(void){
    DynamicsSettings s={.low=200,.high=2500,.lowcut=0,.blend=1};
    for(int b=0;b<4;b++) {DynamicsBand *p=&s.band[b];p->attack=10;p->release=150;p->sustain=10;p->width=1;p->ceiling=0;p->count=3;p->points[0]=(DynamicsPoint){-72,-72,0,0};p->points[1]=(DynamicsPoint){0,0,0,0};p->points[2]=(DynamicsPoint){12,0,0,0};}
    return s;
}
DynamicsSettings dynamics_factory(unsigned index){DynamicsSettings s=dynamics_default();
    if(index==1){for(int b=0;b<4;b++){s.band[b].points[1]=(DynamicsPoint){-18,-18,.25f,0};s.band[b].points[2].y=-8;s.band[b].attack=b==3?2:15;s.band[b].release=110;}s.band[0].width=0;s.blend=.65f;}
    if(index==2){s.band[0].width=0;s.band[2].width=1.3f;for(int b=0;b<4;b++){s.band[b].points[1]=(DynamicsPoint){-12,-12,.2f,0};s.band[b].points[2].y=0;s.band[b].attack=25;s.band[b].release=250;}}
    return s;
}
const char *dynamics_factory_name(unsigned i){static const char *names[]={"0 dB Ceiling","Drum Punch","Gentle Glue"};return names[i<3?i:0];}
const float *dynamics_parameter(const DynamicsSettings *s,unsigned n){if(n<4){switch(n){case 0:return &s->low;case 1:return &s->high;case 2:return &s->lowcut;default:return &s->blend;}}n-=4;if(n>=48)return NULL;const DynamicsBand *b=&s->band[n/12];switch(n%12){case 0:return &b->pre;case 1:return &b->post;case 2:return &b->attack;case 3:return &b->release;case 4:return &b->release2;case 5:return &b->sustain;case 6:return &b->width;case 7:return &b->mode;case 8:return &b->solo;case 9:return &b->rms;case 10:return &b->ceiling;default:return &b->saturation;}}
int dynamics_valid(DynamicsSettings s){
    for(unsigned i=0;i<DYNAMICS_PARAMETERS;i++){float v=*dynamics_parameter(&s,i);if(!isfinite(v))return 0;}
    if(s.low<40 || s.low>1000 || s.high<1000 || s.high>16000 || s.high<s.low || s.lowcut<0 || s.lowcut>80 || s.blend<0 || s.blend>1)return 0;
    for(int i=0;i<4;i++){DynamicsBand *b=&s.band[i];if(b->pre< -24 || b->pre>24 || b->post< -24 || b->post>24 || b->attack<0 || b->attack>200 || b->release<0 || b->release>2000 || b->release2<0 || b->release2>2000 || b->sustain<0 || b->sustain>1000 || b->width<0 || b->width>2 || b->mode<0 || b->mode>3 || b->mode!=floorf(b->mode) || (b->solo!=0 && b->solo!=1) || (b->rms!=0 && b->rms!=1) || b->ceiling< -24 || b->ceiling>12 || b->saturation<0 || b->saturation>1 || !isfinite(b->count) || b->count<2 || b->count>DYNAMICS_POINTS || b->count!=floorf(b->count))return 0;
        for(int j=0;j<DYNAMICS_POINTS;j++){DynamicsPoint p=b->points[j];if(!isfinite(p.x)||!isfinite(p.y)||!isfinite(p.bend)||!isfinite(p.shape))return 0;if(j<(int)b->count && (p.x< -72 || p.x>12 || p.y< -72 || p.y>12 || p.bend< -1 || p.bend>1 || p.shape<0 || p.shape>4 || p.shape!=floorf(p.shape) || (j && p.x<=b->points[j-1].x)))return 0;}
        if(b->points[0].x!=-72 || b->points[(int)b->count-1].x!=12)return 0;
    }return 1;
}
float dynamics_curve(const DynamicsBand *b,float x){
    int n=(int)b->count;if(x<=b->points[0].x)return x+b->points[0].y-b->points[0].x;
    if(x>=b->points[n-1].x){DynamicsPoint a=b->points[n-2],z=b->points[n-1];return z.y+(x-z.x)*(z.y-a.y)/(z.x-a.x);}
    int i=0;while(i<n-2 && x>=b->points[i+1].x)i++;
    DynamicsPoint a=b->points[i],z=b->points[i+1];float t=(x-a.x)/(z.x-a.x),k=a.bend;
    if(a.shape==2)t=0;
    else if(a.shape>=3){float steps=2+floorf((k+1)*15);float q=t*steps;t=(floorf(q)+(a.shape==4?(q-floorf(q))*(q-floorf(q))*(3-2*(q-floorf(q))):0))/steps;}
    else {
        if(a.shape==1)t=t*t*(3-2*t);
        /* Bend the segment itself, including horizontal segments. Endpoint-only
           easing cannot bend a flat line because both endpoint values match. */
        float span=fmaxf(fabsf(z.y-a.y),z.x-a.x);
        return clamp(a.y+(z.y-a.y)*t+2*k*span*t*(1-t),-72,12);
    }
    return a.y+(z.y-a.y)*t;
}
static void filter(Biquad *f,float hz,int high){
    int initial=f->frequency==0;
    if(f->frequency!=hz){double w=2*PI*hz/RATE,c=cos(w),a=sin(w)/sqrt(2.),denom=1+a;f->target_a[1]=-2*c/denom;f->target_a[2]=(1-a)/denom;f->target_b[0]=(1+(high?c:-c))*.5/denom;f->target_b[1]=(high?-2:2)*f->target_b[0];f->target_b[2]=f->target_b[0];f->frequency=hz;}
    double slew=initial?1:.0327838995;for(int i=0;i<3;i++){f->a[i]+=(f->target_a[i]-f->a[i])*slew;f->b[i]+=(f->target_b[i]-f->b[i])*slew;}
}
/* Eighth-order Butterworth high-pass, four distinct pole pairs: 48 dB/oct. */
static void cut_filter(Biquad *f,float hz,double q){
    int initial=f->frequency==0;
    if(f->frequency!=hz){double w=2*PI*hz/RATE,c=cos(w),a=sin(w)/(2*q),denom=1+a;
        f->target_a[1]=-2*c/denom;f->target_a[2]=(1-a)/denom;
        f->target_b[0]=(1+c)*.5/denom;f->target_b[1]=-2*f->target_b[0];f->target_b[2]=f->target_b[0];f->frequency=hz;}
    double slew=initial?1:.0327838995;
    for(int i=0;i<3;i++){f->a[i]+=(f->target_a[i]-f->a[i])*slew;f->b[i]+=(f->target_b[i]-f->b[i])*slew;}
}
float dynamics_lowcut_gain(float cutoff,float hz){
    if(cutoff<=0)return 1;if(hz<=0)return 0;
    double r=tan(PI*fminf(80,cutoff)/RATE)/tan(PI*hz/RATE);
    double p=r*r;p*=p;p*=p;p*=p;return (float)(1/sqrt(1+p));
}
static float run(Biquad *f,int side,float x){double y=f->b[0]*x+f->z[side][0];f->z[side][0]=f->b[1]*x-f->a[1]*y+f->z[side][1];f->z[side][1]=f->b[2]*x-f->a[2]*y;if(fabs(f->z[side][0])<1e-25)f->z[side][0]=0;if(fabs(f->z[side][1])<1e-25)f->z[side][1]=0;return (float)y;}
Dynamics *dynamics_create(void){Dynamics *d=calloc(1,sizeof *d);if(d){atomic_init(&d->serial,0);for(int b=0;b<4;b++)for(int i=0;i<DYNAMICS_HISTORY;i++)atomic_init(&d->history[b][i],0);}return d;}
void dynamics_free(Dynamics *d){free(d);}
void dynamics_reset(Dynamics *d){if(!d)return;/* Meter atomics may be concurrently read: never memset them. */memset(d,0,offsetof(Dynamics,serial));atomic_store_explicit(&d->serial,0,memory_order_release);}
size_t dynamics_storage_bytes(void){return sizeof(Dynamics);}
static void coefficients(Dynamics *d,const DynamicsSettings *s){
    for(int x=0;x<2;x++)for(int h=0;h<2;h++)for(int stage=0;stage<2;stage++)filter(&d->split[x][h][stage],x?s->high:fmaxf(s->low,clamp(s->lowcut,0,80)),h);
    for(int h=0;h<2;h++)for(int stage=0;stage<2;stage++)filter(&d->align[h][stage],s->high,h);
    static const double q[]={.5097955791041592,.6013448869350453,.8999762231364156,2.5629154477415064};
    float cut=clamp(s->lowcut,0,80);
    for(int i=0;i<4;i++)cut_filter(&d->cut[i],cut>0?fmaxf(.1f,cut):80,q[i]);
    for(int b=0;b<4;b++){const DynamicsBand *p=&s->band[b];
        if(d->curve_count[b]!=p->count || memcmp(d->curve_cache[b],p->points,sizeof p->points)){memcpy(d->curve_cache[b],p->points,sizeof p->points);d->curve_count[b]=p->count;if(d->ready)d->curve_slew[b]=960;}
        d->attack[b]=time_coefficient(p->attack);d->release[b]=time_coefficient(p->release);d->release2[b]=time_coefficient(p->release2);d->average[b]=time_coefficient(fmaxf(.1f,p->sustain));d->target_pre[b]=amp(p->pre);d->target_post[b]=amp(p->post);d->sat[b]=p->saturation;d->ceil[b]=amp(p->ceiling);}
}
static float process_band(Dynamics *d,const DynamicsBand *p,int b,float f[2]){
    if(p->mode==3){d->rms[b]=d->envelope[b]=0;d->gain[b]=d->slow_gain[b]=1;f[0]=f[1]=0;return 0;}
    const float slew=1.f/960;
    d->width[b]+=(p->width-d->width[b])*slew;d->pre[b]+=(d->target_pre[b]-d->pre[b])*slew;d->post[b]+=(d->target_post[b]-d->post[b])*slew;
    float m=(f[0]+f[1])*.5f,side=(f[0]-f[1])*.5f*d->width[b];f[0]=(m+side)*d->pre[b];f[1]=(m-side)*d->pre[b];
    float level=fmaxf(fabsf(f[0]),fabsf(f[1]));d->rms[b]=d->average[b]*d->rms[b]+(1-d->average[b])*(f[0]*f[0]+f[1]*f[1])*.5f;
    float detect=p->rms?sqrtf(fmaxf(0,d->rms[b])):level;
    float a=detect>d->envelope[b]?d->attack[b]:d->release[b];d->envelope[b]=a*d->envelope[b]+(1-a)*detect;
    float in=db(d->envelope[b]),target=p->mode==1?0:clamp(dynamics_curve(p,in)-in,-72,24);
    /* Second release smooths recovery only; attack follows the first envelope. */
    float g=amp(target);if(g<d->slow_gain[b] || !p->release2)d->slow_gain[b]=g;else d->slow_gain[b]=d->release2[b]*d->slow_gain[b]+(1-d->release2[b])*g;
    if(d->curve_slew[b]){d->gain[b]+=(d->slow_gain[b]-d->gain[b])/96;d->curve_slew[b]--;}else d->gain[b]=d->slow_gain[b];
    for(int side_index=0;side_index<2;side_index++){float y=f[side_index]*d->gain[b]*d->post[b];if(d->sat[b]>0){float c=d->ceil[b],soft=c*tanhf(y/c);y+=d->sat[b]*(soft-y);}f[side_index]=p->mode>=2?0:y;}
    d->peak_in[b]=fmaxf(d->peak_in[b],level);d->peak_out[b]=fmaxf(d->peak_out[b],fmaxf(fabsf(f[0]),fabsf(f[1])));if(fabsf(db(d->gain[b]))>fabsf(d->peak_gain[b]))d->peak_gain[b]=db(d->gain[b]);return level;
}
static unsigned encode(float x){return (unsigned)lroundf(clamp((x+72)*255/96,0,255));}
void dynamics_process(Dynamics *d,const DynamicsSettings *s,float wet,float f[2]){
    if(!d || !s)return;
    if(!d->ready){coefficients(d,s);for(int b=0;b<4;b++){d->width[b]=s->band[b].width;d->pre[b]=amp(s->band[b].pre);d->post[b]=amp(s->band[b].post);d->gain[b]=d->slow_gain[b]=1;}d->wet=wet;d->cut_mix=s->lowcut>0?1:0;d->ready=1;}
    if(!(d->tick%32))coefficients(d,s);
    d->cut_mix+=((s->lowcut>0?1.f:0.f)-d->cut_mix)/960;
    float band[3][2],aligned[2],original[2]={f[0],f[1]};
    for(int side=0;side<2;side++){
        float x=f[side],hp=x;for(int i=0;i<4;i++)hp=run(&d->cut[i],side,hp);
        x+=(hp-x)*d->cut_mix;
        float lo=run(&d->split[0][0][1],side,run(&d->split[0][0][0],side,x));
        float rest=run(&d->split[0][1][1],side,run(&d->split[0][1][0],side,x));
        band[0][side]=run(&d->align[0][1],side,run(&d->align[0][0],side,lo))+run(&d->align[1][1],side,run(&d->align[1][0],side,lo));
        band[1][side]=run(&d->split[1][0][1],side,run(&d->split[1][0][0],side,rest));
        band[2][side]=run(&d->split[1][1][1],side,run(&d->split[1][1][0],side,rest));aligned[side]=band[0][side]+band[1][side]+band[2][side];
    }
    int solo=s->band[0].solo || s->band[1].solo || s->band[2].solo;
    float combined[2]={0};for(int b=0;b<3;b++){process_band(d,&s->band[b],b,band[b]);if(!solo || s->band[b].solo)for(int c=0;c<2;c++)combined[c]+=band[b][c];}
    for(int c=0;c<2;c++)combined[c]=aligned[c]*(1-s->blend)+combined[c]*s->blend;
    process_band(d,&s->band[3],3,combined);
    d->wet+=(clamp(wet,0,1)-d->wet)/960;
    /* LMH blend is phase aligned; outer rack bypass fades to original input. */
    for(int c=0;c<2;c++)f[c]=original[c]*(1-d->wet)+combined[c]*d->wet;
    if(++d->tick%256==0){unsigned serial=atomic_load_explicit(&d->serial,memory_order_relaxed);for(int b=0;b<4;b++){unsigned packed=encode(db(d->peak_in[b]))|(encode(db(d->peak_out[b]))<<8)|(encode(d->peak_gain[b])<<16);atomic_store_explicit(&d->history[b][serial%DYNAMICS_HISTORY],packed,memory_order_relaxed);d->peak_in[b]=d->peak_out[b]=0;d->peak_gain[b]=0;}atomic_store_explicit(&d->serial,serial+1,memory_order_release);}
}
void dynamics_monitor(const Dynamics *d,DynamicsMonitor *m){memset(m,0,sizeof *m);if(!d)return;unsigned end=atomic_load_explicit(&d->serial,memory_order_acquire);m->serial=end;m->count=end<DYNAMICS_HISTORY?end:DYNAMICS_HISTORY;for(unsigned i=0;i<m->count;i++)for(int b=0;b<4;b++){unsigned p=atomic_load_explicit(&d->history[b][(end-m->count+i)%DYNAMICS_HISTORY],memory_order_relaxed);m->input[b][i]=(p&255)*96.f/255-72;m->output[b][i]=((p>>8)&255)*96.f/255-72;m->gain[b][i]=((p>>16)&255)*96.f/255-72;}}
