// SPDX-License-Identifier: GPL-3.0-only
#define main old_frontend_piano_capture_main
#include "frontend_piano_capture.c"
#undef main
int main(void) {
    project_new(&ui.project);ui.project.pattern_steps[0]=16;ui.arrangement.source_pattern=0;ui.arrangement.source_steps=8;
    ui.piano_note_length=1.5f;ui.piano_zoom=1;ui.piano_top=72;ui.piano_span[0]=32;ui.velocity_drag=-1;
    ui.windows.editors[2].rect=(Rect){0,0,484,580};ui.input_enabled=1;ui.piano_key=-1;
    previews=fopen("previews.txt","w");audition=fopen("audition.txt","w");FILE *out=fopen("conditions.txt","w");if(!previews || !audition || !out)return 1;
    test_left=1;ui.mouse=(DrawPoint){60+4.25f*12.5f,86+(72-59.5f)*16};piano(484,580);
    fprintf(out,"inside %.9g %.9g\n",ui.project.pattern_steps[0],ui.arrangement.source_steps);
    ui.note_drag=NULL;ui.input_enabled=1;ui.piano_span[0]=64;ui.mouse=(DrawPoint){60+35.25f*6.25f,86+(72-59.5f)*16};piano(484,580);
    fprintf(out,"extend %.9g %.9g\n",ui.project.pattern_steps[0],ui.arrangement.source_steps);
    ui.arrangement.source_steps=7;desktop_piano_note_drag_update(ui.note_grab.x,(ui.note_grab.y),grid_interval(6.25f));
    fprintf(out,"drag-inside %.9g %.9g\n",ui.project.pattern_steps[0],ui.arrangement.source_steps);
    fclose(out);fclose(previews);fclose(audition);return 0;
}
