// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"

float track_height(int lane) { return fmaxf(28,fminf(640,(ui.track_heights[lane]>0?ui.track_heights[lane]:52)*ui.track_zoom)); }

float track_position(float lane) {
    float y=0; int l=fmaxf(0,fminf(LANES,floorf(lane)));
    for(int i=0;i<l;i++) y+=track_height(i);
    return y+(l<LANES?(lane-l)*track_height(l):0);
}

float track_at(float position) {
    for(int l=0;l<LANES;l++) { float h=track_height(l); if(position<h) return l+position/h; position-=h; }
    return LANES;
}

void previews_update(float scale) {
    float pixels=(ui.windows.editors[1].rect.w-236)*ui.arrangement.zoom/(BARS*STEPS)*scale;
    for(int pat=0;pat<ui.project.pattern_count;pat++) {
        Color content=clip_foreground(pattern_color(pat));
        float size=ui.project.pattern_steps[pat]*pixels;
        ui.previews[pat].valid=0;
        if(!isfinite(size) || size>2048) continue; /* Keep vector detail at extreme zoom. */
        int w=fmaxf(1,ceilf(size)),h=fmaxf(1,ceilf(32*scale));
        if(!ui.previews[pat].image.id || ui.previews[pat].image.texture.width!=w || ui.previews[pat].image.texture.height!=h ||
           ui.previews[pat].steps!=ui.project.pattern_steps[pat] || ui.previews[pat].channels!=ui.project.channel_count ||
           memcmp(&ui.previews[pat].color,&content,sizeof content) || memcmp(ui.previews[pat].notes,ui.project.notes[pat],sizeof ui.previews[pat].notes)) {
            if(!ui.previews[pat].image.id || ui.previews[pat].image.texture.width!=w || ui.previews[pat].image.texture.height!=h) {
                if(ui.previews[pat].image.id) UnloadRenderTexture(ui.previews[pat].image);
                ui.previews[pat].image=LoadRenderTexture(w,h);
                SetTextureFilter(ui.previews[pat].image.texture,TEXTURE_FILTER_BILINEAR);
                SetTextureWrap(ui.previews[pat].image.texture,TEXTURE_WRAP_CLAMP);
            }
            if(!ui.previews[pat].image.id) continue;
            memcpy(ui.previews[pat].notes,ui.project.notes[pat],sizeof ui.previews[pat].notes);
            ui.previews[pat].steps=ui.project.pattern_steps[pat]; ui.previews[pat].channels=ui.project.channel_count; ui.previews[pat].color=content;
            BeginTextureMode(ui.previews[pat].image); ClearBackground(BLANK);
            int low,high; pattern_pitch_range(pat,&low,&high);
            float note_height=fmaxf(2,fminf(5,32.f/fmaxf(1,high-low+1)));
            for(int c=0;c<ui.project.channel_count;c++) for(int n=0;n<NOTES;n++) {
                Note note=ui.project.notes[pat][c][n]; if(!note.velocity || note.start>=ui.project.pattern_steps[pat]) continue;
                float x=note.start*w/ui.project.pattern_steps[pat]+scale;
                float fraction=high>low?(high-note.pitch)/(float)(high-low):.5f;
                float y=fraction*(32-note_height)*h/32;
                float length=fmaxf(scale,(note.length?note.length:1)*w/ui.project.pattern_steps[pat]-2*scale);
                DrawRectangleRec((Rectangle){x,y,length,note_height*h/32},content);
            }
            EndTextureMode();
        }
        ui.previews[pat].valid=1;
    }
}

void clip_preview(int pat,float x,float y,float left,float right,float pixels,float steps,float height) {
    Color content=clip_foreground(pattern_color(pat));
    float full=ui.project.pattern_steps[pat]*pixels;
    if(ui.previews[pat].valid) {
        right=fminf(right,x+full);
        if(right>left) {
            Texture2D image=ui.previews[pat].image.texture;
            DrawTexturePro(image,(Rectangle){(left-x)*image.width/full,0,(right-left)*image.width/full,-image.height},
                           (Rectangle){left,y+16,right-left,height-20},(Vector2){0},0,WHITE);
        }
    } else {
        int low,high; pattern_pitch_range(pat,&low,&high);
        float area=fmaxf(1,height-20),note_height=fmaxf(2,fminf(5,32.f/fmaxf(1,high-low+1)))*area/32;
        for(int c=0;c<ui.project.channel_count;c++) for(int n=0;n<NOTES;n++) {
            Note note=ui.project.notes[pat][c][n]; if(!note.velocity || note.start>=steps) continue;
            float fraction=high>low?(high-note.pitch)/(float)(high-low):.5f;
            float yy=y+16+fraction*(area-note_height);
            float nx=x+note.start*pixels+1,nw=fmaxf(1,fminf(note.length?note.length:1,steps-note.start)*pixels-2);
            if(nx<=right && nx+nw>=left) DrawRectangleRec((Rectangle){fmaxf(left,nx),yy,fmaxf(1,fminf(right,nx+nw)-fmaxf(left,nx)),note_height},content);
        }
    }
}

void picker_notes(int p,float x,float y) {
    Color content=clip_foreground(pattern_color(p)); int low=127,high=0,count=0; float end=0;
    for(int c=0;c<ui.project.channel_count;c++) for(int n=0;n<NOTES;n++) {
        Note note=ui.project.notes[p][c][n]; if(!note.velocity || note.start>=ui.project.pattern_steps[p]) continue;
        low=fminf(low,note.pitch); high=fmaxf(high,note.pitch); count++;
        end=fmaxf(end,fminf(ui.project.pattern_steps[p],note.start+(note.length?note.length:1)));
    }
    if(!count) return;
    float span=fmaxf(STEPS,ceilf(end/STEPS)*STEPS),note_height=fmaxf(2,fminf(5,26.f/(high-low+1)));
    for(int c=0;c<ui.project.channel_count;c++) for(int n=0;n<NOTES;n++) {
        Note note=ui.project.notes[p][c][n]; if(!note.velocity || note.start>=ui.project.pattern_steps[p]) continue;
        float px=x+4+104*note.start/span;
        float length=fmaxf(1,104*fminf(note.length?note.length:1,span-note.start)/span-1);
        float fraction=high>low?(high-note.pitch)/(float)(high-low):.5f;
        float py=y+20+fraction*(26-note_height);
        DrawRectangleRec((Rectangle){px,py,length,note_height},content);
    }
}

Color source_color(int source) {
    return source>=AUTOMATION_SOURCE?pattern_rgb(ui.project.automations[source-AUTOMATION_SOURCE].color):AUDIO_SOURCE(source)?audio_color(source-PATTERNS):pattern_color(source);
}

const char *source_name(int source) {
    return source>=AUTOMATION_SOURCE?ui.project.automations[source-AUTOMATION_SOURCE].name:AUDIO_SOURCE(source)?channel_caption(source-PATTERNS):ui.project.pattern_names[source];
}

void automation_curve(int a,float origin,float y,float pixels,float height,float left,float right,int nodes) {
    Color content=clip_foreground(source_color(AUTOMATION_SOURCE+a));
    const Automation *automation=&ui.project.automations[a];
    float top=y+19,range=fmaxf(1,height-25);
    if(automation->count) {
        float first=origin+automation->points[0].step*pixels,last=origin+automation->points[automation->count-1].step*pixels;
        float first_y=top+(1-automation->points[0].value)*range,last_y=top+(1-automation->points[automation->count-1].value)*range;
        if(first>left) smooth_line((Vector2){left,first_y},(Vector2){fminf(right,first),first_y},1.5f,content);
        if(last<right) smooth_line((Vector2){fmaxf(left,last),last_y},(Vector2){right,last_y},1.5f,content);
    }
    for(int i=1;i<automation->count;i++) {
        AutomationPoint p=automation->points[i-1],q=automation->points[i];
        float x=origin+p.step*pixels,xx=origin+q.step*pixels;
        if(xx<left || x>right) continue;
        if(xx==x) {
            if(x>=left && x<=right) smooth_line((Vector2){x,top+(1-p.value)*range},(Vector2){x,top+(1-q.value)*range},1.5f,content);
            continue;
        }
        float l=fmaxf(left,x),r=fminf(right,xx);
        float lv=p.value+(q.value-p.value)*(l-x)/(xx-x),rv=p.value+(q.value-p.value)*(r-x)/(xx-x);
        smooth_line((Vector2){l,top+(1-lv)*range},(Vector2){r,top+(1-rv)*range},1.5f,content);
    }
    if(nodes) for(int i=0;i<automation->count;i++) {
        float x=origin+automation->points[i].step*pixels,yy=top+(1-automation->points[i].value)*range;
        if(x>=left && x<=right+2) { x=fmaxf(left+4,fminf(right-4,x)); circle(x,yy,4,content); circle(x,yy,2,source_color(AUTOMATION_SOURCE+a)); }
    }
}

void draw_picker_drag(void) {
    if(ui.picker_drag<0 || (fabsf(ui.mouse.x-ui.picker_origin.x)<3 && fabsf(ui.mouse.y-ui.picker_origin.y)<3)) return;
    int source=ui.picker_drag,audio=AUDIO_SOURCE(source);
    float x=ui.mouse.x-ui.picker_offset.x,y=ui.mouse.y-ui.picker_offset.y,steps=clip_source_steps(&ui.project,source);
    Color color=source_color(source);
    ui_surface((Rectangle){x,y,112,50},color);
    if(audio) audio_waveform(source-PATTERNS,x+4,y+20,x+4,x+108,104/fmaxf(.001f,steps),28);
    else if(source>=AUTOMATION_SOURCE) automation_curve(source-AUTOMATION_SOURCE,x+4,y,104/steps,50,x+4,x+108,0);
    else picker_notes(source,x,y);
    label(fit_text(source_name(source),100,12),x+4,y+4,12,clip_foreground(color));
    DrawRectangleLinesEx((Rectangle){x,y,112,50},2,WHITE);
}

void draw_browser_drag(void) {
    if(!ui.sample_drag[0] || !ui.sample_moved) return;
    float x=ui.mouse.x-ui.sample_offset.x,y=ui.mouse.y-ui.sample_offset.y;
    ui_surface((Rectangle){x,y,ui.sample_drag_width,21},accent);
    label(fit_text(GetFileName(ui.sample_drag),fmaxf(0,ui.sample_drag_width-ui.sample_text_offset-2),13),x+ui.sample_text_offset,y+4,13,ui_theme.selected_text);
}

int tool_button(int tool,int active,int x,const char *tip) {
    int over=hover(x,28,20,20);
    if(over) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    Color surface=active==tool?accent:over?ui_theme.hover:cell;
    Color c=active==tool || over?theme_foreground(surface):ink;
    ui_surface((Rectangle){x,28,20,20},surface);
    icon(tool==PENCIL?ICON_PENCIL:tool==BRUSH?ICON_BRUSH:tool==CUT?ICON_CUT:tool==STRETCH?ICON_STRETCH:ICON_SELECT,x+10,38,20,c);
    if(over) snprintf(ui.status,sizeof ui.status,"%s",tip);
    return over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void row_zoom_update(float y) {
    float zoom=ui.row_zoom_start*expf((ui.row_zoom_y-y)/160);
    if(ui.row_zoom_drag==1) { ui.track_zoom=fmaxf(.4f,fminf(4,zoom)); ui.track_scroll=track_position(ui.row_zoom_anchor); }
    else if(ui.row_zoom_drag==2) { ui.piano_zoom=fmaxf(25.f/128,fminf(25.f/8,zoom)); ui.piano_top=fmaxf(piano_min_top(),ui.piano_top); ui.piano_scroll_remainder=0; }
}

void row_zoom_button(int id,float width,float top) {
    float x=width-4-EDITOR_CORNER;
    int over=hover(x,top,EDITOR_CORNER,EDITOR_CORNER),active=over || ui.row_zoom_drag==id;
    Color surface=ui.row_zoom_drag==id?(over?ui_theme.active_hover:accent):over?ui_theme.hover:cell;
    ui_surface((Rectangle){x,top,EDITOR_CORNER,EDITOR_CORNER},surface);
    if(over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        ui.row_zoom_drag=id; ui.row_zoom_y=ui.mouse.y+ui.windows.editors[id].rect.y;
        ui.row_zoom_start=id==1?ui.track_zoom:ui.piano_zoom; ui.row_zoom_anchor=track_at(ui.track_scroll); ui.input_enabled=0;
    }
    icon(ICON_ROW_ZOOM,x+EDITOR_CORNER/2,top+EDITOR_CORNER/2,16,active?theme_foreground(surface):muted);
    if(active) { SetMouseCursor(MOUSE_CURSOR_RESIZE_NS); snprintf(ui.status,sizeof ui.status,"Vertical zoom: drag up for taller %s, down to show more",id==1?"tracks":"notes"); }
}

void ruler_press(int id,int button,float at,double time,Vector2 position) {
    float *range=id==1?ui.song_loop:ui.pattern_loop[ui.pattern];
    int twice=ui.ruler_last_id==id && (id==1 || ui.ruler_last_pattern==ui.pattern) && ui.ruler_last_button==button &&
        time-ui.ruler_last_click<.3 && fabsf(position.x-ui.ruler_last_position.x)<6 && fabsf(position.y-ui.ruler_last_position.y)<6;
    ui.ruler_last_id=id; ui.ruler_last_pattern=ui.pattern; ui.ruler_last_button=button; ui.ruler_last_click=time; ui.ruler_last_position=position;
    ui.stop_armed=0;
    if(ui.song!=(id==1)) { ui.song=id==1; ui.reset=1; }
    if(twice) { range[0]=range[1]=0; ui.marker_drag=-1; ui.ruler_last_id=-1; return; }
    ui.marker_drag=id; ui.ruler_loop_drag=button==MOUSE_BUTTON_RIGHT;
    if(ui.ruler_loop_drag) {
        ui.ruler_anchor=range[1]>range[0]?(at<(range[0]+range[1])/2?range[1]:range[0]):at;
        range[0]=fminf(ui.ruler_anchor,at); range[1]=fmaxf(ui.ruler_anchor,at);
        if(range[1]>range[0]) { if(id==1) ui.playlist_start=range[0]/STEPS; else ui.piano_start[ui.pattern]=range[0]; }
    } else if(id==1) ui.playlist_start=at/STEPS; else ui.piano_start[ui.pattern]=at;
    if(!ui.playing) ui.reset=1;
}

void timeline_ruler(int id,float start,float span,float gx,float y,float width,float q) {
    float *range=id==1?ui.song_loop:ui.pattern_loop[ui.pattern],point=id==1?ui.playlist_start*STEPS:ui.piano_start[ui.pattern];
    float pixels=width/span;
    DrawRectangle(gx,y,width,14,ui_theme.browser);
    if(range[1]>range[0]) {
        float left=fmaxf(gx,gx+(range[0]-start)*pixels),right=fminf(gx+width,gx+(range[1]-start)*pixels);
        if(right>left) DrawRectangleRec((Rectangle){left,y,right-left,14},Fade(ui_theme.loop,.6f));
    }
    float spacing=timeline_grid_layout(pixels).labels;
    float label_width=text_width(TextFormat("%.0f",fmaxf(0,start+span)/STEPS+1),11)+8;
    while(spacing*pixels<label_width && spacing<1e30f) spacing*=2;
    for(int i=0;i<width/(spacing*pixels)+2;i++) {
        float step=(floorf(start/spacing)+i)*spacing,x=gx+(step-start)*pixels;
        if(step>=0 && x>=gx && x<gx+width) {
            const char *number=TextFormat("%.0f",step/STEPS+1);
            int major=fmodf(step,STEPS*4)==0,size=major?11:10;
            if(x+3+text_width(number,size)<=gx+width) label(number,x+3,y+1,size,major?ink:muted);
        }
    }
    if(hover(gx,y,width,14)) {
        snprintf(ui.status,sizeof ui.status,"Ruler: left-click/drag sets playback start; right-click/drag adjusts the nearest loop edge; double-click clears loop");
        int right=IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        if(right || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            float at=fmaxf(0,snap_round(start+(ui.mouse.x-gx)/pixels,q));
            ruler_press(id,right?MOUSE_BUTTON_RIGHT:MOUSE_BUTTON_LEFT,at,GetTime(),ui.mouse);
            ui.input_enabled=0;
        }
    }
    float x=gx+(point-start)*pixels;
    if(x>=gx && x<=gx+width) DrawTriangle((Vector2){x-5,y+1},(Vector2){x,y+12},(Vector2){x+5,y+1},ui.marker_drag==id?ink:ui_theme.signal);
}

void timeline_grid(float start,float span,float gx,float y,float width,float height) {
    float pixels=width/span;
    TimelineGrid grid=timeline_grid_layout(pixels);
    /* Keep phrase shading anchored to bar 1, including while panning between bars. */
    float band=STEPS*4;
    if(grid.band_alpha>0) for(int i=0;i<span/band+2;i++) {
        float step=(floorf(start/band)+i)*band;
        if(step<0 || fmodf(step/band,2)==0) continue;
        float left=fmaxf(gx,gx+(step-start)*pixels),right=fminf(gx+width,gx+(step+band-start)*pixels);
        if(right>left) DrawRectangleRec((Rectangle){left,y,right-left,height},Fade(BLACK,grid.band_alpha));
    }
    if(!grid.lines) return;
    for(int i=0;i<span/grid.lines+2;i++) {
        float step=(floorf(start/grid.lines)+i)*grid.lines,x=gx+(step-start)*pixels;
        int bar=fmodf(step,STEPS)==0,beat=fmodf(step,4)==0;
        if(step>=0 && x>=gx && x<=gx+width)
            DrawLineEx((Vector2){x,y},(Vector2){x,y+height},1,bar?ui_theme.grid_major:Fade(ui_theme.grid_minor,beat?1:.65f));
    }
}

void follow_view(float *start,float span,double position) {
    if(!ui.follow_playhead || !ui.playing || ui.navigation_active || captured() || ui.windows.grab>=0) return;
    *start=position-span*.5;
}

void track_activity_update(void) {
    audio_pattern_activity(ui.pattern,ui.playing_notes); memset(ui.playing_keys,0,sizeof ui.playing_keys);
    for(int c=0;c<ui.project.channel_count;c++) for(int n=0;n<NOTES;n++) if(ui.playing_notes[c][n] && ui.project.notes[ui.pattern][c][n].velocity) ui.playing_keys[c][ui.project.notes[ui.pattern][c][n].pitch]=1;
    uint8_t channel_triggered[CHANNELS]; audio_channel_activity(ui.channel_active_ui,channel_triggered);
    ui.channel_decay_active=0;
    for(int c=0;c<CHANNELS;c++) {
        ui.channel_flash[c]=channel_triggered[c]?1:ui.channel_flash[c]*expf(-GetFrameTime()*20);
        ui.channel_decay_active|=ui.channel_active_ui[c] || ui.channel_flash[c]>.005f;
    }
    uint8_t triggered[LANES]; audio_track_activity(ui.track_active_ui,triggered);
    float decay=expf(-GetFrameTime()*20);
    for(int l=0;l<LANES;l++) {
        if(!ui.playing || !ui.song) { ui.track_flash[l]=0; ui.track_active_ui[l]=0; }
        else ui.track_flash[l]=triggered[l]?1:ui.track_flash[l]*decay;
    }
}

int clip_edge(int lane,int clip,float bar,float barw) {
    if(clip<0) return 0;
    float left=(bar-ui.project.clip_starts[lane][clip])*barw,right=clip_length(&ui.project,lane,clip)/STEPS*barw-left;
    return fminf(left,right)<7?(left<right?-1:1):0;
}

int picker_double_click(int source) {
    static double last_click=-1;
    static int last_source=-1;
    static unsigned epoch;
    static Vector2 position;
    double now=GetTime();
    int twice=epoch==ui.sample_epoch && source==last_source && now-last_click<.35 &&
        fabsf(ui.mouse.x-position.x)<5 && fabsf(ui.mouse.y-position.y)<5;
    last_click=twice?-1:now; last_source=source; position=ui.mouse; epoch=ui.sample_epoch;
    if(twice) {
        if(source<PATTERNS) { ui.rack_filter=0; ui.rack_scroll=0; windows_focus(&ui.windows,0); }
        else if(AUDIO_SOURCE(source)) { ui.channel=ui.instrument_channel=source-PATTERNS; windows_focus(&ui.windows,4); }
        else ui.automation_selected=source-AUTOMATION_SOURCE;
        ui.picker_drag=-1; ui.browser_focus=0; ui.input_enabled=0;
    }
    return twice;
}

void playlist(float width,float height,float scale) {
    static double last_click=-1;
    static int last_lane=-1,last_clip=-1;
    static Vector2 last_position;
    Rect rect=ui.windows.editors[1].rect;
    int gx=212,gy=PLAYLIST_GRID_TOP; float gridw=width-gx-24,track_area=height-gy-PLAYLIST_BOTTOM,total_height=track_position(LANES);
    ui.track_scroll=fmaxf(0,fminf(total_height-track_area,ui.track_scroll));
    float span=BARS/ui.arrangement.zoom,barw=gridw/span;
    if(ui.song) follow_view(&ui.arrangement.view_start,span,ui.visual_step/STEPS);
    ui.arrangement.range=fmaxf(ui.arrangement.range,fmaxf(ui.arrangement.view_start+span*2,song_steps(&ui.project)/STEPS+span));
    ui.arrangement.snap=grid_interval(barw/STEPS);
    float pointer_bar=ui.arrangement.view_start+(ui.mouse.x-gx)/barw;
    int divider=-1;
    if(hover(120,gy,gx-120,track_area)) {
        float pos=ui.track_scroll+ui.mouse.y-gy; int lane=fminf(LANES-1,fmaxf(0,track_at(pos)));
        if(pos-track_position(lane)<3 && lane>0) divider=lane-1;
        else if(track_position(lane+1)-pos<3) divider=lane;
        if(divider>=0) {
            SetMouseCursor(MOUSE_CURSOR_RESIZE_NS); snprintf(ui.status,sizeof ui.status,"Drag divider to resize Track %d",divider+1);
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.track_resize=divider; ui.track_resize_y=ui.mouse.y+rect.y; ui.track_resize_height=track_height(divider); ui.input_enabled=0; }
        }
    }
    if(divider<0 && hover(gx,gy,gridw,track_area)) {
        int lane=track_at(ui.track_scroll+ui.mouse.y-gy),hit=arrangement_hit(&ui.project,lane,pointer_bar);
        if(ui.arrangement.tool!=CUT && !shortcut_down() && hit>=0 && !recording_source(ui.project.clips[lane][hit]-1) && clip_edge(lane,hit,pointer_bar,barw)) SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
    }
    if(ui.arrangement.gesture==SIZE_CLIP || ui.arrangement.gesture==STRETCH_CLIP) SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
    if(ui.track_resize>=0) SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
    const char *tips[]={"Pencil: place one clip or drag a clip to move it","Brush: drag to paint copies of the last clicked clip","Select: drag a rectangle, then drag the selected clips together","Cut: click to split a clip at the snap position","Stretch: drag an audio edge; uses the sampler Resample/Stretch mode"};
    for(int i=0;i<5;i++) if(tool_button(i,ui.arrangement.tool,8+i*24,tips[i])) ui.arrangement.tool=i;

    const char *picker_names[]={"Patterns","Audio clips","Automation"};
    const int picker_icons[]={ICON_PIANO,ICON_WAVE,ICON_AUTOMATION};
    for(int t=0;t<3;t++) {
        if(button("",4+t*38,57,36,23,ui.picker_tab==t)) { ui.picker_tab=t; ui.picker_scroll=0; ui.picker_drag=-1; if(t==0) { ui.arrangement.source_pattern=ui.pattern; ui.arrangement.source_steps=ui.project.pattern_steps[ui.pattern]; ui.arrangement.source_offset=0; } }
        icon(picker_icons[t],22+t*38,68,18,ui.picker_tab==t?ui_theme.selected_text:ink);
        if(hover(4+t*38,57,36,23)) snprintf(ui.status,sizeof ui.status,"%s%s",picker_names[t],"");
    }
    label("Tracks",128,63,12,muted);
    int audio_ids[CHANNELS],audio_count=0;
    for(int c=0;c<ui.project.channel_count;c++) if(ui.project.channel_audio[c]) audio_ids[audio_count++]=c;
    int picker_count=ui.picker_tab==0?ui.project.pattern_count:ui.picker_tab==1?audio_count:ui.project.automation_count;
    int picker_visible=fmaxf(1,(height-66-gy)/52);
    if(hover(4,gy,112,height-66-gy)) ui.picker_scroll-=GetMouseWheelMove();
    ui.picker_scroll=fmaxf(0,fminf(fmaxf(0,picker_count-picker_visible),ui.picker_scroll));
    for(int row=ui.picker_scroll;row<picker_count;row++) {
        int p=ui.picker_tab==1?audio_ids[row]:row;
        int y=gy+(row-ui.picker_scroll)*52; if(y+50>height-66) break;
        if(ui.picker_tab==2) {
            int source=AUTOMATION_SOURCE+p;
            if(picker_button(y,source_color(source),ui.automation_selected==p)) {
                ui.automation_selected=p; ui.arrangement.source_pattern=source; ui.arrangement.source_steps=ui.project.automations[p].steps; ui.arrangement.source_offset=0; ui.picker_drag=source;
                ui.picker_origin=(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y}; ui.picker_offset=(Vector2){ui.mouse.x-4,ui.mouse.y-y};
            }
            label(fit_text(source_name(source),100,12),8,y+4,12,clip_foreground(source_color(source)));
            automation_curve(p,8,y,104/ui.project.automations[p].steps,50,8,112,0);
            if(hover(4,y,112,50)) {
                snprintf(ui.status,sizeof ui.status,"Automation: select or drag to place; right-click for Rename, Color or Delete");
                if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) open_context(11,p,(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y});
            }
            continue;
        }
        if(ui.picker_tab==1) {
            if(picker_button(y,audio_color(p),ui.arrangement.source_pattern==PATTERNS+p) && !recording_source(PATTERNS+p)) {
                ui.arrangement.source_pattern=PATTERNS+p; ui.arrangement.source_steps=ui.project.audio_seconds[p]; ui.arrangement.source_offset=0; ui.picker_drag=PATTERNS+p;
                ui.picker_origin=(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y};
                ui.picker_offset=(Vector2){ui.mouse.x-4,ui.mouse.y-y};
                picker_double_click(PATTERNS+p);
            }
            if(ui.arrangement.source_pattern==PATTERNS+p) DrawRectangleLinesEx((Rectangle){4,y,112,50},2,WHITE);
            label(fit_text(ui.project.channel_names[p],100,12),8,y+4,12,clip_foreground(audio_color(p)));
            audio_waveform(p,8,y+20,8,112,104/fmaxf(.001f,audio_view_steps(p)),28);
            if(hover(4,y,112,50)) {
                snprintf(ui.status,sizeof ui.status,"Audio clip: double-click for sampler; select or drag to place; right-click for Rename, Color or Delete");
                if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) open_context(7,p,(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y});
            }
            continue;
        }
        if(picker_button(y,pattern_color(p),ui.pattern==p)) {
            if(ui.pattern!=p) ui.reset=1;
            ui.pattern=p; ui.arrangement.source_pattern=p; ui.arrangement.source_steps=ui.project.pattern_steps[p]; ui.arrangement.source_offset=0; ui.picker_drag=p;
            ui.picker_origin=(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y};
            ui.picker_offset=(Vector2){ui.mouse.x-4,ui.mouse.y-y};
            picker_double_click(p);
        }
        if(ui.pattern==p) DrawRectangleLinesEx((Rectangle){4,y,112,50},2,WHITE);
        label(fit_text(ui.project.pattern_names[p],100,12),8,y+4,12,clip_foreground(pattern_color(p)));
        picker_notes(p,4,y);
        if(hover(4,y,112,50)) {
            snprintf(ui.status,sizeof ui.status,"Pattern: double-click for Channel Rack; select or drag to place; right-click for Rename, Color or Delete");
            if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { ui.pattern=p; ui.arrangement.source_pattern=p; ui.arrangement.source_steps=ui.project.pattern_steps[p]; ui.reset=1; open_context(4,p,(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y}); }
        }
    }
    if(ui.sample_drag[0] && ui.sample_moved && CheckCollisionPointRec(ui.mouse,(Rectangle){4,gy,112,height-gy-66}) && windows_hit(&ui.windows,ui.mouse.x+rect.x,ui.mouse.y+rect.y)==1) {
        DrawRectangleLinesEx((Rectangle){4,gy,112,height-gy-66},2,ui_theme.signal);
        snprintf(ui.status,sizeof ui.status,"Release to import into the Audio list without placing a clip");
    }
    if(ui.picker_tab==2 && !ui.project.automation_count) { label("No automation",8,gy+8,12,muted); label("Right-click a control",8,gy+28,11,muted); }
    if(ui.picker_tab==1 && !audio_count) { label("No audio clips",8,gy+8,12,muted); label("Drop on Playlist",8,gy+28,11,muted); }
    if(ui.picker_tab==0 && button("+",4,height-62,112,20,0)) {
        if(ui.project.pattern_count<PATTERNS) {
            ui.pattern=ui.project.pattern_count++; ui.arrangement.source_pattern=ui.pattern; ui.arrangement.source_steps=STEPS; memset(ui.project.notes[ui.pattern],0,sizeof ui.project.notes[ui.pattern]);
            ui.project.pattern_steps[ui.pattern]=STEPS; memset(ui.piano_channels[ui.pattern],0,sizeof ui.piano_channels[ui.pattern]);
            for(int n=1;n<=PATTERNS+1;n++) {
                snprintf(ui.project.pattern_names[ui.pattern],PATTERN_NAME,"Pattern %d",n);
                int duplicate=0;
                for(int i=0;i<ui.pattern;i++) if(!strcmp(ui.project.pattern_names[i],ui.project.pattern_names[ui.pattern])) duplicate=1;
                if(!duplicate) break;
            }
            ui.project.pattern_colors[ui.pattern]=next_source_color(ui.project.pattern_colors,ui.pattern);
            ui.rack_view[ui.pattern]=ui.rack_range[ui.pattern]=0; ui.reset=1;
            ui.picker_scroll=fmaxf(0,ui.project.pattern_count-picker_visible);
        } else snprintf(ui.status,sizeof ui.status,"This prototype supports %d patterns.",PATTERNS);
    }
    if(ui.picker_tab==0 && hover(4,height-62,112,20) && ui.project.pattern_count<PATTERNS) snprintf(ui.status,sizeof ui.status,"Add a new blank pattern");
    timeline_ruler(1,ui.arrangement.view_start*STEPS,span*STEPS,gx,gy-14,gridw,ui.arrangement.snap);
    for(int l=fmaxf(0,track_at(ui.track_scroll));l<LANES;l++) {
        float y=gy+track_position(l)-ui.track_scroll,rowh=track_height(l);
        if(y>=height-PLAYLIST_BOTTOM) break;
        BeginScissorMode(rect.x*scale,(rect.y+gy)*scale,gx*scale,track_area*scale);
        DrawRectangleRec((Rectangle){120,y,gx-122,rowh-1},cell); label(fit_text(ui.project.track_names[l],gx-142,12),128,y+fmaxf(1,(rowh-12)/2),12,ink);
        float brightness=fmaxf(ui.track_active_ui[l]?.32f:0,ui.track_flash[l]);
        Rectangle activity={gx-9,y+1,8,rowh-2};
        DrawRectangleRec(activity,ui_theme.track);
        if(brightness>.005f) DrawRectangleRec(activity,Fade(WHITE,brightness));
        if(ui.selected_track==l) DrawRectangleLinesEx(activity,1,accent);
        mute_light(gx-18,y+rowh-10,ui.project.lane_mute,LANES,l,TextFormat("Playlist track %d",l+1));
        BeginScissorMode((rect.x+gx)*scale,(rect.y+gy)*scale,gridw*scale,track_area*scale);
        DrawRectangleRec((Rectangle){gx,y,gridw,rowh},ui_theme.track);
        timeline_grid(ui.arrangement.view_start*STEPS,span*STEPS,gx,y,gridw,rowh-1);
        for(int b=0;b<CLIPS;b++) if(ui.project.clips[l][b]) {
            int pat=ui.project.clips[l][b]-1; float x=gx+(ui.project.clip_starts[l][b]-ui.arrangement.view_start)*barw,w=playlist_clip_length(l,b)*barw/STEPS;
            if(w<=0 || x>gx+gridw || x+w<gx) continue;
            float left=fmaxf(gx,x),right=fminf(gx+gridw,x+w);
            Rectangle bounds={left,y,fmaxf(1,right-left),rowh-1};
            float top=fmaxf(gy,y),bottom=fminf(height-PLAYLIST_BOTTOM,y+rowh-1);
            DrawRectangleRec(bounds,source_color(pat));
            const char *name=source_name(pat); if(pat<PATTERNS && !strcmp(name,TextFormat("Pattern %d",pat+1))) name=TextFormat("P%d",pat+1);
            if(x>=gx) label(fit_text(name,fminf(gridw,w-9),10),x+3,y+2,10,clip_foreground(source_color(pat)));
            float clip_left=floorf((rect.x+left)*scale),clip_top=floorf((rect.y+top)*scale);
            /* Cached pattern previews already crop their geometry to the clip.
               Keep per-clip scissors for curves, waveforms and vector fallbacks. */
            int clip_scissor=pat>=PATTERNS || !ui.previews[pat].valid;
            if(clip_scissor) BeginScissorMode(clip_left,clip_top,ceilf((rect.x+left+bounds.width)*scale)-clip_left,fmaxf(0,ceilf((rect.y+bottom)*scale)-clip_top));
            float offset=clip_offset_steps(&ui.project,l,b),origin=x-offset*barw/STEPS;
            if(pat>=AUTOMATION_SOURCE) automation_curve(pat-AUTOMATION_SOURCE,origin,y,barw/STEPS,rowh,left,right,1);
            else if(AUDIO_SOURCE(pat)) audio_waveform_at(pat-PATTERNS,x,y,left,right,barw/STEPS,rowh,l,b);
            else clip_preview(pat,origin,y,left,right,barw/STEPS,clip_length(&ui.project,l,b)+offset,rowh);
            if(clip_scissor) BeginScissorMode((rect.x+gx)*scale,(rect.y+gy)*scale,gridw*scale,track_area*scale);
            int selected=ui.arrangement.gesture==MOVE_CLIPS?ui.arrangement.moved[l][b]:ui.arrangement.selected[l][b];
            DrawRectangleLinesEx(bounds,selected?2:1,selected?WHITE:clip_foreground(source_color(pat)));
        }
        BeginScissorMode(rect.x*scale,rect.y*scale,rect.w*scale,rect.h*scale);
    }
    BeginScissorMode((rect.x+gx)*scale,(rect.y+gy)*scale,gridw*scale,track_area*scale);
    for(int l=fmaxf(0,track_at(ui.track_scroll));l<LANES;l++) {
        float y=gy+track_position(l)-ui.track_scroll,rowh=track_height(l);
        if(y>=height-PLAYLIST_BOTTOM) break;
        float bottom=roundf((y+rowh)*scale+ui.text_origin.y);
        DrawRectangleRec((Rectangle){gx,(bottom-1-ui.text_origin.y)/scale,gridw,1/scale},ui_theme.grid_major);
        BeginScissorMode((rect.x+120)*scale,(rect.y+gy)*scale,(gx-120)*scale,track_area*scale);
        DrawRectangleRec((Rectangle){120,(bottom-1-ui.text_origin.y)/scale,gx-120,1/scale},divider==l || ui.track_resize==l?accent:ui_theme.border);
        BeginScissorMode((rect.x+gx)*scale,(rect.y+gy)*scale,gridw*scale,track_area*scale);
    }
    BeginScissorMode(rect.x*scale,rect.y*scale,rect.w*scale,rect.h*scale);
    if(ui.picker_drag>=0 && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if(CheckCollisionPointRec(ui.mouse,(Rectangle){gx,gy,gridw,track_area}) && windows_hit(&ui.windows,ui.mouse.x+rect.x,ui.mouse.y+rect.y)==1) {
            float start=snap_floor(pointer_bar*STEPS,ui.arrangement.snap)/STEPS;
            arrangement_place(&ui.project,track_at(ui.track_scroll+ui.mouse.y-gy),start,ui.picker_drag,clip_source_steps(&ui.project,ui.picker_drag));
        }
        ui.picker_drag=-1;
    }
    /* Curve editing owns the body; the name strip keeps normal clip gestures. */
    if(ui.automation_node>=0) {
        if(ui.automation_selected<0 || ui.automation_selected>=ui.project.automation_count || ui.automation_lane<0 || ui.automation_clip<0 || ui.project.clips[ui.automation_lane][ui.automation_clip]!=AUTOMATION_SOURCE+ui.automation_selected+1) ui.automation_node=-1;
        else {
            Automation *a=&ui.project.automations[ui.automation_selected];
            float y=gy+track_position(ui.automation_lane)-ui.track_scroll,rowh=track_height(ui.automation_lane);
            float origin=gx+(ui.project.clip_starts[ui.automation_lane][ui.automation_clip]-ui.arrangement.view_start)*barw-clip_offset_steps(&ui.project,ui.automation_lane,ui.automation_clip)*barw/STEPS;
            if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                float step=(ui.mouse.x-ui.automation_grab_x-origin)/barw*STEPS;
                step=snap_round(step,ui.arrangement.snap);
                int lane=ui.automation_lane,clip=ui.automation_clip;
                float offset=ui.project.clip_offsets[lane][clip],length=clip_length(&ui.project,lane,clip);
                step=fmaxf(offset-ui.project.clip_starts[lane][clip]*STEPS,step);
                float value=fmaxf(0,fminf(1,1-(ui.mouse.y-y-19)/fmaxf(1,rowh-25)));
                int moved=automation_move_point(&ui.project,ui.automation_selected,ui.automation_node,step,value);
                if(moved>=0) {
                    ui.automation_node=moved; step=a->points[moved].step; offset=ui.project.clip_offsets[lane][clip];
                    if(step<offset) { float delta=offset-step; ui.project.clip_starts[lane][clip]-=delta/STEPS; ui.project.clip_offsets[lane][clip]=step; length+=delta; }
                    else if(step>offset+length) length=step-offset;
                    ui.project.clip_steps[lane][clip]=length;
                    ui.arrangement.source_pattern=AUTOMATION_SOURCE+ui.automation_selected; ui.arrangement.source_steps=length; ui.arrangement.source_offset=ui.project.clip_offsets[lane][clip];
                }
            } else ui.automation_node=-1;
        }
    }
    if(ui.automation_node<0 && ui.arrangement.tool==PENCIL && !shortcut_down() && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT) && hover(gx,gy,gridw,track_area)) {
        int lane=track_at(ui.track_scroll+ui.mouse.y-gy),slot=arrangement_hit(&ui.project,lane,pointer_bar);
        float y=gy+track_position(lane)-ui.track_scroll,rowh=track_height(lane);
        if(slot>=0 && ui.project.clips[lane][slot]>AUTOMATION_SOURCE && ui.mouse.y>=y+15) {
            int index=ui.project.clips[lane][slot]-AUTOMATION_SOURCE-1; Automation *a=&ui.project.automations[index];
            float origin=gx+(ui.project.clip_starts[lane][slot]-ui.arrangement.view_start)*barw-clip_offset_steps(&ui.project,lane,slot)*barw/STEPS;
            int hit=-1; float nearest=49;
            for(int n=0;n<a->count;n++) {
                float dx=ui.mouse.x-origin-a->points[n].step*barw/STEPS,dy=ui.mouse.y-y-19-(1-a->points[n].value)*fmaxf(1,rowh-25),distance=dx*dx+dy*dy;
                if(distance<nearest) { hit=n; nearest=distance; }
            }
            snprintf(ui.status,sizeof ui.status,"Automation: click to add a point; drag points; right-click a point to delete; drag the title to move the clip");
            if(hit>=0 || !clip_edge(lane,slot,pointer_bar,barw)) {
                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    ui.automation_selected=index;
                    if(hit<0) {
                                float step=(ui.mouse.x-origin)/barw*STEPS;
                        step=snap_round(step,ui.arrangement.snap);
                        hit=automation_point(a,fmaxf(0,fminf(a->steps,step)),fmaxf(0,fminf(1,1-(ui.mouse.y-y-19)/fmaxf(1,rowh-25))));
                    }
                    ui.automation_node=hit; ui.automation_lane=lane; ui.automation_clip=slot;
                    if(hit>=0) ui.automation_grab_x=ui.mouse.x-origin-a->points[hit].step*barw/STEPS;
                    memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected); ui.arrangement.selected[lane][slot]=1; ui.input_enabled=0;
                }
                if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    if(hit>0 && hit<a->count-1) { memmove(&a->points[hit],&a->points[hit+1],(a->count-hit-1)*sizeof a->points[0]); a->count--; }
                    ui.input_enabled=0;
                }
            }
        }
    }
    if(divider<0 && hover(120,gy,gx+gridw-120,track_area)) {
        float bx=ui.mouse.x<gx?-1:ui.arrangement.view_start+(ui.mouse.x-gx)/barw,ly=track_at(ui.track_scroll+ui.mouse.y-gy); int hit=arrangement_hit(&ui.project,(int)ly,bx);
        if(bx<0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.selected_track=(int)ly;
        int live=hit>=0 && recording_source(ui.project.clips[(int)ly][hit]-1);
        int edge=shortcut_down() || live || ui.arrangement.tool==CUT?0:clip_edge((int)ly,hit,bx,barw);
        if(edge) { SetMouseCursor(MOUSE_CURSOR_RESIZE_EW); snprintf(ui.status,sizeof ui.status,"%.100s · %.2f s | Drag edge to resize",source_name(ui.project.clips[(int)ly][hit]-1),playlist_clip_length((int)ly,hit)*15/ui.project.bpm); }
        else if(ui.mouse.x<gx) snprintf(ui.status,sizeof ui.status,"Track %d: drag to select tracks; right-click to rename",(int)ly+1);
        else if(hit>=0) {
            int source=ui.project.clips[(int)ly][hit]-1;
            snprintf(ui.status,sizeof ui.status,"%.100s · %.2f s | %s",source_name(source),playlist_clip_length((int)ly,hit)*15/ui.project.bpm,source>=AUTOMATION_SOURCE?"Drag title to move; body to edit":AUDIO_SOURCE(source)?"Double-click: Sampler; drag: move; right-click: erase":"Double-click: Channel Rack; drag: move; right-click: erase");
        }
        else snprintf(ui.status,sizeof ui.status,"Playlist: %s; Shift adds to selection, right-drag erases",ui.arrangement.tool==PENCIL?"place or move a clip":ui.arrangement.tool==BRUSH?"paint copies of the current pattern":"select and move clips");
        if(ui.mouse.x>=gx && ui.arrangement.tool==CUT) snprintf(ui.status,sizeof ui.status,"Cut: click a clip to split at the snap position");
        if(ui.mouse.x>=gx && ui.arrangement.tool==STRETCH) snprintf(ui.status,sizeof ui.status,"Stretch: drag either audio edge; Resample changes pitch, Stretch preserves pitch");
        if(ui.mouse.x<gx && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            ui.rename_channel=ui.rename_mixer=-1; ui.rename_track=(int)ly; snprintf(ui.rename_text,sizeof ui.rename_text,"%s",ui.project.track_names[ui.rename_track]); ui.rename_select_all=1; open_popup(2); ui.input_enabled=0; return;
        }
        if(!live && ui.mouse.x>=gx && hit>=0 && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) &&
           ui.mouse.y<gy+track_position((int)ly)-ui.track_scroll+14) {
            int source=ui.project.clips[(int)ly][hit]-1;
            if(source<PATTERNS) { ui.pattern=source; open_context(4,source,(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y}); }
            else if(AUDIO_SOURCE(source)) open_context(7,source-PATTERNS,(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y});
            else open_context(11,source-AUTOMATION_SOURCE,(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y});
            ui.arrangement.gesture=IDLE; last_click=-1; return;
        }
        if(shortcut_down() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            arrangement_select_press(&ui.arrangement,bx,ly,IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
            last_click=-1; ui.input_enabled=0;
        } else if(!live && !recording_source(ui.arrangement.source_pattern) && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))) {
            int plain_left=ui.arrangement.tool!=CUT && ui.arrangement.tool!=STRETCH && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT) && hit>=0 && (!edge || ui.project.clips[(int)ly][hit]>PATTERNS);
            double now=GetTime();
            if(plain_left && last_lane==(int)ly && last_clip==hit && now-last_click<.35 && fabsf(ui.mouse.x-last_position.x)<5 && fabsf(ui.mouse.y-last_position.y)<5) {
                int source=ui.project.clips[(int)ly][hit]-1;
                if(source<PATTERNS) { ui.pattern=source; ui.reset=1; }
                else if(AUDIO_SOURCE(source)) { ui.channel=ui.instrument_channel=source-PATTERNS; ui.rack_filter=1; ui.rack_scroll=0; }
                else ui.automation_selected=source-AUTOMATION_SOURCE;
                ui.arrangement.gesture=IDLE; windows_focus(&ui.windows,source>=AUTOMATION_SOURCE?1:AUDIO_SOURCE(source)?4:0); last_click=-1;
                ui.input_enabled=0; return;
            }
            last_click=plain_left?now:-1; last_lane=(int)ly; last_clip=hit; last_position=ui.mouse;
            arrangement_press(&ui.arrangement,&ui.project,bx,ly,IsMouseButtonPressed(MOUSE_BUTTON_RIGHT),edge,ui.pattern,IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
            if(ui.arrangement.source_pattern>=0 && ui.arrangement.source_pattern<PATTERNS && ui.arrangement.source_pattern!=ui.pattern) { ui.pattern=ui.arrangement.source_pattern; ui.reset=1; }
            ui.input_enabled=0;
        }
    }
    BeginScissorMode((rect.x+gx)*scale,(rect.y+gy)*scale,gridw*scale,(height-gy-PLAYLIST_BOTTOM)*scale);
    if(ui.arrangement.tool==CUT && !shortcut_down() && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT) && hover(gx,gy,gridw,track_area)) {
        int lane=track_at(ui.track_scroll+ui.mouse.y-gy),hit=arrangement_hit(&ui.project,lane,pointer_bar);
        if(hit>=0 && !recording_source(ui.project.clips[lane][hit]-1)) {
            float cut=snap_floor(pointer_bar*STEPS+.00001f,ui.arrangement.snap);
            float start=ui.project.clip_starts[lane][hit]*STEPS,end=start+clip_length(&ui.project,lane,hit);
            if(cut>start+.0001f && cut<end-.0001f) {
                float x=gx+(cut/STEPS-ui.arrangement.view_start)*barw,y=gy+track_position(lane)-ui.track_scroll;
                x=roundf((rect.x+x)*scale)/scale-rect.x;
                DrawRectangleRec((Rectangle){x,y,1/scale,track_height(lane)-1},ui_theme.signal);
            }
        }
    }
    if(ui.picker_drag>=0 && CheckCollisionPointRec(ui.mouse,(Rectangle){gx,gy,gridw,track_area}) && windows_hit(&ui.windows,ui.mouse.x+rect.x,ui.mouse.y+rect.y)==1) {
        float start=snap_floor(pointer_bar*STEPS,ui.arrangement.snap)/STEPS;
        int lane=track_at(ui.track_scroll+ui.mouse.y-gy);
        Rectangle ghost={gx+(start-ui.arrangement.view_start)*barw,gy+track_position(lane)-ui.track_scroll,fmaxf(4,clip_source_steps(&ui.project,ui.picker_drag)*barw/STEPS),track_height(lane)-1};
        DrawRectangleRec(ghost,Fade(ui_theme.signal,.2f)); DrawRectangleLinesEx(ghost,1,ui_theme.signal);
        snprintf(ui.status,sizeof ui.status,"Release to place %s",source_name(ui.picker_drag));
    }
    if(ui.sample_drag[0] && ui.sample_moved && ui.mouse.x>=gx && ui.mouse.x<gx+gridw && ui.mouse.y>=gy && ui.mouse.y<height-PLAYLIST_BOTTOM && windows_hit(&ui.windows,ui.mouse.x+rect.x,ui.mouse.y+rect.y)==1) {
        float start=snap_floor(pointer_bar*STEPS,ui.arrangement.snap)/STEPS;
        float length=ui.audition.frames/(float)RATE*ui.project.bpm/15;
        int lane=fminf(LANES-1,fmaxf(0,track_at(ui.track_scroll+ui.mouse.y-gy)));
        Rectangle ghost={gx+(start-ui.arrangement.view_start)*barw,gy+track_position(lane)-ui.track_scroll,fmaxf(4,length*barw/STEPS),track_height(lane)-1};
        int hit=arrangement_hit(&ui.project,lane,pointer_bar);
        if(hit>=0 && AUDIO_SOURCE(ui.project.clips[lane][hit]-1)) {
            ghost.x=gx+(ui.project.clip_starts[lane][hit]-ui.arrangement.view_start)*barw;
            ghost.width=fmaxf(4,playlist_clip_length(lane,hit)*barw/STEPS);
            snprintf(ui.status,sizeof ui.status,"Release to replace %s's sample",source_name(ui.project.clips[lane][hit]-1));
            DrawRectangleLinesEx(ghost,1,ui_theme.signal);
        } else {
            DrawRectangleRec(ghost,Fade(ui_theme.signal,.2f)); DrawRectangleLinesEx(ghost,1,ui_theme.signal);
        }
    }
    if(ui.arrangement.gesture==BOX_SELECT) {
        float x=gx+(fminf(ui.arrangement.x,ui.arrangement.now_x)-ui.arrangement.view_start)*barw,y=gy+track_position(fminf(ui.arrangement.y,ui.arrangement.now_y))-ui.track_scroll;
        Rectangle box={x,y,fabsf(ui.arrangement.x-ui.arrangement.now_x)*barw,fabsf(track_position(ui.arrangement.y)-track_position(ui.arrangement.now_y))};
        DrawRectangleRec(box,Fade(ui_theme.signal,.15f)); DrawRectangleLinesEx(box,1,ui_theme.signal);
    }
    BeginScissorMode(rect.x*scale,rect.y*scale,rect.w*scale,rect.h*scale);
    if(ui.playing && ui.song) { float x=gx+(ui.visual_step/STEPS-ui.arrangement.view_start)*barw; if(x>=gx && x<=gx+gridw) DrawLineEx((Vector2){x,gy},(Vector2){x,height-PLAYLIST_BOTTOM},1.5f,ui_theme.signal); }
    float rail_top=gy-14,rail_height=track_area+14;
    float vthumb=timeline_thumb(rail_height,track_area,total_height),travel_y=rail_height-vthumb,maximum_y=fmaxf(0,total_height-track_area),vy=rail_top+(maximum_y>0?ui.track_scroll/maximum_y*travel_y:0);
    DrawRectangleRec((Rectangle){width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,rail_height},bg); ui_surface((Rectangle){width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,vy,EDITOR_SCROLLBAR,fmaxf(12,vthumb)},ui.playlist_vpan || hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,rail_height)?accent:muted);
    if(hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,rail_height)) snprintf(ui.status,sizeof ui.status,"Drag to scroll through the 100 Playlist tracks");
    if(hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,rail_height) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if(ui.mouse.y<vy || ui.mouse.y>vy+vthumb) ui.track_scroll=fmaxf(0,fminf(maximum_y,(ui.mouse.y-rail_top-vthumb/2)/fmaxf(1,travel_y)*maximum_y));
        ui.playlist_vpan=1; ui.playlist_vpan_y=ui.mouse.y+rect.y; ui.playlist_vpan_start=ui.track_scroll;
    }
    float rail_width=width-4-EDITOR_CORNER-gx,thumbw=timeline_thumb(rail_width,span,ui.arrangement.range),travel=rail_width-thumbw,maximum=ui.arrangement.range-span,thumbx=gx+(maximum>0?fmaxf(0,ui.arrangement.view_start)/maximum*travel:0);
    DrawRectangleRec((Rectangle){gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR},bg); ui_surface((Rectangle){thumbx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,thumbw,EDITOR_SCROLLBAR},ui.playlist_pan || hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR)?accent:muted);
    if(hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR)) snprintf(ui.status,sizeof ui.status,"Drag to scroll along the Playlist timeline");
    if(hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if(ui.mouse.x<thumbx || ui.mouse.x>thumbx+thumbw) ui.arrangement.view_start=fmaxf(0,fminf(maximum,(ui.mouse.x-gx-thumbw/2)/fmaxf(1,travel)*maximum));
        ui.playlist_pan=1; ui.playlist_pan_x=ui.mouse.x+rect.x; ui.playlist_pan_start=ui.arrangement.view_start; ui.playlist_pan_range=maximum/fmaxf(1,travel);
    }
    row_zoom_button(1,width,gy-14-EDITOR_CORNER);

}
