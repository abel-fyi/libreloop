// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"
#include "rack.h"

void desktop_velocity_resize(void) {
    if(!ui.studio_style)return;
    Rect r=desktop_piano_rect();float top=r.y+r.h-22-desktop_velocity_height();
    int over=ui.windows.editors[2].visible && windows_hit(&ui.windows,ui.mouse.x,ui.mouse.y)==2 &&
        ui.mouse.x>=r.x+4 && ui.mouse.x<r.x+r.w-24 && fabsf(ui.mouse.y-(top-5))<=4;
    int blocked=ui.pattern_popup || ui.context_kind || ui.sample_drag[0] || !host_is_window_focused();
    int press=over && !blocked && !captured() && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT);
    if(ui.velocity_lane.dragging || (over && !blocked && !captured())) {
        host_set_mouse_cursor(HOST_MOUSE_CURSOR_RESIZE_NS);
        snprintf(ui.status,sizeof ui.status,ui.velocity_lane.collapsed?"Velocity collapsed: click or drag up to restore":"Drag to resize velocity; drag down past minimum to collapse");
    }
    velocity_lane_update(&ui.velocity_lane,r.h,ui.mouse.y,press,host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT),
        blocked || !ui.windows.editors[2].visible || host_is_key_pressed(HOST_KEY_ESCAPE));
}

float piano_visible_rows(void) { return piano_rows_for_area(desktop_piano_grid_rect().h-PIANO_GRID_PADDING,ui.piano_zoom,desktop_piano_reference_area()); }

int piano_min_top(void) { return (int)fminf(128,ceilf(piano_visible_rows()))-1; }

void edit_selection(int action) {
    if(ui.midi_take.active){snprintf(ui.status,sizeof ui.status,"Finish the MIDI take before editing notes or clips");return;}
    int id=ui.windows.focused;
    if((id!=1 && id!=2) || !ui.windows.editors[id].visible || ui.browser_focus || captured()) { snprintf(ui.status,sizeof ui.status,"Focus the Piano Roll or Arrangement to edit its selection"); return; }
    if(action==3) {
        if(id==2) for(int i=0;i<NOTES;i++) ui.note_selected[ui.pattern][ui.piano_channel][i]=!!ui.project.notes[ui.pattern][ui.piano_channel][i].velocity;
        else for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) ui.arrangement.selected[l][b]=!!ui.project.clips[l][b];
        snprintf(ui.status,sizeof ui.status,"Selected all %s",id==2?"notes":"clips"); return;
    }
    int result;
    if(action<2) {
        result=id==2?clipboard_copy_notes(&ui.edit_clipboard,&ui.project,ui.pattern,ui.piano_channel,ui.note_selected[ui.pattern][ui.piano_channel]):clipboard_copy_clips(&ui.edit_clipboard,&ui.project,(const uint8_t (*)[CLIPS])ui.arrangement.selected);
        if(result && action==0) {
            if(!history_checkpoint()) return;
            if(id==2) { for(int i=0;i<NOTES;i++) if(ui.note_selected[ui.pattern][ui.piano_channel][i]) ui.project.notes[ui.pattern][ui.piano_channel][i].velocity=0; memset(ui.note_selected[ui.pattern][ui.piano_channel],0,NOTES); }
            else { for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(ui.arrangement.selected[l][b]) { ui.project.clips[l][b]=0; ui.project.clip_steps[l][b]=0; } memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected); }
        }
        snprintf(ui.status,sizeof ui.status,result?"%s %d %s":"Nothing selected",action==0?"Cut":"Copied",result,id==2?"notes":"clips"); return;
    }
    if(!history_checkpoint()) return;
    result=id==2?clipboard_paste_notes(&ui.edit_clipboard,&ui.project,ui.pattern,ui.piano_channel,ui.piano_start[ui.pattern],ui.note_selected[ui.pattern][ui.piano_channel]):clipboard_paste_clips(&ui.edit_clipboard,&ui.project,ui.selected_track>=0?ui.selected_track:ui.edit_clipboard.lane,ui.playlist_start,ui.arrangement.selected);
    if(result>0) { if(id==2) { ui.piano_channels[ui.pattern][ui.piano_channel]=1; if(ui.arrangement.source_pattern==ui.pattern) ui.arrangement.source_steps=ui.project.pattern_steps[ui.pattern]; } snprintf(ui.status,sizeof ui.status,"Pasted %d %s at the start marker",result,id==2?"notes":"clips"); }
    else snprintf(ui.status,sizeof ui.status,"%s",result==-1?"Clip sources changed; copy the selection again":result==-2?"No room for the entire selection":result==-3?"Paste position is occupied; move the start marker to empty space":result==-4?"Selection does not fit at this destination":"Copy a selection from this editor first");
}

void piano_shift(float dx,int dy,const char *action) {
    if(ui.windows.focused!=2 || !ui.windows.editors[2].visible || captured() || ui.browser_focus || ui.midi_take.active) return;
    uint8_t *selected=ui.note_selected[ui.pattern][ui.piano_channel]; int count=0,low=127,high=0;
    float end=edit_steps();
    for(int i=0;i<NOTES;i++) if(selected[i] && ui.project.notes[ui.pattern][ui.piano_channel][i].velocity) {
        Note n=ui.project.notes[ui.pattern][ui.piano_channel][i]; int pitch=n.pitch+dy; count++;
        low=fminf(low,pitch); high=fmaxf(high,pitch);
        if(pitch<0 || pitch>127 || n.start+dx<0) { snprintf(ui.status,sizeof ui.status,"Move stopped at the note or timeline limit"); return; }
        end=fmaxf(end,n.start+dx+(n.length?n.length:1));
    }
    if(!count) { snprintf(ui.status,sizeof ui.status,"Select notes to move"); return; }
    if(!history_checkpoint()) return;
    Note before[NOTES]; memcpy(before,ui.project.notes[ui.pattern][ui.piano_channel],sizeof before);
    float previous=ui.project.pattern_steps[ui.pattern];
    ui.project.pattern_steps[ui.pattern]=end>previous?ceilf(end/STEPS)*STEPS:previous;
    if(!notes_move(&ui.project,ui.pattern,ui.piano_channel,before,selected,dx,dy,ui.project.pattern_steps[ui.pattern])) {
        ui.project.pattern_steps[ui.pattern]=previous;
        snprintf(ui.status,sizeof ui.status,"Move stopped at the note or timeline limit"); return;
    }
    if(ui.arrangement.source_pattern==ui.pattern) ui.arrangement.source_steps=ui.project.pattern_steps[ui.pattern];
    if(dy) {
        if(high>ui.piano_top) ui.piano_top=high;
        else if(low<ui.piano_top-piano_min_top()) ui.piano_top=low+piano_min_top();
        ui.piano_top=fmaxf(piano_min_top(),fminf(127,ui.piano_top));
    }
    snprintf(ui.status,sizeof ui.status,"Moved %d notes %s",count,action);
}

void piano_octave(int direction) {
    if(ui.piano_tool==PENCIL) piano_shift(0,direction*12,direction>0?"up an octave":"down an octave");
}

int desktop_piano_inactive(float step) { float end=edit_steps(); return step>=end && step<ceilf(end/STEPS)*STEPS; }

void piano_extend(float end) { pattern_extend(end); }

static PianoViewport desktop_piano_view(void) {
    Rect r=desktop_piano_grid_rect();
    return (PianoViewport){r.w,r.h,ui.piano_span[ui.pattern],ui.piano_pan[ui.pattern],ui.piano_zoom,ui.piano_top,desktop_piano_reference_area(),ui.piano_grid};
}
static PianoGesture desktop_piano_state(void) {
    return (PianoGesture){.kind=ui.piano_gesture,.pattern=ui.pattern,.channel=ui.piano_channel,
        .additive=ui.piano_additive,.moving=ui.moving_note,.melodic=ui.piano_channels[ui.pattern][ui.piano_channel],
        .from={ui.piano_from.x,ui.piano_from.y},.now={ui.piano_now.x,ui.piano_now.y},
        .grab={ui.note_grab.x,ui.note_grab.y},.length=ui.piano_note_length,.drag=ui.note_drag,
        .before=ui.note_before,.selected=ui.note_selected[ui.pattern][ui.piano_channel],
        .selection_before=ui.note_selection_before};
}
static void desktop_piano_state_commit(PianoGesture g,float before) {
    ui.piano_gesture=g.kind;ui.piano_additive=g.additive;ui.moving_note=g.moving;
    ui.piano_channels[g.pattern][g.channel]=g.melodic;
    ui.piano_from=(DrawPoint){g.from.x,g.from.y};ui.piano_now=(DrawPoint){g.now.x,g.now.y};
    ui.note_grab=(DrawPoint){g.grab.x,g.grab.y};ui.piano_note_length=g.length;ui.note_drag=g.drag;
    if(ui.project.pattern_steps[ui.pattern]!=before && ui.arrangement.source_pattern==ui.pattern)ui.arrangement.source_steps=ui.project.pattern_steps[ui.pattern];
}
static void desktop_note_preview(void *context,int channel,Note note) {(void)context;audio_note(channel,note);}
static void desktop_key_preview(void *context,int slot,int channel,int pitch,int down) {(void)context;audio_key(slot,channel,pitch,down);}
void desktop_piano_gesture_update(float step,float pitch,float q) {
    float before=ui.project.pattern_steps[ui.pattern];PianoGesture g=desktop_piano_state();piano_gesture_update(&g,&ui.project,step,pitch,q,desktop_note_preview,NULL);desktop_piano_state_commit(g,before);
}
void desktop_piano_note_drag_update(float step,float row,float q) {
    float before=ui.project.pattern_steps[ui.pattern];PianoGesture g=desktop_piano_state();piano_note_drag_update(&g,&ui.project,step,row,q,desktop_note_preview,NULL);desktop_piano_state_commit(g,before);
}

int piano_black_key(int pitch) {
    int pc=pitch%12; return pc==1 || pc==3 || pc==6 || pc==8 || pc==10;
}

DrawRect piano_key_rect(int pitch,float rh) {
    float center=PIANO_GRID_TOP+(ui.piano_top-pitch+.5f)*rh;
    if(piano_black_key(pitch)) return (DrawRect){4,center-rh*.5f,34,rh};
    float top=center-rh*(piano_black_key(pitch+1)?1:.5f);
    float bottom=center+rh*(piano_black_key(pitch-1)?1:.5f);
    top=fmaxf(PIANO_GRID_TOP,top); bottom=fminf(PIANO_GRID_TOP+rh*piano_visible_rows(),bottom);
    return (DrawRect){4,top,54,fmaxf(0,bottom-top)};
}

int piano_key_at(float x,float y,float rh) {
    if(x<4 || x>=58 || y<PIANO_GRID_TOP || y>=PIANO_GRID_TOP+rh*piano_visible_rows()) return -1;
    float row=(y-PIANO_GRID_TOP)/rh; int pitch=ui.piano_top-(int)floorf(row);
    if(piano_black_key(pitch) && x>=38) pitch+=row-floorf(row)<.5f?1:-1;
    return pitch>=0 && pitch<=127?pitch:-1;
}

void desktop_piano_key_update(float x,float y,int cancel) {
    PianoViewport v=desktop_piano_view();PianoKeys k={ui.piano_key,ui.piano_key_channel,ui.piano_key_drag};
    piano_key_update(&k,&v,(PianoPoint){x,y},cancel,desktop_key_preview,NULL);
    ui.piano_key=k.pitch;ui.piano_key_channel=k.channel;ui.piano_key_drag=k.drag;
}

void piano(float width,float height) {
    int channel=ui.piano_channel;
    int gx=60,gy=PIANO_GRID_TOP; float gridw=width-84,rh=(height-PIANO_GRID_PADDING-(desktop_velocity_height()-62))/piano_visible_rows();
    float extent=ui.piano_span[ui.pattern];
    Rect r=desktop_piano_rect(); float scale=ui_scale();
    const char *tips[]={"Pencil: draw one note; drag note bodies to move; edges to resize","Brush: drag to paint notes on the visible grid","Select: drag a rectangle; drag selected notes together; Shift adds; Delete deletes"};
    if(!ui.studio_style)for(int i=0;i<3;i++) if(tool_button(i,ui.piano_tool,8+i*24,tips[i])) ui.piano_tool=i;
    if(!ui.studio_style)label(fit_text(host_text_format("%s / %s",ui.project.channel_names[channel],ui.project.pattern_names[ui.pattern]),width-158,12),88,30,12,muted);
    if(ui.windows.workspace.enabled) {
        for(int i=0;i<3;i++)if(tool_button(i,ui.piano_tool,8+i*24,tips[i]))ui.piano_tool=i;
        grid_selector(2,88,26,110,22);
        int hits=ui.pattern_hits[channel];
        if(button(hits?"Hits: On":"Hits: Off",206,26,104,22,hits)) {
            ui.pattern_hits[channel]^=1;
            if(ui.pattern_hits[channel] && (60>ui.piano_top || 60<ui.piano_top-piano_visible_rows()+1))ui.piano_top=fminf(127,fmaxf(piano_min_top(),60+piano_visible_rows()*.5f));
        }
        if(hover(206,26,104,22))snprintf(ui.status,sizeof ui.status,"Drum hits: left-drag writes C4 one-shots on whole steps; right-drag erases C4; other notes are kept");
        if(button("Fill X...",318,26,104,22,0))begin_rack_fill(channel);
    }
    int grid_over=hover(gx,gy,gridw,rh*piano_visible_rows());
    extent=ui.piano_span[ui.pattern];
    double playback=pattern_playback_position();
    if(playback>=0) follow_view(&ui.piano_pan[ui.pattern],extent,playback);
    float cw=gridw/extent,view=ui.piano_pan[ui.pattern],q=grid_choice_interval(ui.piano_grid,cw),end=edit_steps();
    ui.piano_range[ui.pattern]=fmaxf(ui.piano_range[ui.pattern],fmaxf(view+extent*2,end+extent));
    float partial=ceilf(end/STEPS)*STEPS;
    if(hover(4,gy,gx-6,rh*piano_visible_rows())) {
        snprintf(ui.status,sizeof ui.status,"Piano keys: click to play; hold and drag up/down to audition pitches");
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) {
            ui.piano_key_drag=1; ui.piano_key_channel=channel; desktop_piano_key_update(ui.mouse.x,ui.mouse.y,0); ui.input_enabled=0;
        }
    }
    host_draw_rectangle_rec((DrawRect){4,gy,gx-6,rh*piano_visible_rows()},ui_theme.piano_white);
    host_begin_scissor_mode((r.x+4)*scale,(r.y+gy)*scale,54*scale,(height-PIANO_GRID_PADDING-(desktop_velocity_height()-62))*scale);
    int hovered_key=ui.input_enabled?piano_key_at(ui.mouse.x,ui.mouse.y,rh):-1;
    /* Draw full white keys first; black keys cover their overlapping left ends. */
    for(int black=0;black<2;black++) for(int row=-1;row<=(int)ceilf(piano_visible_rows());row++) {
        int pitch=ui.piano_top-row;
        if(pitch<0 || pitch>127 || piano_black_key(pitch)!=black) continue;
        DrawRect key=piano_key_rect(pitch,rh);
        float bottom=fminf(gy+rh*piano_visible_rows(),key.y+key.height);
        key.y=fmaxf(gy,key.y); key.height=fmaxf(0,bottom-key.y);
        if(key.height<=0) continue;
        int active=ui.keyboard_notes[channel][pitch] || ui.playing_keys[channel][pitch] || (ui.piano_key==pitch && ui.piano_key_channel==channel);
        DrawColor base=black?ui_theme.piano_black:pitch%12==0?ui_theme.piano_c:ui_theme.piano_white;
        host_draw_rectangle_rec(key,active?accent:base);
        if(ui.studio_style && !ui.profile_flat && !active && key.height>3) {
            if(!black)ui_gradient(host_draw(),key,base,.12f,1);
            host_draw_line_ex((DrawPoint){key.x+1,key.y+1},(DrawPoint){key.x+key.width-1,key.y+1},1,black?(DrawColor){67,80,90,255}:(DrawColor){232,237,240,255});
            host_draw_line_ex((DrawPoint){key.x+key.width-1,key.y+1},(DrawPoint){key.x+key.width-1,key.y+key.height-1},1,black?(DrawColor){0,0,0,255}:(DrawColor){111,124,133,255});
        }
        if(!active && hovered_key==pitch) host_draw_rectangle_rec(key,host_fade(accent,.22f));
        if(!black) host_draw_line_ex((DrawPoint){key.x,key.y+key.height},(DrawPoint){key.x+key.width,key.y+key.height},1,host_fade(ui_theme.piano_black,.15f));
        if(!ui.studio_style && pitch%12==0) {
            const char *name=host_text_format("C%d",pitch/12-1);
            label(name,gx-5-text_width(name,10),fmaxf(gy,fminf(gy+rh*piano_visible_rows()-10,gy+(row+.5f)*rh-5)),10,active?ui_theme.selected_text:ui_theme.piano_black);
        }
    }
    if(ui.studio_style)for(int pitch=0;pitch<=120;pitch+=12) {
        DrawRect key=piano_key_rect(pitch,rh);
        float top=fmaxf(gy,key.y),bottom=fminf(gy+rh*piano_visible_rows(),key.y+key.height);
        if(bottom-top>1) {
            /* Keep octave ink on the white-key tip, beyond the black keys. */
            DrawRect bounds={39,top+1,18,fmaxf(0,bottom-top-2)};
            label_key(host_text_format("C%d",pitch/12-1),bounds,ui_theme.piano_black);
        }
    }
    host_begin_scissor_mode((r.x+gx)*scale,(r.y+gy)*scale,gridw*scale,rh*piano_visible_rows()*scale);
    for(int row=0;row<(int)ceilf(piano_visible_rows());row++) {
        int pc=(ui.piano_top-row)%12,black=pc==1 || pc==3 || pc==6 || pc==8 || pc==10;
        float y=gy+row*rh;
        host_draw_rectangle_rec((DrawRect){gx,y,gridw,rh},ui_theme.piano_row[black]);
        int pitch=ui.piano_top-row;
        if(pitch>=0 && pitch<=127 && (ui.keyboard_notes[channel][pitch] || ui.playing_keys[channel][pitch] ||
           (ui.piano_key==pitch && ui.piano_key_channel==channel)))
            host_draw_rectangle_rec((DrawRect){gx,y,gridw,rh},host_fade(accent,.15f));
        host_draw_line_ex((DrawPoint){gx,y},(DrawPoint){gx+gridw,y},1,host_fade(ui_theme.grid_minor,.45f));
        if(partial>end) { float left=fmaxf(gx,gx+(end-view)*cw),right=fminf(gx+gridw,gx+(partial-view)*cw); if(right>left) host_draw_rectangle_rec((DrawRect){left,y,right-left,rh-1},ui_theme.disabled); }
        if(pc==0) host_draw_line_ex((DrawPoint){gx,y+rh},(DrawPoint){gx+gridw,y+rh},1,ui_theme.grid_major);
    }
    if(ui.studio_style)ui_graph_dots(host_draw(),(DrawRect){gx,gy,gridw,rh*piano_visible_rows()});
    timeline_grid_choice(ui.piano_grid,view,extent,gx,gy,gridw,rh*piano_visible_rows());
    if(ui.windows.workspace.enabled)for(int c=0;c<ui.project.channel_count;c++)if(c!=channel)for(int i=0;i<NOTES;i++) {
        Note n=ui.project.notes[ui.pattern][c][i];
        if(!n.velocity || n.pitch>ui.piano_top || n.pitch<ui.piano_top-(int)ceilf(piano_visible_rows())+1 || n.start>=end)continue;
        float left=fmaxf(gx,gx+(n.start-view)*cw),right=fminf(gx+gridw,gx+(fminf(end,n.start+(n.length?n.length:1))-view)*cw);
        if(right<=left)continue;
        DrawRect ghost={left,gy+(ui.piano_top-n.pitch)*rh+1,right-left,fmaxf(1,rh-2)};
        host_draw_rectangle_rec(ghost,host_fade(ui_theme.piano_black,.22f));
        host_draw_rectangle_lines_ex(ghost,fminf(1,rh*.2f),host_fade(ui_theme.piano_black,.38f));
    }

    for(int i=0;i<NOTES;i++) {
        Note *n=&ui.project.notes[ui.pattern][channel][i];
        if(!n->velocity || n->pitch<ui.piano_top-(int)ceilf(piano_visible_rows())+1 || n->pitch>ui.piano_top || n->start>=end || n->start+(n->length?n->length:1)<=view) continue;
        float note_end=fminf(n->start+(n->length?n->length:1),end);
        float x=gx+(n->start-view)*cw,y=gy+(ui.piano_top-n->pitch)*rh,w=(note_end-n->start)*cw;
        float left=fmaxf(gx,x),right=fminf(gx+gridw,x+w);
        if(right<=left) continue;
        float inset=fminf(1,rh*.15f),note_height=rh-2*inset;
        if(ui.note_selected[ui.pattern][channel][i]) host_draw_rectangle_lines_ex((DrawRect){left,y,right-left,rh},fminf(1,rh*.25f),ink);
        int selected=n==ui.note_drag || ui.note_selected[ui.pattern][channel][i];
        DrawColor note_color=ui.studio_style?ui_note_velocity_color(n->velocity,selected):selected?ui_theme.note_drag:ui_theme.note;
        host_draw_rectangle_rec((DrawRect){left+1,y+inset,fmaxf(1,right-left-2),note_height},note_color);
        host_draw_rectangle_lines_ex((DrawRect){left+1,y+inset,fmaxf(1,right-left-2),note_height},fminf(1,rh*.25f),host_fade(ui_theme.signal,.45f));
        int edge=grid_over && !(ui.windows.workspace.enabled && ui.pattern_hits[channel]) && !shortcut_down() && hover(x+w-7,y,7,rh);
        if(x+w>=gx && x+w<=gx+gridw) host_draw_rectangle_rec((DrawRect){x+w-5,y+inset,3,note_height},edge || (n==ui.note_drag && !ui.moving_note)?ink:host_fade(bg,.35f));
        if(edge || (n==ui.note_drag && !ui.moving_note)) host_set_mouse_cursor(HOST_MOUSE_CURSOR_RESIZE_EW);
    }
    if(grid_over) {
        float start=snap_floor(view+(ui.mouse.x-gx)/cw,q);
        snprintf(ui.status,sizeof ui.status,"Piano Roll: %s | Shift: add selection | Right-drag: erase | Delete: delete selected notes | Wheel: zoom",ui.piano_tool==PENCIL?"draw or move notes":ui.piano_tool==BRUSH?"paint notes":"select or move notes");
        if(ui.windows.workspace.enabled && ui.pattern_hits[channel])snprintf(ui.status,sizeof ui.status,"Drum hits: left-drag adds C4 one-shots; right-drag erases; whole-step spacing; Fill X repeats hits");
        if(desktop_piano_inactive(start)) snprintf(ui.status,sizeof ui.status,"Inactive steps: extend the Playlist clip to enable them");
        else if(ui.windows.workspace.enabled && ui.pattern_hits[channel] && ui.piano_tool!=SELECT && !shortcut_down() &&
            !host_is_key_down(HOST_KEY_LEFT_SHIFT) && !host_is_key_down(HOST_KEY_RIGHT_SHIFT) &&
            (host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT))) {
            if(history_checkpoint()) {
                ui.pattern_hit_active=1;ui.pattern_hit_erase=host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT);
                ui.pattern_hit_step=floorf(view+(ui.mouse.x-gx)/cw);pattern_hit_update(ui.pattern_hit_step);
                ui.input_enabled=0;
            }
        }
        else if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) {
            float before=ui.project.pattern_steps[ui.pattern];PianoGesture g=desktop_piano_state();PianoViewport v=desktop_piano_view();
            PianoPress press={{ui.mouse.x,ui.mouse.y},ui.input_enabled,
                host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT),host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT),
                host_is_key_down(HOST_KEY_LEFT_SHIFT) || host_is_key_down(HOST_KEY_RIGHT_SHIFT),shortcut_down(),ui.piano_tool};
            int result=piano_press(&g,&ui.project,&v,press,desktop_note_preview,NULL);
            desktop_piano_state_commit(g,before);
            if(result&PIANO_PRESS_NOTE_LIMIT)snprintf(ui.status,sizeof ui.status,"No room here: remove a note (limit %d).",NOTES);
            if(result&PIANO_PRESS_CONSUMED)ui.input_enabled=0;
        }
    }
    if(ui.piano_gesture==PIANO_BOX) {
        float left=fminf(ui.piano_from.x,ui.piano_now.x),high=fmaxf(ui.piano_from.y,ui.piano_now.y);
        DrawRect box={gx+(left-view)*cw,gy+(ui.piano_top-high)*rh,fabsf(ui.piano_from.x-ui.piano_now.x)*cw,fabsf(ui.piano_from.y-ui.piano_now.y)*rh};
        host_draw_rectangle_rec(box,host_fade(ui_theme.signal,.15f)); host_draw_rectangle_lines_ex(box,1,ui_theme.signal);
    }

    host_begin_scissor_mode(r.x*scale,r.y*scale,r.w*scale,r.h*scale);
    float vh=desktop_velocity_height(),vs=velocity_lane_scale(vh),vy=height-22-vh;
    if(ui.studio_style){
        ui_divider_grip(host_draw(),(DrawRect){4,vy-9,width-28,8},0,ui.velocity_lane.dragging || hover(4,vy-9,width-28,8));
    }
    if(vh>0) {
    label(ui.studio_style?"VEL":"Velocity",6,vy+4,10,muted);
    host_begin_scissor_mode((r.x+gx)*scale,(r.y+vy)*scale,gridw*scale,vh*scale);
    host_draw_rectangle(gx,vy,gridw,vh,ui.studio_style?ui_skin.graph:ui_theme.piano_row[0]);
    if(ui.studio_style)ui_graph_dots(host_draw(),(DrawRect){gx,vy,gridw,vh});
    for(int line=1;line<5;line++) host_draw_line(gx,vy+line*vh/5,gx+gridw,vy+line*vh/5,host_fade(ui_theme.grid_minor,.6f));
    timeline_grid_choice(ui.piano_grid,view,extent,gx,vy,gridw,vh);
    if(partial>end) { float left=fmaxf(gx,gx+(end-view)*cw),right=fminf(gx+gridw,gx+(partial-view)*cw); if(right>left) host_draw_rectangle(left,vy,right-left,vh,ui_theme.disabled); }
    for(int i=0;i<NOTES;i++) {
        Note n=ui.project.notes[ui.pattern][channel][i]; if(!n.velocity || n.start>=end) continue;
        float x=gx+(n.start-view)*cw;
        if(x<gx-10 || x>gx+gridw) continue;
        float top=vy+vh-n.velocity*vs;
        DrawColor color=ui.studio_style?ui_note_velocity_color(n.velocity,ui.note_selected[ui.pattern][channel][i]):ui.note_selected[ui.pattern][channel][i]?ui_theme.note_drag:ui_theme.note;
        host_draw_rectangle_rec((DrawRect){x,top,2,n.velocity*vs},color);
        circle(x+1,top,2.5f,color); circle(x+1,top,1.25f,ui.studio_style?ui_skin.graph:ui_theme.piano_row[0]);
    }
    if(hover(gx,vy,gridw,vh)) {
        snprintf(ui.status,sizeof ui.status,"Velocity: left-drag paints; right-drag draws a straight ramp");
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) {
            ui.velocity_drag=host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)?1:0;
            ui.velocity_pattern=ui.pattern; ui.velocity_channel=channel;
            ui.velocity_from=ui.velocity_now=(DrawPoint){view+(ui.mouse.x-gx)/cw,fmaxf(1,fminf(127,(vy+vh-ui.mouse.y)/vs))};
            memcpy(ui.velocity_before,ui.project.notes[ui.pattern][channel],sizeof ui.velocity_before);
            notes_velocity(ui.project.notes[ui.pattern][channel],ui.velocity_drag?ui.velocity_before:NULL,
                ui.velocity_from.x,ui.velocity_from.x,ui.velocity_from.y,ui.velocity_from.y,3/cw);
            ui.input_enabled=0;
        }
    }
    if(ui.velocity_drag==1 && ui.velocity_pattern==ui.pattern && ui.velocity_channel==channel)
        host_draw_line_ex((DrawPoint){gx+(ui.velocity_from.x-view)*cw,vy+vh-ui.velocity_from.y*vs},
            (DrawPoint){gx+(ui.velocity_now.x-view)*cw,vy+vh-ui.velocity_now.y*vs},1,accent);
    host_begin_scissor_mode(r.x*scale,r.y*scale,r.w*scale,r.h*scale);
    }
    float rail_top=gy-14,varea=rh*piano_visible_rows()+14,vthumb=varea*fminf(128,piano_visible_rows())/128,vypos=rail_top+(127-ui.piano_top)/128.f*varea;
    if(ui.studio_style)ui_scrollbar(host_draw(),(DrawRect){width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,varea},(DrawRect){width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,vypos,EDITOR_SCROLLBAR,vthumb},ui.piano_vdrag || hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,varea));
    else { host_draw_rectangle_rec((DrawRect){width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,varea},bg); ui_surface(host_draw(),(DrawRect){width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,vypos,EDITOR_SCROLLBAR,vthumb},ui.piano_vdrag || hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,varea)?accent:muted); }
    if(hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,varea)) {
        snprintf(ui.status,sizeof ui.status,"Drag to scroll higher or lower pitches");
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) {
            if(ui.mouse.y<vypos || ui.mouse.y>vypos+vthumb) ui.piano_top=127-(int)fmaxf(0,fminf(127-piano_min_top(),(ui.mouse.y-rail_top-vthumb/2)/varea*128));
            ui.piano_vscroll_y=ui.mouse.y+r.y; ui.piano_vscroll_start=127-ui.piano_top; ui.piano_vdrag=1; ui.input_enabled=0;
        }
    }
    float rail_width=width-4-EDITOR_CORNER-gx,thumb=timeline_thumb(rail_width,extent,ui.piano_range[ui.pattern]),travel=rail_width-thumb,maximum=ui.piano_range[ui.pattern]-extent,x=gx+(maximum>0?fmaxf(0,ui.piano_pan[ui.pattern])/maximum*travel:0);
    if(ui.studio_style)ui_scrollbar(host_draw(),(DrawRect){gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR},(DrawRect){x,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,thumb,EDITOR_SCROLLBAR},ui.piano_scroll_drag || hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR));
    else { host_draw_rectangle_rec((DrawRect){gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR},bg); ui_surface(host_draw(),(DrawRect){x,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,thumb,EDITOR_SCROLLBAR},ui.piano_scroll_drag || hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR)?accent:muted); }
    if(hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR)) {
        snprintf(ui.status,sizeof ui.status,"Drag to pan the zoomed Piano Roll; wheel over notes to zoom; the timeline grows as you navigate");
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) {
            if(ui.mouse.x<x || ui.mouse.x>x+thumb) ui.piano_pan[ui.pattern]=fmaxf(0,fminf(maximum,(ui.mouse.x-gx-thumb/2)/fmaxf(1,travel)*maximum));
            ui.piano_scroll_x=ui.mouse.x+r.x; ui.piano_scroll_start=ui.piano_pan[ui.pattern]; ui.piano_scroll_range=maximum/fmaxf(1,travel); ui.piano_scroll_drag=1; ui.input_enabled=0;
        }
    }
    row_zoom_button(2,width,gy-14-EDITOR_CORNER);
    timeline_ruler(2,view,extent,gx,gy-14,gridw,q);
    if(playback>=0) {
        float x=gx+(playback-view)*cw;
        if(x>=gx && x<=gx+gridw) host_draw_line_ex((DrawPoint){x,gy},(DrawPoint){x,gy+rh*piano_visible_rows()},1.5f,ui_theme.signal);
    }

}

/* Pattern uses a channel list rather than a second timeline. */
void pattern_hit_update(float step) {
    int result=pattern_hits_paint(&ui.project,ui.pattern,ui.piano_channel,ui.pattern_hit_step,step,ui.pattern_hit_erase);
    if(result<0){snprintf(ui.status,sizeof ui.status,"Hit stroke stopped: not enough note slots.");ui.pattern_hit_active=0;return;}
    ui.pattern_hit_step=floorf(step);
    if(ui.arrangement.source_pattern==ui.pattern)ui.arrangement.source_steps=ui.project.pattern_steps[ui.pattern];
    if(result && !ui.pattern_hit_erase)audio_note(ui.piano_channel,(Note){60,100,floorf(step),0});
}
static void pattern_channels(float width,float height) {
    ui_panel(host_draw(),(DrawRect){1,TITLE,width-1,height-TITLE-1},panel);
    host_draw_line(width-1,TITLE,width-1,height-1,ui_theme.border);
    if(button("Back to Arrangement",8,26,width-16,24,0)){windows_focus(&ui.windows,1);ui.input_enabled=0;}
    label(fit_text(ui.project.pattern_names[ui.pattern],82,12),8,59,12,ink);
    label("Swing",98,59,10,muted);
    knob_style(146,65,&ui.project.swing,0,1,0,"Swing",KNOB_SWING);
    int visible=fmaxf(1,(height-198)/28),count=ui.project.channel_count;
    if(hover(4,80,width-8,visible*28)) {
        float wheel=host_get_mouse_wheel_move();
        if(wheel)ui.pattern_channel_scroll-=wheel>0?1:-1;
    }
    ui.pattern_channel_scroll=fmaxf(0,fminf(fmaxf(0,count-visible),ui.pattern_channel_scroll));
    for(int row=0;row<visible && row+ui.pattern_channel_scroll<count;row++) {
        int c=row+ui.pattern_channel_scroll,y=80+row*28,selected=c==ui.piano_channel;
        if(button("M",8,y,18,22,ui.project.mute[c]&1))ui.project.mute[c]^=1;
        if(button("S",28,y,18,22,ui.project.mute[c]&2))solo_toggle(ui.project.mute,count,c);
        if(button(fit_text(ui.project.channel_names[c],72,12),50,y,80,22,selected))pattern_select_channel(c);
        if(button("...",134,y,24,22,0)){pattern_select_channel(c);ui.instrument_channel=c;windows_focus(&ui.windows,4);ui.input_enabled=0;}
        if(hover(50,y,80,22) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)){ui.channel=c;Rect r=desktop_editor_rect(2);open_context(2,c,(DrawPoint){r.x+ui.mouse.x,r.y+ui.mouse.y});}
        if(hover(8,y,width-16,22))snprintf(ui.status,sizeof ui.status,"%s: M mute, S solo; name selects notes; ... opens instrument; right-click name for channel actions",ui.project.channel_names[c]);
        if(ui.sample_drag[0] && ui.sample_moved && hover(4,y,width-8,24))host_draw_rectangle_lines_ex((DrawRect){4,y,width-8,24},1,ui_theme.signal);
    }
    if(count>visible) {
        if(button("^",8,height-118,24,18,0))ui.pattern_channel_scroll=fmaxf(0,ui.pattern_channel_scroll-1);
        if(button("v",36,height-118,24,18,0))ui.pattern_channel_scroll=fminf(count-visible,ui.pattern_channel_scroll+1);
        label(host_text_format("%d–%d / %d",ui.pattern_channel_scroll+1,(int)fminf(count,ui.pattern_channel_scroll+visible),count),68,height-115,10,muted);
    }
    int c=ui.piano_channel;
    if(button("+ Add channel",8,height-96,width-16,24,0))open_popup(6);
    knob_style(28,height-46,&ui.project.pan[c],-1,1,0,"Channel pan",KNOB_PAN);
    knob_style(76,height-46,&ui.project.volume[c],0,VOLUME_KNOB_MAX,1,"Channel volume",KNOB_VOLUME);
    channel_route(c,120,height-58);
    label("PAN",17,height-27,9,muted);label("VOL",66,height-27,9,muted);label("ROUTE",115,height-27,9,muted);
}

void piano_tool_panel(float width,float height) {
    if(ui.windows.workspace.enabled){pattern_channels(width,height);return;}
    if(!ui.profile_flat)ui_gradient(host_draw(),(DrawRect){1,TITLE,width-1,height-TITLE-1},panel,.035f,0);
    else ui_surface(host_draw(),(DrawRect){1,TITLE,width-1,height-TITLE-1},panel);
    host_draw_line(width-1,TITLE,width-1,height-1,ui_theme.border);
    const DrawRect groups[]={{8,24,width-16,50},{8,80,width-16,54},{8,138,width-16,52},{8,194,width-16,46}};
    for(unsigned i=0;i<sizeof groups/sizeof *groups;i++)ui_bevel(host_draw(),groups[i],1);
    for(int i=0;i<3;i++)host_draw_line(width-30,31+i*3,width-15,31+i*3,(DrawColor){116,130,112,255});
    if(ui.windows.workspace.enabled){if(button("Back to Rack",12,28,width-24,18,0))windows_focus(&ui.windows,0);}else label("Tool",12,28,13,ink);
    const char *tips[]={"Pencil: draw and move notes","Brush: paint notes","Select: box select notes; Shift adds"};
    for(int i=0;i<3;i++)if(tool_button(i,ui.piano_tool,12+i*30,tips[i]))ui.piano_tool=i;
    label("Grid / snap",12,86,13,ink);
    grid_selector(2,12,104,width-24,24);
    label("Note length",12,142,13,ink);
    ui_surface(host_draw(),(DrawRect){12,160,width-24,24},cell);
    ui_bevel(host_draw(),(DrawRect){12,160,width-24,24},1);
    label(host_text_format("%.2g steps",ui.piano_note_length),20,165,13,ink);
    int selected=0;
    for(int i=0;i<NOTES;i++)selected+=ui.note_selected[ui.pattern][ui.piano_channel][i]!=0;
    label("Selection",12,198,13,ink);
    label(host_text_format("%d notes",selected),20,220,13,muted);
    if(height>=380) {
        ui_surface(host_draw(),(DrawRect){8,252,width-16,112},(DrawColor){57,70,59,255});
        ui_bevel(host_draw(),(DrawRect){8,252,width-16,112},1);
        DrawColor type={199,212,176,255};
        label("PATTERN",16,260,9,type);
        label(fit_text(ui.project.pattern_names[ui.pattern],width-32,11),16,277,11,(DrawColor){127,210,190,255});
        label("CHANNEL",16,304,9,type);
        label(fit_text(ui.project.channel_names[ui.piano_channel],width-32,11),16,321,11,(DrawColor){179,184,222,255});
        label(host_text_format("%.0f steps",ui.project.pattern_steps[ui.pattern]),16,344,9,type);
    }
    if(height>=570) {
        ui_bevel(host_draw(),(DrawRect){8,380,width-16,112},1);
        label("EDIT KEYS",16,389,10,ink);
        label("Shift: add selection",16,413,9,muted);
#ifdef __APPLE__
        label("Cmd: box select",16,433,9,muted);
#else
        label("Ctrl: box select",16,433,9,muted);
#endif
        label("Drag edge: resize",16,453,9,muted);
        label("Delete: remove notes",16,473,9,muted);
    }
    if(height>=420) {
        for(int i=0;i<3;i++) {
            host_draw_line(16,height-65+i*4,width-16,height-65+i*4,(DrawColor){129,138,120,255});
            host_draw_line(16,height-64+i*4,width-16,height-64+i*4,(DrawColor){231,233,211,255});
        }
    }
    if(!ui.windows.workspace.enabled && height>=280 && button("Channel Rack",12,height-38,width-24,24,0))windows_focus(&ui.windows,0);
}
