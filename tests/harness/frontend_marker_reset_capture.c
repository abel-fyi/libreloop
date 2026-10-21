// SPDX-License-Identifier: GPL-3.0-only
#define main old_frontend_ruler_main
#include "frontend_ruler_capture.c"
#undef main
int main(void){ui.pattern=0;ui.marker_drag=2;ui.reset=0;desktop_ruler_drag(8,0,0,1);FILE *out=fopen("reset.txt","wb");if(!out)return 1;fprintf(out,"%d %d %.9g\n",ui.reset,ui.marker_drag,ui.piano_start[0]);ui.reset=0;ui.playing=1;desktop_ruler_drag(8,0,0,1);fprintf(out,"%d %d %.9g\n",ui.reset,ui.marker_drag,ui.piano_start[0]);fclose(out);return 0;}
