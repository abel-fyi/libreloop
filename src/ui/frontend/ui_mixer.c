// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"
#include "signal_palette.h"

int mixer_input_at(DrawPoint point) {
    Rect r=desktop_editor_rect(3); if(!ui.windows.editors[3].visible) return -1;
    if(ui.studio_style && ui.windows.workspace.enabled)return mixer_route_hit(mixer_view(r.w,r.h,ui.project.insert_count,ui.mixer_offset,ui.mixer_fx_hidden),point.x-r.x,point.y-r.y);
    int capacity=fmaxf(1,(r.w-MIXER_PANEL-MIXER_LEFT-8)/mixer_strip_width()-1),visible=fminf(ui.project.insert_count,capacity);
    for(int col=0;col<=visible;col++) if(host_check_collision_point_rec(point,(DrawRect){r.x+MIXER_LEFT+col*mixer_strip_width()+7,r.y+mixer_route_y(r.h),12,12})) return col?col+ui.mixer_scroll:0;
    return -1;
}

void meters_update(void) {
    float peaks[INSERTS+1][2]; audio_meters(peaks);
    double now=host_get_time(),elapsed=fmax(0,now-ui.meter_time); ui.meter_time=now;
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
    int bus=ui.windows.editors[5].visible && (!ui.windows.workspace.enabled || ui.project.effect_type[ui.eq_bus][ui.eq_slot]==EFFECT_EQ)?ui.eq_bus:-1;
    if(ui.windows.workspace.enabled) {
        bus=-1;for(int i=windows_count(&ui.windows)-1;i>=0;i--) {int id=ui.windows.order[i];if(id>=5 && ui.windows.editors[id].visible){int b=ui.effect_windows[id].bus,slot=ui.effect_windows[id].slot;if(ui.project.effect_type[b][slot]==EFFECT_EQ){bus=b;break;}}}
        ui.eq_spectrum_bus=bus;
    }
    audio_spectrum_bus(bus);
    float pcm[8192*2]; unsigned n;
    n=audio_spectrum_read(0,pcm,8192); spectrum_push(&ui.master_spectrum,pcm,n);
    n=audio_spectrum_read(1,pcm,8192);
    if(bus!=last_bus) { spectrum_reset(&ui.eq_spectrum); last_bus=bus; }
    else spectrum_push(&ui.eq_spectrum,pcm,n);
    ui.spectrum_active=0;
    for(int i=0;i<SPECTRUM_BINS;i++) ui.spectrum_active|=ui.master_spectrum.db[i]>-78 || (bus>=0 && ui.eq_spectrum.db[i]>-78);
}

void spectrum_draw(const Spectrum *s,DrawRect r,int colored) {
    if(ui.studio_style && !colored)ui_display_grid(host_draw(),r,6,4);
    if(ui.studio_style) {
        DrawVertex fill[6*(SPECTRUM_BINS+1)];int count=0;
        DrawColor body={65,119,94,155},edge={148,191,152,230};
        float base=r.y+r.height;
        DrawPoint previous={r.x,base-fmaxf(0,fminf(1,(s->db[0]+78)/78))*r.height};
        for(int i=0;i<=SPECTRUM_BINS;i++) {
            int bin=i<SPECTRUM_BINS?i:SPECTRUM_BINS-1;
            float height=fmaxf(0,fminf(1,(s->db[bin]+78)/78))*r.height;
            DrawPoint point={i<SPECTRUM_BINS?r.x+r.width*(i+.5f)/SPECTRUM_BINS:r.x+r.width,base-height};
            DrawPoint corners[]={previous,{previous.x,base},{point.x,base},previous,{point.x,base},point};
            for(int k=0;k<6;k++)fill[count++]=(DrawVertex){corners[k],0,0,body};
            previous=point;
        }
        draw_triangles(host_draw(),NULL,fill,count);
        /* Outline above the fill, preserving each bin's exact center/height. */
        previous=(DrawPoint){r.x,base-fmaxf(0,fminf(1,(s->db[0]+78)/78))*r.height};
        for(int i=0;i<SPECTRUM_BINS;i++) {
            DrawPoint point={r.x+r.width*(i+.5f)/SPECTRUM_BINS,base-fmaxf(0,fminf(1,(s->db[i]+78)/78))*r.height};
            if(point.y<base || previous.y<base)host_draw_line_ex(previous,point,1.5f,edge);
            previous=point;
        }
        return;
    }
    for(int i=0;i<SPECTRUM_BINS;i++) {
        float height=fmaxf(0,fminf(1,(s->db[i]+78)/78))*r.height;
        DrawColor color=colored?host_color_from_h_s_v(260-240.f*i/(SPECTRUM_BINS-1),.75f,1):accent;
        host_draw_rectangle_rec((DrawRect){r.x+r.width*i/SPECTRUM_BINS,r.y+r.height-height,r.width/SPECTRUM_BINS+.1f,height},color);
    }
}

float eq_x(float frequency,DrawRect r) { return r.x+logf(frequency/20)/logf(1000)*r.width; }

float eq_y(float gain,DrawRect r) { return r.y+r.height*(.5f-gain/36); }

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
    label(fit_text(host_text_format("%s · Slot %d",ui.eq_bus?ui.project.insert_names[ui.eq_bus-1]:"Master",ui.eq_slot+1),width-254,12),12,30,12,muted);
    if(button("Bypass",width-226,26,70,24,ui.project.effect_bypass[ui.eq_bus][ui.eq_slot])) ui.project.effect_bypass[ui.eq_bus][ui.eq_slot]^=1;
    if(button("Load",width-152,26,66,24,0)) preset_action(PRESET_EQ,ui.eq_bus,ui.eq_slot,0);
    if(button("Save",width-82,26,70,24,0)) preset_action(PRESET_EQ,ui.eq_bus,ui.eq_slot,1);
    DrawRect graph={42,64,width-58,height-196};
    if(ui.studio_style)ui_graph_paper(host_draw(),graph);
    else host_draw_rectangle_rec(graph,bg);
    if(!ui.windows.workspace.enabled || ui.eq_spectrum_bus==ui.eq_bus)spectrum_draw(&ui.eq_spectrum,graph,1);
    const float frequencies[]={20,100,1000,10000,20000};
    const char *names[]={"20","100","1k","10k","20k"};
    for(int i=0;i<5;i++) {
        float x=eq_x(frequencies[i],graph);
        host_draw_line_ex((DrawPoint){x,graph.y},(DrawPoint){x,graph.y+graph.height},1,ui_theme.grid_minor);
        label(names[i],fminf(width-32,fmaxf(32,x-text_width(names[i],10)/2)),graph.y+graph.height+5,10,muted);
    }
    for(int db=-18;db<=18;db+=6) {
        float y=eq_y(db,graph);
        host_draw_line_ex((DrawPoint){graph.x,y},(DrawPoint){graph.x+graph.width,y},1,db?ui_theme.grid_minor:muted);
        label(host_text_format("%+d",db),8,y-5,10,muted);
    }
    static EQSettings drawn; static float response[257]; static int cached;
    if(!cached || memcmp(&drawn,&shown,sizeof drawn)) {
        for(int i=0;i<=256;i++) response[i]=equalizer_response(shown,spectrum_frequency(i/256.f));
        drawn=shown; cached=1;
    }
    DrawPoint last={0};
    for(int x=0;x<=256;x++) {
        float db=response[x];
        DrawPoint next={graph.x+graph.width*x/256,eq_y(fmaxf(-18,fminf(18,db)),graph)};
        if(x) {
            smooth_line(last,next,4,BLACK);
            smooth_line(last,next,1.8f,ui.studio_style?ui_skin.graph_line:ink);
        }
        last=next;
    }
    for(int i=0;i<EQ_BANDS;i++) {
        EQBand b=shown.bands[i]; DrawPoint p={eq_x(b.frequency,graph),eq_y(b.shape==EQ_LOW_CUT || b.shape==EQ_HIGH_CUT?0:b.gain,graph)};
        int over=hover(p.x-10,p.y-10,20,20);
        circle(p.x,p.y,ui.eq_band==i?8:6,b.shape==EQ_OFF?muted:accent);
        label(host_text_format("%d",i+1),p.x-text_width(host_text_format("%d",i+1),10)/2,p.y-(ui.studio_style?UI_TEXT_SIZE/2:5),10,ui_theme.selected_text);
        if(over) {
            snprintf(ui.status,sizeof ui.status,"EQ band %d: %.0f Hz, %+.1f dB | drag to tune; wheel changes Q; right-click to reset",i+1,b.frequency,b.gain);
            if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) { ui.eq_band=i; ui.eq_drag=i;ui.eq_drag_window=ui.knob_context; }
            float wheel=host_get_mouse_wheel_move();
            if(wheel) settings->bands[i].q=fmaxf(.2f,fminf(10,settings->bands[i].q+wheel*.1f));
            if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) {
                ui.eq_band=i; open_context(16,i,(DrawPoint){ui.mouse.x+desktop_editor_rect(ui.knob_context).x,ui.mouse.y+desktop_editor_rect(ui.knob_context).y}); ui.input_enabled=0;
            }
        }
    }
    if(ui.eq_drag>=0 && (!ui.windows.workspace.enabled || ui.eq_drag_window==ui.knob_context)) {
        if(host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT)) {
            EQBand *b=&settings->bands[ui.eq_drag];
            b->frequency=spectrum_frequency(fmaxf(0,fminf(1,(ui.mouse.x-graph.x)/graph.width)));
            if(b->shape!=EQ_LOW_CUT && b->shape!=EQ_HIGH_CUT) b->gain=fmaxf(-18,fminf(18,(.5f-(ui.mouse.y-graph.y)/graph.height)*36));
        } else ui.eq_drag=-1;
    }
    label(ui.windows.workspace.enabled && ui.eq_spectrum_bus!=ui.eq_bus?"Spectrum follows selected EQ":"Spectrum: -78 to 0 dBFS",graph.x,graph.y+graph.height+22,10,muted);
    float y=height-55;
    for(int i=0;i<EQ_BANDS;i++) if(button(host_text_format("%d",i+1),12+i*28,y-34,24,22,ui.eq_band==i)) ui.eq_band=i;
    EQBand *b=&settings->bands[ui.eq_band];
    if(choice_button(equalizer_shape_name(b->shape),216,y-34,152,22)) open_context(14,ui.eq_band,(DrawPoint){ui.mouse.x+desktop_editor_rect(ui.knob_context).x,ui.mouse.y+desktop_editor_rect(ui.knob_context).y});
    if(hover(216,y-34,152,22)) snprintf(ui.status,sizeof ui.status,"Band shape: Bell, shelves, cuts or Off");
    knob_style(92,y+4,&b->frequency,20,20000,equalizer_default().bands[ui.eq_band].frequency,"EQ frequency (Hz)",KNOB_LOGARITHMIC);
    int cut=b->shape==EQ_LOW_CUT || b->shape==EQ_HIGH_CUT;
    if(cut) label("12 dB/oct",174,y,11,muted);
    else knob_style(200,y+4,&b->gain,-18,18,0,"EQ gain (dB)",KNOB_CENTER);
    knob(310,y+4,&b->q,.2f,10,equalizer_default().bands[ui.eq_band].q,"EQ Q (bandwidth)");
    label(host_text_format("%.0f Hz",b->frequency),66,y+24,11,muted);
    label(cut?"Slope":host_text_format("%+.1f dB",b->gain),178,y+24,11,muted);
    label(host_text_format("Q %.2f",b->q),288,y+24,11,muted);
    knob(width-44,y+4,&ui.project.effect_mix[ui.eq_bus][ui.eq_slot],0,1,1,"Equalizer mix");
    label("Mix",width-54,y+24,11,muted);
}

float meter_height(float level) { return level>.001f?fader_position(level):0; }

void meter_draw(int id,int x,int top,int bottom,int bar_width) {
    int height=bottom-top,spacing=bar_width+2;
    float orange=meter_height(powf(10,-12.f/20)),zero=meter_height(1);
    host_draw_rectangle(x-1,top,spacing*2,height,ui.studio_style?ui_skin.display:bg);
    if(ui.studio_style) {
        ui_bevel(host_draw(),(DrawRect){x-3,top-2,spacing*2+4,height+4},1);
        const int scale[]={-48,-24,-12,-6,0};
        for(unsigned i=0;i<sizeof scale/sizeof *scale;i++) {
            float y=bottom-meter_height(powf(10,scale[i]/20.f))*height;
            host_draw_line(x, y,x+spacing*2-2,y,(DrawColor){53,76,57,255});
        }
    }
    for(int side=0;side<2;side++) {
        int left=x+side*spacing; float level=meter_height(ui.meter_level[id][side]),peak=meter_height(ui.meter_hold[id][side]);
        if(ui.studio_style)ui_level_fill(host_draw(),(DrawRect){left,top,bar_width,height},level,orange,zero,1);
        else {
            if(level>0) host_draw_rectangle(left,bottom-level*height,bar_width,level*height,ui_theme.meter_low);
            if(level>orange) host_draw_rectangle(left,bottom-level*height,bar_width,(level-orange)*height,ui_theme.meter_mid);
            if(level>zero) host_draw_rectangle(left,bottom-level*height,bar_width,(level-zero)*height,ui_theme.meter_high);
        }
        if(peak>0) host_draw_rectangle(left-1,fmaxf(top,bottom-peak*height),bar_width+2,1,ui.meter_hold[id][side]>1?ui_theme.meter_high:ui.studio_style?(DrawColor){222,232,181,255}:ink);
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
        const char *number=host_text_format("%d",ticks[i]);
        label(number,16-text_width(number,10)/2,fmaxf(top,fminf(bottom-10,y-5)),10,muted);
        host_draw_line(29,y,31,y,muted);
    }
    float peak=fmaxf(ui.meter_hold[ui.mixer_selected][0],ui.meter_hold[ui.mixer_selected][1]);
    label(peak?host_text_format("%.1f",gain_db(peak)):"-inf",8,height-29,11,peak>1?ui_theme.meter_high:ink);
    if(hover(4,TITLE,54,height-TITLE-18)) snprintf(ui.status,sizeof ui.status,"Selected %s | Peak L %.1f dBFS, R %.1f dBFS",mixer_name(ui.mixer_selected),gain_db(ui.meter_hold[ui.mixer_selected][0]),gain_db(ui.meter_hold[ui.mixer_selected][1]));
}

int mixer_effect_window(int bus,int slot) {
    for(int id=5;id<WINDOW_CAPACITY;id++)if(ui.windows.editors[id].visible && ui.effect_windows[id].bus==bus && ui.effect_windows[id].slot==slot)return id;
    return -1;
}
void mixer_effect_open(int bus,int slot,int toggle) {
    int type=ui.project.effect_type[bus][slot];if(!type)return;
    ui.eq_bus=bus;ui.eq_slot=slot;ui.effect_slot=slot;
    if(!ui.windows.workspace.enabled){windows_focus(&ui.windows,5);return;}
    int id=mixer_effect_window(bus,slot);
    if(id>=5){if(toggle)studio_workspace_hide(&ui.windows,id);else windows_focus(&ui.windows,id);return;}
    for(id=5;id<WINDOW_CAPACITY && ui.windows.editors[id].visible;id++){}
    if(id==WINDOW_CAPACITY){snprintf(ui.status,sizeof ui.status,"Close an effect editor before opening another (16 open).");return;}
    StudioWorkspace *s=&ui.windows.workspace;float width=type==EFFECT_CHORUS?360:620,height=type==EFFECT_CHORUS?280:380;
    width=fminf(width,s->width-s->sidebar-10);height=fminf(height,s->height-84);
    int count=0;for(int i=5;i<WINDOW_CAPACITY;i++)count+=ui.windows.editors[i].visible;
    ui.effect_windows[id].bus=bus;ui.effect_windows[id].slot=slot;ui.effect_windows[id].band=0;
    ui.windows.editors[id].minw=type==EFFECT_CHORUS?330:480;ui.windows.editors[id].minh=type==EFFECT_CHORUS?280:330;
    ui.windows.editors[id].rect=(Rect){fmaxf(s->sidebar+6,s->width-width-12-count*20),fminf(70+count*22,s->height-24-height),width,height};
    ui.windows.workspace.floating[id]=1;ui.windows.editors[id].maximized=0;
    windows_focus(&ui.windows,id);
}
void mixer_effect_add(int bus,int slot,int type) {
    if(bus<0 || bus>ui.project.insert_count || slot<0 || slot>=EFFECT_SLOTS || (type!=EFFECT_CHORUS && type!=EFFECT_EQ))return;
    ui.project.effect_type[bus][slot]=type;ui.project.effect_bypass[bus][slot]=0;
    if(type==EFFECT_CHORUS){ui.project.chorus[bus][slot]=chorus_default();ui.project.effect_mix[bus][slot]=.5f;}
    else{ui.project.eq[bus][slot]=equalizer_default();ui.project.effect_mix[bus][slot]=1;}
    mixer_effect_open(bus,slot,0);
}
void mixer_effect_remove(int bus,int slot) {
    for(int id=5;id<WINDOW_CAPACITY;id++)if(ui.windows.workspace.enabled && ui.effect_windows[id].bus==bus && ui.effect_windows[id].slot==slot)studio_workspace_hide(&ui.windows,id);
    ui.project.effect_type[bus][slot]=EFFECT_EMPTY;
    for(int a=ui.project.automation_count-1;a>=0;a--){ParameterTarget t=ui.project.automations[a].target;
        if(((t.parameter>=PARAM_CHORUS_RATE && t.parameter<=PARAM_EFFECT_MIX) || (t.parameter>=PARAM_EQ_FIRST && t.parameter<=PARAM_EQ_LAST)) && t.owner==(unsigned)bus && t.slot==(unsigned)slot)automation_delete(&ui.project,a);}
    ui.control_drag=NULL;ui.automation_selected=ui.automation_node=-1;ui.picker_drag=-1;ui.eq_drag=-1;
}
void effect_slot_menu(int bus,int slot) {
    ui.eq_bus=bus;ui.eq_slot=slot;ui.effect_slot=slot;
    open_context(25,slot,(DrawPoint){ui.mouse.x+desktop_editor_rect(3).x,ui.mouse.y+desktop_editor_rect(3).y});
}
void effect_editor(float width,float height) {
    int bus=ui.eq_bus,slot=ui.eq_slot;
    if(bus<0 || bus>ui.project.insert_count || slot<0 || slot>=EFFECT_SLOTS)return;
    if(ui.windows.workspace.enabled)ui.windows.editors[ui.knob_context].minw=ui.project.effect_type[bus][slot]==EFFECT_CHORUS?330:480;
    if(!ui.project.effect_type[bus][slot]){label("Select an effect slot in the Mixer.",16,40,13,muted);return;}
    if(ui.project.effect_type[bus][slot]!=EFFECT_CHORUS){eq_editor(width,height);return;}
    ChorusSettings *s=&ui.project.chorus[bus][slot];
    label(fit_text(host_text_format("%s / Slot %d",mixer_name(bus),slot+1),width-24,13),12,32,13,ink);
    float col=(width-32)/3;
    knob(20+col*.5f,100,&s->rate,.05f,5,.513f,"Chorus rate (Hz)");
    knob(20+col*1.5f,100,&s->depth,0,8,1.85f,"Chorus depth (ms)");
    knob(20+col*2.5f,100,&ui.project.effect_mix[bus][slot],0,1,.5f,"Chorus mix");
    const char *names[]={"Rate","Depth","Mix"};for(int i=0;i<3;i++)label(names[i],20+col*(i+.5f)-16,125,12,muted);
    label(host_text_format("%.2f Hz",displayed_value(&s->rate,s->rate)),20,153,12,muted);
    label(host_text_format("%.2f ms",displayed_value(&s->depth,s->depth)),20+col,153,12,muted);
    if(button("Bypass",12,190,width-24,26,ui.project.effect_bypass[bus][slot]))ui.project.effect_bypass[bus][slot]^=1;
    if(button("Load",12,228,(width-28)/2,26,0))preset_action(PRESET_CHORUS,bus,slot,0);
    if(button("Save",16+(width-28)/2,228,(width-28)/2,26,0))preset_action(PRESET_CHORUS,bus,slot,1);
}
static void mixer_slots(float fx,float height) {
    int bus=ui.mixer_selected;device_button(0,fx+8,TITLE+4,MIXER_PANEL-16);
    float top=TITLE+26,row=22;
    int visible=fmaxf(1,fminf(EFFECT_SLOTS,(height-top-4)/row));
    if(hover(fx,top,MIXER_PANEL,height-top))ui.effect_list_scroll-=host_get_mouse_wheel_move();
    ui.effect_list_scroll=fmaxf(0,fminf(EFFECT_SLOTS-visible,ui.effect_list_scroll));
    for(int i=ui.effect_list_scroll;i<EFFECT_SLOTS && i<ui.effect_list_scroll+visible;i++) {
        int type=ui.project.effect_type[bus][i];float y=top+(i-ui.effect_list_scroll)*row;
        const char *name=type?effect_descriptor(type)->name:"Empty";
        if(button(host_text_format("%02d  %s",i+1,name),fx+8,y,MIXER_PANEL-36,row-1,mixer_effect_window(bus,i)>=5)) {
            if(type)mixer_effect_open(bus,i,1);else effect_slot_menu(bus,i);
        }
        if(type && button(ui.project.effect_bypass[bus][i]?"-":"o",fx+MIXER_PANEL-25,y,18,row-1,!ui.project.effect_bypass[bus][i]))ui.project.effect_bypass[bus][i]^=1;
        if(hover(fx+8,y,MIXER_PANEL-16,row-1)) {
            snprintf(ui.status,sizeof ui.status,"Slot %d: %s | click to %s; right-click for slot options",i+1,name,type?"open":"add an effect");
            if(type && ui.mouse.x>=fx+MIXER_PANEL-25)snprintf(ui.status,sizeof ui.status,"Slot %d: %s | click the indicator to bypass / enable",i+1,name);
            if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT))effect_slot_menu(bus,i);
        }
    }
}

static void mixer_legacy(float width,float height) {
    selected_meter(height);
    int capacity=fmaxf(1,(width-MIXER_PANEL-MIXER_LEFT-8)/mixer_strip_width()-1),visible=ui.project.insert_count<capacity?ui.project.insert_count:capacity;
    ui.mixer_scroll=(int)fmaxf(0,fminf(fmaxf(0,ui.project.insert_count-visible),ui.mixer_scroll));
    if(hover(0,TITLE,width-220,height-TITLE) && ui.mouse.y>TITLE+60) {
        ui.mixer_scroll=(int)fmaxf(0,fminf(fmaxf(0,ui.project.insert_count-visible),ui.mixer_scroll-host_get_mouse_wheel_move()));
    }
    int stride=mixer_strip_width(),stripw=ui.studio_style?stride-4:48;
    float fader_top=ui.studio_style?TITLE+48:TITLE+76,fader_bottom=height-(ui.studio_style?54:74);
    float route_y=mixer_route_y(height),control_y=ui.studio_style?50:TITLE+52;
    float width_y=ui.studio_style?50:height-59;
    float width_x=ui.studio_style?65:33,arm_x=ui.studio_style?91:12;
    static const DrawColor strip_colors[]={
        {0,216,200,255},{248,219,62,255},{100,201,239,255},{245,215,65,255},
        {226,110,219,255},{105,181,243,255},{92,225,132,255},{238,148,77,255}};
    for(int col=0;col<=visible;col++) {
        int id=col?col+ui.mixer_scroll:0,x=MIXER_LEFT+col*mixer_strip_width();
        float *v=id?&ui.project.insert_volume[id-1]:&ui.project.master;
        int over=hover(x,TITLE+4,stripw,height-TITLE-12);
        DrawColor strip=ui.mixer_selected==id?(ui.studio_style?(DrawColor){150,160,169,255}:ui_theme.title_focus):cell;
        if(ui.studio_style)ui_panel(host_draw(),(DrawRect){x,TITLE+4,stripw,height-TITLE-12},strip);
        else ui_surface(host_draw(),(DrawRect){x,TITLE+4,stripw,height-TITLE-12},strip);
        if(over) snprintf(ui.status,sizeof ui.status,"%s: click to select this mixer channel",mixer_name(id));
        if(over && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) ui.mixer_selected=id;
        host_draw_rectangle(x+3,TITLE+7,ui.studio_style?stripw-6:42,17,
            ui.studio_style?(id?strip_colors[(id-1)%8]:ui_theme.control):ui.mixer_selected==id?ui_theme.title_focus:cell);
        if(ui.studio_style){
            ui_bevel(host_draw(),(DrawRect){x,TITLE+4,stripw,height-TITLE-12},0);
            ui_bevel(host_draw(),(DrawRect){x+3,TITLE+7,stripw-6,20},1);
            if(ui.mixer_selected==id)host_draw_line(x+3,TITLE+26,x+stripw-4,TITLE+26,accent);
            host_draw_line(x+4,control_y+13,x+stripw-5,control_y+13,(DrawColor){126,135,120,255});
            host_draw_line(x+4,route_y-3,x+stripw-5,route_y-3,(DrawColor){126,135,120,255});
        }
        label(id?host_text_format("%d",id):"Master",x+5,TITLE+9,id?12:10,ink);
        if(id) {
            label(fit_text(ui.project.insert_names[id-1],ui.studio_style?stripw-28:42,10),x+(ui.studio_style?24:3),ui.studio_style?TITLE+9:TITLE+26,10,muted);
            if(hover(x+(ui.studio_style?24:3),ui.studio_style?TITLE+7:TITLE+25,ui.studio_style?stripw-28:42,16)) {
                snprintf(ui.status,sizeof ui.status,"%s: right-click to rename",ui.project.insert_names[id-1]);
                if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) {
                    ui.rename_channel=ui.rename_track=-1; ui.rename_mixer=id-1; ui.mixer_selected=id;
                    snprintf(ui.rename_text,sizeof ui.rename_text,"%s",ui.project.insert_names[id-1]); ui.rename_select_all=1; open_popup(2); ui.input_enabled=0;
                }
            }
        }
        int input_over=hover(x+7,route_y,12,12) || (ui.cable_drag>0 && host_check_collision_point_rec(ui.mouse,(DrawRect){x+7,route_y,12,12}));
        host_draw_rectangle_lines(x+9,route_y+2,8,8,input_over?ui_theme.signal:muted);

        if(input_over) snprintf(ui.status,sizeof ui.status,"%s input: drop an insert output cable here",mixer_name(id));
        if(over && ui.mouse.y<TITLE+24 && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) { ui.mixer_selected=id; open_context(3,id,(DrawPoint){ui.mouse.x+desktop_editor_rect(3).x,ui.mouse.y+desktop_editor_rect(3).y}); }
        if(over && ui.mouse.y<TITLE+24) snprintf(ui.status,sizeof ui.status,"%s: right-click for mixer channel actions",mixer_name(id));
        mute_light(x+11,control_y,id?ui.project.insert_mute:&ui.project.master_mute,id?ui.project.insert_count:0,id?id-1:0,mixer_name(id));
        if(id) knob_style(x+35,control_y,&ui.project.insert_pan[id-1],-1,1,0,"Insert pan",KNOB_PAN);
        meter_draw(id,x+(ui.studio_style?65:8),fader_top,fader_bottom,ui.studio_style?6:3);
        if(ui.studio_style) {
            const int ticks[]={0,-6,-12,-24,-48};
            float last_label=-1000;
            for(unsigned t=0;t<sizeof ticks/sizeof *ticks;t++) {
                float y=fader_bottom-meter_height(powf(10,ticks[t]/20.f))*(fader_bottom-fader_top);
                host_draw_line(x+82,y,x+85,y,muted);
                if(y-last_label>=(ui.studio_style?UI_TEXT_SIZE+2:11)){label(host_text_format("%d",ticks[t]),x+86,y-5,10,muted);last_label=y;}
            }

        }
        if(ui.studio_style){
            ui_surface(host_draw(),(DrawRect){x+30,fader_top-2,9,fader_bottom-fader_top+4},(DrawColor){63,72,81,255});
            ui_bevel(host_draw(),(DrawRect){x+30,fader_top-2,9,fader_bottom-fader_top+4},1);
            for(int tick=0;tick<=8;tick++) {
                float y=fader_top+(fader_bottom-fader_top)*tick/8;
                host_draw_line(x+(tick%2?24:20),y,x+28,y,muted);
            }
            ui_surface(host_draw(),(DrawRect){x+20,height-51,46,18},ui_skin.display);
            ui_bevel(host_draw(),(DrawRect){x+20,height-51,46,18},1);
            label(host_text_format("%.1f",gain_db(*v)),x+24,height-49,10,ui_theme.signal);
        }
        host_draw_rectangle(x+33,fader_top,3,fader_bottom-fader_top,bg);
        if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) control_menu(v,0,MIXER_GAIN_MAX,1,id?"Insert volume":"Master volume");
        if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) {
            capture_control(v,0,MIXER_GAIN_MAX,1); ui.control_range=fader_bottom-fader_top;
            /* Anchor the drag to the current handle position rather than the click position. */
            ui.control_bottom=ui.mouse.y+desktop_editor_rect(3).y+fader_position(*v)*ui.control_range;
        }
        if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) || ui.control_drag==v) host_set_mouse_cursor(HOST_MOUSE_CURSOR_RESIZE_NS);
        if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) || ui.control_drag==v) snprintf(ui.status,sizeof ui.status,"%s fader: %.1f dB | Mark = 0 dB; right-click for value / automation",mixer_name(id),gain_db(*v));
        host_draw_line(x+25,fader_bottom-fader_position(1)*(fader_bottom-fader_top),x+44,fader_bottom-fader_position(1)*(fader_bottom-fader_top),muted);
        int fy=fader_bottom-fader_position(displayed_value(v,*v))*(fader_bottom-fader_top);
        ui_fader_handle(host_draw(),(DrawRect){x+(ui.studio_style?22:26),fy-9,ui.studio_style?22:14,18},ui.mixer_selected==id);
        if(ui.studio_style && !ui.profile_flat)host_draw_rectangle_lines(x+22,fy-9,22,18,ui_theme.border);
        knob_style(x+width_x,width_y,id?&ui.project.insert_width[id-1]:&ui.project.master_width,0,2,1,"Stereo width (left wider, center unchanged, right mono)",KNOB_WIDTH);
        int arm_over=hover(x+arm_x-8,width_y-8,16,16);
        if(ui.studio_style)ui_check_face(host_draw(),(DrawRect){x+arm_x-8,width_y-8,16,16},ui.record_armed[id],ui_theme.meter_high);
        else {
            circle(x+arm_x,width_y,6,arm_over || ui.record_armed[id]?ui_theme.meter_high:ui_theme.border);
            circle(x+arm_x,width_y,5,ui.record_armed[id]?ui_theme.meter_high:cell);
        }
        if(arm_over) {
            snprintf(ui.status,sizeof ui.status,"%s record arm: %s | Top-bar Record captures this track's post-fader audio",mixer_name(id),ui.record_armed[id]?"On":"Off");
            if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) && !recording_active()) { ui.record_armed[id]^=1; ui.input_enabled=0; }
        }
        if(id) {

            int output_over=hover(x+29,route_y,12,12);
            circle(x+35,route_y+6,5,output_over || ui.cable_drag==id?accent:muted);
            circle(x+35,route_y+6,2,ui.project.insert_output[id-1]==255?bg:accent);
            if(output_over) {
                snprintf(ui.status,sizeof ui.status,"Insert %d output: drag to an input; right-click to unplug",id);
                if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) insert_connect(&ui.project,id,255);
                if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) { ui.cable_drag=id; ui.mixer_selected=id; ui.input_enabled=0; }
            }
        }
    }
    if(ui.mixer_selected>0) {
        int col=ui.mixer_selected-ui.mixer_scroll,destination=ui.project.insert_output[ui.mixer_selected-1];
        int target=destination?destination-ui.mixer_scroll:0;
        if(col>=1 && col<=visible && (ui.cable_drag==ui.mixer_selected || (destination!=255 && target>=0 && target<=visible && (!destination || target>=1)))) {
            DrawPoint start={MIXER_LEFT+col*mixer_strip_width()+35,route_y+6},end=ui.cable_drag==ui.mixer_selected?ui.mouse:(DrawPoint){MIXER_LEFT+target*mixer_strip_width()+13,route_y+6};
            DrawPoint points[]={start,{start.x,height-21},{end.x,height-21},end};
            cable(points,host_fade(ui_theme.signal,.8f));
        }
    }
    int fx=width-MIXER_PANEL;
    host_draw_rectangle(fx,TITLE,MIXER_PANEL,height-TITLE,ui_theme.mixer);
    if(ui.windows.workspace.enabled)mixer_slots(fx,height);else {
    int slot_y=ui.studio_style?TITLE+28:TITLE+90,slot_pitch=ui.studio_style?22:24;
    int effect_y=ui.studio_style?TITLE+74:TITLE+148;
    int effect_button_y=ui.studio_style?TITLE+96:TITLE+177;
    int effect_knob_y=ui.studio_style?TITLE+100:TITLE+183;
    int effect_label_y=ui.studio_style?TITLE+120:TITLE+203;
    int bypass_y=ui.studio_style?TITLE+140:TITLE+225,preset_y=ui.studio_style?TITLE+164:TITLE+253;
    int small_h=ui.studio_style?20:24;
    if(!ui.studio_style)label("Recording input",fx+8,TITLE+8,11,muted);
    device_button(0,fx+8,ui.studio_style?TITLE+4:TITLE+26,MIXER_PANEL-16);
    if(!ui.studio_style)label("Effects",fx+8,TITLE+70,12,ink);
    for(int slot=0;slot<EFFECT_SLOTS;slot++) {
        int x=fx+8+(slot%5)*29,y=slot_y+(slot/5)*slot_pitch;
        if(button(host_text_format("%d%s",slot+1,ui.project.effect_type[ui.mixer_selected][slot]?"·":""),x,y,27,ui.studio_style?20:22,ui.effect_slot==slot)) ui.effect_slot=slot;
        if(hover(x,y,27,ui.studio_style?20:22)) snprintf(ui.status,sizeof ui.status,"Effect slot %d: %s",slot+1,ui.project.effect_type[ui.mixer_selected][slot]?effect_descriptor(ui.project.effect_type[ui.mixer_selected][slot])->name:"Empty");
    }
    int bus=ui.mixer_selected,slot=ui.effect_slot;
    if(ui.project.effect_type[bus][slot]==EFFECT_EMPTY) {
        if(button("+ Chorus",fx+8,effect_y,MIXER_PANEL-16,ui.studio_style?20:25,0)) {
            ui.project.effect_type[bus][slot]=EFFECT_CHORUS; effect_descriptor(EFFECT_CHORUS)->defaults(&ui.project.chorus[bus][slot]);
            ui.project.effect_mix[bus][slot]=.5f; ui.project.effect_bypass[bus][slot]=0;
        }
        if(button("+ Equalizer",fx+8,effect_button_y,MIXER_PANEL-16,ui.studio_style?20:25,0)) {
            ui.project.effect_type[bus][slot]=EFFECT_EQ; effect_descriptor(EFFECT_EQ)->defaults(&ui.project.eq[bus][slot]); ui.project.effect_mix[bus][slot]=1; ui.project.effect_bypass[bus][slot]=0;
            ui.eq_bus=bus; ui.eq_slot=slot; ui.eq_drag=-1; windows_focus(&ui.windows,5);
        }
    } else {
        label(effect_descriptor(ui.project.effect_type[bus][slot])->name,fx+8,effect_y,12,ink);
        if(button("x",fx+MIXER_PANEL-28,ui.studio_style?effect_y-2:TITLE+144,20,ui.studio_style?20:22,0)) {
            ui.project.effect_type[bus][slot]=EFFECT_EMPTY;
            for(int a=ui.project.automation_count-1;a>=0;a--) {
                ParameterTarget t=ui.project.automations[a].target;
                if(((t.parameter>=PARAM_CHORUS_RATE && t.parameter<=PARAM_EFFECT_MIX) || (t.parameter>=PARAM_EQ_FIRST && t.parameter<=PARAM_EQ_LAST)) && t.owner==(unsigned)bus && t.slot==(unsigned)slot) automation_delete(&ui.project,a);
            }
            ui.control_drag=NULL; ui.automation_selected=ui.automation_node=-1; ui.picker_drag=-1;
        } else if(ui.project.effect_type[bus][slot]==EFFECT_EQ) {
            if(button("Open Equalizer",fx+8,effect_button_y,MIXER_PANEL-16,ui.studio_style?20:25,0)) { ui.eq_bus=bus; ui.eq_slot=slot; ui.eq_drag=-1; windows_focus(&ui.windows,5); }
            if(button("Bypass",fx+8,bypass_y,MIXER_PANEL-16,small_h,ui.project.effect_bypass[bus][slot])) ui.project.effect_bypass[bus][slot]^=1;
            if(button("Load",fx+8,preset_y,70,small_h,0)) preset_action(PRESET_EQ,bus,slot,0);
            if(button("Save",fx+82,preset_y,70,small_h,0)) preset_action(PRESET_EQ,bus,slot,1);
        } else {
            ChorusSettings *settings=&ui.project.chorus[bus][slot];
            knob(fx+27,effect_knob_y,&settings->rate,.05f,5,.513f,"Chorus rate (Hz)");
            knob(fx+78,effect_knob_y,&settings->depth,0,8,1.85f,"Chorus depth (ms)");
            knob(fx+129,effect_knob_y,&ui.project.effect_mix[bus][slot],0,1,.5f,"Chorus mix");
            label("Rate",fx+15,effect_label_y,10,muted); label("Depth",fx+64,effect_label_y,10,muted); label("Mix",fx+120,effect_label_y,10,muted);
            if(button("Bypass",fx+8,bypass_y,MIXER_PANEL-16,small_h,ui.project.effect_bypass[bus][slot])) ui.project.effect_bypass[bus][slot]^=1;
            if(button("Load",fx+8,preset_y,70,small_h,0)) preset_action(PRESET_CHORUS,bus,slot,0);
            if(button("Save",fx+82,preset_y,70,small_h,0)) preset_action(PRESET_CHORUS,bus,slot,1);
        }
    }
    }
    float area=fx-MIXER_LEFT-8,thumb=area*visible/INSERTS,pos=MIXER_LEFT+area*ui.mixer_scroll/INSERTS;
    if(ui.studio_style)ui_scrollbar(host_draw(),(DrawRect){MIXER_LEFT,height-17,area,9},(DrawRect){pos,height-17,fmaxf(12,thumb),9},ui.mixer_pan || hover(MIXER_LEFT,height-20,area,14));
    else { host_draw_rectangle(MIXER_LEFT,height-17,area,9,bg); ui_surface(host_draw(),(DrawRect){pos,height-17,fmaxf(12,thumb),9},ui.mixer_pan || hover(MIXER_LEFT,height-20,area,14)?accent:muted); }
    if(hover(MIXER_LEFT,height-20,area,14)) {
        snprintf(ui.status,sizeof ui.status,"Mixer: wheel over inserts or drag this scrollbar to browse all 100 inserts");
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) { ui.mixer_pan=1; ui.mixer_scroll=fmaxf(0,fminf(INSERTS-visible,(ui.mouse.x-MIXER_LEFT-thumb/2)/area*INSERTS)); ui.input_enabled=0; }
    }
}

/* The studio mixer keeps Master fixed and translates insert-local controls
   together. Input, text and drawing share that translation, including partial
   strips, while the engine and the frozen legacy renderer remain unchanged. */
static void studio_fx_slots(int counts[INSERTS+1]) {
    for(int bus=0;bus<=INSERTS;bus++)counts[bus]=ui.project.effect_type[bus][EFFECT_SLOTS-1]?EFFECT_SLOTS:STUDIO_FX_SLOTS;
    for(int i=0;i<ui.project.automation_count+ui.project.midi_binding_count;i++) {
        ParameterTarget t=i<ui.project.automation_count?ui.project.automations[i].target:ui.project.midi_bindings[i-ui.project.automation_count].target;
        if(t.owner<=INSERTS && t.slot==EFFECT_SLOTS-1 && ((t.parameter>=PARAM_CHORUS_RATE && t.parameter<=PARAM_EFFECT_MIX) || (t.parameter>=PARAM_EQ_FIRST && t.parameter<=PARAM_EQ_LAST)))counts[t.owner]=EFFECT_SLOTS;
    }
}
void desktop_mixer_scrollbar_update(void) {
    Rect r=desktop_editor_rect(3);MixerView v=mixer_view(r.w,r.h,ui.project.insert_count,ui.mixer_offset,ui.mixer_fx_hidden);
    ui.mixer_offset=mixer_scroll_from_thumb(v,ui.mouse.x-r.x,ui.mixer_scroll_grab);
    if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT))ui.mixer_pan=0;
}
static void studio_mixer_scale(MixerView v) {
    label("dBFS",3,v.fader_top-20,UI_TEXT_SIZE,muted);
    const int ticks[]={6,0,-6,-12,-24,-36,-48,-60};float last=-1000;
    for(unsigned i=0;i<sizeof ticks/sizeof *ticks;i++) {
        float y=v.fader_bottom-meter_height(powf(10,ticks[i]/20.f))*(v.fader_bottom-v.fader_top);
        host_draw_line(36,y,41,y,muted);
        if(y-last>=UI_TEXT_SIZE+2){label(host_text_format("%d",ticks[i]),3,y-8,UI_TEXT_SIZE,muted);last=y;}
    }
}
static void studio_mixer_strip(MixerView v,int bus,int slots) {
    int over=hover(0,v.fx_top,MIXER_STRIDE-4,v.height-v.fx_top-18);
    if(over && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT))ui.mixer_selected=bus;
    ui_panel(host_draw(),(DrawRect){0,v.fx_top,MIXER_STRIDE-4,v.height-v.fx_top-18},ui.mixer_selected==bus?(DrawColor){150,160,169,255}:cell);
    static const DrawColor colors[]={
        {0,216,200,255},{248,219,62,255},{100,201,239,255},{245,215,65,255},
        {226,110,219,255},{105,181,243,255},{92,225,132,255},{238,148,77,255}};
    if(v.fx_rows) {
        int body_input=ui.input_enabled;
        ui.input_enabled=body_input && ui.mouse.y>=v.fx_top && ui.mouse.y<v.fx_top+v.fx_height;
        int fx_input=ui.input_enabled,first=(int)(ui.mixer_fx_scroll/22);
        Rect root=desktop_editor_rect(3);float x=mixer_bus_x(v,bus),scale=ui_scale();
        float left=bus?fmaxf(v.insert_x,x):x,right=bus?fminf(v.insert_x+v.viewport,x+108):x+108;
        host_begin_scissor_mode((root.x+left)*scale,(root.y+v.fx_top)*scale,(right-left)*scale,v.fx_height*scale);
        for(int slot=first;slot<slots && slot<first+v.fx_rows+1;slot++) {
            float y=v.fx_top+slot*22-ui.mixer_fx_scroll;
            int type=ui.project.effect_type[bus][slot];
            const char *name=type?effect_descriptor(type)->name:"Empty";
            if(type==EFFECT_EQ)name="EQ";
            if(button(fit_text(name,type?74:98,UI_TEXT_SIZE),2,y, type?82:104,21,mixer_effect_window(bus,slot)>=5)) {
                if(type)mixer_effect_open(bus,slot,1);else effect_slot_menu(bus,slot);
                ui.input_enabled=0;
            }
            if(type && button(ui.project.effect_bypass[bus][slot]?"-":"o",86,y,18,21,!ui.project.effect_bypass[bus][slot]))ui.project.effect_bypass[bus][slot]^=1;
            if(hover(0,y,108,21)) {
                snprintf(ui.status,sizeof ui.status,"%s FX %d: %s | click to %s; right-click for slot options",mixer_name(bus),slot+1,name,type?"open / close":"add an effect");
                if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT))effect_slot_menu(bus,slot);
            }
        }
        ui.input_enabled=body_input && !(fx_input && !ui.input_enabled);
        host_begin_scissor_mode((root.x+left)*scale,(root.y+v.fx_top)*scale,(right-left)*scale,(v.height-v.fx_top-18)*scale);
    }
    ui_surface(host_draw(),(DrawRect){2,v.header,104,20},bus?colors[(bus-1)%8]:ui_theme.control);
    if(bus)label(host_text_format("%d",bus),5,v.header+2,UI_TEXT_SIZE,(DrawColor){12,22,26,255});
    label(fit_text(mixer_name(bus),bus?76:98,UI_TEXT_SIZE),bus?28:6,v.header+2,UI_TEXT_SIZE,(DrawColor){12,22,26,255});
    if(hover(0,v.header,108,20)) {
        snprintf(ui.status,sizeof ui.status,"%s: right-click name to rename; left edge for channel actions",mixer_name(bus));
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) {
            ui.mixer_selected=bus;
            if(bus && ui.mouse.x>=24) {
                ui.rename_channel=ui.rename_track=-1;ui.rename_mixer=bus-1;
                snprintf(ui.rename_text,sizeof ui.rename_text,"%s",ui.project.insert_names[bus-1]);ui.rename_select_all=1;open_popup(2);ui.input_enabled=0;
            } else open_context(3,bus,(DrawPoint){ui.mouse.x+desktop_editor_rect(3).x,ui.mouse.y+desktop_editor_rect(3).y});
        }
    }
    mute_solo_buttons(3,v.controls-10,bus?ui.project.insert_mute:&ui.project.master_mute,bus?ui.project.insert_count:0,bus?bus-1:0,mixer_name(bus));
    if(bus)knob_style(55,v.controls,&ui.project.insert_pan[bus-1],-1,1,0,"Insert pan",KNOB_PAN);
    knob_style(77,v.controls,bus?&ui.project.insert_width[bus-1]:&ui.project.master_width,0,2,1,"Stereo width (left wider, center unchanged, right mono)",KNOB_WIDTH);
    int arm=hover(91,v.controls-8,16,16);
    ui_check_face(host_draw(),(DrawRect){91,v.controls-8,16,16},ui.record_armed[bus],ui_theme.meter_high);
    if(arm){snprintf(ui.status,sizeof ui.status,"%s record arm: %s",mixer_name(bus),ui.record_armed[bus]?"On":"Off");if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) && !recording_active()){ui.record_armed[bus]^=1;ui.input_enabled=0;}}
    float *volume=bus?&ui.project.insert_volume[bus-1]:&ui.project.master;
    ui_surface(host_draw(),(DrawRect){30,v.fader_top-2,9,v.fader_bottom-v.fader_top+4},(DrawColor){63,72,81,255});
    ui_bevel(host_draw(),(DrawRect){30,v.fader_top-2,9,v.fader_bottom-v.fader_top+4},1);
    meter_draw(bus,65,v.fader_top,v.fader_bottom,6);
    for(int tick=0;tick<=8;tick++){float y=v.fader_top+(v.fader_bottom-v.fader_top)*tick/8;host_draw_line(tick%2?24:20,y,28,y,muted);}
    int fader_over=hover(23,v.fader_top-4,22,v.fader_bottom-v.fader_top+8);
    if(fader_over){
        host_set_mouse_cursor(HOST_MOUSE_CURSOR_RESIZE_NS);
        snprintf(ui.status,sizeof ui.status,"%s fader: %.1f dB | right-click for value / automation",mixer_name(bus),gain_db(*volume));
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT))control_menu(volume,0,MIXER_GAIN_MAX,1,bus?"Insert volume":"Master volume");
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)){capture_control(volume,0,MIXER_GAIN_MAX,1);ui.control_range=v.fader_bottom-v.fader_top;ui.control_bottom=ui.mouse.y+desktop_editor_rect(3).y+fader_position(*volume)*ui.control_range;}
    }
    float mark=v.fader_bottom-fader_position(1)*(v.fader_bottom-v.fader_top);
    host_draw_line(25,mark,44,mark,muted);
    float fy=v.fader_bottom-fader_position(displayed_value(volume,*volume))*(v.fader_bottom-v.fader_top);
    ui_fader_handle(host_draw(),(DrawRect){22,fy-9,22,18},ui.mixer_selected==bus);
    ui_text_well(host_draw(),(DrawRect){20,v.height-49,58,18},0);
    label(host_text_format("%.1f",gain_db(*volume)),24,v.height-48,UI_TEXT_SIZE,ui_theme.signal);
    int input=hover(7,v.route,12,12);
    host_draw_rectangle_lines(9,v.route+2,8,8,input?ui_theme.signal:muted);
    if(input)snprintf(ui.status,sizeof ui.status,"%s input: drop an insert output cable here",mixer_name(bus));
    if(bus){
        int output=hover(29,v.route,12,12);
        circle(35,v.route+6,5,output || ui.cable_drag==bus?accent:muted);
        circle(35,v.route+6,2,ui.project.insert_output[bus-1]==255?bg:accent);
        if(output){snprintf(ui.status,sizeof ui.status,"Insert %d output: drag to an input; right-click to unplug",bus);if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT))insert_connect(&ui.project,bus,255);if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)){ui.cable_drag=bus;ui.mixer_selected=bus;ui.input_enabled=0;}}
    }
}
static void mixer_studio(float width,float height) {
    Rect root=desktop_editor_rect(3);float scale=ui_scale();
    MixerView v=mixer_view(width,height,ui.project.insert_count,ui.mixer_offset,ui.mixer_fx_hidden);
    ui.mixer_offset=v.offset;
    int shift=host_is_key_down(HOST_KEY_LEFT_SHIFT) || host_is_key_down(HOST_KEY_RIGHT_SHIFT);
    NavigationMotion motion=navigation_motion(ui.navigation_input,shift,0,scale);
    if(!captured() && ui.input_enabled)ui.mixer_offset=mixer_scroll_motion(v,motion.x,motion.y,shift,ui.mouse.x,ui.mouse.y,&ui.mixer_fx_scroll);
    int counts[INSERTS+1];studio_fx_slots(counts);
    int slots=STUDIO_FX_SLOTS;for(int bus=0;bus<=ui.project.insert_count;bus++)slots=fmaxf(slots,counts[bus]);
    ui.mixer_fx_scroll=fmaxf(0,fminf(fmaxf(0,slots*22-v.fx_height),ui.mixer_fx_scroll));
    v.offset=ui.mixer_offset;
    if(button("FX",4,TITLE+4,60,22,!ui.mixer_fx_hidden)){ui.mixer_fx_hidden^=1;ui.input_enabled=0;v=mixer_view(width,height,ui.project.insert_count,ui.mixer_offset,ui.mixer_fx_hidden);}
    label(fit_text(mixer_name(ui.mixer_selected),112,UI_TEXT_SIZE),72,TITLE+7,UI_TEXT_SIZE,ink);
    device_button(0,196,TITLE+4,170);device_button(1,374,TITLE+4,170);
    ui.mixer_fx_scroll=fmaxf(0,fminf(fmaxf(0,slots*22-v.fx_height),ui.mixer_fx_scroll));
    if(v.fx_rows){
        if(button("<",width-44,TITLE+4,18,22,0))ui.mixer_fx_scroll=fmaxf(0,ui.mixer_fx_scroll-22);
        if(button(">",width-24,TITLE+4,18,22,0))ui.mixer_fx_scroll=fminf(slots*22-v.fx_height,ui.mixer_fx_scroll+22);
        if(width>=800)label(host_text_format("%d-%d/%d",(int)(ui.mixer_fx_scroll/22)+1,(int)fminf(slots,ceilf((ui.mixer_fx_scroll+v.fx_height)/22)),slots),width-116,TITLE+7,UI_TEXT_SIZE,muted);
    }
    studio_mixer_scale(v);
    DrawPoint pointer=ui.mouse,origin=ui.text_origin;int enabled=ui.input_enabled;
    for(int bus=0;bus<=v.count;bus++) {
        if(!mixer_bus_visible(v,bus))continue;
        float x=mixer_bus_x(v,bus),left=bus?fmaxf(v.insert_x,x):x,right=bus?fminf(v.insert_x+v.viewport,x+108):x+108;
        if(right<=left)continue;
        host_begin_scissor_mode((root.x+left)*scale,(root.y+v.fx_top)*scale,(right-left)*scale,(height-v.fx_top-18)*scale);
        ui.text_origin=(DrawPoint){(root.x+x)*scale,root.y*scale};
        host_begin_mode2_d((HostCamera){.offset=ui.text_origin,.zoom=scale});
        int strip_enabled=enabled && pointer.x>=left && pointer.x<right && pointer.y>=v.fx_top;
        ui.mouse=(DrawPoint){pointer.x-x,pointer.y};ui.input_enabled=strip_enabled;
        int context=ui.context_opened;
        studio_mixer_strip(v,bus,counts[bus]);
        if(!context && ui.context_opened)ui.context_position.x+=x;
        if(strip_enabled && !ui.input_enabled){enabled=0;}
    }
    ui.mouse=pointer;ui.text_origin=origin;ui.input_enabled=enabled;
    host_begin_mode2_d((HostCamera){.offset=origin,.zoom=scale});
    host_begin_scissor_mode(root.x*scale,root.y*scale,width*scale,height*scale);
    if(ui.mixer_selected>0 && mixer_port_visible(v,ui.mixer_selected,35)) {
        int bus=ui.mixer_selected,dest=ui.project.insert_output[bus-1];
        if(ui.cable_drag==bus || (dest!=255 && mixer_port_visible(v,dest,13))){
            DrawPoint start={mixer_bus_x(v,bus)+35,v.route+6},end=ui.cable_drag==bus?pointer:(DrawPoint){mixer_bus_x(v,dest)+13,v.route+6};
            DrawPoint points[]={start,{start.x,height-19},{end.x,height-19},end};cable(points,host_fade(ui_theme.signal,.8f));
        }
    }
    DrawRect rail={v.insert_x,height-16,v.viewport,12},thumb={mixer_thumb_x(v),height-16,mixer_thumb_width(v),12};
    ui_scrollbar(host_draw(),rail,thumb,ui.mixer_pan || hover(rail.x,rail.y,rail.width,rail.height));
    if(hover(rail.x,rail.y,rail.width,rail.height)) {
        snprintf(ui.status,sizeof ui.status,"Drag to scroll inserts; Master stays fixed. Wheel scrolls; Shift+wheel or trackpad scrolls horizontally.");
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)){
            ui.mixer_pan=1;ui.mixer_scroll_grab=ui.mouse.x>=thumb.x && ui.mouse.x<thumb.x+thumb.width?ui.mouse.x-thumb.x:thumb.width/2;
            ui.mixer_offset=mixer_scroll_from_thumb(v,ui.mouse.x,ui.mixer_scroll_grab);ui.input_enabled=0;
        }
    }
}
void mixer(float width,float height) {
    if(ui.studio_style && ui.windows.workspace.enabled)mixer_studio(width,height);
    else mixer_legacy(width,height);
}
