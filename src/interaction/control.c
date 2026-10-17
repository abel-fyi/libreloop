// SPDX-License-Identifier: GPL-3.0-only
/* Original ui_widgets.c control capture/wheel and main.c drag arithmetic. */
#include "control.h"
#include <math.h>
void control_capture(ControlDrag *s,Project *p,float *value,float low,float high,int fader) {
    ParameterTarget target;const ParameterDescriptor *descriptor=parameter_from_pointer(p,value,&target)?parameter_descriptor(target.parameter):NULL;
    s->integer=descriptor && descriptor->kind==PARAMETER_INTEGER;s->logarithmic=descriptor && descriptor->kind==PARAMETER_LOGARITHMIC;
    s->magnetic=descriptor && target.parameter==PARAM_DELAY_STEPS;
    s->raw=*value;s->value=value;s->low=descriptor?descriptor->low:low;s->high=descriptor?descriptor->high:high;s->fader=fader;s->reverse=0;
}
static const float step_marks[]={.125f,1.f/6,.25f,1.f/3,.5f,2.f/3,.75f,1,4.f/3,1.5f,2,8.f/3,3,4,16.f/3,6,8,32.f/3,12,16};
const float *control_step_marks(unsigned *count) {
    *count=sizeof step_marks/sizeof *step_marks;return step_marks;
}
float control_snap_steps(float raw) {
    unsigned count;const float *marks=control_step_marks(&count);
    float best=raw,distance=.035f;
    for(unsigned i=0;i<count;i++) {
        float d=fabsf(logf(raw/marks[i]));if(d<distance){distance=d;best=marks[i];}
    }
    return best;
}
void control_drag_update_fine(ControlDrag *s,Project *p,float delta,float scale,float y,int fine) {
    if(!s->value)return;
    if(s->magnetic) {
        s->raw=fmaxf(s->low,fminf(s->high,s->raw*powf(s->high/s->low,-delta/scale/(fine?1600:160))));
        *s->value=fine?s->raw:control_snap_steps(s->raw);return;
    }
    float value=s->fader?fader_gain(fmaxf(0,fminf(1,(s->bottom-y)/s->range))):*s->value-delta/scale*(s->reverse?-1:1)*(s->value==&p->bpm?.25f:(s->high-s->low)/160);
    if(s->logarithmic)value=*s->value*powf(s->high/s->low,-delta/scale/160);
    if(s->integer){s->raw=fmaxf(s->low,fminf(s->high,s->raw-delta/scale*(s->high-s->low)/160));value=roundf(s->raw);}
    *s->value=fmaxf(s->low,fminf(s->high,value));
}
void control_knob_wheel(Project *p,float *value,float low,float high,int style,float wheel) {
    if(!wheel)return;ParameterTarget target;const ParameterDescriptor *info=parameter_from_pointer(p,value,&target)?parameter_descriptor(target.parameter):NULL;
    int integer=info && info->kind==PARAMETER_INTEGER;
    float next=style==CONTROL_LOGARITHMIC?*value*powf(high/low,wheel/20):*value+wheel*(integer?1:(high-low)/20)*(style==CONTROL_WIDTH?-1:1);
    *value=fmaxf(low,fminf(high,integer?roundf(next):next));
}
void control_fixed_alias(float *value,int style,int over,int pressed,float wheel) {
    if(style==CONTROL_FIXED_COARSE && over && (pressed || wheel))*value=(int)roundf(*value)&3;
}

void control_drag_update(ControlDrag *s,Project *p,float delta,float scale,float y) {control_drag_update_fine(s,p,delta,scale,y,0);}
void control_knob_wheel_fine(ControlWheel *state,Project *p,float *value,float low,float high,int style,float wheel,int fine) {
    if(style!=CONTROL_MAGNETIC){state->value=NULL;control_knob_wheel(p,value,low,high,style,wheel);return;}
    if(!wheel)return;
    if(state->value!=value || state->shown!=*value){state->value=value;state->raw=*value;}
    state->raw=fmaxf(low,fminf(high,state->raw*powf(high/low,wheel/(fine?800:80))));
    *value=fine?state->raw:control_snap_steps(state->raw);state->shown=*value;
}
