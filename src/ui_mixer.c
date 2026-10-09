// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"

int mixer_input_at(Vector2 point) {
    Rect r=ui.windows.editors[3].rect; if(!ui.windows.editors[3].visible) return -1;
    int capacity=fmaxf(1,(r.w-MIXER_PANEL-MIXER_LEFT-8)/51-1),visible=fminf(ui.project.insert_count,capacity);
    for(int col=0;col<=visible;col++) if(CheckCollisionPointRec(point,(Rectangle){r.x+MIXER_LEFT+col*51+7,r.y+r.h-46,12,12})) return col?col+ui.mixer_scroll:0;
    return -1;
}

void meters_update(void) {
    float peaks[INSERTS+1][2]; audio_meters(peaks);
    double now=GetTime(),elapsed=fmax(0,now-ui.meter_time); ui.meter_time=now;
    float release=expf(-elapsed/.18),peak_release=expf(-elapsed/.45); ui.mixer_decay_active=0;
    for(int id=0;id<=INSERTS;id++) for(int side=0;side<2;side++) {
        ui.meter_level[id][side]=fmaxf(peaks[id][side],ui.meter_level[id][side]*release);
        if(peaks[id][side]>=ui.meter_hold[id][side] && peaks[id][side]>.00001f) { ui.meter_hold[id][side]=peaks[id][side]; ui.meter_until[id][side]=now+.5; }
        else if(now>ui.meter_until[id][side]) ui.meter_hold[id][side]*=peak_release;
        ui.mixer_decay_active|=ui.meter_hold[id][side]>.001f || ui.meter_level[id][side]>.001f;
    }
}

void spectrum_update(void) {
    static int initialized,last_bus=-1;
    if(!initialized) { spectrum_reset(&ui.master_spectrum); spectrum_reset(&ui.eq_spectrum); initialized=1; }
    int bus=ui.windows.editors[5].visible?ui.eq_bus:-1;
    audio_spectrum_bus(bus);
    float pcm[8192*2]; unsigned n;
    n=audio_spectrum_read(0,pcm,8192); spectrum_push(&ui.master_spectrum,pcm,n);
    n=audio_spectrum_read(1,pcm,8192);
    if(bus!=last_bus) { spectrum_reset(&ui.eq_spectrum); last_bus=bus; }
    else spectrum_push(&ui.eq_spectrum,pcm,n);
    ui.spectrum_active=0;
    for(int i=0;i<SPECTRUM_BINS;i++) ui.spectrum_active|=ui.master_spectrum.db[i]>-78 || (bus>=0 && ui.eq_spectrum.db[i]>-78);
}

void spectrum_draw(const Spectrum *s,Rectangle r,int colored) {
    for(int i=0;i<SPECTRUM_BINS;i++) {
        float height=fmaxf(0,fminf(1,(s->db[i]+78)/78))*r.height;
        Color color=colored?ColorFromHSV(260-240.f*i/(SPECTRUM_BINS-1),.75f,1):accent;
        float x=r.x+r.width*i/SPECTRUM_BINS,w=r.width/SPECTRUM_BINS;
        DrawRectangleRec((Rectangle){x,r.y+r.height-height,w+.1f,height},color);
    }
}

float eq_x(float frequency,Rectangle r) { return r.x+logf(frequency/20)/logf(1000)*r.width; }

float eq_y(float gain,Rectangle r) { return r.y+r.height*(.5f-gain/36); }

void eq_editor(float width,float height) {
    if(ui.eq_bus<0 || ui.eq_bus>ui.project.insert_count || ui.eq_slot<0 || ui.eq_slot>=EFFECT_SLOTS || ui.project.effect_type[ui.eq_bus][ui.eq_slot]!=EFFECT_EQ) {
        ui.eq_drag=-1; label("Select an Equalizer in the Mixer.",16,40,14,muted); return;
    }
    EQSettings *settings=&ui.project.eq[ui.eq_bus][ui.eq_slot],shown=*settings;
    for(int i=0;i<EQ_BANDS;i++) {
        shown.bands[i].frequency=displayed_value(&settings->bands[i].frequency,settings->bands[i].frequency);
        shown.bands[i].gain=displayed_value(&settings->bands[i].gain,settings->bands[i].gain);
        shown.bands[i].q=displayed_value(&settings->bands[i].q,settings->bands[i].q);
    }
    label(fit_text(TextFormat("%s · Slot %d",ui.eq_bus?ui.project.insert_names[ui.eq_bus-1]:"Master",ui.eq_slot+1),width-254,12),12,30,12,muted);
    if(button("Bypass",width-226,26,70,24,ui.project.effect_bypass[ui.eq_bus][ui.eq_slot])) ui.project.effect_bypass[ui.eq_bus][ui.eq_slot]^=1;
    if(button("Load",width-152,26,66,24,0)) preset_action(PRESET_EQ,ui.eq_bus,ui.eq_slot,0);
    if(button("Save",width-82,26,70,24,0)) preset_action(PRESET_EQ,ui.eq_bus,ui.eq_slot,1);
    Rectangle graph={42,64,width-58,height-196};
    DrawRectangleRec(graph,bg); spectrum_draw(&ui.eq_spectrum,graph,1);
    const float frequencies[]={20,100,1000,10000,20000};
    const char *names[]={"20","100","1k","10k","20k"};
    for(int i=0;i<5;i++) {
        float x=eq_x(frequencies[i],graph);
        DrawLineEx((Vector2){x,graph.y},(Vector2){x,graph.y+graph.height},1,ui_theme.grid_minor);
        label(names[i],fminf(width-32,fmaxf(32,x-text_width(names[i],10)/2)),graph.y+graph.height+5,10,muted);
    }
    for(int db=-18;db<=18;db+=6) {
        float y=eq_y(db,graph);
        DrawLineEx((Vector2){graph.x,y},(Vector2){graph.x+graph.width,y},1,db?ui_theme.grid_minor:muted);
        label(TextFormat("%+d",db),8,y-5,10,muted);
    }
    static EQSettings drawn; static float response[257]; static int cached;
    if(!cached || memcmp(&drawn,&shown,sizeof drawn)) {
        for(int i=0;i<=256;i++) response[i]=equalizer_response(shown,spectrum_frequency(i/256.f));
        drawn=shown; cached=1;
    }
    Vector2 last={0};
    for(int x=0;x<=256;x++) {
        float db=response[x];
        Vector2 next={graph.x+graph.width*x/256,eq_y(fmaxf(-18,fminf(18,db)),graph)};
        if(x) {
            smooth_line(last,next,4,BLACK);
            smooth_line(last,next,1.8f,ink);
        }
        last=next;
    }
    for(int i=0;i<EQ_BANDS;i++) {
        EQBand b=shown.bands[i]; Vector2 p={eq_x(b.frequency,graph),eq_y(b.shape==EQ_LOW_CUT || b.shape==EQ_HIGH_CUT?0:b.gain,graph)};
        int over=hover(p.x-10,p.y-10,20,20);
        circle(p.x,p.y,ui.eq_band==i?8:6,b.shape==EQ_OFF?muted:accent);
        label(TextFormat("%d",i+1),p.x-text_width(TextFormat("%d",i+1),10)/2,p.y-5,10,ui_theme.selected_text);
        if(over) {
            snprintf(ui.status,sizeof ui.status,"EQ band %d: %.0f Hz, %+.1f dB | drag to tune; wheel changes Q; right-click to reset",i+1,b.frequency,b.gain);
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.eq_band=i; ui.eq_drag=i; }
            float wheel=GetMouseWheelMove();
            if(wheel) settings->bands[i].q=fmaxf(.2f,fminf(10,settings->bands[i].q+wheel*.1f));
            if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                ui.eq_band=i; open_context(16,i,(Vector2){ui.mouse.x+ui.windows.editors[5].rect.x,ui.mouse.y+ui.windows.editors[5].rect.y}); ui.input_enabled=0;
            }
        }
    }
    if(ui.eq_drag>=0) {
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            EQBand *b=&settings->bands[ui.eq_drag];
            b->frequency=spectrum_frequency(fmaxf(0,fminf(1,(ui.mouse.x-graph.x)/graph.width)));
            if(b->shape!=EQ_LOW_CUT && b->shape!=EQ_HIGH_CUT) b->gain=fmaxf(-18,fminf(18,(.5f-(ui.mouse.y-graph.y)/graph.height)*36));
        } else ui.eq_drag=-1;
    }
    label("Spectrum: -78 to 0 dBFS",graph.x,graph.y+graph.height+22,10,muted);
    float y=height-55;
    for(int i=0;i<EQ_BANDS;i++) if(button(TextFormat("%d",i+1),12+i*28,y-34,24,22,ui.eq_band==i)) ui.eq_band=i;
    EQBand *b=&settings->bands[ui.eq_band];
    if(button(equalizer_shape_name(b->shape),216,y-34,152,22,0)) open_context(14,ui.eq_band,(Vector2){ui.mouse.x+ui.windows.editors[5].rect.x,ui.mouse.y+ui.windows.editors[5].rect.y});
    if(hover(216,y-34,152,22)) snprintf(ui.status,sizeof ui.status,"Band shape: Bell, shelves, cuts or Off");
    knob_style(92,y+4,&b->frequency,20,20000,equalizer_default().bands[ui.eq_band].frequency,"EQ frequency (Hz)",KNOB_LOGARITHMIC);
    int cut=b->shape==EQ_LOW_CUT || b->shape==EQ_HIGH_CUT;
    if(cut) label("12 dB/oct",174,y,11,muted);
    else knob_style(200,y+4,&b->gain,-18,18,0,"EQ gain (dB)",KNOB_CENTER);
    knob(310,y+4,&b->q,.2f,10,equalizer_default().bands[ui.eq_band].q,"EQ Q (bandwidth)");
    label(TextFormat("%.0f Hz",b->frequency),66,y+24,11,muted);
    label(cut?"Slope":TextFormat("%+.1f dB",b->gain),178,y+24,11,muted);
    label(TextFormat("Q %.2f",b->q),288,y+24,11,muted);
    knob(width-44,y+4,&ui.project.effect_mix[ui.eq_bus][ui.eq_slot],0,1,1,"Equalizer mix");
    label("Mix",width-54,y+24,11,muted);
}

float meter_height(float level) { return level>.001f?fader_position(level):0; }

void meter_draw(int id,int x,int top,int bottom,int bar_width) {
    int height=bottom-top,spacing=bar_width+2;
    float orange=meter_height(powf(10,-12.f/20)),zero=meter_height(1);
    DrawRectangle(x-1,top,spacing*2,height,bg);
    for(int side=0;side<2;side++) {
        int left=x+side*spacing; float level=meter_height(ui.meter_level[id][side]),peak=meter_height(ui.meter_hold[id][side]);
        if(level>0) DrawRectangle(left,bottom-level*height,bar_width,level*height,ui_theme.meter_low);
        if(level>orange) DrawRectangle(left,bottom-level*height,bar_width,(level-orange)*height,ui_theme.meter_mid);
        if(level>zero) DrawRectangle(left,bottom-level*height,bar_width,(level-zero)*height,ui_theme.meter_high);
        if(peak>0) DrawRectangle(left-1,fmaxf(top,bottom-peak*height),bar_width+2,1,ui.meter_hold[id][side]>1?ui_theme.meter_high:ink);
    }
    if(hover(x-3,top,spacing*2+4,height)) snprintf(ui.status,sizeof ui.status,"%s output | Peak L %.1f dBFS, R %.1f dBFS | Red above 0 dBFS; markers hold 0.5 s",mixer_name(id),gain_db(ui.meter_hold[id][0]),gain_db(ui.meter_hold[id][1]));
}

void selected_meter(float height) {
    int top=TITLE+28,bottom=height-34;
    label("dBFS",8,TITLE+6,11,muted);
    meter_draw(ui.mixer_selected,32,top,bottom,7);
    const int ticks[]={6,0,-6,-12,-24,-36,-48,-60};
    for(unsigned i=0;i<sizeof ticks/sizeof *ticks;i++) {
        float y=bottom-meter_height(powf(10,ticks[i]/20.f))*(bottom-top);
        const char *number=TextFormat("%d",ticks[i]);
        label(number,16-text_width(number,10)/2,fmaxf(top,fminf(bottom-10,y-5)),10,muted);
        DrawLine(29,y,31,y,muted);
    }
    float peak=fmaxf(ui.meter_hold[ui.mixer_selected][0],ui.meter_hold[ui.mixer_selected][1]);
    label(peak?TextFormat("%.1f",gain_db(peak)):"-inf",8,height-29,11,peak>1?ui_theme.meter_high:ink);
    if(hover(4,TITLE,54,height-TITLE-18)) snprintf(ui.status,sizeof ui.status,"Selected %s | Peak L %.1f dBFS, R %.1f dBFS",mixer_name(ui.mixer_selected),gain_db(ui.meter_hold[ui.mixer_selected][0]),gain_db(ui.meter_hold[ui.mixer_selected][1]));
}

void mixer(float width,float height) {
    selected_meter(height);
    int capacity=fmaxf(1,(width-MIXER_PANEL-MIXER_LEFT-8)/51-1),visible=ui.project.insert_count<capacity?ui.project.insert_count:capacity;
    ui.mixer_scroll=(int)fmaxf(0,fminf(fmaxf(0,ui.project.insert_count-visible),ui.mixer_scroll));
    if(hover(0,TITLE,width-220,height-TITLE) && ui.mouse.y>TITLE+60) {
        ui.mixer_scroll=(int)fmaxf(0,fminf(fmaxf(0,ui.project.insert_count-visible),ui.mixer_scroll-GetMouseWheelMove()));
    }
    float fader_top=TITLE+76,fader_bottom=height-74;
    for(int col=0;col<=visible;col++) {
        int id=col?col+ui.mixer_scroll:0,x=MIXER_LEFT+col*51;
        float *v=id?&ui.project.insert_volume[id-1]:&ui.project.master;
        int over=hover(x,TITLE+4,48,height-TITLE-12);
        ui_surface((Rectangle){x,TITLE+4,48,height-TITLE-12},ui.mixer_selected==id?ui_theme.title_focus:cell);
        if(over) snprintf(ui.status,sizeof ui.status,"%s: click to select this mixer channel",mixer_name(id));
        if(over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.mixer_selected=id;
        DrawRectangle(x+3,TITLE+7,42,17,ui.mixer_selected==id?ui_theme.title_focus:cell);
        label(id?TextFormat("%d",id):"Master",x+5,TITLE+9,id?12:10,ink);
        if(id) {
            label(fit_text(ui.project.insert_names[id-1],42,10),x+3,TITLE+26,10,muted);
            if(hover(x+3,TITLE+25,42,16)) {
                snprintf(ui.status,sizeof ui.status,"%s: right-click to rename",ui.project.insert_names[id-1]);
                if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    ui.rename_channel=ui.rename_track=-1; ui.rename_mixer=id-1; ui.mixer_selected=id;
                    snprintf(ui.rename_text,sizeof ui.rename_text,"%s",ui.project.insert_names[id-1]); ui.rename_select_all=1; open_popup(2); ui.input_enabled=0;
                }
            }
        }
        int input_over=hover(x+7,height-46,12,12) || (ui.cable_drag>0 && CheckCollisionPointRec(ui.mouse,(Rectangle){x+7,height-46,12,12}));
        DrawRectangleLines(x+9,height-44,8,8,input_over?ui_theme.signal:muted);

        if(input_over) snprintf(ui.status,sizeof ui.status,"%s input: drop an insert output cable here",mixer_name(id));
        if(over && ui.mouse.y<TITLE+24 && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { ui.mixer_selected=id; open_context(3,id,(Vector2){ui.mouse.x+ui.windows.editors[3].rect.x,ui.mouse.y+ui.windows.editors[3].rect.y}); }
        if(over && ui.mouse.y<TITLE+24) snprintf(ui.status,sizeof ui.status,"%s: right-click for mixer channel actions",mixer_name(id));
        mute_light(x+11,TITLE+52,id?ui.project.insert_mute:&ui.project.master_mute,id?ui.project.insert_count:0,id?id-1:0,mixer_name(id));
        if(id) knob_style(x+35,TITLE+52,&ui.project.insert_pan[id-1],-1,1,0,"Insert pan",KNOB_PAN);
        meter_draw(id,x+8,fader_top,fader_bottom,3);
        DrawRectangle(x+33,fader_top,3,fader_bottom-fader_top,bg);
        if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(v,0,MIXER_GAIN_MAX,1,id?"Insert volume":"Master volume");
        if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            capture_control(v,0,MIXER_GAIN_MAX,1); ui.control_range=fader_bottom-fader_top;
            /* Anchor the drag to the current handle position rather than the click position. */
            ui.control_bottom=ui.mouse.y+ui.windows.editors[3].rect.y+fader_position(*v)*ui.control_range;
        }
        if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) || ui.control_drag==v) SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
        if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) || ui.control_drag==v) snprintf(ui.status,sizeof ui.status,"%s fader: %.1f dB | Mark = 0 dB; right-click for value / automation",mixer_name(id),gain_db(*v));
        DrawLine(x+25,fader_bottom-fader_position(1)*(fader_bottom-fader_top),x+44,fader_bottom-fader_position(1)*(fader_bottom-fader_top),muted);
        int fy=fader_bottom-fader_position(displayed_value(v,*v))*(fader_bottom-fader_top);
        ui_fader_handle((Rectangle){x+26,fy-9,14,18},ui.mixer_selected==id);
        knob_style(x+33,height-59,id?&ui.project.insert_width[id-1]:&ui.project.master_width,0,2,1,"Stereo width (left wider, center unchanged, right mono)",KNOB_WIDTH);
        int arm_over=hover(x+4,height-67,16,16);
        circle(x+12,height-59,6,arm_over || ui.record_armed[id]?ui_theme.meter_high:ui_theme.border);
        circle(x+12,height-59,5,ui.record_armed[id]?ui_theme.meter_high:cell);
        if(arm_over) {
            snprintf(ui.status,sizeof ui.status,"%s record arm: %s | Top-bar Record captures this track's post-fader audio",mixer_name(id),ui.record_armed[id]?"On":"Off");
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !recording_active()) { ui.record_armed[id]^=1; ui.input_enabled=0; }
        }
        if(id) {

            int output_over=hover(x+29,height-46,12,12);
            circle(x+35,height-40,5,output_over || ui.cable_drag==id?accent:muted);
            circle(x+35,height-40,2,ui.project.insert_output[id-1]==255?bg:accent);
            if(output_over) {
                snprintf(ui.status,sizeof ui.status,"Insert %d output: drag to an input; right-click to unplug",id);
                if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) insert_connect(&ui.project,id,255);
                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.cable_drag=id; ui.mixer_selected=id; ui.input_enabled=0; }
            }
        }
    }
    if(ui.mixer_selected>0) {
        int col=ui.mixer_selected-ui.mixer_scroll,destination=ui.project.insert_output[ui.mixer_selected-1];
        int target=destination?destination-ui.mixer_scroll:0;
        if(col>=1 && col<=visible && (ui.cable_drag==ui.mixer_selected || (destination!=255 && target>=0 && target<=visible && (!destination || target>=1)))) {
            Vector2 start={MIXER_LEFT+col*51+35,height-40},end=ui.cable_drag==ui.mixer_selected?ui.mouse:(Vector2){MIXER_LEFT+target*51+13,height-40};
            Vector2 points[]={start,{start.x,height-21},{end.x,height-21},end};
            cable(points,Fade(ui_theme.signal,.8f));
        }
    }
    int fx=width-MIXER_PANEL;
    DrawRectangle(fx,TITLE,MIXER_PANEL,height-TITLE,ui_theme.mixer);
    label("Recording input",fx+8,TITLE+8,11,muted);
    device_button(0,fx+8,TITLE+26,MIXER_PANEL-16);
    label("Effects",fx+8,TITLE+70,12,ink);
    for(int slot=0;slot<EFFECT_SLOTS;slot++) {
        int x=fx+8+(slot%5)*29,y=TITLE+90+(slot/5)*24;
        if(button(TextFormat("%d%s",slot+1,ui.project.effect_type[ui.mixer_selected][slot]?"·":""),x,y,27,22,ui.effect_slot==slot)) ui.effect_slot=slot;
        if(hover(x,y,27,22)) snprintf(ui.status,sizeof ui.status,"Effect slot %d: %s",slot+1,ui.project.effect_type[ui.mixer_selected][slot]?effect_descriptor(ui.project.effect_type[ui.mixer_selected][slot])->name:"Empty");
    }
    int bus=ui.mixer_selected,slot=ui.effect_slot;
    if(ui.project.effect_type[bus][slot]==EFFECT_EMPTY) {
        if(button("+ Chorus",fx+8,TITLE+148,MIXER_PANEL-16,25,0)) {
            ui.project.effect_type[bus][slot]=EFFECT_CHORUS; effect_descriptor(EFFECT_CHORUS)->defaults(&ui.project.chorus[bus][slot]);
            ui.project.effect_mix[bus][slot]=.5f; ui.project.effect_bypass[bus][slot]=0;
        }
        if(button("+ Equalizer",fx+8,TITLE+177,MIXER_PANEL-16,25,0)) {
            ui.project.effect_type[bus][slot]=EFFECT_EQ; effect_descriptor(EFFECT_EQ)->defaults(&ui.project.eq[bus][slot]); ui.project.effect_mix[bus][slot]=1; ui.project.effect_bypass[bus][slot]=0;
            ui.eq_bus=bus; ui.eq_slot=slot; ui.eq_drag=-1; windows_focus(&ui.windows,5);
        }
    } else {
        label(effect_descriptor(ui.project.effect_type[bus][slot])->name,fx+8,TITLE+148,12,ink);
        if(button("x",fx+MIXER_PANEL-28,TITLE+144,20,22,0)) {
            ui.project.effect_type[bus][slot]=EFFECT_EMPTY;
            for(int a=ui.project.automation_count-1;a>=0;a--) {
                ParameterTarget t=ui.project.automations[a].target;
                if(((t.parameter>=PARAM_CHORUS_RATE && t.parameter<=PARAM_EFFECT_MIX) || (t.parameter>=PARAM_EQ_FIRST && t.parameter<=PARAM_EQ_LAST)) && t.owner==(unsigned)bus && t.slot==(unsigned)slot) automation_delete(&ui.project,a);
            }
            ui.control_drag=NULL; ui.automation_selected=ui.automation_node=-1; ui.picker_drag=-1;
        } else if(ui.project.effect_type[bus][slot]==EFFECT_EQ) {
            if(button("Open Equalizer",fx+8,TITLE+177,MIXER_PANEL-16,25,0)) { ui.eq_bus=bus; ui.eq_slot=slot; ui.eq_drag=-1; windows_focus(&ui.windows,5); }
            if(button("Bypass",fx+8,TITLE+225,MIXER_PANEL-16,24,ui.project.effect_bypass[bus][slot])) ui.project.effect_bypass[bus][slot]^=1;
            if(button("Load",fx+8,TITLE+253,70,24,0)) preset_action(PRESET_EQ,bus,slot,0);
            if(button("Save",fx+82,TITLE+253,70,24,0)) preset_action(PRESET_EQ,bus,slot,1);
        } else {
            ChorusSettings *settings=&ui.project.chorus[bus][slot];
            knob(fx+27,TITLE+183,&settings->rate,.05f,5,.513f,"Chorus rate (Hz)");
            knob(fx+78,TITLE+183,&settings->depth,0,8,1.85f,"Chorus depth (ms)");
            knob(fx+129,TITLE+183,&ui.project.effect_mix[bus][slot],0,1,.5f,"Chorus mix");
            label("Rate",fx+15,TITLE+203,10,muted); label("Depth",fx+64,TITLE+203,10,muted); label("Mix",fx+120,TITLE+203,10,muted);
            if(button("Bypass",fx+8,TITLE+225,MIXER_PANEL-16,24,ui.project.effect_bypass[bus][slot])) ui.project.effect_bypass[bus][slot]^=1;
            if(button("Load",fx+8,TITLE+253,70,24,0)) preset_action(PRESET_CHORUS,bus,slot,0);
            if(button("Save",fx+82,TITLE+253,70,24,0)) preset_action(PRESET_CHORUS,bus,slot,1);
        }
    }
    float area=fx-MIXER_LEFT-8,thumb=area*visible/INSERTS,pos=MIXER_LEFT+area*ui.mixer_scroll/INSERTS;
    DrawRectangle(MIXER_LEFT,height-17,area,9,bg); ui_surface((Rectangle){pos,height-17,fmaxf(12,thumb),9},ui.mixer_pan || hover(MIXER_LEFT,height-20,area,14)?accent:muted);
    if(hover(MIXER_LEFT,height-20,area,14)) {
        snprintf(ui.status,sizeof ui.status,"Mixer: wheel over inserts or drag this scrollbar to browse all 100 inserts");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.mixer_pan=1; ui.mixer_scroll=fmaxf(0,fminf(INSERTS-visible,(ui.mouse.x-MIXER_LEFT-thumb/2)/area*INSERTS)); ui.input_enabled=0; }
    }
}
