// SPDX-License-Identifier: GPL-3.0-only
#include "../../src/ui/frontend/ui_internal.h"
UiState ui;
#include "../../src/ui/frontend/ui_rack.c"
static FILE *out;
static void snapshot(const char *name){int rows[CHANNELS],n=desktop_rack_channels(rows);fprintf(out,"%s %d %.9g %.9g %d %d",name,ui.rack_paint_channel,ui.rack_paint_step,ui.project.pattern_steps[0],ui.rack_filter,ui.rack_scroll);for(int i=0;i<n;i++)fprintf(out," %d",rows[i]);fprintf(out,"\n");}
#define PAINT(w,x,y) paint_rack(w,x,y)
#define SNAP snapshot
#define SET_TOOL(v) (ui.rack_paint=(v))
#define SET_FILTER(v) (ui.rack_filter=(v))
#define SET_SCROLL(v) (ui.rack_scroll=(v))
#define SET_VIEW(v) (ui.rack_view[0]=(v))
static void run(void){PAINT(480,208,40);SNAP("first");PAINT(480,360,40);SNAP("interpolation");PAINT(480,199.999f,40);SNAP("outside");PAINT(480,328,68);SNAP("second-row");SET_TOOL(1);PAINT(480,360,68);SNAP("erase");SET_FILTER(1);PAINT(480,240,40);SNAP("audio-filter");SET_FILTER(2);PAINT(480,240,40);SNAP("non-audio-filter");SET_FILTER(0);SET_SCROLL(2);PAINT(480,240,40);SNAP("melodic-note-block");SET_SCROLL(3);PAINT(480,240,40);SNAP("melodic-flag-block");SET_SCROLL(0);SET_VIEW(16.25f);SET_TOOL(0);PAINT(480,440,40);SNAP("duration-extension");PAINT(480,454,40);SNAP("right-boundary");}

int main(void){project_new(&ui.project);ui.project.insert_count=INSERTS; /* Frozen reference project geometry. */ui.project.channel_count=4;ui.project.channel_audio[1]=1;ui.project.notes[0][2][0]=(Note){61,100,0,0};ui.piano_channels[0][3]=1;ui.rack_paint_channel=-1;ui.windows.editors[0].rect.h=168;out=fopen("rack.txt","w");if(!out)return 1;run();fclose(out);return project_save("expected.llp",&ui.project)?0:1;}
