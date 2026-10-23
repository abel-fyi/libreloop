// SPDX-License-Identifier: GPL-3.0-only
/* Controlled hardware/writer services, real recording preparation and mapping. */
#ifdef _WIN32
#include <direct.h>
#define fixture_mkdir(path) _mkdir(path)
#else
#include <sys/stat.h>
#define fixture_mkdir(path) mkdir(path,0700)
#endif
static int mode,writer_index,checkpoint_calls,record_starts,record_ends,record_modes,updates,publications,panic_calls,popup;
static double fixture_time=10,visual_frame;
static int last_song,last_pattern;static float last_loop_start,last_loop_end;
struct RecordingWriter {char path[256];unsigned frames;};
static void put32(unsigned char *p,unsigned v){for(int i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));}
static int write_fixture(RecordingWriter *w,unsigned frames) {
    unsigned char header[44]={0};memcpy(header,"RIFF",4);put32(header+4,36+frames*8);memcpy(header+8,"WAVEfmt ",8);put32(header+16,16);header[20]=3;header[22]=2;put32(header+24,RATE);put32(header+28,RATE*8);header[32]=8;header[34]=32;memcpy(header+36,"data",4);put32(header+40,frames*8);
    FILE *f=fopen(w->path,"wb");if(!f)return 0;fwrite(header,1,44,f);for(unsigned i=0;i<frames;i++){float p[2]={i*.0001f,-i*.0002f};fwrite(p,4,2,f);}return fclose(f)==0;
}
RecordingWriter *recording_writer_open(const char *directory){(void)directory;if(mode==8)return NULL;RecordingWriter *w=calloc(1,sizeof *w);if(!w)return NULL;snprintf(w->path,sizeof w->path,"recordings/take-%d.wav",writer_index++);if(!write_fixture(w,0)){free(w);return NULL;}return w;}
const char *recording_writer_path(const RecordingWriter *w){return w->path;}
int recording_writer_start(RecordingWriter *w,RecordingRead read,void *context,int take){(void)w;(void)read;(void)context;(void)take;return mode!=10;}
unsigned recording_writer_frames(const RecordingWriter *w){return w->frames;}
int recording_writer_failed(const RecordingWriter *w){(void)w;return mode==13;}
int recording_writer_finish(RecordingWriter *w){w->frames=mode==11?0:256;if(mode==12){remove(w->path);return 1;}return write_fixture(w,w->frames);}
void recording_writer_free(RecordingWriter *w,int remove_file){if(!w)return;if(remove_file)remove(w->path);free(w);}
unsigned audio_record_read(int take,float *stereo,unsigned frames){(void)take;(void)stereo;(void)frames;return 0;}
int audio_record_start(const Project *p,const int buses[],int count,float start,float output,int song,int pattern,float loop_start,float loop_end,char error[256]){(void)p;(void)buses;(void)count;(void)start;(void)output;last_song=song;last_pattern=pattern;last_loop_start=loop_start;last_loop_end=loop_end;record_starts++;if(mode==9){strcpy(error,"fixture device unavailable");return 0;}return 1;}
void audio_record_end(void){record_ends++;}
int audio_record_failed(void){return mode==13;}
void audio_record_mode(int enabled){record_modes+=enabled?1:10;}
void audio_sample(int c,Sample s){(void)c;(void)s;publications++;}
void audio_channels(const Project *p,const Sample s[CHANNELS]){(void)p;(void)s;publications++;}
void audio_update(const Project *p,int run,int song,int pattern,int reset,float output,float start,float a,float b){(void)p;(void)run;(void)reset;(void)output;(void)start;last_song=song;last_pattern=pattern;last_loop_start=a;last_loop_end=b;updates++;}
double audio_visual_position(void){return visual_frame;}
double midi_input_time(void){return fixture_time;}
int midi_poll(void){return 0;}
void midi_panic(void){panic_calls++;}
/* Typing is disabled in the original hardware/audio recording fixtures. */
#ifndef RECORDING_FIXTURE_TYPING
void midi_typing_record_begin(double time){(void)time;}
void midi_typing_record_end(double time){(void)time;}
#endif
int midi_target(void){return 0;}
int history_checkpoint(void){checkpoint_calls++;return 1;}
int sampler_flush(void){return mode!=7;}
void open_popup(int kind){popup=kind;}
#ifndef RECORDING_FIXTURE_TRANSPORT
float playback_start(void){return ui.playlist_start*STEPS;}
float *playback_loop(void){return ui.song_loop;}
#endif
static int directory_fixture(const char *path){if(mode==6)return -1;return fixture_mkdir(path)==0 || errno==EEXIST?0:-1;}
static char *canonical_fixture(const char *p,char *out){snprintf(out,1024,"/fixture/%s",p);return out;}
