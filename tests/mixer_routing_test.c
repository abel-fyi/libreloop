// SPDX-License-Identifier: GPL-3.0-only
#include "playback_plan.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project project,loaded;
static Sample samples[CHANNELS];
static float source[INSERTS+1][2];
static void input(void *unused,float buses[INSERTS+1][2]){(void)unused;for(int id=1;id<=INSERTS;id++){buses[id][0]+=source[id][0];buses[id][1]+=source[id][1];}}
static float run(int cached) {
    static PlaybackPlan plan;Player player;player_reset(&player);
    MixerIO io={.input=input};for(int id=1;id<=project.insert_count;id++)io.active[id]=source[id][0]!=0 || source[id][1]!=0;
    playback_plan_prepare(&plan,&project,0,&io,0);if(cached)player.plan=&plan;
    float out[2]={0};render_mixer_io(&player,NULL,&project,samples,out,1,0,NULL,&io);return out[0];
}
int main(void) {
    project_new(&project);project.master=1;source[1][0]=source[1][1]=.125f;
    CHECK(run(0)==.125f);
    CHECK(insert_send_add(&project,1,2) && insert_send_add(&project,1,2));
    CHECK(run(0)==.25f && run(1)==run(0));
    CHECK(insert_send_add(&project,1,3) && insert_send_add(&project,3,2));
    CHECK(run(0)==.5f && run(1)==run(0)); /* 1->M, 1->2->M, 1->3->M, 1->3->2->M */
    CHECK(!insert_send_add(&project,2,1) && !insert_send_add(&project,2,3) && !insert_send_add(&project,1,1));
    CHECK(insert_routes_valid(&project));
    CHECK(project_save("multi-routing.llp",&project) && project_load("multi-routing.llp",&loaded));
    CHECK(!memcmp(loaded.insert_sends,project.insert_sends,sizeof project.insert_sends));
    CHECK(insert_send_remove(&project,1,0) && !insert_has_send(&project,1,0));
    CHECK(run(0)==.375f);
    CHECK(insert_send_remove(&project,3,0));CHECK(run(0)==.25f);
    project.insert_mute[1]=1;CHECK(run(0)==0);project.insert_mute[1]=0;
    CHECK(insert_send_remove(&project,3,2));
    CHECK(insert_connect(&project,1,3) && insert_connect(&project,2,3));
    CHECK(!project.insert_sends[0][2] && insert_has_send(&project,1,3));
    source[2][0]=source[2][1]=.25f;
    CHECK(insert_send_add(&project,3,0));CHECK(run(0)==.375f && run(1)==.375f);
    CHECK(insert_reset(&project,1));CHECK(!project.insert_sends[0][3] && insert_has_send(&project,1,0));
    project.insert_sends[2][1]=1;CHECK(insert_send_add(&project,1,3)==0);
    project.insert_sends[0][3]=1;CHECK(!insert_routes_valid(&project) && !project_save("invalid-routing.llp",&project));
    project.insert_sends[0][3]=0;project.insert_sends[2][1]=0;project.insert_sends[0][0]=1;
    CHECK(!insert_routes_valid(&project));
    remove("multi-routing.llp");remove("invalid-routing.llp");
    puts("Fan-out, fan-in, unity summation, duplicate suppression, individual removal, cached rendering, roundtrip and cycle rejection passed.");return 0;
}
