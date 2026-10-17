// SPDX-License-Identifier: GPL-3.0-only
#include "velocity_lane.h"
#include "piano.h"
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void) {
    VelocityLane lane={0};
    CHECK(velocity_lane_height(0,720)==62);
    CHECK(velocity_lane_scale(62)==.48f);
    velocity_lane_update(&lane,720,631,1,1,0);
    velocity_lane_update(&lane,720,531,0,1,0);
    CHECK(lane.dragging && lane.height==162);
    velocity_lane_update(&lane,720,500,0,0,0);
    CHECK(!lane.dragging && lane.height==193);
    velocity_lane_update(&lane,720,500,1,1,0);
    velocity_lane_update(&lane,720,-1000,0,1,0);
    CHECK(lane.height==506); /* 96-unit minimum note area. */
    velocity_lane_update(&lane,720,10000,0,1,0);
    CHECK(lane.collapsed && lane.height==193);
    velocity_lane_update(&lane,720,10000,0,1,1);
    CHECK(!lane.dragging && lane.height==193);
    CHECK(velocity_lane_height(193,300)==86);
    CHECK(velocity_lane_height(lane.height,720)==193); /* window resize is temporary */
    /* Collapse keeps a restore size; click or upward drag reopens it. */
    velocity_lane_update(&lane,720,500,1,1,0);
    velocity_lane_update(&lane,720,800,0,1,0);
    CHECK(lane.collapsed && lane.dragging);
    velocity_lane_update(&lane,720,800,0,0,0);
    CHECK(lane.collapsed && !lane.dragging);
    float remembered=lane.height;
    velocity_lane_update(&lane,720,700,1,1,0);
    velocity_lane_update(&lane,720,700,0,0,0);
    CHECK(!lane.collapsed && lane.height==remembered);
    lane.collapsed=1;
    velocity_lane_update(&lane,720,700,1,1,0);
    velocity_lane_update(&lane,720,600,0,1,0);
    CHECK(!lane.collapsed && lane.height==100);
    velocity_lane_update(&lane,720,600,0,1,1);
    CHECK(lane.collapsed && lane.height==remembered && !lane.dragging);
    VelocityLane direct={.height=180},sampled={.height=180};
    velocity_lane_update(&direct,720,500,1,1,0);
    velocity_lane_update(&sampled,720,500,1,1,0);
    velocity_lane_update(&direct,720,800,0,0,0);
    for(int y=510;y<=800;y+=10)velocity_lane_update(&sampled,720,y,0,y<800,0);
    CHECK(direct.collapsed && sampled.collapsed && direct.height==sampled.height && direct.height==180);
    for(int height=300;height<=1080;height+=30)for(int requested=40;requested<=600;requested+=13){
        float h=velocity_lane_height(requested,height),scale=velocity_lane_scale(h);
        PianoViewport view={.width=800,.height=height-(h-62),.span=64,.zoom=1,.top=72,.reference_area=130};
        CHECK(fabsf((view.height-180)/piano_rows(&view)-130/25.f)<.0001f);
        for(int velocity=1;velocity<=127;velocity++) {
            float y=height-22-velocity*scale;
            float result=(height-22-y)/scale;
            CHECK(fabsf(result-velocity)<.001f);
        }
    }
    puts("Velocity lane drag/release/cancel, resize bounds, value mapping and fixed note zoom passed.");
    return 0;
}
