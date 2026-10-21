// SPDX-License-Identifier: GPL-3.0-only
#define main old_frontend_rack_main
#include "frontend_rack_capture.c"
#undef main
int main(void){project_new(&ui.project);ui.project.pattern_steps[0]=16;ui.project.channel_count=1;ui.windows.editors[0].rect.h=168;ui.rack_paint_channel=-1;ui.arrangement.source_pattern=0;ui.arrangement.source_steps=8;
    FILE *out=fopen("conditions.txt","wb");if(!out)return 1;paint_rack(480,208,40);fprintf(out,"inside %.9g %.9g\n",ui.project.pattern_steps[0],ui.arrangement.source_steps);
    ui.rack_view[0]=16;paint_rack(480,208,40);fprintf(out,"extend %.9g %.9g\n",ui.project.pattern_steps[0],ui.arrangement.source_steps);
    fclose(out);return 0;}
