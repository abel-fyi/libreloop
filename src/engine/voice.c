// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include <math.h>
int voice_settings_valid(VoiceSettings s) {
    return isfinite(s.limit) && s.limit>=0 && s.limit<=128 && s.limit==floorf(s.limit) &&
        (s.mono==0 || s.mono==1) && (s.legato==0 || s.legato==1) && isfinite(s.glide) && s.glide>=0 && s.glide<=2;
}
int voice_policy_enabled(const Project *p,int c) {
    VoiceSettings s=p->voice[c];return !p->channel_audio[c] && (s.limit || s.mono);
}
static void retarget(Voice *to,const Voice *from,VoiceSettings s) {
    float begin=from?from->current_pitch:to->note_pitch;to->fresh=from==0;
    if(from && s.legato && to->instrument==from->instrument) {
        to->device=from->device;to->base_pitch=from->base_pitch;
    }
    to->current_pitch=s.glide && from?begin:to->note_pitch;
    to->glide_left=s.glide && from?(unsigned)fmaxf(1,s.glide*RATE):0;
    to->glide_step=to->glide_left?(to->note_pitch-to->current_pitch)/to->glide_left:0;
}
/* Preserve the sounding legato state before a key slot is reused. A fresh
   Analog attack also gets a short overlap with the retiring voice, rather than
   cutting a nonzero waveform off at the slot boundary. Fixed pool only. */
void player_voice_prepare(Player *p,const Project *pr,int slot) {
    Voice *old=&p->voices[slot];int c=old->channel;
    if(!old->gain || c<0 || c>=pr->channel_count || old->audio_clip)return;
    if(pr->voice[c].mono && pr->voice[c].legato && old->selected){
        Voice *held=0;
        for(int i=0;i<128;i++){Voice *v=&p->voices[i];if(i!=slot && v->gain && v->channel==c && v->held>0 && !v->fade_out && (!held || v->order>held->order))held=v;}
        if(held){held->device=old->device;held->base_pitch=old->base_pitch;held->current_pitch=old->current_pitch;held->selected=1;old->selected=0;return;}
    }
    if(old->instrument==INSTRUMENT_FM && old->fm.engine==2 && pr->fm[c].analog.model){
        for(int i=126;i>=0;i--)if(i!=slot && !p->voices[i].gain){
            p->voices[i]=*old;p->voices[i].held=-1;p->voices[i].selected=0;p->voices[i].remaining=-1;p->voices[i].fade_out=RATE/200;break;
        }
    }
}
void player_voice_on(Player *p,const Project *pr,int slot,float pitch) {
    Voice *v=&p->voices[slot];int c=v->channel;VoiceSettings s=pr->voice[c];
    v->order=++p->voice_order;v->held=1;v->note_pitch=v->base_pitch=v->current_pitch=pitch;v->selected=1;
    if(!s.mono && !s.limit)return;
    Voice *prior=0,*held=0;
    for(int i=0;i<128;i++)if(i!=slot && p->voices[i].gain && p->voices[i].channel==c && !p->voices[i].audio_clip && p->voices[i].selected){
        if(!p->voices[i].fade_out && (!prior || p->voices[i].order>prior->order))prior=&p->voices[i];
    }
    for(int i=0;i<128;i++)if(i!=slot && p->voices[i].gain && p->voices[i].channel==c && p->voices[i].held>0 && !p->voices[i].fade_out && (!held || p->voices[i].order>held->order))held=&p->voices[i];
    if(s.mono){retarget(v,held?(prior?prior:held):0,s);for(int i=0;i<128;i++)if(i!=slot && p->voices[i].channel==c)p->voices[i].selected=0;}
    else if(s.limit) {
        for(;;){int count=0,old=-1;for(int i=0;i<128;i++)if(p->voices[i].gain && p->voices[i].channel==c && !p->voices[i].audio_clip && !p->voices[i].fade_out){
            count++;if(old<0 || p->voices[i].order<p->voices[old].order)old=i;
        }if(count<=(int)s.limit || old<0)break;p->voices[old].gain=0;p->voices[old].held=-1;}
    }
}
void player_voice_off(Player *p,const Project *pr,int slot) {
    Voice *v=&p->voices[slot];if(!v->gain)return;
    if(voice_policy_enabled(pr,v->channel) && !v->audio_clip){if(v->held>0)v->held=0;return;}
    v->held=-1;
    if(v->instrument==INSTRUMENT_FM)instrument_release(&v->device,v->instrument,(InstrumentSettings){&pr->sampler[v->channel],&pr->fm[v->channel]});
    else v->remaining=RATE*.005;
}
void player_voice_tick_pair(Player *p,Player *live,const Project *pr) {
    Player *pools[2]={p,live};int pools_count=live?2:1;
    Voice *chosen[CHANNELS]={0},*previous[CHANNELS]={0};
    unsigned counts[CHANNELS]={0};
    for(int pool=0;pool<pools_count;pool++)for(int i=0;i<128;i++) {
        Player *owner=pools[pool];Voice *v=&owner->voices[i];int c=v->channel;
        if(!v->gain || c>=pr->channel_count || v->audio_clip || v->fade_out || !voice_policy_enabled(pr,c))continue;
        /* Adopt already sounding notes if the page is changed during playback. */
        if(!v->order){v->order=++owner->voice_order;v->held=1;v->selected=1;}
        if(v->remaining>=0 && --v->remaining<=0){if(v->held>0)v->held=0;else if(v->instrument!=INSTRUMENT_FM)v->gain=0;}
        if(!pr->voice[c].glide && v->glide_left){v->current_pitch=v->note_pitch;v->glide_left=0;}
        counts[c]++;
        if(!pr->voice[c].mono)continue;
        if(v->selected && (!previous[c] || v->order<previous[c]->order))previous[c]=v;
        if(v->held>0 && (!chosen[c] || v->order>chosen[c]->order))chosen[c]=v;
    }
    /* Commands may release and replace the foreground in one pool while a
       held anchor lives in the other. Resolve that overlap before releasing
       pending voices, even if on() could not see the peer pool. */
    for(int c=0;c<pr->channel_count;c++)if(pr->voice[c].mono && chosen[c] && chosen[c]->fresh && previous[c]==chosen[c]){
        Voice *prior=0;int overlap=0;
        for(int pool=0;pool<pools_count;pool++)for(int i=0;i<128;i++){Voice *v=&pools[pool]->voices[i];
            if(v!=chosen[c] && v->gain && v->channel==c && !v->fade_out && v->held>=0){overlap|=v->held>0;if(!prior || v->order>prior->order)prior=v;}}
        if(overlap)previous[c]=prior;
    }
    for(int c=0;c<pr->channel_count;c++)if(pr->voice[c].mono && chosen[c] && chosen[c]!=previous[c]) {
        Voice *v=chosen[c],*old=previous[c];VoiceSettings s=pr->voice[c];
        if(!s.legato || !old) {
            instrument_start(&v->device,v->instrument,(InstrumentSettings){&pr->sampler[c],&pr->fm[c]},(InstrumentNote){.frequency=440*exp2((v->note_pitch-69)/12.),.speed=exp2((v->note_pitch-60)/12.),.velocity=v->gain});
            v->base_pitch=v->note_pitch;
        }
        retarget(v,old,s);
        v->selected=1;
    }
    for(int c=0;c<pr->channel_count;c++)if(chosen[c])chosen[c]->fresh=0;
    for(int c=0;c<pr->channel_count;c++)if(pr->voice[c].limit && !pr->voice[c].mono && !pr->channel_audio[c]) {
        while(counts[c]>pr->voice[c].limit){Voice *old=0;
            for(int pool=0;pool<pools_count;pool++)for(int i=0;i<128;i++){Voice *v=&pools[pool]->voices[i];
                if(v->gain && v->channel==c && !v->audio_clip && !v->fade_out && (!old || v->order<old->order))old=v;}
            if(!old)break;old->gain=0;old->held=-1;counts[c]--;
        }
    }
    for(int pool=0;pool<pools_count;pool++)for(int i=0;i<128;i++) {
        Player *owner=pools[pool];Voice *v=&owner->voices[i];int c=v->channel;
        if(!v->gain || c>=pr->channel_count || v->audio_clip || v->fade_out || !voice_policy_enabled(pr,c))continue;
        if(pr->voice[c].mono && chosen[c])v->selected=v==chosen[c];
        if(v->held==0){
            if(pr->voice[c].mono && !v->selected)v->gain=0;
            else if(v->instrument==INSTRUMENT_FM)instrument_release(&v->device,v->instrument,(InstrumentSettings){&pr->sampler[c],&pr->fm[c]});
            else v->remaining=RATE*.005;
            v->held=-1;
        }
    }
}
void player_voice_tick(Player *p,const Project *pr){player_voice_tick_pair(p,0,pr);}
double player_voice_pitch(Voice *v) {
    if(v->glide_left){v->current_pitch+=v->glide_step;if(!--v->glide_left)v->current_pitch=v->note_pitch;}
    return v->current_pitch==v->base_pitch?1:exp2((v->current_pitch-v->base_pitch)/12.);
}
