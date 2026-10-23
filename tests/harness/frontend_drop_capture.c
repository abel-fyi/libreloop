// SPDX-License-Identifier: GPL-3.0-only
#include "../../src/ui/frontend/ui_internal.h"
#include "files.h"
UiState ui;
#define POINT DrawPoint
static void current_drop_setup(int mode) {
    ui.picker_bounds=(Rect){4,689,230,373};
    if(mode<=5 || mode==11) {
        Rect old=ui.windows.editors[1].rect,now={244,98,858,468};
        ui.mouse.x=now.x+148+(ui.mouse.x-old.x-212)*(now.w-172)/(old.w-236);
        ui.mouse.y=now.y+74+ui.mouse.y-old.y-96;
        if(mode==1)ui.mouse=(POINT){12,800};
        if(mode==2)ui.mouse.y=now.y+TITLE-1;
        ui.windows.editors[1].rect=now;
    }
}
#define DROP_SETUP_EXTRA current_drop_setup
#include "drop_sequence.h"
void recording_finish(void){finish_calls++;}
int sample_load(const char *path,Sample *s){return sample_load_fixture(path,s);}
char *host_realpath(const char *path,char *out){snprintf(out,LL_PATH_CAPACITY,"/fixture/%s",path);return out;}
const char *host_get_file_name(const char *p){return platform_basename(p);}
const char *host_get_file_name_without_ext(const char *p){static char name[256];snprintf(name,sizeof name,"%s",platform_basename(p));char *dot=strrchr(name,'.');if(dot)*dot=0;return name;}
void audio_sample(int c,Sample sample){(void)c;(void)sample;sample_commands++;}
void audio_channels(const Project *p,const Sample samples[CHANNELS]){(void)p;(void)samples;channel_commands++;}
int host_get_screen_height(void){return 675;}
float ui_scale(void){return 1;}
#include "../../src/ui/frontend/app_project.c"
#include "../../src/ui/frontend/ui_rack.c"
#include "../../src/ui/frontend/ui_playlist.c"
int main(int argc,char **argv){if(argc!=2)return 1;int mode=atoi(argv[1]);if(mode<0 || mode>11)return 1;return run(mode);}
