// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include <math.h>
#include <string.h>
ArpSettings arp_default(void){return (ArpSettings){.time=1,.gate=.8f,.range=1,.repeat=1};}
static int whole(float x,int low,int high){return isfinite(x) && x>=low && x<=high && x==floorf(x);}
int arp_valid(ArpSettings s){return whole(s.direction,0,ARP_DIRECTIONS-1) && isfinite(s.time) && s.time>=.125f && s.time<=16 && isfinite(s.gate) && s.gate>=.01f && s.gate<=1 && whole(s.range,1,4) && whole(s.repeat,1,8) && whole(s.chord,0,ARP_CHORDS-1) && (s.slide==0 || s.slide==1);}
const char *arp_direction_name(unsigned d){static const char *names[]={"Off","Up","Down","Up-Down","Bounce","Random"};return d<ARP_DIRECTIONS?names[d]:names[0];}
const char *arp_chord_name(unsigned c){static const char *names[]={"Auto (sustain)","Auto","None (octaves)","Major","Minor","Major 7","Minor 7","Dominant 7","Sus 2","Sus 4","Diminished","Augmented"};return c<ARP_CHORDS?names[c]:names[0];}
int player_arp_on(Player *p,const Project *pr,int channel,uint32_t id,int pitch,float velocity,double remaining,int lane,int pattern,int note_id){
    if(channel<0 || channel>=pr->channel_count || !pr->arp[channel].direction || pitch<0 || pitch>127 || !isfinite(velocity) || velocity<=0 || velocity>1 || !isfinite(remaining) || remaining< -1)return 0;
    int slot=-1;
    for(int i=0;i<128;i++){ArpHeld *h=&p->arp.held[i];if(h->active && h->id==id){slot=i;break;}if(!h->active && slot<0)slot=i;}
    if(slot<0){slot=0;for(int i=1;i<128;i++)if(p->arp.held[i].order<p->arp.held[slot].order)slot=i;}
    if(!p->arp.held[slot].active)p->arp.count++;else p->arp.counts[p->arp.held[slot].channel]--;
    p->arp.counts[channel]++;
    p->arp.held[slot]=(ArpHeld){.active=1,.id=id,.channel=channel,.pitch=pitch,.velocity=velocity,.remaining=remaining,.lane=lane,.pattern=pattern,.note_id=note_id,.order=++p->voice_order};return 1;
}
int player_arp_off(Player *p,uint32_t id){int found=0;for(int i=0;i<128;i++)if(p->arp.held[i].active && p->arp.held[i].id==id){p->arp.counts[p->arp.held[i].channel]--;p->arp.held[i].active=0;p->arp.count--;found=1;}return found;}
void player_arp_clear(Player *p,int channel){
    if(channel<0){memset(&p->arp,0,sizeof p->arp);return;}
    for(int i=0;i<128;i++)if(p->arp.held[i].active && p->arp.held[i].channel==channel){p->arp.counts[p->arp.held[i].channel]--;p->arp.held[i].active=0;p->arp.count--;}
    if(channel<CHANNELS)memset(&p->arp.channels[channel],0,sizeof p->arp.channels[channel]);
}
static uint32_t sequence_id(const Voice *v){return 0x10000u+(v->lane+1)*PATTERNS*NOTES+v->pattern*NOTES+v->note_id-1;}
void player_arp_adopt(Player *p,const Project *pr,int live){
    for(int c=pr->channel_count;c<CHANNELS;c++)if(p->arp.counts[c] || p->arp.channels[c].running)player_arp_clear(p,c);
    for(int i=0;i<128;i++){Voice *v=&p->voices[i];if(v->gain && !v->arp_generated && !v->audio_clip && !v->fade_out && v->held>0 && v->channel<pr->channel_count && pr->arp[v->channel].direction){
        double remaining=v->remaining>=0?v->remaining:live?-1:RATE*15/pr->bpm;
        player_arp_on(p,pr,v->channel,live?(uint32_t)i:sequence_id(v),(int)v->note_pitch,v->gain,remaining,v->lane,v->pattern,v->note_id);v->gain=0;
    }}
}
static int free_voice(Player *p){
    int oldest=-1;
    for(int i=0;i<128;i++){Voice *v=&p->voices[i];if(!v->gain)return i;if(!v->audio_clip && (v->held<0 || v->arp_generated) && (oldest<0 || v->order<p->voices[oldest].order))oldest=i;}
    return oldest;
}
static Voice *current(Player *p,int c){int slot=p->arp.channels[c].slot-1;if(slot<0 || slot>=128)return NULL;Voice *v=&p->voices[slot];return v->gain && v->arp_generated && v->channel==c && !v->fade_out?v:NULL;}
static void release(Player *p,const Project *pr,int c){Voice *v=current(p,c);if(v)player_voice_off(p,pr,(int)(v-p->voices));p->arp.channels[c].slot=0;}
static int start(Player *p,const Project *pr,const FMSettings *fm,const ArpHeld *h,int pitch,int generated,int slot){
    if(slot<0)slot=free_voice(p);if(slot<0)return -1;
    player_voice_prepare(p,pr,slot);
    Voice *v=&p->voices[slot];*v=(Voice){.channel=h->channel,.instrument=pr->instrument[h->channel],.gain=h->velocity,.remaining=generated?-1:h->remaining,.lane=h->lane,.pattern=h->pattern,.note_id=h->note_id,.arp_generated=generated};
    instrument_start(&v->device,v->instrument,(InstrumentSettings){&pr->sampler[h->channel],&fm[h->channel]},(InstrumentNote){.frequency=440*exp2((pitch-69)/12.),.speed=exp2((pitch-60)/12.),.velocity=h->velocity});
    player_voice_on(p,pr,slot,pitch);return slot;
}
static int candidate(ArpChannel *a,ArpSettings s,int count){
    int total=count*(int)s.range*(int)s.repeat,index=0;uint64_t step=a->step++;
    switch((int)s.direction){
    case ARP_DOWN:index=total-1-step%total;break;
    case ARP_UP_DOWN:{int period=total>1?2*(total-1):1;index=step%period;if(index>=total)index=period-index;break;}
    case ARP_BOUNCE:{int period=2*total;index=step%period;if(index>=total)index=period-1-index;break;}
    case ARP_RANDOM:if(!a->random)a->random=0x6d2b79f5u;a->random=a->random*1664525u+1013904223u;index=(a->random>>8)%total;break;
    default:index=step%total;break;
    }
    return index/(count*(int)s.repeat)*count+index%count;
}
int player_arp_tick(Player *p,Player *live,const Project *pr,const FMSettings *fm){
    Player *pools[2]={p,live};int pools_count=live?2:1,changed=0;
    for(int c=0;c<pr->channel_count;c++){
        ArpChannel *a=&p->arp.channels[c];ArpSettings s=pr->arp[c];
        if(!p->arp.counts[c] && (!live || !live->arp.counts[c]) && !a->running)continue;
        const ArpHeld *notes[128]={0},*latest=NULL;int count=0;
        for(int pool=0;pool<pools_count;pool++)for(int i=0;i<128;i++){
            ArpHeld *h=&pools[pool]->arp.held[i];if(!h->active || h->channel!=c)continue;
            if(!s.direction){
                start(pools[pool],pr,fm,h,h->pitch,0,pool==1 && h->id<128?(int)h->id:-1);
                h->active=0;pools[pool]->arp.count--;pools[pool]->arp.counts[c]--;changed=1;continue;
            }
            if(h->pitch>=0 && h->pitch<128 && (!notes[h->pitch] || h->order>notes[h->pitch]->order))notes[h->pitch]=h;
            if(!latest || h->order>latest->order)latest=h;
        }
        if(!latest){if(a->running){release(p,pr,c);memset(a,0,sizeof *a);changed=1;}continue;}
        if(!a->running){a->running=1;a->pending=1;a->phase=0;a->step=0;a->random=0x6d2b79f5u+c;a->slot=0;}
        int pitches[128];const ArpHeld *roots[128];
        if(s.chord<=ARP_AUTO){for(int pitch=0;pitch<128;pitch++)if(notes[pitch]){pitches[count]=pitch;roots[count++]=notes[pitch];}}
        else {
            static const int intervals[ARP_CHORDS-ARP_OCTAVES][4]={{0,-1},{0,4,7,-1},{0,3,7,-1},{0,4,7,11},{0,3,7,10},{0,4,7,10},{0,2,7,-1},{0,5,7,-1},{0,3,6,-1},{0,4,8,-1}};
            for(int i=0;i<4;i++){int offset=intervals[(int)s.chord-ARP_OCTAVES][i];if(offset<0)break;int pitch=latest->pitch+offset;if(pitch<=127){pitches[count]=pitch;roots[count++]=latest;}}
        }
        int single=s.chord==ARP_AUTO_SUSTAIN && count==1 && s.range==1;
        if(single!=a->single){a->phase=0;a->step=0;a->pending=1;}a->single=single;
        if((single && (a->pending || a->pitch!=pitches[0] || a->root_order!=roots[0]->order)) || (!single && a->pending)){
            int index=single?0:candidate(a,s,count),tries=0;
            while(pitches[index%count]+12*(index/count)>127 && tries++<4096)index=candidate(a,s,count);
            int pitch=pitches[index%count]+12*(index/count);if(pitch>127){index=0;pitch=pitches[0];}const ArpHeld *h=roots[index%count];Voice *old=current(p,c);
            if(s.slide && old){
                old->note_pitch=pitch;old->gain=h->velocity;old->current_pitch=s.slide && pr->voice[c].glide?old->current_pitch:pitch;
                old->glide_left=pr->voice[c].glide?(unsigned)fmaxf(1,pr->voice[c].glide*RATE):0;
                old->glide_step=old->glide_left?(pitch-old->current_pitch)/old->glide_left:0;
                old->lane=h->lane;old->pattern=h->pattern;old->note_id=h->note_id;
            }else {release(p,pr,c);int slot=start(p,pr,fm,h,pitch,1,-1);a->slot=slot+1;}
            a->pitch=pitch;a->root_order=h->order;a->pending=0;changed=1;
        }
        if(!single && a->phase>=s.time*s.gate && a->phase<s.time && a->slot){release(p,pr,c);changed=1;}
        if(!single){a->phase+=pr->bpm/(15.*RATE);if(a->phase>=s.time-1e-10){a->phase=fmax(0,fmod(a->phase,s.time));if(a->phase>s.time-1e-10)a->phase=0;a->pending=1;}}
    }
    for(int pool=0;pool<pools_count;pool++)for(int i=0;i<128;i++){
        ArpHeld *h=&pools[pool]->arp.held[i];if(h->active && h->remaining>=0 && --h->remaining<=0){h->active=0;pools[pool]->arp.count--;pools[pool]->arp.counts[h->channel]--;}
    }
    return changed;
}
