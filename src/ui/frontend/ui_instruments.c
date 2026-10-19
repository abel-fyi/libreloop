// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"
#include "control.h"
#include "envelope_view.h"
static void instrument_group(float x,float y,float width,float height,const char *title);
static void instrument_hint(const char *text,float width,int y,int size) {
    if(!ui.studio_style){label(text,20,y,size,muted);return;}
    char line[256]={0};size_t used=0;
    while(*text) {
        while(*text==' ')text++;
        size_t length=strcspn(text," ");if(!length)break;
        char candidate[256];
        int count=snprintf(candidate,sizeof candidate,"%s%s%.*s",line,used?" ":"",(int)length,text);
        if(used && (count>=(int)sizeof candidate || text_width(candidate,size)>width-40)) {
            label(line,20,y,size,muted);y+=size+6;used=0;line[0]=0;
        }
        if(used){line[used++]=' ';line[used]=0;}
        size_t room=sizeof line-used-1;if(length>room)length=room;
        memcpy(line+used,text,length);used+=length;line[used]=0;text+=length;
    }
    if(used)label(line,20,y,size,muted);
}
static void instrument_preset_at(float width,const char *name,float y,float h) {
    int compact=ui.studio_style && y<70;
    float field_width=width-(compact?300:ui.studio_style?24:160);
    if(compact)y=30;
    if(ui.studio_style) {
        ui_surface(host_draw(),(DrawRect){12,y,field_width,h},ui_skin.display);
        ui_bevel(host_draw(),(DrawRect){12,y,field_width,h},1);
    }
    label(fit_text(name,field_width-(ui.studio_style?8:0),11),12+(ui.studio_style?4:0),y+(h-14)/2,11,ui.studio_style?ui_theme.signal:muted);
    if(ui.studio_style && hover(12,y,field_width,h))snprintf(ui.status,sizeof ui.status,"Preset: %s",name);
}
static void instrument_preset_field(float width,const char *name) { instrument_preset_at(width,name,52,22); }

int sampler_toggle(int x,int y,int width,const char *name,int active) {
    int over=hover(x,y,width,22);
    if(over) host_set_mouse_cursor(HOST_MOUSE_CURSOR_POINTING_HAND);
    if(ui.studio_style) {
        ui_check_face(host_draw(),(DrawRect){x,y+2,18,18},active,accent);
        label(name,x+24,y+(22-UI_TEXT_SIZE)/2,UI_TEXT_SIZE,ink);
    } else {
        DrawColor foreground=over?theme_foreground(ui_theme.hover):ink;
        if(over) ui_surface(host_draw(),(DrawRect){x,y,width,22},ui_theme.hover);
        circle(x+9,y+11,7,over?foreground:ui_theme.border); circle(x+9,y+11,5,cell);
        if(active) circle(x+9,y+11,3,accent);
        label(name,x+24,y+4,13,foreground);
    }
    return over && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT);
}

void sampler_switch(int x,int y,int width,const char *name,uint8_t *flags,int bit) {
    if(sampler_toggle(x,y,width,name,*flags&bit)) *flags^=bit;
}

void fm_graph_update_at(float x,float y) {
    if(ui.fm_graph_channel<0 || ui.fm_graph_channel>=ui.project.channel_count) return;
    FMSettings *s=&ui.project.fm[ui.fm_graph_channel];
    if(ui.fm_graph_drag>=4) {
        int stage=ui.fm_graph_drag-4;
        if(ui.studio_style && ui.fm_graph_seconds>0){
            int base=ui.fm_dx_drag_base;
            s->dx7.value[base+4+stage]=roundf(fmaxf(0,fminf(99,(1-(y-ui.fm_graph_area.y-12)/fmaxf(1,ui.fm_graph_area.height-34))*99)));
            double wanted=fmax(0,(x-ui.fm_graph_area.x-8)/(ui.fm_graph_area.width-16)*ui.fm_graph_seconds-ui.fm_graph_stage_start),times[4];
            int best=(int)s->dx7.value[base+stage];dx7_envelope_times(&s->dx7,base,60,100.f/127,times);double error=fabs(times[stage]-wanted);
            DX7Settings trial=s->dx7;
            for(int rate=0;rate<=99;rate++){trial.value[base+stage]=rate;dx7_envelope_times(&trial,base,60,100.f/127,times);double e=fabs(times[stage]-wanted);if(e<error){best=rate;error=e;}}
            s->dx7.value[base+stage]=best;return;
        }
        float slot=(ui.fm_graph_area.width-16)/4;
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
    int tab=ui.studio_style && ui.fm_graph_tab>=0?ui.fm_graph_tab:ui.fm_tab;
    int field=ui.fm_graph_drag==0?0:ui.fm_graph_drag==1?1:3;
    const ParameterDescriptor *info=parameter_descriptor(ui.fm_envelope_ids[tab][field]);
    if(ui.studio_style && ui.fm_graph_seconds>0){
        double seconds=(x-ui.fm_graph_area.x-8)/(ui.fm_graph_area.width-16)*ui.fm_graph_seconds-ui.fm_graph_stage_start;
        *(float *)fm_parameter_pointer(s,info->id)=fmaxf(info->low,fminf(info->high,seconds));
        if(ui.fm_graph_drag==1)*(float *)fm_parameter_pointer(s,ui.fm_envelope_ids[tab][2])=fmaxf(0,fminf(1,(ui.fm_graph_area.y+ui.fm_graph_area.height-22-y)/(ui.fm_graph_area.height-34)));
        return;
    }
    float slot=(ui.fm_graph_area.width-24)/3;
    float position=fmaxf(0,fminf(1,(x-ui.fm_graph_area.x-8-slot*ui.fm_graph_drag)/(.9f*slot)));
    float value=expm1f(position*log1pf(info->high*100))/100;
    *(float *)fm_parameter_pointer(s,info->id)=fmaxf(info->low,fminf(info->high,value));
    if(ui.fm_graph_drag==1) {
        float sustain=fmaxf(0,fminf(1,(ui.fm_graph_area.y+ui.fm_graph_area.height-22-y)/(ui.fm_graph_area.height-34)));
        *(float *)fm_parameter_pointer(s,ui.fm_envelope_ids[tab][2])=sustain;
    }
}

void fm_graph_update(void) {
    Rect r=desktop_editor_rect(4); fm_graph_update_at(ui.mouse.x-r.x,ui.mouse.y-r.y);
}

static const char *fm_value_caption(FMSettings *s,unsigned id,float display) {
    const char *shown;
    if(id>=PARAM_VA_FIRST && id<=PARAM_VA_LAST) {
        unsigned n=id-PARAM_VA_FIRST;
        if(n<12 && n%4==0){static const char *waves[]={"Saw","Pulse","Triangle","Sine"};int wave=(int)roundf(display);shown=fabsf(display-wave)<.03f?waves[wave]:host_text_format("%.2f",display);}
        else if(n<12 && n%4==1)shown=host_text_format("%+.0f st",display);
        else if(n<12 && n%4==2)shown=host_text_format("%+.0f ct",display);
        else if(n==12)shown=host_text_format("%.1f dB",display);
        else shown=host_text_format("%.0f%%",display*100);
    }
    else if(id>=PARAM_DX7_FIRST && id<=PARAM_DX7_LAST) {
        int index=id-PARAM_DX7_FIRST,field=index%21;
        if(index>=DX7_NATIVE_PARAMETERS) shown=host_text_format("%.1f%%",display);
        else if(index<126 && field==18) {
            int base=index-field;
            double fine=s->dx7.value[base+19]/100;
            if(s->dx7.value[base+17]) {
                double frequency=pow(10,((int)display&3)+fine);
                shown=frequency>=1000?host_text_format("%.2f kHz",frequency/1000):
                    host_text_format(frequency<10?"%.2f Hz":"%.0f Hz",frequency);
            } else shown=host_text_format("%.2fx",fmaxf(.5f,display)*(1+fine));
        }
        else if(index<126 && field==8) { static const char *notes[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}; int key=(int)display+17; shown=host_text_format("%s%d",notes[key%12],key/12-1); }
        else if(index<126 && (field==11 || field==12)) { static const char *curves[]={"-Lin","-Exp","+Exp","+Lin"}; shown=curves[(int)display]; }
        else if(index==136 || index==141) shown=display>=.5f?"On":"Off";
        else if(index==142) { static const char *waves[]={"Triangle","Saw down","Saw up","Square","Sine","S&H"}; shown=waves[(int)display]; }
        else if(index<126 && field==20) shown=host_text_format("%+.0f",display-7);
        else if(index==134) shown=host_text_format("%.0f",display+1);
        else if(index==144) shown=host_text_format("%+.0f st",display-24);
        else shown=host_text_format("%.0f",display);
    }
    else if(s->engine==2 && s->analog.model && (id==PARAM_FM_ANALOG_NOISE || id==PARAM_FM_ANALOG_PULSE || id==PARAM_FM_ANALOG_PWM || id==PARAM_FM_FILTER_RESONANCE || id==PARAM_FM_SUSTAIN || id==PARAM_FM_MOD_SUSTAIN))shown=host_text_format("%.0f%%",display*100);
    else if(s->engine==2 && s->analog.model && id==PARAM_FM_FILTER_ENV)shown=host_text_format("%+.1f oct",display);
    else if(id==PARAM_FM_FILTER_CUTOFF) shown=host_text_format(display>=1000?"%.2g kHz":"%.0f Hz",display>=1000?display/1000:display);
    else if(id==PARAM_FM_CARRIER_RATIO || id==PARAM_FM_ATTACK_RATIO) shown=host_text_format("%.2gx",display);
    else if(id==PARAM_FM_VIBRATO) shown=host_text_format("%.0f ct",display);
    else if(id==PARAM_FM_BODY_PITCH) shown=host_text_format("%+.1f st",display);
    else if(id==PARAM_FM_CARRIER_DETUNE || id==PARAM_FM_BODY_DETUNE || id==PARAM_FM_ATTACK_DETUNE) shown=host_text_format("%+.0f ct",display);
    else if(id==PARAM_FM_ATTACK || id==PARAM_FM_DECAY || id==PARAM_FM_RELEASE || id==PARAM_FM_MOD_ATTACK || id==PARAM_FM_MOD_DECAY || id==PARAM_FM_MOD_RELEASE ||
            id==PARAM_FM_ATTACK_ATTACK || id==PARAM_FM_ATTACK_DECAY || id==PARAM_FM_ATTACK_RELEASE || id==PARAM_FM_LFO_FADE)
        shown=display<1?host_text_format("%.0f ms",display*1000):host_text_format("%.2g s",display);
    else shown=host_text_format("%.2g",display);
    return shown;
}

static void fm_control_caption(const char *text,float x,float y,int size,float width,DrawColor color) {
    if(width>0) {
        if(!ui.studio_style)while(size>10 && text_width(text,size)>width)size--;
        text=fit_text(text,width,size);
    }
    label(text,x-text_width(text,size)/2,y,size,color);
}
static void fm_control_spaced(FMSettings *s,unsigned id,const char *caption,int x,int y,int spacious,float caption_width) {
    const ParameterDescriptor *info=parameter_descriptor(id);
    float *value=(float *)fm_parameter_pointer(s,id);
    int logarithmic=info->kind==PARAMETER_LOGARITHMIC ||
        (info->low>0 && (!strcmp(caption,"Attack") || !strcmp(caption,"Decay") || !strcmp(caption,"Release")));
    int fixed_coarse=id>=PARAM_DX7_FIRST && id<PARAM_DX7_FIRST+126 &&
        (id-PARAM_DX7_FIRST)%21==18 && s->dx7.value[id-PARAM_DX7_FIRST-1];
    int style=fixed_coarse?KNOB_FIXED_COARSE:logarithmic?KNOB_LOGARITHMIC:info->low<0?KNOB_CENTER:KNOB_NORMAL;
    if(spacious)knob_sized(x,y,value,info->low,info->high,info->initial,info->name,style,PANEL_KNOB_RADIUS);
    else knob_style(x,y,value,info->low,info->high,info->initial,info->name,style);
    if(ui.control_drag==value && logarithmic) ui.control_logarithmic=1;
    int caption_font=spacious?14:ui.studio_style?10:11,value_font=spacious?13:ui.studio_style?9:10;
    fm_control_caption(caption,x,y+(spacious?24:ui.studio_style?12:17),caption_font,caption_width,muted);
    float display=displayed_value(value,*value);
    const char *shown=fm_value_caption(s,id,display);
    fm_control_caption(shown,x,y+(spacious?39:ui.studio_style?25:31),value_font,caption_width,ink);
    if(id>=PARAM_DX7_FIRST+DX7_NATIVE_PARAMETERS && id<=PARAM_DX7_LAST &&
       (hover(x-16,y-16,32,32) || ui.control_drag==value))
        snprintf(ui.status,sizeof ui.status,"Key tracking: 0%% fixed, 50%% half an octave per octave, 100%% follows notes. Anchor: middle C (C4).");
}
void fm_control(FMSettings *s,unsigned id,const char *caption,int x,int y) { fm_control_spaced(s,id,caption,x,y,0,0); }

/* Envelope materials are shared; stage geometry and gestures belong to each synth. */
static void envelope_grid(DrawRect plot,float top,float base) {
    host_draw_rectangle_rec(plot,ui.studio_style?ui_skin.graph:bg);
    if(ui.studio_style)for(int i=0;i<=4;i++) {
        float y=top+(base-top)*i/4;
        host_draw_line_ex((DrawPoint){plot.x+8,y},(DrawPoint){plot.x+plot.width-8,y},1,(DrawColor){58,91,121,255});
    }
}
static void envelope_guide(float x,float top,float base) {
    host_draw_line_ex((DrawPoint){x,top},(DrawPoint){x,base},1,(DrawColor){255,255,255,255});
}
static void envelope_frame(DrawRect plot) {if(ui.studio_style && !ui.profile_flat)ui_bevel(host_draw(),plot,1);}
static void envelope_axis(DrawRect plot,float base,double seconds) {
    label("0 s",plot.x+8,base+4,10,ui_theme.signal);
    const char *axis=host_text_format("%.2f s",seconds);
    label(axis,plot.x+plot.width-8-text_width(axis,10),base+4,10,ui_theme.signal);
}

/* Draw beneath the existing envelope segments, without changing their geometry. */
static void envelope_segment(DrawPoint a,DrawPoint b,float base) {
    if(ui.studio_style) {
        ui_graph_area(host_draw(),a,b,base);
    }
    smooth_line(a,b,ui.studio_style?2:1.5f,ui.studio_style?ui_skin.graph_line:accent);
}

static void lfo_filled_segment(DrawPoint a,DrawPoint b,float mid,DrawColor color) {
    ui_graph_area(host_draw(),a,b,mid);
    smooth_line(a,b,1.5f,color);
}
/* Read-only illustration of the existing per-note LFO, never a new control. */
static void lfo_display(const FMSettings *s,DrawRect plot,float cycles) {
    if(plot.width<20 || plot.height<12)return;
    cycles=fmaxf(2,ceilf(cycles));
    ui_display_grid(host_draw(),plot,4,4);
    float mid=plot.y+plot.height/2,range=(plot.height-8)*.42f;
    DrawPoint previous={plot.x+4,mid};
    int points=fminf(192,plot.width-8);
    for(int i=0;i<=points;i++) {
        float phase=i/(float)points*cycles;
        DrawPoint point={plot.x+4+(plot.width-8)*i/points,mid-fm_lfo_value(phase,(int)s->lfo_shape)*range};
        if(i)lfo_filled_segment(previous,point,mid,ui_theme.signal);
        previous=point;
    }
    host_draw_line_ex((DrawPoint){plot.x+3,mid},(DrawPoint){plot.x+plot.width-4,mid},1,(DrawColor){112,149,128,255});
}

static void analog_envelope_segment(DrawPoint a,DrawPoint b,float base,int attack,int filled) {
    DrawPoint previous=a;
    for(int i=1;i<=40;i++) {
        float t=i/40.f,progress=attack?1.01f*(1-expf(-4.6151205f*t)):1-expf(-9.2103404f*t);
        if(i==40)progress=1;
        DrawPoint next={a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*progress};
        if(filled)envelope_segment(previous,next,base);else smooth_line(previous,next,2,ui_skin.graph_line);
        previous=next;
    }
}

void fm_envelope_graph(FMSettings *s,DrawRect plot) {
    int modern=ui.studio_style && s->engine==2 && s->analog.model;
    float slot=(plot.width-24)/3,base=plot.y+plot.height-22,range=plot.height-34;
    float times[3]={*fm_parameter_pointer(s,ui.fm_envelope_ids[ui.fm_tab][0]),*fm_parameter_pointer(s,ui.fm_envelope_ids[ui.fm_tab][1]),*fm_parameter_pointer(s,ui.fm_envelope_ids[ui.fm_tab][3])};
    int fields[]={0,1,3}; DrawPoint handles[3];
    float sustain=*fm_parameter_pointer(s,ui.fm_envelope_ids[ui.fm_tab][2]);
    for(int i=0;i<3;i++) {
        const ParameterDescriptor *info=parameter_descriptor(ui.fm_envelope_ids[ui.fm_tab][fields[i]]);
        float t=log1pf(times[i]*100)/log1pf(info->high*100);
        handles[i]=(DrawPoint){plot.x+8+slot*i+.9f*slot*t,i==0?base-range:i==1?base-sustain*range:base};

    }
    int dragging=ui.fm_graph_drag>=0 && ui.fm_graph_drag<3 && ui.fm_graph_channel==ui.instrument_channel && ui.fm_graph_tab==ui.fm_tab;
    EnvelopeTimeView timing=adsr_time_view(times[0],times[1],times[2],dragging?ui.fm_graph_seconds:0);
    if(ui.studio_style)for(int i=0;i<3;i++)handles[i].x=plot.x+8+(plot.width-16)*timing.end[i==2?3:i]/timing.seconds;
    envelope_grid(plot,base-range,base);
    if(!ui.studio_style)for(int i=0;i<3;i++)host_draw_line_ex((DrawPoint){plot.x+8+slot*i,plot.y+8},(DrawPoint){plot.x+8+slot*i,base},1,ui_theme.grid_minor);
    if(modern)analog_envelope_segment((DrawPoint){plot.x+8,base},handles[0],base,1,1);
    else envelope_segment((DrawPoint){plot.x+8,base},handles[0],base);
    DrawPoint previous=handles[0];
    for(int i=1;i<=40;i++) {
        float t=i/40.f,level=(ui.fm_tab || modern)?sustain+(1-sustain)*expf(-4.6051702f*t):1-(1-sustain)*t;
        DrawPoint point={handles[0].x+(handles[1].x-handles[0].x)*t,base-level*range};
        envelope_segment(previous,point,base); previous=point;
    }
    envelope_segment(previous,handles[1],base);
    DrawPoint release={ui.studio_style?plot.x+8+(plot.width-16)*timing.end[2]/timing.seconds:plot.x+8+slot*2,handles[1].y};
    envelope_segment(handles[1],release,base);
    if(modern)analog_envelope_segment(release,handles[2],base,0,1);else envelope_segment(release,handles[2],base);
    if(ui.studio_style) {
        /* Stage boundaries follow actual handles; neither the plot nor its hits move. */
        float stages[]={handles[0].x,handles[1].x,release.x};
        for(int i=0;i<3;i++)envelope_guide(stages[i],plot.y+6,base);
        /* Repaint the contour over the grid without changing its coordinates. */
        DrawColor edge=ui_skin.graph_line;
        if(modern)analog_envelope_segment((DrawPoint){plot.x+8,base},handles[0],base,1,0);
        else smooth_line((DrawPoint){plot.x+8,base},handles[0],2,edge);
        DrawPoint prior=handles[0];
        for(int i=1;i<=40;i++) {
            float t=i/40.f,level=(ui.fm_tab || modern)?sustain+(1-sustain)*expf(-4.6051702f*t):1-(1-sustain)*t;
            DrawPoint point={handles[0].x+(handles[1].x-handles[0].x)*t,base-level*range};
            smooth_line(prior,point,2,edge);prior=point;
        }
        smooth_line(prior,handles[1],2,edge);smooth_line(handles[1],release,2,edge);
        if(modern)analog_envelope_segment(release,handles[2],base,0,0);else smooth_line(release,handles[2],2,edge);
        ui_graph_handle(host_draw(),(DrawPoint){plot.x+8,base});
        ui_graph_handle(host_draw(),release);
        envelope_frame(plot);
    }
    const char *captions[]={"Attack","Decay / sustain","Release"};
    for(int i=0;i<3;i++) {
        if(ui.studio_style) {
            ui_graph_handle(host_draw(),handles[i]);
        } else {circle(handles[i].x,handles[i].y,4,ink);circle(handles[i].x,handles[i].y,2,accent);}
        if(!ui.studio_style)label(captions[i],plot.x+8+slot*i,base+7,10,muted);
        if(hover(handles[i].x-7,handles[i].y-7,14,14)) {
            host_set_mouse_cursor(HOST_MOUSE_CURSOR_POINTING_HAND);
            if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) { ui.fm_graph_drag=i; ui.fm_graph_tab=ui.fm_tab; ui.fm_graph_channel=ui.instrument_channel; ui.fm_graph_area=plot;ui.fm_graph_seconds=timing.seconds;ui.fm_graph_stage_start=i==0?0:i==1?timing.end[0]:timing.end[2]; ui.input_enabled=0; }
        }
    }
    if(ui.studio_style)envelope_axis(plot,base,timing.seconds);
    if(hover(plot.x,plot.y,plot.width,plot.height)) snprintf(ui.status,sizeof ui.status,ui.studio_style?"Linear time axis; sustain hold is illustrative. Drag envelope points: left/right changes time; the middle point also changes sustain. Knobs offer exact entry and automation.":"Drag envelope points: left/right changes time; the middle point also changes sustain. Knobs offer exact entry and automation.");
}

void fm_legacy_editor(float width,float height) {
    int c=ui.instrument_channel; FMSettings *s=&ui.project.fm[c];
    if(!ui.studio_style)label(fit_text(ui.project.channel_names[c],width-298,14),12,30,14,ink);
    channel_controls(c,width);
    instrument_preset_field(width,ui.studio_style?fm_sound_name(s):"Custom three-operator FM");
    const char *tabs[]={"Sound","Body","Attack","Motion"}; float tabw=(width-24)/4;
    for(int i=0;i<4;i++) if(button(tabs[i],12+i*tabw,78,tabw-3,26,ui.fm_tab==i)) ui.fm_tab=i;
    if(ui.studio_style){
        instrument_group(12,108,width-24,68," ");
        instrument_group(12,180,width-24,height-222," ");
    }
    float column=(width-32)/4;
    const unsigned head[4][4]={
        {PARAM_FM_CARRIER_RATIO,PARAM_FM_CARRIER_DETUNE,PARAM_FM_VELOCITY,0},
        {PARAM_FM_BODY_PITCH,PARAM_FM_BODY_DETUNE,PARAM_FM_RATIO,PARAM_FM_DEPTH},
        {PARAM_FM_ATTACK_RATIO,PARAM_FM_ATTACK_DETUNE,PARAM_FM_ATTACK_DEPTH,PARAM_FM_VELOCITY},
        {PARAM_FM_LFO_RATE,PARAM_FM_VIBRATO,PARAM_FM_TREMOLO,PARAM_FM_LFO_FADE}
    };
    const char *captions[4][4]={{"Pitch","Fine","Velocity",""},{"Pitch","Fine","Harmonic","FM amount"},{"Pitch","Fine","FM amount","Velocity"},{"Speed",ui.studio_style?"Pitch amt":"Pitch motion",ui.studio_style?"Vol. amt":"Volume motion","Fade in"}};
    const char *descriptions[]={"Audible oscillator and volume envelope","Long-lived tone modulation","Short, bright attack modulation","A separate, per-note LFO for pitch and volume"};
    label(ui.studio_style?fit_text(descriptions[ui.fm_tab],width-40,12):descriptions[ui.fm_tab],20,113,12,muted);
    if(ui.studio_style && hover(20,110,width-40,18))snprintf(ui.status,sizeof ui.status,"%s",descriptions[ui.fm_tab]);
    for(int i=0;i<4;i++) if(head[ui.fm_tab][i]) fm_control(s,head[ui.fm_tab][i],captions[ui.fm_tab][i],20+column*(i+.5f),139);
    if(ui.fm_tab==0) {
        int x=20+column*3;
        if(choice_button(s->routing?"Stacked":ui.studio_style?"B + A":"Body + Attack",x,131,column-8,24)) open_context(20,c,(DrawPoint){ui.mouse.x+desktop_editor_rect(4).x,ui.mouse.y+desktop_editor_rect(4).y});
        if(hover(x,131,column-8,24))snprintf(ui.status,sizeof ui.status,"FM routing: Body + Attack = parallel modulation; Stacked = attack modulates body");
        if(hover(x,131,column-8,24) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) control_menu(&s->routing,0,1,0,"FM routing");
        label("Routing",x+8,159,11,muted);
    }
    if(ui.fm_tab<3) {
        const char *names[]={"Attack","Decay","Sustain","Release"};
        label("Envelope",20,184,12,muted);
        for(int i=0;i<4;i++) fm_control(s,ui.fm_envelope_ids[ui.fm_tab][i],names[i],20+column*(i+.5f),207);
        fm_envelope_graph(s,(DrawRect){12,252,width-24,height-294});
    } else {
        const char *shapes[]={"Sine","Triangle","Saw","Square"};
        if(choice_button(shapes[(int)s->lfo_shape],12,184,112,24)) open_context(19,c,(DrawPoint){ui.mouse.x+desktop_editor_rect(4).x,ui.mouse.y+desktop_editor_rect(4).y});
        if(hover(12,184,112,24) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) control_menu(&s->lfo_shape,0,3,0,"LFO shape");
        if(button("Pitch",132,184,72,24,!ui.fm_motion_target)) ui.fm_motion_target=0;
        if(button("Volume",208,184,80,24,ui.fm_motion_target)) ui.fm_motion_target=1;
        DrawRect plot={12,216,width-24,height-258}; host_draw_rectangle_rec(plot,bg);
        if(ui.studio_style)ui_display_grid(host_draw(),plot,4,6);
        float base=plot.y+plot.height-22,range=plot.height-34,mid=plot.y+(plot.height-22)/2;
        float amount=ui.fm_motion_target?s->tremolo:s->vibrato/100;
        DrawPoint previous={plot.x+8,mid};
        for(int i=0;i<=(int)plot.width-16;i++) {
            float t=i/(plot.width-16)*fmaxf(2,ceilf(s->lfo_rate*.5f));
            DrawPoint point={plot.x+8+i,mid-fm_lfo_value(t,(int)s->lfo_shape)*range*.42f};
            if(i) {
                DrawColor color=ui.studio_style?ui_theme.signal:host_fade(accent,.35f+amount*.65f);
                if(ui.studio_style)lfo_filled_segment(previous,point,mid,color);
                else smooth_line(previous,point,1.5f,color);
            }
            previous=point;
        }
        DrawPoint handle={plot.x+8+logf(s->lfo_rate/.1f)/logf(120)*(plot.width-16),base-amount*range};
        circle(handle.x,handle.y,5,ui.studio_style?ui_theme.signal:ink); circle(handle.x,handle.y,3,accent);
        label("Drag: speed / amount",plot.x+8,base+7,10,ui.studio_style?(DrawColor){160,190,170,255}:muted);
        if(hover(plot.x,plot.y,plot.width,plot.height)) {
            host_set_mouse_cursor(HOST_MOUSE_CURSOR_POINTING_HAND);
            snprintf(ui.status,sizeof ui.status,"Drag the LFO graph horizontally for speed, vertically for %s amount; choose a waveform above",ui.fm_motion_target?"volume":"pitch");
            if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) { ui.fm_graph_drag=3; ui.fm_graph_channel=c; ui.fm_graph_area=plot; fm_graph_update_at(ui.mouse.x,ui.mouse.y); ui.input_enabled=0; }
        }
    }
    if(ui.studio_style)fm_footer(c,width,height);
    else {
        if(button("Factory",12,height-34,98,24,0)) open_context(21,c,(DrawPoint){ui.mouse.x+desktop_editor_rect(4).x,ui.mouse.y+desktop_editor_rect(4).y});
        if(button("Load",114,height-34,ui.studio_style && width<440?58:64,24,0)) preset_action(PRESET_FM,c,0,0);
        if(button("Save",ui.studio_style && width<440?176:182,height-34,ui.studio_style && width<440?42:64,24,0)) preset_action(PRESET_FM,c,0,1);
        double active=audio_key_position(127,c);
        if(!ui.studio_style && button(active>=0?"Release C4":"Play C4",width-112,height-34,100,24,active>=0)) { ui.channel=c; audio_key(127,c,60,active<0); }
    }
}

const char *fm_sound_name(const FMSettings *s) {
    if(s->engine==2 && s->analog.model)for(int i=0;i<3;i++){FMSettings factory=fm_analog_factory(i);if(!memcmp(s,&factory,sizeof factory))return fm_analog_factory_name(i);}
    for(int i=0;i<FM_FACTORY_COUNT;i++) { FMSettings factory=fm_factory(i); if(!memcmp(s,&factory,sizeof factory)) return fm_factory_name(i); }
    return "Edited sound";
}

void fm_footer(int c,float width,float height) {
    if(ui.studio_style) {
        int tall=ui.project.fm[c].engine==1 && sixop_layout(width,height,1).spacious;
        float y=height-(tall?44:34),h=tall?34:24,w=(width-32)/3;
        if(button("Factory",12,y,w,h,0))open_context(21,c,(DrawPoint){ui.mouse.x+desktop_editor_rect(4).x,ui.mouse.y+desktop_editor_rect(4).y});
        if(button("Load",16+w,y,w,h,0))preset_action(PRESET_FM,c,0,0);
        if(button("Save",20+2*w,y,w,h,0))preset_action(PRESET_FM,c,0,1);
        return;
    }
    if(button("Factory",12,height-34,98,24,0)) open_context(21,c,(DrawPoint){ui.mouse.x+desktop_editor_rect(4).x,ui.mouse.y+desktop_editor_rect(4).y});
    if(button("Load",114,height-34,ui.studio_style && width<440?58:64,24,0)) preset_action(PRESET_FM,c,0,0);
    if(button("Save",ui.studio_style && width<440?176:182,height-34,ui.studio_style && width<440?42:64,24,0)) preset_action(PRESET_FM,c,0,1);
    int active=audio_key_position(127,c)>=0;
    if(button(active?"Release C4":"Play C4",width-112,height-34,100,24,active)) { ui.channel=c; audio_key(127,c,60,!active); }
}

void fm_controls_row(FMSettings *s,const unsigned *ids,const char *const *names,int count,float width,int y) {
    float column=(width-32)/count;
    for(int i=0;i<count;i++) fm_control(s,ids[i],names[i],20+column*(i+.5f),y);
}
static void fm_spaced_row(FMSettings *s,const unsigned *ids,const char *const *names,int count,float width,float y,int spacious) {
    float column=(width-32)/count;
    for(int i=0;i<count;i++)fm_control_spaced(s,ids[i],names[i],20+column*(i+.5f),y,spacious,(spacious || ui.studio_style)?column-8:0);
}

void dx7_envelope_graph(FMSettings *s,int base,DrawRect plot) {
    if(ui.studio_style){
        double times[4];dx7_envelope_times(&s->dx7,base,60,100.f/127,times);
        int dragging=ui.fm_graph_drag>=4 && ui.fm_graph_channel==ui.instrument_channel && ui.fm_dx_drag_base==base;
        EnvelopeTimeView v=dx7_time_view(times,dragging?ui.fm_graph_seconds:0);
        float height=fmaxf(1,plot.height-34),bottom=plot.y+12+height;
        DrawPoint points[6]={{plot.x+8,base==126?plot.y+12+height*(1-s->dx7.value[base+7]/99):bottom}};
        for(int i=0;i<3;i++)points[i+1]=(DrawPoint){plot.x+8+(plot.width-16)*v.end[i]/v.seconds,plot.y+12+height*(1-s->dx7.value[base+4+i]/99)};
        points[4]=(DrawPoint){plot.x+8+(plot.width-16)*(v.end[2]+v.hold)/v.seconds,points[3].y};
        points[5]=(DrawPoint){plot.x+8+(plot.width-16)*v.end[3]/v.seconds,plot.y+12+height*(1-s->dx7.value[base+7]/99)};
        envelope_grid(plot,plot.y+12,bottom);
        for(int i=0;i<5;i++)ui_graph_area(host_draw(),points[i],points[i+1],bottom);
        for(int i=0;i<4;i++)envelope_guide(points[i==3?5:i+1].x,plot.y+12,bottom);
        for(int i=0;i<5;i++)smooth_line(points[i],points[i+1],2,ui_skin.graph_line);
        envelope_frame(plot);
        envelope_axis(plot,bottom,v.seconds);
        for(int i=0;i<4;i++){
            DrawPoint point=points[i==3?5:i+1];ui_graph_handle(host_draw(),point);
            if(hover(point.x-7,point.y-7,14,14)){
                host_set_mouse_cursor(HOST_MOUSE_CURSOR_POINTING_HAND);snprintf(ui.status,sizeof ui.status,"Stage %d: linear time at C4, velocity 100; sustain hold is illustrative. Drag rate / level.",i+1);
                if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)){ui.fm_graph_drag=4+i;ui.fm_graph_channel=ui.instrument_channel;ui.fm_graph_area=plot;ui.fm_dx_drag_base=base;ui.fm_graph_seconds=v.seconds;ui.fm_graph_stage_start=i==0?0:i==3?v.end[2]+v.hold:v.end[i-1];ui.input_enabled=0;}
            }
        }
        return;
    }
    float width=(plot.width-16)/4,height=fmaxf(1,plot.height-12);
    envelope_grid(plot,plot.y+6,plot.y+6+height);
    DrawPoint points[5]={{plot.x+8,plot.y+height}};
    for(int i=0;i<4;i++) {
        float rate=s->dx7.value[base+i],level=s->dx7.value[base+4+i];
        points[i+1]=(DrawPoint){plot.x+8+width*(i+.1f+.8f*(1-rate/99)),plot.y+6+height*(1-level/99)};
    }
    if(ui.studio_style) {
        float baseline=plot.y+6+height;
        for(int i=0;i<4;i++) {
            DrawPoint a=points[i],b=points[i+1];
            ui_graph_area(host_draw(),a,b,baseline);
        }
        for(int i=1;i<=4;i++)envelope_guide(points[i].x,plot.y+6,baseline);
        envelope_frame(plot);
    }
    for(int i=0;i<4;i++) {
        DrawPoint point=points[i+1];
        smooth_line(points[i],point,ui.studio_style?2:1.5f,ui.studio_style?ui_skin.graph_line:accent);
        if(ui.studio_style) {
            ui_graph_handle(host_draw(),point);
        } else circle(point.x,point.y,3,ink);
        if(hover(point.x-7,point.y-7,14,14)) {
            host_set_mouse_cursor(HOST_MOUSE_CURSOR_POINTING_HAND); snprintf(ui.status,sizeof ui.status,"Drag stage %d: left/right changes rate, up/down changes level",i+1);
            if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) { ui.fm_graph_drag=4+i; ui.fm_graph_channel=ui.instrument_channel; ui.fm_graph_area=plot; ui.fm_dx_drag_base=base; ui.input_enabled=0; }
        }
    }
}

void fm_dx_editor(float width,float height) {
    int c=ui.instrument_channel; FMSettings *s=&ui.project.fm[c];
    SixOpLayout layout=sixop_layout(width,height,ui.studio_style);
    if(!ui.studio_style)label(fit_text(ui.project.channel_names[c],width-298,14),12,30,14,ink);
    channel_controls(c,width);
    instrument_preset_at(width,fm_sound_name(s),layout.preset,layout.spacious?28:22);
    float tab=(width-24)/7;
    for(int i=0;i<7;i++) if(button(i==6?"Global":host_text_format("Op %d",i+1),12+i*tab,layout.tabs,tab-3,layout.spacious?30:26,ui.fm_dx_operator==i)) ui.fm_dx_operator=i;
    if(ui.fm_dx_operator<6) {
        int base=(5-ui.fm_dx_operator)*21;
        label(host_text_format("Operator %d",ui.fm_dx_operator+1),20,layout.heading,12,muted);
        unsigned head[]={PARAM_DX7_FIRST+base+16,PARAM_DX7_FIRST+base+18,PARAM_DX7_FIRST+base+19,PARAM_DX7_FIRST+base+20,PARAM_DX7_FIRST+base+15,PARAM_DX7_FIRST+DX7_NATIVE_PARAMETERS+base/21};
        const char *head_names[]={"Output","Coarse","Fine","Detune",ui.studio_style?"Vel.":"Velocity",ui.studio_style?"Track":"Tracking"}; fm_spaced_row(s,head,head_names,6,width,layout.head,layout.spacious);
        float page_width=ui.studio_style?(width-32)/3:100;
        float keyboard_x=ui.studio_style?16+page_width:116,mode_x=ui.studio_style?20+page_width*2:width-112;
        if(button("Envelope",12,layout.pages,page_width,24,!ui.fm_dx_page)) ui.fm_dx_page=0;
        if(button("Keyboard",keyboard_x,layout.pages,page_width,24,ui.fm_dx_page)) ui.fm_dx_page=1;
        float *mode=&s->dx7.value[base+17];
        if(button(*mode?"Hz at C4":"Ratio",mode_x,layout.pages,page_width,24,0)) {
            *mode=1-*mode; s->dx7.value[DX7_NATIVE_PARAMETERS+base/21]=*mode?0:100;
            if(*mode) s->dx7.value[base+18]=(int)s->dx7.value[base+18]&3;
        }
        if(hover(mode_x,layout.pages,page_width,24) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) control_menu(mode,0,1,0,"Operator frequency mode");
        if(!ui.fm_dx_page) {
            unsigned rates[4],levels[4]; for(int i=0;i<4;i++) { rates[i]=PARAM_DX7_FIRST+base+i; levels[i]=PARAM_DX7_FIRST+base+4+i; }
            const char *rnames[]={"Rate 1","Rate 2","Rate 3",ui.studio_style?"Rel. rate":"Release rate"},*lnames[]={"Level 1","Level 2",ui.studio_style?"Sustain":"Sustain level",ui.studio_style?"Rel. level":"Release level"};
            fm_spaced_row(s,rates,rnames,4,width,layout.row[0],layout.spacious); fm_spaced_row(s,levels,lnames,4,width,layout.row[1],layout.spacious);
            dx7_envelope_graph(s,base,(DrawRect){12,layout.graph,width-24,layout.graph_height});
        } else {
            const int fields[]={8,9,10,14,11,12,13,15}; const char *names[]={"Breakpoint","Left depth","Right depth","Amp motion","Left curve","Right curve","Key rate","Velocity"};
            for(int row=0;row<2;row++) { unsigned ids[4]; for(int i=0;i<4;i++) ids[i]=PARAM_DX7_FIRST+base+fields[row*4+i]; fm_spaced_row(s,ids,names+row*4,4,width,layout.row[row],layout.spacious); }
            instrument_hint("Curves: 0 -linear, 1 -exponential, 2 +exponential, 3 +linear",width,layout.hint,11);
        }
    } else {
        unsigned head[]={PARAM_DX7_FIRST+134,PARAM_DX7_FIRST+135,PARAM_DX7_FIRST+144,PARAM_DX7_FIRST+136};
        const char *names[]={"Algorithm","Feedback","Transpose","Osc sync"}; label("Routing and tuning",20,layout.heading,12,muted); fm_spaced_row(s,head,names,4,width,layout.head,layout.spacious);
        if(button("Motion",12,layout.pages,100,24,!ui.fm_dx_page)) ui.fm_dx_page=0;
        if(button("Pitch envelope",116,layout.pages,140,24,ui.fm_dx_page)) ui.fm_dx_page=1;
        if(!ui.fm_dx_page) {
            unsigned a[]={PARAM_DX7_FIRST+137,PARAM_DX7_FIRST+138,PARAM_DX7_FIRST+139,PARAM_DX7_FIRST+140},b[]={PARAM_DX7_FIRST+142,PARAM_DX7_FIRST+141,PARAM_DX7_FIRST+143};
            const char *an[]={"Speed","Delay",ui.studio_style?"Pitch amt":"Pitch amount",ui.studio_style?"Vol. amt":"Volume amount"},*bn[]={"Waveform","Key sync","Pitch sense"};
            fm_spaced_row(s,a,an,4,width,layout.row[0],layout.spacious); fm_spaced_row(s,b,bn,3,width,layout.row[1],layout.spacious);
            instrument_hint("Waves: triangle, saw down, saw up, square, sine, sample & hold",width,layout.hint,11);
        } else {
            unsigned rates[4],levels[4]; for(int i=0;i<4;i++) { rates[i]=PARAM_DX7_FIRST+126+i; levels[i]=PARAM_DX7_FIRST+130+i; }
            const char *rn[]={"Rate 1","Rate 2","Rate 3",ui.studio_style?"Rel. rate":"Release rate"},*ln[]={"Level 1","Level 2",ui.studio_style?"Sustain":"Sustain level",ui.studio_style?"Rel. level":"Release level"};
            fm_spaced_row(s,rates,rn,4,width,layout.row[0],layout.spacious); fm_spaced_row(s,levels,ln,4,width,layout.row[1],layout.spacious);
            dx7_envelope_graph(s,126,(DrawRect){12,layout.graph,width-24,layout.graph_height});
        }
    }
    fm_footer(c,width,height);
}

static void instrument_group(float x,float y,float width,float height,const char *title) {
    if(!ui.studio_style)return;
    ui_panel(host_draw(),(DrawRect){x,y,width,height},ui_theme.surface);
    if(!ui.profile_flat)ui_bevel(host_draw(),(DrawRect){x,y,width,height},1);
    label(title,x+10,y+10,14,ink);
}

/* Relative horizontal dragging keeps the original 160-unit sensitivity. */
static void fm_slider(FMSettings *s,unsigned id,const char *caption,float width,float y) {
    const ParameterDescriptor *info=parameter_descriptor(id);float *value=(float *)fm_parameter_pointer(s,id);
    int logarithmic=info->kind==PARAMETER_LOGARITHMIC || (info->low>0 && (!strcmp(caption,"Attack") || !strcmp(caption,"Decay") || !strcmp(caption,"Release")));
    float shown=displayed_value(value,*value),fraction=logarithmic?logf(shown/info->low)/logf(info->high/info->low):(shown-info->low)/(info->high-info->low);
    label(caption,22,y+3,10,muted);
    DrawRect number={98,y,62,18},rail={170,y+5,width-194,8};
    ui_surface(host_draw(),number,ui_skin.display);ui_bevel(host_draw(),number,1);
    label(fit_text(fm_value_caption(s,id,shown),58,9),101,y+3,9,ui_theme.signal);
    ui_surface(host_draw(),rail,ui_skin.display);ui_bevel(host_draw(),rail,1);
    float thumb=rail.x+fmaxf(0,fminf(1,fraction))*(rail.width-8);
    ui_surface(host_draw(),(DrawRect){rail.x+1,rail.y+2,fmaxf(0,thumb-rail.x),4},ui_theme.highlight);
    ui_button_face(host_draw(),(DrawRect){thumb,y+1,8,16},ui_theme.control,0);
    int on_number=hover(number.x,number.y,number.width,number.height),on_rail=hover(rail.x,y,rail.width,18);
    if(on_number || on_rail) {
        snprintf(ui.status,sizeof ui.status,"%s: %.4g | Drag %s or wheel; right-click for value / automation",info->name,shown,on_rail?"left/right":"up/down");
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT))control_menu(value,info->low,info->high,info->initial,info->name);
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) {capture_control(value,info->low,info->high,0);ui.control_horizontal=on_rail;ui.control_logarithmic=logarithmic;}
        float wheel=host_get_mouse_wheel_move();if(wheel)control_knob_wheel(&ui.project,value,info->low,info->high,logarithmic?KNOB_LOGARITHMIC:KNOB_NORMAL,wheel);
    }
}

static void fm_analog_overview(float width,float height) {
    int c=ui.instrument_channel;FMSettings *s=&ui.project.fm[c];
    /* Saved 788-unit windows retain their overview under the taller caption.
       Only its plot loses the 14 units; all sliders and footer keep their size. */
    float compact=fmaxf(0,788-height);
    instrument_group(12,110,width-24,234,"Oscillators");
    const char *waves[]={"Saw","Pulse","Triangle","Sine"};
    label("Wave",22,138,10,muted);
    if(choice_button(waves[(int)s->analog.wave],98,134,width-122,22))open_context(23,c,(DrawPoint){ui.mouse.x+desktop_editor_rect(4).x,ui.mouse.y+desktop_editor_rect(4).y});
    if(hover(98,134,width-122,22) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT))control_menu(&s->analog.wave,0,3,0,"Oscillator waveform");
    unsigned oscillator[]={PARAM_FM_CARRIER_RATIO,PARAM_FM_ANALOG_DETUNE,PARAM_FM_ANALOG_MIX,PARAM_FM_ANALOG_SUB,PARAM_FM_ANALOG_NOISE,PARAM_FM_ANALOG_PULSE,PARAM_FM_ANALOG_PWM,PARAM_FM_ANALOG_CHORUS,PARAM_FM_VELOCITY};
    const char *names[]={"Pitch","Detune","Osc 2 mix","Sub","Noise","Pulse width","PWM","Chorus","Velocity"};
    for(int i=0;i<9;i++)fm_slider(s,oscillator[i],names[i],width,158+i*20);
    instrument_group(12,352,width-24,94,"Filter");
    unsigned filter[]={PARAM_FM_FILTER_CUTOFF,PARAM_FM_FILTER_RESONANCE,PARAM_FM_FILTER_ENV};
    const char *filter_names[]={"Cutoff","Resonance","Envelope"};
    for(int i=0;i<3;i++)fm_slider(s,filter[i],filter_names[i],width,376+i*22);
    instrument_group(12,454,width-24,160-compact,"Envelope (AMP)");
    int previous_tab=ui.fm_tab;ui.fm_tab=0;
    fm_envelope_graph(s,(DrawRect){22,480,width-44,96-compact});
    ui.fm_tab=previous_tab;
    /* Compact numeric drags retain the original knob control logic. */
    const char *envelope_names[]={"Attack","Decay","Sustain","Release"};
    float column=(width-44)/4;
    for(int i=0;i<4;i++) {
        unsigned id=ui.fm_envelope_ids[0][i];const ParameterDescriptor *info=parameter_descriptor(id);float *value=(float *)fm_parameter_pointer(s,id);
        float x=22+i*column,y=584-compact;DrawRect field={x,y,column-3,22};
        ui_surface(host_draw(),field,ui_skin.display);ui_bevel(host_draw(),field,1);
        label(host_text_format("%c %s",envelope_names[i][0],fm_value_caption(s,id,displayed_value(value,*value))),x+3,y+6,9,ui_theme.signal);
        if(hover(x,y,column-3,22)) {
            if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT))control_menu(value,info->low,info->high,info->initial,info->name);
            if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)){capture_control(value,info->low,info->high,0);ui.control_logarithmic=i!=2;}
            float wheel=host_get_mouse_wheel_move();if(wheel)control_knob_wheel(&ui.project,value,info->low,info->high,i==2?KNOB_NORMAL:KNOB_LOGARITHMIC,wheel);
        }
    }
    instrument_group(12,622-compact,width-24,124,"LFO (per note)");
    const char *shapes[]={"Sine","Triangle","Saw","Square"};
    if(choice_button(shapes[(int)s->lfo_shape],170,628-compact,width-194,22))open_context(19,c,(DrawPoint){ui.mouse.x+desktop_editor_rect(4).x,ui.mouse.y+desktop_editor_rect(4).y});
    if(hover(170,628-compact,width-194,22) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT))control_menu(&s->lfo_shape,0,3,0,"LFO shape");
    unsigned motion[]={PARAM_FM_LFO_RATE,PARAM_FM_VIBRATO,PARAM_FM_TREMOLO,PARAM_FM_LFO_FADE};
    const char *motion_names[]={"Rate","Vibrato","Tremolo","Fade in"};
    for(int i=0;i<4;i++)fm_slider(s,motion[i],motion_names[i],width,654-compact+i*22);
    fm_footer(c,width,height);
}

void fm_analog_editor(float width,float height) {
    int c=ui.instrument_channel; FMSettings *s=&ui.project.fm[c];
    if(!ui.studio_style)label(fit_text(ui.project.channel_names[c],width-298,14),12,30,14,ink);
    channel_controls(c,width);
    instrument_preset_field(width,fm_sound_name(s));
    const char *tabs[]={"Sound","Amplitude","Filter","Motion"}; float tab=(width-24)/4;
    for(int i=0;i<4;i++) if(button(tabs[i],12+i*tab,78,tab-3,26,ui.fm_analog_tab==i)) ui.fm_analog_tab=i;
    if(ui.studio_style && ui.fm_analog_tab==0 && height>=774){fm_analog_overview(width,height);return;}
    int amplitude=ui.studio_style && ui.fm_analog_tab==1;
    if(ui.studio_style){
        instrument_group(12,108,width-24,amplitude?height-150:68," ");
        if(!amplitude)instrument_group(12,180,width-24,height-222," ");
    }
    if(ui.fm_analog_tab==0) {
        unsigned a[]={PARAM_FM_CARRIER_RATIO,PARAM_FM_ANALOG_DETUNE,PARAM_FM_ANALOG_MIX,PARAM_FM_ANALOG_SUB,PARAM_FM_ANALOG_NOISE},b[]={PARAM_FM_ANALOG_PULSE,PARAM_FM_ANALOG_PWM,PARAM_FM_ANALOG_CHORUS,PARAM_FM_VELOCITY};
        const char *an[]={"Pitch","Detune","Osc 2","Sub","Noise"},*bn[]={"Pulse width","PWM","Chorus","Velocity"};
        label("Oscillators",20,113,12,muted); fm_controls_row(s,a,an,5,width,136);
        const char *waves[]={"Saw","Pulse","Triangle","Sine"};
        if(choice_button(waves[(int)s->analog.wave],12,184,112,24)) open_context(23,c,(DrawPoint){ui.mouse.x+desktop_editor_rect(4).x,ui.mouse.y+desktop_editor_rect(4).y});
        if(hover(12,184,112,24) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) control_menu(&s->analog.wave,0,3,0,"Oscillator waveform");
        fm_controls_row(s,b,bn,4,width,226); instrument_hint("PWM moves the Pulse shape. Chorus spreads the sound in stereo.",width,296,11);
    } else if(ui.fm_analog_tab==1 || ui.fm_analog_tab==2) {
        ui.fm_tab=ui.fm_analog_tab==1?0:1;
        if(ui.fm_analog_tab==2) {
            unsigned a[]={PARAM_FM_FILTER_CUTOFF,PARAM_FM_FILTER_RESONANCE,PARAM_FM_FILTER_ENV,PARAM_FM_VELOCITY}; const char *names[]={"Cutoff","Resonance","Envelope","Velocity"};
            label("Filter",20,113,12,muted); fm_controls_row(s,a,names,4,width,136);
        } else label("Volume envelope",20,113,12,muted);
        unsigned ids[4]; for(int i=0;i<4;i++) ids[i]=ui.fm_envelope_ids[ui.fm_tab][i]; const char *names[]={"Attack","Decay","Sustain","Release"};
        fm_controls_row(s,ids,names,4,width,amplitude?139:207); fm_envelope_graph(s,(DrawRect){12,amplitude?184:252,width-24,height-(amplitude?226:294)});
    } else {
        unsigned ids[]={PARAM_FM_LFO_RATE,PARAM_FM_VIBRATO,PARAM_FM_TREMOLO,PARAM_FM_LFO_FADE}; const char *names[]={"Speed",ui.studio_style?"Pitch amt":"Pitch motion",ui.studio_style?"Vol. amt":"Volume motion","Fade in"};
        label("Per-note modulation",20,113,12,muted); fm_controls_row(s,ids,names,4,width,136);
        const char *waves[]={"Sine","Triangle","Saw","Square"};
        if(choice_button(waves[(int)s->lfo_shape],12,184,112,24)) open_context(19,c,(DrawPoint){ui.mouse.x+desktop_editor_rect(4).x,ui.mouse.y+desktop_editor_rect(4).y});
        if(hover(12,184,112,24) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) control_menu(&s->lfo_shape,0,3,0,"LFO shape");
        instrument_hint("Pitch motion adds vibrato; volume motion adds tremolo. PWM follows this LFO.",width,228,11);
        if(ui.studio_style)lfo_display(s,(DrawRect){12,280,width-24,height-322},s->lfo_rate*.5f);
    }
    fm_footer(c,width,height);
}

/* Dedicated polyphonic Analog panel. Legacy Analog keeps its carried editor. */
static void analog_modern_editor(float width,float height) {
    int c=ui.instrument_channel;FMSettings *s=&ui.project.fm[c];
    channel_controls(c,width);instrument_preset_field(width,fm_sound_name(s));
    const char *tabs[]={"Oscs","Filter","Amplitude","Motion"};float tab=(width-24)/4;
    for(int i=0;i<4;i++)if(button(tabs[i],12+i*tab,78,tab-3,26,ui.fm_analog_tab==i))ui.fm_analog_tab=i;
    if(ui.fm_analog_tab==0) {
        const char *names[]={"Shape","Tune","Fine","Level"};
        for(int osc=0;osc<3;osc++){
            float y=108+osc*70;instrument_group(12,y,width-24,68," ");
            label(host_text_format("%d",osc+1),22,y+12,UI_TEXT_SIZE,muted);
            unsigned ids[4];for(int i=0;i<4;i++)ids[i]=PARAM_VA_FIRST+osc*4+i;
            fm_spaced_row(s,ids,names,4,width,y+20,0);
        }
        unsigned ids[]={PARAM_VA_FIRST+12,PARAM_FM_ANALOG_NOISE,PARAM_VA_FIRST+13,PARAM_FM_ANALOG_PULSE};
        const char *names2[]={"Drive","Noise","Drift","Pulse"};fm_spaced_row(s,ids,names2,4,width,344,0);
    } else if(ui.fm_analog_tab==1 || ui.fm_analog_tab==2) {
        int filter=ui.fm_analog_tab==1;ui.fm_tab=filter?1:0;
        instrument_group(12,108,width-24,height-150," ");
        label(filter?"Ladder filter / envelope":"Amplitude envelope",20,113,12,muted);
        if(filter){unsigned ids[]={PARAM_FM_FILTER_CUTOFF,PARAM_FM_FILTER_RESONANCE,PARAM_FM_FILTER_ENV,PARAM_VA_FIRST+14};const char *names[]={"Cutoff","Resonance","Env depth","Key track"};fm_spaced_row(s,ids,names,4,width,139,0);}
        unsigned ids[4];for(int i=0;i<4;i++)ids[i]=ui.fm_envelope_ids[ui.fm_tab][i];
        const char *names[]={"Attack","Decay","Sustain","Release"};fm_spaced_row(s,ids,names,4,width,filter?207:139,0);
        fm_envelope_graph(s,(DrawRect){12,filter?252:184,width-24,height-(filter?294:226)});
    } else {
        instrument_group(12,108,width-24,68," ");
        unsigned ids[]={PARAM_FM_LFO_RATE,PARAM_FM_VIBRATO,PARAM_FM_TREMOLO,PARAM_FM_LFO_FADE,PARAM_FM_ANALOG_PWM};
        const char *names[]={"Rate","Pitch amt","Vol. amt","Fade","PWM"};fm_spaced_row(s,ids,names,5,width,136,0);
        const char *waves[]={"Sine","Triangle","Saw","Square"};
        if(choice_button(waves[(int)s->lfo_shape],12,184,112,24))open_context(19,c,(DrawPoint){ui.mouse.x+desktop_editor_rect(4).x,ui.mouse.y+desktop_editor_rect(4).y});
        instrument_hint("Per-voice motion. Mixer drive feeds the ladder; chorus is not needed for the basic sound.",width,222,11);
        lfo_display(s,(DrawRect){12,280,width-24,height-322},s->lfo_rate*.5f);
    }
    fm_footer(c,width,height);
}

void fm_editor(float width,float height) {
    FMSettings *s=&ui.project.fm[ui.instrument_channel];
    if(s->engine==1) fm_dx_editor(width,height); else if(s->engine==2 && s->analog.model && ui.studio_style)analog_modern_editor(width,height);else if(s->engine==2) fm_analog_editor(width,height); else fm_legacy_editor(width,height);
    if(ui.studio_style)return;
    const char *engines[]={"Custom FM","Six-op FM","Analog"};
    SixOpLayout layout=sixop_layout(width,height,ui.studio_style);
    int compact=ui.studio_style && !(s->engine==1 && layout.spacious);
    float engine_x=compact?12:width-132,engine_w=compact?width-300:120;
    float engine_y=compact?52:s->engine==1?layout.preset:52,engine_h=s->engine==1 && layout.spacious?28:22;
    if(choice_button(engines[(int)s->engine],engine_x,engine_y,engine_w,engine_h)) open_context(22,ui.instrument_channel,(DrawPoint){ui.mouse.x+desktop_editor_rect(4).x,ui.mouse.y+desktop_editor_rect(4).y});
    if(hover(engine_x,engine_y,engine_w,engine_h)) snprintf(ui.status,sizeof ui.status,"Choose the synthesis engine; factory presets select their own engine");
}

void sampler(float width,float height) {
    int c=ui.instrument_channel;
    Sampler *settings=&ui.project.sampler[c];
    if(!ui.studio_style || channel_missing(c))label(fit_text(channel_caption(c),width-298,14),12,30,14,ink);
    channel_controls(c,width);

    float right=width/2+6,groupw=width/2-18;
    int compact=ui.studio_style && width<SAMPLER_COMPACT_WIDTH;
    int mode_x=compact?right+8:right+116,mode_y=compact?146:98;
    int mode_w=compact?groupw-16:groupw-128,fit_y=compact?174:146;
    int region_y=compact?206:182,region_knob_y=compact?236:212;
    int wave_y=sampler_wave_top(width,ui.studio_style);
    float trim_x=compact?right+groupw-28:right+156;
    if(ui.studio_style){
        instrument_group(12,76,groupw,106," ");
        instrument_group(12,184,groupw,60," ");
        instrument_group(right,76,groupw,compact?124:100," ");
        instrument_group(right,compact?204:178,groupw,compact?68:66," ");
    }
    label(fit_text(!ui.originals[c].frames?"Drop a sample from the Browser":ui.project.paths[c][0]?host_get_file_name(ui.project.paths[c]):"Built-in sample",compact?width-24:width-298,12),12,ui.studio_style?wave_y-14:54,12,muted);
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
    label(ui.studio_style?"Pitch":"Pitch shift",right+4,128,11,muted); label("Time",right+71,128,11,muted);
    if(choice_button(settings->stretch?"Stretch":"Resample",mode_x,mode_y,mode_w,22)) open_context(13,c,(DrawPoint){ui.mouse.x+desktop_editor_rect(4).x,ui.mouse.y+desktop_editor_rect(4).y});
    if(!ui.studio_style)host_draw_triangle((DrawPoint){mode_x+mode_w-23,mode_y+9},(DrawPoint){mode_x+mode_w-19,mode_y+14},(DrawPoint){mode_x+mode_w-15,mode_y+9},muted);
    if(!compact)label("Mode",right+128,128,11,muted);
    if(sampler_toggle(right+8,fit_y,groupw-16,"Fit to tempo",settings->fit_bpm>0)) {
        settings->fit_bpm=settings->fit_bpm?0:ui.project.bpm;
    }
    if(hover(right+8,fit_y,groupw-16,22)) snprintf(ui.status,sizeof ui.status,"Fit to tempo: keep audio fixed on the grid when BPM changes; uses Resample/Stretch mode");
    if(hover(mode_x,mode_y,mode_w,22)) snprintf(ui.status,sizeof ui.status,"Time mode: Resample changes speed/pitch; Stretch preserves pitch; click to choose");
    label("Presets",20,190,11,muted);
    if(button("Load",20,210,(groupw-20)/2,24,0)) preset_action(PRESET_SAMPLER,c,0,0);
    if(button("Save",24+(groupw-20)/2,210,(groupw-20)/2,24,0)) preset_action(PRESET_SAMPLER,c,0,1);
    label("Sample region",right+8,region_y,12,muted);
    knob(right+28,region_knob_y,&settings->start,0,1,0,"Start");
    knob(right+84,region_knob_y,&settings->length,0,1,1,"Length");
    label("Start",right+14,region_knob_y+18,11,muted); label("Length",right+67,region_knob_y+18,11,muted);
    knob(trim_x,region_knob_y,&settings->trim,0,1,0,"Trim quiet beginning and end (threshold)");
    label("Trim",trim_x-13,region_knob_y+18,11,muted);
    int pending=!sampler_processing_equal(*settings,ui.sampler_applied[c]);
    int live_preview=sampler_live_preview(c);
    float duration=live_preview?sampler_view_frames(c)/(float)RATE:ui.samples[c].frames/(float)RATE;

    if(settings->fit_bpm) duration*=settings->fit_bpm/ui.project.bpm;

    Sample sample=ui.samples[c];
    if(live_preview) sample.frames=sampler_view_frames(c);
    float area=height-wave_y-24;
    DrawRect display={12,wave_y,width-24,area};
    if(ui.studio_style)ui_graph_paper(host_draw(),display);
    else host_draw_rectangle_rec(display,bg);
    if(ui.studio_style)host_draw_rectangle_lines(display.x,display.y,display.width,display.height,ui_theme.border);
    draw_waveform((WaveDisplay){.sample=sample,.wave=live_preview?NULL:processed_waveform(c),.channel=c,.preview=live_preview,
        .origin=12,.width=width-24},display,12,width-12,ui_theme.waveform);
    double progress=audio_key_position(127,c);
    if(hover(12,wave_y,width-24,area)) {
        snprintf(ui.status,sizeof ui.status,"Sample waveform: click to %s; drop a Browser sample here to load it",progress>=0?"stop playback":"play from the beginning");
        if(ui.samples[c].frames && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) { ui.channel=c; audio_key(127,c,60,progress<0); }
    }
    if(progress>=0 && !live_preview) {
        float x=12+progress*(width-24);
        host_draw_line_ex((DrawPoint){x,wave_y},(DrawPoint){x,wave_y+area},1.5f,ui.studio_style?ui_theme.signal:ink);
    }
    label("0 s",16,wave_y+2,10,ui.studio_style?(DrawColor){193,208,204,255}:muted);
    label(host_text_format("%.2f s%s",duration,pending?" *":""),width-78,wave_y+2,10,ui.studio_style?(DrawColor){193,208,204,255}:muted);
    if(live_preview && hover(12,wave_y,width-24,area)) snprintf(ui.status,sizeof ui.status,"Live envelope preview; processed pitch/stretch detail appears when ready; audio changes after release");
    if(ui.sample_drag[0] && ui.sample_moved) host_draw_rectangle_lines(12,wave_y,width-24,area,ui_theme.signal);

}

/* One channel-owned page shared by every instrument; synthesis pages remain separate. */
void instrument_voice_editor(float width,float height) {
    int c=ui.instrument_channel;VoiceSettings *v=&ui.project.voice[c];
    channel_controls(c,width);
    label("Voice",16,31,12,ink);
    float half=(width-36)/2,right=24+half;
    instrument_group(12,80,half,146,"Levels");
    instrument_group(right,80,half,146,"Polyphony");
    knob_style(12+half*.28f,139,&ui.project.pan[c],-1,1,0,"Channel pan",KNOB_PAN);
    knob_style(12+half*.72f,139,&ui.project.volume[c],0,VOLUME_KNOB_MAX,1,"Channel volume",KNOB_VOLUME);
    label("Pan",12+half*.28f-12,164,12,muted);label("Volume",12+half*.72f-24,164,12,muted);
    label("Max",right+12,112,12,muted);
    if(drag_button(v->limit?host_text_format("%.0f",v->limit):"All",right+56,108,54,24,ui.control_drag==&v->limit)){capture_control(&v->limit,0,128,0);ui.control_integer=1;}
    if(hover(right+56,108,54,24)){
        v->limit=fmaxf(0,fminf(128,v->limit+roundf(host_get_mouse_wheel_move())));
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)){begin_number(&v->limit,0,128,0,"Voice limit (0 = all)");ui.number_integer=1;}
        snprintf(ui.status,sizeof ui.status,"Maximum simultaneous voices: 0 = all; oldest voices are replaced first. Mono uses last-note priority.");
    }
    if(sampler_toggle(right+12,151,half-24,"Mono",v->mono))v->mono=!v->mono;
    if(sampler_toggle(right+12,181,half-24,"Legato",v->legato))v->legato=!v->legato;
    if(hover(right+12,151,half-24,52))snprintf(ui.status,sizeof ui.status,"Mono: last held note wins; releasing it returns to the previous held note. Legato keeps envelopes running across overlapping notes in Mono mode.");
    instrument_group(12,238,half,148,"Pitch");
    instrument_group(right,238,half,148,"Portamento");
    knob_style(12+half*.28f,294,&ui.project.channel_pitch[c],-1,1,0,"Playback pitch",KNOB_CENTER);
    label("Pitch",12+half*.28f-20,319,12,muted);
    label("Range",12+half*.72f-20,265,12,muted);
    if(drag_button(host_text_format("%.0f",ui.project.pitch_range[c]),12+half*.72f-22,290,44,24,ui.control_drag==&ui.project.pitch_range[c])){capture_control(&ui.project.pitch_range[c],1,48,0);ui.control_integer=1;}
    label("semitones",12+half*.72f-36,321,10,muted);
    knob_style(right+half*.5f,294,&v->glide,0,2,0,"Glide time (seconds)",KNOB_NORMAL);
    label("Glide",right+half*.5f-20,319,12,muted);
    const char *time=v->glide?host_text_format("%.0f ms",v->glide*1000):"Off";
    label(time,right+(half-text_width(time,12))/2,345,12,ink);
    if(hover(right+12,260,half-24,114))snprintf(ui.status,sizeof ui.status,"Glide: constant time between overlapping Mono notes, 0–2000 ms. Zero changes pitch immediately; Legato separately controls envelope retriggering.");
    instrument_hint(ui.project.channel_audio[c]?"Audio clips keep their arrangement timing.":"Mono: last held note wins. Legato keeps envelopes running.",width,397,11);
    (void)height;
}
