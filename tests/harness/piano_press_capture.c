// SPDX-License-Identifier: GPL-3.0-only
#include "piano.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static Project project;
static Note before[NOTES];
static uint8_t selected[NOTES],selection_before[NOTES];
static PianoGesture state={.before=before,.selected=selected,.selection_before=selection_before};
static PianoViewport view;
static PianoScrollbars bars;
static int tool,test_left,test_right,test_shift,test_shortcut;
static FILE *previews,*audition;
static PianoKeys keys;
static Note velocity_before[NOTES];
static PianoVelocity velocity={.ramp=-1,.before=velocity_before};
static void key_preview(void *unused,int slot,int channel,int pitch,int down){(void)unused;fprintf(audition,"%d %d %d %d\n",slot,channel,pitch,down);}
static void preview(void *context,int channel,Note n){(void)context;fprintf(previews,"%d %d %d %.9g %.9g\n",channel,n.pitch,n.velocity,n.start,n.length);}
int main(int argc,char **argv) {
    if(argc!=2)return 1;int mode=atoi(argv[1]);if(mode<0 || mode>29)return 1;
    project_new(&project);project.insert_count=INSERTS; /* Frozen reference project geometry. */project.pattern_steps[0]=17.25f;
    state.length=1.5f;view.zoom=1;view.top=72;view.span=32;
    view.width=484;view.height=580;int enabled=1;
    project.notes[0][0][0]=(Note){60,100,4,2};project.notes[0][0][1]=(Note){60,70,4,1};project.notes[0][0][2]=(Note){64,80,8,1.25f};
    float step=10.25f,pitch=64.5f;test_left=1;
    if(mode>=1 && mode<=5){step=4.25f;pitch=59.5f;}
    if(mode==2)step=4.9f;
    if(mode==3)step=4.44f;
    if(mode==4){test_shift=1;selected[1]=1;}
    if(mode==5){test_shortcut=1;selected[0]=1;}
    if(mode==6){test_shift=1;selected[0]=1;}
    if(mode==7){tool=PIANO_TOOL_SELECT;selected[0]=1;}
    if(mode==8 || mode==9){tool=PIANO_TOOL_BRUSH;step=4.25f;pitch=59.5f;selected[1]=1;if(mode==9)selected[0]=1;}
    if(mode==10 || mode==11){test_right=1;test_left=mode==11;step=4.25f;pitch=59.5f;}
    if(mode==12)step=24.25f;
    if(mode==13)step=33;
    if(mode==14)for(int i=3;i<NOTES;i++)project.notes[0][0][i]=(Note){40,20,0,1};
    if(mode==15){view.span=64;step=35.25f;}
    if(mode==20){project.notes[0][0][0]=(Note){60,100,17,4};project.notes[0][0][1].velocity=0;step=17.1f;pitch=59.5f;}
    if(mode==21){view.pan=8;step=.25f;pitch=63.5f;}
    PianoPoint mouse=(PianoPoint){60+step*(400/view.span),86+(view.top-pitch)*16};
    if(mode==16)mouse=(PianoPoint){150,64};
    if(mode==17)mouse=(PianoPoint){400,64};
    if(mode==18)mouse=(PianoPoint){472,80};
    if(mode==19)mouse=(PianoPoint){472,480};
    if(mode>=22)keys.pitch=-1;
    if(mode==22 || mode==23)mouse=(PianoPoint){mode==22?50:20,mode==22?286:266};
    if(mode>=24){mouse=(PianoPoint){111,mode==25?510:520};test_right=mode==25 || mode==27;test_left=!test_right;}
    previews=fopen("previews.txt","w");audition=fopen("audition.txt","w");
    PianoPress press={mouse,enabled,test_left,test_right,test_shift,test_shortcut,tool};
    if(piano_key_press(&keys,&view,0,press,key_preview,NULL))enabled=0;
    press.enabled=enabled;int result=piano_press(&state,&project,&view,press,preview,NULL);if(result&PIANO_PRESS_CONSUMED)enabled=0;
    press.enabled=enabled;if(piano_velocity_press(&velocity,&project,0,0,&view,press))enabled=0;
    press.enabled=enabled;if(piano_scrollbar_press(&view,&bars,view.span*2,(PianoPoint){0,0},press))enabled=0;
    if(mode==22 || mode==23){piano_key_update(&keys,&view,(PianoPoint){20,266},0,key_preview,NULL);piano_key_update(&keys,&view,(PianoPoint){50,266},0,key_preview,NULL);piano_key_update(&keys,&view,(PianoPoint){58,266},0,key_preview,NULL);piano_key_update(&keys,&view,(PianoPoint){50,286},1,key_preview,NULL);}
    if(mode>=24)piano_velocity_update(&velocity,&project,mode==28?1:0,0,&view,(PianoPoint){164,530},mode==26,mode==27,mode!=29);
    fclose(previews);fclose(audition);
    if(!project_save("expected.llp",&project))return 2;
    FILE *selection=fopen("selection.bin","wb"),*state_file=fopen("state.txt","w");
    fwrite(selected,1,NOTES,selection);fwrite(selection_before,1,NOTES,selection);fwrite(before,sizeof(Note),NOTES,selection);fclose(selection);
    fprintf(state_file,"%d %d %d %d %d %d %.9g %.9g %.9g %.9g %.9g %.9g %.9g\n",enabled,state.kind,state.additive,state.moving,state.melodic,state.drag?(int)(state.drag-project.notes[0][0]):-1,state.length,state.from.x,state.from.y,state.now.x,state.now.y,state.grab.x,state.grab.y);fclose(state_file);
    FILE *scroll=fopen("scrollbar.txt","w");fprintf(scroll,"%d %d %d %.9g %.9g %.9g %.9g %.9g %.9g\n",view.top,bars.vertical,bars.horizontal,view.pan,bars.y,bars.pitch_start,bars.x,bars.pan_start,bars.range_per_pixel);fclose(scroll);
    FILE *controls=fopen("controls.txt","w"),*snapshot=fopen("velocity-before.bin","wb");
    fprintf(controls,"%d %d %d %d %d %d %.9g %.9g %.9g %.9g\n",keys.pitch,keys.channel,keys.drag,velocity.ramp,velocity.pattern,velocity.channel,velocity.from.x,velocity.from.y,velocity.now.x,velocity.now.y);fclose(controls);
    fwrite(velocity_before,sizeof(Note),NOTES,snapshot);fclose(snapshot);return 0;
}
