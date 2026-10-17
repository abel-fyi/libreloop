// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"
#include "rack.h"

static RackState desktop_rack_state(void) {
    return (RackState){ui.pattern,ui.rack_filter,ui.rack_scroll,ui.rack_paint,ui.rack_paint_channel,
        ui.rack_view[ui.pattern],ui.rack_paint_step,ui.piano_channels[ui.pattern],ui.studio_style?ui.rack_step_width[ui.pattern]:0};
}
float desktop_rack_step_width(void){RackState state=desktop_rack_state();return rack_step_width(&state);}
int desktop_rack_channels(int *rows){RackState state=desktop_rack_state();return rack_channels(&state,&ui.project,rows);}
int desktop_rack_channel_at(int row){RackState state=desktop_rack_state();return rack_channel_at(&state,&ui.project,row);}

void channel_route(int c,int x,int y) {
    if(drag_button(host_text_format("%d",ui.project.route[c]),x,y,24,22,ui.route_drag==c)) {
        ui.route_drag=c; ui.route_start=ui.project.route[c]; ui.route_y=ui.mouse.y+desktop_editor_rect(ui.knob_context).y;
        ui.channel=c; ui.mixer_selected=ui.project.route[c]; ui.input_enabled=0;
    }
    if(hover(x,y,24,22)) snprintf(ui.status,sizeof ui.status,"%s mixer destination: drag up/down; 0 = Master",ui.project.channel_names[c]);
}

void channel_controls(int c,float width) {
    int x=width-276;
    mute_light(x,36,ui.project.mute,ui.project.channel_count,c,ui.project.channel_names[c]);
    knob_style(x+28,36,&ui.project.pan[c],-1,1,0,"Channel pan",KNOB_PAN);
    knob_style(x+58,36,&ui.project.volume[c],0,VOLUME_KNOB_MAX,1,"Channel volume",KNOB_VOLUME);
    knob_style(x+94,36,&ui.project.channel_pitch[c],-1,1,0,"Playback pitch",KNOB_CENTER);
    if(hover(x+83,25,22,22) || ui.control_drag==&ui.project.channel_pitch[c]) snprintf(ui.status,sizeof ui.status,"Playback pitch: %+.2f semitones | range ±%.0f semitones",ui.project.channel_pitch[c]*ui.project.pitch_range[c],ui.project.pitch_range[c]);
    float *range=&ui.project.pitch_range[c];
    if(drag_button(host_text_format("%.0f",*range),x+150,25,30,22,ui.control_drag==range)) { capture_control(range,1,48,0); ui.control_integer=1; }
    if(hover(x+150,25,30,22)) {
        *range=fmaxf(1,fminf(48,*range+roundf(host_get_mouse_wheel_move())));
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) { control_menu(range,1,48,2,"Pitch range (semitones)"); }
        snprintf(ui.status,sizeof ui.status,"Pitch range: %.0f semitones | drag up/down, 1–48 whole semitones; right-click to enter",*range);
    }
    channel_route(c,x+205,25);
    label("PAN",x+18,53,10,muted); label("VOL",x+48,53,10,muted);
    label("PITCH",x+80,53,10,muted); label("RANGE",x+150,53,10,muted); label("ROUTE",x+196,53,10,muted);
}

double pattern_playback_position(void) {
    if(!ui.playing) return -1;
    if(!ui.song) return ui.visual_step;
    double latest=-1,position=-1; int solo=solo_any(ui.project.lane_mute,LANES);
    for(int l=0;l<LANES;l++) if(!(ui.project.lane_mute[l]&1) && (!solo || (ui.project.lane_mute[l]&2)))
        for(int b=0;b<CLIPS;b++) if(ui.project.clips[l][b]==ui.pattern+1) {
            double start=ui.project.clip_starts[l][b]*STEPS,local=ui.visual_step-start;
            if(start>latest && local>=0 && local<clip_length(&ui.project,l,b) && local+ui.project.clip_offsets[l][b]<ui.project.pattern_steps[ui.pattern]) { latest=start; position=local+ui.project.clip_offsets[l][b]; }
        }
    return position;
}

float edit_steps(void) { return arrangement_edit_steps(&ui.arrangement,&ui.project,ui.pattern); }

void pattern_extend(float end) {
    if(end<=ui.project.pattern_steps[ui.pattern]) return;
    ui.project.pattern_steps[ui.pattern]=ceilf(end/STEPS)*STEPS;
    if(ui.arrangement.source_pattern==ui.pattern) ui.arrangement.source_steps=ui.project.pattern_steps[ui.pattern];
}

int desktop_rack_melodic(int c){RackState state=desktop_rack_state();return rack_melodic(&state,&ui.project,c);}
void paint_rack(float width,float x,float y) {
    float before=ui.project.pattern_steps[ui.pattern];RackState state=desktop_rack_state();rack_paint(&state,&ui.project,width,ui.windows.editors[0].rect.h,x,y);
    ui.rack_paint_channel=state.paint_channel;ui.rack_paint_step=state.paint_step;
    if(ui.project.pattern_steps[ui.pattern]!=before && ui.arrangement.source_pattern==ui.pattern)ui.arrangement.source_steps=ui.project.pattern_steps[ui.pattern];
}

void rack(float width,float height) {
    float stepw=desktop_rack_step_width(),gridw=width-226,view=ui.rack_view[ui.pattern],span=gridw/stepw;
    ui.rack_range[ui.pattern]=fmaxf(ui.rack_range[ui.pattern],fmaxf(view+span*2,ui.project.pattern_steps[ui.pattern]+span));
    timeline_ruler(0,view,span,200,TITLE,gridw,1);
    int visible=fmaxf(1,(height-RACK_TOP-48)/28),rows[CHANNELS],count=desktop_rack_channels(rows);
    if(!ui.studio_style && hover(0,RACK_TOP,width,height-RACK_TOP-18)) ui.rack_scroll-=host_get_mouse_wheel_move();
    ui.rack_scroll=fmaxf(0,fminf(fmaxf(0,count-visible),ui.rack_scroll));
    for(int index=ui.rack_scroll;index<count && index<ui.rack_scroll+visible;index++) {
        int c=rows[index],row=RACK_TOP+(index-ui.rack_scroll)*28;
        mute_light(12,row+11,ui.project.mute,ui.project.channel_count,c,ui.project.channel_names[c]);
        knob_style(36,row+11,&ui.project.pan[c],-1,1,0,"Channel pan",KNOB_PAN); knob_style(62,row+11,&ui.project.volume[c],0,VOLUME_KNOB_MAX,1,"Channel volume",KNOB_VOLUME);
        channel_route(c,78,row);
        if(button_color(fit_text(channel_caption(c),68,13),106,row,78,22,0,ui.project.channel_audio[c]?audio_color(c):cell)) {
            ui.channel=ui.instrument_channel=c; windows_focus(&ui.windows,4);
        }
        if(hover(106,row,78,22) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) { ui.channel=c; open_context(2,c,(DrawPoint){ui.mouse.x+ui.windows.editors[0].rect.x,ui.mouse.y+ui.windows.editors[0].rect.y}); }
        if(hover(106,row,78,22)) snprintf(ui.status,sizeof ui.status,"%s: open instrument; right-click for Piano Roll, rename, mute or delete; Keys: Z/Q white, S/2 black",ui.project.channel_names[c]);
        if(ui.sample_drag[0] && ui.sample_moved && ui.mouse.y>=row && ui.mouse.y<row+28 && ui.mouse.x>=0 && ui.mouse.x<width && windows_hit(&ui.windows,ui.mouse.x+ui.windows.editors[0].rect.x,ui.mouse.y+ui.windows.editors[0].rect.y)==0) {
            host_draw_rectangle_lines_ex((DrawRect){106,row,78,22},1,ui_theme.signal);
            snprintf(ui.status,sizeof ui.status,"Release to replace %s's sample",ui.project.channel_names[c]);
        }
        float brightness=fmaxf(ui.channel_active_ui[c]?.32f:0,ui.channel_flash[c]);
        host_draw_rectangle(187,row,3,22,host_fade(WHITE,.08f+brightness*.92f));
        int melodic=desktop_rack_melodic(c);
        if(melodic) {
            host_draw_rectangle(200,row+2,gridw,20,bg);
            float active=fmaxf(0,fminf(gridw,(edit_steps()-view)*stepw));
            if(active<gridw) host_draw_rectangle(200+active,row+2,gridw-active,20,ui_theme.disabled);
            for(int i=0;i<NOTES;i++) {
                Note n=ui.project.notes[ui.pattern][c][i]; float start=200+(n.start-view)*stepw,end=start+fmaxf(2,(n.length?n.length:1)*stepw-2);
                if(n.velocity && end>=200 && start<200+gridw) host_draw_rectangle(fmaxf(200,start),row+3+fmaxf(0,fminf(16,(72-(int)n.pitch)*.65f)),fminf(200+gridw,end)-fmaxf(200,start),2,ui.playing_notes[c][i]?ink:accent);
            }
            if(hover(200,row+2,gridw,20)) snprintf(ui.status,sizeof ui.status,"%s notes: click to open Piano Roll; right-click for channel options",ui.project.channel_names[c]);
            if(hover(200,row+2,gridw,20)) {
                host_set_mouse_cursor(HOST_MOUSE_CURSOR_POINTING_HAND);
                if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) { ui.channel=ui.piano_channel=c; ui.piano_channels[ui.pattern][c]=1; ui.browser_focus=0; windows_focus(&ui.windows,2); ui.input_enabled=0; }
            }
            if(hover(200,row+2,gridw,20) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) { ui.channel=c; open_context(2,c,(DrawPoint){ui.mouse.x+ui.windows.editors[0].rect.x,ui.mouse.y+ui.windows.editors[0].rect.y}); }
            continue;
        }
        for(int i=0;i<span+2;i++) {
            float step=floorf(view)+i,x=200+(step-view)*stepw,left=fmaxf(200,x),right=fminf(200+gridw,x+stepw-3);
            if(right<=left) continue;
            Note *n=note_at(&ui.project,ui.pattern,c,step,60); int available=step<edit_steps();
            ui_surface(host_draw(),(DrawRect){left+1,row+2,fmaxf(1,right-left-2),18},!available?ui_theme.disabled:n?ui_theme.step_on[(int)floorf(step/4)%2]:fmodf(floorf(step/4),2)?ui_theme.step_alt:cell);
            if(n && ui.playing_notes[c][n-ui.project.notes[ui.pattern][c]]) host_draw_rectangle_lines_ex((DrawRect){left+1,row+2,fmaxf(1,right-left-2),18},1,clip_foreground(ui_theme.step_on[(int)floorf(step/4)%2]));
            if(fmodf(step,STEPS)==0 && x>=200) host_draw_line(x,row,x,row+22,muted);
            if(hover(left,row+2,right-left,18)) {
                snprintf(ui.status,sizeof ui.status,"%s step %.0f: left paints%s, right erases",ui.project.channel_names[c],step+1,available?"":" and extends the pattern");
                if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) {
                    ui.rack_paint=host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)?HOST_MOUSE_BUTTON_LEFT:HOST_MOUSE_BUTTON_RIGHT;
                    ui.rack_paint_channel=-1; paint_rack(width,ui.mouse.x,ui.mouse.y); ui.input_enabled=0;
                }
            }
        }
    }
    double position=pattern_playback_position();
    if(position>=0) {
        float x=200+(position-view)*stepw;
        if(x>=200 && x<=200+gridw)
            host_draw_line_ex((DrawPoint){x,RACK_TOP-4},(DrawPoint){x,RACK_TOP+fminf(visible,count-ui.rack_scroll)*28-6},1.5f,ink);
    }
    int add_y=RACK_TOP+(int)fminf(visible,count-ui.rack_scroll)*28;
    if(add_y+22<=height-18) {
        int dropping=ui.sample_drag[0] && ui.sample_moved && ui.mouse.y>=add_y && ui.mouse.y<height-18 && ui.mouse.x>=0 && ui.mouse.x<width && windows_hit(&ui.windows,ui.mouse.x+ui.windows.editors[0].rect.x,ui.mouse.y+ui.windows.editors[0].rect.y)==0;
        if(button("+",106,add_y,78,22,0)) open_popup(6);
        if(dropping) {
            ui_surface(host_draw(),(DrawRect){106,add_y,78,22},host_fade(ui_theme.signal,.18f));
            host_draw_rectangle_lines_ex((DrawRect){106,add_y,78,22},1,ui_theme.signal);
            snprintf(ui.status,sizeof ui.status,"Release to add a new sample channel");
        } else if(hover(106,add_y,78,22)) snprintf(ui.status,sizeof ui.status,"Add instrument: choose a plugin, or drop a sample here");
    }
    float area=visible*28,maximum=fmaxf(0,count-visible),thumb=fminf(area,fmaxf(24,area*visible/fmaxf(1,count))),travel=area-thumb;
    float y=RACK_TOP+(maximum?ui.rack_scroll/maximum*travel:0);
    if(maximum>0) {
        host_draw_rectangle(width-12,RACK_TOP,8,area,bg); ui_surface(host_draw(),(DrawRect){width-12,y,8,thumb},ui.rack_vdrag || hover(width-14,RACK_TOP,14,area)?accent:muted);
        if(hover(width-14,RACK_TOP,14,area)) {
            snprintf(ui.status,sizeof ui.status,"Drag to scroll Rack channels; wheel over rows also scrolls");
            if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) {
                if(ui.mouse.y<y || ui.mouse.y>y+thumb) ui.rack_scroll=fmaxf(0,fminf(maximum,(ui.mouse.y-RACK_TOP-thumb/2)/fmaxf(1,travel)*maximum));
                ui.rack_vy=ui.mouse.y+ui.windows.editors[0].rect.y; ui.rack_vstart=ui.rack_scroll; ui.rack_vscale=maximum/fmaxf(1,travel); ui.rack_vdrag=1; ui.rack_scroll_remainder=0; ui.input_enabled=0;
            }
        }
    }
    if(ui.studio_style) {
        if(button("-",106,height-22,24,18,0))rack_zoom(&ui.rack_step_width[ui.pattern],&ui.rack_view[ui.pattern],gridw,-2,.5f);
        if(button("+",132,height-22,24,18,0))rack_zoom(&ui.rack_step_width[ui.pattern],&ui.rack_view[ui.pattern],gridw,2,.5f);
        if(button(host_text_format("%.0f%%",stepw/16*100),158,height-22,38,18,0)) {
            float center=view+span*.5f;ui.rack_step_width[ui.pattern]=16;
            ui.rack_view[ui.pattern]=fmaxf(0,center-gridw/16*.5f);
        }
        if(hover(106,height-22,90,18))snprintf(ui.status,sizeof ui.status,"Rack zoom: - / +; percentage resets to 100%% | Ctrl/Cmd + wheel or pinch over steps; Shift + wheel pans");
    }
    float total=ui.rack_range[ui.pattern],hthumb=timeline_thumb(gridw,span,total),htravel=gridw-hthumb,hmaximum=total-span;
    float hx=200+(hmaximum>0?ui.rack_view[ui.pattern]/hmaximum*htravel:0);
    host_draw_rectangle(200,height-16,gridw,8,bg); ui_surface(host_draw(),(DrawRect){hx,height-16,hthumb,8},ui.rack_hdrag || hover(200,height-19,gridw,14)?accent:muted);
    if(hover(200,height-19,gridw,14)) {
        snprintf(ui.status,sizeof ui.status,"Drag to scroll steps horizontally; grey steps extend the pattern when painted");
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) {
            if(ui.mouse.x<hx || ui.mouse.x>hx+hthumb) ui.rack_view[ui.pattern]=fmaxf(0,fminf(hmaximum,(ui.mouse.x-200-hthumb/2)/fmaxf(1,htravel)*hmaximum));
            ui.rack_hx=ui.mouse.x+ui.windows.editors[0].rect.x; ui.rack_hstart=ui.rack_view[ui.pattern]; ui.rack_hscale=hmaximum/fmaxf(1,htravel); ui.rack_hdrag=1; ui.input_enabled=0;
        }
    }
}

DrawColor pattern_rgb(uint32_t rgb) {
    return source_rgb(rgb);
}

DrawColor pattern_color(int id) { return pattern_rgb(ui.project.pattern_colors[id]); }

void pattern_pitch_range(int pat,int *low,int *high) {
    *low=127; *high=0;
    for(int c=0;c<ui.project.channel_count;c++) for(int n=0;n<NOTES;n++) {
        Note note=ui.project.notes[pat][c][n]; if(!note.velocity || note.start>=ui.project.pattern_steps[pat]) continue;
        *low=fminf(*low,note.pitch); *high=fmaxf(*high,note.pitch);
    }
}

int desktop_rack_fill(int pattern,int channel,int interval) {
    int count=rack_fill_steps(&ui.project,pattern,channel,interval,0);
    if(count<0){snprintf(ui.status,sizeof ui.status,"Fill cancelled: not enough note slots for the entire fill.");return 0;}
    if(!count){snprintf(ui.status,sizeof ui.status,"Those steps are already filled.");return 0;}
    if(!history_checkpoint())return 0;
    rack_fill_steps(&ui.project,pattern,channel,interval,1);
    snprintf(ui.status,sizeof ui.status,"Added %d notes, every %d steps. Existing notes kept.",count,interval);
    return count;
}
