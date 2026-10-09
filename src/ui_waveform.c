// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"

Color source_rgb(uint32_t rgb) {
    Color color={rgb>>16,(rgb>>8)&255,rgb&255,255};
    Vector3 hsv=ColorToHSV(color);
    /* Keep source identity soft, including colors saved in existing projects. */
    return ColorFromHSV(hsv.x,fminf(hsv.y,.25f),hsv.z);
}

Color clip_foreground(Color fill) {
    return theme_foreground(fill);
}

Color audio_color(int c) {
    if(ui.project.channel_colors[c]) return source_rgb(ui.project.channel_colors[c]);
    float t=fminf(1,log1pf(ui.samples[c].frames/(float)RATE)/log1pf(30));
    /* Short takes start pink; longer takes sweep through violet/cyan toward lime. */
    Color color=ColorFromHSV(320-235*t,.25f,.98f);
    return color;
}

SamplerView *sampler_view(int c) {
    SamplerView *view=&ui.sampler_views[c]; Sample source=ui.originals[c]; Sampler settings=ui.project.sampler[c];
    int changed=!view->valid || view->generation!=ui.sample_generation[c] || view->epoch!=ui.sample_epoch;
    if(changed) waveform_build(&view->wave,source);
    if(changed || !sampler_processing_equal(view->settings,settings)) {
        unsigned offset=llround(source.frames*(double)settings.start);
        Sample crop={.data=source.data?source.data+(size_t)offset*sample_channels(source):NULL,.frames=llround((source.frames-offset)*(double)settings.length),.channels=source.channels};
        view->crop=sample_trim(crop,settings.trim);
        view->offset=source.data?(view->crop.data-source.data)/sample_channels(source):0;
        view->settings=settings; view->generation=ui.sample_generation[c]; view->epoch=ui.sample_epoch; view->valid=1;
        WavePeak peak=waveform_range(&view->wave,source,view->offset,view->offset+view->crop.frames);
        float maximum=fmaxf(-peak.low,peak.high);
        view->gain=(settings.flags&SAMPLE_NORMALIZE) && maximum>0?1/maximum:1;
    }
    return view;
}

int sampler_live_preview(int c) {
    Sampler current=ui.project.sampler[c],applied=ui.sampler_applied[c];
    return !recording_source(PATTERNS+c) && !sampler_processing_equal(current,applied) && current.pitch==applied.pitch && current.stretch==applied.stretch;
}

unsigned sampler_view_frames(int c) {
    unsigned frames=sampler_view(c)->crop.frames;
    return frames?fmax(1,llround(frames*(double)ui.project.sampler[c].time)):0;
}

WavePeak sampler_view_envelope(int c,double position,double width,unsigned frames) {
    SamplerView *view=sampler_view(c); unsigned count=view->crop.frames;
    double ratio=frames?count/(double)frames:0;
    position*=ratio; width*=ratio;
    if(view->settings.flags&SAMPLE_REVERSE) position=count-position;
    WavePeak peak=waveform_envelope_region(&view->wave,ui.originals[c],view->offset,count,position,width);
    if(view->settings.flags&SAMPLE_POLARITY) { float low=peak.low; peak.low=-peak.high; peak.high=-low; }
    peak.low*=view->gain; peak.high*=view->gain; return peak;
}

float audio_view_steps(int c) {
    float seconds=sampler_live_preview(c)?sampler_view_frames(c)/(float)RATE:ui.project.audio_seconds[c];
    return seconds*audio_source_bpm(&ui.project,c)/15/channel_speed(&ui.project,c);
}

float playlist_clip_length(int lane,int clip) {
    int c=ui.project.clips[lane][clip]-PATTERNS-1;
    if(c>=0 && c<CHANNELS && !ui.project.clip_steps[lane][clip] && sampler_live_preview(c)) {
        double remaining=fmax(0,sampler_view_frames(c)/(double)RATE-ui.project.clip_offsets[lane][clip])*audio_source_bpm(&ui.project,c)/15;
        AudioTimeline map; audio_timeline_init(&map,&ui.project,c);
        return audio_timeline_duration(&map,ui.project.clip_starts[lane][clip]*STEPS,remaining);
    }
    return clip_length(&ui.project,lane,clip);
}

Waveform *processed_waveform(int c) {
    Sample sample=ui.samples[c];
    if(!ui.audio_wave_states[c].valid || ui.audio_wave_states[c].sample.data!=sample.data || ui.audio_wave_states[c].sample.frames!=sample.frames ||
       ui.audio_wave_states[c].sample.channels!=sample.channels || ui.audio_wave_states[c].generation!=ui.sample_generation[c] ||
       ui.audio_wave_states[c].epoch!=ui.sample_epoch || !sampler_processing_equal(ui.audio_wave_states[c].settings,ui.sampler_applied[c])) {
        waveform_build(&ui.audio_waves[c],sample);
        ui.audio_wave_states[c].sample=sample; ui.audio_wave_states[c].settings=ui.sampler_applied[c];
        ui.audio_wave_states[c].generation=ui.sample_generation[c]; ui.audio_wave_states[c].epoch=ui.sample_epoch;
        ui.audio_wave_states[c].revision++; ui.audio_wave_states[c].valid=1;
    }
    return &ui.audio_waves[c];
}

double wave_source(const WaveDisplay *view,float x) {
    if(view->timeline) return view->offset+audio_timeline_source(view->timeline,view->start,
        view->start+fmax(0,x-view->origin)/view->pixels_per_step)*view->frames_per_step;
    return (x-view->origin)/view->width*view->sample.frames;
}

void wave_quad(float x,float xx,float top,float next_top,float bottom,float next_bottom,float v,float vv) {
    rlTexCoord2f(.5f,v); rlVertex2f(x,top); rlTexCoord2f(.5f,vv); rlVertex2f(x,bottom);
    rlTexCoord2f(.5f,vv); rlVertex2f(xx,next_bottom); rlTexCoord2f(.5f,v); rlVertex2f(xx,next_top);
}

void draw_waveform(WaveDisplay view,Rectangle area,float left,float right,Color color) {
    if(!view.sample.frames || view.width<=0 || right<=left || area.height<=0) return;
    float step=1/ui_scale(),feather=.5f/ui.font_scale,mid=area.y+area.height*.5f,amplitude=fmaxf(0,area.height*.5f-2/ui.font_scale);
    float previous_x=left,previous_top=mid,previous_bottom=mid;
    int columns=ceilf((right-left)/step);
    rlSetTexture(ui.cable_texture.id); rlBegin(RL_QUADS); rlColor4ub(color.r,color.g,color.b,color.a);
    for(int i=0;i<=columns;i++) {
        float x=fminf(right,left+i*step);
        double source=wave_source(&view,x),width=fabs(wave_source(&view,x+step)-source);
        WavePeak peak;
        if(view.preview) peak=sampler_view_envelope(view.channel,source,width,view.sample.frames);
        else peak=waveform_envelope(view.wave,view.sample,source,width);
        float top=mid-fmaxf(0,peak.high)*amplitude,bottom=mid-fminf(0,peak.low)*amplitude;
        top=fminf(top,mid-feather); bottom=fmaxf(bottom,mid+feather);
        if(i) {
            wave_quad(previous_x,x,previous_top-feather,top-feather,previous_top+feather,top+feather,0,.25f);
            wave_quad(previous_x,x,previous_top+feather,top+feather,previous_bottom-feather,bottom-feather,.5f,.5f);
            wave_quad(previous_x,x,previous_bottom-feather,bottom-feather,previous_bottom+feather,bottom+feather,.75f,1);
        }
        previous_x=x; previous_top=top; previous_bottom=bottom;
    }
    rlEnd(); rlSetTexture(0);
}

void audio_waveform_at(int c,float x,float y,float left,float right,float pixels_per_step,float height,int lane,int clip) {
    Sample sample=ui.samples[c]; Waveform *wave=&ui.audio_waves[c]; int recording=0,preview=sampler_live_preview(c);
    if(preview) sample.frames=sampler_view_frames(c);
    if(ui.recording_ui.active) for(int i=0;i<ui.recording_ui.count;i++) if(ui.recording_ui.takes[i].channel==c) {
        sample=ui.recording_ui.takes[i].sample; wave=&ui.recording_ui.takes[i].wave; recording=1; preview=0; break;
    }
    if(recording) lane=-1;
    if(!sample.frames) return;
    if(!recording && !preview) wave=processed_waveform(c);
    AudioTimeline map; double start=0,offset=0;
    float full=sample.frames/(float)RATE*audio_source_bpm(&ui.project,c)/15/channel_speed(&ui.project,c)*pixels_per_step;
    if(lane>=0) {
        audio_timeline_init(&map,&ui.project,c); start=ui.project.clip_starts[lane][clip]*STEPS; offset=ui.project.clip_offsets[lane][clip]*RATE;
        full=audio_timeline_duration(&map,start,fmax(0,sample.frames-offset)/RATE*audio_source_bpm(&ui.project,c)/15)*pixels_per_step;
    }
    WaveDisplay view={.sample=sample,.wave=wave,.channel=c,.preview=preview,.timeline=lane>=0?&map:NULL,
        .start=start,.offset=offset,.frames_per_step=RATE*15/audio_source_bpm(&ui.project,c),.origin=x,.width=full,.pixels_per_step=pixels_per_step};
    draw_waveform(view,(Rectangle){x,y+14,full,height-14},left,fminf(right,x+full),clip_foreground(audio_color(c)));
}

void audio_waveform(int c,float x,float y,float left,float right,float pixels_per_step,float height) {
    audio_waveform_at(c,x,y-14,left,right,pixels_per_step,height+14,-1,-1);
}
