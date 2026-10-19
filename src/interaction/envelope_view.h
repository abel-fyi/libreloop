// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_ENVELOPE_VIEW_H
#define LIBRELOOP_ENVELOPE_VIEW_H
#include <math.h>
/* Linear seconds along x. Sustain is an illustrative held interval, never a
   claim about note length. A captured span stays fixed throughout a drag. */
typedef struct { double end[4],hold,seconds; } EnvelopeTimeView;
static inline EnvelopeTimeView adsr_time_view(double attack,double decay,double release,double span) {
    EnvelopeTimeView v={0};v.hold=fmax(.05,(attack+decay+release)*.15);
    v.end[0]=attack;v.end[1]=attack+decay;v.end[2]=v.end[1]+v.hold;v.end[3]=v.end[2]+release;
    v.seconds=span>0?span:fmax(.02,v.end[3]);return v;
}
static inline EnvelopeTimeView dx7_time_view(const double times[4],double span) {
    EnvelopeTimeView v={0};v.hold=fmax(.05,(times[0]+times[1]+times[2]+times[3])*.15);
    v.end[0]=times[0];v.end[1]=v.end[0]+times[1];v.end[2]=v.end[1]+times[2];v.end[3]=v.end[2]+v.hold+times[3];
    v.seconds=span>0?span:fmax(.02,v.end[3]);return v;
}
#endif
