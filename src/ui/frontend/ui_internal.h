// SPDX-License-Identifier: GPL-3.0-only
#ifndef UI_INTERNAL_H
#define UI_INTERNAL_H
/* Private desktop interface. UiState owns document, interaction, caches and
   services; DSP and project libraries do not depend on this header. The UI is
   single-threaded. Its sampler worker uses only the dedicated sampler_job. */
#include "host.h"
#include "grid.h"
#include "control.h"
#include "studio_layout.h"
#include "icons.h"
#include "audio.h"
#include "editor_windows.h"
#include "browser.h"
#include "theme.h"
#include "arrangement.h"
#include "edit_clipboard.h"
#include "waveform.h"
#include "navigation.h"
#include "history.h"
#include "file_chooser.h"
#include "project_assets.h"
#include "project_document.h"
#include "project_format.h"
#include "text_fonts.h"
#include "recording.h"
#include "midi_input.h"
#include "menu_navigation.h"
#include "midi_session.h"
#include "record_count_in.h"
#include "instrument_layout.h"
#include "velocity_lane.h"
#include "mixer_view.h"
#include "effect_chain.h"
#include "transport_readout.h"
#include "piano.h"
#include "atomic_file.h"
#include "preset.h"
#include "spectrum.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stdatomic.h>
#include <stddef.h>

enum { RECORD_AUDIO=1,RECORD_NOTES=2,RECORD_AUTOMATION=4 };
enum { PLAYLIST_GRID_TOP=96, PLAYLIST_BOTTOM=18, PIANO_GRID_TOP=86, PIANO_GRID_PADDING=180, EDITOR_SCROLLBAR=16, EDITOR_CORNER=16 };
typedef struct {
    Waveform wave;
    Sample crop;
    Sampler settings;
    unsigned offset,generation,epoch;
    int valid;
    float gain;
} SamplerView;
typedef struct {
    Sample sample; Waveform *wave;
    int channel,preview;
    const AudioTimeline *timeline;
    double start,offset,frames_per_step;
    float origin,width,pixels_per_step;
} WaveDisplay;
enum { KNOB_NORMAL,KNOB_SWING,KNOB_PAN,KNOB_WIDTH,KNOB_CENTER,KNOB_VOLUME,KNOB_LOGARITHMIC,KNOB_FIXED_COARSE,KNOB_MAGNETIC };

#define host_is_mouse_button_pressed mouse_pressed
#define host_set_mouse_cursor request_cursor
#define bg ui_theme.background
#define panel ui_theme.surface
#define cell ui_theme.control
#define ink ui_theme.text
#define muted ui_theme.secondary
#define accent ui_theme.highlight
#define RACK_STEP_WIDTH 16
#define RACK_TOP (TITLE+14)
#define MIXER_LEFT 62
#define MIXER_PANEL 160
#define KNOB_RADIUS 10
#define SWING_RADIUS 8

typedef struct {
    char samples_path[LL_PATH_CAPACITY],font_path[LL_PATH_CAPACITY],logo_path[LL_PATH_CAPACITY];
    int studio_style,profile_flat;
    Rect picker_bounds;
    unsigned pending_clicks[8],frame_clicks[8];
    int requested_cursor,current_cursor;
    PlatformWindow *app_window;
    Project project;
    History edit_history;
    Sample samples[CHANNELS],originals[CHANNELS];
    RecordingSession recording_ui;
    MidiTake midi_take;
    unsigned record_mask;
    int midi_record_target;
    MidiDevice midi_devices[64];
    int midi_device_count,midi_connected,midi_scroll,midi_learning;
    char midi_selected[128],midi_selected_name[128],midi_settings_path[LL_PATH_CAPACITY];
    double midi_refresh_time;
    ParameterTarget midi_learn_target;
    MidiHeld midi_keys[32];
    unsigned char midi_sustain[16];
    Sampler sampler_applied[CHANNELS];
    unsigned sample_generation[CHANNELS],sampler_generation[CHANNELS],sample_epoch;
    struct {
        PlatformThread * thread; atomic_int done; int busy,channel,ok;
        unsigned generation,epoch; Sampler settings; Sample input,result;
    } sampler_job;
    DrawPoint mouse;
    int pattern,channel,piano_channel,instrument_channel,playing,song,reset;
    RecordCountIn record_count_in;
    int record_count_enabled,record_count_target;
    float record_count_steps,record_count_origin;
    int typing_keys;
    float output_volume;
    int metronome;
    int follow_playhead;
    int position_seconds;
    double visual_step;
    Windows windows;
    int input_enabled;
    int picker_tab,picker_scroll,picker_drag;
    int automation_selected,automation_node,automation_lane,automation_clip;
    int rename_automation;
    float automation_grab_x;
    ParameterTarget automation_target;
    float *menu_value,menu_low,menu_high,menu_initial;
    char menu_name[96];
    DrawPoint picker_origin,picker_offset;
    Arrangement arrangement;
    int piano_grid,playlist_grid;
    float rack_view[PATTERNS],rack_range[PATTERNS],rack_step_width[PATTERNS];
    int rack_hdrag;
    float rack_hx,rack_hstart,rack_hscale;
    int playlist_pan,playlist_vpan;
    float track_scroll,playlist_vpan_y,playlist_vpan_start;
    float track_heights[LANES],track_resize_y,track_resize_height;
    float track_zoom,row_zoom_y,row_zoom_start,row_zoom_anchor;
    int row_zoom_drag;
    int selected_track;
    int track_resize;
    float playlist_pan_x,playlist_pan_start,playlist_pan_range;
    float playlist_start,piano_start[PATTERNS];
    int marker_drag,ruler_loop_drag,stop_armed;
    int ruler_last_id,ruler_last_pattern,ruler_last_button;
    double ruler_last_click;
    DrawPoint ruler_last_position;
    float song_loop[2],pattern_loop[PATTERNS][2],ruler_anchor;
    unsigned char piano_channels[PATTERNS][CHANNELS];
    int rack_vdrag;
    float rack_vy,rack_vstart,rack_vscale;
    int rack_filter;
    const char *rack_filters[3];
    float rack_scroll_remainder;
    struct {int bus,slot,band;} effect_windows[WINDOW_CAPACITY];
    int effect_context_window,eq_drag_window,eq_spectrum_bus;
    int rack_scroll,context_kind,context_target,context_opened,instrument_replace_scroll;
    DrawPoint context_position;
    int pattern_popup,popup_opened,rename_pattern,rename_select_all,popup_drag;
    DrawPoint popup_position[14],popup_offset;
    char rename_text[PATTERN_NAME],folder_text[LL_PATH_CAPACITY],number_text[64];
    float *number_target,number_low,number_high,number_default;
    const char *number_name;
    char device_names[64][128];
    int device_count,device_target,device_io,device_scroll;
    Browser browser;
    int route_drag,route_start,mixer_selected,mixer_scroll,mixer_pan,cable_drag;
    float mixer_offset,mixer_scroll_grab,mixer_fx_scroll[INSERTS+1];
    int mixer_fx_hidden,fx_press,fx_dragging,fx_bus,fx_slot,fx_drop,fx_add_pending,route_menu_scroll;
    DrawPoint fx_press_point;
    float route_y;
    Sample audition;
    double browser_progress;
    char audition_path[LL_PATH_CAPACITY];
    Waveform audition_wave;
    int browser_focus,sample_moved,browser_hidden,browser_resize;
    float browser_width,last_sidebar;
    char sample_drag[LL_PATH_CAPACITY];
    DrawPoint sample_origin,sample_offset;
    float sample_drag_width,sample_text_offset;
    float *control_drag,control_low,control_high,control_bottom,control_range;
    int control_horizontal,control_fader,control_reverse,control_integer,control_logarithmic,control_magnetic,number_integer;
    float control_raw;
    ControlWheel control_wheel;
    Note *note_drag;
    int moving_note,piano_scroll_drag,piano_vdrag;
    int piano_top;
    float piano_zoom;
    int piano_key_drag,piano_key,piano_key_channel;
    float piano_scroll_x,piano_scroll_start,piano_scroll_range,piano_vscroll_y,piano_vscroll_start;
    Note note_before[NOTES];
    uint8_t note_selected[PATTERNS][CHANNELS][NOTES],note_selection_before[NOTES];
    uint8_t keyboard_notes[CHANNELS][128],playing_notes[CHANNELS][NOTES],playing_keys[CHANNELS][128];
    int piano_tool,piano_gesture,piano_additive;
    uint8_t pattern_hits[CHANNELS]; /* View/entry mode only; never changes saved notes. */
    int pattern_channel_scroll,pattern_hit_active,pattern_hit_erase;
    float pattern_hit_step;
    DrawPoint piano_from,piano_now;
    DrawPoint note_grab;
    float piano_note_length;
    EditClipboard edit_clipboard;
    float piano_span[PATTERNS],piano_pan[PATTERNS],piano_range[PATTERNS];
    VelocityLane velocity_lane;
    int velocity_drag,velocity_pattern,velocity_channel;
    DrawPoint velocity_from,velocity_now;
    Note velocity_before[NOTES];
    int rack_paint,rack_paint_channel;
    float rack_paint_step,rack_fill_interval;
    int rack_fill_pattern,rack_fill_channel;
    int navigation_drag,navigation_active;
    DrawPoint navigation_last;
    NavigationInput navigation_input;
    float piano_scroll_remainder;
    int eq_drag;
    int fm_dx_operator,fm_dx_page,fm_analog_tab,fm_dx_drag_base;
    int fm_tab,fm_motion_target,fm_graph_drag,fm_graph_channel,fm_graph_tab;
    DrawRect fm_graph_area;
    char status[256];
    ProjectDocument document;
    int help_scroll,relink_channel;
    FileChooser file_picker;
    int preset_kind,preset_owner,preset_slot;
    int file_action,file_field,file_confirm,file_scroll_drag,file_last_row;
    double file_last_click;
    char file_directory[LL_PATH_CAPACITY],file_pending[LL_PATH_CAPACITY];
    float font_scale;
    DrawPoint text_origin;
    HostTexture circle_texture;
    HostTexture icons;
    int icon_large,icon_small,icon_medium;
    HostTexture knob_arcs;
    int knob_context;
    HostTexture cable_texture;
    MenuNavigation menu_keys;
    float automation_shown[AUTOMATIONS];
    uint8_t automation_visible[AUTOMATIONS];
    int rename_track,rename_mixer,rename_channel;
    SamplerView sampler_views[CHANNELS];
    Waveform audio_waves[CHANNELS];
    struct { Sample sample; Sampler settings; unsigned generation,epoch,revision; int valid; } audio_wave_states[CHANNELS];
    float channel_flash[CHANNELS];
    uint8_t channel_active_ui[CHANNELS];
    int channel_decay_active;
    struct {
        HostTarget image;
        Note notes[CHANNELS][NOTES];
        float steps; int channels,valid; DrawColor color;
    } previews[PATTERNS];
    float track_flash[LANES];
    uint8_t track_active_ui[LANES];
    float meter_level[INSERTS+1][2],meter_hold[INSERTS+1][2];
    double meter_until[INSERTS+1][2],meter_time;
    uint8_t record_armed[INSERTS+1];
    int mixer_decay_active,effect_slot,effect_list_scroll;
    int eq_bus,eq_slot,eq_band;
    DynamicsBand dynamics_copy;int dynamics_has_copy;
    int dynamics_band[32],dynamics_view[32],dynamics_pause[32],dynamics_drag,dynamics_drag_window,dynamics_drag_band,dynamics_drag_kind;
    float dynamics_drag_x,dynamics_drag_y,dynamics_speed[32];
    Spectrum master_spectrum,eq_spectrum;
    int spectrum_active;
    const unsigned fm_envelope_ids[3][4];
} UiState;
extern UiState ui;
static inline Rect desktop_editor_rect(int id) { return windows_content_frame(&ui.windows,id); }
static inline void pattern_select_channel(int c) {
    if(c<0 || c>=ui.project.channel_count)return;
    ui.channel=ui.piano_channel=c;ui.browser_focus=0;
    if(ui.windows.editors[4].visible)ui.instrument_channel=c;
}
static inline int pattern_channel_at(float x,float y) {
    Rect r=desktop_editor_rect(2);
    int visible=(int)fmaxf(1,(r.h-198)/28);
    if(!ui.windows.editors[2].visible || x<r.x+4 || x>=r.x+162 || y<r.y+80 || y>=r.y+80+visible*28)return -1;
    int c=ui.pattern_channel_scroll+(int)((y-r.y-80)/28);
    return c>=0 && c<ui.project.channel_count?c:-1;
}

/* The carried Piano controller keeps its original local grid geometry.
   Caption and channel-sidebar offsets translate that content as a unit. */
static inline Rect desktop_piano_rect(void) {
    Rect r=desktop_editor_rect(2);
    if(ui.studio_style){r.x+=166;r.w-=166;if(!ui.windows.workspace.enabled){r.y-=34;r.h+=34;}}
    return r;
}
void piano_tool_panel(float width,float height);
void pattern_hit_update(float step);
static inline int playlist_grid_x(void){return ui.studio_style?148:212;}
static inline int playlist_grid_top(void){return ui.studio_style?74:PLAYLIST_GRID_TOP;}
static inline int playlist_header_x(void){return ui.studio_style?4:120;}
static inline float picker_width(void){return ui.studio_style?ui.picker_bounds.w-8:112;}
static inline int picker_bottom(void){return ui.studio_style?28:66;}
static inline int desktop_picker_hit(float x,float y) {
    Rect r=ui.picker_bounds;
    return ui.studio_style && !ui.browser_hidden && x>=r.x && x<r.x+r.w && y>=r.y+57 && y<r.y+r.h;
}
static inline int desktop_window_hit(float x,float y){return desktop_picker_hit(x,y)?1:windows_hit(&ui.windows,x,y);}
void playlist_picker(Rect rect,float height);
static inline void desktop_playback_page(int song){
    song=!!song;
    if(ui.song!=song){ui.song=song;ui.reset=1;}
    if(ui.windows.workspace.enabled)windows_focus(&ui.windows,song?1:2);
}
static inline int desktop_mixer_fx_rows(void) {
    int rows=0;for(int bus=0;bus<=ui.project.insert_count;bus++) {
        int count=effect_chain_list(&ui.project,bus).count;if(count>rows)rows=count;
    }
    return rows>3?3:rows;
}
static inline MixerView desktop_mixer_view(float width,float height) {
    int rows=desktop_mixer_fx_rows();
    if(ui.windows.workspace.enabled && ui.windows.workspace.mixer_fx_rows>=0 && rows>ui.windows.workspace.mixer_fx_rows)rows=ui.windows.workspace.mixer_fx_rows;
    return mixer_view_rows(width,height,ui.project.insert_count,ui.mixer_offset,ui.mixer_fx_hidden,rows);
}
static inline int mixer_strip_width(void){
    if(!ui.studio_style)return 51;
    return MIXER_STRIDE;
}
static inline float mixer_route_y(float height){return height-(ui.studio_style?37:46);}

int shortcut_down(void);
int command_down(void);
int delete_pressed(void);
void resource_paths(void);
int mouse_pressed(int button);
void request_cursor(int cursor);
void flush_cursor(void);
int recording_active(void);
int recording_source(int source);
int channel_missing(int c);
const char *channel_caption(int c);
void sample_duration(Project *p,int c,Sample processed,Sample original);
Sample sample_copy(Sample source);
void *sampler_worker(void *unused);
void sampler_update(void);
int sampler_flush(void);
float track_height(int lane);
float track_position(float lane);
float track_at(float position);
float *playback_loop(void);
int desktop_rack_fill(int pattern,int channel,int interval);
float desktop_rack_step_width(void);
int desktop_rack_channels(int *rows);
int desktop_rack_channel_at(int row);
/* Default studio content: 276px pane + 34px toolbar translation - 180px padding. */
static inline float desktop_piano_reference_area(void) { return ui.studio_style?130.f:0.f; }
static inline float desktop_velocity_height(void) { return ui.studio_style?(ui.velocity_lane.collapsed?0:velocity_lane_height(ui.velocity_lane.height,desktop_piano_rect().h)):62; }
static inline Rect desktop_piano_grid_rect(void) { Rect r=desktop_piano_rect();r.h-=desktop_velocity_height()-62;return r; }

void desktop_velocity_resize(void);
float piano_visible_rows(void);
int piano_min_top(void);
int captured(void);
void sampler_queue(void);
int project_dirty(void);
float ui_scale(void);
float playback_start(void);
void transport_toggle(void);
void transport_stop(void);
void workspace_view_scale(float *arrangement_width,float *piano_width);
void workspace_dividers(void);
void workspace_load(void);
void workspace_save(void);
void effect_editor(float width,float height);
void effect_slot_menu(int bus,int slot);
int mixer_effect_window(int bus,int slot);
void mixer_effect_open(int bus,int slot,int toggle);
void mixer_effect_add(int bus,int slot,int type);
void mixer_effect_remove(int bus,int slot);
void mixer_effect_replace(int bus,int slot,int type);
void fonts_update(float scale);
int text_width(const char *text,int size);
float raster_position(float value,float origin);
void label_key(const char *text,DrawRect bounds,DrawColor color);
void label_moving(const char *text,float x,float y,int size,DrawColor color);
void lcd_time(const char *text,float x,float y,float width,DrawColor color);
void icon_in_button(int id,DrawRect button,DrawColor color);
void lcd_number(const char *text,float x,float y,float width,float height,DrawColor color);
void label(const char *text,float x,float y,int size,DrawColor color);
void backspace(char *text);
const char *fit_text(const char *text,int width,int size);
void circles_init(void);
void circle(float x,float y,float radius,DrawColor color);
void icons_init(float scale);
void icon(int id,float x,float y,float size,DrawColor color);
void arcs_init(void);
void cables_init(void);
void cable(DrawPoint points[4],DrawColor color);
void smooth_line(DrawPoint start,DrawPoint end,float width,DrawColor color);
int hover(float x,float y,float w,float h);
int menu_key(int key);
void menu_keys_begin(int id,int opened,int default_item,int *scroll,int total);
void menu_keys_end(void);
int symbol(const char *text,int x,int y,DrawColor c);
int button_color(const char *text,int x,int y,int w,int h,int active,DrawColor base);
int button(const char *text,int x,int y,int w,int h,int active);
int choice_button(const char *text,int x,int y,int w,int h);
int drag_button(const char *text,int x,int y,int w,int h,int active);
int color_picker(int x,int y,int width,uint32_t selected);
int picker_button(int y,DrawColor color,int selected);
void automation_display_update(void);
float displayed_value(const void *pointer,float manual);
void mute_light(float x,float y,uint8_t *states,int count,int selected,const char *name);
int editor_button(int id,int x);
void open_context(int kind,int target,DrawPoint position);
int preset_directory(char *out,size_t capacity,int kind);
void open_popup(int kind);
const char *mixer_name(int id);
void begin_rename(void);
void begin_rack_fill(int channel);
void begin_number(float *value,float low,float high,float initial,const char *name);
void control_menu(float *value,float low,float high,float initial,const char *name);
void create_automation(void);
void text_input(char *text,size_t capacity);
const char *device_caption(const char *name);
void device_button(int io,int x,int y,int width);
void help_group(int x,int *y,const char *title);
void help_binding(int x,int *y,const char *keys,const char *action);
void draw_popup_content(void);
void draw_popup(void);
int import_sample(const char *path,int c);
void audition_entry(int entry);
void rack_reveal_last(void);
void add_instrument(int type);
void replace_instrument(int c,int type);
int pattern_instruments(int pat,int rows[CHANNELS]);
DrawColor source_rgb(uint32_t rgb);
DrawColor clip_foreground(DrawColor fill);
DrawColor audio_color(int c);
SamplerView *sampler_view(int c);
int sampler_live_preview(int c);
unsigned sampler_view_frames(int c);
WavePeak sampler_view_envelope(int c,double position,double width,unsigned frames);
float audio_view_steps(int c);
float playlist_clip_length(int lane,int clip);
Waveform *processed_waveform(int c);
double wave_source(const WaveDisplay *view,float x);
void wave_quad(float x,float xx,float top,float next_top,float bottom,float next_bottom,float v,float vv);
void draw_waveform(WaveDisplay view,DrawRect area,float left,float right,DrawColor color);
void audio_waveform_at(int c,float x,float y,float left,float right,float pixels_per_step,float height,int lane,int clip);
void audio_waveform(int c,float x,float y,float left,float right,float pixels_per_step,float height);
void drop_sample(const char *path);
void delete_channel(int c);
void install_project(Project *next,Sample fresh[CHANNELS],Sample processed[CHANNELS],const char *filename);
void new_project(int demo);
int load_project(const char *path);
void replace_project(int action);
void request_load_project(const char *path);
void replacement_saved(void);
void file_picker_sync(void);
void preset_action(int kind,int owner,int slot,int save);
int preset_commit(const char *chosen,int save);
int project_file_commit(const char *chosen);
void project_file_action(int action);
int file_picker_commit(void);
void file_picker_submit(void);
void draw_file_picker(int x,int y,int w,int h,float scale);
void history_stamps(uint64_t stamps[CHANNELS]);
int history_checkpoint(void);
void undo_redo(int direction);
void capture_control(float *value,float low,float high,int fader);
void desktop_control_update(float dy,float scale,float y);
void knob_style(int x,int y,float *value,float low,float high,float initial,const char *name,int style);
void knob_sized(int x,int y,float *value,float low,float high,float initial,const char *name,int style,float size);
void knob(int x,int y,float *value,float low,float high,float initial,const char *name);
void channel_route(int c,int x,int y);
void channel_controls(int c,float width);
double pattern_playback_position(void);
float edit_steps(void);
void pattern_extend(float end);
int desktop_rack_melodic(int c);
void paint_rack(float width,float x,float y);
void rack(float width,float height);
DrawColor pattern_rgb(uint32_t rgb);
DrawColor pattern_color(int id);
void pattern_pitch_range(int pat,int *low,int *high);
void previews_update(float scale);
void clip_preview(int pat,float x,float y,float left,float right,float pixels,float steps,float height);
void picker_notes(int p,float x,float y);
DrawColor source_color(int source);
const char *source_name(int source);
void automation_curve(int a,float origin,float y,float pixels,float height,float left,float right,int nodes);
void draw_picker_drag(void);
void draw_browser_drag(void);
int tool_button(int tool,int active,int x,const char *tip);
void row_zoom_update(float y);
void row_zoom_button(int id,float width,float top);
void desktop_ruler_press(int id,int button,float at,double time,DrawPoint position);
void desktop_ruler_drag(float at,int cancel,int pressed,int held);
void timeline_ruler(int id,float start,float span,float gx,float y,float width,float q);
void grid_selector(int id,int x,int y,int width,int height);
void timeline_grid_choice(int choice,float start,float span,float gx,float y,float width,float height);
void timeline_grid(float start,float span,float gx,float y,float width,float height);
void follow_view(float *start,float span,double position);
void track_activity_update(void);
int clip_edge(int lane,int clip,float bar,float barw);
int picker_double_click(int source);
void playlist(float width,float height,float scale);
void edit_selection(int action);
void piano_shift(float dx,int dy,const char *action);
void piano_octave(int direction);
int desktop_piano_inactive(float step);
void piano_extend(float end);
void desktop_piano_gesture_update(float step,float pitch,float q);
void desktop_piano_note_drag_update(float step,float row,float q);
int piano_black_key(int pitch);
DrawRect piano_key_rect(int pitch,float rh);
int piano_key_at(float x,float y,float rh);
void desktop_piano_key_update(float x,float y,int cancel);
void piano(float width,float height);
int mixer_input_at(DrawPoint point);
void desktop_mixer_scrollbar_update(void);
void desktop_fx_drag_update(void);
void mute_solo_buttons(float x,float y,uint8_t *states,int count,int selected,const char *name);
void recording_finish(void);
void recording_poll(void);
void recording_start_audio(void);
int midi_target(void);
void midi_panic(void);
void midi_typing_record_begin(double time);
void midi_typing_record_end(double time);
void midi_save_settings(void);
void midi_refresh(void);
void midi_select(int index);
void midi_initialize(void);
void midi_release_key(int slot,double time);
void midi_control(ParameterTarget target,float normalized,double time);
void midi_poll(void);
void recording_start(void);
void meters_update(void);
void spectrum_update(void);
void spectrum_draw(const Spectrum *s,DrawRect r,int colored);
float eq_x(float frequency,DrawRect r);
float eq_y(float gain,DrawRect r);
void eq_editor(float width,float height);
float meter_height(float level);
void meter_draw(int id,int x,int top,int bottom,int bar_width);
void selected_meter(float height);
void mixer(float width,float height);
void delete_pattern(int target);
void draw_context_content(void);
void draw_context(void);
void typing_piano(int blocked);
int sampler_toggle(int x,int y,int width,const char *name,int active);
void sampler_switch(int x,int y,int width,const char *name,uint8_t *flags,int bit);
void fm_graph_update_at(float x,float y);
void fm_graph_update(void);
void fm_control(FMSettings *s,unsigned id,const char *caption,int x,int y);
void fm_envelope_graph(FMSettings *s,DrawRect plot);
void fm_legacy_editor(float width,float height);
const char *fm_sound_name(const FMSettings *s);
void fm_footer(int c,float width,float height);
void fm_controls_row(FMSettings *s,const unsigned *ids,const char *const *names,int count,float width,int y);
void dx7_envelope_graph(FMSettings *s,int base,DrawRect plot);
void fm_dx_editor(float width,float height);
void fm_analog_editor(float width,float height);
void fm_editor(float width,float height);
void sampler(float width,float height);
void draw_editor(int id,float scale);
void navigate_editors(float scale);
#endif
