// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"
#include "signal_palette.h"
#include "mixer_cables.h"
#include "dynamics_curve.h"

int mixer_input_at(DrawPoint point) {
    Rect r=desktop_editor_rect(3); if(!ui.windows.editors[3].visible) return -1;
    if(ui.studio_style && ui.windows.workspace.enabled)return mixer_route_hit(desktop_mixer_view(r.w,r.h),point.x-r.x,point.y-r.y);
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
    if(ui.studio_style && !colored)ui_display_grid(host_draw(),r,1,1); /* Frame and display material, no grid divisions. */
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
    label(fit_text(host_text_format("%s · Slot %d",ui.eq_bus?ui.project.insert_names[ui.eq_bus-1]:"Master",ui.eq_slot+1),width-292,12),12,30,12,muted);
    if(button("Bypass",width-272,26,92,24,ui.project.effect_bypass[ui.eq_bus][ui.eq_slot])) ui.project.effect_bypass[ui.eq_bus][ui.eq_slot]^=1;
    if(button("Load",width-172,26,78,24,0)) preset_action(PRESET_EQ,ui.eq_bus,ui.eq_slot,0);
    if(button("Save",width-88,26,80,24,0)) preset_action(PRESET_EQ,ui.eq_bus,ui.eq_slot,1);
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
    if(ui.studio_style)ui_display_well(host_draw(),(DrawRect){x-1,top,spacing*2,height});
    else host_draw_rectangle(x-1,top,spacing*2,height,bg);
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
        if(ui.studio_style) {
            ui_meter_marks(host_draw(),(DrawRect){left,top,bar_width,height},peak,ui.meter_hold[id][side]>1,1);
        } else if(peak>0) host_draw_rectangle(left-1,fmaxf(top,bottom-peak*height),bar_width+2,1,ui.meter_hold[id][side]>1?ui_theme.meter_high:ui.studio_style?(DrawColor){222,232,181,255}:ink);
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
/* Effect panels share the instrument knob size; displays retain their space. */
static Rect effect_panel_size(int type,int modern_delay) {
    switch(type) {
    case EFFECT_CHORUS:return (Rect){330,164,330,164};
    case EFFECT_PHASER:case EFFECT_FLANGER:return (Rect){600,176,600,176};
    case EFFECT_REVERB:return (Rect){640,176,640,176};
    case EFFECT_TUBE:return (Rect){440,176,440,176};
    case EFFECT_DYNAMICS:return (Rect){960,570,820,480};
    case EFFECT_DELAY:return modern_delay?(Rect){680,184,620,184}:(Rect){420,340,400,340};
    default:return (Rect){620,380,480,330};
    }
}
void mixer_effect_open(int bus,int slot,int toggle) {
    int type=ui.project.effect_type[bus][slot];if(!type)return;
    ui.eq_bus=bus;ui.eq_slot=slot;ui.effect_slot=slot;
    if(!ui.windows.workspace.enabled){windows_focus(&ui.windows,5);return;}
    int id=mixer_effect_window(bus,slot);
    if(id>=5){if(toggle)studio_workspace_hide(&ui.windows,id);else windows_focus(&ui.windows,id);return;}
    for(id=5;id<WINDOW_CAPACITY && ui.windows.editors[id].visible;id++){}
    if(id==WINDOW_CAPACITY){snprintf(ui.status,sizeof ui.status,"Close an effect editor before opening another (16 open).");return;}
    Rect size=effect_panel_size(type,ui.project.delay[bus][slot].model);
    StudioWorkspace *s=&ui.windows.workspace;float width=size.w,height=size.h;
    width=fminf(width,s->width-s->sidebar-10);height=fminf(height,s->height-84);
    int count=0;for(int i=5;i<WINDOW_CAPACITY;i++)count+=ui.windows.editors[i].visible;
    ui.effect_windows[id].bus=bus;ui.effect_windows[id].slot=slot;ui.effect_windows[id].band=0;
    if(type==EFFECT_DYNAMICS)ui.dynamics_band[id]=3;
    ui.windows.editors[id].minw=size.w;ui.windows.editors[id].minh=size.h;
    ui.windows.editors[id].rect=(Rect){fmaxf(s->sidebar+6,s->width-width-12-count*20),fminf(70+count*22,s->height-24-height),width,height};
    ui.windows.workspace.floating[id]=1;ui.windows.editors[id].maximized=0;
    windows_focus(&ui.windows,id);
}
void mixer_effect_add(int bus,int slot,int type) {
    if(bus<0 || bus>ui.project.insert_count || slot<0 || slot>=EFFECT_SLOTS || !effect_descriptor(type))return;
    ui.project.effect_type[bus][slot]=type;ui.project.effect_bypass[bus][slot]=0;
    if(type==EFFECT_CHORUS){ui.project.chorus[bus][slot]=chorus_default();ui.project.effect_mix[bus][slot]=.5f;}
    else if(type==EFFECT_PHASER){ui.project.phaser[bus][slot]=phaser_default();ui.project.effect_mix[bus][slot]=.5f;}
    else if(type==EFFECT_FLANGER){ui.project.flanger[bus][slot]=flanger_default();ui.project.effect_mix[bus][slot]=.5f;}
    else if(type==EFFECT_DYNAMICS){ui.project.dynamics[bus][slot]=dynamics_default();ui.project.effect_mix[bus][slot]=1;}
    else if(type==EFFECT_REVERB){ui.project.reverb[bus][slot]=reverb_default();ui.project.effect_mix[bus][slot]=.25f;}
    else if(type==EFFECT_TUBE){ui.project.tube[bus][slot]=tube_default();ui.project.effect_mix[bus][slot]=1;}
    else if(type==EFFECT_DELAY){ui.project.delay[bus][slot]=delay_default();ui.project.effect_mix[bus][slot]=1;}
    else{ui.project.eq[bus][slot]=equalizer_default();ui.project.effect_mix[bus][slot]=1;}
    if(ui.fx_add_pending && bus==ui.eq_bus && slot==ui.eq_slot) {
        ui.fx_add_pending=0;ui.mixer_fx_scroll[bus]=0;
        if(effect_chain_move(&ui.project,bus,slot,0)) {
            for(int id=5;id<WINDOW_CAPACITY;id++)if(ui.effect_windows[id].bus==bus)ui.effect_windows[id].slot=effect_slot_after_move(ui.effect_windows[id].slot,slot,0);
            effect_target_after_move(&ui.midi_learn_target,bus,slot,0);effect_target_after_move(&ui.automation_target,bus,slot,0);
        }
        slot=0;
    }
    mixer_effect_open(bus,slot,0);
}
void mixer_effect_remove(int bus,int slot) {
    for(int id=5;id<WINDOW_CAPACITY;id++)if(ui.windows.workspace.enabled && ui.effect_windows[id].bus==bus && ui.effect_windows[id].slot==slot)studio_workspace_hide(&ui.windows,id);
    ui.project.effect_type[bus][slot]=EFFECT_EMPTY;
    for(int a=ui.project.automation_count-1;a>=0;a--){ParameterTarget t=ui.project.automations[a].target;
        if(parameter_is_effect(t.parameter) && t.owner==(unsigned)bus && t.slot==(unsigned)slot)automation_delete(&ui.project,a);}
    ui.control_drag=NULL;ui.automation_selected=ui.automation_node=-1;ui.picker_drag=-1;ui.eq_drag=-1;
}
void mixer_effect_replace(int bus,int slot,int type) {
    if(bus<0 || bus>ui.project.insert_count || slot<0 || slot>=EFFECT_SLOTS || !effect_descriptor(type))return;
    ui.fx_add_pending=0;
    for(int i=ui.project.midi_binding_count-1;i>=0;i--) {
        ParameterTarget t=ui.project.midi_bindings[i].target;
        if(parameter_is_effect(t.parameter) && t.owner==(unsigned)bus && t.slot==(unsigned)slot)midi_unbind(&ui.project,t);
    }
    if(parameter_is_effect(ui.midi_learn_target.parameter) && ui.midi_learn_target.owner==(unsigned)bus && ui.midi_learn_target.slot==(unsigned)slot)ui.midi_learning=0;
    mixer_effect_remove(bus,slot);
    mixer_effect_add(bus,slot,type);
}
void effect_slot_menu(int bus,int slot) {
    ui.fx_add_pending=0;
    ui.eq_bus=bus;ui.eq_slot=slot;ui.effect_slot=slot;
    open_context(25,slot,(DrawPoint){ui.mouse.x+desktop_editor_rect(3).x,ui.mouse.y+desktop_editor_rect(3).y});
}
static void delay_editor_compact(float width,float height,int bus,int slot) {
    DelaySettings *d=&ui.project.delay[bus][slot];
    const float edges[]={0,.22f,.62f,.87f,1};
    const char *groups[]={"INPUT","FEEDBACK","TIME","DRY"};
    for(int i=0;i<4;i++) {
        float x=edges[i]*width,w=(edges[i+1]-edges[i])*width;
        host_draw_rectangle(x+1,24,w-2,124,i&1?cell:bg);
        label(groups[i],x+8,30,13,muted);
        if(i)host_draw_line(x,24,x,148,ui_theme.border);
    }
    const char *modes[]={"Normal","Invert","P.pong"};
    for(unsigned i=0;i<3;i++) {
        float x=width*.22f+8,y=54+i*30;
        if(button(modes[i],x,y,width*.12f-8,24,d->mode==i))d->mode=i;
    }
    const float centers[]={.055f,.165f,.40f,.53f,.6825f,.8075f,.935f};
    float *values[]={&d->input_pan,&d->input_gain,&d->feedback,&d->cutoff,&d->steps,&d->offset,&d->dry};
    const unsigned ids[]={PARAM_DELAY_INPUT_PAN,PARAM_DELAY_INPUT_GAIN,PARAM_DELAY_FEEDBACK,PARAM_DELAY_CUTOFF,PARAM_DELAY_STEPS,PARAM_DELAY_OFFSET,PARAM_DELAY_DRY};
    const int styles[]={KNOB_PAN,KNOB_NORMAL,KNOB_NORMAL,KNOB_LOGARITHMIC,KNOB_MAGNETIC,KNOB_PAN,KNOB_NORMAL};
    const char *names[]={"PAN","VOL","VOL","CUT","TIME","OFS","VOL"};
    for(int i=0;i<7;i++) {
        float x=centers[i]*width,radius=i==4?18:PANEL_KNOB_RADIUS;
        const ParameterDescriptor *info=parameter_descriptor(ids[i]);
        if(i==4) {
            unsigned count;const float *marks=control_step_marks(&count);
            float shown=displayed_value(values[i],*values[i]),ring=radius+7;
            for(unsigned dot=0;dot+1<count;dot++) {
                float fraction=logf(marks[dot]/info->low)/logf(info->high/info->low);
                float angle=-PI*.5f+fraction*2*PI;
                int active=fabsf(shown-marks[dot])<marks[dot]*.00001f || (!dot && fabsf(shown-info->high)<info->high*.00001f);
                /* Endpoints share the knob's existing full-turn position. */
                circle(x+cosf(angle)*ring,85+sinf(angle)*ring,active?2.3f:1.7f,active?accent:muted);
            }
        }
        knob_sized(x,85,values[i],info->low,info->high,info->initial,info->name,styles[i],radius);
        label(names[i],x-text_width(names[i],12)/2,116,12,ink);
        float shown=displayed_value(values[i],*values[i]);
        const char *number=i==4?host_text_format("%.3g",shown):i==3?host_text_format("%.0f",shown):host_text_format("%.0f%%",shown*100);
        label(number,x-text_width(number,11)/2,134,11,muted);
        if(i==4 && (hover(x-radius-8,59,2*radius+16,52) || ui.control_drag==values[i]))
            snprintf(ui.status,sizeof ui.status,"Delay time: %.5g steps / %.3f ms | magnetic rhythm marks; Ctrl = fine, unsnapped",shown,delay_time_ms((DelaySettings){.model=1,.steps=shown},ui.project.bpm));
        if(i==5 && hover(x-radius,85-radius,2*radius,2*radius))
            snprintf(ui.status,sizeof ui.status,"Stereo offset: negative delays Left, positive delays Right; center = equal times");
    }
    float bottom=height-30;
    if(button("Bypass",8,bottom,92,24,ui.project.effect_bypass[bus][slot]))ui.project.effect_bypass[bus][slot]^=1;
    if(button("Load",width-172,bottom,78,24,0))preset_action(PRESET_DELAY,bus,slot,0);
    if(button("Save",width-88,bottom,80,24,0))preset_action(PRESET_DELAY,bus,slot,1);
}
/* The transfer curve is level-in -> level-out, not a frequency response. */
static float dyn_x(float db,DrawRect r){return r.x+dynamics_level_position(db)*r.width;}
static float dyn_y(float db,DrawRect r){return r.y+r.height-dynamics_level_position(db)*r.height;}
static void dynamics_editor(float width,float height,int bus,int slot){
    static DynamicsMonitor monitor[WINDOW_CAPACITY];
    static const DrawColor colors[4]={{223,103,96,255},{219,168,85,255},{211,207,111,255},{142,204,113,255}};
    static const char *bands[]={"LOW","MID","HIGH","MASTER"};
    int window=ui.knob_context,selected=ui.dynamics_band[window];if(selected<0 || selected>3)selected=3;
    DynamicsSettings *s=&ui.project.dynamics[bus][slot];DynamicsBand *b=&s->band[selected];DrawColor color=colors[selected],graph_ink={151,167,162,255};
    s->lowcut=fminf(80,s->lowcut);s->low=fmaxf(s->low,s->lowcut);
    DynamicsMonitor *m=&monitor[window];if(!ui.dynamics_pause[window])audio_dynamics_monitor(bus,slot,m);
    float graph_h=fmaxf(180,height-320),left_w=width*.36f;
    DrawRect curve={32,49,left_w-42,graph_h},history={left_w+8,49,width-left_w-40,graph_h};
    host_draw_rectangle(8,29,width-16,graph_h+48,(DrawColor){32,40,40,255});
    if(ui.studio_style){ui_graph_paper(host_draw(),curve);ui_graph_paper(host_draw(),history);}
    unsigned tick_count;const float *ticks=dynamics_grid_values(&tick_count);
    for(unsigned ti=1;ti<tick_count;ti++){int db=ticks[ti];float y=dyn_y(db,curve);host_draw_line(curve.x,y,curve.x+curve.width,y,ui_theme.grid_minor);host_draw_line(dyn_x(db,curve),curve.y,dyn_x(db,curve),curve.y+curve.height,ui_theme.grid_minor);label(host_text_format("%d",db),9,y-6,12,graph_ink);host_draw_line(history.x,y,history.x+history.width,y,ui_theme.grid_minor);}
    int magnet_hover=hover(curve.x+12,curve.y+curve.height+3,112,22);
    if(button("Magnet",curve.x+12,curve.y+curve.height+3,112,22,ui.dynamics_magnet[window]))ui.dynamics_magnet[window]^=1;
    if(magnet_hover)snprintf(ui.status,sizeof ui.status,"Magnet: snap curve points to visible grid intersections; bend handles stay free");
    label("OUT",curve.x,32,12,graph_ink);label("IN",curve.x+curve.width-20,curve.y+curve.height+3,12,graph_ink);
    host_draw_line_ex((DrawPoint){curve.x,curve.y+curve.height},(DrawPoint){curve.x+curve.width,curve.y},1,graph_ink);
    /* One live band-level meter: horizontal extent follows input level, and
       each filled column ends at the transfer curve rather than crossing it. */
    float live_level=m->count?m->input[selected][m->count-1]:-72;
    for(int px=0;px<(int)curve.width;px++){
        float at=(px+.5f)/curve.width,h=dynamics_meter_height(b,live_level,at);
        if(h>0){float top=ceilf(curve.y+curve.height*(1-h));host_draw_line(curve.x+px,top,curve.x+px,curve.y+curve.height,(DrawColor){66,76,75,255});}
    }
    DrawPoint previous={curve.x,dyn_y(dynamics_curve(b,-72),curve)};
    for(int px=1;px<=(int)curve.width;px++){float x=dynamics_level_db(px/curve.width);DrawPoint next={curve.x+px,dyn_y(dynamics_curve(b,x),curve)};host_draw_line_ex(previous,next,2,color);previous=next;}
    int point_hit=-1,bend_hit=-1;
    for(int i=0;i<(int)b->count;i++){
        DrawPoint p={dyn_x(b->points[i].x,curve),dyn_y(b->points[i].y,curve)};circle(p.x,p.y,4,color);
        if(hover(p.x-8,p.y-8,16,16))point_hit=i;
        if(i<(int)b->count-1){float x=dynamics_curve_handle_input(b,i);DrawPoint h={dyn_x(x,curve),dyn_y(dynamics_curve(b,x),curve)};circle(h.x,h.y,4,color);circle(h.x,h.y,2,(DrawColor){32,40,40,255});if(hover(h.x-7,h.y-7,14,14))bend_hit=i;}
    }
    int in_curve=hover(curve.x-8,curve.y-8,curve.width+16,curve.height+16) && !magnet_hover;
    float input=dynamics_level_db((ui.mouse.x-curve.x)/curve.width),output=dynamics_level_db(1-(ui.mouse.y-curve.y)/curve.height);
    if(in_curve){
        snprintf(ui.status,sizeof ui.status,"Curve: right-click adds/deletes a point; Magnet snaps points to grid; drag hollow handles to bend; Shift locks Y, Ctrl locks X; wheel changes bend shape");
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)){if(point_hit>=0)dynamics_curve_delete(b,point_hit);else if(bend_hit>=0)dynamics_curve_bend(b,bend_hit,0);else dynamics_curve_insert(b,ui.dynamics_magnet[window]?dynamics_grid_snap(input):roundf(input),ui.dynamics_magnet[window]?dynamics_grid_snap(output):roundf(output));}
        if(bend_hit>=0 && point_hit<0){float wheel=host_get_mouse_wheel_move();if(wheel)b->points[bend_hit].shape=fmodf(b->points[bend_hit].shape+(wheel>0?1:4),5);}
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) && (point_hit>=0 || bend_hit>=0)){
            ui.dynamics_drag=1+(point_hit>=0?point_hit:bend_hit);ui.dynamics_drag_kind=point_hit<0;ui.dynamics_drag_window=window;ui.dynamics_drag_band=selected;ui.dynamics_drag_y=ui.mouse.y;ui.dynamics_drag_x=point_hit>=0?0:b->points[bend_hit].bend;
        }
    }
    if(ui.dynamics_drag && ui.dynamics_drag_window==window){
        if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT) || ui.dynamics_drag_band!=selected)ui.dynamics_drag=0;
        else if(ui.dynamics_drag_kind)dynamics_curve_bend(b,ui.dynamics_drag-1,ui.dynamics_drag_x+(ui.dynamics_drag_y-ui.mouse.y)/(host_is_key_down(HOST_KEY_LEFT_CONTROL)?600:120));
        else dynamics_curve_move_grid(b,ui.dynamics_drag-1,input,output,host_is_key_down(HOST_KEY_LEFT_CONTROL) || host_is_key_down(HOST_KEY_RIGHT_CONTROL),host_is_key_down(HOST_KEY_LEFT_SHIFT) || host_is_key_down(HOST_KEY_RIGHT_SHIFT),ui.dynamics_magnet[window]);
    }
    int knob_hold=0;
    if(host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT)) {
        knob_hold=ui.control_drag==&s->low || ui.control_drag==&s->high || ui.control_drag==&s->lowcut;
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) {
            float top=graph_h+81;
            knob_hold|=hover(width*.78f-13,top+37,26,26) || hover(width*.93f-13,top+37,26,26) || hover(width*.93f-13,top+103,26,26);
        }
    }
    if(ui.dynamics_view[window] || knob_hold){
        float lo=logf(s->low/20)/logf(1000),hi=logf(s->high/20)/logf(1000);
        float bounds[4];dynamics_band_bounds(s->low,s->high,s->lowcut,bounds);
        for(int px=0;px<(int)history.width;px++){
            float position=(px+.5f)/history.width,hz=20*powf(1000,position);
            int band=position<lo?0:position<hi?1:2;
            float gain=dynamics_lowcut_gain(s->lowcut,hz);DrawColor fill=colors[band];fill.a=90;
            host_draw_rectangle(history.x+px,history.y+history.height*(1-gain),1,history.height*gain,fill);
        }
        const float starts[]={0,lo,hi},ends[]={lo,hi,1};
        for(int i=0;i<3;i++)if(history.width*(ends[i]-starts[i])>=text_width(bands[i],12)+12)
            label(bands[i],history.x+history.width*starts[i]+6,history.y+8,12,colors[i]);
        if(s->lowcut>0){float x=history.x+history.width*bounds[0];host_draw_line(x,history.y,x,history.y+history.height,graph_ink);}
        for(int i=0;i<2;i++){float x=history.x+history.width*(i?hi:lo);host_draw_line(x,history.y,x,history.y+history.height,ink);if(!knob_hold && hover(x-8,history.y,16,history.height) && host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT)){float hz=20*powf(1000,(ui.mouse.x-history.x)/history.width);if(i)s->high=fmaxf(fmaxf(1000,s->low+1),fminf(16000,hz));else s->low=fmaxf(fmaxf(40,s->lowcut),fminf(fminf(1000,s->high-1),hz));}}
        label("20 Hz",history.x,history.y+history.height+3,12,graph_ink);label("20 kHz",history.x+history.width-54,history.y+history.height+3,12,graph_ink);
    }else{
        if(hover(history.x,history.y,history.width,history.height) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT))ui.dynamics_pause[window]^=1;
        for(unsigned i=1;i<m->count;i++){
            float at0=dynamics_monitor_position(m->count-i,ui.dynamics_speed[window]),at1=dynamics_monitor_position(m->count-1-i,ui.dynamics_speed[window]);
            if(at0<0 || at1>1)continue;
            float x0=history.x+at0*history.width,x1=history.x+at1*history.width;
            DrawPoint a={x0,dyn_y(m->input[selected][i-1],history)},z={x1,dyn_y(m->input[selected][i],history)},bottom0={x0,history.y+history.height},bottom1={x1,history.y+history.height};
            host_draw_triangle(a,bottom0,z,(DrawColor){72,80,79,255});host_draw_triangle(z,bottom0,bottom1,(DrawColor){72,80,79,255});
            host_draw_line_ex((DrawPoint){x0,dyn_y(m->output[selected][i-1],history)},(DrawPoint){x1,dyn_y(m->output[selected][i],history)},1,color);
            host_draw_line_ex((DrawPoint){x0,dyn_y(m->gain[selected][i-1],history)},(DrawPoint){x1,dyn_y(m->gain[selected][i],history)},1.5f,(DrawColor){128,204,220,255});
        }
        label("INPUT / OUTPUT / GAIN",history.x+5,32,12,graph_ink);
        float gain=m->count?m->gain[selected][m->count-1]:0;label(host_text_format("Gain %+.1f dB",fabsf(gain)<.25f?0:gain),history.x+history.width-128,32,12,accent);
    }
    float speed_x=history.x+history.width-112,speed_y=history.y+history.height+17;
    knob_sized(speed_x,speed_y,&ui.dynamics_speed[window],-1,1,0,"Monitor speed (center = 0.85x)",KNOB_CENTER,10);
    label("SPEED",speed_x-65,speed_y-6,12,graph_ink);
    label(host_text_format("%.2fx",dynamics_monitor_speed(ui.dynamics_speed[window])),speed_x+18,speed_y-6,12,graph_ink);
    if(hover(speed_x-12,speed_y-12,24,24) || ui.control_drag==&ui.dynamics_speed[window])snprintf(ui.status,sizeof ui.status,"Monitor speed: left = slower / more history; center = 0.85x; right = faster / more detail; audio is unchanged");
    float tabs=graph_h+81;int narrow=width<760;
    float band_w=narrow?60:78,mode_x=narrow?76:92;
    host_draw_line(width*.405f,tabs,width*.405f,height-36,ui_theme.border);
    host_draw_line(width*.705f,tabs+29,width*.705f,height-36,ui_theme.border);
    for(int i=0;i<4;i++)if(button(bands[i],8,tabs+i*26,band_w,23,i==selected)){ui.dynamics_band[window]=i;ui.dynamics_drag=0;}
    if(button("Monitor",width-264,tabs,84,23,!ui.dynamics_view[window]))ui.dynamics_view[window]=0;
    if(button("Bands",width-176,tabs,78,23,ui.dynamics_view[window]))ui.dynamics_view[window]=1;
    if(button(ui.dynamics_pause[window]?"Resume":"Pause",width-94,tabs,86,23,ui.dynamics_pause[window]))ui.dynamics_pause[window]^=1;
    const char *modes[]={"On","Comp off","Muted","Off"};
    for(int i=0;i<4;i++)if(sampler_toggle(mode_x,tabs+i*22,narrow?88:104,modes[i],(int)b->mode==i))b->mode=i;
    if(sampler_toggle(8,tabs+122,80,"Solo",b->solo))b->solo=!b->solo;
    if(sampler_toggle(width*.51f,tabs+112,100,"RMS",b->rms))b->rms=!b->rms;
    if(hover(width*.51f,tabs+112,100,22))snprintf(ui.status,sizeof ui.status,"Detector: RMS measures average energy; unchecked uses peaks");
    float *values[]={&b->pre,&b->post,&b->attack,&b->release,&b->release2,&b->sustain,&b->width,&b->saturation,&b->ceiling,&s->low,&s->high,&s->lowcut,&s->blend};
    const int offsets[]={0,1,2,3,4,5,6,11,10,-4,-3,-2,-1};
    const char *names[]={"PRE","POST","ATTACK","RELEASE","REL 2","HOLD","WIDTH","SAT","CEIL","LOW","HIGH","LOW CUT","BAND MIX"};
    const float positions[]={.28f,.37f,.46f,.55f,.46f,.64f,.16f,.28f,.37f,.78f,.93f,.93f,.78f};
    for(int i=0;i<13;i++){
        int off=offsets[i];const ParameterDescriptor *info=parameter_descriptor(PARAM_DYNAMICS_FIRST+(off<0?off+4:4+selected*12+off));
        int bottom=i==4 || (i>=6 && i!=9 && i!=10);
        float x=width*positions[i],ky=tabs+(bottom?116:50);
        if(i==6)ky=tabs+122;
        knob_sized(x,ky,values[i],i==9?fmaxf(40,s->lowcut):info->low,info->high,info->initial,info->name,i==6?KNOB_WIDTH:info->kind==PARAMETER_LOGARITHMIC?KNOB_LOGARITHMIC:KNOB_NORMAL,PANEL_KNOB_RADIUS);
        label(names[i],x-text_width(names[i],12)/2,ky+20,12,ink);
        float shown=displayed_value(values[i],*values[i]);
        const char *number=i==0 || i==1 || i==8?host_text_format("%+.1f",shown):i>=2 && i<=5?host_text_format("%.0fms",shown):i==9 || i==10 || i==11?(shown>=1000?host_text_format("%.2gkHz",shown/1000):host_text_format("%.0fHz",shown)):host_text_format("%.0f%%",shown*100);
        label(number,x-text_width(number,12)/2,ky+36,12,muted);
        if(i==6 && hover(x-14,ky-14,28,28))snprintf(ui.status,sizeof ui.status,"Stereo width: 0%% = mono, 100%% = unchanged stereo; higher widens this band");
        if(i==12 && hover(x-14,ky-14,28,28))snprintf(ui.status,sizeof ui.status,"Band Mix: blend compressed low/mid/high bands with phase-aligned uncompressed audio before Master");
    }
    s->low=fmaxf(s->low,s->lowcut);
    float footer=height-30;
    if(button("Bypass",8,footer,92,24,ui.project.effect_bypass[bus][slot]))ui.project.effect_bypass[bus][slot]^=1;
    if(button("Reset curve",108,footer,112,24,0)){DynamicsSettings reset=dynamics_default();memcpy(b->points,reset.band[selected].points,sizeof b->points);b->count=3;}
    if(button("Copy",226,footer,64,24,0)){ui.dynamics_copy=*b;ui.dynamics_has_copy=1;}
    if(button("Paste",296,footer,68,24,0) && ui.dynamics_has_copy){memcpy(b->points,ui.dynamics_copy.points,sizeof b->points);b->count=ui.dynamics_copy.count;}
    float wet_x=width-204;
    knob_sized(wet_x,footer+12,&ui.project.effect_mix[bus][slot],0,1,1,"Multiband Compressor wet/dry mix",KNOB_NORMAL,PANEL_KNOB_RADIUS);
    label("WET",wet_x-44,footer+5,12,muted);
    if(button("Load",width-172,footer,78,24,0))preset_action(PRESET_DYNAMICS,bus,slot,0);
    if(button("Save",width-88,footer,80,24,0))preset_action(PRESET_DYNAMICS,bus,slot,1);

}

static void modulation_editor(float width,float height,int bus,int slot){
    int phaser=ui.project.effect_type[bus][slot]==EFFECT_PHASER;
    PhaserSettings *p=&ui.project.phaser[bus][slot];FlangerSettings *f=&ui.project.flanger[bus][slot];
    label(phaser?"Phaser / swept allpass":"Flanger / short-delay sweep",12,32,13,ink);
    if(!phaser && button("Invert wet",width-126,28,114,24,f->invert))f->invert=!f->invert;
    const char *phaser_names[]={"RATE","CENTER","DEPTH","FEEDBACK","STEREO","STAGES","MIX"};
    const char *flanger_names[]={"RATE","DELAY","DEPTH","FEEDBACK","STEREO","MIX"};
    int count=phaser?7:6;
    for(int i=0;i<count;i++){
        unsigned id=i==count-1?PARAM_EFFECT_MIX:(phaser?PARAM_PHASER_FIRST:PARAM_FLANGER_FIRST)+i;
        float *value=i==count-1?&ui.project.effect_mix[bus][slot]:(float *)(phaser?phaser_parameter(p,i):flanger_parameter(f,i));
        const ParameterDescriptor *info=parameter_descriptor(id);const char *name=phaser?phaser_names[i]:flanger_names[i];
        float x=width*(i+.5f)/count;
        knob_sized(x,82,value,info->low,info->high,i==count-1?.5f:info->initial,info->name,info->kind==PARAMETER_LOGARITHMIC?KNOB_LOGARITHMIC:i==3?KNOB_PAN:KNOB_NORMAL,PANEL_KNOB_RADIUS);
        label(name,x-text_width(name,12)/2,106,12,ink);
        float shown=displayed_value(value,*value);
        const char *number=i==0?host_text_format("%.2f Hz",shown):i==1?(phaser?host_text_format("%.0f Hz",shown):host_text_format("%.2f ms",shown)):phaser && i==2?host_text_format("%.2f oct",shown):phaser && i==5?host_text_format("%.0f",shown):i==3?host_text_format("%+.0f%%",shown*100):host_text_format("%.0f%%",shown*100);
        label(number,x-text_width(number,12)/2,126,12,muted);
        if(i==4 && (hover(x-13,69,26,26) || ui.control_drag==value))snprintf(ui.status,sizeof ui.status,"Stereo spread: LFO phase offset, 0%% keeps a centered source centered; 100%% = 180 degrees");
    }
    float bottom=height-30;
    if(button("Bypass",8,bottom,92,24,ui.project.effect_bypass[bus][slot]))ui.project.effect_bypass[bus][slot]^=1;
    if(button("Load",width-172,bottom,78,24,0))preset_action(phaser?PRESET_PHASER:PRESET_FLANGER,bus,slot,0);
    if(button("Save",width-88,bottom,80,24,0))preset_action(phaser?PRESET_PHASER:PRESET_FLANGER,bus,slot,1);
}

void effect_editor(float width,float height) {
    int bus=ui.eq_bus,slot=ui.eq_slot;
    if(bus<0 || bus>ui.project.insert_count || slot<0 || slot>=EFFECT_SLOTS)return;
    if(ui.windows.workspace.enabled){Rect size=effect_panel_size(ui.project.effect_type[bus][slot],ui.project.delay[bus][slot].model);ui.windows.editors[ui.knob_context].minw=size.w;ui.windows.editors[ui.knob_context].minh=size.h;}
    if(!ui.project.effect_type[bus][slot]){label("Select an effect slot in the Mixer.",16,40,13,muted);return;}
    if(ui.project.effect_type[bus][slot]==EFFECT_PHASER || ui.project.effect_type[bus][slot]==EFFECT_FLANGER){modulation_editor(width,height,bus,slot);return;}
    if(ui.project.effect_type[bus][slot]==EFFECT_DYNAMICS){dynamics_editor(width,height,bus,slot);return;}
    if(ui.project.effect_type[bus][slot]==EFFECT_REVERB) {
        ReverbSettings *r=&ui.project.reverb[bus][slot];
        label("Reverb / diffuse stereo",12,32,13,ink);
        float *values[]={&r->size,&r->decay,&r->damping,&r->predelay,&r->width,&r->mod,&r->gate,&ui.project.effect_mix[bus][slot]};
        const unsigned ids[]={PARAM_REVERB_SIZE,PARAM_REVERB_DECAY,PARAM_REVERB_DAMPING,PARAM_REVERB_PREDELAY,PARAM_REVERB_WIDTH,PARAM_REVERB_MOD,PARAM_REVERB_GATE,PARAM_EFFECT_MIX};
        const char *names[]={"SIZE","DECAY","DAMP","PRE","WIDTH","MOD","GATE","MIX"};
        for(int i=0;i<8;i++) {
            const ParameterDescriptor *info=parameter_descriptor(ids[i]);float x=width*(i+.5f)/8;
            int style=i==1 || i==2?KNOB_LOGARITHMIC:i==4?KNOB_WIDTH:KNOB_NORMAL;
            knob_sized(x,82,values[i],info->low,info->high,i==7?.25f:info->initial,info->name,style,PANEL_KNOB_RADIUS);
            label(names[i],x-text_width(names[i],12)/2,106,12,ink);
            float shown=displayed_value(values[i],*values[i]);
            const char *number=i==1?host_text_format("%.2f s",shown):i==2?(shown>=19999?"Open":host_text_format("%.0f Hz",shown)):i==3 || i==6?(i==6 && shown==0?"Off":host_text_format("%.0f ms",shown)):host_text_format("%.0f%%",shown*100);
            label(number,x-text_width(number,12)/2,126,12,muted);
            if(i==6 && (hover(x-13,69,26,26) || ui.control_drag==values[i]))snprintf(ui.status,sizeof ui.status,"Reverb gate: wet-only hold after input above -48 dBFS; Off = natural decay");
        }
        float bottom=height-30;
        if(button("Bypass",8,bottom,92,24,ui.project.effect_bypass[bus][slot]))ui.project.effect_bypass[bus][slot]^=1;
        if(button("Load",width-172,bottom,78,24,0))preset_action(PRESET_REVERB,bus,slot,0);
        if(button("Save",width-88,bottom,80,24,0))preset_action(PRESET_REVERB,bus,slot,1);
        return;
    }
    if(ui.project.effect_type[bus][slot]==EFFECT_TUBE) {
        TubeSettings *t=&ui.project.tube[bus][slot];
        float voicing=displayed_value(&t->natural,t->natural);
        label(voicing>=.5f?"Tube / natural saturation":"Tube / warm saturation",12,32,13,(DrawColor){100,50,13,255});
        if(button("Warm",width-172,28,78,24,voicing<.5f))t->natural=0;
        if(button("Natural",width-88,28,80,24,voicing>=.5f))t->natural=1;
        if(hover(width-172,28,164,24)){
            snprintf(ui.status,sizeof ui.status,"Tube voicing: Warm = original sound; Natural = more high-end bandwidth; Tone still applies in both");
            if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT))control_menu(&t->natural,0,1,0,"Tube voicing (0 Warm / 1 Natural)");
        }
        float *values[]={&t->drive,&t->tone,&ui.project.effect_mix[bus][slot],&t->output};
        const unsigned ids[]={PARAM_TUBE_DRIVE,PARAM_TUBE_TONE,PARAM_EFFECT_MIX,PARAM_TUBE_OUTPUT};
        const char *names[]={"DRIVE","TONE","MIX","OUTPUT"};
        for(int i=0;i<4;i++) {
            const ParameterDescriptor *info=parameter_descriptor(ids[i]);float x=width*(i+.5f)/4;
            knob_sized(x,82,values[i],info->low,info->high,i==2?1:info->initial,info->name,i==1?KNOB_LOGARITHMIC:KNOB_NORMAL,PANEL_KNOB_RADIUS);
            label(names[i],x-text_width(names[i],12)/2,106,12,ink);
            float shown=displayed_value(values[i],*values[i]);
            const char *number=i==1?(shown>TUBE_TONE_MAX?"None":host_text_format("%.0f Hz",shown)):i==2?host_text_format("%.0f%%",shown*100):host_text_format("%.1f dB",shown);
            label(number,x-text_width(number,12)/2,126,12,muted);
            if(i==1 && (hover(x-13,69,26,26) || ui.control_drag==values[i]))snprintf(ui.status,sizeof ui.status,shown>TUBE_TONE_MAX?"Tube Tone: None | Tone low-pass bypassed":"Tube Tone: %.0f Hz | turn past 20000 Hz for None",shown);
        }
        float bottom=height-30;
        if(button("Bypass",8,bottom,92,24,ui.project.effect_bypass[bus][slot]))ui.project.effect_bypass[bus][slot]^=1;
        if(button("Load",width-172,bottom,78,24,0))preset_action(PRESET_TUBE,bus,slot,0);
        if(button("Save",width-88,bottom,80,24,0))preset_action(PRESET_TUBE,bus,slot,1);
        return;
    }
    if(ui.project.effect_type[bus][slot]==EFFECT_DELAY) {
        DelaySettings *d=&ui.project.delay[bus][slot];
        if(d->model){delay_editor_compact(width,height,bus,slot);return;}
        label(fit_text(host_text_format("%s / Delay",mixer_name(bus)),width-24,13),12,32,13,ink);
        if(button(d->sync?"Tempo sync":"Milliseconds",12,60,(width-28)/2,24,d->sync))d->sync^=1;
        if(button(delay_division_name(d->division),16+(width-28)/2,60,(width-28)/2,24,d->sync))d->division=(d->division+1)%DELAY_DIVISIONS;
        if(hover(16+(width-28)/2,60,(width-28)/2,24))snprintf(ui.status,sizeof ui.status,"Delay division: click to cycle; T = triplet, D = dotted (maximum 4 seconds)");
        float col=(width-32)/3;
        float *values[]={&d->time_ms,&d->feedback,&d->cutoff,&d->stereo,&ui.project.effect_mix[bus][slot],&d->output};
        const float lo[]={1,0,100,0,0,0},hi[]={4000,.95f,20000,1,1,2},initial[]={375,.35f,8000,0,.35f,1};
        const char *names[]={"Time ms","Feedback","Tone Hz","Ping-pong","Mix","Output"};
        const unsigned ids[]={PARAM_DELAY_TIME,PARAM_DELAY_FEEDBACK,PARAM_DELAY_CUTOFF,PARAM_DELAY_STEREO,PARAM_EFFECT_MIX,PARAM_DELAY_OUTPUT};
        for(int i=0;i<6;i++) {
            float x=16+col*(i%3+.5f),y=116+86*(i/3);
            if(i!=0 || !d->sync)knob(x,y,values[i],lo[i],hi[i],initial[i],parameter_descriptor(ids[i])->name);
            else {label(host_text_format("%.0f",delay_time_ms(*d,ui.project.bpm)),x-24,y-8,13,ink);}
            label(names[i],x-text_width(names[i],12)/2,y+24,12,muted);
            if(i)label(host_text_format(i==2?"%.0f":"%.2f",displayed_value(values[i],*values[i])),x-24,y+42,12,ink);
        }
        if(button("Bypass",12,270,width-24,24,ui.project.effect_bypass[bus][slot]))ui.project.effect_bypass[bus][slot]^=1;
        if(button("Load",12,302,(width-28)/2,24,0))preset_action(PRESET_DELAY,bus,slot,0);
        if(button("Save",16+(width-28)/2,302,(width-28)/2,24,0))preset_action(PRESET_DELAY,bus,slot,1);
        return;
    }
    if(ui.project.effect_type[bus][slot]!=EFFECT_CHORUS){eq_editor(width,height);return;}
    ChorusSettings *s=&ui.project.chorus[bus][slot];
    label("Chorus / stereo modulation",12,32,13,ink);
    float *values[]={&s->rate,&s->depth,&ui.project.effect_mix[bus][slot]};
    const unsigned ids[]={PARAM_CHORUS_RATE,PARAM_CHORUS_DEPTH,PARAM_EFFECT_MIX};
    const char *names[]={"RATE","DEPTH","MIX"};
    for(int i=0;i<3;i++) {
        const ParameterDescriptor *info=parameter_descriptor(ids[i]);float x=width*(i+.5f)/3;
        knob_sized(x,72,values[i],info->low,info->high,i==2?.5f:info->initial,info->name,KNOB_NORMAL,PANEL_KNOB_RADIUS);
        label(names[i],x-text_width(names[i],12)/2,94,12,ink);
        float shown=displayed_value(values[i],*values[i]);
        const char *number=host_text_format(i==0?"%.2f Hz":i==1?"%.2f ms":"%.0f%%",i==2?shown*100:shown);
        label(number,x-text_width(number,12)/2,114,12,muted);
    }
    float bottom=height-30;
    if(button("Bypass",8,bottom,92,24,ui.project.effect_bypass[bus][slot]))ui.project.effect_bypass[bus][slot]^=1;
    if(button("Load",width-172,bottom,78,24,0))preset_action(PRESET_CHORUS,bus,slot,0);
    if(button("Save",width-88,bottom,80,24,0))preset_action(PRESET_CHORUS,bus,slot,1);

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
        knob_style(x+35,control_y,id?&ui.project.insert_pan[id-1]:&ui.project.master_pan,-1,1,0,id?"Insert pan":"Master pan",KNOB_PAN);
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
            ui_display_well(host_draw(),(DrawRect){x+20,height-51,46,18});
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
        if(button("+ Delay",fx+8,effect_button_y+27,MIXER_PANEL-16,24,0))mixer_effect_add(bus,slot,EFFECT_DELAY);
        if(button("+ Tube",fx+8,effect_button_y+54,MIXER_PANEL-16,24,0))mixer_effect_add(bus,slot,EFFECT_TUBE);
        if(button("+ Phaser",fx+8,effect_button_y+135,MIXER_PANEL-16,24,0))mixer_effect_add(bus,slot,EFFECT_PHASER);
        if(button("+ Flanger",fx+8,effect_button_y+162,MIXER_PANEL-16,24,0))mixer_effect_add(bus,slot,EFFECT_FLANGER);
        if(button("+ Multiband Compressor",fx+8,effect_button_y+108,MIXER_PANEL-16,24,0))mixer_effect_add(bus,slot,EFFECT_DYNAMICS);
        if(button("+ Reverb",fx+8,effect_button_y+81,MIXER_PANEL-16,24,0))mixer_effect_add(bus,slot,EFFECT_REVERB);
    } else {
        label(effect_descriptor(ui.project.effect_type[bus][slot])->name,fx+8,effect_y,12,ink);
        if(button("x",fx+MIXER_PANEL-28,ui.studio_style?effect_y-2:TITLE+144,20,ui.studio_style?20:22,0)) {
            ui.project.effect_type[bus][slot]=EFFECT_EMPTY;
            for(int a=ui.project.automation_count-1;a>=0;a--) {
                ParameterTarget t=ui.project.automations[a].target;
                if(parameter_is_effect(t.parameter) && t.owner==(unsigned)bus && t.slot==(unsigned)slot) automation_delete(&ui.project,a);
            }
            ui.control_drag=NULL; ui.automation_selected=ui.automation_node=-1; ui.picker_drag=-1;
        } else if(ui.project.effect_type[bus][slot]==EFFECT_PHASER) {if(button("Open Phaser",fx+8,effect_button_y,MIXER_PANEL-16,24,0))mixer_effect_open(bus,slot,0);
        } else if(ui.project.effect_type[bus][slot]==EFFECT_FLANGER) {if(button("Open Flanger",fx+8,effect_button_y,MIXER_PANEL-16,24,0))mixer_effect_open(bus,slot,0);
        } else if(ui.project.effect_type[bus][slot]==EFFECT_DYNAMICS) { if(button("Open Multiband Compressor",fx+8,effect_button_y,MIXER_PANEL-16,24,0))mixer_effect_open(bus,slot,0);
        } else if(ui.project.effect_type[bus][slot]==EFFECT_REVERB) {
            if(button("Open Reverb",fx+8,effect_button_y,MIXER_PANEL-16,24,0))mixer_effect_open(bus,slot,0);
        } else if(ui.project.effect_type[bus][slot]==EFFECT_TUBE) {
            if(button("Open Tube",fx+8,effect_button_y,MIXER_PANEL-16,24,0))mixer_effect_open(bus,slot,0);
        } else if(ui.project.effect_type[bus][slot]==EFFECT_DELAY) {
            if(button("Open Delay",fx+8,effect_button_y,MIXER_PANEL-16,24,0))mixer_effect_open(bus,slot,0);
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
void desktop_fx_drag_update(void) {
    Rect root=desktop_editor_rect(3);MixerView v=desktop_mixer_view(root.w,root.h);
    DrawPoint point={ui.mouse.x-root.x,ui.mouse.y-root.y};
    if(host_is_key_pressed(HOST_KEY_ESCAPE) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT) || ui.mixer_fx_hidden){ui.fx_press=ui.fx_dragging=0;return;}
    if(fabsf(ui.mouse.x-ui.fx_press_point.x)>4 || fabsf(ui.mouse.y-ui.fx_press_point.y)>4)ui.fx_dragging=1;
    int bus=ui.fx_bus;EffectChainList chain=effect_chain_list(&ui.project,bus);
    float top=mixer_fx_top(v,chain.count),list_height=mixer_fx_list_height(v,chain.count);
    if(ui.fx_dragging) {
        NavigationMotion motion=navigation_motion(ui.navigation_input,0,0,ui_scale());
        ui.mixer_fx_scroll[bus]=fmaxf(0,fminf(fmaxf(0,chain.count*22-list_height),ui.mixer_fx_scroll[bus]+motion.y));
    }
    ui.fx_drop=-1;
    if(mixer_fx_bus_at(v,point.x)==bus && point.y>=top && point.y<top+list_height && chain.count)
        ui.fx_drop=chain.slots[(int)fmaxf(0,fminf(chain.count-1,floorf((point.y-top+ui.mixer_fx_scroll[bus])/22)))];
    if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT)) {
        if(ui.fx_dragging && ui.fx_drop>=0 && effect_chain_move(&ui.project,bus,ui.fx_slot,ui.fx_drop)) {
            for(int id=5;id<WINDOW_CAPACITY;id++)if(ui.effect_windows[id].bus==bus)ui.effect_windows[id].slot=effect_slot_after_move(ui.effect_windows[id].slot,ui.fx_slot,ui.fx_drop);
            if(ui.eq_bus==bus){ui.eq_slot=effect_slot_after_move(ui.eq_slot,ui.fx_slot,ui.fx_drop);ui.effect_slot=effect_slot_after_move(ui.effect_slot,ui.fx_slot,ui.fx_drop);}
            effect_target_after_move(&ui.midi_learn_target,bus,ui.fx_slot,ui.fx_drop);
            effect_target_after_move(&ui.automation_target,bus,ui.fx_slot,ui.fx_drop);
            snprintf(ui.status,sizeof ui.status,"Effect moved to slot %d; the chain runs top to bottom.",ui.fx_drop+1);
        } else if(!ui.fx_dragging && ui.fx_drop==ui.fx_slot)mixer_effect_open(bus,ui.fx_slot,1);
        ui.fx_press=ui.fx_dragging=0;
    }
}
void desktop_mixer_scrollbar_update(void) {
    Rect r=desktop_editor_rect(3);MixerView v=desktop_mixer_view(r.w,r.h);
    ui.mixer_offset=mixer_scroll_from_thumb(v,ui.mouse.x-r.x,ui.mixer_scroll_grab);
    if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT))ui.mixer_pan=0;
}
static void studio_mixer_scale(MixerView v) {
    label("dBFS",3,v.fader_top-28,UI_TEXT_SIZE,muted);
    const int ticks[]={6,0,-6,-12,-24,-36,-48,-60};float positions[8];
    for(unsigned i=0;i<8;i++){
        positions[i]=v.fader_bottom-meter_height(powf(10,ticks[i]/20.f))*(v.fader_bottom-v.fader_top);
        host_draw_line(36,positions[i],41,positions[i],muted);
    }
    /* Full scale at the default height. When deliberately compacted, retain
       unity first, followed by the main attenuation marks, then extra detail. */
    const int priority[]={1,3,4,5,6,7,2,0};float drawn[8];int count=0;
    for(unsigned j=0;j<8;j++){
        int i=priority[j],fits=1;
        for(int n=0;n<count;n++)if(fabsf(positions[i]-drawn[n])<UI_TEXT_SIZE+2-.001f)fits=0;
        if(fits){label(host_text_format("%d",ticks[i]),3,positions[i]-8,UI_TEXT_SIZE,muted);drawn[count++]=positions[i];}
    }
}
/* Fixed selected-bus monitor; strip meters remain independent. */
static void studio_mixer_monitor(MixerView v) {
    meter_draw(ui.mixer_selected,48,v.fader_top,v.fader_bottom,10);
}
static void studio_mixer_strip(MixerView v,int bus) {
    int over=hover(0,v.fx_top,MIXER_STRIDE-4,v.height-v.fx_top-18);
    if(over && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT))ui.mixer_selected=bus;
    ui_panel(host_draw(),(DrawRect){0,v.fx_top,MIXER_STRIDE-4,v.height-v.fx_top-18},ui.mixer_selected==bus?(DrawColor){150,160,169,255}:cell);
    static const DrawColor colors[]={
        {0,216,200,255},{248,219,62,255},{100,201,239,255},{245,215,65,255},
        {226,110,219,255},{105,181,243,255},{92,225,132,255},{238,148,77,255}};
    if(v.fx_height) {
        EffectChainList chain=effect_chain_list(&ui.project,bus);
        int body_input=ui.input_enabled;
        float add_y=mixer_fx_adder_top(v,chain.count),top=mixer_fx_top(v,chain.count),list_height=mixer_fx_list_height(v,chain.count);
        int can_add=chain.add_slot>=0;ui.input_enabled=body_input && can_add;
        if(button_available("",2,add_y,104,21,0,can_add)){effect_slot_menu(bus,chain.add_slot);ui.fx_add_pending=1;ui.input_enabled=0;}
        icon(ICON_ADD,54,add_y+10,16,can_add?WHITE:host_fade(WHITE,.4f));
        int consumed=can_add && body_input && !ui.input_enabled;
        ui.input_enabled=body_input && !consumed;
        if(hover(2,add_y,104,21))snprintf(ui.status,sizeof ui.status,can_add?"%s: add an effect":"%s: all nine FX slots are occupied",mixer_name(bus));
        ui.input_enabled=body_input && !consumed && ui.mouse.y>=top && ui.mouse.y<top+list_height;
        int fx_input=ui.input_enabled,first=(int)(ui.mixer_fx_scroll[bus]/22);
        Rect root=desktop_editor_rect(3);float x=mixer_bus_x(v,bus),scale=ui_scale();
        float left=bus?fmaxf(v.insert_x,x):x,right=bus?fminf(v.insert_x+v.viewport,x+108):x+108;
        host_begin_scissor_mode((root.x+left)*scale,(root.y+top)*scale,(right-left)*scale,list_height*scale);
        for(int row=first;row<chain.count && row<first+v.fx_rows+1;row++) {
            int slot=chain.slots[row];float y=top+row*22-ui.mixer_fx_scroll[bus];
            int type=ui.project.effect_type[bus][slot];const char *name=type==EFFECT_EQ?"EQ":effect_descriptor(type)->name;
            if(button(fit_text(name,74,UI_TEXT_SIZE),2,y,82,21,mixer_effect_window(bus,slot)>=5)) {
                ui.fx_press=1;ui.fx_dragging=0;ui.fx_bus=bus;ui.fx_slot=slot;ui.fx_drop=slot;
                ui.fx_press_point=(DrawPoint){ui.mouse.x+root.x+x,ui.mouse.y+root.y};ui.input_enabled=0;
            }
            if(button(ui.project.effect_bypass[bus][slot]?"-":"o",86,y,18,21,!ui.project.effect_bypass[bus][slot]))ui.project.effect_bypass[bus][slot]^=1;
            if(hover(0,y,108,21)) {
                snprintf(ui.status,sizeof ui.status,"%s FX %d: %s | click to open; drag to reorder; right-click for options",mixer_name(bus),row+1,name);
                if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT))effect_slot_menu(bus,slot);
            }
        }
        if(ui.fx_press && ui.fx_dragging && ui.fx_bus==bus && ui.fx_drop>=0) {
            int row=0;while(row<chain.count && chain.slots[row]!=ui.fx_drop)row++;
            float y=top+(row+(ui.fx_drop>ui.fx_slot))*22-ui.mixer_fx_scroll[bus];
            host_draw_rectangle_rec((DrawRect){2,y-1,102,2},accent);
        }
        if(chain.count*22>list_height && list_height>0) {
            float thumb=list_height*list_height/(chain.count*22.f);
            host_draw_rectangle_rec((DrawRect){105,top+ui.mixer_fx_scroll[bus]/(chain.count*22-list_height)*(list_height-thumb),2,thumb},muted);
        }
        ui.input_enabled=body_input && !consumed && !(fx_input && !ui.input_enabled);
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
    knob_style(55,v.controls,bus?&ui.project.insert_pan[bus-1]:&ui.project.master_pan,-1,1,0,bus?"Insert pan":"Master pan",KNOB_PAN);
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
    ui_text_well(host_draw(),(DrawRect){20,v.height-49-v.cable_space,58,18},0);
    label(host_text_format("%.1f",gain_db(*volume)),24,v.height-48-v.cable_space,UI_TEXT_SIZE,ui_theme.signal);
    int input=hover(7,v.route,12,12);
    host_draw_rectangle_lines(9,v.route+2,8,8,input?ui_theme.signal:muted);
    if(input){
        snprintf(ui.status,sizeof ui.status,"%s input: drop outputs here; right-click to remove incoming cables",mixer_name(bus));
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)){ui.route_menu_scroll=0;open_context(27,bus,(DrawPoint){ui.mouse.x+desktop_editor_rect(3).x,ui.mouse.y+desktop_editor_rect(3).y});}
    }
    if(bus){
        int output=hover(29,v.route,12,12);
        circle(35,v.route+6,5,output || ui.cable_drag==bus?accent:muted);
        int connected=0;for(int dest=0;dest<=v.count;dest++)connected|=insert_has_send(&ui.project,bus,dest);
        circle(35,v.route+6,2,connected?accent:bg);
        if(output){snprintf(ui.status,sizeof ui.status,"Insert %d output: drag to add a destination; right-click to remove cables",bus);if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)){ui.route_menu_scroll=0;open_context(26,bus,(DrawPoint){ui.mouse.x+desktop_editor_rect(3).x,ui.mouse.y+desktop_editor_rect(3).y});}if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)){ui.cable_drag=bus;ui.mixer_selected=bus;ui.input_enabled=0;}}
    }
}
void mixer_add_inserts(int count) {
    int first=insert_add(&ui.project,count);
    if(!first){snprintf(ui.status,sizeof ui.status,"Cannot add inserts: limit is %d.",INSERTS);return;}
    ui.mixer_selected=first;
    Rect root=desktop_editor_rect(3);MixerView v=desktop_mixer_view(root.w,root.h);
    ui.mixer_offset=fmaxf(0,fminf(v.maximum,(first-1)*MIXER_STRIDE));
    snprintf(ui.status,sizeof ui.status,"Added %d mixer insert%s (%d / %d).",count,count==1?"":"s",ui.project.insert_count,INSERTS);
}
static void mixer_studio(float width,float height) {
    Rect root=desktop_editor_rect(3);float scale=ui_scale();
    MixerView v=desktop_mixer_view(width,height);
    ui.mixer_offset=v.offset;
    int shift=host_is_key_down(HOST_KEY_LEFT_SHIFT) || host_is_key_down(HOST_KEY_RIGHT_SHIFT);
    NavigationMotion motion=navigation_motion(ui.navigation_input,shift,0,scale);
    int hovered_bus=mixer_fx_bus_at(v,ui.mouse.x);float unused_scroll=0;
    if(!captured() && ui.input_enabled)ui.mixer_offset=mixer_scroll_motion(v,motion.x,motion.y,shift,ui.mouse.x,ui.mouse.y,hovered_bus>=0?&ui.mixer_fx_scroll[hovered_bus]:&unused_scroll);
    v.offset=ui.mixer_offset;
    if(button("FX",4,TITLE+4,60,22,!ui.mixer_fx_hidden)){ui.mixer_fx_hidden^=1;ui.input_enabled=0;v=desktop_mixer_view(width,height);}
    label(fit_text(mixer_name(ui.mixer_selected),112,UI_TEXT_SIZE),72,TITLE+7,UI_TEXT_SIZE,ink);
    device_button(0,196,TITLE+4,170);device_button(1,374,TITLE+4,170);
    int add_enabled=ui.input_enabled;ui.input_enabled=add_enabled && ui.project.insert_count<INSERTS;
    if(button_available("+",width-32,TITLE+4,24,22,0,ui.project.insert_count<INSERTS)){mixer_add_inserts(1);v=desktop_mixer_view(width,height);}
    if(hover(width-32,TITLE+4,24,22)){
        snprintf(ui.status,sizeof ui.status,"%d / %d inserts | Click to add one; right-click to add multiple",ui.project.insert_count,INSERTS);
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT))open_context(29,0,(DrawPoint){ui.mouse.x+root.x,ui.mouse.y+root.y});
    }
    ui.input_enabled=add_enabled;
    for(int bus=0;bus<=v.count;bus++){int count=effect_chain_list(&ui.project,bus).count;ui.mixer_fx_scroll[bus]=fmaxf(0,fminf(fmaxf(0,count*22-mixer_fx_list_height(v,count)),ui.mixer_fx_scroll[bus]));}
    studio_mixer_scale(v);
    studio_mixer_monitor(v);
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
        studio_mixer_strip(v,bus);
        if(!context && ui.context_opened)ui.context_position.x+=x;
        if(strip_enabled && !ui.input_enabled){enabled=0;}
    }
    ui.mouse=pointer;ui.text_origin=origin;ui.input_enabled=enabled;
    host_begin_mode2_d((HostCamera){.offset=origin,.zoom=scale});
    host_begin_scissor_mode(root.x*scale,root.y*scale,width*scale,height*scale);
    MixerCable wires[INSERTS+1];int wire_count=0;
    for(int source=1;source<=v.count;source++)if(mixer_port_visible(v,source,35)) {
        for(int dest=0;dest<=v.count;dest++)if((source==ui.mixer_selected || dest==ui.mixer_selected) && insert_has_send(&ui.project,source,dest) && mixer_port_visible(v,dest,13)) {
            wires[wire_count++]=(MixerCable){.source=source,.destination=dest,.start=mixer_bus_x(v,source)+35,.end=mixer_bus_x(v,dest)+13};
        }
    }
    int lanes=mixer_cable_lanes(wires,wire_count);
    /* Draw outlines first so neighboring lanes cannot erase each other's core. */
    for(int pass=0;pass<2;pass++)for(int i=0;i<wire_count;i++) {
        float x[4],y[4];mixer_cable_path(wires[i],lanes,v.route+6,height-20,x,y);
        DrawColor color=pass?host_fade(wires[i].source==ui.mixer_selected?ui_theme.signal:ui_theme.meter_mid,.95f):host_fade(bg,.9f);
        for(int segment=0;segment<3;segment++)host_draw_line_ex((DrawPoint){x[segment],y[segment]},(DrawPoint){x[segment+1],y[segment+1]},pass?1.25f:3,color);
    }
    if(ui.cable_drag>0 && mixer_port_visible(v,ui.cable_drag,35)) {
        float x[4],y[4];MixerCable preview={.start=mixer_bus_x(v,ui.cable_drag)+35,.end=pointer.x};
        mixer_cable_path(preview,1,v.route+6,height-20,x,y);y[3]=pointer.y;
        for(int segment=0;segment<3;segment++)host_draw_line_ex((DrawPoint){x[segment],y[segment]},(DrawPoint){x[segment+1],y[segment+1]},1.5f,accent);
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
