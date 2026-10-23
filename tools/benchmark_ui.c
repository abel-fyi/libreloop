// SPDX-License-Identifier: GPL-3.0-only
/* Component render timings, not native GUI/input/GPU or audio-device timings. */
#include "editor_draw.h"
#include "piano_draw.h"
#include "waveform_draw.h"
#include "theme.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
static Project project;
static double seconds(clockid_t id){struct timespec t;clock_gettime(id,&t);return t.tv_sec+t.tv_nsec/1e9;}
static int order(const void *a,const void *b){double x=*(const double*)a,y=*(const double*)b;return (x>y)-(x<y);}
int main(int argc,char **argv) {
    if(argc!=4 && argc!=5)return 1;int blit=argc==5 && !strcmp(argv[4],"--blit");float density=(float)atof(argv[2]);int frames=atoi(argv[3]);
    if((density!=1 && density!=2) || frames<10 || frames>10000)return 1;
    if(!SDL_Init(0))return 2;
    SDL_Surface *surface=SDL_CreateSurface((int)(1200*density),(int)(675*density),SDL_PIXELFORMAT_RGBA32);
    SDL_Renderer *native=surface?SDL_CreateSoftwareRenderer(surface):NULL;if(!native)return 2;
    DrawContext *draw=draw_create(native);UIRender *render=ui_render_create(draw,argv[1]);if(!render)return 2;
    theme_init();theme_studio();project_new(&project);project.pattern_steps[0]=128;
    for(int i=0;i<NOTES;i++)project.notes[0][0][i]=(Note){48+i%24,40+i%88,(float)i,.5f+(i%4)*.5f};
    Windows windows;windows_init(&windows,1200,675);windows_focus(&windows,2);Rect rect=windows.editors[2].rect;
    Note before[NOTES]={0},velocity_before[NOTES]={0};uint8_t selected[NOTES]={0},selection_before[NOTES]={0};
    PianoViewport view={rect.w,rect.h,16,0,1,72};PianoGesture gesture={.before=before,.selected=selected,.selection_before=selection_before};
    PianoKeys keys={.pitch=-1};PianoVelocity velocity={.ramp=-1,.before=velocity_before};PianoScrollbars bars={0};
    PianoDrawing piano={.project=&project,.view=&view,.gesture=&gesture,.keys=&keys,.velocity=&velocity,.bars=&bars,.range=128,.hovered_key=-1,.hovered_tool=-1};
    WindowGeometry geometry={.width=1200,.height=675,.pixel_width=(int)(1200*density),.pixel_height=(int)(675*density),.layout_scale=1,.raster_scale=density,.dpi_x=density,.dpi_y=density,.dpi_scissor=1};
    EditorDrawing editor={&windows,2,-1,0};
    Sample sample={.data=malloc(131072*2*sizeof(float)),.frames=131072,.channels=2};if(!sample.data)return 2;
    for(unsigned i=0;i<sample.frames;i++){sample.data[i*2]=sinf(i*.011f)*.7f;sample.data[i*2+1]=cosf(i*.013f)*.6f;}
    Waveform wave={0};if(!waveform_build(&wave,sample))return 2;
    double *times=malloc((size_t)frames*sizeof *times);if(!times)return 2;
    double cpu=0,wall=0;
    for(int i=-40;i<frames;i++) {
        if(i==0){cpu=seconds(CLOCK_PROCESS_CPUTIME_ID);wall=seconds(CLOCK_MONOTONIC);}
        double start=seconds(CLOCK_MONOTONIC);
        int step=i+40;view.span=16+(step%80)*.5f;view.pan=(step%160)*.125f;piano.playback=view.pan+view.span*.6;
        draw_clear(draw,ui_theme.background);editor_draw_begin(render,geometry,&editor);
        piano_draw_toolbar(render,&piano);piano_draw_content(render,&piano);editor_draw_end(render,&editor);
        ui_render_view(render,geometry,(DrawRect){0,0,1200,675});
        WaveDrawing drawing={.sample=sample,.wave=&wave,.offset=(step%32)*128,.origin=8,.width=158};
        waveform_draw(render,drawing,(DrawRect){8,577,158,68},8,166,1,density,ui_theme.waveform,NULL,NULL);
        if(blit){if(!draw_present(draw))return 3;}else draw_flush(draw);if(i>=0)times[i]=seconds(CLOCK_MONOTONIC)-start;
    }
    cpu=seconds(CLOCK_PROCESS_CPUTIME_ID)-cpu;wall=seconds(CLOCK_MONOTONIC)-wall;
    qsort(times,(size_t)frames,sizeof *times,order);
    printf("{\"renderer\":\"SDL software, offscreen\",\"palette\":\"visual preview\",\"includes_sdl_present\":%s,\"density\":%.1f,\"frames\":%d,\"warmup_frames\":40,\"cpu_seconds\":%.6f,\"wall_seconds\":%.6f,\"frame_p50_ms\":%.6f,\"frame_p99_ms\":%.6f,\"frame_max_ms\":%.6f}\n",blit?"true":"false",density,frames,cpu,wall,times[frames/2]*1000,times[(int)((frames-1)*.99)]*1000,times[frames-1]*1000);
    free(times);free(wave.tree);sample_free(sample);ui_render_destroy(render);draw_destroy(draw);SDL_DestroyRenderer(native);SDL_DestroySurface(surface);SDL_Quit();return 0;
}
