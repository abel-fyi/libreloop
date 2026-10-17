// SPDX-License-Identifier: GPL-3.0-only
#include "record_count_in.h"
#include <math.h>
int record_count_in_begin(RecordCountIn *c,int steps,float bpm,double time) {
    if(steps<1 || steps>4096 || !isfinite(bpm) || bpm<=0 || !isfinite(time))return 0;
    *c=(RecordCountIn){1,steps,bpm,time,time+steps*15.0/bpm};return 1;
}
int record_count_in_remaining(const RecordCountIn *c,double time) {
    if(!c->active || !isfinite(time) || time>=c->end)return 0;
    return (int)fmin(c->steps,ceil((c->end-time)*c->bpm/15.0));
}
