// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"
#include "text_edit.h"

/* y is the carried dialog's virtual content origin. */
static void popup_caption(const char *text,int x,int y,int width) {
    int size=ui.studio_style?14:13,pad=ui.studio_style?STUDIO_TITLE-TITLE:0;
    label(fit_text(text,width-48,size),x+8,y-pad+3,size,ui.studio_style?theme_foreground(ui_theme.title_focus):ink);
}

void open_popup(int kind) {
    if(kind==2) ui.rename_automation=-1;
    if(kind==4) ui.help_scroll=0;
    ui.pattern_popup=kind; ui.popup_opened=1; ui.popup_drag=0;
    ui.popup_position[kind]=kind==7?(DrawPoint){96,34}:(DrawPoint){-1,-1};
}

const char *mixer_name(int id) { return id?ui.project.insert_names[id-1]:"Master"; }

void begin_rename(void) {
    ui.rename_track=ui.rename_mixer=ui.rename_channel=-1;
    ui.rename_pattern=ui.pattern; snprintf(ui.rename_text,sizeof ui.rename_text,"%s",ui.project.pattern_names[ui.pattern]);
    ui.rename_select_all=1; open_popup(2);
}

void begin_number(float *value,float low,float high,float initial,const char *name) {
    ui.number_integer=0; ui.number_target=value; ui.number_low=low; ui.number_high=high; ui.number_default=initial; ui.number_name=name;
    snprintf(ui.number_text,sizeof ui.number_text,"%.6g",*value); ui.rename_select_all=1; open_popup(5);
}

void begin_mixer_add(void) {
    if(ui.project.insert_count>=INSERTS)return;
    ui.mixer_add_count=1;
    begin_number(&ui.mixer_add_count,1,INSERTS-ui.project.insert_count,1,"Number of mixer inserts to add");ui.number_integer=1;
}

void control_menu(float *value,float low,float high,float initial,const char *name) {
    int automated=parameter_from_pointer(&ui.project,value,&ui.automation_target);
    ui.menu_value=value; ui.menu_low=low; ui.menu_high=high; ui.menu_initial=initial;
    snprintf(ui.menu_name,sizeof ui.menu_name,"%s",name);
    DrawPoint position=ui.mouse;
    if(ui.knob_context>=0) { position.x+=desktop_editor_rect(ui.knob_context).x; position.y+=desktop_editor_rect(ui.knob_context).y; }
    open_context(automated?9:15,0,position);
}

void begin_rack_fill(int channel) {
    ui.rack_fill_pattern=ui.pattern;ui.rack_fill_channel=channel;
    if(ui.rack_fill_interval<1)ui.rack_fill_interval=4;
    begin_number(&ui.rack_fill_interval,1,1000000,4,"Fill every X steps");ui.number_integer=1;
}

void create_automation(void) {
    float start=ui.song_loop[1]>ui.song_loop[0]?ui.song_loop[0]/STEPS:floorf((ui.playing && ui.song?ui.visual_step:ui.playlist_start)/STEPS);
    float steps=ui.song_loop[1]>ui.song_loop[0]?ui.song_loop[1]-ui.song_loop[0]:fmaxf(STEPS,ui.project.pattern_steps[ui.pattern]);
    char name[PATTERN_NAME];
    unsigned id=ui.automation_target.parameter,owner=ui.automation_target.owner;
    const char *prefix=((id>=PARAM_FM_RATIO && id<=PARAM_FM_LAST) || (id>=PARAM_DX7_FIRST && id<=PARAM_DX7_LAST))?ui.project.channel_names[owner]:parameter_is_effect(id)?(owner?ui.project.insert_names[owner-1]:"Master"):id<=PARAM_CHANNEL_PITCH || id==PARAM_CHANNEL_MUTE || id==PARAM_PITCH_RANGE?ui.project.channel_names[owner]:id<=PARAM_INSERT_WIDTH || id==PARAM_INSERT_MUTE?ui.project.insert_names[owner]:"Master";
    const char *parameter=ui.menu_name;
    if(!strncmp(parameter,"Insert ",7)) parameter+=7;
    else if(!strncmp(parameter,"Channel ",8)) parameter+=8;
    else if(!strncmp(parameter,"Master ",7)) parameter+=7;
    snprintf(name,sizeof name,"%.20s %.23s",prefix,parameter);
    int a=automation_create(&ui.project,ui.automation_target,name,steps);
    if(a<0) { snprintf(ui.status,sizeof ui.status,"Cannot create automation: %d clip limit.",AUTOMATIONS); return; }
    int lane,slot=-1;
    for(lane=0;lane<LANES;lane++) if((slot=arrangement_place(&ui.project,lane,start,AUTOMATION_SOURCE+a,steps))>=0) break;
    if(slot<0) { automation_delete(&ui.project,a); snprintf(ui.status,sizeof ui.status,"No free Playlist space for automation."); return; }
    ui.automation_selected=a; ui.picker_tab=2; ui.picker_scroll=a; ui.picker_drag=-1;
    ui.arrangement.source_pattern=AUTOMATION_SOURCE+a; ui.arrangement.source_steps=steps; ui.arrangement.source_offset=0;
    memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected); ui.arrangement.selected[lane][slot]=1;
    /* Creation selects the clip without moving the user's viewport. */
    ui.browser_focus=0; windows_focus(&ui.windows,1);
    snprintf(ui.status,sizeof ui.status,"Automation created: click the line to add points; drag points; right-click points to delete.");
}

void text_input(char *text,size_t capacity) {
    int paste=shortcut_down() && host_is_key_pressed(HOST_KEY_V);
    text_edit(text,capacity,&ui.rename_select_all,(TextEditInput){
        .select_all=shortcut_down() && host_is_key_pressed(HOST_KEY_A),
        .paste=paste,.clipboard=paste?host_get_clipboard_text():NULL,
        .backspace=host_is_key_pressed(HOST_KEY_BACKSPACE),
        .arrow=host_is_key_pressed(HOST_KEY_LEFT) || host_is_key_pressed(HOST_KEY_RIGHT),
        .command_down=command_down(),.input=host_input()});
}

const char *device_caption(const char *name) { return !*name?"None":!strcmp(name,"@default")?"Default device":name; }

void device_button(int io,int x,int y,int width) {
    const char *selected=ui.project.audio_io[ui.mixer_selected][io];
    const char *caption=host_text_format("%s: %s",io?"Out":"In",device_caption(selected));
    int clicked=ui.studio_style?choice_button(caption,x,y,width,22):
        button_color(fit_text(caption,width-30,13),x,y,width,22,0,ui_theme.browser);
    if(clicked && !recording_active()) {
        ui.device_target=ui.mixer_selected; ui.device_io=io; ui.device_scroll=0;
        ui.device_count=audio_devices(!io,ui.device_names,64);
        open_popup(8);
        Rect r=desktop_editor_rect(3);
        ui.popup_position[8]=(DrawPoint){r.x+x,r.y+y+24};
    }
    if(hover(x,y,width,22)) snprintf(ui.status,sizeof ui.status,io?"Output device choice is saved; external-output routing is not implemented yet":"Input device for recording this mixer track; None records its internal audio");
    if(!ui.studio_style)host_draw_triangle((DrawPoint){x+width-16,y+8},(DrawPoint){x+width-12,y+14},(DrawPoint){x+width-8,y+8},muted);
}

void help_group(int x,int *y,const char *title) {
    label(title,x,*y,13,accent); *y+=26;
}

void help_binding(int x,int *y,const char *keys,const char *action) {
    label(keys,x,*y,11,ink); label(action,x+126,*y,11,muted); *y+=23;
}

static void popup_menu_anchor(int menu,int *x,int *y) {
    if(!ui.studio_style)return;
    StudioToolbar toolbar=studio_toolbar_layout(host_get_screen_width()/ui_scale());
    *x=toolbar.menus[menu].x;*y=toolbar.menus[menu].y+toolbar.menus[menu].h+2;
}

void draw_popup_content(void) {
    if(!ui.pattern_popup) { ui.popup_drag=0; return; }
    ui.input_enabled=1;
    if(ui.pattern_popup==10) {
        int count=ui.windows.focused==2 && ui.piano_tool==PENCIL?8:6;
        int x=52,y=34,w=210,h=8+count*26;
        popup_menu_anchor(1,&x,&y);
        if(host_is_key_pressed(HOST_KEY_ESCAPE) || (!ui.popup_opened && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { ui.pattern_popup=0; return; }
        ui_frame(host_draw(),(DrawRect){x,y,w,h});
        const char *names[]={"Undo","Redo","Cut","Copy","Paste","Select all","Octave up","Octave down"};
        for(int i=0;i<count;i++) if(button(names[i],x+4,y+4+i*26,w-8,26,0) && !ui.popup_opened) { ui.pattern_popup=0; if(i<2) undo_redo(i?1:-1); else if(i<6) edit_selection(i-2); else piano_octave(i==6?1:-1); }
        return;
    }
    if(ui.pattern_popup==1 || ui.pattern_popup==9) {
        int file=ui.pattern_popup==1,x=file?8:140,y=34,w=ui.studio_style?(file?272:184):210,h=file?190:34;
        popup_menu_anchor(file?0:3,&x,&y);
        if(host_is_key_pressed(HOST_KEY_ESCAPE) || (!ui.popup_opened && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { ui.pattern_popup=0; return; }
        ui_frame(host_draw(),(DrawRect){x,y,w,h});
        const char *items[]={"New","Demo","Save","Save As...","Open...","Export...","Collect samples and save..."};
        for(int i=0;i<(file?7:1);i++) if(button(file?items[i]:"Keybindings",x+4,y+4+i*26,w-8,24,0) && !ui.popup_opened) {
            if(file) recording_finish();
            ui.pattern_popup=0;
            if(!file) open_popup(4);
            else if(i==0) replace_project(1);
            else if(i==1) replace_project(2);
            else if(i==4) replace_project(3);
            else project_file_action(i);
            return;
        }
        return;
    }
    if(ui.pattern_popup==8) {
        int visible=fminf(10,ui.device_count+2),w=300,h=visible*24+8;
        float scale=ui_scale();
        int x=fmaxf(0,fminf(host_get_screen_width()/scale-w,ui.popup_position[8].x)),y=fmaxf(42,fminf(host_get_screen_height()/scale-24-h,ui.popup_position[8].y));
        if(host_is_key_pressed(HOST_KEY_ESCAPE) || (!ui.popup_opened && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { ui.pattern_popup=0; return; }
        ui_surface(host_draw(),(DrawRect){x,y,w,h},ui_theme.browser);
        host_draw_rectangle_lines_ex((DrawRect){x,y,w,h},1,ui_theme.border);
        if(hover(x,y,w,h)) ui.device_scroll=fmaxf(0,fminf(ui.device_count+2-visible,ui.device_scroll-host_get_mouse_wheel_move()));
        for(int row=0;row<visible;row++) {
            int i=row+ui.device_scroll; const char *name=i==0?"":i==1?"@default":ui.device_names[i-2];
            char *selected=ui.project.audio_io[ui.device_target][ui.device_io];
            if(button_color(fit_text(device_caption(name),w-32,13),x+4,y+4+row*24,w-8,22,!strcmp(name,selected),ui_theme.browser) && !ui.popup_opened) {
                snprintf(selected,128,"%s",name); ui.pattern_popup=0;
                snprintf(ui.status,sizeof ui.status,ui.device_io?"Output device choice saved; external-output routing is not implemented yet":"Input device saved; arm this mixer track and press Record");
            }
        }
        return;
    }
    if(ui.pattern_popup==7) {
        int x=96,y=34,w=ui.studio_style?208:304,h=ui.studio_style?60:34;
        popup_menu_anchor(2,&x,&y);
        if(host_is_key_pressed(HOST_KEY_ESCAPE) || (!ui.popup_opened && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { ui.pattern_popup=0; return; }
        ui_frame(host_draw(),(DrawRect){x,y,w,h});
        if(button("MIDI / Recording",x+4,y+4,w-8,25,0) && !ui.popup_opened){midi_refresh();open_popup(13);return;}
        if(ui.studio_style && button("UI Scale...",x+4,y+30,w-8,25,0) && !ui.popup_opened){open_popup(14);return;}
        return;
    }
    if(ui.pattern_popup==12) {
        float scale=ui_scale(); int w=400,h=152,x=(host_get_screen_width()/scale-w)/2,y=(host_get_screen_height()/scale-h)/2;
        ui_frame(host_draw(),(DrawRect){x,y,w,h}); label("Save your changes?",x+16,y+16,16,ink);
        label(fit_text(host_get_file_name(ui.document.path),w-32,13),x+16,y+48,13,muted);
        if(button("Save",x+16,y+100,100,28,1)) { ui.pattern_popup=0; project_file_action(2); return; }
        if(button("Discard",x+124,y+100,120,28,0)) { int action=ui.document.pending_action; ui.document.pending_action=0; ui.pattern_popup=0; if(action==4) load_project(ui.document.replacement_path); else if(action==3) project_file_action(4); else if(action==REPLACE_QUIT) ui.document.quit=1; else new_project(action==2); return; }
        if(button("Cancel",x+252,y+100,100,28,0) || host_is_key_pressed(HOST_KEY_ESCAPE)) { ui.document.pending_action=0; ui.pattern_popup=0; }
        return;
    }
    int picker=ui.pattern_popup==11,folder=ui.pattern_popup==3,help=ui.pattern_popup==4,number=ui.pattern_popup==5,w=ui.pattern_popup==14?400:ui.pattern_popup==13?460:picker?700:ui.pattern_popup==6?320:help?660:number?360:folder?620:260,h=ui.pattern_popup==14?208:ui.pattern_popup==13?390:picker?(ui.file_confirm?180:500):help?645:ui.pattern_popup==6 && ui.studio_style?188:116;
    int title=ui.studio_style?STUDIO_TITLE:TITLE,pad=title-TITLE;
    h+=pad;
    float scale=ui_scale();
    if(help || picker || ui.pattern_popup==13) h=fminf(h,host_get_screen_height()/scale-56);
    DrawPoint *position=&ui.popup_position[ui.pattern_popup];
    if(position->x<0 || position->y<0) *position=(DrawPoint){(host_get_screen_width()/scale-w)/2,(host_get_screen_height()/scale-h)/2};
    if(ui.popup_drag) {
        position->x=ui.mouse.x-ui.popup_offset.x; position->y=ui.mouse.y-ui.popup_offset.y;
        if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) ui.popup_drag=0;
    }
    position->x=fmaxf(0,fminf(host_get_screen_width()/scale-w,position->x));
    position->y=fmaxf(0,fminf(host_get_screen_height()/scale-h,position->y));
    int x=position->x,y=position->y;
    ui_frame(host_draw(),(DrawRect){x,y,w,h});
    if(ui.studio_style && !ui.profile_flat)ui_titlebar(host_draw(),(DrawRect){x,y,w,title},1);
    else ui_surface(host_draw(),(DrawRect){x,y,w,title},ui_theme.title_focus);
    if(button("x",x+w-(ui.studio_style?21:24),y+(ui.studio_style?3:1),ui.studio_style?18:20,ui.studio_style?18:16,0)) { if(ui.pattern_popup==11) ui.document.pending_action=0; ui.pattern_popup=0; ui.popup_drag=0; return; }
    if(hover(x,y,w-(ui.studio_style?STUDIO_TITLE:26),title)) snprintf(ui.status,sizeof ui.status,"Drag this title bar to move the dialog");
    if(hover(x,y,w-(ui.studio_style?STUDIO_TITLE:26),title) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) { ui.popup_drag=1; ui.popup_offset=(DrawPoint){ui.mouse.x-x,ui.mouse.y-y}; }
    if(host_is_key_pressed(HOST_KEY_ESCAPE)) { if(ui.pattern_popup==11) ui.document.pending_action=0; ui.pattern_popup=0; ui.popup_drag=0; return; }
    if(!ui.popup_drag && !ui.popup_opened && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) && !hover(x,y,w,h)) { if(ui.pattern_popup==11) ui.document.pending_action=0; ui.pattern_popup=0; return; }
    ui.input_enabled=!ui.popup_drag;
    y+=pad;h-=pad;
    if(ui.pattern_popup==14) {
        popup_caption("UI Scale",x,y,w);
        float requested=ui.ui_scale_preference>0?ui.ui_scale_preference:scale,next=ui.ui_scale_preference;
        label(ui.ui_scale_preference==0?host_text_format("Automatic (%.0f%%)",scale*100):(requested>scale+.005f?host_text_format("%.0f%% (using %.0f%%)",requested*100,scale*100):host_text_format("%.0f%%",requested*100)),x+100,y+50,14,ink);
        if(button("",x+16,y+44,56,28,0))next=fminf(2,fmaxf(.75f,(ceilf(requested*4-.001f)-1)/4));
        if(button("",x+w-72,y+44,56,28,0))next=fminf(2,(floorf(requested*4+.001f)+1)/4);
        host_draw_line_ex((DrawPoint){x+38,y+58},(DrawPoint){x+50,y+58},2,ink);
        host_draw_line_ex((DrawPoint){x+w-50,y+58},(DrawPoint){x+w-38,y+58},2,ink);
        host_draw_line_ex((DrawPoint){x+w-44,y+52},(DrawPoint){x+w-44,y+64},2,ink);
        if(hover(x+16,y+44,56,28))snprintf(ui.status,sizeof ui.status,"Smaller interface (minimum 75%%)");
        if(hover(x+w-72,y+44,56,28))snprintf(ui.status,sizeof ui.status,"Larger interface (maximum 200%%; limited to fit this window)");
        if(button("Automatic",x+16,y+88,174,28,ui.ui_scale_preference==0))next=0;
        if(button("100%",x+204,y+88,180,28,ui.ui_scale_preference==1))next=1;
        label("Whole interface; project zoom is unchanged.",x+16,y+132,12,muted);
        label(requested>scale+.005f?"Scale limited to fit this window.":"System HiDPI is handled automatically.",x+16,y+154,12,muted);
        if(button("Close",x+w-100,y+h-34,84,26,0))ui.pattern_popup=0;
        if(next!=ui.ui_scale_preference){ui.ui_scale_preference=next;if(!ui_scale_save())snprintf(ui.status,sizeof ui.status,"UI scale changed; could not save preference.");}
        return;
    }
    if(picker) { draw_file_picker(x,y,w,h,scale); host_end_scissor_mode(); return; }
    if(ui.pattern_popup==13) {
        popup_caption("MIDI / Recording",x,y,w);
        int enabled=ui.input_enabled;if(recording_active())ui.input_enabled=0;
        const char *modes[]={"Audio","Notes","Automation"};
        for(int i=0;i<3;i++)if(button(modes[i],x+12+i*145,y+30,140,28,ui.record_mask&(1u<<i))){ui.record_mask^=1u<<i;midi_save_settings();}
        int target=midi_target();label(fit_text(host_text_format("Keyboard plays: %s",target>=0 && target<ui.project.channel_count?ui.project.channel_names[target]:"Select an instrument"),w-24,12),x+12,y+68,12,ink);
        label(ui.studio_style?"Laptop keys work without a MIDI device; enable Notes to record.":"Select a channel in the Rack or Piano Roll to play it.",x+12,y+88,11,muted);
        if(button("Refresh inputs",x+12,y+112,140,24,0))midi_refresh();
        if(button("Clear MIDI links",x+160,y+112,140,24,0)){ui.project.midi_binding_count=0;memset(ui.project.midi_bindings,0,sizeof ui.project.midi_bindings);ui.midi_learning=0;}
        if(button(ui.record_count_enabled?"Count-in: On":"Count-in: Off",x+12,y+140,140,24,ui.record_count_enabled)){ui.record_count_enabled^=1;midi_save_settings();}
        if(button(host_text_format("%.0f steps...",ui.record_count_steps),x+160,y+140,140,24,0)){begin_number(&ui.record_count_steps,1,4096,16,"Count-in steps (sixteenth notes)");ui.number_integer=1;}
        label(ui.midi_connected?"Input connected":"Select an input (USB or virtual MIDI)",x+12,y+173,11,ui.midi_connected?accent:muted);
        int visible=fmaxf(1,(h-208)/24);if(hover(x+12,y+196,w-24,h-208))ui.midi_scroll=fmaxf(0,fminf(fmaxf(0,ui.midi_device_count+1-visible),ui.midi_scroll-host_get_mouse_wheel_move()));
        for(int row=0;row<visible && row+ui.midi_scroll<=ui.midi_device_count;row++) {
            int i=row+ui.midi_scroll-1;const char *name=i<0?"None":ui.midi_devices[i].name;
            if(button(fit_text(name,w-40,12),x+12,y+196+row*24,w-24,22,i<0?!ui.midi_selected[0]:!strcmp(ui.midi_selected,ui.midi_devices[i].id)))midi_select(i);
        }
        ui.input_enabled=enabled;return;
    }
    if(ui.pattern_popup==6) {
        popup_caption("Add instrument",x,y,w);
        if(ui.studio_style){
            for(int choice=0;choice<INSTRUMENT_CHOICES;choice++)if(button(instrument_choice_name(choice),x+12,y+30+choice*36,w-24,30,0)){add_instrument_choice(choice);ui.pattern_popup=0;}
            return;
        }
        if(button(instrument_descriptor(INSTRUMENT_SAMPLER)->name,x+12,y+30,w-24,30,0)) { add_instrument(INSTRUMENT_SAMPLER); ui.pattern_popup=0; }
        if(button(instrument_descriptor(INSTRUMENT_FM)->name,x+12,y+66,w-24,30,0)) { add_instrument(INSTRUMENT_FM); ui.pattern_popup=0; }
        return;
    }
    if(help) {
        popup_caption("Help - Keybindings",x,y,w);
        int maximum=767-h;
        if(hover(x,y+TITLE,w,h-TITLE)) ui.help_scroll-=host_get_mouse_wheel_move()*36;
        ui.help_scroll=fmaxf(0,fminf(maximum,ui.help_scroll));
        host_begin_scissor_mode(x*scale,(y+TITLE)*scale,w*scale,(h-TITLE-30)*scale);
        int left=x+18,right=x+348,ly=y+34-ui.help_scroll,ry=ly;
        help_group(left,&ly,"Playlist & Piano Roll navigation");
        help_binding(left,&ly,"Wheel","Scroll vertically");
        help_binding(left,&ly,"Shift + wheel","Scroll horizontally");
#ifdef __APPLE__
        help_binding(left,&ly,"Cmd + wheel","Zoom around pointer");
        help_binding(left,&ly,"Two-finger scroll","Pan in both directions");
        help_binding(left,&ly,"Pinch","Zoom around pointer");
#else
        help_binding(left,&ly,"Ctrl + wheel","Zoom around pointer");
#endif
        help_binding(left,&ly,"Middle drag","Pan freely");
        ly+=12; help_group(left,&ly,"Transport & keyboard audition");
        help_binding(left,&ly,"Space","Play / stop from marker");
        help_binding(left,&ly,"Z row + S/D/G/H/J","Play notes from C3");
        help_binding(left,&ly,"Q row + number keys","Play notes from C4");
        ly+=12; help_group(left,&ly,"Editor windows");
        help_binding(left,&ly,"Drag title bar","Move window");
        if(ui.windows.workspace.enabled)help_binding(left,&ly,"PATT / SONG","Choose playback source");
        else help_binding(left,&ly,"Double-click title","Maximize / restore");
        ly+=12; help_group(left,&ly,"Playlist tools");
        help_binding(left,&ly,"Cut: click clip","Split at snapped position");
        help_binding(left,&ly,"Stretch: drag edge","Resample / stretch audio");
#ifdef __APPLE__
        help_binding(left,&ly,"Cmd + Z / Shift + Z","Undo / redo");
        help_binding(left,&ly,"Cmd + S / O","Save / open (Shift+S: As)");
#else
        help_binding(left,&ly,"Ctrl + Z / Shift + Z","Undo / redo (also Ctrl + Y)");
        help_binding(left,&ly,"Ctrl + S / O","Save / open (Shift+S: As)");
#endif
        ly+=12; help_group(left,&ly,"Piano Roll velocity lane");
        help_binding(left,&ly,"Left-drag","Paint note velocities");
        help_binding(left,&ly,"Right-drag","Draw a straight velocity ramp");
        ly+=12; help_group(left,&ly,"MIDI & recording");
        help_binding(left,&ly,"Record type icons","Audio / Notes / Automation");
        help_binding(left,&ly,"Right-click Record","Choose MIDI input / settings");
        help_binding(left,&ly,"Control right-click","MIDI Learn: move a CC knob");
        help_group(right,&ry,"Editing & selection");
#ifdef __APPLE__
        help_binding(right,&ry,"Cmd + drag","Temporarily box-select");
#else
        help_binding(right,&ry,"Ctrl + drag","Temporarily box-select");
#endif
        help_binding(right,&ry,"Shift-click","Add / remove selection");
        help_binding(right,&ry,"Shift-drag empty","Add selection rectangle");
        help_binding(right,&ry,"Shift + Left/Right","Move notes one step");
        help_binding(right,&ry,"Shift + Up/Down","Move notes one semitone");
#ifdef __APPLE__
        help_binding(right,&ry,"Cmd + C / X","Copy / cut selection");
        help_binding(right,&ry,"Cmd + V","Paste at start marker");
        help_binding(right,&ry,"Cmd + A","Select all notes / clips");
        help_binding(right,&ry,"Cmd + Up/Down (K/J)","Shift notes an octave");
#else
        help_binding(right,&ry,"Ctrl + C / X","Copy / cut selection");
        help_binding(right,&ry,"Ctrl + V","Paste at start marker");
        help_binding(right,&ry,"Ctrl + A","Select all notes / clips");
        help_binding(right,&ry,"Ctrl + Up/Down (K/J)","Shift notes an octave");
#endif
        help_binding(right,&ry,"Delete","Delete selected clips / notes");
        help_binding(right,&ry,"Right-drag","Erase clips / notes");
        ry+=12; help_group(right,&ry,"Automation");
        help_binding(right,&ry,"Right-click control","Create automation");
        help_binding(right,&ry,"Click / drag curve","Add / move a point");
        help_binding(right,&ry,"Right-click point","Delete point");
        ry+=12; help_group(right,&ry,"Text fields & dialogs");
#ifdef __APPLE__
        help_binding(right,&ry,"Cmd + A / V","Select all / paste");
#else
        help_binding(right,&ry,"Ctrl + A / V","Select all / paste");
#endif
        help_binding(right,&ry,"Enter / Escape","Apply / cancel");
        help_binding(right,&ry,"Backspace","Delete text");
        ry+=12; help_group(right,&ry,"Browser (hover or focus)");
        help_binding(right,&ry,"Up/Down or K/J","Select & preview");
        help_binding(right,&ry,"Left or H","Collapse / parent folder");
        help_binding(right,&ry,"Right or L","Expand / child folder");
        host_end_scissor_mode();
        label(maximum>0?"Wheel to scroll keybindings":"Navigation follows the pointer. Audition requires Keys enabled.",x+18,y+h-24,11,muted);
        return;
    }
    char *text=number?ui.number_text:folder?ui.folder_text:ui.rename_text; size_t capacity=number?sizeof ui.number_text:folder?sizeof ui.folder_text:sizeof ui.rename_text;
    popup_caption(number?ui.number_name:folder?"Add Browser folder":(ui.rename_automation>=0?"Rename automation":ui.rename_channel>=0?host_text_format("Rename channel %d",ui.rename_channel+1):ui.rename_mixer>=0?host_text_format("Rename insert %d",ui.rename_mixer+1):ui.rename_track>=0?host_text_format("Rename track %d",ui.rename_track+1):host_text_format("Rename pattern %d",ui.rename_pattern+1)),x,y,w);
    if(ui.studio_style)ui_text_well(host_draw(),(DrawRect){x+8,y+32,w-16,28},1);
    else {host_draw_rectangle(x+8,y+32,w-16,28,bg); host_draw_rectangle_lines(x+8,y+32,w-16,28,cell);}
    int selection_painted=ui.rename_select_all;
    if(ui.rename_select_all) host_draw_rectangle(x+12,y+38,fminf(w-26,text_width(text,13)),16,ui.studio_style?accent:cell);
    if(hover(x+8,y+32,w-16,28)) snprintf(ui.status,sizeof ui.status,number?"Enter an exact value using a decimal dot or comma":folder?"Enter the folder path to add as a Browser root":"Enter a name");
    if(hover(x+8,y+32,w-16,28) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) ui.rename_select_all=0;
    text_input(text,capacity);
    label(fit_text(text,w-26,13),x+12,y+40,13,ui.studio_style?theme_foreground(selection_painted?accent:ui_skin.field):ink);
    label(number?host_text_format("%.6g to %.6g; decimal dot or comma",ui.number_low,ui.number_high):"Choose Apply or Cancel below",x+8,y+66,11,muted);
    int apply=button("Apply",x+8,y+84,76,24,1) || host_is_key_pressed(HOST_KEY_ENTER);
    if(button("Cancel",x+92,y+84,76,24,0)) ui.pattern_popup=0;
    if(number && button("Reset",x+176,y+84,76,24,0)) { if(ui.number_target==&ui.rack_fill_interval || ui.number_target==&ui.mixer_add_count){snprintf(ui.number_text,sizeof ui.number_text,ui.number_target==&ui.mixer_add_count?"1":"4");ui.rename_select_all=1;}else{*ui.number_target=ui.number_default;if(ui.number_target==&ui.record_count_steps)midi_save_settings();ui.pattern_popup=0;} }
    if(apply) {
        if(number) {
            if(!text_number_parse(text,ui.number_low,ui.number_high,ui.number_integer,ui.number_target)) { snprintf(ui.status,sizeof ui.status,"Enter a number from %.6g to %.6g.",ui.number_low,ui.number_high); return; }
            if(ui.number_target==&ui.mixer_add_count)mixer_add_inserts((int)ui.mixer_add_count);
            if(ui.number_target==&ui.record_count_steps)midi_save_settings();
            if(ui.number_target==&ui.rack_fill_interval)desktop_rack_fill(ui.rack_fill_pattern,ui.rack_fill_channel,(int)ui.rack_fill_interval);
        } else if(folder) {
            if(!browser_add(&ui.browser,text)) { snprintf(ui.status,sizeof ui.status,"Cannot add folder: check its path, permissions or the eight-folder limit."); return; }
            snprintf(ui.status,sizeof ui.status,"Browser folder saved.");
        } else {
            const char *name=text_rename_name(text);
            if(!name) { snprintf(ui.status,sizeof ui.status,"Name cannot be empty."); return; }
            snprintf(ui.rename_automation>=0?ui.project.automations[ui.rename_automation].name:ui.rename_channel>=0?ui.project.channel_names[ui.rename_channel]:ui.rename_mixer>=0?ui.project.insert_names[ui.rename_mixer]:ui.rename_track>=0?ui.project.track_names[ui.rename_track]:ui.project.pattern_names[ui.rename_pattern],PATTERN_NAME,"%s",name);
        }
        ui.pattern_popup=0;
    }
}

void draw_popup(void) {
    int kind=ui.pattern_popup;
    int navigation=kind==1 || kind==6 || kind==7 || kind==8 || kind==9 || kind==10 || kind==12;
    if(navigation) menu_keys_begin(kind,ui.popup_opened,0,kind==8?&ui.device_scroll:NULL,ui.device_count+2);
    draw_popup_content();
    if(navigation) menu_keys_end();
    if(!ui.pattern_popup && !ui.context_kind) ui.menu_keys.id=0;
}

void file_picker_sync(void) {
    snprintf(ui.file_directory,sizeof ui.file_directory,"%s",ui.file_picker.directory);
    ui.file_last_row=-1; ui.file_last_click=-1; ui.file_scroll_drag=0;
}

void preset_action(int kind,int owner,int slot,int save) {
    recording_finish();
    ui.preset_kind=kind; ui.preset_owner=owner; ui.preset_slot=slot;
    char initial[LL_PATH_CAPACITY],directory[LL_PATH_CAPACITY];
    if(!preset_directory(directory,sizeof directory,kind)) snprintf(directory,sizeof directory,"%s",host_get_working_directory());
    const char *name=kind==PRESET_FM && ui.studio_style?channel_instrument_name(&ui.project,owner):device_descriptor(kind)->name;
    if(snprintf(initial,sizeof initial,"%s/%s.llpreset",directory,name)>=(int)sizeof initial || !file_chooser_begin_recent(&ui.file_picker,initial,"llpreset",save,save?FILE_PRESET_SAVE:FILE_PRESET_OPEN)) {
        snprintf(ui.status,sizeof ui.status,"Cannot open preset chooser"); return;
    }
    ui.file_action=save?8:9; ui.file_field=save?2:0; ui.file_confirm=0;
    file_picker_sync(); ui.rename_select_all=save; open_popup(11);
}





void project_file_action(int action) {
    recording_finish();
    if(action==2 && ui.document.saved_on_disk) {
        int ok=sampler_flush() && project_save_assets(ui.document.path,&ui.project,ui.originals,0);
        snprintf(ui.status,sizeof ui.status,ok?"Saved %.220s":"Save failed: %.220s",ui.document.path);
        if(ok) replacement_saved(); else if(ui.document.pending_action) open_popup(12);
        return;
    }
    const char *current=action==7?ui.project.paths[ui.relink_channel]:action==5?(ui.document.export_path[0]?ui.document.export_path:"song.wav"):ui.document.path;
    char initial[LL_PATH_CAPACITY];
    int length=host_path_absolute(current)?snprintf(initial,sizeof initial,"%s",current):snprintf(initial,sizeof initial,"%s/%s",host_get_working_directory(),current);
    if(length>=(int)sizeof initial) { snprintf(ui.status,sizeof ui.status,"File path is too long."); return; }
    if(!file_chooser_begin_recent(&ui.file_picker,initial,action==7?"wav;flac;mp3":action==5?"wav":PROJECT_FILE_EXTENSION,action!=4 && action!=7,action==4?FILE_OPEN:action==5?FILE_EXPORT:action==7?FILE_SAMPLE:FILE_SAVE)) { snprintf(ui.status,sizeof ui.status,"%s",ui.file_picker.error); return; }
    ui.file_action=action; ui.file_field=action==4 || action==7?0:2; ui.file_confirm=0;
    file_picker_sync(); ui.rename_select_all=ui.file_field==2; open_popup(11);
}

int file_picker_commit(void) {
    if(!project_file_commit(ui.file_pending)) return 0;
    file_chooser_remember(&ui.file_picker,ui.file_pending); return 1;
}

void file_picker_submit(void) {
    int result=file_chooser_path(&ui.file_picker,ui.file_pending,sizeof ui.file_pending);
    if(result<0) return;
    if(result==0) { ui.file_picker.name[0]=0; file_picker_sync(); return; }
    if(result==2) { ui.file_confirm=1; ui.file_field=0; ui.rename_select_all=0; return; }
    if(file_picker_commit()) { ui.pattern_popup=0; if(ui.file_action==2 || ui.file_action==3 || ui.file_action==6) replacement_saved(); }
    else snprintf(ui.file_picker.error,sizeof ui.file_picker.error,"%.159s",ui.status);
}

void draw_file_picker(int x,int y,int w,int h,float scale) {
    FileChooser *c=&ui.file_picker;
    popup_caption(ui.file_action==8?"Save preset":ui.file_action==9?"Load preset":ui.file_action==4?"Open project":ui.file_action==5?"Export WAV":ui.file_action==6?"Collect samples and save":ui.file_action==7?"Relink sample":ui.file_action==3?"Save project as":"Save project",x,y,w);
    if(ui.file_confirm) {
        label("Replace this existing file?",x+16,y+56,15,ink);
        label(fit_text(ui.file_pending,w-32,13),x+16,y+86,13,muted);
        if(button("Replace",x+16,y+126,100,26,1) || host_is_key_pressed(HOST_KEY_ENTER)) {
            if(file_picker_commit()) { ui.pattern_popup=0; if(ui.file_action==2 || ui.file_action==3 || ui.file_action==6) replacement_saved(); }
            else { ui.file_confirm=0; snprintf(c->error,sizeof c->error,"%.159s",ui.status); }
        }
        if(button("Back",x+124,y+126,100,26,0)) ui.file_confirm=0;
        return;
    }
    if(button("Up",x+12,y+30,44,26,0) && file_chooser_parent(c)) file_picker_sync();
    const char *home=getenv("HOME");
    if(button("Home",x+60,y+30,54,26,0) && home && file_chooser_folder(c,home)) file_picker_sync();
    if(button("Hidden",x+w-84,y+30,72,26,c->hidden)) {
        c->hidden=!c->hidden; if(file_chooser_folder(c,c->directory)) file_picker_sync();
    }
    DrawRect path_box={x+122,y+30,w-214,26};
    if(ui.studio_style)ui_text_well(host_draw(),path_box,ui.file_field==1);
    else {host_draw_rectangle_rec(path_box,bg); host_draw_rectangle_lines_ex(path_box,1,ui.file_field==1?accent:cell);}
    if(hover(path_box.x,path_box.y,path_box.width,path_box.height) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) { ui.file_field=1; ui.rename_select_all=1; }
    int path_submitted=ui.file_field==1 && host_is_key_pressed(HOST_KEY_ENTER);
    if(ui.file_field==1) {
        text_input(ui.file_directory,sizeof ui.file_directory);
        if(host_is_key_pressed(HOST_KEY_ENTER) && file_chooser_folder(c,ui.file_directory)) { file_picker_sync(); ui.file_field=0; ui.rename_select_all=0; }
    }
    if(ui.file_field==1 && ui.rename_select_all) host_draw_rectangle(path_box.x+4,path_box.y+5,fminf(path_box.width-8,text_width(ui.file_directory,12)),16,ui.studio_style?accent:cell);
    label(fit_text(ui.file_directory,path_box.width-8,12),path_box.x+4,path_box.y+6,12,ui.studio_style?theme_foreground(ui.file_field==1 && ui.rename_select_all?accent:ui_skin.field):ink);
    int list_y=y+66,list_h=h-174,rows=list_h/24,list_w=w-38;
    if(hover(x+12,list_y,w-24,list_h)) c->scroll-=host_get_mouse_wheel_move()*3;
    if(ui.file_field!=1 && ui.file_field!=2) {
        if(host_is_key_pressed(HOST_KEY_UP)) c->selected=c->selected<0?0:(int)fmaxf(0,c->selected-1);
        if(host_is_key_pressed(HOST_KEY_DOWN)) c->selected=(int)fminf(c->count-1,c->selected+1);
        if(c->selected>=0 && c->selected<c->count) {
            if(c->selected<c->scroll) c->scroll=c->selected;
            if(c->selected>=c->scroll+rows) c->scroll=c->selected-rows+1;
            if(!c->entries[c->selected].directory) snprintf(c->name,sizeof c->name,"%s",c->entries[c->selected].name);
        }
    }
    int maximum=(int)fmaxf(0,c->count-rows); c->scroll=(int)fmaxf(0,fminf(maximum,c->scroll));
    if(ui.studio_style)ui_text_well(host_draw(),(DrawRect){x+12,list_y,w-24,list_h},0);
    else host_draw_rectangle(x+12,list_y,w-24,list_h,bg);
    host_begin_scissor_mode((x+12)*scale,list_y*scale,list_w*scale,list_h*scale);
    int activate=-1;
    for(int row=c->scroll;row<c->count && row<c->scroll+rows;row++) {
        FileEntry *entry=&c->entries[row]; int ry=list_y+(row-c->scroll)*24;
        int clicked=ui.studio_style?button_color("",x+12,ry,list_w,24,c->selected==row,ui_skin.field):button("",x+12,ry,list_w,24,c->selected==row);
        if(clicked) {
            double now=host_get_time(); activate=ui.file_last_row==row && now-ui.file_last_click<.35?row:-1;
            c->selected=row; ui.file_field=0; ui.rename_select_all=0; c->error[0]=0;
            if(!entry->directory) snprintf(c->name,sizeof c->name,"%s",entry->name);
            ui.file_last_row=row; ui.file_last_click=now;
        }
        if(entry->directory) {
            if(ui.studio_style)ui_folder_icon(host_draw(),x+20,ry+5);
            else {host_draw_rectangle_lines(x+20,ry+8,12,9,c->selected==row?ui_theme.selected_text:muted); host_draw_rectangle(x+20,ry+5,6,3,c->selected==row?ui_theme.selected_text:muted);}
        }
        label(fit_text(entry->name,list_w-40,13),x+40,ry+5,13,c->selected==row?ui_theme.selected_text:ui.studio_style?theme_foreground(cell):ink);
    }
    host_begin_scissor_mode(x*scale,y*scale,w*scale,h*scale);
    float thumb=fminf(list_h,fmaxf(24,list_h*rows/(float)fmaxf(1,c->count))),travel=list_h-thumb;
    float thumb_y=list_y+(maximum?c->scroll/(float)maximum*travel:0);
    if(hover(x+w-22,list_y,10,list_h) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) ui.file_scroll_drag=1;
    if(ui.file_scroll_drag) {
        if(host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT)) c->scroll=(int)fmaxf(0,fminf(maximum,(ui.mouse.y-list_y-thumb/2)/fmaxf(1,travel)*maximum));
        else ui.file_scroll_drag=0;
    }
    if(ui.studio_style) {
        ui_scrollbar(host_draw(),(DrawRect){x+w-22,list_y,8,list_h},(DrawRect){x+w-22,thumb_y,8,thumb},ui.file_scroll_drag);
        ui_bevel(host_draw(),(DrawRect){x+12,list_y,w-24,list_h},1);
    } else ui_surface(host_draw(),(DrawRect){x+w-22,thumb_y,8,thumb},ui.file_scroll_drag?accent:muted);
    if(!c->count) label("No matching files in this folder",x+24,list_y+12,13,muted);
    int enter=host_is_key_pressed(HOST_KEY_ENTER) && ui.file_field!=1 && !path_submitted;
    if(activate>=0 || (enter && ui.file_field==0 && c->selected>=0 && c->selected<c->count && c->entries[c->selected].directory)) {
        int selected=activate>=0?activate:c->selected;
        if(c->entries[selected].directory) {
            char path[LL_PATH_CAPACITY];
            if(snprintf(path,sizeof path,"%s/%s",c->directory,c->entries[selected].name)<(int)sizeof path && file_chooser_folder(c,path)) file_picker_sync();
        } else file_picker_submit();
        enter=0;
    }
    label(ui.file_action==8 || ui.file_action==9?"Device presets (*.llpreset)":ui.file_action==7?"Audio samples (*.wav, *.flac, *.mp3)":ui.file_action==5?"WAV audio (*.wav)":"LibreLoop projects (*" PROJECT_FILE_SUFFIX ")",x+12,y+h-102,11,muted);
    label("Filename",x+12,y+h-76,12,muted);
    DrawRect name_box={x+82,y+h-82,w-94,26};
    if(ui.studio_style)ui_text_well(host_draw(),name_box,ui.file_field==2);
    else {host_draw_rectangle_rec(name_box,bg); host_draw_rectangle_lines_ex(name_box,1,ui.file_field==2?accent:cell);}
    if(hover(name_box.x,name_box.y,name_box.width,name_box.height) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) { ui.file_field=2; ui.rename_select_all=1; }
    if(ui.file_field==2) text_input(c->name,sizeof c->name);
    if(ui.file_field==2 && ui.rename_select_all) host_draw_rectangle(name_box.x+4,name_box.y+5,fminf(name_box.width-8,text_width(c->name,13)),16,ui.studio_style?accent:cell);
    label(fit_text(c->name,name_box.width-8,13),name_box.x+4,name_box.y+6,13,ui.studio_style?theme_foreground(ui.file_field==2 && ui.rename_select_all?accent:ui_skin.field):ink);
    label(fit_text(c->error,w-210,11),x+12,y+h-30,11,ui_theme.meter_high);
    if(button("Cancel",x+w-188,y+h-38,80,26,0)) { ui.document.pending_action=0; ui.pattern_popup=0; snprintf(ui.status,sizeof ui.status,"Cancelled."); }
    if(button(ui.file_action==4 || ui.file_action==9?"Open":ui.file_action==7?"Relink":ui.file_action==5?"Export":"Save",x+w-100,y+h-38,88,26,1) || enter) file_picker_submit();
}

void delete_pattern(int target) {
    if(ui.edit_clipboard.kind==COPY_CLIPS) ui.edit_clipboard.kind=COPY_EMPTY;
    int old_count=ui.project.pattern_count;
    if(!pattern_delete(&ui.project,target)) return;
    for(int i=target;i<old_count-1;i++) {
        ui.rack_step_width[i]=ui.rack_step_width[i+1]; ui.rack_view[i]=ui.rack_view[i+1]; ui.rack_range[i]=ui.rack_range[i+1]; ui.piano_start[i]=ui.piano_start[i+1];
        ui.piano_span[i]=ui.piano_span[i+1]; ui.piano_pan[i]=ui.piano_pan[i+1]; ui.piano_range[i]=ui.piano_range[i+1];
        memcpy(ui.pattern_loop[i],ui.pattern_loop[i+1],sizeof ui.pattern_loop[i]);
        memcpy(ui.piano_channels[i],ui.piano_channels[i+1],sizeof ui.piano_channels[i]);
        memcpy(ui.note_selected[i],ui.note_selected[i+1],sizeof ui.note_selected[i]);
    }
    int last=old_count>1?ui.project.pattern_count:0;
    ui.rack_step_width[last]=0; ui.rack_view[last]=ui.rack_range[last]=ui.piano_start[last]=ui.piano_pan[last]=ui.piano_range[last]=0; ui.piano_span[last]=STEPS;
    memset(ui.pattern_loop[last],0,sizeof ui.pattern_loop[last]); memset(ui.piano_channels[last],0,sizeof ui.piano_channels[last]); memset(ui.note_selected[last],0,sizeof ui.note_selected[last]);
    ui.pattern=fminf(target,ui.project.pattern_count-1); ui.playing=0; ui.reset=1; ui.note_drag=NULL; ui.piano_gesture=ui.moving_note=0;
    ui.arrangement.gesture=IDLE; ui.arrangement.source_pattern=-1;
    memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected);
    snprintf(ui.status,sizeof ui.status,"Deleted pattern and its Playlist clips%s",old_count==1?"; kept one blank pattern":"");
}

static int studio_factory_count(int c){return c<0 || c>=ui.project.channel_count?0:fm_family_factory_count(ui.project.fm[c].engine);}
static FMSettings studio_factory(int c,int index){return fm_family_factory(ui.project.fm[c].engine,index);}
static const char *studio_factory_name(int c,int index){return fm_family_factory_name(ui.project.fm[c].engine,index);}
void draw_context_content(void) {
    if(!ui.context_kind) return;
    if(ui.windows.workspace.enabled && (ui.context_kind==14 || ui.context_kind==16) && ui.effect_context_window>=5){ui.eq_bus=ui.effect_windows[ui.effect_context_window].bus;ui.eq_slot=ui.effect_windows[ui.effect_context_window].slot;}
    float scale=ui_scale();
    int instrument_rows[CHANNELS],instrument_count=ui.context_kind==17?pattern_instruments(ui.context_target,instrument_rows):0;
    int route_rows[INSERTS+1],route_count=0;
    if(ui.context_kind==26 || ui.context_kind==27) {
        for(int id=ui.context_kind==26?0:1;id<=ui.project.insert_count;id++)
            if(ui.context_kind==26?insert_has_send(&ui.project,ui.context_target,id):insert_has_send(&ui.project,id,ui.context_target))route_rows[route_count++]=id;
    }
    int w=ui.context_kind==14 || ui.context_kind==16?160:ui.context_kind==28 || (ui.context_kind==25 && !ui.project.effect_type[ui.eq_bus][ui.context_target])?344:(ui.context_kind==5 || ui.context_kind==8 || ui.context_kind==12)?304:228,h=ui.context_kind==30?8+ARP_CHORDS*25:ui.context_kind==29?58:(ui.context_kind==26 || ui.context_kind==27)?8+25*(int)fminf(8,route_count+1):ui.context_kind==28?209:ui.context_kind==25?(ui.project.effect_type[ui.eq_bus][ui.context_target]?158:209):ui.context_kind==24?8+25*GRID_CHOICES:ui.context_kind==9?133:ui.context_kind==21?8+25*(ui.studio_style?studio_factory_count(ui.context_target):FM_FACTORY_COUNT):ui.context_kind==22?83:ui.context_kind==23?108:ui.context_kind==19?108:ui.context_kind==20?58:ui.context_kind==17?8+25*(int)fminf(8,instrument_count):ui.context_kind==18?(ui.studio_style?108:58):ui.context_kind==4?108:ui.context_kind==16?32:ui.context_kind==15?58:ui.context_kind==14?8+EQ_SHAPES*25:ui.context_kind==13?58:ui.context_kind==2?183:ui.context_kind==7?133:(ui.context_kind==5 || ui.context_kind==8 || ui.context_kind==12)?76:ui.context_kind==3 && !ui.context_target?32:82,x=fmaxf(0,fminf(host_get_screen_width()/scale-w,ui.context_position.x)),y=fmaxf(0,fminf(host_get_screen_height()/scale-h,ui.context_position.y));
    ui.input_enabled=1;
    if(host_is_key_pressed(HOST_KEY_ESCAPE) || (!ui.context_opened && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { ui.context_kind=0; return; }
    ui_frame(host_draw(),(DrawRect){x,y,w,h});
    int target=ui.context_target,kind=ui.context_kind;
    if(kind==29) {
        if(button("Add insert",x+4,y+4,w-8,24,0)){mixer_add_inserts(1);ui.context_kind=0;return;}
        if(button("Add multiple...",x+4,y+29,w-8,24,0)){begin_mixer_add();ui.context_kind=0;return;}
    } else if(kind==26 || kind==27) {
        int visible=fminf(8,route_count+1);
        if(hover(x,y,w,h))ui.route_menu_scroll=fmaxf(0,fminf(route_count+1-visible,ui.route_menu_scroll-host_get_mouse_wheel_move()));
        for(int row=0;row<visible;row++) {
            int index=row+ui.route_menu_scroll;
            const char *name=index==route_count?"Remove all cables":host_text_format("Remove %s %s",kind==26?"to":"from",mixer_name(route_rows[index]));
            if(button(name,x+4,y+4+row*25,w-8,24,0)) {
                for(int i=0;i<route_count;i++)if(index==route_count || i==index) {
                    if(kind==26)insert_send_remove(&ui.project,target,route_rows[i]);else insert_send_remove(&ui.project,route_rows[i],target);
                }
                ui.context_kind=0;return;
            }
        }
    } else if(kind==25 || kind==28) {
        int bus=ui.eq_bus,slot=target,type=ui.project.effect_type[bus][slot];
        if(!type || kind==28){
            const int types[]={EFFECT_CHORUS,EFFECT_EQ,EFFECT_DELAY,EFFECT_TUBE,EFFECT_REVERB,EFFECT_DYNAMICS,EFFECT_PHASER,EFFECT_FLANGER};
            for(unsigned row=0;row<sizeof types/sizeof *types;row++) {
                int next=types[row];
                if(button(host_text_format("%s %s",kind==28?"Replace with":"Add",effect_descriptor(next)->name),x+4,y+4+row*25,w-8,24,0)) {
                    if(kind==28)mixer_effect_replace(bus,slot,next);else mixer_effect_add(bus,slot,next);
                    ui.context_kind=0;return;
                }
            }
        } else {
            if(button("Open effect",x+4,y+4,w-8,24,0)){mixer_effect_open(bus,slot,0);ui.context_kind=0;}
            if(button("Bypass",x+4,y+29,w-8,24,ui.project.effect_bypass[bus][slot])){ui.project.effect_bypass[bus][slot]^=1;ui.context_kind=0;}
            if(button("Replace effect...",x+4,y+129,w-8,24,0)){open_context(28,slot,(DrawPoint){x,y});return;}
            if(button("Remove effect",x+4,y+54,w-8,24,0)){mixer_effect_remove(bus,slot);ui.context_kind=0;}
            if(button("Load preset",x+4,y+79,w-8,24,0)){preset_action(type==EFFECT_PHASER?PRESET_PHASER:type==EFFECT_FLANGER?PRESET_FLANGER:type==EFFECT_DYNAMICS?PRESET_DYNAMICS:type==EFFECT_REVERB?PRESET_REVERB:type==EFFECT_TUBE?PRESET_TUBE:type==EFFECT_DELAY?PRESET_DELAY:type==EFFECT_EQ?PRESET_EQ:PRESET_CHORUS,bus,slot,0);ui.context_kind=0;}
            if(button("Save preset",x+4,y+104,w-8,24,0)){preset_action(type==EFFECT_PHASER?PRESET_PHASER:type==EFFECT_FLANGER?PRESET_FLANGER:type==EFFECT_DYNAMICS?PRESET_DYNAMICS:type==EFFECT_REVERB?PRESET_REVERB:type==EFFECT_TUBE?PRESET_TUBE:type==EFFECT_DELAY?PRESET_DELAY:type==EFFECT_EQ?PRESET_EQ:PRESET_CHORUS,bus,slot,1);ui.context_kind=0;}
        }
    } else if(kind==24) {
        int *choice=target==1?&ui.playlist_grid:&ui.piano_grid;
        for(int row=0;row<GRID_CHOICES;row++){int item=grid_menu_choice(row);if(button(grid_choice_name(item),x+4,y+4+row*25,w-8,24,*choice==item)) {*choice=item;ui.context_kind=0;return;}}
    } else if(kind==21 || kind==22 || kind==23) {
        if(target<0 || target>=ui.project.channel_count || ui.project.instrument[target]!=INSTRUMENT_FM) { ui.context_kind=0; return; }
        const char *engines[]={"Custom FM","Six-operator FM","Analog"},*waves[]={"Saw","Pulse","Triangle","Sine"};
        int count=kind==21?(ui.studio_style?studio_factory_count(target):FM_FACTORY_COUNT):kind==22?3:4;
        for(int i=0;i<count;i++) if(button(kind==21?(ui.studio_style?studio_factory_name(target,i):fm_factory_name(i)):kind==22?engines[i]:waves[i],x+4,y+4+i*25,w-8,24,0)) {
            if(kind==21) ui.project.fm[target]=ui.studio_style?studio_factory(target,i):fm_factory(i);
            else if(kind==22) ui.project.fm[target].engine=i;
            else ui.project.fm[target].analog.wave=i;
            ui.fm_graph_drag=-1; ui.context_kind=0; return;
        }
    } else if(kind==19 || kind==20) {
        if(target<0 || target>=ui.project.channel_count || ui.project.instrument[target]!=INSTRUMENT_FM) { ui.context_kind=0; return; }
        const char *shapes[]={"Sine","Triangle","Saw","Square"},*routing[]={"Body + Attack","Attack into Body"};
        float *value=kind==19?&ui.project.fm[target].lfo_shape:&ui.project.fm[target].routing;
        for(int i=0;i<(kind==19?4:2);i++) if(button(kind==19?shapes[i]:routing[i],x+4,y+4+i*25,w-8,24,*value==i)) { *value=i; ui.context_kind=0; return; }
    } else if(kind==17) {
        int visible=(int)fminf(8,instrument_count);
        if(hover(x,y,w,h)) ui.instrument_replace_scroll=(int)fmaxf(0,fminf(instrument_count-visible,ui.instrument_replace_scroll-host_get_mouse_wheel_move()));
        for(int row=0;row<visible;row++) {
            int c=instrument_rows[row+ui.instrument_replace_scroll];
            if(button(fit_text(host_text_format("Replace %s...",ui.project.channel_names[c]),w-16,13),x+4,y+4+row*25,w-8,24,0)) {
                open_context(18,c,ui.context_position); return;
            }
        }
        if(hover(x,y,w,h)) snprintf(ui.status,sizeof ui.status,"Choose the pattern's instrument to replace; the change applies across all patterns. Scroll for more instruments.");
    } else if(kind==30) {
        if(target<0 || target>=ui.project.channel_count){ui.context_kind=0;return;}
        for(int i=0;i<ARP_CHORDS;i++)if(button(arp_chord_name(i),x+4,y+4+i*25,w-8,24,ui.project.arp[target].chord==i)){ui.project.arp[target].chord=i;ui.context_kind=0;return;}
    } else if(kind==18) {
        if(target<0 || target>=ui.project.channel_count) { ui.context_kind=0; return; }
        if(ui.studio_style){
            for(int choice=0;choice<INSTRUMENT_CHOICES;choice++)if(button_available(instrument_choice_name(choice),x+4,y+4+choice*25,w-8,24,channel_instrument_choice(&ui.project,target)==choice,choice==CHOICE_SAMPLER || !channel_uses_audio_clips(&ui.project,target))){
                ui.context_kind=0;replace_instrument_choice(target,choice);return;
            }
            return;
        }
        for(int type=INSTRUMENT_SAMPLER;type<=INSTRUMENT_FM;type++) {
            int enabled=ui.input_enabled;
            if(type==INSTRUMENT_FM && channel_uses_audio_clips(&ui.project,target)) ui.input_enabled=0;
            if(button(instrument_descriptor(type)->name,x+4,y+4+type*25,w-8,24,ui.project.instrument[target]==type)) {
                ui.context_kind=0; replace_instrument(target,type); return;
            }
            ui.input_enabled=enabled;
            if(hover(x+4,y+4+type*25,w-8,24)) snprintf(ui.status,sizeof ui.status,
                type==INSTRUMENT_FM && channel_uses_audio_clips(&ui.project,target)?"Audio clips require a Sampler; FM Synth replaces pattern instruments":"Replace this channel's instrument; keep notes, sample settings, name and mixer routing across all patterns");
        }
    } else if(kind==16) {
        if(ui.eq_bus>ui.project.insert_count || ui.project.effect_type[ui.eq_bus][ui.eq_slot]!=EFFECT_EQ || target<0 || target>=EQ_BANDS) { ui.context_kind=0; return; }
        if(button("Reset",x+4,y+4,w-8,24,0)) { ui.project.eq[ui.eq_bus][ui.eq_slot].bands[target]=equalizer_default().bands[target]; ui.context_kind=0; }
        if(hover(x+4,y+4,w-8,24)) snprintf(ui.status,sizeof ui.status,"Reset band %d frequency, gain, Q and shape to defaults",target+1);
    } else if(kind==14) {
        if(ui.eq_bus>ui.project.insert_count || ui.project.effect_type[ui.eq_bus][ui.eq_slot]!=EFFECT_EQ || target<0 || target>=EQ_BANDS) { ui.context_kind=0; return; }
        EQBand *band=&ui.project.eq[ui.eq_bus][ui.eq_slot].bands[target];
        for(int shape=0;shape<EQ_SHAPES;shape++) if(button(equalizer_shape_name(shape),x+4,y+4+shape*25,w-8,24,band->shape==(unsigned)shape)) { band->shape=shape; ui.context_kind=0; }
    } else if(kind==13) {
        for(int mode=0;mode<2;mode++) if(button(mode?"Stretch":"Resample",x+4,y+4+mode*25,w-8,24,ui.project.sampler[target].stretch==mode)) { ui.project.sampler[target].stretch=mode; ui.context_kind=0; }
    } else if(kind==9 || kind==15) {
        if(kind==9) {
            if(button("MIDI Learn",x+4,y+79,w-8,24,0)){ui.midi_learn_target=ui.automation_target;ui.midi_learning=1;ui.context_kind=0;snprintf(ui.status,sizeof ui.status,"Move a MIDI CC knob or fader to link %s; Escape cancels",ui.menu_name);}
            if(button("Clear MIDI link",x+4,y+104,w-8,24,0)){midi_unbind(&ui.project,ui.automation_target);ui.context_kind=0;}
        }
        if(button("Reset",x+4,y+4,w-8,24,0)) { *ui.menu_value=ui.menu_initial; ui.context_kind=0; }
        if(hover(x+4,y+4,w-8,24)) snprintf(ui.status,sizeof ui.status,"Reset %s to %.6g",ui.menu_name,ui.menu_initial);
        if(button("Enter value",x+4,y+29,w-8,24,0)) {
            begin_number(ui.menu_value,ui.menu_low,ui.menu_high,ui.menu_initial,ui.menu_name);
            const ParameterDescriptor *descriptor=kind==9?parameter_descriptor(ui.automation_target.parameter):NULL;
            ui.number_integer=descriptor && descriptor->kind==PARAMETER_INTEGER; ui.context_kind=0;
        }
        if(kind==9 && button("Create automation",x+4,y+54,w-8,24,0)) { create_automation(); ui.context_kind=0; }
    } else if(kind==10) {
        if(button("Create automation",x+4,y+4,w-8,24,0)) { create_automation(); ui.context_kind=0; }
        if(button(ui.automation_target.parameter==PARAM_MASTER_MUTE?"Clear all solos":"Toggle solo",x+4,y+29,w-8,24,0)) {
            if(ui.automation_target.parameter==PARAM_CHANNEL_MUTE) solo_toggle(ui.project.mute,ui.project.channel_count,ui.automation_target.owner);
            else if(ui.automation_target.parameter==PARAM_INSERT_MUTE) solo_toggle(ui.project.insert_mute,ui.project.insert_count,ui.automation_target.owner);
            else { for(int c=0;c<CHANNELS;c++) ui.project.mute[c]&=~2; for(int i=0;i<INSERTS;i++) ui.project.insert_mute[i]&=~2; for(int l=0;l<LANES;l++) ui.project.lane_mute[l]&=~2; }
            ui.context_kind=0;
        }
    } else if(kind==11) {
        if(button("Rename",x+4,y+4,w-8,24,0)) { ui.rename_track=ui.rename_mixer=ui.rename_channel=-1; snprintf(ui.rename_text,sizeof ui.rename_text,"%s",ui.project.automations[target].name); ui.rename_select_all=1; open_popup(2); ui.rename_automation=target; ui.context_kind=0; }
        if(button("Color",x+4,y+29,w-8,24,0)) { ui.context_kind=12; ui.context_opened=1; }
        if(button("Delete",x+4,y+54,w-8,24,0)) { automation_delete(&ui.project,target); ui.automation_selected=-1; ui.automation_node=-1; ui.picker_drag=-1; ui.arrangement.gesture=IDLE; ui.arrangement.source_pattern=-1; memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected); ui.context_kind=0; }
    } else if(kind==6) {
        for(int i=0;i<3;i++) if(button(ui.rack_filters[i],x+4,y+4+i*25,w-8,24,ui.rack_filter==i)) { ui.rack_filter=i; ui.rack_scroll=0; ui.rack_scroll_remainder=0; ui.context_kind=0; }
    } else if(kind==5 || kind==8 || kind==12) {
        uint32_t *color=kind==12?&ui.project.automations[target].color:kind==8?&ui.project.channel_colors[target]:&ui.project.pattern_colors[target];
        int choice=color_picker(x+8,y+10,w-16,*color);
        if(choice>=0) { *color=pattern_palette[choice]; ui.context_kind=0; }
    } else if(kind==7) {
        if(button("Rename",x+4,y+4,w-8,24,0)) { ui.rename_track=ui.rename_mixer=-1; ui.rename_channel=target; snprintf(ui.rename_text,sizeof ui.rename_text,"%s",ui.project.channel_names[target]); ui.rename_select_all=1; open_popup(2); ui.context_kind=0; }
        if(button("Color",x+4,y+29,w-8,24,0)) { ui.context_kind=8; ui.context_opened=1; }
        if(button("Delete",x+4,y+54,w-8,24,0)) { ui.picker_drag=-1; delete_channel(target); ui.context_kind=0; }
        if(button(ui.project.instrument[target]==INSTRUMENT_FM?"Replace with sample...":"Relink sample...",x+4,y+79,w-8,24,0)) { ui.relink_channel=target; ui.context_kind=0; project_file_action(7); }
        if(button("Replace instrument...",x+4,y+104,w-8,24,0)) { open_context(18,target,ui.context_position); return; }
        if(hover(x+4,y+54,w-8,24)) snprintf(ui.status,sizeof ui.status,"%s",ui.project.channel_audio[target]?"Delete this Audio channel and all its Playlist clips; the source file stays on disk":"Delete this sampler channel, its Pattern notes and Arrangement clips; the source file stays on disk");
    } else if(kind==4) {
        if(button("Rename",x+4,y+4,w-8,24,0)) { ui.pattern=target; begin_rename(); ui.context_kind=0; }
        if(button("Color",x+4,y+29,w-8,24,0)) { ui.context_kind=5; ui.context_opened=1; }
        if(button("Delete",x+4,y+54,w-8,24,0)) { delete_pattern(target); ui.context_kind=0; }
        if(button("Replace instrument...",x+4,y+79,w-8,24,0)) { ui.instrument_replace_scroll=0; open_context(17,target,ui.context_position); return; }
        if(hover(x+4,y+54,w-8,24)) snprintf(ui.status,sizeof ui.status,"Delete this pattern and all its Playlist clips; the last pattern becomes blank");
    } else if(kind==1 && ui.windows.workspace.enabled) {
        if(target>=4 && button("Close editor",x+4,y+4,w-8,24,0)){studio_workspace_hide(&ui.windows,target);ui.context_kind=0;}
        else if(target<4)label(target==3?"Resize using divider":"Use View switch above",x+8,y+12,11,muted);
    } else if(kind==1) {
        if(button("Stay on top",x+4,y+4,w-8,24,ui.windows.editors[target].pinned)) { windows_pin(&ui.windows,target); ui.context_kind=0; }
        if(button(ui.windows.editors[target].maximized?"Restore":"Maximize",x+4,y+29,w-8,24,0)) {
            Editor *e=&ui.windows.editors[target]; if(e->maximized) { e->rect=e->restore; e->maximized=0; } else { e->restore=e->rect; e->maximized=1; } ui.context_kind=0;
        }
        if(button("Hide window",x+4,y+54,w-8,24,0)) { ui.windows.editors[target].visible=0; ui.context_kind=0; }
    } else if(kind==2) {
        if(button("Go to Piano Roll",x+4,y+4,w-8,24,0)) { ui.channel=ui.piano_channel=target; ui.piano_channels[ui.pattern][target]=1; windows_focus(&ui.windows,2); ui.context_kind=0; }
        if(button("Mute channel",x+4,y+29,w-8,24,ui.project.mute[target]&1)) { ui.project.mute[target]^=1; ui.context_kind=0; }
        if(button("Rename",x+4,y+54,w-8,24,0)) { ui.rename_track=ui.rename_mixer=-1; ui.rename_channel=target; snprintf(ui.rename_text,sizeof ui.rename_text,"%s",ui.project.channel_names[target]); ui.rename_select_all=1; open_popup(2); ui.context_kind=0; }
        if(button("Delete channel",x+4,y+79,w-8,24,0)) { delete_channel(target); ui.context_kind=0; }
        if(button(ui.project.instrument[target]==INSTRUMENT_FM?"Replace with sample...":"Relink sample...",x+4,y+104,w-8,24,0)) { ui.relink_channel=target; ui.context_kind=0; project_file_action(7); }
        if(button("Replace instrument...",x+4,y+129,w-8,24,0)) { open_context(18,target,ui.context_position); return; }
        if(button("Fill every X steps...",x+4,y+154,w-8,24,0)) {
            begin_rack_fill(target);ui.context_kind=0;
        }
    } else if(!target) {
        if(button("Reset Master volume",x+4,y+4,w-8,24,0)) { ui.project.master=1; ui.context_kind=0; }
    } else {
        if(button("Mute insert",x+4,y+4,w-8,24,target && (ui.project.insert_mute[target-1]&1))) { if(target) ui.project.insert_mute[target-1]^=1; ui.context_kind=0; }
        if(button("Solo insert",x+4,y+29,w-8,24,target && (ui.project.insert_mute[target-1]&2))) { if(target) solo_toggle(ui.project.insert_mute,ui.project.insert_count,target-1); ui.context_kind=0; }
        if(button("Reset mixer channel",x+4,y+54,w-8,24,0)) { if(target) insert_reset(&ui.project,target); else ui.project.master=1; ui.context_kind=0; }
    }
}

void draw_context(void) {
    int kind=ui.context_kind;
    int navigation=kind && kind!=5 && kind!=8 && kind!=12;
    int rows[CHANNELS],total=kind==17?pattern_instruments(ui.context_target,rows):0;
    if(navigation) menu_keys_begin(100+kind,ui.context_opened,0,kind==17?&ui.instrument_replace_scroll:NULL,total);
    draw_context_content();
    if(navigation) menu_keys_end();
    if(!ui.pattern_popup && !ui.context_kind) ui.menu_keys.id=0;
}
