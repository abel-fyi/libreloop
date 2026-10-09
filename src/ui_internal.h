// SPDX-License-Identifier: GPL-3.0-only
#ifndef UI_INTERNAL_H
#define UI_INTERNAL_H
/* Private desktop interface. UiState owns document, interaction, caches and
   services; DSP and project libraries do not depend on this header. The UI is
   single-threaded. Its sampler worker uses only the dedicated sampler_job. */
#include "raylib.h"
#include "rlgl.h"
#include "audio.h"
#include "windows.h"
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
#include "atomic_file.h"
#include "preset.h"
#include "spectrum.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <unistd.h>

enum { RECORD_AUDIO=1,RECORD_NOTES=2,RECORD_AUTOMATION=4 };
typedef struct {int used,pressed,mchannel,pitch,target,velocity;} MidiHeld;
enum { PLAYLIST_GRID_TOP=96, PLAYLIST_BOTTOM=18, PIANO_GRID_TOP=86, PIANO_GRID_PADDING=180, EDITOR_SCROLLBAR=16, EDITOR_CORNER=16 };
enum { PIANO_IDLE, PIANO_BRUSH, PIANO_ERASE, PIANO_BOX };
enum { ICON_RACK,ICON_PLAYLIST,ICON_PIANO,ICON_MIXER,ICON_PENCIL,ICON_BRUSH,ICON_SELECT,ICON_METRO,ICON_PLAY,ICON_STOP,ICON_PAUSE,ICON_CLOSE,ICON_BACK,ICON_FOLLOW,ICON_KEYS,ICON_WAVE,ICON_AUTOMATION,ICON_CUT,ICON_STRETCH,ICON_ROW_ZOOM,ICON_COUNT };
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
enum { KNOB_NORMAL,KNOB_SWING,KNOB_PAN,KNOB_WIDTH,KNOB_CENTER,KNOB_VOLUME,KNOB_LOGARITHMIC,KNOB_FIXED_COARSE };

#define IsMouseButtonPressed mouse_pressed
#define SetMouseCursor request_cursor
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
    char samples_path[PATH_MAX],font_path[PATH_MAX],logo_path[PATH_MAX];
    GLFWmousebuttonfun raylib_mouse_callback;
    unsigned pending_clicks[8],frame_clicks[8];
    int requested_cursor,current_cursor;
    GLFWcursor *resize_cursor,*track_cursor,*hand_cursor;
    GLFWwindow *app_window;
    Project project;
    History edit_history;
    Sample samples[CHANNELS],originals[CHANNELS];
    RecordingSession recording_ui;
    MidiTake midi_take;
    unsigned record_mask;
    int midi_record_target;
    MidiDevice midi_devices[64];
    int midi_device_count,midi_connected,midi_scroll,midi_learning;
    char midi_selected[128],midi_selected_name[128],midi_settings_path[PATH_MAX];
    double midi_refresh_time;
    ParameterTarget midi_learn_target;
    MidiHeld midi_keys[32];
    unsigned char midi_sustain[16];
    Sampler sampler_applied[CHANNELS];
    unsigned sample_generation[CHANNELS],sampler_generation[CHANNELS],sample_epoch;
    struct {
        pthread_t thread; atomic_int done; int busy,channel,ok;
        unsigned generation,epoch; Sampler settings; Sample input,result;
    } sampler_job;
    Vector2 mouse;
    int pattern,channel,piano_channel,instrument_channel,playing,song,reset;
    int typing_keys;
    float output_volume;
    int metronome;
    int follow_playhead;
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
    Vector2 picker_origin,picker_offset;
    Arrangement arrangement;
    float rack_view[PATTERNS],rack_range[PATTERNS];
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
    Vector2 ruler_last_position;
    float song_loop[2],pattern_loop[PATTERNS][2],ruler_anchor;
    unsigned char piano_channels[PATTERNS][CHANNELS];
    int rack_vdrag;
    float rack_vy,rack_vstart,rack_vscale;
    int rack_filter;
    const char *rack_filters[3];
    int rack_scroll,context_kind,context_target,context_opened,instrument_replace_scroll;
    Vector2 context_position;
    int pattern_popup,popup_opened,rename_pattern,rename_select_all,popup_drag;
    Vector2 popup_position[14],popup_offset;
    char rename_text[PATTERN_NAME],folder_text[PATH_MAX],number_text[64];
    float *number_target,number_low,number_high,number_default;
    const char *number_name;
    char device_names[64][128];
    int device_count,device_target,device_io,device_scroll;
    Browser browser;
    int route_drag,route_start,mixer_selected,mixer_scroll,mixer_pan,cable_drag;
    float route_y;
    Sample audition;
    double browser_progress;
    char audition_path[PATH_MAX];
    Waveform audition_wave;
    int browser_focus,sample_moved,browser_hidden,browser_resize;
    float browser_width,last_sidebar;
    char sample_drag[PATH_MAX];
    Vector2 sample_origin,sample_offset;
    float sample_drag_width,sample_text_offset;
    float *control_drag,control_low,control_high,control_bottom,control_range;
    int control_fader,control_reverse,control_integer,control_logarithmic,number_integer;
    float control_raw;
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
    Vector2 piano_from,piano_now;
    Vector2 note_grab;
    float piano_note_length;
    EditClipboard edit_clipboard;
    float piano_span[PATTERNS],piano_pan[PATTERNS],piano_range[PATTERNS];
    int velocity_drag,velocity_pattern,velocity_channel;
    Vector2 velocity_from,velocity_now;
    Note velocity_before[NOTES];
    int rack_paint,rack_paint_channel;
    float rack_paint_step;
    int navigation_drag,navigation_active;
    Vector2 navigation_last;
    NavigationInput navigation_input;
    float piano_scroll_remainder;
    int eq_drag;
    int fm_dx_operator,fm_dx_page,fm_analog_tab,fm_dx_drag_base;
    int fm_tab,fm_motion_target,fm_graph_drag,fm_graph_channel;
    Rectangle fm_graph_area;
    char status[256];
    ProjectDocument document;
    int help_scroll,relink_channel;
    FileChooser file_picker;
    int preset_kind,preset_owner,preset_slot;
    int file_action,file_field,file_confirm,file_scroll_drag,file_last_row;
    double file_last_click;
    char file_directory[PATH_MAX],file_pending[PATH_MAX];
    float font_scale;
    Vector2 text_origin;
    Texture2D circle_texture;
    Texture2D icons;
    int icon_large,icon_small,icon_medium;
    Texture2D knob_arcs;
    int knob_context;
    Texture2D cable_texture;
    struct {
        int id, selected, count, items, active, enter;
        Vector2 pointer;
        Rectangle bounds[128];
    } menu_keys;
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
        RenderTexture2D image;
        Note notes[CHANNELS][NOTES];
        float steps; int channels,valid; Color color;
    } previews[PATTERNS];
    float track_flash[LANES];
    uint8_t track_active_ui[LANES];
    float meter_level[INSERTS+1][2],meter_hold[INSERTS+1][2];
    double meter_until[INSERTS+1][2],meter_time;
    uint8_t record_armed[INSERTS+1];
    int mixer_decay_active,effect_slot;
    int eq_bus,eq_slot,eq_band;
    Spectrum master_spectrum,eq_spectrum;
    int spectrum_active;
    const unsigned fm_envelope_ids[3][4];
} UiState;
extern UiState ui;

int shortcut_down(void);
int command_down(void);
int delete_pressed(void);
void resource_paths(void);
void mouse_callback(GLFWwindow *window,int button,int action,int mods);
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
int rack_channels(int *rows);
int rack_channel_at(int row);
float piano_visible_rows(void);
int piano_min_top(void);
int captured(void);
void sampler_queue(void);
int project_dirty(void);
float ui_scale(void);
float playback_start(void);
void transport_toggle(void);
void transport_stop(void);
void fonts_update(float scale);
int text_width(const char *text,int size);
float raster_position(float value,float origin);
void label_moving(const char *text,float x,float y,int size,Color color);
void label(const char *text,float x,float y,int size,Color color);
void backspace(char *text);
const char *fit_text(const char *text,int width,int size);
void circles_init(void);
void circle(float x,float y,float radius,Color color);
void icons_init(float scale);
void icon(int id,float x,float y,float size,Color color);
void arcs_init(void);
void cables_init(void);
void cable(Vector2 points[4],Color color);
void smooth_line(Vector2 start,Vector2 end,float width,Color color);
int hover(float x,float y,float w,float h);
int menu_key(int key);
void menu_keys_begin(int id,int opened,int default_item,int *scroll,int total);
void menu_keys_end(void);
int symbol(const char *text,int x,int y,Color c);
int button_color(const char *text,int x,int y,int w,int h,int active,Color base);
int button(const char *text,int x,int y,int w,int h,int active);
int drag_button(const char *text,int x,int y,int w,int h,int active);
int color_picker(int x,int y,int width,uint32_t selected);
int picker_button(int y,Color color,int selected);
void automation_display_update(void);
float displayed_value(const void *pointer,float manual);
void mute_light(float x,float y,uint8_t *states,int count,int selected,const char *name);
int editor_button(int id,int x);
void open_context(int kind,int target,Vector2 position);
int preset_directory(char *out,size_t capacity,int kind);
void open_popup(int kind);
const char *mixer_name(int id);
void begin_rename(void);
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
Color source_rgb(uint32_t rgb);
Color clip_foreground(Color fill);
Color audio_color(int c);
SamplerView *sampler_view(int c);
int sampler_live_preview(int c);
unsigned sampler_view_frames(int c);
WavePeak sampler_view_envelope(int c,double position,double width,unsigned frames);
float audio_view_steps(int c);
float playlist_clip_length(int lane,int clip);
Waveform *processed_waveform(int c);
double wave_source(const WaveDisplay *view,float x);
void wave_quad(float x,float xx,float top,float next_top,float bottom,float next_bottom,float v,float vv);
void draw_waveform(WaveDisplay view,Rectangle area,float left,float right,Color color);
void audio_waveform_at(int c,float x,float y,float left,float right,float pixels_per_step,float height,int lane,int clip);
void audio_waveform(int c,float x,float y,float left,float right,float pixels_per_step,float height);
void drop_sample(const char *path);
void delete_channel(int c);
void install_project(Project next,Sample fresh[CHANNELS],Sample processed[CHANNELS],const char *filename);
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
void knob_style(int x,int y,float *value,float low,float high,float initial,const char *name,int style);
void knob(int x,int y,float *value,float low,float high,float initial,const char *name);
void channel_route(int c,int x,int y);
void channel_controls(int c,float width);
double pattern_playback_position(void);
float edit_steps(void);
void pattern_extend(float end);
int rack_melodic(int c);
void paint_rack(float width,float x,float y);
void rack(float width,float height);
Color pattern_rgb(uint32_t rgb);
Color pattern_color(int id);
void pattern_pitch_range(int pat,int *low,int *high);
void previews_update(float scale);
void clip_preview(int pat,float x,float y,float left,float right,float pixels,float steps,float height);
void picker_notes(int p,float x,float y);
Color source_color(int source);
const char *source_name(int source);
void automation_curve(int a,float origin,float y,float pixels,float height,float left,float right,int nodes);
void draw_picker_drag(void);
void draw_browser_drag(void);
int tool_button(int tool,int active,int x,const char *tip);
void row_zoom_update(float y);
void row_zoom_button(int id,float width,float top);
void ruler_press(int id,int button,float at,double time,Vector2 position);
void timeline_ruler(int id,float start,float span,float gx,float y,float width,float q);
void timeline_grid(float start,float span,float gx,float y,float width,float height);
void follow_view(float *start,float span,double position);
void track_activity_update(void);
int clip_edge(int lane,int clip,float bar,float barw);
int picker_double_click(int source);
void playlist(float width,float height,float scale);
void edit_selection(int action);
void piano_shift(float dx,int dy,const char *action);
void piano_octave(int direction);
int piano_inactive(float step);
void piano_extend(float end);
void piano_gesture_update(float step,float pitch,float q);
void piano_note_drag_update(float step,float row,float q);
int piano_black_key(int pitch);
Rectangle piano_key_rect(int pitch,float rh);
int piano_key_at(float x,float y,float rh);
void piano_key_update(float x,float y,int cancel);
void piano(float width,float height);
int mixer_input_at(Vector2 point);
void recording_finish(void);
void recording_poll(void);
void recording_start_audio(void);
int midi_target(void);
void midi_panic(void);
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
void spectrum_draw(const Spectrum *s,Rectangle r,int colored);
float eq_x(float frequency,Rectangle r);
float eq_y(float gain,Rectangle r);
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
void fm_envelope_graph(FMSettings *s,Rectangle plot);
void fm_legacy_editor(float width,float height);
const char *fm_sound_name(const FMSettings *s);
void fm_footer(int c,float width,float height);
void fm_controls_row(FMSettings *s,const unsigned *ids,const char *const *names,int count,float width,int y);
void dx7_envelope_graph(FMSettings *s,int base,Rectangle plot);
void fm_dx_editor(float width,float height);
void fm_analog_editor(float width,float height);
void fm_editor(float width,float height);
void sampler(float width,float height);
void draw_editor(int id,float scale);
void navigate_editors(float scale);
#endif
