// SPDX-License-Identifier: GPL-3.0-only
#ifndef ENGINE_H
#define ENGINE_H
#include <stdint.h>
#define SAMPLE_EMPTY "@empty" /* stored in paths for an unloaded sampler */
#define CHANNELS 32
#define PATTERNS 8
#define PATTERN_NAME 48
#define STEPS 16
#define NOTES 128
#define LANES 100
#define BARS 16 /* initial Playlist view, not a timeline limit */
#define CLIPS 64
#define RATE 48000
#define INSERTS 100
enum { SAMPLE_NORMALIZE=1, SAMPLE_REVERSE=2, SAMPLE_POLARITY=4 };
typedef struct { float pitch,time,start,length; uint8_t flags,stretch; } Sampler;
typedef struct { uint8_t pitch, velocity; float start, length; } Note; /* length 0 = drum one-shot */
typedef struct {
    float bpm;
    int pattern_count,channel_count;
    float volume[CHANNELS], pan[CHANNELS], master;
    uint8_t mute[CHANNELS];
    Note notes[PATTERNS][CHANNELS][NOTES];
    uint8_t clips[LANES][CLIPS]; /* zero = empty; otherwise pattern + 1 */
    float clip_steps[LANES][CLIPS],clip_starts[LANES][CLIPS];
    float pattern_steps[PATTERNS];
    char paths[CHANNELS][1024],channel_names[CHANNELS][PATTERN_NAME];
    char pattern_names[PATTERNS][PATTERN_NAME];
    int insert_count;
    uint8_t route[CHANNELS],insert_mute[INSERTS]; /* route: 0 Master, 1..count insert; insert_mute bits: 1 mute, 2 solo */
    float insert_volume[INSERTS],insert_pan[INSERTS];
    uint8_t insert_output[INSERTS]; /* 0 = Master, 1..count = insert, 255 = disconnected */
    Sampler sampler[CHANNELS];
    float master_pitch; /* semitones; sample speed, independent of tempo */
} Project;
typedef struct { float *data; unsigned frames; } Sample;
typedef struct { int channel; double position, speed, remaining; float gain; } Voice;
typedef struct {
    uint64_t frame;
    int64_t last_step;
    int song, pattern;
    float start_step;
    Voice voices[128];
} Player;
enum { SNAP_AUTO, SNAP_BAR, SNAP_BEAT, SNAP_HALF_BEAT, SNAP_THIRD_BEAT, SNAP_STEP, SNAP_SIXTH_BEAT, SNAP_HALF_STEP, SNAP_THIRD_STEP, SNAP_QUARTER_STEP, SNAP_COUNT };
extern const char *snap_names[SNAP_COUNT];
float snap_interval(int mode,float pixels_per_step);
void timeline_zoom(float *span,float *start,float wheel,float anchor,float unit);
float timeline_thumb(float width,float span,float range);
void project_default(Project *p);
int insert_reset(Project *p,int id);
int channel_delete(Project *p,int channel);
int insert_connect(Project *p,int source,int destination);
Note *note_at(Project *p,int pattern,int channel,float start,int pitch);
Note *note_add(Project *p,int pattern,int channel,float start,int pitch,float length);
int note_move(Project *p,int pattern,int channel,Note *note,float start,int pitch,float limit);
int notes_move(Project *p,int pattern,int channel,const Note before[NOTES],const uint8_t selected[NOTES],float dx,int dy,float limit);
float clip_length(const Project *p,int lane,int bar);
float song_steps(const Project *p);
void samples_default(Sample s[CHANNELS]);
int sampler_valid(Sampler settings);
int sampler_equal(Sampler a,Sampler b);
int sample_process(Sample source,Sampler settings,Sample *result);
void player_reset(Player *p);
void player_seek(Player *p,const Project *project,float step);
void render(Player *p, const Project *project, const Sample s[CHANNELS], float *out, unsigned frames);
void render_live(Player *p,const Project *project,const Sample s[CHANNELS],float *out,unsigned frames);
int project_save(const char *path, const Project *p);
int project_load(const char *path, Project *p);
int export_wav(const char *path, const Project *p, const Sample s[CHANNELS]);
#endif
