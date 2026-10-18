// SPDX-License-Identifier: GPL-3.0-only
#include "dynamics.h"
#include "dynamics_curve.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static float lowcut_response(float hz,float cutoff){
    DynamicsSettings s=dynamics_default();s.lowcut=cutoff;
    Dynamics *d=dynamics_create();double power=0;
    for(int i=0;i<96000;i++){
        float x=.02f*(float)sin(6.283185307179586*hz*i/48000),f[2]={x,x};
        dynamics_process(d,&s,1,f);if(i>=48000)power+=(double)f[0]*f[0];
    }
    dynamics_free(d);return sqrt(power/48000)*1.4142135623730951/.02;
}
int main(void){
    const float tones[]={20,40,80,160};
    for(unsigned i=0;i<4;i++){
        float measured=lowcut_response(tones[i],80),expected=dynamics_lowcut_gain(80,tones[i]);
        printf("Low Cut 80 Hz / %.0f Hz: %.3f dB\n",tones[i],20*log10f(measured));
        CHECK(fabsf(20*log10f(measured/expected))<.2f);
    }
    CHECK(lowcut_response(40,80)<.0041f && lowcut_response(160,80)>.999f);
    DynamicsSettings cutoff_settings=dynamics_default();cutoff_settings.lowcut=81;CHECK(!dynamics_valid(cutoff_settings));
    for(int db=-72;db<=12;db++)CHECK(fabsf(dynamics_level_db(dynamics_level_position(db))-db)<.001f);
    CHECK(dynamics_level_position(0)>.49f && dynamics_level_position(0)<.51f);
    DynamicsSettings s=dynamics_default();CHECK(dynamics_valid(s));for(int i=0;i<3;i++)CHECK(dynamics_valid(dynamics_factory(i)));
    for(int i=-90;i<=12;i++)CHECK(fabsf(dynamics_curve(&s.band[0],i)-fminf(i,0))<.0001f);
    for(int band=0;band<4;band++)for(int db=-72;db<=24;db++)CHECK(fabsf(dynamics_curve(&s.band[band],db)-fminf(db,0))<.0001f);
    for(int segment=0;segment<2;segment++){
        DynamicsBand copy=s.band[0];float x=dynamics_curve_handle_input(&copy,segment);
        float lo=dynamics_level_position(copy.points[segment].x),hi=dynamics_level_position(copy.points[segment+1].x);
        CHECK(fabsf(dynamics_level_position(x)-(lo+hi)*.5f)<1e-6f);
        float straight=dynamics_curve(&copy,x);dynamics_curve_bend(&copy,segment,.5f);CHECK(dynamics_curve(&copy,x)>straight);
        dynamics_curve_bend(&copy,segment,-.5f);CHECK(dynamics_curve(&copy,x)<straight);
        CHECK(dynamics_curve(&copy,copy.points[segment].x)==copy.points[segment].y);
        CHECK(dynamics_curve(&copy,copy.points[segment+1].x)==copy.points[segment+1].y);
    }
    DynamicsBand *b=&s.band[0];int p=dynamics_curve_insert(b,-18,-18);CHECK(p==1);dynamics_curve_move(b,p,-18,-18,0,0);dynamics_curve_bend(b,p,.5f);CHECK(dynamics_valid(s));CHECK(dynamics_curve_delete(b,p));CHECK(!dynamics_curve_delete(b,0));
    for(int i=0;i<8;i++)dynamics_curve_insert(b,-60+i*8,-60+i*8);CHECK(b->count==8);CHECK(dynamics_curve_insert(b,-10,-10)<0);CHECK(dynamics_valid(s));
    s=dynamics_default();s.band[3].points[1]=(DynamicsPoint){-12,-12,0,0};s.band[3].points[2].y=-6;CHECK(fabsf(dynamics_curve(&s.band[3],0)+9)<.001f);
    s.band[3].points[1].x=NAN;CHECK(!dynamics_valid(s));s=dynamics_default();
    CHECK(dynamics_monitor_speed(0)==.85f && dynamics_monitor_speed(-1)==.25f && dynamics_monitor_speed(1)==4);
    CHECK(dynamics_monitor_position(0,-1)==1 && dynamics_monitor_position(0,1)==1);
    CHECK(fabsf(dynamics_monitor_position(255,0)-.15f)<1e-6f);
    CHECK(dynamics_monitor_position(255,-1)>.7f && dynamics_monitor_position(255,1)<0);
    float bounds[4];dynamics_band_bounds(200,2500,0,bounds);CHECK(bounds[0]==0 && bounds[3]==1);
    float original=bounds[1];dynamics_band_bounds(200,2500,100,bounds);
    CHECK(bounds[0]>0 && bounds[0]<bounds[1] && bounds[1]==original);
    dynamics_band_bounds(40,2500,200,bounds);CHECK(bounds[0]==bounds[1] && bounds[1]<bounds[2]);
    /* A single input level fills from the origin only as far as its x value,
       with every column capped by the drawn curve, including held segments. */
    s=dynamics_default();s.band[0].points[1]=(DynamicsPoint){-12,-12,0,0};s.band[0].points[2].y=-12;
    for(int shape=0;shape<5;shape++){s.band[0].points[0].shape=shape;s.band[0].points[0].bend=.4f;
        for(int i=0;i<=1000;i++){float at=i/1000.f,h=dynamics_meter_height(&s.band[0],0,at);CHECK(h>=0 && h<=dynamics_level_position(dynamics_curve(&s.band[0],dynamics_level_db(at)))+1e-6f);if(at>dynamics_level_position(0))CHECK(h==0);}
    }
    CHECK(dynamics_meter_height(&s.band[0],-72,.1f)==0);
    CHECK(dynamics_meter_height(&s.band[0],-6,dynamics_level_position(-18))>0);
    CHECK(dynamics_meter_height(&s.band[0],-24,dynamics_level_position(-18))==0);
    s=dynamics_default();
    Dynamics *d=dynamics_create(),*other=dynamics_create();CHECK(d && other);float reference[4096][2];
    DynamicsSettings crossed=dynamics_default(),limited=dynamics_default();
    crossed.low=40;crossed.lowcut=80;crossed.band[0].solo=1;limited=crossed;limited.low=80;
    for(int i=0;i<4096;i++){
        float a[2]={.1f*sinf(i*.01f),.1f*cosf(i*.02f)},b[2];memcpy(b,a,sizeof a);
        dynamics_process(d,&crossed,1,a);dynamics_process(other,&limited,1,b);CHECK(!memcmp(a,b,sizeof a));
    }
    dynamics_reset(d);dynamics_reset(other);
    for(int i=0;i<48000;i++){float f[2]={0};dynamics_process(d,&s,1,f);CHECK(f[0]==0 && f[1]==0);}
    /* LR4 complementary split: unity magnitude across the spectrum. */
    const float frequencies[]={30,100,200,500,1000,2500,5000,15000};
    for(unsigned k=0;k<sizeof frequencies/sizeof *frequencies;k++){dynamics_reset(d);double in=0,out=0;for(int i=0;i<48000;i++){float x=.1f*sinf(i*6.28318530718f*frequencies[k]/48000);float f[2]={x,x};dynamics_process(d,&s,1,f);CHECK(f[0]==f[1] && isfinite(f[0]));if(i>24000){in+=x*x;out+=f[0]*f[0];}}CHECK(fabs(10*log10(out/in))<.05);}
    /* Exact bypass and stereo-linked gain, including unbalanced stereo input. */
    dynamics_reset(d);for(int i=0;i<1000;i++){float f[2]={.2f,-.1f};dynamics_process(d,&s,0,f);CHECK(f[0]==.2f && f[1]==-.1f);}
    s=dynamics_default();s.band[3].points[1]=(DynamicsPoint){-18,-18,0,0};s.band[3].points[2].y=-18;s.band[3].attack=0;s.band[3].release=100;
    dynamics_reset(d);double compressed=0;for(int i=0;i<48000;i++){float x=.8f*sinf(i*.1f);float f[2]={x,x*.5f};dynamics_process(d,&s,1,f);CHECK(fabsf(f[1]*2-f[0])<1e-6f);if(i>24000)compressed=fmax(compressed,fabsf(f[0]));}CHECK(compressed<.14);
    DynamicsMonitor m;dynamics_monitor(d,&m);CHECK(m.count==187 && m.gain[3][m.count-1]<-10 && m.output[3][m.count-1]<m.input[3][m.count-1]);
    /* Block-independent changes and fresh state isolation. */
    for(int pass=0;pass<2;pass++){Dynamics *r=pass?other:d;dynamics_reset(r);s=dynamics_factory(1);for(int i=0;i<4096;i++){if(i==1024)s.band[0].width=.5f;if(i==2048)s.band[3].attack=2;if(i==3072)s.band[3].points[1].y=-30;float f[2]={.1f*sinf(i*.1f),.2f*cosf(i*.07f)};dynamics_process(r,&s,1,f);CHECK(isfinite(f[0]) && isfinite(f[1]));if(!pass)memcpy(reference[i],f,sizeof f);else CHECK(!memcmp(reference[i],f,sizeof f));}}
    s=dynamics_default();s.band[0].width=0;s.band[0].solo=1;dynamics_reset(d);for(int i=0;i<4800;i++){float f[2]={sinf(i*.01f),0};dynamics_process(d,&s,1,f);CHECK(f[0]==f[1]);}
    dynamics_reset(d);dynamics_monitor(d,&m);CHECK(!m.count);float silence[2]={0};dynamics_process(d,&s,1,silence);CHECK(!silence[0] && !silence[1]);
    /* RMS/dual-release response and repeated crossover sweeps stay finite. */
    s=dynamics_factory(1);s.band[3].rms=1;s.band[3].release2=450;dynamics_reset(d);
    for(int i=0;i<48000;i++){if(!(i%127)){s.low=40+(i%960);s.high=1100+(i%14000);s.lowcut=(i/127)%3==0?0:(i/127)%3==1?80:20;}float x=i<24000?.5f*sinf(i*.17f):0;float f[2]={x,x};dynamics_process(d,&s,1,f);CHECK(isfinite(f[0]) && isfinite(f[1]) && fabsf(f[0])<4 && f[0]==f[1]);}
    s=dynamics_default();s.band[3].saturation=1;s.band[3].ceiling=-6;dynamics_reset(d);
    for(int i=0;i<1000;i++){float f[2]={2*sinf(i*.13f),2*sinf(i*.13f)};dynamics_process(d,&s,1,f);CHECK(fabsf(f[0])<=powf(10,-6.f/20)+1e-6f);}
    s=dynamics_default();s.band[3].mode=2;dynamics_reset(d);for(int i=0;i<1000;i++){float f[2]={.3f,.3f};dynamics_process(d,&s,1,f);CHECK(f[0]==0 && f[1]==0);}
    dynamics_reset(d);s=dynamics_default();for(int i=0;i<48000*6;i++){float f[2]={0};dynamics_process(d,&s,1,f);}
    dynamics_monitor(d,&m);CHECK(m.count==DYNAMICS_HISTORY && m.serial==1125);
    CHECK(dynamics_monitor_position(m.count-1,-1)<0 && dynamics_monitor_position(m.count-5,-1)>=0);
    dynamics_free(d);dynamics_free(other);printf("Dynamics: curve editing, unity crossovers, compression, stereo linking, meters, bypass, mono bass and deterministic replay passed (%zu bytes).\n",dynamics_storage_bytes());return 0;
}
