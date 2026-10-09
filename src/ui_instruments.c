// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"

int sampler_toggle(int x,int y,int width,const char *name,int active) {
    int over=hover(x,y,width,22);
    if(over) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    Color foreground=over?theme_foreground(ui_theme.hover):ink;
    if(over) ui_surface((Rectangle){x,y,width,22},ui_theme.hover);
    circle(x+9,y+11,7,over?foreground:ui_theme.border); circle(x+9,y+11,5,cell);
    if(active) circle(x+9,y+11,3,accent);
    label(name,x+24,y+4,13,foreground);
    return over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void sampler_switch(int x,int y,int width,const char *name,uint8_t *flags,int bit) {
    if(sampler_toggle(x,y,width,name,*flags&bit)) *flags^=bit;
}

void fm_graph_update_at(float x,float y) {
    if(ui.fm_graph_channel<0 || ui.fm_graph_channel>=ui.project.channel_count) return;
    FMSettings *s=&ui.project.fm[ui.fm_graph_channel];
    if(ui.fm_graph_drag>=4) {
        int stage=ui.fm_graph_drag-4; float slot=(ui.fm_graph_area.width-16)/4;
        float position=fmaxf(0,fminf(1,(x-ui.fm_graph_area.x-8-slot*(stage+.1f))/(slot*.8f)));
        s->dx7.value[ui.fm_dx_drag_base+stage]=roundf((1-position)*99);
        s->dx7.value[ui.fm_dx_drag_base+4+stage]=roundf(fmaxf(0,fminf(99,(1-(y-ui.fm_graph_area.y-6)/fmaxf(1,ui.fm_graph_area.height-12))*99)));
        return;
    }
    if(ui.fm_graph_drag==3) {
        float position=fmaxf(0,fminf(1,(x-ui.fm_graph_area.x-8)/(ui.fm_graph_area.width-16)));
        s->lfo_rate=.1f*powf(120,position);
        float amount=fmaxf(0,fminf(1,(ui.fm_graph_area.y+ui.fm_graph_area.height-22-y)/(ui.fm_graph_area.height-34)));
        if(ui.fm_motion_target) s->tremolo=amount; else s->vibrato=amount*100;
        return;
    }
    int field=ui.fm_graph_drag==0?0:ui.fm_graph_drag==1?1:3;
    const ParameterDescriptor *info=parameter_descriptor(ui.fm_envelope_ids[ui.fm_tab][field]);
    float slot=(ui.fm_graph_area.width-24)/3;
    float position=fmaxf(0,fminf(1,(x-ui.fm_graph_area.x-8-slot*ui.fm_graph_drag)/(.9f*slot)));
    float value=expm1f(position*log1pf(info->high*100))/100;
    *(float *)fm_parameter_pointer(s,info->id)=fmaxf(info->low,fminf(info->high,value));
    if(ui.fm_graph_drag==1) {
        float sustain=fmaxf(0,fminf(1,(ui.fm_graph_area.y+ui.fm_graph_area.height-22-y)/(ui.fm_graph_area.height-34)));
        *(float *)fm_parameter_pointer(s,ui.fm_envelope_ids[ui.fm_tab][2])=sustain;
    }
}

void fm_graph_update(void) {
    Rect r=ui.windows.editors[4].rect; fm_graph_update_at(ui.mouse.x-r.x,ui.mouse.y-r.y);
}

void fm_control(FMSettings *s,unsigned id,const char *caption,int x,int y) {
    const ParameterDescriptor *info=parameter_descriptor(id);
    float *value=(float *)fm_parameter_pointer(s,id);
    int logarithmic=info->kind==PARAMETER_LOGARITHMIC ||
        (info->low>0 && (!strcmp(caption,"Attack") || !strcmp(caption,"Decay") || !strcmp(caption,"Release")));
    int fixed_coarse=id>=PARAM_DX7_FIRST && id<PARAM_DX7_FIRST+126 &&
        (id-PARAM_DX7_FIRST)%21==18 && s->dx7.value[id-PARAM_DX7_FIRST-1];
    knob_style(x,y,value,info->low,info->high,info->initial,info->name,
        fixed_coarse?KNOB_FIXED_COARSE:logarithmic?KNOB_LOGARITHMIC:info->low<0?KNOB_CENTER:KNOB_NORMAL);
    if(ui.control_drag==value && logarithmic) ui.control_logarithmic=1;
    label(caption,x-text_width(caption,11)/2,y+17,11,muted);
    float display=displayed_value(value,*value);
    const char *shown;
    if(id>=PARAM_DX7_FIRST && id<=PARAM_DX7_LAST) {
        int index=id-PARAM_DX7_FIRST,field=index%21;
        if(index>=DX7_NATIVE_PARAMETERS) shown=TextFormat("%.1f%%",display);
        else if(index<126 && field==18) {
            int base=index-field;
            double fine=s->dx7.value[base+19]/100;
            if(s->dx7.value[base+17]) {
                double frequency=pow(10,((int)display&3)+fine);
                shown=frequency>=1000?TextFormat("%.2f kHz",frequency/1000):
                    TextFormat(frequency<10?"%.2f Hz":"%.0f Hz",frequency);
            } else shown=TextFormat("%.2fx",fmaxf(.5f,display)*(1+fine));
        }
        else if(index<126 && field==8) { static const char *notes[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}; int key=(int)display+17; shown=TextFormat("%s%d",notes[key%12],key/12-1); }
        else if(index<126 && (field==11 || field==12)) { static const char *curves[]={"-Lin","-Exp","+Exp","+Lin"}; shown=curves[(int)display]; }
        else if(index==136 || index==141) shown=display>=.5f?"On":"Off";
        else if(index==142) { static const char *waves[]={"Triangle","Saw down","Saw up","Square","Sine","S&H"}; shown=waves[(int)display]; }
        else if(index<126 && field==20) shown=TextFormat("%+.0f",display-7);
        else if(index==134) shown=TextFormat("%.0f",display+1);
        else if(index==144) shown=TextFormat("%+.0f st",display-24);
        else shown=TextFormat("%.0f",display);
    }
    else if(id==PARAM_FM_FILTER_CUTOFF) shown=TextFormat(display>=1000?"%.2g kHz":"%.0f Hz",display>=1000?display/1000:display);
    else if(id==PARAM_FM_CARRIER_RATIO || id==PARAM_FM_ATTACK_RATIO) shown=TextFormat("%.2gx",display);
    else if(id==PARAM_FM_BODY_PITCH) shown=TextFormat("%+.1f st",display);
    else if(id==PARAM_FM_CARRIER_DETUNE || id==PARAM_FM_BODY_DETUNE || id==PARAM_FM_ATTACK_DETUNE) shown=TextFormat("%+.0f ct",display);
    else if(id==PARAM_FM_ATTACK || id==PARAM_FM_DECAY || id==PARAM_FM_RELEASE || id==PARAM_FM_MOD_ATTACK || id==PARAM_FM_MOD_DECAY || id==PARAM_FM_MOD_RELEASE ||
            id==PARAM_FM_ATTACK_ATTACK || id==PARAM_FM_ATTACK_DECAY || id==PARAM_FM_ATTACK_RELEASE || id==PARAM_FM_LFO_FADE)
        shown=display<1?TextFormat("%.0f ms",display*1000):TextFormat("%.2g s",display);
    else shown=TextFormat("%.2g",display);
    label(shown,x-text_width(shown,10)/2,y+31,10,ink);
    if(id>=PARAM_DX7_FIRST+DX7_NATIVE_PARAMETERS && id<=PARAM_DX7_LAST &&
       (hover(x-16,y-16,32,32) || ui.control_drag==value))
        snprintf(ui.status,sizeof ui.status,"Key tracking: 0%% fixed, 50%% half an octave per octave, 100%% follows notes. Anchor: middle C (C4).");
}

void fm_envelope_graph(FMSettings *s,Rectangle plot) {
    DrawRectangleRec(plot,bg);
    float slot=(plot.width-24)/3,base=plot.y+plot.height-22,range=plot.height-34;
    float times[3]={*fm_parameter_pointer(s,ui.fm_envelope_ids[ui.fm_tab][0]),*fm_parameter_pointer(s,ui.fm_envelope_ids[ui.fm_tab][1]),*fm_parameter_pointer(s,ui.fm_envelope_ids[ui.fm_tab][3])};
    int fields[]={0,1,3}; Vector2 handles[3];
    float sustain=*fm_parameter_pointer(s,ui.fm_envelope_ids[ui.fm_tab][2]);
    for(int i=0;i<3;i++) {
        const ParameterDescriptor *info=parameter_descriptor(ui.fm_envelope_ids[ui.fm_tab][fields[i]]);
        float t=log1pf(times[i]*100)/log1pf(info->high*100);
        handles[i]=(Vector2){plot.x+8+slot*i+.9f*slot*t,i==0?base-range:i==1?base-sustain*range:base};
        DrawLineEx((Vector2){plot.x+8+slot*i,plot.y+8},(Vector2){plot.x+8+slot*i,base},1,ui_theme.grid_minor);
    }
    smooth_line((Vector2){plot.x+8,base},handles[0],1.5f,accent);
    Vector2 previous=handles[0];
    for(int i=1;i<=40;i++) {
        float t=i/40.f,level=ui.fm_tab?sustain+(1-sustain)*expf(-4.6051702f*t):1-(1-sustain)*t;
        Vector2 point={handles[0].x+(handles[1].x-handles[0].x)*t,base-level*range};
        smooth_line(previous,point,1.5f,accent); previous=point;
    }
    smooth_line(previous,handles[1],1.5f,accent);
    Vector2 release={plot.x+8+slot*2,handles[1].y};
    smooth_line(handles[1],release,1.5f,accent); smooth_line(release,handles[2],1.5f,accent);
    const char *captions[]={"Attack","Decay / sustain","Release"};
    for(int i=0;i<3;i++) {
        circle(handles[i].x,handles[i].y,4,ink); circle(handles[i].x,handles[i].y,2,accent);
        label(captions[i],plot.x+8+slot*i,base+7,10,muted);
        if(hover(handles[i].x-7,handles[i].y-7,14,14)) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.fm_graph_drag=i; ui.fm_graph_channel=ui.instrument_channel; ui.fm_graph_area=plot; ui.input_enabled=0; }
        }
    }
    if(hover(plot.x,plot.y,plot.width,plot.height)) snprintf(ui.status,sizeof ui.status,"Drag envelope points: left/right changes time; the middle point also changes sustain. Knobs offer exact entry and automation.");
}

void fm_legacy_editor(float width,float height) {
    int c=ui.instrument_channel; FMSettings *s=&ui.project.fm[c];
    label(fit_text(ui.project.channel_names[c],width-298,14),12,30,14,ink); channel_controls(c,width);
    label("Custom three-operator FM",12,56,11,muted);
    const char *tabs[]={"Sound","Body","Attack","Motion"}; float tabw=(width-24)/4;
    for(int i=0;i<4;i++) if(button(tabs[i],12+i*tabw,78,tabw-3,26,ui.fm_tab==i)) ui.fm_tab=i;
    float column=(width-32)/4;
    const unsigned head[4][4]={
        {PARAM_FM_CARRIER_RATIO,PARAM_FM_CARRIER_DETUNE,PARAM_FM_VELOCITY,0},
        {PARAM_FM_BODY_PITCH,PARAM_FM_BODY_DETUNE,PARAM_FM_RATIO,PARAM_FM_DEPTH},
        {PARAM_FM_ATTACK_RATIO,PARAM_FM_ATTACK_DETUNE,PARAM_FM_ATTACK_DEPTH,PARAM_FM_VELOCITY},
        {PARAM_FM_LFO_RATE,PARAM_FM_VIBRATO,PARAM_FM_TREMOLO,PARAM_FM_LFO_FADE}
    };
    const char *captions[4][4]={{"Pitch","Fine","Velocity",""},{"Pitch","Fine","Harmonic","FM amount"},{"Pitch","Fine","FM amount","Velocity"},{"Speed","Pitch motion","Volume motion","Fade in"}};
    const char *descriptions[]={"Audible oscillator and volume envelope","Long-lived tone modulation","Short, bright attack modulation","A separate, per-note LFO for pitch and volume"};
    label(descriptions[ui.fm_tab],20,113,12,muted);
    for(int i=0;i<4;i++) if(head[ui.fm_tab][i]) fm_control(s,head[ui.fm_tab][i],captions[ui.fm_tab][i],20+column*(i+.5f),139);
    if(ui.fm_tab==0) {
        int x=20+column*3;
        if(button(s->routing?"Stacked":"Body + Attack",x,131,column-8,24,0)) open_context(20,c,(Vector2){ui.mouse.x+ui.windows.editors[4].rect.x,ui.mouse.y+ui.windows.editors[4].rect.y});
        if(hover(x,131,column-8,24) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(&s->routing,0,1,0,"FM routing");
        label("Routing",x+8,159,11,muted);
    }
    if(ui.fm_tab<3) {
        const char *names[]={"Attack","Decay","Sustain","Release"};
        label("Envelope",20,184,12,muted);
        for(int i=0;i<4;i++) fm_control(s,ui.fm_envelope_ids[ui.fm_tab][i],names[i],20+column*(i+.5f),207);
        fm_envelope_graph(s,(Rectangle){12,252,width-24,height-294});
    } else {
        const char *shapes[]={"Sine","Triangle","Saw","Square"};
        if(button(shapes[(int)s->lfo_shape],12,184,112,24,0)) open_context(19,c,(Vector2){ui.mouse.x+ui.windows.editors[4].rect.x,ui.mouse.y+ui.windows.editors[4].rect.y});
        if(hover(12,184,112,24) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(&s->lfo_shape,0,3,0,"LFO shape");
        if(button("Pitch",132,184,72,24,!ui.fm_motion_target)) ui.fm_motion_target=0;
        if(button("Volume",208,184,80,24,ui.fm_motion_target)) ui.fm_motion_target=1;
        Rectangle plot={12,216,width-24,height-258}; DrawRectangleRec(plot,bg);
        float base=plot.y+plot.height-22,range=plot.height-34,mid=plot.y+(plot.height-22)/2;
        float amount=ui.fm_motion_target?s->tremolo:s->vibrato/100;
        Vector2 previous={plot.x+8,mid};
        for(int i=0;i<(int)plot.width-16;i++) {
            float t=i/(plot.width-16)*s->lfo_rate*.5f;
            Vector2 point={plot.x+8+i,mid-fm_lfo_value(t,(int)s->lfo_shape)*range*.42f};
            if(i) smooth_line(previous,point,1.5f,Fade(accent,.35f+amount*.65f));
            previous=point;
        }
        Vector2 handle={plot.x+8+logf(s->lfo_rate/.1f)/logf(120)*(plot.width-16),base-amount*range};
        circle(handle.x,handle.y,5,ink); circle(handle.x,handle.y,3,accent);
        label("Drag: speed / amount",plot.x+8,base+7,10,muted);
        if(hover(plot.x,plot.y,plot.width,plot.height)) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            snprintf(ui.status,sizeof ui.status,"Drag the LFO graph horizontally for speed, vertically for %s amount; choose a waveform above",ui.fm_motion_target?"volume":"pitch");
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.fm_graph_drag=3; ui.fm_graph_channel=c; ui.fm_graph_area=plot; fm_graph_update_at(ui.mouse.x,ui.mouse.y); ui.input_enabled=0; }
        }
    }
    if(button("Factory",12,height-34,98,24,0)) open_context(21,c,(Vector2){ui.mouse.x+ui.windows.editors[4].rect.x,ui.mouse.y+ui.windows.editors[4].rect.y});
    if(button("Load",114,height-34,64,24,0)) preset_action(PRESET_FM,c,0,0);
    if(button("Save",182,height-34,64,24,0)) preset_action(PRESET_FM,c,0,1);
    double active=audio_key_position(127,c);
    if(button(active>=0?"Release C4":"Play C4",width-112,height-34,100,24,active>=0)) { ui.channel=c; audio_key(127,c,60,active<0); }
}

const char *fm_sound_name(const FMSettings *s) {
    for(int i=0;i<FM_FACTORY_COUNT;i++) { FMSettings factory=fm_factory(i); if(!memcmp(s,&factory,sizeof factory)) return fm_factory_name(i); }
    return "Edited sound";
}

void fm_footer(int c,float width,float height) {
    if(button("Factory",12,height-34,98,24,0)) open_context(21,c,(Vector2){ui.mouse.x+ui.windows.editors[4].rect.x,ui.mouse.y+ui.windows.editors[4].rect.y});
    if(button("Load",114,height-34,64,24,0)) preset_action(PRESET_FM,c,0,0);
    if(button("Save",182,height-34,64,24,0)) preset_action(PRESET_FM,c,0,1);
    int active=audio_key_position(127,c)>=0;
    if(button(active?"Release C4":"Play C4",width-112,height-34,100,24,active)) { ui.channel=c; audio_key(127,c,60,!active); }
}

void fm_controls_row(FMSettings *s,const unsigned *ids,const char *const *names,int count,float width,int y) {
    float column=(width-32)/count;
    for(int i=0;i<count;i++) fm_control(s,ids[i],names[i],20+column*(i+.5f),y);
}

void dx7_envelope_graph(FMSettings *s,int base,Rectangle plot) {
    DrawRectangleRec(plot,bg); float width=(plot.width-16)/4,height=fmaxf(1,plot.height-12);
    Vector2 previous={plot.x+8,plot.y+height};
    for(int i=0;i<4;i++) {
        float rate=s->dx7.value[base+i],level=s->dx7.value[base+4+i];
        Vector2 point={plot.x+8+width*(i+.1f+.8f*(1-rate/99)),plot.y+6+height*(1-level/99)};
        smooth_line(previous,point,1.5f,accent); circle(point.x,point.y,3,ink); previous=point;
        if(hover(point.x-7,point.y-7,14,14)) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND); snprintf(ui.status,sizeof ui.status,"Drag stage %d: left/right changes rate, up/down changes level",i+1);
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.fm_graph_drag=4+i; ui.fm_graph_channel=ui.instrument_channel; ui.fm_graph_area=plot; ui.fm_dx_drag_base=base; ui.input_enabled=0; }
        }
    }
}

void fm_dx_editor(float width,float height) {
    int c=ui.instrument_channel; FMSettings *s=&ui.project.fm[c];
    label(fit_text(ui.project.channel_names[c],width-298,14),12,30,14,ink); channel_controls(c,width);
    label(fit_text(fm_sound_name(s),width-160,11),12,56,11,muted);
    float tab=(width-24)/7;
    for(int i=0;i<7;i++) if(button(i==6?"Global":TextFormat("Op %d",i+1),12+i*tab,78,tab-3,26,ui.fm_dx_operator==i)) ui.fm_dx_operator=i;
    if(ui.fm_dx_operator<6) {
        int base=(5-ui.fm_dx_operator)*21;
        label(TextFormat("Operator %d",ui.fm_dx_operator+1),20,113,12,muted);
        unsigned head[]={PARAM_DX7_FIRST+base+16,PARAM_DX7_FIRST+base+18,PARAM_DX7_FIRST+base+19,PARAM_DX7_FIRST+base+20,PARAM_DX7_FIRST+base+15,PARAM_DX7_FIRST+DX7_NATIVE_PARAMETERS+base/21};
        const char *head_names[]={"Output","Coarse","Fine","Detune","Velocity","Tracking"}; fm_controls_row(s,head,head_names,6,width,136);
        if(button("Envelope",12,184,100,24,!ui.fm_dx_page)) ui.fm_dx_page=0;
        if(button("Keyboard",116,184,100,24,ui.fm_dx_page)) ui.fm_dx_page=1;
        float *mode=&s->dx7.value[base+17];
        if(button(*mode?"Hz at C4":"Ratio",width-112,184,100,24,0)) {
            *mode=1-*mode; s->dx7.value[DX7_NATIVE_PARAMETERS+base/21]=*mode?0:100;
            if(*mode) s->dx7.value[base+18]=(int)s->dx7.value[base+18]&3;
        }
        if(hover(width-112,184,100,24) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(mode,0,1,0,"Operator frequency mode");
        if(!ui.fm_dx_page) {
            unsigned rates[4],levels[4]; for(int i=0;i<4;i++) { rates[i]=PARAM_DX7_FIRST+base+i; levels[i]=PARAM_DX7_FIRST+base+4+i; }
            const char *rnames[]={"Rate 1","Rate 2","Rate 3","Release rate"},*lnames[]={"Level 1","Level 2","Sustain level","Release level"};
            fm_controls_row(s,rates,rnames,4,width,226); fm_controls_row(s,levels,lnames,4,width,292);
            dx7_envelope_graph(s,base,(Rectangle){12,336,width-24,height-378});
        } else {
            const int fields[]={8,9,10,14,11,12,13,15}; const char *names[]={"Breakpoint","Left depth","Right depth","Amp motion","Left curve","Right curve","Key rate","Velocity"};
            for(int row=0;row<2;row++) { unsigned ids[4]; for(int i=0;i<4;i++) ids[i]=PARAM_DX7_FIRST+base+fields[row*4+i]; fm_controls_row(s,ids,names+row*4,4,width,226+row*66); }
            label("Curves: 0 -linear, 1 -exponential, 2 +exponential, 3 +linear",20,344,11,muted);
        }
    } else {
        unsigned head[]={PARAM_DX7_FIRST+134,PARAM_DX7_FIRST+135,PARAM_DX7_FIRST+144,PARAM_DX7_FIRST+136};
        const char *names[]={"Algorithm","Feedback","Transpose","Osc sync"}; label("Routing and tuning",20,113,12,muted); fm_controls_row(s,head,names,4,width,136);
        if(button("Motion",12,184,100,24,!ui.fm_dx_page)) ui.fm_dx_page=0;
        if(button("Pitch envelope",116,184,140,24,ui.fm_dx_page)) ui.fm_dx_page=1;
        if(!ui.fm_dx_page) {
            unsigned a[]={PARAM_DX7_FIRST+137,PARAM_DX7_FIRST+138,PARAM_DX7_FIRST+139,PARAM_DX7_FIRST+140},b[]={PARAM_DX7_FIRST+142,PARAM_DX7_FIRST+141,PARAM_DX7_FIRST+143};
            const char *an[]={"Speed","Delay","Pitch amount","Volume amount"},*bn[]={"Waveform","Key sync","Pitch sense"};
            fm_controls_row(s,a,an,4,width,226); fm_controls_row(s,b,bn,3,width,292);
            label("Waves: triangle, saw down, saw up, square, sine, sample & hold",20,344,11,muted);
        } else {
            unsigned rates[4],levels[4]; for(int i=0;i<4;i++) { rates[i]=PARAM_DX7_FIRST+126+i; levels[i]=PARAM_DX7_FIRST+130+i; }
            const char *rn[]={"Rate 1","Rate 2","Rate 3","Release rate"},*ln[]={"Level 1","Level 2","Sustain level","Release level"};
            fm_controls_row(s,rates,rn,4,width,226); fm_controls_row(s,levels,ln,4,width,292);
            dx7_envelope_graph(s,126,(Rectangle){12,336,width-24,height-378});
        }
    }
    fm_footer(c,width,height);
}

void fm_analog_editor(float width,float height) {
    int c=ui.instrument_channel; FMSettings *s=&ui.project.fm[c];
    label(fit_text(ui.project.channel_names[c],width-298,14),12,30,14,ink); channel_controls(c,width);
    label(fit_text(fm_sound_name(s),width-160,11),12,56,11,muted);
    const char *tabs[]={"Sound","Amplitude","Filter","Motion"}; float tab=(width-24)/4;
    for(int i=0;i<4;i++) if(button(tabs[i],12+i*tab,78,tab-3,26,ui.fm_analog_tab==i)) ui.fm_analog_tab=i;
    if(ui.fm_analog_tab==0) {
        unsigned a[]={PARAM_FM_CARRIER_RATIO,PARAM_FM_ANALOG_DETUNE,PARAM_FM_ANALOG_MIX,PARAM_FM_ANALOG_SUB,PARAM_FM_ANALOG_NOISE},b[]={PARAM_FM_ANALOG_PULSE,PARAM_FM_ANALOG_PWM,PARAM_FM_ANALOG_CHORUS,PARAM_FM_VELOCITY};
        const char *an[]={"Pitch","Detune","Osc 2","Sub","Noise"},*bn[]={"Pulse width","PWM","Chorus","Velocity"};
        label("Oscillators",20,113,12,muted); fm_controls_row(s,a,an,5,width,136);
        const char *waves[]={"Saw","Pulse","Triangle","Sine"};
        if(button(waves[(int)s->analog.wave],12,184,112,24,0)) open_context(23,c,(Vector2){ui.mouse.x+ui.windows.editors[4].rect.x,ui.mouse.y+ui.windows.editors[4].rect.y});
        if(hover(12,184,112,24) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(&s->analog.wave,0,3,0,"Oscillator waveform");
        fm_controls_row(s,b,bn,4,width,226); label("PWM moves the Pulse shape. Chorus spreads the sound in stereo.",20,296,11,muted);
    } else if(ui.fm_analog_tab==1 || ui.fm_analog_tab==2) {
        ui.fm_tab=ui.fm_analog_tab==1?0:1;
        if(ui.fm_analog_tab==2) {
            unsigned a[]={PARAM_FM_FILTER_CUTOFF,PARAM_FM_FILTER_RESONANCE,PARAM_FM_FILTER_ENV,PARAM_FM_VELOCITY}; const char *names[]={"Cutoff","Resonance","Envelope","Velocity"};
            label("Filter",20,113,12,muted); fm_controls_row(s,a,names,4,width,136);
        } else label("Volume envelope",20,113,12,muted);
        unsigned ids[4]; for(int i=0;i<4;i++) ids[i]=ui.fm_envelope_ids[ui.fm_tab][i]; const char *names[]={"Attack","Decay","Sustain","Release"};
        fm_controls_row(s,ids,names,4,width,207); fm_envelope_graph(s,(Rectangle){12,252,width-24,height-294});
    } else {
        unsigned ids[]={PARAM_FM_LFO_RATE,PARAM_FM_VIBRATO,PARAM_FM_TREMOLO,PARAM_FM_LFO_FADE}; const char *names[]={"Speed","Pitch motion","Volume motion","Fade in"};
        label("Per-note modulation",20,113,12,muted); fm_controls_row(s,ids,names,4,width,136);
        const char *waves[]={"Sine","Triangle","Saw","Square"};
        if(button(waves[(int)s->lfo_shape],12,184,112,24,0)) open_context(19,c,(Vector2){ui.mouse.x+ui.windows.editors[4].rect.x,ui.mouse.y+ui.windows.editors[4].rect.y});
        if(hover(12,184,112,24) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(&s->lfo_shape,0,3,0,"LFO shape");
        label("Pitch motion adds vibrato; volume motion adds tremolo. PWM follows this LFO.",20,228,11,muted);
    }
    fm_footer(c,width,height);
}

void fm_editor(float width,float height) {
    FMSettings *s=&ui.project.fm[ui.instrument_channel];
    if(s->engine==1) fm_dx_editor(width,height); else if(s->engine==2) fm_analog_editor(width,height); else fm_legacy_editor(width,height);
    const char *engines[]={"Custom FM","Six-op FM","Analog"};
    if(button(engines[(int)s->engine],width-132,52,120,22,0)) open_context(22,ui.instrument_channel,(Vector2){ui.mouse.x+ui.windows.editors[4].rect.x,ui.mouse.y+ui.windows.editors[4].rect.y});
    if(hover(width-132,52,120,22)) snprintf(ui.status,sizeof ui.status,"Choose the synthesis engine; factory presets select their own engine");
}

void sampler(float width,float height) {
    int c=ui.instrument_channel;
    Sampler *settings=&ui.project.sampler[c];
    label(fit_text(channel_caption(c),width-298,14),12,30,14,ink);
    channel_controls(c,width);
    label(fit_text(!ui.originals[c].frames?"Drop a sample from the Browser":ui.project.paths[c][0]?GetFileName(ui.project.paths[c]):"Built-in sample",width-298,12),12,54,12,muted);
    float right=width/2+6,groupw=width/2-18;
    label("Sample effects",20,82,12,muted);
    sampler_switch(20,100,groupw-16,"Normalize",&settings->flags,SAMPLE_NORMALIZE);
    sampler_switch(20,128,groupw-16,"Reverse",&settings->flags,SAMPLE_REVERSE);
    sampler_switch(20,156,groupw-16,"Reverse polarity",&settings->flags,SAMPLE_POLARITY);
    if(hover(20,100,groupw-16,22)) snprintf(ui.status,sizeof ui.status,"Normalize: bring the processed sample peak to full scale");
    if(hover(20,128,groupw-16,22)) snprintf(ui.status,sizeof ui.status,"Reverse: play the cropped sample backwards");
    if(hover(20,156,groupw-16,22)) snprintf(ui.status,sizeof ui.status,"Polarity: flip the waveform vertically");
    label("Sample processing",right+8,82,12,muted);
    knob_style(right+28,108,&settings->pitch,-12,12,0,"Process pitch (semitones)",KNOB_CENTER);
    knob_style(right+84,108,&settings->time,.25f,4,1,"Time multiplier",KNOB_CENTER);
    label("Pitch shift",right+4,128,11,muted); label("Time",right+71,128,11,muted);
    if(button(settings->stretch?"Stretch":"Resample",right+116,98,groupw-128,22,0)) open_context(13,c,(Vector2){ui.mouse.x+ui.windows.editors[4].rect.x,ui.mouse.y+ui.windows.editors[4].rect.y});
    DrawTriangle((Vector2){right+groupw-23,107},(Vector2){right+groupw-19,112},(Vector2){right+groupw-15,107},muted);
    label("Mode",right+116,128,11,muted);
    if(sampler_toggle(right+8,146,groupw-16,"Fit to tempo",settings->fit_bpm>0)) {
        settings->fit_bpm=settings->fit_bpm?0:ui.project.bpm;
    }
    if(hover(right+8,146,groupw-16,22)) snprintf(ui.status,sizeof ui.status,"Fit to tempo: keep audio fixed on the grid when BPM changes; uses Resample/Stretch mode");
    if(hover(right+116,98,groupw-128,22)) snprintf(ui.status,sizeof ui.status,"Time mode: Resample changes speed/pitch; Stretch preserves pitch; click to choose");
    label("Presets",20,190,11,muted);
    if(button("Load",20,210,(groupw-20)/2,24,0)) preset_action(PRESET_SAMPLER,c,0,0);
    if(button("Save",24+(groupw-20)/2,210,(groupw-20)/2,24,0)) preset_action(PRESET_SAMPLER,c,0,1);
    label("Sample region",right+8,182,12,muted);
    knob(right+28,212,&settings->start,0,1,0,"Start");
    knob(right+84,212,&settings->length,0,1,1,"Length");
    label("Start",right+14,230,11,muted); label("Length",right+67,230,11,muted);
    knob(right+156,212,&settings->trim,0,1,0,"Trim quiet beginning and end (threshold)");
    label("Trim",right+143,230,11,muted);
    int pending=!sampler_processing_equal(*settings,ui.sampler_applied[c]);
    int live_preview=sampler_live_preview(c);
    float duration=live_preview?sampler_view_frames(c)/(float)RATE:ui.samples[c].frames/(float)RATE;

    if(settings->fit_bpm) duration*=settings->fit_bpm/ui.project.bpm;

    Sample sample=ui.samples[c];
    if(live_preview) sample.frames=sampler_view_frames(c);
    float area=height-276;
    Rectangle display={12,252,width-24,area};
    DrawRectangleRec(display,bg);
    draw_waveform((WaveDisplay){.sample=sample,.wave=live_preview?NULL:processed_waveform(c),.channel=c,.preview=live_preview,
        .origin=12,.width=width-24},display,12,width-12,ui_theme.waveform);
    double progress=audio_key_position(127,c);
    if(hover(12,252,width-24,area)) {
        snprintf(ui.status,sizeof ui.status,"Sample waveform: click to %s; drop a Browser sample here to load it",progress>=0?"stop playback":"play from the beginning");
        if(ui.samples[c].frames && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.channel=c; audio_key(127,c,60,progress<0); }
    }
    if(progress>=0 && !live_preview) {
        float x=12+progress*(width-24);
        DrawLineEx((Vector2){x,252},(Vector2){x,252+area},1.5f,ink);
    }
    label("0 s",16,254,10,muted);
    label(TextFormat("%.2f s%s",duration,pending?" *":""),width-78,254,10,muted);
    if(live_preview && hover(12,252,width-24,area)) snprintf(ui.status,sizeof ui.status,"Live envelope preview; processed pitch/stretch detail appears when ready; audio changes after release");
    if(ui.sample_drag[0] && ui.sample_moved) DrawRectangleLines(12,252,width-24,area,ui_theme.signal);

}
