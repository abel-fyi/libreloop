// SPDX-License-Identifier: GPL-3.0-only
#include "instrument_layout.h"
#include <stdio.h>
#include <math.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void) {
    CHECK(sampler_min_height(0)==300);
    for(int width=330;width<=1920;width++) {
        float content_height=sampler_min_height(1)-6;
        CHECK(content_height-sampler_wave_top(width,1)-24>=110);
        CHECK(sampler_wave_top(width,0)==252);
        float groupw=width/2.f-18;
        float mode_width=width<SAMPLER_COMPACT_WIDTH?groupw-16:groupw-128;
        CHECK(mode_width>=112); /* Full Resample caption plus padding and arrow. */
    }
    SixOpLayout reference=sixop_layout(376,640,1);
    CHECK(reference.spacious && reference.preset==72 && reference.tabs==108);
    CHECK(reference.heading==143 && reference.head==184 && reference.pages==244);
    CHECK(reference.row[0]==298 && reference.row[1]==370);
    CHECK(reference.graph==432 && reference.graph_height==152);
    for(int width=330;width<=640;width+=10)for(int height=406;height<=1200;height+=7) {
        SixOpLayout l=sixop_layout(width,height,1),legacy=sixop_layout(width,height,0);
        CHECK(!legacy.spacious && legacy.preset==52 && legacy.tabs==78 && legacy.head==136);
        CHECK(legacy.pages==184 && legacy.row[0]==226 && legacy.row[1]==292 && legacy.graph==336 && legacy.graph_height==height-378);
        if(l.spacious) {
            CHECK(width>=360 && height>=600);
            CHECK(l.preset+22<l.tabs && l.tabs+26<l.heading);
            CHECK(l.heading+14<l.head-12 && l.head+54<l.pages);
            CHECK(l.pages+24<l.row[0]-12 && l.row[0]+54<l.row[1]-12);
            CHECK(l.row[1]+54<l.graph && l.graph_height>130);
            CHECK(fabsf(l.graph+l.graph_height-(height-56))<.001f);
        } else CHECK(l.tabs==78 && l.graph==336);
    }
    puts("Reference six-op rows match 90s.png and preserve non-overlapping controls/plots across compact and tall windows.");
    return 0;
}
