// SPDX-License-Identifier: GPL-3.0-only
#include "chorus.h"
#include "sample.h"
#include <math.h>
#include <string.h>
ChorusSettings chorus_default(void) { return (ChorusSettings){.rate=.8f,.depth=3}; }
int chorus_valid(ChorusSettings s) { return isfinite(s.rate) && s.rate>=.05f && s.rate<=5 && isfinite(s.depth) && s.depth>=0 && s.depth<=8; }
void chorus_reset(Chorus *c) { memset(c,0,sizeof *c); }
void chorus_process(Chorus *c,ChorusSettings s,float wet,float stereo[2]) {
    if(!c->ready) { c->rate=s.rate; c->depth=s.depth; c->ready=1; }
    const float slew=1.f/(RATE*.02f);
    c->rate+=(s.rate-c->rate)*slew; c->depth+=(s.depth-c->depth)*slew;
    c->wet+=(wet-c->wet)*slew;
    for(int side=0;side<2;side++) {
        float input=isfinite(stereo[side])?stereo[side]:0;
        c->delay[c->cursor][side]=input;
        double delay=RATE*.001*(10+c->depth*sin(c->phase+side*1.5707963267948966));
        double position=c->cursor-delay;
        if(position<0) position+=CHORUS_DELAY_FRAMES;
        unsigned at=(unsigned)position,next=(at+1)%CHORUS_DELAY_FRAMES;
        float a=c->delay[at][side],b=c->delay[next][side],delayed=a+(b-a)*(position-at);
        stereo[side]=input*(1-c->wet)+delayed*c->wet;
    }
    c->cursor=(c->cursor+1)%CHORUS_DELAY_FRAMES;
    c->phase+=6.283185307179586*c->rate/RATE;
    if(c->phase>=6.283185307179586) c->phase-=6.283185307179586;
}
