// SPDX-License-Identifier: GPL-3.0-only
#include "piano.h"
#include "editor_navigation.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x);return 1;}} while(0)
static Project project;
static float row_height(PianoViewport v){return (v.height-180)/piano_rows(&v);}
int main(void) {
    project_new(&project);project.pattern_steps[0]=64;
    project.notes[0][0][0]=(Note){60,100,4,2};
    for(int density=1;density<=2;density++)for(int zi=0;zi<3;zi++) {
        float zoom=zi==0?.5f:zi==1?1:2;
        PianoViewport view={692,310,64,0,zoom,72,130};
        float initial=row_height(view);
        const float sizes[]={310,600,1000,390,310};
        for(unsigned i=0;i<sizeof sizes/sizeof sizes[0];i++) {
            view.height=sizes[i];
            CHECK(fabsf(row_height(view)-initial)<.00001f);
            PianoPoint p={60+4.5f*(view.width-84)/view.span,86+12.5f*initial};
            /* Device pixels return through the same logical coordinate boundary. */
            p.x=p.x*density/density;p.y=p.y*density/density;
            if(p.y<86+view.height-180) {
                CHECK(piano_pointer_note(&project,0,0,&view,p)==&project.notes[0][0][0]);
                CHECK(piano_key_hit(&view,(PianoPoint){20,p.y})==60);
            }
        }
        PianoViewport legacy=view;legacy.reference_area=0;
        float old=row_height(legacy);legacy.height+=200;
        CHECK(row_height(legacy)>old); /* Frozen legacy path remains available. */
    }
    Windows windows;windows_init(&windows,1448,1086);
    for(int id=0;id<EDITORS;id++)windows.editors[id].visible=id==2;
    windows.editors[2].rect=(Rect){244,572,858,276};
    windows.editors[2].minh=260;windows_focus(&windows,2);
    Rect desktop={244,98,1200,964},original=windows.editors[2].rect;
    float x=original.x+original.w-27,y=original.y+9;
    windows_update(&windows,desktop,x,y,1,1,1);
    CHECK(windows.editors[2].maximized);
    PianoViewport large={1034,windows.editors[2].rect.h+34,64,0,1,127,130};
    CHECK(fabsf(row_height(large)-5.2f)<.00001f);
    windows_update(&windows,desktop,0,0,0,0,1.1);
    x=windows.editors[2].rect.x+windows.editors[2].rect.w-27;y=windows.editors[2].rect.y+9;
    windows_update(&windows,desktop,x,y,1,1,1.2);
    CHECK(!windows.editors[2].maximized && windows.editors[2].rect.h==original.h);
    /* Precise scrolling has the same pitch sensitivity before/after resize. */
    for(int density=1;density<=2;density++) {
        EditorNavigation small={.windows=&windows,.drag=-1,.piano_zoom=1,.piano_top=110};
        small.piano_span[0]=64;EditorNavigation big=small;
        NavigationFrame f={.pointer={500,700},.layout_scale=density,
            .piano_content={410,538,692,310},.piano_reference_area=130,
            .scroll={.y=-3.25f,.precise=1}};
        /* Keep the pointer in both content regions and the owning window. */
        f.pointer.y=original.y+110;
        editor_navigate(&small,f);f.piano_content.h=600;editor_navigate(&big,f);
        CHECK(small.active && big.active);
        CHECK(small.piano_top==big.piano_top);
        CHECK(fabsf(small.piano_remainder-big.piano_remainder)<.00001f);
    }
    PianoViewport enormous={692,2000,128,0,1,127,130};
    PianoGesture gesture={.pattern=0,.channel=0,.length=2};
    float end=project.pattern_steps[0];
    int result=piano_press(&gesture,&project,&enormous,(PianoPress){
        .pointer={60+100.f/128*(692-84),86+130*5.2f},.enabled=1,.left=1},NULL,NULL);
    CHECK(result==PIANO_PRESS_INACTIVE && project.pattern_steps[0]==end);
    puts("Piano resize/maximize/restore preserves row height, note/key hits and precise-scroll sensitivity at 1x/2x.");
    return 0;
}
