// SPDX-License-Identifier: GPL-3.0-only
#ifndef ENGINE_H
#define ENGINE_H
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#define SAMPLE_MAX_FRAMES (INT_MAX/8) /* Leave room for 4x time and 2x pitch processing. */
#define SAMPLE_EMPTY "@empty" /* stored in paths for an unloaded sampler */
#define CHANNELS 32
#define PATTERNS 8
#define PATTERN_NAME 48
#define COLOR_HUES 12
#define COLOR_COUNT (COLOR_HUES*2)
extern const uint32_t pattern_palette[COLOR_COUNT];
#define STEPS 16
#define NOTES 128
#define LANES 100
#define BARS 16 /* initial Playlist view, not a timeline limit */
#define CLIPS 64
#define AUTOMATIONS 32
#define AUTOMATION_POINTS 64
#define AUTOMATION_SOURCE (PATTERNS+CHANNELS)
#define SOURCES (AUTOMATION_SOURCE+AUTOMATIONS)
#define AUDIO_SOURCE(s) ((s)>=PATTERNS && (s)<AUTOMATION_SOURCE)
/* Saved IDs, independent of UI addresses. Plugin IDs are reserved for future processors. */
enum { PARAM_CHANNEL_VOLUME=1, PARAM_CHANNEL_PAN, PARAM_CHANNEL_PITCH, PARAM_INSERT_VOLUME, PARAM_INSERT_PAN, PARAM_INSERT_WIDTH, PARAM_MASTER_VOLUME, PARAM_MASTER_WIDTH, PARAM_MASTER_PITCH, PARAM_CHANNEL_MUTE, PARAM_INSERT_MUTE, PARAM_MASTER_MUTE, PARAM_SWING, PARAM_PITCH_RANGE, PARAM_PLUGIN=1024 };
typedef struct { unsigned parameter,owner,slot; } ParameterTarget;
typedef struct { float step,value; } AutomationPoint;
typedef struct { ParameterTarget target; float steps; int count; uint32_t color; char name[48]; AutomationPoint points[AUTOMATION_POINTS]; } Automation;
#define RATE 48000
#define INSERTS 100
#define VOLUME_KNOB_MAX 1.25f
#define MIXER_GAIN_MAX 2.f /* +6.02 dB; unity is 1 */
float gain_db(float gain);
float fader_position(float gain);
float fader_gain(float position);
enum { SAMPLE_NORMALIZE=1, SAMPLE_REVERSE=2, SAMPLE_POLARITY=4 };
typedef struct { float pitch,time,start,length,trim; uint8_t flags,stretch; float fit_bpm; /* reference BPM; zero disables tempo fitting */ } Sampler;
typedef struct { uint8_t pitch, velocity; float start, length; } Note; /* length 0 = drum one-shot */
typedef struct {
    float bpm;
    int pattern_count,channel_count;
    float volume[CHANNELS], pan[CHANNELS], master;
    float channel_pitch[CHANNELS],pitch_range[CHANNELS]; /* normalized -1..1, range 1..48 semitones */
    uint8_t mute[CHANNELS]; /* bits: 1 mute, 2 solo */
    Note notes[PATTERNS][CHANNELS][NOTES];
    uint8_t clips[LANES][CLIPS]; /* 0 empty; pattern + 1, PATTERNS + Audio + 1, AUTOMATION_SOURCE + automation + 1 */
    float clip_steps[LANES][CLIPS],clip_starts[LANES][CLIPS]; /* Audio lengths in seconds; patterns/automation in steps */
    float clip_offsets[LANES][CLIPS]; /* source offset: seconds for Audio, steps for patterns/automation */
    uint8_t channel_audio[CHANNELS];
    float audio_seconds[CHANNELS]; /* processed sample duration */
    float pattern_steps[PATTERNS];
    char paths[CHANNELS][1024],channel_names[CHANNELS][PATTERN_NAME];
    char pattern_names[PATTERNS][PATTERN_NAME];
    uint32_t pattern_colors[PATTERNS]; /* RGB */
    uint32_t channel_colors[CHANNELS]; /* RGB; 0 uses automatic duration color */
    int insert_count;
    uint8_t route[CHANNELS],insert_mute[INSERTS]; /* route: 0 Master, 1..count insert; insert_mute bits: 1 mute, 2 solo */
    float insert_volume[INSERTS],insert_pan[INSERTS];
    uint8_t insert_output[INSERTS]; /* 0 = Master, 1..count = insert, 255 = disconnected */
    Sampler sampler[CHANNELS];
    float master_pitch; /* semitones; sample speed, independent of tempo */
    float insert_width[INSERTS],master_width; /* 0 mono, 1 unchanged, 2 wider */
    uint8_t master_mute,lane_mute[LANES]; /* lane bits: 1 mute, 2 solo */
    char audio_io[INSERTS+1][2][128]; /* input/output device choices; routing/recording reserved */
    float effect_mix[INSERTS+1][10]; /* wet/dry; reserved until effects exist */
    uint8_t effect_bypass[INSERTS+1][10];
    char track_names[LANES][PATTERN_NAME];
    char insert_names[INSERTS][PATTERN_NAME];
    int automation_count;
    Automation automations[AUTOMATIONS];
    float swing; /* 0..1; delays offbeat sixteenths up to half a step */
} Project;
/* Interleaved PCM; channels 0 retains compatibility with mono initializers. */
typedef struct { float *data; unsigned frames,channels; } Sample;
static inline unsigned sample_channels(Sample s) { return s.channels?s.channels:1; }
static inline float sample_at(Sample s,unsigned frame,unsigned side) {
    unsigned channels=sample_channels(s);
    return frame<s.frames?s.data[(size_t)frame*channels+(channels==1?0:side)]:0;
}
typedef struct { int channel; double position, speed, remaining; float gain; int lane,audio_clip; double tempo_rate,grains[2]; unsigned grain_phase; int grain_ready; } Voice;
typedef struct {
    uint64_t frame;
    float clock_bpm; /* preserve musical position when BPM changes */
    int64_t last_step;
    int song, pattern;
    int audio_resync; /* reconcile edited Song Audio clips at the current frame */
    float start_step,loop_start,loop_end; /* loop_end 0 disables the playback region */
    Voice voices[128];
    uint8_t channel_active[CHANNELS],channel_trigger[CHANNELS];
    uint8_t lane_active[LANES],lane_trigger[LANES]; /* activity from the latest render block */
} Player;
/* Editing always follows the visible grid (ruler spacing at distant zooms). */
float grid_interval(float pixels_per_step);
float snap_floor(float value,float interval);
float snap_round(float value,float interval);
void timeline_zoom(float *span,float *start,float wheel,float anchor,float unit);
float timeline_thumb(float width,float span,float range);
/* Visual density is independent of the editing snap interval. Zero lines means bands only. */
typedef struct { float lines,labels,band_alpha; } TimelineGrid;
TimelineGrid timeline_grid_layout(float pixels_per_step);
int parameter_info(const Project *p,ParameterTarget target,float *value,float *low,float *high);
int parameter_from_pointer(const Project *p,const void *pointer,ParameterTarget *target);
float automation_value(const Automation *a,float step);
/* Song-position lookup for future built-in processors; 0 means no automation has started. */
int automation_evaluate(const Project *p,ParameterTarget target,float step,float *normalized);
int automation_point(Automation *a,float step,float value);
int automation_move_point(Project *p,int index,int point,float step,float value);
int automation_create(Project *p,ParameterTarget target,const char *name,float steps);
int automation_delete(Project *p,int index);
int automation_valid(const Project *p);
int sampler_processing_equal(Sampler a,Sampler b);
float audio_source_bpm(const Project *p,int channel);
void voice_tempo_sample(Voice *voice,Sample sample,double pitch,double rate,int stretch,float stereo[2]);
void project_default(Project *p);
void project_new(Project *p);
void project_demo(Project *p);
int insert_reset(Project *p,int id);
int pattern_delete(Project *p,int pattern);
int channel_delete(Project *p,int channel);
int insert_connect(Project *p,int source,int destination);
int solo_any(const uint8_t *states,int count);
void solo_toggle(uint8_t *states,int count,int selected);
Note *note_at(Project *p,int pattern,int channel,float start,int pitch);
Note *note_add(Project *p,int pattern,int channel,float start,int pitch,float length);
int note_move(Project *p,int pattern,int channel,Note *note,float start,int pitch,float limit);
int notes_move(Project *p,int pattern,int channel,const Note before[NOTES],const uint8_t selected[NOTES],float dx,int dy,float limit);
float channel_speed(const Project *p,int channel);
float clip_source_steps(const Project *p,int source);
float clip_offset_steps(const Project *p,int lane,int clip);
float clip_length(const Project *p,int lane,int bar);
float song_steps(const Project *p);
typedef struct { const Project *project; int channel; unsigned count; uint16_t clips[LANES*CLIPS]; } AudioTimeline;
void audio_timeline_init(AudioTimeline *map,const Project *p,int channel);
double audio_timeline_source(const AudioTimeline *map,double start,double end);
double audio_timeline_duration(const AudioTimeline *map,double start,double source_steps);
float audio_clip_steps(const Project *p,int lane,int clip);
double audio_clip_position(const Project *p,int lane,int clip,double step);
void samples_default(Sample s[CHANNELS]);
int sampler_valid(Sampler settings);
int sampler_equal(Sampler a,Sampler b);
/* Non-owning view of the source with quiet leading/trailing frames removed. */
Sample sample_trim(Sample source,float threshold);
int sample_process(Sample source,Sampler settings,Sample *result);
void player_reset(Player *p);
void player_seek(Player *p,const Project *project,float step);
void render(Player *p, const Project *project, const Sample s[CHANNELS], float *out, unsigned frames);
void render_live(Player *p,const Project *project,const Sample s[CHANNELS],float *out,unsigned frames);
/* Mix sequenced/live voices through the same stereo buses; optional post-fader peaks. */
void render_mixer(Player *p,Player *live,const Project *project,const Sample s[CHANNELS],float *out,unsigned frames,int sequence,float peaks[INSERTS+1][2]);
/* Optional realtime input and post-fader taps; callbacks allocate nothing. */
typedef struct {
    uint8_t active[INSERTS+1];
    void (*input)(void *context,float buses[INSERTS+1][2]);
    void (*output)(void *context,int bus,float left,float right);
    void *context;
} MixerIO;
void render_mixer_io(Player *p,Player *live,const Project *project,const Sample s[CHANNELS],float *out,unsigned frames,int sequence,float peaks[INSERTS+1][2],const MixerIO *io);
int project_save(const char *path, const Project *p);
int project_load(const char *path, Project *p);
int export_wav(const char *path, const Project *p, const Sample s[CHANNELS]);
#endif
