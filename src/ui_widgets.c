// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"

void fonts_update(float scale) {
    if(ui.font_scale==scale) return;
    ui.font_scale=scale; icons_init(scale); text_fonts_update(ui.font_path,scale);
}

int text_width(const char *text,int size) {
    return (int)ceilf(text_font_width(text,size));
}

float raster_position(float value,float origin) {
    return text_pixel_position(value,origin,ui_scale(),ui.font_scale);
}

void label_moving(const char *text,float x,float y,int size,Color color) {
    Font font=text_font(text,size);
    DrawTextEx(font,text,(Vector2){x,y},font.baseSize/ui.font_scale,0,color);
}

void label(const char *text,float x,float y,int size,Color color) {
    label_moving(text,raster_position(x,ui.text_origin.x),raster_position(y,ui.text_origin.y),size,color);
}

void backspace(char *text) {
    size_t n=strlen(text); if(!n) return;
    do { n--; } while(n && ((unsigned char)text[n]&0xc0)==0x80);
    text[n]=0;
}

const char *fit_text(const char *text,int width,int size) {
    static char fitted[PATH_MAX]; text_fit(fitted,sizeof fitted,text,width,size); return fitted;
}

void circles_init(void) {
    Image image=GenImageColor(64,64,WHITE); Color *pixels=image.data;
    for(int y=0;y<64;y++) for(int x=0;x<64;x++) {
        float dx=x+.5f-32,dy=y+.5f-32;
        pixels[y*64+x].a=255*fmaxf(0,fminf(1,(32-sqrtf(dx*dx+dy*dy))/3));
    }
    ui.circle_texture=LoadTextureFromImage(image); UnloadImage(image);
    SetTextureFilter(ui.circle_texture,TEXTURE_FILTER_BILINEAR);
}

void circle(float x,float y,float radius,Color color) {
    DrawTexturePro(ui.circle_texture,(Rectangle){0,0,64,64},(Rectangle){x-radius,y-radius,radius*2,radius*2},(Vector2){0},0,color);
}

void icons_init(float scale) {
    if(ui.icons.id) UnloadTexture(ui.icons);
    ui.icon_large=fmaxf(1,roundf(28*scale)); ui.icon_small=fmaxf(1,roundf(16*scale));
    ui.icon_medium=fmaxf(1,roundf(20*scale));
    int high=ui.icon_large*4;
    RenderTexture2D target=LoadRenderTexture(ICON_COUNT*high,high);
    BeginTextureMode(target); ClearBackground((Color){255,255,255,0});
    for(int id=0;id<ICON_COUNT;id++) {
        BeginMode2D((Camera2D){.offset={id*high,0},.zoom=high/28.f});
        if(id==ICON_RACK) for(int row=0;row<3;row++) {
            DrawRectangle(5,7+row*6,4,3,WHITE);
            for(int step=0;step<3;step++) DrawRectangle(12+step*4,7+row*6,3,3,WHITE);
        }
        if(id==ICON_PLAYLIST) for(int row=0;row<3;row++) {
            DrawLineEx((Vector2){5,8+row*6},(Vector2){23,8+row*6},1,Fade(WHITE,.45f));
            DrawRectangle(5+(row%2)*7,6+row*6,10,4,WHITE);
        }
        if(id==ICON_PIANO) {
            DrawRectangleLinesEx((Rectangle){4,6,20,16},1.5f,WHITE);
            for(int key=1;key<4;key++) DrawLineEx((Vector2){4+key*5,6},(Vector2){4+key*5,22},1,WHITE);
            for(int key=0;key<2;key++) DrawRectangle(7+key*5,6,4,9,WHITE);
            DrawRectangle(21,6,3,9,WHITE); /* Keep the partial F-sharp distinct from the border at small sizes. */
        }
        if(id==ICON_MIXER) for(int strip=0;strip<3;strip++) {
            int x=7+strip*7,y=9+strip*4;
            DrawLineEx((Vector2){x,5},(Vector2){x,23},1.5f,WHITE);
            DrawRectangle(x-3,y-2,6,4,WHITE);
        }
        if(id==ICON_PENCIL) {
            DrawLineEx((Vector2){8,20},(Vector2){21,7},3.5f,WHITE);
            DrawTriangle((Vector2){5,23},(Vector2){10,21},(Vector2){7,18},WHITE);
        }
        if(id==ICON_BRUSH) {
            DrawLineEx((Vector2){14,14},(Vector2){21,6},3,WHITE);
            DrawCircle(11,17,4,WHITE);
            DrawTriangle((Vector2){5,23},(Vector2){13,21},(Vector2){8,15},WHITE);
        }
        if(id==ICON_SELECT) for(int i=0;i<3;i++) {
            int p=6+i*6;
            DrawLineEx((Vector2){p,6},(Vector2){p+3,6},1.5f,WHITE);
            DrawLineEx((Vector2){p,22},(Vector2){p+3,22},1.5f,WHITE);
            DrawLineEx((Vector2){6,p},(Vector2){6,p+3},1.5f,WHITE);
            DrawLineEx((Vector2){22,p},(Vector2){22,p+3},1.5f,WHITE);
        }
        if(id==ICON_CUT) {
            DrawCircleLines(8,19,4,WHITE); DrawCircleLines(20,19,4,WHITE);
            DrawLineEx((Vector2){10,16},(Vector2){22,5},2,WHITE);
            DrawLineEx((Vector2){18,16},(Vector2){6,5},2,WHITE);
            DrawCircle(14,12,1.5f,WHITE);
        }
        if(id==ICON_STRETCH) {
            DrawLineEx((Vector2){5,14},(Vector2){23,14},2,WHITE);
            DrawLineEx((Vector2){5,14},(Vector2){10,9},2,WHITE);
            DrawLineEx((Vector2){5,14},(Vector2){10,19},2,WHITE);
            DrawLineEx((Vector2){23,14},(Vector2){18,9},2,WHITE);
            DrawLineEx((Vector2){23,14},(Vector2){18,19},2,WHITE);
        }
        if(id==ICON_ROW_ZOOM) {
            DrawLineEx((Vector2){14,5},(Vector2){14,23},1.75f,WHITE);
            DrawLineEx((Vector2){9,10},(Vector2){14,5},1.75f,WHITE);
            DrawLineEx((Vector2){19,10},(Vector2){14,5},1.75f,WHITE);
            DrawLineEx((Vector2){9,18},(Vector2){14,23},1.75f,WHITE);
            DrawLineEx((Vector2){19,18},(Vector2){14,23},1.75f,WHITE);
        }
        if(id==ICON_METRO) {
            DrawLineEx((Vector2){7,22},(Vector2){11,6},1.8f,WHITE);
            DrawLineEx((Vector2){11,6},(Vector2){16,22},1.8f,WHITE);
            DrawLineEx((Vector2){7,22},(Vector2){16,22},1.8f,WHITE);
            DrawLineEx((Vector2){12,19},(Vector2){21,8},1.8f,WHITE); DrawCircle(21,8,2,WHITE);
        }
        if(id==ICON_FOLLOW) {
            DrawLineEx((Vector2){7,5},(Vector2){7,23},1.5f,WHITE);
            DrawLineEx((Vector2){12,14},(Vector2){23,14},1.8f,WHITE);
            DrawLineEx((Vector2){18,9},(Vector2){23,14},1.8f,WHITE);
            DrawLineEx((Vector2){18,19},(Vector2){23,14},1.8f,WHITE);
        }
        if(id==ICON_KEYS) {
            DrawRectangleLinesEx((Rectangle){3,7,22,14},1.5f,WHITE);
            for(int row=0;row<2;row++) for(int key=0;key<5;key++) DrawRectangle(6+key*3.5f,10+row*4,2,2,WHITE);
            DrawRectangleRec((Rectangle){9,18,10,1.5f},WHITE);
        }
        if(id==ICON_WAVE) {
            Vector2 previous={4,14};
            for(int step=1;step<=48;step++) {
                float phase=step/48.f;
                Vector2 next={4+20*phase,14-7*sinf(phase*2*PI)};
                DrawLineEx(previous,next,2,WHITE); previous=next;
            }
        }
        if(id==ICON_AUTOMATION) { DrawLineEx((Vector2){5,21},(Vector2){14,7},2,WHITE); DrawLineEx((Vector2){14,7},(Vector2){23,17},2,WHITE); DrawCircle(5,21,3,WHITE); DrawCircle(14,7,3,WHITE); DrawCircle(23,17,3,WHITE); }
        if(id==ICON_PLAY) DrawTriangle((Vector2){9,6},(Vector2){9,22},(Vector2){22,14},WHITE);
        if(id==ICON_BACK) DrawTriangle((Vector2){19,6},(Vector2){6,14},(Vector2){19,22},WHITE);
        if(id==ICON_STOP) DrawRectangle(7,7,14,14,WHITE);
        if(id==ICON_PAUSE) { DrawRectangle(7,6,5,16,WHITE); DrawRectangle(16,6,5,16,WHITE); }
        if(id==ICON_CLOSE) {
            DrawLineEx((Vector2){7,7},(Vector2){21,21},2,WHITE);
            DrawLineEx((Vector2){7,21},(Vector2){21,7},2,WHITE);
        }
        EndMode2D();
    }
    EndTextureMode();
    Image source=LoadImageFromTexture(target.texture); ImageFlipVertical(&source);
    Image image=GenImageColor(ICON_COUNT*ui.icon_large,ui.icon_large+ui.icon_medium+ui.icon_small,BLANK);
    Color *input=source.data,*output=image.data;
    /* Exact area coverage avoids the blur of bicubic resize and a second filter. */
    for(int variant=0;variant<3;variant++) {
        int size=variant==2?ui.icon_small:variant==1?ui.icon_medium:ui.icon_large,row=variant==2?ui.icon_large+ui.icon_medium:variant==1?ui.icon_large:0;
        float ratio=high/(float)size;
        for(int id=0;id<ICON_COUNT;id++) for(int y=0;y<size;y++) for(int x=0;x<size;x++) {
            float left=x*ratio,top=y*ratio,right=(x+1)*ratio,bottom=(y+1)*ratio,alpha=0;
            for(int sy=floorf(top);sy<ceilf(bottom) && sy<high;sy++) for(int sx=floorf(left);sx<ceilf(right) && sx<high;sx++) {
                float weight=(fminf(right,sx+1)-fmaxf(left,sx))*(fminf(bottom,sy+1)-fmaxf(top,sy));
                alpha+=input[sy*source.width+id*high+sx].a*weight;
            }
            output[(row+y)*image.width+id*ui.icon_large+x]=(Color){255,255,255,roundf(alpha/(ratio*ratio))};
        }
    }
    ui.icons=LoadTextureFromImage(image); SetTextureFilter(ui.icons,TEXTURE_FILTER_POINT);
    UnloadImage(source); UnloadImage(image); UnloadRenderTexture(target);
}

void icon(int id,float x,float y,float size,Color color) {
    int variant=size<=16?2:size<=20?1:0,pixels=variant==2?ui.icon_small:variant==1?ui.icon_medium:ui.icon_large;
    float left=raster_position(x-pixels/ui.font_scale/2,ui.text_origin.x);
    float top=raster_position(y-pixels/ui.font_scale/2,ui.text_origin.y);
    DrawTexturePro(ui.icons,(Rectangle){id*ui.icon_large,variant==2?ui.icon_large+ui.icon_medium:variant==1?ui.icon_large:0,pixels,pixels},(Rectangle){left,top,pixels/ui.font_scale,pixels/ui.font_scale},(Vector2){0},0,color);
}

void arcs_init(void) {
    /* Swing horseshoe, centered full turn, and minimum-to-value full turn. */
    Image image=GenImageColor(1536,288,BLANK);
    Color *pixels=image.data;
    for(int bank=0;bank<3;bank++) for(int level=0;level<=128;level++) for(int y=0;y<32;y++) for(int x=0;x<32;x++) {
        float dx=x+.5f-16,dy=y+.5f-16,distance=sqrtf(dx*dx+dy*dy);
        float origin=bank==0?2.4f:bank==1?PI/2:-PI/2;
        float angle=atan2f(dy,dx)-origin; if(angle<0) angle+=2*PI;
        float end=level/128.f*(bank==0?4.6f:2*PI),start=bank==1?PI:0;
        float alpha=fmaxf(0,fminf(1,fminf(16-distance,distance-(16-48.f/KNOB_RADIUS))/1.4f));
        if(bank!=2 || level!=128) alpha*=fmaxf(0,fminf(1,fminf(angle-fminf(start,end),fmaxf(start,end)-angle)*distance));
        pixels[(level/16*32+y)*1536+bank*512+level%16*32+x]=(Color){255,255,255,255*alpha};
    }
    ui.knob_arcs=LoadTextureFromImage(image); UnloadImage(image);
    SetTextureFilter(ui.knob_arcs,TEXTURE_FILTER_BILINEAR);
}

void cables_init(void) {
    Image image=GenImageColor(1,16,WHITE); Color *pixels=image.data;
    for(int y=0;y<16;y++) pixels[y].a=255*fminf(1,(8-fabsf(y+.5f-8))/4);
    ui.cable_texture=LoadTextureFromImage(image); UnloadImage(image);
    SetTextureFilter(ui.cable_texture,TEXTURE_FILTER_BILINEAR);
}

void cable(Vector2 points[4],Color color) {
    Vector2 previous[2]={{0}},normal={2,0};
    rlSetTexture(ui.cable_texture.id); rlBegin(RL_QUADS); rlColor4ub(color.r,color.g,color.b,color.a);
    for(int i=0;i<=64;i++) {
        float t=i/64.f,u=1-t;
        Vector2 p=GetSplinePointBezierCubic(points[0],points[1],points[2],points[3],t);
        float dx=3*u*u*(points[1].x-points[0].x)+6*u*t*(points[2].x-points[1].x)+3*t*t*(points[3].x-points[2].x);
        float dy=3*u*u*(points[1].y-points[0].y)+6*u*t*(points[2].y-points[1].y)+3*t*t*(points[3].y-points[2].y);
        float length=hypotf(dx,dy); if(length>.001f) normal=(Vector2){-dy/length*2,dx/length*2};
        Vector2 edge[2]={{p.x+normal.x,p.y+normal.y},{p.x-normal.x,p.y-normal.y}};
        if(i) {
            rlTexCoord2f(.5f,0); rlVertex2f(previous[0].x,previous[0].y);
            rlTexCoord2f(.5f,0); rlVertex2f(edge[0].x,edge[0].y);
            rlTexCoord2f(.5f,1); rlVertex2f(edge[1].x,edge[1].y);
            rlTexCoord2f(.5f,1); rlVertex2f(previous[1].x,previous[1].y);
        }
        previous[0]=edge[0]; previous[1]=edge[1];
    }
    rlEnd(); rlSetTexture(0);
}

void smooth_line(Vector2 start,Vector2 end,float width,Color color) {
    float dx=end.x-start.x,dy=end.y-start.y,length=hypotf(dx,dy);
    if(length<.001f) return;
    float radius=width*2/3;
    Vector2 normal={-dy/length*radius,dx/length*radius};
    rlSetTexture(ui.cable_texture.id); rlBegin(RL_QUADS); rlColor4ub(color.r,color.g,color.b,color.a);
    rlTexCoord2f(.5f,0); rlVertex2f(start.x+normal.x,start.y+normal.y);
    rlTexCoord2f(.5f,0); rlVertex2f(end.x+normal.x,end.y+normal.y);
    rlTexCoord2f(.5f,1); rlVertex2f(end.x-normal.x,end.y-normal.y);
    rlTexCoord2f(.5f,1); rlVertex2f(start.x-normal.x,start.y-normal.y);
    rlEnd(); rlSetTexture(0);
}

int hover(float x,float y,float w,float h) { return ui.input_enabled && CheckCollisionPointRec(ui.mouse,(Rectangle){x,y,w,h}); }

int menu_key(int key) { return IsKeyPressed(key) || IsKeyPressedRepeat(key); }

void menu_keys_begin(int id,int opened,int default_item,int *scroll,int total) {
    if(ui.menu_keys.id!=id || opened) {
        ui.menu_keys.id=id; ui.menu_keys.selected=default_item; ui.menu_keys.count=0; ui.menu_keys.pointer=ui.mouse;
    }
    /* Resolve mouse focus before drawing so the old and new rows never both light up. */
    int moved=ui.mouse.x!=ui.menu_keys.pointer.x || ui.mouse.y!=ui.menu_keys.pointer.y;
    if(moved || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        for(int i=0;i<ui.menu_keys.count;i++) if(CheckCollisionPointRec(ui.mouse,ui.menu_keys.bounds[i])) {
            ui.menu_keys.selected=i; break;
        }
    ui.menu_keys.pointer=ui.mouse;
    ui.menu_keys.active=1; ui.menu_keys.items=0; ui.menu_keys.enter=0;
    int next=menu_key(KEY_DOWN) || menu_key(KEY_J) || menu_key(KEY_RIGHT) || menu_key(KEY_L);
    int previous=menu_key(KEY_UP) || menu_key(KEY_K) || menu_key(KEY_LEFT) || menu_key(KEY_H);
    if(!opened && ui.menu_keys.count>0) {
        if(next!=previous) {
            if(scroll && next && ui.menu_keys.selected==ui.menu_keys.count-1 && *scroll+ui.menu_keys.count<total) ++*scroll;
            else if(scroll && previous && ui.menu_keys.selected==0 && *scroll>0) --*scroll;
            else {
                if(scroll && next && ui.menu_keys.selected==ui.menu_keys.count-1) *scroll=0;
                if(scroll && previous && ui.menu_keys.selected==0) *scroll=fmaxf(0,total-ui.menu_keys.count);
                ui.menu_keys.selected=(ui.menu_keys.selected+(next?1:ui.menu_keys.count-1))%ui.menu_keys.count;
            }
        }
        ui.menu_keys.enter=IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    }
}

void menu_keys_end(void) {
    ui.menu_keys.count=ui.menu_keys.items<128?ui.menu_keys.items:128; ui.menu_keys.active=0; ui.menu_keys.enter=0;
    if(ui.menu_keys.count>0 && ui.menu_keys.selected>=ui.menu_keys.count) ui.menu_keys.selected=ui.menu_keys.count-1;
}

int symbol(const char *text,int x,int y,Color c) {
    int id;
    if(!strcmp(text,"x")) id=ICON_CLOSE;
    else if(!strcmp(text,"[]")) id=ICON_STOP;
    else if(!strcmp(text,"||")) id=ICON_PAUSE;
    else if(!strcmp(text,">")) id=ICON_PLAY;
    else if(!strcmp(text,"<")) id=ICON_BACK;
    else return 0;
    icon(id,x,y,16,c);
    return 1;
}

int button_color(const char *text,int x,int y,int w,int h,int active,Color base) {
    int over=hover(x,y,w,h);
    int menu_item=ui.menu_keys.active && ui.input_enabled && strcmp(text,"x")?ui.menu_keys.items++:-1;
    if(menu_item>=0 && menu_item<128) ui.menu_keys.bounds[menu_item]=(Rectangle){x,y,w,h};
    int focused=menu_item>=0 && menu_item==ui.menu_keys.selected;
    int activate=focused && ui.menu_keys.enter;
    if(activate) ui.menu_keys.enter=0;
    if(over) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    if((menu_item>=0?focused:over) && *text) {
        const char *description=text;
        if(!strcmp(text,"New")) description="Start an empty project with one unloaded Sampler";
        else if(!strcmp(text,"Demo")) description="Load the built-in drum and bass demo";
        else if(!strcmp(text,"Save")) description="Save the current project";
        else if(!strcmp(text,"Open...")) description="Choose a project to open";
        else if(!strcmp(text,"Export...")) description="Choose where to export the Playlist as a WAV file";
        else if(!strcmp(text,"HELP")) description="Show all current keybindings";
        else if(!strcmp(text,"Dark")) description="Use dark colors; saved automatically for next launch";
        else if(!strcmp(text,"Light")) description="Use light colors; saved automatically for next launch";
        else if(!strcmp(text,"+ Add folder")) description="Add a folder as a new Browser tree root";
        else if(!strcmp(text,"PAT") || !strcmp(text,"SONG")) description="Switch playback between the current pattern and Playlist";
        else if(!strcmp(text,"Rack")) description="Show/focus or hide the Channel Rack";
        else if(!strcmp(text,"List")) description="Show/focus or hide the Playlist";
        else if(!strcmp(text,"Piano")) description="Show/focus or hide the Piano Roll";
        else if(!strcmp(text,"Mix")) description="Show/focus or hide the Mixer";
        else if(!strcmp(text,"x")) description="Close this dialog";
        else if(!strcmp(text,"<")) description="Previous";
        else if(!strcmp(text,">")) description="Next";
        else if(!strcmp(text,"[]")) description="Stop playback and return to the beginning";
        else if(!strcmp(text,"||")) description="Pause playback";
        else if(!strcmp(text,"Reset")) description="Restore this control's default value";
        else if(!strcmp(text,"Apply")) description="Apply the entered value";
        else if(!strcmp(text,"Cancel")) description="Cancel without changing the value";
        snprintf(ui.status,sizeof ui.status,"%s",description);
    }
    int highlighted=menu_item>=0?focused:over && !ui.menu_keys.active;
    Color surface=menu_item>=0?(focused?ui_theme.hover:base):
        active?(highlighted?ui_theme.active_hover:accent):highlighted?ui_theme.hover:base;
    ui_surface((Rectangle){x,y,w,h},surface);
    Color foreground=(menu_item>=0?focused:active || highlighted)?theme_foreground(surface):ink;
    if(w>52 || !symbol(text,x+w/2,y+h/2,foreground)) {
        int available=w-(w<=72?8:16);
        int size=text_button_size(text,available,h);
        const char *caption=fit_text(text,available,size);
        label(caption,x+((w<=72 || !strcmp(text,"+"))?(w-text_width(caption,size))/2:8),y+(h-size)/2,size,foreground);
    }
    if(menu_item>=0 && over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.menu_keys.selected=menu_item;
    return activate || (over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
}

int button(const char *text,int x,int y,int w,int h,int active) {
    return button_color(text,x,y,w,h,active,cell);
}

int drag_button(const char *text,int x,int y,int w,int h,int active) {
    int clicked=button(text,x,y,w,h,active);
    if(hover(x,y,w,h) || active || clicked) SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
    return clicked;
}

int color_picker(int x,int y,int width,uint32_t selected) {
    int chosen=-1,spacing=width/COLOR_HUES;
    int hover_index=-1;
    for(int i=0;i<COLOR_COUNT;i++)
        if(hover(x+(i%COLOR_HUES)*spacing,y+(i/COLOR_HUES)*28,spacing-4,24)) hover_index=i;
    for(int i=0;i<COLOR_COUNT;i++) {
        unsigned rgb=pattern_palette[i]; Color color=source_rgb(rgb);
        Rectangle swatch={x+(i%COLOR_HUES)*spacing,y+(i/COLOR_HUES)*28,spacing-4,24};
        int over=hover(swatch.x,swatch.y,swatch.width,swatch.height);
        ui_surface(swatch,color);
        if(hover_index>=0?i==hover_index:selected==rgb) DrawRectangleLinesEx((Rectangle){swatch.x+2,swatch.y+2,swatch.width-4,swatch.height-4},2,clip_foreground(color));
        if(over) { SetMouseCursor(MOUSE_CURSOR_POINTING_HAND); snprintf(ui.status,sizeof ui.status,"Choose color #%06X",rgb); if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) chosen=i; }
    }
    return chosen;
}

int picker_button(int y,Color color,int selected) {
    int over=hover(4,y,112,50);
    if(over) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    ui_surface((Rectangle){4,y,112,50},color);
    if(over || selected) DrawRectangleLinesEx((Rectangle){4,y,112,50},2,WHITE);
    return over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void automation_display_update(void) {
    memset(ui.automation_visible,0,sizeof ui.automation_visible);
    if(!ui.playing || !ui.song || !ui.project.automation_count) return;
    int solo=solo_any(ui.project.lane_mute,LANES);
    double held_end[AUTOMATIONS]; for(int a=0;a<ui.project.automation_count;a++) held_end[a]=-INFINITY;
    for(int pass=0;pass<2;pass++) for(int l=0;l<LANES;l++) if(!(ui.project.lane_mute[l]&1) && (!solo || (ui.project.lane_mute[l]&2))) for(int b=0;b<CLIPS;b++) {
        int a=ui.project.clips[l][b]-AUTOMATION_SOURCE-1; if(a<0 || a>=ui.project.automation_count) continue;
        float start=ui.project.clip_starts[l][b]*STEPS;
        float end=start+clip_length(&ui.project,l,b);
        if(pass==0?(ui.visual_step<end || end<held_end[a]):(ui.visual_step<start || ui.visual_step>=end)) continue;
        ParameterTarget target=ui.project.automations[a].target;
        float normalized=automation_value(&ui.project.automations[a],(pass==0?end:ui.visual_step)-start+ui.project.clip_offsets[l][b]);
        for(int i=0;i<ui.project.automation_count;i++) {
            ParameterTarget t=ui.project.automations[i].target;
            if(t.parameter==target.parameter && t.owner==target.owner && t.slot==target.slot) { ui.automation_visible[i]=1; ui.automation_shown[i]=normalized; if(pass==0) held_end[i]=end; }
        }
    }
}

float displayed_value(const void *pointer,float manual) {
    if(!ui.playing || !ui.song || !ui.project.automation_count) return manual;
    ParameterTarget target; float value,lo,hi;
    if(!parameter_from_pointer(&ui.project,pointer,&target) || !parameter_info(&ui.project,target,&value,&lo,&hi)) return manual;
    for(int a=0;a<ui.project.automation_count;a++) {
        ParameterTarget t=ui.project.automations[a].target;
        if(ui.automation_visible[a] && t.parameter==target.parameter && t.owner==target.owner && t.slot==target.slot) {
            float shown=lo+(hi-lo)*ui.automation_shown[a]; return target.parameter==PARAM_PITCH_RANGE?roundf(shown):shown;
        }
    }
    return manual;
}

void mute_light(float x,float y,uint8_t *states,int count,int selected,const char *name) {
    uint8_t state=states[selected]; if(displayed_value(&states[selected],!!(state&1))>=.5f) state|=1; else state&=~1; int solo=count?solo_any(states,count):0;
    int over=hover(x-8,y-8,16,16);
    if(over) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    int enabled=!(state&1) && (!solo || (state&2));
    Color color=enabled?(state&2?ui_theme.meter_mid:ui_theme.signal):cell;
    if(enabled && !(state&2) && !over)
        color=ColorAlphaBlend(cell,Fade(color,.48f),WHITE);
    circle(x,y,6,over?ink:state&2?ui_theme.meter_mid:ui_theme.border); circle(x,y,5,color);
    if(over) {
        snprintf(ui.status,sizeof ui.status,"%s: %s | Left-click: mute/unmute; right-click: %s",name,state&1?"Muted":state&2?"Solo":solo?"Silenced by solo":"Enabled",states==ui.project.lane_mute?"add/remove solo":"automation / solo menu");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { states[selected]^=1; ui.input_enabled=0; }
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            if(parameter_from_pointer(&ui.project,&states[selected],&ui.automation_target)) {
                snprintf(ui.menu_name,sizeof ui.menu_name,"Mute");
                Vector2 position=ui.mouse; if(ui.knob_context>=0) { position.x+=ui.windows.editors[ui.knob_context].rect.x; position.y+=ui.windows.editors[ui.knob_context].rect.y; }
                open_context(10,0,position); return;
            }
            if(count) solo_toggle(states,count,selected);
            else {
                for(int i=0;i<INSERTS;i++) ui.project.insert_mute[i]&=~2;
                for(int c=0;c<CHANNELS;c++) ui.project.mute[c]&=~2;
                for(int l=0;l<LANES;l++) ui.project.lane_mute[l]&=~2;
            }
            ui.input_enabled=0;
        }
    }
}

int editor_button(int id,int x) {
    static const char *names[]={"Channel Rack","Playlist","Piano Roll","Mixer"};
    int front=0;
    for(int i=EDITORS-1;i>=0;i--) {
        int other=ui.windows.order[i];
        if(ui.windows.editors[other].visible && ui.windows.editors[other].pinned==ui.windows.editors[id].pinned) { front=other==id; break; }
    }
    int clicked=button("",x,8,22,22,0);
    if(clicked) { if(front) ui.windows.editors[id].visible=0; else windows_focus(&ui.windows,id); }
    icon(id,x+11,19,20,ink);
    if(hover(x,8,22,22)) snprintf(ui.status,sizeof ui.status,"%s: %s",names[id],front?"hide this window":"show / bring this window to the front");
    return clicked;
}

void open_context(int kind,int target,Vector2 position) {
    ui.context_kind=kind; ui.context_target=target; ui.context_position=position; ui.context_opened=1; ui.input_enabled=0;
}

int preset_directory(char *out,size_t capacity,int kind) {
    const char *slash=strrchr(ui.browser.config,'/');
    if(!slash) return 0;
    char root[PATH_MAX];
    if(snprintf(root,sizeof root,"%.*s/presets",(int)(slash-ui.browser.config),ui.browser.config)>=(int)sizeof root || MakeDirectory(root)) return 0;
    const char *device=kind==PRESET_FM?"FM Synth":kind==PRESET_EQ?"Equalizer":kind==PRESET_CHORUS?"Chorus":"Sampler";
    if(snprintf(out,capacity,"%s/%s",root,device)>=(int)capacity || MakeDirectory(out)) return 0;
    if(kind==PRESET_FM) for(int i=0;i<FM_FACTORY_COUNT;i++) {
        char factory[PATH_MAX];
        if(snprintf(factory,sizeof factory,"%s/%s.llpreset",out,fm_factory_name(i))<(int)sizeof factory && !FileExists(factory)) {
            DevicePreset preset={.kind=PRESET_FM,.fm=fm_factory(i)}; preset_save(factory,&preset);
        }
    }
    return 1;
}

void capture_control(float *value,float low,float high,int fader) {
    ParameterTarget target;
    const ParameterDescriptor *descriptor=parameter_from_pointer(&ui.project,value,&target)?parameter_descriptor(target.parameter):NULL;
    ui.control_integer=descriptor && descriptor->kind==PARAMETER_INTEGER;
    ui.control_logarithmic=descriptor && descriptor->kind==PARAMETER_LOGARITHMIC;
    ui.control_raw=*value; ui.control_drag=value;
    ui.control_low=descriptor?descriptor->low:low; ui.control_high=descriptor?descriptor->high:high;
    ui.control_fader=fader; ui.control_reverse=0; ui.input_enabled=0;
}

void knob_style(int x,int y,float *value,float low,float high,float initial,const char *name,int style) {
    float size=style==KNOB_SWING?SWING_RADIUS:KNOB_RADIUS,shown=displayed_value(value,*value);
    int over=hover(x-size-1,y-size-1,size*2+2,size*2+2);
    int fixed_coarse=style==KNOB_FIXED_COARSE;
    if(fixed_coarse) {
        high=3; initial=(int)initial&3; shown=(int)roundf(shown)&3;
        /* Old DX files may store aliases 4..31. Canonicalize on interaction,
           preserving their audible frequency and leaving unopened files alone. */
        if(over && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || GetMouseWheelMove()))
            *value=(int)roundf(*value)&3;
    }
    if(over || ui.control_drag==value) {
        if(style==KNOB_VOLUME) snprintf(ui.status,sizeof ui.status,"%s: %.4g (%.2f dB) | Dot marks 0 dB; drag or wheel; right-click for value / automation",name,shown,gain_db(shown));
        else snprintf(ui.status,sizeof ui.status,"%s: %.4g | Drag up/down or wheel; right-click for value / automation",name,shown);
    }
    if(over) {
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(value,low,high,initial,name);
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { capture_control(value,low,high,0); ui.control_low=low; ui.control_high=high; ui.control_reverse=style==KNOB_WIDTH; }
        float wheel=GetMouseWheelMove();
        if(wheel) {
            ParameterTarget target;
            const ParameterDescriptor *info=parameter_from_pointer(&ui.project,value,&target)?parameter_descriptor(target.parameter):NULL;
            int integer=info && info->kind==PARAMETER_INTEGER;
            float next=style==KNOB_LOGARITHMIC?*value*powf(high/low,wheel/20):*value+wheel*(integer?1:(high-low)/20)*(style==KNOB_WIDTH?-1:1);
            *value=fmaxf(low,fminf(high,integer?roundf(next):next));
        }
    }
    float radius=size,fraction=(shown-low)/(high-low);
    if(style==KNOB_LOGARITHMIC) fraction=logf(shown/low)/logf(high/low);
    if(style==KNOB_CENTER) fraction=shown<initial?.5f*(shown-low)/(initial-low):.5f+.5f*(shown-initial)/(high-initial);
    if(style==KNOB_WIDTH) fraction=1-fraction;
    if(style==KNOB_VOLUME) fraction=shown<=1?.75f*(shown):.75f+.25f*(shown-1)/(high-1);
    int centered=style==KNOB_PAN || style==KNOB_WIDTH || style==KNOB_CENTER;
    float origin=centered?PI/2:style==KNOB_SWING?2.4f:style==KNOB_VOLUME?PI/2:-PI/2;
    float angle=origin+fraction*(style==KNOB_SWING?4.6f:2*PI);
    Color color=style==KNOB_SWING || style==KNOB_CENTER?ui_theme.swing:style==KNOB_PAN?(fraction<.5f?ui_theme.pan_left:ui_theme.pan_right):style==KNOB_WIDTH?(fraction<.5f?ui_theme.stereo:ui_theme.mono):ui_theme.knob;
    if((over || ui.control_drag==value) && (style==KNOB_NORMAL || style==KNOB_VOLUME || style==KNOB_LOGARITHMIC || style==KNOB_FIXED_COARSE)) color=accent;
    if(style==KNOB_SWING) {
        DrawTexturePro(ui.knob_arcs,(Rectangle){0,256,32,32},(Rectangle){x-radius,y-radius,radius*2,radius*2},(Vector2){0},0,ui_theme.knob_track);
        circle(x,y,radius*.68f,cell);
    } else { circle(x,y,radius,ui_theme.knob_track); circle(x,y,radius*.8f,cell); }
    if(style==KNOB_PAN || style==KNOB_WIDTH) {
        Color left=style==KNOB_PAN?ui_theme.pan_left:ui_theme.stereo;
        Color right=style==KNOB_PAN?ui_theme.pan_right:ui_theme.mono;
        float opacity=.35f;
        Rectangle ring={x,y,radius*2,radius*2}; Vector2 center={radius,radius};
        DrawTexturePro(ui.knob_arcs,(Rectangle){512,0,32,32},ring,center,0,Fade(left,opacity));
        DrawTexturePro(ui.knob_arcs,(Rectangle){512,256,32,32},ring,center,0,Fade(right,opacity));
    }
    int level=fmaxf(0,fminf(128,roundf(fraction*128)));
    float rotation=style==KNOB_SWING || centered?0:(origin+PI/2)*RAD2DEG;
    DrawTexturePro(ui.knob_arcs,(Rectangle){(style==KNOB_SWING?0:centered?512:1024)+level%16*32,level/16*32,32,32},(Rectangle){x,y,radius*2,radius*2},(Vector2){radius,radius},rotation,color);
    if(style==KNOB_VOLUME) {
        float unity=origin+.75f*2*PI;
        circle(x+cosf(unity)*size*1.22f,y+sinf(unity)*size*1.22f,1.2f,muted);
    }
    Vector2 pointer={x+cosf(angle)*radius*.85f,y+sinf(angle)*radius*.85f};
    float pointer_scale=size/KNOB_RADIUS;
    circle(pointer.x,pointer.y,2.3f*pointer_scale,(Color){35,24,49,255});
    circle(pointer.x,pointer.y,1.7f*pointer_scale,WHITE);
}

void knob(int x,int y,float *value,float low,float high,float initial,const char *name) {
    knob_style(x,y,value,low,high,initial,name,KNOB_NORMAL);
}
