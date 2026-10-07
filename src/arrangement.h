// SPDX-License-Identifier: GPL-3.0-only
#ifndef ARRANGEMENT_H
#define ARRANGEMENT_H
#include "engine.h"
enum { PENCIL, BRUSH, SELECT, CUT, STRETCH };
enum { IDLE, MOVE_CLIPS, SIZE_CLIP, STRETCH_CLIP, PAINT_CLIPS, BOX_SELECT, TRACK_SELECT, ERASE_CLIPS };
typedef struct {
    int tool,gesture,lane,bar,source_pattern,last_lane,edge;
    float source_offset,size_start,size_length,size_offset,size_cap,paint_origin,size_time,size_seconds;
    float x,y,now_x,now_y,source_steps,snap; /* source_steps uses seconds for Audio, steps for patterns */
    uint8_t selected[LANES][CLIPS],before[LANES][CLIPS],selection_before[LANES][CLIPS];
    int additive;
    float zoom,view_start,range;
    float lengths[LANES][CLIPS],starts[LANES][CLIPS],offsets[LANES][CLIPS],caps[LANES][CLIPS];
    uint8_t moved[LANES][CLIPS];
} Arrangement;
int arrangement_place(Project *p,int lane,float bar,int source,float steps);
int arrangement_hit(const Project *p,int lane,float bar);
float arrangement_edit_steps(const Arrangement *a,const Project *p,int pattern);
void arrangement_select_press(Arrangement *a,float bar,float lane,int additive);
void arrangement_press(Arrangement *a,Project *p,float bar,float lane,int right,int edge,int pattern,int additive);
void arrangement_zoom(Arrangement *a,float wheel,float cursor);
void arrangement_release(Arrangement *a);
void arrangement_drag(Arrangement *a,Project *p,float bar,float lane);
#endif
