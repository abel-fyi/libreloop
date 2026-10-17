// SPDX-License-Identifier: GPL-3.0-only
#include "recording.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project project;
static RecordingSession session;
static unsigned read_fixture(void *context,int take,float *stereo,unsigned frames) {
    unsigned *positions=context,n=4096-positions[take]; if(n>frames) n=frames;
    for(unsigned i=0;i<n;i++) { stereo[i*2]=.25f; stereo[i*2+1]=-.5f; }
    positions[take]+=n; return n;
}
int main(void) {
    char directory[]="/tmp/libreloop-session-XXXXXX"; CHECK(mkdtemp(directory));
    project_new(&project); int buses[]={0,1},invalid[]={INSERTS+1};
    CHECK(!recording_prepare(&session,&project,invalid,1,0,directory) && !session.count && project.channel_count==1);
    CHECK(recording_prepare(&session,&project,buses,2,4,directory));
    CHECK(session.count==2 && project.channel_count==3);
    unsigned positions[2]={0};
    for(int i=0;i<2;i++) CHECK(recording_writer_start(session.takes[i].writer,read_fixture,positions,i));
    int old_lane=session.takes[0].lane,old_clip=session.takes[0].clip;
    project.clips[4][7]=project.clips[old_lane][old_clip]; project.clips[old_lane][old_clip]=0;
    session.active=1; CHECK(recording_finish_writers(&session,&project) && !session.active);
    CHECK(session.takes[0].lane==4 && session.takes[0].clip==7);
    for(int i=0;i<2;i++) {
        CHECK(session.takes[i].sample.frames==4096 && session.takes[i].sample.storage);
        CHECK(project.audio_seconds[session.takes[i].channel]==4096.f/RATE);
        CHECK(session.takes[i].sample.data[1]==-.5f);
    }
    recording_cancel(&session); CHECK(!session.count && !rmdir(directory));
    puts("Recording preparation, moved-clip reconciliation, final mapping and setup cleanup passed without audio hardware."); return 0;
}
