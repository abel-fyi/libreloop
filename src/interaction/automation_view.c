// SPDX-License-Identifier: GPL-3.0-only
/* Frozen ui_widgets.c automation display/held-value behavior. */
#include "automation_view.h"
#include <math.h>
#include <string.h>
void automation_view_update(AutomationView *view,const Project *p,int playing,int song,double step) {
    memset(view->visible,0,sizeof view->visible);if(!playing || !song || !p->automation_count)return;
    int solo=solo_any(p->lane_mute,LANES);double held_end[AUTOMATIONS];for(int a=0;a<p->automation_count;a++)held_end[a]=-INFINITY;
    for(int pass=0;pass<2;pass++)for(int l=0;l<LANES;l++)if(!(p->lane_mute[l]&1) && (!solo || (p->lane_mute[l]&2)))for(int b=0;b<CLIPS;b++) {
        int a=p->clips[l][b]-AUTOMATION_SOURCE-1;if(a<0 || a>=p->automation_count)continue;
        float start=p->clip_starts[l][b]*STEPS,end=start+clip_length(p,l,b);
        if(pass==0?(step<end || end<held_end[a]):(step<start || step>=end))continue;
        ParameterTarget target=p->automations[a].target;
        float normalized=automation_value(&p->automations[a],(pass==0?end:step)-start+p->clip_offsets[l][b]);
        for(int i=0;i<p->automation_count;i++) {
            ParameterTarget t=p->automations[i].target;
            if(t.parameter==target.parameter && t.owner==target.owner && t.slot==target.slot){view->visible[i]=1;view->shown[i]=normalized;if(pass==0)held_end[i]=end;}
        }
    }
}
float automation_displayed_value(const AutomationView *view,const Project *p,int playing,int song,const void *pointer,float manual) {
    if(!playing || !song || !p->automation_count)return manual;ParameterTarget target;float value,lo,hi;
    if(!parameter_from_pointer(p,pointer,&target) || !parameter_info(p,target,&value,&lo,&hi))return manual;
    for(int a=0;a<p->automation_count;a++) {
        ParameterTarget t=p->automations[a].target;
        if(view->visible[a] && t.parameter==target.parameter && t.owner==target.owner && t.slot==target.slot) {
            float shown=lo+(hi-lo)*view->shown[a];return target.parameter==PARAM_PITCH_RANGE?roundf(shown):shown;
        }
    }
    return manual;
}
