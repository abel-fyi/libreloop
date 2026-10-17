// SPDX-License-Identifier: GPL-3.0-only
/* Extracted from frozen ui_piano.c; formulas/branch order retained. */
#include "grid.h"
#include "piano.h"
#include <math.h>
#include <string.h>
int piano_inactive(const Project *project,int pattern,float step) {
    float end=project->pattern_steps[pattern]; return step>=end && step<ceilf(end/STEPS)*STEPS;
}
static void piano_extend(Project *project,int pattern,float end) {
    if(end<=project->pattern_steps[pattern]) return;
    project->pattern_steps[pattern]=ceilf(end/STEPS)*STEPS;
}
void piano_gesture_update(PianoGesture *state,Project *project,float step,float pitch,float q,PianoPreview preview,void *context) {
    int channel=state->channel;
    uint8_t *selected=state->selected;
    if(state->kind==PIANO_BOX) {
        float left=fminf(state->from.x,step),right=fmaxf(state->from.x,step),low=fminf(state->from.y,pitch),high=fmaxf(state->from.y,pitch);
        for(int i=0;i<NOTES;i++) {
            Note n=project->notes[state->pattern][channel][i];
            selected[i]=(state->additive && state->selection_before[i]) || (n.velocity && n.start<project->pattern_steps[state->pattern] && n.start<=right && n.start+(n.length?n.length:1)>left && n.pitch>=low && n.pitch-1<=high);
        }
    } else {
        float dx=step-state->now.x,dy=pitch-state->now.y,spacing=state->kind==PIANO_BRUSH?fmaxf(state->length,q):q>0?q:1;
        int count=fminf(NOTES*2,ceilf(fmaxf(fabsf(dx)/spacing,fabsf(dy))*2)); if(count<1) count=1;
        for(int j=0;j<=count;j++) {
            float t=j/(float)count,at=state->now.x+dx*t; int key=ceilf(state->now.y+dy*t);
            if(at<0 || piano_inactive(project,state->pattern,at) || key<0 || key>127) continue;
            if(state->kind==PIANO_ERASE) {
                /* Hit the gesture's original stack so holding the button or
                   repeated stroke samples cannot peel off underlying notes. */
                int i=note_hit(state->before,at,key);
                if(i>=0) { project->notes[state->pattern][channel][i].velocity=0; selected[i]=0; }
            } else {
                float start=state->from.x+floorf((at-state->from.x)/spacing)*spacing;
                piano_extend(project,state->pattern,start+state->length);
                int exists=note_at(project,state->pattern,channel,start,key)!=NULL;
                Note *n=exists?NULL:note_add(project,state->pattern,channel,start,key,state->length);
                if(n) { if(preview) preview(context,channel,*n); state->melodic=1; }
            }
        }
    }
    state->now=(PianoPoint){step,pitch};
}

void piano_note_drag_update(PianoGesture *state,Project *project,float step,float row,float q,PianoPreview preview,void *context) {
    int channel=state->channel;
    if(state->moving) {
        uint8_t *selected=state->selected;
        float dx=snap_round(step-state->grab.x,q),low=0,high=0; int dy=roundf(state->grab.y-row),bottom=0,top=127,first=1;
        for(int i=0;i<NOTES;i++) if(selected[i]) {
            Note n=state->before[i]; float end=n.start+(n.length?n.length:1);
            if(first || n.start<low) low=n.start;
            if(first || end>high) high=end;
            if(first || n.pitch<bottom) bottom=n.pitch;
            if(first || n.pitch>top) top=n.pitch;
            first=0;
        }
        dx=fmaxf(-low,dx); piano_extend(project,state->pattern,high+dx); dy=fmaxf(-bottom,fminf(127-top,dy));
        int old_pitch=state->drag->pitch;
        notes_move(project,state->pattern,channel,state->before,selected,dx,dy,project->pattern_steps[state->pattern]);
        if(state->drag->pitch!=old_pitch && preview) preview(context,channel,*state->drag);
    } else {
        float delta=snap_round(step-state->grab.x,q);
        float end=notes_resize(project,state->pattern,channel,state->before,state->selected,delta,q>0?q:.01f);
        piano_extend(project,state->pattern,end); state->length=state->drag->length;
    }
}

float piano_rows_for_area(float area,float zoom,float reference_area) {
    float rows=fmaxf(8,fminf(128,25/zoom));
    return reference_area>0?fmaxf(1,area)*rows/reference_area:rows;
}
float piano_rows(const PianoViewport *view) { return piano_rows_for_area(view->height-180,view->zoom,view->reference_area); }
static int inside(PianoPoint point,float x,float y,float width,float height) {
    return point.x>=x && point.x<x+width && point.y>=y && point.y<y+height;
}
Note *piano_pointer_note(Project *project,int pattern,int channel,const PianoViewport *v,PianoPoint pointer) {
    int gx=60,gy=86;float gridw=v->width-84,rows=piano_rows(v),rh=(v->height-180)/rows;
    if(!inside(pointer,gx,gy,gridw,rh*rows))return NULL;
    float cw=gridw/v->span,end=project->pattern_steps[pattern];Note *hit=NULL;
    for(int i=0;i<NOTES;i++) {
        Note *n=&project->notes[pattern][channel][i];
        if(!n->velocity || n->pitch<v->top-(int)ceilf(rows)+1 || n->pitch>v->top || n->start>=end || n->start+(n->length?n->length:1)<=v->pan)continue;
        float note_end=fminf(n->start+(n->length?n->length:1),end);
        float x=gx+(n->start-v->pan)*cw,y=gy+(v->top-n->pitch)*rh,w=(note_end-n->start)*cw;
        float left=fmaxf(gx,x),right=fminf(gx+gridw,x+w);if(right<=left)continue;
        if(inside(pointer,x,y,w,rh))hit=n;
    }
    return hit;
}
int piano_press(PianoGesture *s,Project *p,const PianoViewport *v,PianoPress press,PianoPreview preview,void *context) {
    int gx=60,gy=86;float gridw=v->width-84,rh=(v->height-180)/piano_rows(v);
    if(!press.enabled || !inside(press.pointer,gx,gy,gridw,rh*piano_rows(v)))return 0;
    if(v->reference_area>0 && press.pointer.y>=gy+(v->top+1)*rh)return PIANO_PRESS_INACTIVE;
    float cw=gridw/v->span,q=grid_choice_interval(v->grid_choice,cw),end=p->pattern_steps[s->pattern];
    float start=snap_floor(v->pan+(press.pointer.x-gx)/cw,q);
    if(piano_inactive(p,s->pattern,start))return PIANO_PRESS_INACTIVE;
    if(!press.left && !press.right)return 0;
    Note *hit=piano_pointer_note(p,s->pattern,s->channel,v,press.pointer);
    uint8_t *selected=s->selected;Note *notes=p->notes[s->pattern][s->channel];
    if(hit && press.left)s->length=hit->length?hit->length:2;
    PianoPoint point={v->pan+(press.pointer.x-gx)/cw,v->top-(press.pointer.y-gy)/rh};
    int group=0;for(int i=0;i<NOTES;i++)group+=selected[i]!=0;
    s->from=s->now=point;s->additive=press.shift;memcpy(s->selection_before,selected,NOTES);
    int result=PIANO_PRESS_CONSUMED;
    if(press.right){memcpy(s->before,notes,NOTES*sizeof *notes);s->kind=PIANO_ERASE;}
    else if(press.shortcut){if(!press.shift)memset(selected,0,NOTES);s->kind=PIANO_BOX;}
    else if(press.shift && hit)selected[hit-notes]^=1;
    else if((press.shift || press.tool==PIANO_TOOL_SELECT) && !hit){if(!press.shift)memset(selected,0,NOTES);s->kind=PIANO_BOX;}
    else if(press.tool==PIANO_TOOL_BRUSH && !(hit && selected[hit-notes] && group>1))s->kind=PIANO_BRUSH;
    else {
        s->moving=0;
        if(hit) {
            float edge=gx+(fminf(hit->start+(hit->length?hit->length:1),end)-v->pan)*cw;
            s->drag=hit;s->moving=press.pointer.x<edge-7;
            if(!selected[hit-notes]){memset(selected,0,NOTES);selected[hit-notes]=1;}
            memcpy(s->before,notes,NOTES*sizeof *notes);s->grab=(PianoPoint){point.x,(press.pointer.y-gy)/rh};
        } else {
            memset(selected,0,NOTES);piano_extend(p,s->pattern,start+s->length);
            s->drag=note_add(p,s->pattern,s->channel,start,ceilf(point.y),s->length);
            if(s->drag) {
                selected[s->drag-notes]=1;s->moving=1;
                memcpy(s->before,notes,NOTES*sizeof *notes);s->grab=(PianoPoint){point.x,(press.pointer.y-gy)/rh};
            }
        }
        if(!s->drag)result|=PIANO_PRESS_NOTE_LIMIT;
        else {if(preview)preview(context,s->channel,*s->drag);s->melodic=1;}
    }
    if(s->kind==PIANO_BRUSH)s->from.x=start;
    if(s->kind)piano_gesture_update(s,p,point.x,point.y,q,preview,context);
    return result;
}
int piano_scrollbar_press(PianoViewport *v,PianoScrollbars *bars,float range,PianoPoint origin,PianoPress press) {
    if(!press.enabled || !press.left)return 0;
    float rows=piano_rows(v),rh=(v->height-180)/rows;
    float rail_top=86-14,varea=rh*rows+14,vthumb=varea*fminf(128,rows)/128;
    float vypos=rail_top+(127-v->top)/128.f*varea;
    if(inside(press.pointer,v->width-4-(16+16)/2,rail_top,16,varea)) {
        if(press.pointer.y<vypos || press.pointer.y>vypos+vthumb)
            v->top=127-(int)fmaxf(0,fminf(127-((int)fminf(128,ceilf(rows))-1),(press.pointer.y-rail_top-vthumb/2)/varea*128));
        bars->y=press.pointer.y+origin.y;bars->pitch_start=127-v->top;bars->vertical=1;return 1;
    }
    float rail_width=v->width-4-16-60,thumb=timeline_thumb(rail_width,v->span,range);
    float travel=rail_width-thumb,maximum=range-v->span;
    float x=60+(maximum>0?fmaxf(0,v->pan)/maximum*travel:0);
    if(inside(press.pointer,60,86-14-(16+16)/2,rail_width,16)) {
        if(press.pointer.x<x || press.pointer.x>x+thumb)
            v->pan=fmaxf(0,fminf(maximum,(press.pointer.x-60-thumb/2)/fmaxf(1,travel)*maximum));
        bars->x=press.pointer.x+origin.x;bars->pan_start=v->pan;bars->range_per_pixel=maximum/fmaxf(1,travel);bars->horizontal=1;return 1;
    }
    return 0;
}
void piano_scrollbar_update(PianoViewport *v,PianoScrollbars *bars,PianoPoint pointer,int held,int pressed) {
    if(bars->horizontal) {
        v->pan=fmaxf(0,bars->pan_start+(pointer.x-bars->x)*bars->range_per_pixel);
        if(!held || pressed)bars->horizontal=0;
    }
    if(bars->vertical) {
        float area=v->height-180+14;
        v->top=127-(int)fmaxf(0,fminf(127-((int)fminf(128,ceilf(piano_rows(v)))-1),bars->pitch_start+(pointer.y-bars->y)/area*128));
        if(!held || pressed)bars->vertical=0;
    }
}
int piano_is_black_key(int pitch) { int pc=pitch%12;return pc==1 || pc==3 || pc==6 || pc==8 || pc==10; }
PianoBounds piano_key_bounds(const PianoViewport *v,int pitch) {
    float rh=(v->height-180)/piano_rows(v),center=86+(v->top-pitch+.5f)*rh;
    if(piano_is_black_key(pitch))return (PianoBounds){4,center-rh*.5f,34,rh};
    float top=center-rh*(piano_is_black_key(pitch+1)?1:.5f);
    float bottom=center+rh*(piano_is_black_key(pitch-1)?1:.5f);
    top=fmaxf(86,top);bottom=fminf(86+rh*piano_rows(v),bottom);
    return (PianoBounds){4,top,54,fmaxf(0,bottom-top)};
}
void piano_note_edges(const Project *p,int pattern,int channel,const PianoViewport *v,PianoPoint pointer,int enabled,int shortcut,uint8_t edges[NOTES]) {
    memset(edges,0,NOTES);float rows=piano_rows(v),rh=(v->height-180)/rows,gridw=v->width-84;
    if(!enabled || shortcut || !inside(pointer,60,86,gridw,rh*rows))return;
    float cw=gridw/v->span,end=p->pattern_steps[pattern];
    for(int i=0;i<NOTES;i++) {
        Note n=p->notes[pattern][channel][i];
        if(!n.velocity || n.pitch<v->top-(int)ceilf(rows)+1 || n.pitch>v->top || n.start>=end || n.start+(n.length?n.length:1)<=v->pan)continue;
        float note_end=fminf(n.start+(n.length?n.length:1),end);
        float x=60+(n.start-v->pan)*cw,y=86+(v->top-n.pitch)*rh,w=(note_end-n.start)*cw;
        float left=fmaxf(60,x),right=fminf(60+gridw,x+w);if(right<=left)continue;
        edges[i]=inside(pointer,x+w-7,y,7,rh);
    }
}
int piano_key_hit(const PianoViewport *v,PianoPoint point) {
    float rh=(v->height-180)/piano_rows(v);
    if(point.x<4 || point.x>=58 || point.y<86 || point.y>=86+rh*piano_rows(v))return -1;
    float row=(point.y-86)/rh;int pitch=v->top-(int)floorf(row);
    if(piano_is_black_key(pitch) && point.x>=38)pitch+=row-floorf(row)<.5f?1:-1;
    return pitch>=0 && pitch<=127?pitch:-1;
}
void piano_key_update(PianoKeys *keys,const PianoViewport *v,PianoPoint pointer,int cancel,PianoKeyPreview preview,void *context) {
    int pitch=piano_key_hit(v,pointer);if(cancel || pitch<0 || pitch>127)pitch=-1;
    if(pitch!=keys->pitch) {
        if(keys->pitch>=0 && preview)preview(context,96+keys->pitch%25,keys->channel,keys->pitch,0);
        keys->pitch=pitch;
        if(pitch>=0 && preview)preview(context,96+pitch%25,keys->channel,pitch,1);
    }
    if(cancel)keys->drag=0;
}
int piano_key_press(PianoKeys *keys,const PianoViewport *v,int channel,PianoPress press,PianoKeyPreview preview,void *context) {
    float rh=(v->height-180)/piano_rows(v);
    if(!press.enabled || !press.left || !inside(press.pointer,4,86,60-6,rh*piano_rows(v)))return 0;
    keys->drag=1;keys->channel=channel;piano_key_update(keys,v,press.pointer,0,preview,context);return 1;
}
int piano_velocity_press(PianoVelocity *velocity,Project *p,int pattern,int channel,const PianoViewport *v,PianoPress press) {
    int vy=v->height-84;float gridw=v->width-84,cw=gridw/v->span;
    if(!press.enabled || !(press.left || press.right) || !inside(press.pointer,60,vy,gridw,62))return 0;
    velocity->ramp=press.right?1:0;velocity->pattern=pattern;velocity->channel=channel;
    velocity->from=velocity->now=(PianoPoint){v->pan+(press.pointer.x-60)/cw,fmaxf(1,fminf(127,(vy+62-press.pointer.y)/.48f))};
    memcpy(velocity->before,p->notes[pattern][channel],NOTES*sizeof(Note));
    notes_velocity(p->notes[pattern][channel],velocity->ramp?velocity->before:NULL,
        velocity->from.x,velocity->from.x,velocity->from.y,velocity->from.y,3/cw);return 1;
}
void piano_velocity_update(PianoVelocity *velocity,Project *p,int pattern,int channel,const PianoViewport *v,PianoPoint pointer,int cancel,int new_press,int held) {
    if(velocity->ramp<0)return;
    if(cancel || pattern!=velocity->pattern || channel!=velocity->channel || new_press){velocity->ramp=-1;return;}
    float cw=(v->width-84)/v->span;
    PianoPoint point={v->pan+(pointer.x-60)/cw,fmaxf(1,fminf(127,(v->height-22-pointer.y)/.48f))};
    PianoPoint from=velocity->ramp?velocity->from:velocity->now;
    notes_velocity(p->notes[pattern][channel],velocity->ramp?velocity->before:NULL,from.x,point.x,from.y,point.y,3/cw);
    velocity->now=point;if(!held)velocity->ramp=-1;
}

