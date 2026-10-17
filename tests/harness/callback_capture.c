/* SPDX-License-Identifier: GPL-3.0-only
 * Captures the frozen original's actual audio.c callback without opening devices.
 * The miniaudio device state is a test double only; DSP and mailbox are original. */
#include "../../src/platform/audio_miniaudio.c"
#include "arrangement.h"
#ifdef LIBRELOOP_TEST_RT_GUARD
#include "realtime_guard.h"
#endif
static Project fixture;
static Sample source[CHANNELS];
static FILE *pcm_file,*state_file;
static void capture(unsigned frames,unsigned total) {
    float out[1024];
#ifdef LIBRELOOP_TEST_RT_GUARD
    realtime_guard_enter();
#endif
    callback(NULL,out,NULL,frames);
#ifdef LIBRELOOP_TEST_RT_GUARD
    realtime_guard_leave();
#endif
    refresh_view();
    for(unsigned i=0;i<frames*2;i++) if(!isfinite(out[i])) exit(3);
    if(fwrite(out,2*sizeof(float),frames,pcm_file)!=frames) exit(4);
    fprintf(state_file,"%u %llu %lld %d %u %u %llu\n",total,(unsigned long long)player.frame,
        (long long)player.last_step,playing,preview_remaining,pending.write-pending.read,(unsigned long long)atomic_load(&acknowledged));
}
int main(int argc,char **argv) {
    if(argc!=3)return 1;
    int mode=atoi(argv[1]);unsigned block=atoi(argv[2]);
    if(mode<0 || mode>6 || (block!=64 && block!=512))return 1;
    project_new(&fixture);fixture.channel_count=2;
    effects=effects_create(INSERTS+1);if(!effects)return 2;
    player_reset(&player);player_reset(&live);player.effects=live.effects=effects;
    for(int c=0;c<2;c++) {
        source[c]=(Sample){.frames=48000,.channels=c?2:1,.data=calloc(48000*(c?2:1),sizeof(float))};
        if(!source[c].data)return 2;
        for(unsigned i=0;i<source[c].frames;i++)for(unsigned side=0;side<source[c].channels;side++)
            source[c].data[i*source[c].channels+side]=(i%113<3?.5f:((int)(i%127)-63)*.004f)*(side?-.7f:1);
        fixture.volume[c]=.7f;fixture.route[c]=c+1;
    }
    fixture.insert_output[0]=2;fixture.insert_width[1]=1.4f;
    fixture.effect_type[2][0]=EFFECT_CHORUS;fixture.effect_mix[2][0]=.2f;
    for(int accent=0;accent<2;accent++)for(int i=0;i<1200;i++)
        clicks[accent][i]=.25f*sinf(6.2831853f*(accent?1800:1200)*i/RATE)*expf(-i/180.f);
    if(mode==0) {
        for(int c=0;c<2;c++)fixture.notes[0][c][0]=(Note){60+c*7,100,0,4};
        fixture.clips[0][0]=1;fixture.clip_steps[0][0]=16;
    } else if(mode==2 || mode==3 || mode==4) {
        fixture.instrument[0]=INSTRUMENT_FM;fixture.fm[0]=mode==2?fm_legacy():fm_factory(mode==3?0:5);
    } else if(mode==6) {
        fixture.channel_audio[0]=1;fixture.audio_seconds[0]=1;fixture.clips[0][0]=PATTERNS+1;
        fixture.clip_offsets[0][0]=.125f;
        int a=automation_create(&fixture,(ParameterTarget){PARAM_MASTER_PITCH,0,0},"Master pitch",16);
        fixture.automations[a].points[0].value=.5f;fixture.automations[a].points[1].value=.75f;
        arrangement_place(&fixture,1,0,AUTOMATION_SOURCE+a,16);
    }
    ready=0;audio_channels(&fixture,source);
    audio_update(&fixture,mode==0 || mode==6,1,0,1,mode==1?.4f:1,0,mode==6?1.5f:0,mode==6?10.25f:0);
    audio_metronome(mode==0);
    if(mode==5) {
        recording.count=3;recording.devices_count=1;recording.active=1;
        recording.buses[0]=1;recording.buses[1]=2;recording.buses[2]=0;
        recording.sources[0]=0;recording.sources[1]=recording.sources[2]=-1;
        recording.inputs[0].pcm=calloc(RECORD_RING_FRAMES*2,sizeof(float));
        for(int i=0;i<3;i++){recording.takes[i].pcm=calloc(RECORD_RING_FRAMES*2,sizeof(float));recording.io.active[recording.buses[i]]=1;}
        recording.io.input=record_input;recording.io.output=record_output;
        audio_update(&fixture,1,1,0,1,1,0,0,0);
    }
    pcm_file=fopen("expected.f32","wb");state_file=fopen("expected-state.txt","w");
    if(!pcm_file || !state_file)return 2;
    FILE *takes[3]={0};if(mode==5)for(int i=0;i<3;i++){char name[32];snprintf(name,sizeof name,"take-%d.f32",i);takes[i]=fopen(name,"wb");if(!takes[i])return 2;}
    ready=1;ma_atomic_device_state_set(&device.state,ma_device_state_started);
    for(unsigned at=0;at<96000;at+=block) {
        unsigned n=96000-at<block?96000-at:block;
        /* Original mutex held across these callback boundaries delayed this
           edit. Stage the same publication until the same boundary, without
           putting any lock in the rewrite callback. */
        if(at==40960) control_begin();
        if(at==45056) control_end();
        if(at==0 && mode>=1 && mode<=4)audio_key_velocity(7,0,69,90);
        if(at==8192 && mode==1) {ready=0;audio_preview(source[1]);ready=1;}
        if(at==16384){fixture.bpm=80;fixture.master_pitch=2;audio_update(&fixture,mode==0 || mode==5 || mode==6,1,0,0,.6f,0,mode==6?1.5f:0,mode==6?10.25f:0);}
        if(at==24576 && mode>=1 && mode<=4)audio_key_velocity(7,0,69,0);
        if(at==32768 && mode>=1 && mode<=4)audio_note(0,(Note){60,100,0,2});
        if(at==40960){fixture.volume[0]=.2f;audio_update(&fixture,mode==0 || mode==5 || mode==6,1,0,0,.6f,0,mode==6?1.5f:0,mode==6?10.25f:0);}
        if(at==49152 && mode==6) {
            fixture.clip_offsets[0][0]=.25f;audio_update(&fixture,1,1,0,0,.6f,0,1.5f,10.25f);
        }
        if(mode==5) {
            float in[1024];for(unsigned i=0;i<n;i++){in[i*2]=((int)((at+i)%101)-50)*.004f;in[i*2+1]=-.7f*in[i*2];}
            ma_device d={0};d.pUserData=&recording.inputs[0];
#ifdef LIBRELOOP_TEST_RT_GUARD
            realtime_guard_enter();
#endif
            capture_callback(&d,NULL,in,n);
#ifdef LIBRELOOP_TEST_RT_GUARD
            realtime_guard_leave();
#endif
        }
        capture(n,at+n);
        if(mode==5)for(int i=0;i<3;i++){float out[1024];unsigned frames=audio_record_read(i,out,n);if(frames!=n)return 5;fwrite(out,2*sizeof(float),frames,takes[i]);}
    }
    ready=0;ma_atomic_device_state_set(&device.state,ma_device_state_uninitialized);
    fclose(pcm_file);fclose(state_file);for(int i=0;i<3;i++)if(takes[i])fclose(takes[i]);
    if(mode==5){recording.active=0;record_release();}
    effects_free(effects);for(int c=0;c<2;c++)sample_free(source[c]);return 0;
}
