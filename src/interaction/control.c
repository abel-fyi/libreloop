// SPDX-License-Identifier: GPL-3.0-only
/* Original ui_widgets.c control capture/wheel and main.c drag arithmetic. */
#include "control.h"
#include <math.h>
void control_capture(ControlDrag *s,Project *p,float *value,float low,float high,int fader) {
    ParameterTarget target;const ParameterDescriptor *descriptor=parameter_from_pointer(p,value,&target)?parameter_descriptor(target.parameter):NULL;
    s->integer=descriptor && descriptor->kind==PARAMETER_INTEGER;s->logarithmic=descriptor && descriptor->kind==PARAMETER_LOGARITHMIC;
    s->raw=*value;s->value=value;s->low=descriptor?descriptor->low:low;s->high=descriptor?descriptor->high:high;s->fader=fader;s->reverse=0;
}
void control_drag_update(ControlDrag *s,Project *p,float delta,float scale,float y) {
    if(!s->value)return;
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
