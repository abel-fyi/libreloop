// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"

float piano_visible_rows(void) { return fmaxf(8,fminf(128,25/ui.piano_zoom)); }

int piano_min_top(void) { return (int)ceilf(piano_visible_rows())-1; }

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

int piano_inactive(float step) { float end=edit_steps(); return step>=end && step<ceilf(end/STEPS)*STEPS; }

void piano_extend(float end) { pattern_extend(end); }

void piano_gesture_update(float step,float pitch,float q) {
    int channel=ui.piano_channel;
    uint8_t *selected=ui.note_selected[ui.pattern][channel];
    if(ui.piano_gesture==PIANO_BOX) {
        float left=fminf(ui.piano_from.x,step),right=fmaxf(ui.piano_from.x,step),low=fminf(ui.piano_from.y,pitch),high=fmaxf(ui.piano_from.y,pitch);
        for(int i=0;i<NOTES;i++) {
            Note n=ui.project.notes[ui.pattern][channel][i];
            selected[i]=(ui.piano_additive && ui.note_selection_before[i]) || (n.velocity && n.start<edit_steps() && n.start<=right && n.start+(n.length?n.length:1)>left && n.pitch>=low && n.pitch-1<=high);
        }
    } else {
        float dx=step-ui.piano_now.x,dy=pitch-ui.piano_now.y,spacing=ui.piano_gesture==PIANO_BRUSH?fmaxf(ui.piano_note_length,q):q>0?q:1;
        int count=fminf(NOTES*2,ceilf(fmaxf(fabsf(dx)/spacing,fabsf(dy))*2)); if(count<1) count=1;
        for(int j=0;j<=count;j++) {
            float t=j/(float)count,at=ui.piano_now.x+dx*t; int key=ceilf(ui.piano_now.y+dy*t);
            if(at<0 || piano_inactive(at) || key<0 || key>127) continue;
            if(ui.piano_gesture==PIANO_ERASE) {
                /* Hit the gesture's original stack so holding the button or
                   repeated stroke samples cannot peel off underlying notes. */
                int i=note_hit(ui.note_before,at,key);
                if(i>=0) { ui.project.notes[ui.pattern][channel][i].velocity=0; selected[i]=0; }
            } else {
                float start=ui.piano_from.x+floorf((at-ui.piano_from.x)/spacing)*spacing;
                piano_extend(start+ui.piano_note_length);
                int exists=note_at(&ui.project,ui.pattern,channel,start,key)!=NULL;
                Note *n=exists?NULL:note_add(&ui.project,ui.pattern,channel,start,key,ui.piano_note_length);
                if(n) { audio_note(channel,*n); ui.piano_channels[ui.pattern][channel]=1; }
            }
        }
    }
    ui.piano_now=(Vector2){step,pitch};
}

void piano_note_drag_update(float step,float row,float q) {
    int channel=ui.piano_channel;
    if(ui.moving_note) {
        uint8_t *selected=ui.note_selected[ui.pattern][channel];
        float dx=snap_round(step-ui.note_grab.x,q),low=0,high=0; int dy=roundf(ui.note_grab.y-row),bottom=0,top=127,first=1;
        for(int i=0;i<NOTES;i++) if(selected[i]) {
            Note n=ui.note_before[i]; float end=n.start+(n.length?n.length:1);
            if(first || n.start<low) low=n.start;
            if(first || end>high) high=end;
            if(first || n.pitch<bottom) bottom=n.pitch;
            if(first || n.pitch>top) top=n.pitch;
            first=0;
        }
        dx=fmaxf(-low,dx); piano_extend(high+dx); dy=fmaxf(-bottom,fminf(127-top,dy));
        int old_pitch=ui.note_drag->pitch;
        notes_move(&ui.project,ui.pattern,channel,ui.note_before,selected,dx,dy,edit_steps());
        if(ui.note_drag->pitch!=old_pitch) audio_note(channel,*ui.note_drag);
    } else {
        float delta=snap_round(step-ui.note_grab.x,q);
        float end=notes_resize(&ui.project,ui.pattern,channel,ui.note_before,ui.note_selected[ui.pattern][channel],delta,q>0?q:.01f);
        piano_extend(end); ui.piano_note_length=ui.note_drag->length;
    }
}

int piano_black_key(int pitch) {
    int pc=pitch%12; return pc==1 || pc==3 || pc==6 || pc==8 || pc==10;
}

Rectangle piano_key_rect(int pitch,float rh) {
    float center=PIANO_GRID_TOP+(ui.piano_top-pitch+.5f)*rh;
    if(piano_black_key(pitch)) return (Rectangle){4,center-rh*.5f,34,rh};
    float top=center-rh*(piano_black_key(pitch+1)?1:.5f);
    float bottom=center+rh*(piano_black_key(pitch-1)?1:.5f);
    top=fmaxf(PIANO_GRID_TOP,top); bottom=fminf(PIANO_GRID_TOP+rh*piano_visible_rows(),bottom);
    return (Rectangle){4,top,54,fmaxf(0,bottom-top)};
}

int piano_key_at(float x,float y,float rh) {
    if(x<4 || x>=58 || y<PIANO_GRID_TOP || y>=PIANO_GRID_TOP+rh*piano_visible_rows()) return -1;
    float row=(y-PIANO_GRID_TOP)/rh; int pitch=ui.piano_top-(int)floorf(row);
    if(piano_black_key(pitch) && x>=38) pitch+=row-floorf(row)<.5f?1:-1;
    return pitch>=0 && pitch<=127?pitch:-1;
}

void piano_key_update(float x,float y,int cancel) {
    Rect r=ui.windows.editors[2].rect; float rh=(r.h-PIANO_GRID_PADDING)/piano_visible_rows();
    int pitch=piano_key_at(x,y,rh);
    if(cancel || pitch<0 || pitch>127) pitch=-1;
    if(pitch!=ui.piano_key) {
        if(ui.piano_key>=0) audio_key(96+ui.piano_key%25,ui.piano_key_channel,ui.piano_key,0);
        ui.piano_key=pitch;
        if(pitch>=0) audio_key(96+pitch%25,ui.piano_key_channel,pitch,1);
    }
    if(cancel) ui.piano_key_drag=0;
}

void piano(float width,float height) {
    int channel=ui.piano_channel;
    int gx=60,gy=PIANO_GRID_TOP; float gridw=width-84,rh=(height-PIANO_GRID_PADDING)/piano_visible_rows();
    float extent=ui.piano_span[ui.pattern];
    Rect r=ui.windows.editors[2].rect; float scale=ui_scale();
    const char *tips[]={"Pencil: draw one note; drag note bodies to move; edges to resize","Brush: drag to paint notes on the visible grid","Select: drag a rectangle; drag selected notes together; Shift adds; Delete deletes"};
    for(int i=0;i<3;i++) if(tool_button(i,ui.piano_tool,8+i*24,tips[i])) ui.piano_tool=i;
    label(fit_text(TextFormat("%s / %s",ui.project.channel_names[channel],ui.project.pattern_names[ui.pattern]),width-158,12),88,30,12,muted);
    int grid_over=hover(gx,gy,gridw,rh*piano_visible_rows());
    extent=ui.piano_span[ui.pattern];
    double playback=pattern_playback_position();
    if(playback>=0) follow_view(&ui.piano_pan[ui.pattern],extent,playback);
    float cw=gridw/extent,view=ui.piano_pan[ui.pattern],q=grid_interval(cw),end=edit_steps();
    ui.piano_range[ui.pattern]=fmaxf(ui.piano_range[ui.pattern],fmaxf(view+extent*2,end+extent));
    float partial=ceilf(end/STEPS)*STEPS;
    if(hover(4,gy,gx-6,rh*piano_visible_rows())) {
        snprintf(ui.status,sizeof ui.status,"Piano keys: click to play; hold and drag up/down to audition pitches");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ui.piano_key_drag=1; ui.piano_key_channel=channel; piano_key_update(ui.mouse.x,ui.mouse.y,0); ui.input_enabled=0;
        }
    }
    DrawRectangleRec((Rectangle){4,gy,gx-6,rh*piano_visible_rows()},ui_theme.piano_white);
    BeginScissorMode((r.x+4)*scale,(r.y+gy)*scale,54*scale,(height-PIANO_GRID_PADDING)*scale);
    int hovered_key=ui.input_enabled?piano_key_at(ui.mouse.x,ui.mouse.y,rh):-1;
    /* Draw full white keys first; black keys cover their overlapping left ends. */
    for(int black=0;black<2;black++) for(int row=-1;row<=(int)ceilf(piano_visible_rows());row++) {
        int pitch=ui.piano_top-row;
        if(pitch<0 || pitch>127 || piano_black_key(pitch)!=black) continue;
        Rectangle key=piano_key_rect(pitch,rh);
        float bottom=fminf(gy+rh*piano_visible_rows(),key.y+key.height);
        key.y=fmaxf(gy,key.y); key.height=fmaxf(0,bottom-key.y);
        if(key.height<=0) continue;
        int active=ui.keyboard_notes[channel][pitch] || ui.playing_keys[channel][pitch] || (ui.piano_key==pitch && ui.piano_key_channel==channel);
        Color base=black?ui_theme.piano_black:pitch%12==0?ui_theme.piano_c:ui_theme.piano_white;
        DrawRectangleRec(key,active?accent:base);
        if(!active && hovered_key==pitch) DrawRectangleRec(key,Fade(accent,.22f));
        if(!black) DrawLineEx((Vector2){key.x,key.y+key.height},(Vector2){key.x+key.width,key.y+key.height},1,Fade(ui_theme.piano_black,.15f));
        if(pitch%12==0) {
            const char *name=TextFormat("C%d",pitch/12-1);
            label(name,gx-5-text_width(name,10),fmaxf(gy,fminf(gy+rh*piano_visible_rows()-10,gy+(row+.5f)*rh-5)),10,active?ui_theme.selected_text:ui_theme.piano_black);
        }
    }
    BeginScissorMode((r.x+gx)*scale,(r.y+gy)*scale,gridw*scale,rh*piano_visible_rows()*scale);
    for(int row=0;row<(int)ceilf(piano_visible_rows());row++) {
        int pc=(ui.piano_top-row)%12,black=pc==1 || pc==3 || pc==6 || pc==8 || pc==10;
        float y=gy+row*rh;
        DrawRectangleRec((Rectangle){gx,y,gridw,rh},ui_theme.piano_row[black]);
        int pitch=ui.piano_top-row;
        if(pitch>=0 && pitch<=127 && (ui.keyboard_notes[channel][pitch] || ui.playing_keys[channel][pitch] ||
           (ui.piano_key==pitch && ui.piano_key_channel==channel)))
            DrawRectangleRec((Rectangle){gx,y,gridw,rh},Fade(accent,.15f));
        DrawLineEx((Vector2){gx,y},(Vector2){gx+gridw,y},1,Fade(ui_theme.grid_minor,.45f));
        if(partial>end) { float left=fmaxf(gx,gx+(end-view)*cw),right=fminf(gx+gridw,gx+(partial-view)*cw); if(right>left) DrawRectangleRec((Rectangle){left,y,right-left,rh-1},ui_theme.disabled); }
        if(pc==0) DrawLineEx((Vector2){gx,y+rh},(Vector2){gx+gridw,y+rh},1,ui_theme.grid_major);
    }
    timeline_grid(view,extent,gx,gy,gridw,rh*piano_visible_rows());
    Note *hit=NULL;
    for(int i=0;i<NOTES;i++) {
        Note *n=&ui.project.notes[ui.pattern][channel][i];
        if(!n->velocity || n->pitch<ui.piano_top-(int)ceilf(piano_visible_rows())+1 || n->pitch>ui.piano_top || n->start>=end || n->start+(n->length?n->length:1)<=view) continue;
        float note_end=fminf(n->start+(n->length?n->length:1),end);
        float x=gx+(n->start-view)*cw,y=gy+(ui.piano_top-n->pitch)*rh,w=(note_end-n->start)*cw;
        float left=fmaxf(gx,x),right=fminf(gx+gridw,x+w);
        if(right<=left) continue;
        float inset=fminf(1,rh*.15f),note_height=rh-2*inset;
        if(ui.note_selected[ui.pattern][channel][i]) DrawRectangleLinesEx((Rectangle){left,y,right-left,rh},fminf(1,rh*.25f),ink);
        DrawRectangleRec((Rectangle){left+1,y+inset,fmaxf(1,right-left-2),note_height},n==ui.note_drag || ui.note_selected[ui.pattern][channel][i]?ui_theme.note_drag:ui_theme.note);
        DrawRectangleLinesEx((Rectangle){left+1,y+inset,fmaxf(1,right-left-2),note_height},fminf(1,rh*.25f),Fade(ui_theme.signal,.45f));
        int edge=grid_over && !shortcut_down() && hover(x+w-7,y,7,rh);
        if(x+w>=gx && x+w<=gx+gridw) DrawRectangleRec((Rectangle){x+w-5,y+inset,3,note_height},edge || (n==ui.note_drag && !ui.moving_note)?ink:Fade(bg,.35f));
        if(edge || (n==ui.note_drag && !ui.moving_note)) SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
        if(grid_over && hover(x,y,w,rh)) hit=n;
    }
    if(grid_over) {
        float start=snap_floor(view+(ui.mouse.x-gx)/cw,q);
        snprintf(ui.status,sizeof ui.status,"Piano Roll: %s | Shift: add selection | Right-drag: erase | Delete: delete selected notes | Wheel: zoom",ui.piano_tool==PENCIL?"draw or move notes":ui.piano_tool==BRUSH?"paint notes":"select or move notes");
        if(piano_inactive(start)) snprintf(ui.status,sizeof ui.status,"Inactive steps: extend the Playlist clip to enable them");
        else if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            uint8_t *selected=ui.note_selected[ui.pattern][channel];
            if(hit && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.piano_note_length=hit->length?hit->length:2;
            Vector2 point={view+(ui.mouse.x-gx)/cw,ui.piano_top-(ui.mouse.y-gy)/rh};
            int shift=IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT),group=0;
            for(int i=0;i<NOTES;i++) group+=selected[i]!=0;
            ui.piano_from=ui.piano_now=point; ui.piano_additive=shift; memcpy(ui.note_selection_before,selected,NOTES);
            if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                memcpy(ui.note_before,ui.project.notes[ui.pattern][channel],sizeof ui.note_before);
                ui.piano_gesture=PIANO_ERASE;
            }
            else if(shortcut_down()) {
                if(!shift) memset(selected,0,NOTES);
                ui.piano_gesture=PIANO_BOX;
            } else if(shift && hit) selected[hit-ui.project.notes[ui.pattern][channel]]^=1;
            else if((shift || ui.piano_tool==SELECT) && !hit) {
                if(!shift) memset(selected,0,NOTES);
                ui.piano_gesture=PIANO_BOX;
            } else if(ui.piano_tool==BRUSH && !(hit && selected[hit-ui.project.notes[ui.pattern][channel]] && group>1)) ui.piano_gesture=PIANO_BRUSH;
            else {
                ui.moving_note=0;
                if(hit) {
                    float edge=gx+(fminf(hit->start+(hit->length?hit->length:1),end)-view)*cw;
                    ui.note_drag=hit; ui.moving_note=ui.mouse.x<edge-7;
                    if(!selected[hit-ui.project.notes[ui.pattern][channel]]) { memset(selected,0,NOTES); selected[hit-ui.project.notes[ui.pattern][channel]]=1; }
                    memcpy(ui.note_before,ui.project.notes[ui.pattern][channel],sizeof ui.note_before);
                    ui.note_grab=(Vector2){point.x,(ui.mouse.y-gy)/rh};
                } else {
                    memset(selected,0,NOTES);
                    piano_extend(start+ui.piano_note_length);
                    ui.note_drag=note_add(&ui.project,ui.pattern,channel,start,ceilf(point.y),ui.piano_note_length);
                    if(ui.note_drag) {
                        selected[ui.note_drag-ui.project.notes[ui.pattern][channel]]=1; ui.moving_note=1;
                        memcpy(ui.note_before,ui.project.notes[ui.pattern][channel],sizeof ui.note_before);
                        ui.note_grab=(Vector2){point.x,(ui.mouse.y-gy)/rh};
                    }
                }
                if(!ui.note_drag) snprintf(ui.status,sizeof ui.status,"No room here: remove a note (limit %d).",NOTES);
                else { audio_note(channel,*ui.note_drag); ui.piano_channels[ui.pattern][channel]=1; }
            }
            if(ui.piano_gesture==PIANO_BRUSH) ui.piano_from.x=start;
            if(ui.piano_gesture) piano_gesture_update(point.x,point.y,q);
            ui.input_enabled=0;
        }
    }
    if(ui.piano_gesture==PIANO_BOX) {
        float left=fminf(ui.piano_from.x,ui.piano_now.x),high=fmaxf(ui.piano_from.y,ui.piano_now.y);
        Rectangle box={gx+(left-view)*cw,gy+(ui.piano_top-high)*rh,fabsf(ui.piano_from.x-ui.piano_now.x)*cw,fabsf(ui.piano_from.y-ui.piano_now.y)*rh};
        DrawRectangleRec(box,Fade(ui_theme.signal,.15f)); DrawRectangleLinesEx(box,1,ui_theme.signal);
    }

    BeginScissorMode(r.x*scale,r.y*scale,r.w*scale,r.h*scale);
    int vy=height-84; label("Velocity",6,vy+4,10,muted);
    BeginScissorMode((r.x+gx)*scale,(r.y+vy)*scale,gridw*scale,62*scale);
    DrawRectangle(gx,vy,gridw,62,ui_theme.piano_row[0]);
    for(int line=1;line<5;line++) DrawLine(gx,vy+line*62/5,gx+gridw,vy+line*62/5,Fade(ui_theme.grid_minor,.6f));
    timeline_grid(view,extent,gx,vy,gridw,62);
    if(partial>end) { float left=fmaxf(gx,gx+(end-view)*cw),right=fminf(gx+gridw,gx+(partial-view)*cw); if(right>left) DrawRectangle(left,vy,right-left,62,ui_theme.disabled); }
    for(int i=0;i<NOTES;i++) {
        Note n=ui.project.notes[ui.pattern][channel][i]; if(!n.velocity || n.start>=end) continue;
        float x=gx+(n.start-view)*cw;
        if(x<gx-10 || x>gx+gridw) continue;
        float top=vy+62-n.velocity*.48f;
        Color color=ui.note_selected[ui.pattern][channel][i]?ui_theme.note_drag:ui_theme.note;
        DrawRectangleRec((Rectangle){x,top,2,n.velocity*.48f},color);
        circle(x+1,top,2.5f,color); circle(x+1,top,1.25f,ui_theme.piano_row[0]);
    }
    if(hover(gx,vy,gridw,62)) {
        snprintf(ui.status,sizeof ui.status,"Velocity: left-drag paints; right-drag draws a straight ramp");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            ui.velocity_drag=IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)?1:0;
            ui.velocity_pattern=ui.pattern; ui.velocity_channel=channel;
            ui.velocity_from=ui.velocity_now=(Vector2){view+(ui.mouse.x-gx)/cw,fmaxf(1,fminf(127,(vy+62-ui.mouse.y)/.48f))};
            memcpy(ui.velocity_before,ui.project.notes[ui.pattern][channel],sizeof ui.velocity_before);
            notes_velocity(ui.project.notes[ui.pattern][channel],ui.velocity_drag?ui.velocity_before:NULL,
                ui.velocity_from.x,ui.velocity_from.x,ui.velocity_from.y,ui.velocity_from.y,3/cw);
            ui.input_enabled=0;
        }
    }
    if(ui.velocity_drag==1 && ui.velocity_pattern==ui.pattern && ui.velocity_channel==channel)
        DrawLineEx((Vector2){gx+(ui.velocity_from.x-view)*cw,vy+62-ui.velocity_from.y*.48f},
            (Vector2){gx+(ui.velocity_now.x-view)*cw,vy+62-ui.velocity_now.y*.48f},1,accent);
    BeginScissorMode(r.x*scale,r.y*scale,r.w*scale,r.h*scale);
    float rail_top=gy-14,varea=rh*piano_visible_rows()+14,vthumb=varea*piano_visible_rows()/128,vypos=rail_top+(127-ui.piano_top)/128.f*varea;
    DrawRectangleRec((Rectangle){width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,varea},bg);
    ui_surface((Rectangle){width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,vypos,EDITOR_SCROLLBAR,vthumb},ui.piano_vdrag || hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,varea)?accent:muted);
    if(hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,varea)) {
        snprintf(ui.status,sizeof ui.status,"Drag to scroll higher or lower pitches");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(ui.mouse.y<vypos || ui.mouse.y>vypos+vthumb) ui.piano_top=127-(int)fmaxf(0,fminf(127-piano_min_top(),(ui.mouse.y-rail_top-vthumb/2)/varea*128));
            ui.piano_vscroll_y=ui.mouse.y+r.y; ui.piano_vscroll_start=127-ui.piano_top; ui.piano_vdrag=1; ui.input_enabled=0;
        }
    }
    float rail_width=width-4-EDITOR_CORNER-gx,thumb=timeline_thumb(rail_width,extent,ui.piano_range[ui.pattern]),travel=rail_width-thumb,maximum=ui.piano_range[ui.pattern]-extent,x=gx+(maximum>0?fmaxf(0,ui.piano_pan[ui.pattern])/maximum*travel:0);
    DrawRectangleRec((Rectangle){gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR},bg); ui_surface((Rectangle){x,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,thumb,EDITOR_SCROLLBAR},ui.piano_scroll_drag || hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR)?accent:muted);
    if(hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR)) {
        snprintf(ui.status,sizeof ui.status,"Drag to pan the zoomed Piano Roll; wheel over notes to zoom; the timeline grows as you navigate");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(ui.mouse.x<x || ui.mouse.x>x+thumb) ui.piano_pan[ui.pattern]=fmaxf(0,fminf(maximum,(ui.mouse.x-gx-thumb/2)/fmaxf(1,travel)*maximum));
            ui.piano_scroll_x=ui.mouse.x+r.x; ui.piano_scroll_start=ui.piano_pan[ui.pattern]; ui.piano_scroll_range=maximum/fmaxf(1,travel); ui.piano_scroll_drag=1; ui.input_enabled=0;
        }
    }
    row_zoom_button(2,width,gy-14-EDITOR_CORNER);
    timeline_ruler(2,view,extent,gx,gy-14,gridw,q);
    if(playback>=0) {
        float x=gx+(playback-view)*cw;
        if(x>=gx && x<=gx+gridw) DrawLineEx((Vector2){x,gy},(Vector2){x,gy+rh*piano_visible_rows()},1.5f,ui_theme.signal);
    }

}
