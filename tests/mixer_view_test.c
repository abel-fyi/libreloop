// SPDX-License-Identifier: GPL-3.0-only
#include "mixer_view.h"
#include "navigation.h"
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
#define NEAR(a,b) (fabsf((a)-(b))<.002f)
int main(void) {
    MixerView v=mixer_view(1200,354,99,37*112+.5f,0);
    CHECK(v.master_x==42 && v.insert_x==158 && v.fx_rows==3);
    CHECK(NEAR(mixer_bus_x(v,38),157.5f));
    CHECK(mixer_bus_visible(v,38) && !mixer_bus_visible(v,37));
    CHECK(mixer_route_hit(v,170.5f,v.route+6)==38);
    CHECK(mixer_route_hit(v,55,v.route+6)==0);
    CHECK(mixer_route_hit(v,157.9f,v.route+6)==-1);
    MixerView clipped=mixer_view(1200,354,99,37*112+40,0);
    CHECK(mixer_bus_visible(clipped,38) && !mixer_port_visible(clipped,38,35));
    CHECK(mixer_port_visible(clipped,0,13));
    float fixed=mixer_bus_x(v,0),start=mixer_bus_x(v,38);
    /* Precise device deltas (including decaying momentum) remain fractional. */
    const float sequence[]={-.625f,-.25f,-.125f,-.0625f};
    for(int i=0;i<4;i++) {
        NavigationMotion m=navigation_motion((NavigationInput){sequence[i],0,0,1},0,0,1);
        v=mixer_view(v.width,v.height,v.count,v.offset+m.x,0);
        CHECK(mixer_bus_x(v,0)==fixed);
    }
    CHECK(NEAR(mixer_bus_x(v,38),start-1.0625f));
    CHECK(navigation_motion((NavigationInput){0,-1,0,0},1,0,1).x==48);
    CHECK(navigation_motion((NavigationInput){-.5f,0,0,1},0,0,2).x==.25f);
    float fx=0;
    CHECK(mixer_scroll_motion(v,.25f,1.5f,0,200,v.fx_top+10,&fx)==v.offset+.25f && fx==1.5f);
    CHECK(mixer_scroll_motion(v,48,0,1,200,v.fx_top+10,&fx)==v.offset+48 && fx==1.5f);
    CHECK(mixer_scroll_motion(v,0,48,0,200,v.fader_top+10,&fx)==v.offset+48);
    CHECK(mixer_scroll_motion(v,0,48,0,200,v.controls,&fx)==v.offset);
    CHECK(mixer_scroll_motion(v,48,0,0,60,v.fader_top,&fx)==v.offset);
    CHECK(mixer_scroll_motion(v,0,2,0,60,v.fx_top+5,&fx)==v.offset && fx==3.5f);
    CHECK(mixer_scroll_motion(v,48,3,0,200,10,&fx)==v.offset && fx==3.5f);
    float offsets[101]={0};
    int target=mixer_fx_bus_at(v,mixer_bus_x(v,39)+50);
    CHECK(target==39);
    mixer_scroll_motion(v,0,7.25f,0,mixer_bus_x(v,39)+50,v.fx_top+5,&offsets[target]);
    CHECK(offsets[39]==7.25f && offsets[38]==0 && offsets[0]==0);
    target=mixer_fx_bus_at(v,v.master_x+50);CHECK(target==0);
    mixer_scroll_motion(v,0,3,0,v.master_x+50,v.fx_top+5,&offsets[target]);
    CHECK(offsets[0]==3 && offsets[39]==7.25f);
    CHECK(mixer_fx_bus_at(v,mixer_bus_x(v,39)+110)==-1);
    CHECK(mixer_fx_bus_at(v,v.insert_x-1)==-1);
    CHECK(mixer_fx_bus_at(v,v.insert_x+v.viewport)==-1);
    float grab=7.25f;
    CHECK(NEAR(mixer_scroll_from_thumb(v,mixer_thumb_x(v)+grab,grab),v.offset));
    CHECK(mixer_scroll_from_thumb(v,-100,grab)==0);
    CHECK(mixer_scroll_from_thumb(v,2000,grab)==v.maximum);
    CHECK(mixer_reveal_bus(v,0)==v.offset);
    CHECK(mixer_reveal_bus(v,1)==0);
    CHECK(mixer_reveal_bus(v,99)==v.maximum);
    for(int width=652;width<=1920;width+=37)for(int height=220;height<=700;height+=17)for(int hidden=0;hidden<2;hidden++) {
        v=mixer_view(width,height,99,1e6f,hidden);
        CHECK(v.offset==v.maximum && (hidden?v.fx_rows==0:v.fx_rows>=1));
        CHECK(v.fx_rows>=0 && v.fx_rows<=3);
        CHECK(v.fader_top>=v.controls+14 && v.fader_bottom>=v.fader_top+12);
        CHECK(v.fader_bottom<=height-66 && v.route>=v.fader_bottom+30);
        CHECK(mixer_bus_visible(v,99));
        CHECK(NEAR(mixer_bus_x(v,99)+112,v.insert_x+v.viewport));
        CHECK(NEAR(mixer_thumb_x(v)+mixer_thumb_width(v),v.insert_x+v.viewport));
        CHECK(mixer_route_hit(v,mixer_bus_x(v,99)+13,v.route+6)==99);
    }
    v=mixer_view(1200,354,1,100,1);
    CHECK(v.offset==0 && v.maximum==0 && mixer_thumb_width(v)==v.viewport);
    CHECK(mixer_scroll_from_thumb(v,500,7)==0);
    v=mixer_view(652,220,100,1e6f,0);
    CHECK(mixer_bus_visible(v,100) && mixer_route_hit(v,mixer_bus_x(v,100)+13,v.route+6)==100);
    puts("Fixed Master, fractional scrolling, routing clips, thumb capture, resize bounds and legacy insert 100 passed.");return 0;
}
