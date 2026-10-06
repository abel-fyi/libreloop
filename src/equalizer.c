// SPDX-License-Identifier: GPL-3.0-only
#include "equalizer.h"
#include <math.h>
#include <string.h>
#define PI 3.14159265358979323846
EQSettings equalizer_default(void) {
    return (EQSettings){.bands={{60,0,.707f,EQ_LOW_SHELF},{150,0,1,EQ_BELL},{400,0,1,EQ_BELL},{1000,0,1,EQ_BELL},{2500,0,1,EQ_BELL},{6000,0,1,EQ_BELL},{12000,0,.707f,EQ_HIGH_SHELF}}};
}
EQSettings equalizer_legacy(void) {
    EQSettings s=equalizer_default(); s.bands[0]=(EQBand){100,0,.707f,EQ_LOW_SHELF}; s.bands[1]=(EQBand){500,0,1,EQ_BELL};
    s.bands[2]=(EQBand){2500,0,1,EQ_BELL}; s.bands[3]=(EQBand){10000,0,.707f,EQ_HIGH_SHELF};
    for(int i=4;i<EQ_BANDS;i++) s.bands[i].shape=EQ_OFF; return s;
}
const char *equalizer_shape_name(unsigned shape) {
    static const char *names[]={"Bell","Low Shelf","High Shelf","Low Cut","High Cut","Off"};
    return shape<EQ_SHAPES?names[shape]:"Unknown";
}
int equalizer_valid(EQSettings s) {
    for(int i=0;i<EQ_BANDS;i++) {
        EQBand b=s.bands[i];
        if(b.shape>=EQ_SHAPES || !isfinite(b.frequency) || b.frequency<20 || b.frequency>20000 || !isfinite(b.gain) || fabsf(b.gain)>18 || !isfinite(b.q) || b.q<.2f || b.q>10) return 0;
    }
    return 1;
}
void equalizer_reset(Equalizer *eq) { memset(eq,0,sizeof *eq); }
static void coefficients(EQFilter *f,EQBand b) {
    if(b.shape==EQ_OFF) { f->b[0]=1; f->b[1]=f->b[2]=f->a[0]=f->a[1]=0; return; }
    double A=pow(10,b.gain/40.),w=2*PI*b.frequency/48000,c=cos(w),alpha=sin(w)/(2*b.q);
    double b0,b1,b2,a0,a1,a2;
    if(b.shape==EQ_LOW_SHELF || b.shape==EQ_HIGH_SHELF) {
        double t=2*sqrt(A)*alpha;
        if(b.shape==EQ_LOW_SHELF) {
            b0=A*((A+1)-(A-1)*c+t); b1=2*A*((A-1)-(A+1)*c); b2=A*((A+1)-(A-1)*c-t);
            a0=(A+1)+(A-1)*c+t; a1=-2*((A-1)+(A+1)*c); a2=(A+1)+(A-1)*c-t;
        } else {
            b0=A*((A+1)+(A-1)*c+t); b1=-2*A*((A-1)+(A+1)*c); b2=A*((A+1)+(A-1)*c-t);
            a0=(A+1)-(A-1)*c+t; a1=2*((A-1)-(A+1)*c); a2=(A+1)-(A-1)*c-t;
        }
    } else if(b.shape==EQ_LOW_CUT || b.shape==EQ_HIGH_CUT) {
        double sign=b.shape==EQ_LOW_CUT?1:-1;
        b0=(1+sign*c)/2; b1=-sign*(1+sign*c); b2=b0;
        a0=1+alpha; a1=-2*c; a2=1-alpha;
    } else {
        b0=1+alpha*A; b1=-2*c; b2=1-alpha*A; a0=1+alpha/A; a1=-2*c; a2=1-alpha/A;
    }
    f->b[0]=b0/a0; f->b[1]=b1/a0; f->b[2]=b2/a0; f->a[0]=a1/a0; f->a[1]=a2/a0;
}
void equalizer_process(Equalizer *eq,EQSettings s,float wet,float stereo[2]) {
    if(!eq->ready) {
        eq->current=eq->target=s; eq->ready=1;
        for(int i=0;i<EQ_BANDS;i++) eq->shape[i]=s.bands[i].shape;
    }
    eq->target=s;
    /* Coefficients update at 750 Hz, with a 20 ms control slew. */
    if(!eq->tick) for(int i=0;i<EQ_BANDS;i++) {
        EQBand *b=&eq->current.bands[i],t=s.bands[i];
        float k=1-expf(-64.f/(48000*.02f));
        b->frequency=expf(logf(b->frequency)+(logf(t.frequency)-logf(b->frequency))*k);
        b->gain+=(t.gain-b->gain)*k; b->q+=(t.q-b->q)*k;
        if(!eq->transition[i] && eq->shape[i]!=t.shape) {
            memset(&eq->next[i],0,sizeof eq->next[i]); eq->next_shape[i]=t.shape; eq->transition[i]=1;
        }
        b->shape=eq->shape[i]; coefficients(&eq->filters[i],*b);
        if(eq->transition[i]) { EQBand next=*b; next.shape=eq->next_shape[i]; coefficients(&eq->next[i],next); }
    }
    eq->tick=(eq->tick+1)%64;
    eq->wet+=(wet-eq->wet)*(1-expf(-1.f/(48000*.02f)));
    for(int side=0;side<2;side++) {
        double dry=stereo[side],x=dry;
        for(int i=0;i<EQ_BANDS;i++) {
            double old=x,new=x;
            for(int pass=0;pass<(eq->transition[i]?2:1);pass++) {
                unsigned shape=pass?eq->next_shape[i]:eq->shape[i];
                if(shape==EQ_OFF) continue;
                EQFilter *f=pass?&eq->next[i]:&eq->filters[i]; double y=f->b[0]*x+f->z[side][0];
                f->z[side][0]=f->b[1]*x-f->a[0]*y+f->z[side][1]; f->z[side][1]=f->b[2]*x-f->a[1]*y;
                if(fabs(f->z[side][0])<1e-25) f->z[side][0]=0;
                if(fabs(f->z[side][1])<1e-25) f->z[side][1]=0;
                if(pass) new=y; else old=y;
            }
            x=eq->transition[i]?old*eq->transition[i]+new*(1-eq->transition[i]):old;
        }
        stereo[side]=(float)(dry+(x-dry)*eq->wet);
    }
    for(int i=0;i<EQ_BANDS;i++) if(eq->transition[i]) {
        eq->transition[i]=fmaxf(0,eq->transition[i]-1.f/960);
        if(!eq->transition[i]) { eq->filters[i]=eq->next[i]; eq->shape[i]=eq->next_shape[i]; }
    }
}
float equalizer_response(EQSettings s,float frequency) {
    double w=2*PI*frequency/48000,db=0;
    for(int i=0;i<EQ_BANDS;i++) {
        if(s.bands[i].shape==EQ_OFF) continue;
        EQFilter f={0}; coefficients(&f,s.bands[i]);
        double br=f.b[0]+f.b[1]*cos(w)+f.b[2]*cos(2*w),bi=-f.b[1]*sin(w)-f.b[2]*sin(2*w);
        double ar=1+f.a[0]*cos(w)+f.a[1]*cos(2*w),ai=-f.a[0]*sin(w)-f.a[1]*sin(2*w);
        db+=10*log10(fmax(1e-20,(br*br+bi*bi)/(ar*ar+ai*ai)));
    }
    return (float)db;
}
