// SPDX-License-Identifier: GPL-3.0-only
#include "rack.h"
#include "editor_navigation.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <string.h>
static Project project;
int main(void) {
    for(int density=1;density<=2;density++) {
        Windows windows;windows_init(&windows,1448,1086);
        for(int i=0;i<EDITORS;i++)windows.editors[i].visible=i==0;
        windows.editors[0].rect=(Rect){100,100,800,360};windows_focus(&windows,0);
        Arrangement arrangement={0};
        EditorNavigation s={.windows=&windows,.arrangement=&arrangement,.drag=-1,.rack_enabled=1,.rack_count=32};
        s.rack_view[0]=20;
        NavigationFrame f={.pointer={100+200+574*.37f,170},.layout_scale=density,.zoom_modifier=1,.scroll={.y=1}};
        float anchor=20+.37f*574/16;
        editor_navigate(&s,f);
        assert(s.active && s.rack_width[0]>16 && fabsf(s.rack_view[0]+.37f*574/s.rack_width[0]-anchor)<.0001f);
        f.scroll.y=-1;editor_navigate(&s,f);
        assert(fabsf(s.rack_width[0]-16)<.0001f && fabsf(s.rack_view[0]-20)<.0001f);
        /* Native pinch and precise modifier-wheel use the same zoom mapping. */
        f.scroll=(NavigationInput){.zoom=.25f};f.zoom_modifier=0;
        editor_navigate(&s,f);float pinch_width=s.rack_width[0];
        s.rack_width[0]=16;s.rack_view[0]=20;f.scroll=(NavigationInput){.y=5,.precise=1};f.zoom_modifier=1;
        editor_navigate(&s,f);assert(s.rack_width[0]==pinch_width);
        assert(fabsf(s.rack_view[0]+.37f*574/s.rack_width[0]-anchor)<.0001f);
        f.zoom_modifier=0;f.scroll=(NavigationInput){.y=-1};editor_navigate(&s,f);assert(s.rack_scroll==1);
        f.shift=1;float before=s.rack_view[0];editor_navigate(&s,f);assert(s.rack_view[0]>before && s.rack_scroll==1);
        f.shift=0;f.scroll=(NavigationInput){.x=-3,.y=-14,.precise=1};before=s.rack_view[0];editor_navigate(&s,f);
        assert(fabsf(s.rack_view[0]-before-3.f/density/s.rack_width[0])<.0001f);
        assert(fabsf(s.rack_scroll-(1+.5f/density))<.0001f);
        float width=s.rack_width[0],view=s.rack_view[0];f.blocked=1;f.scroll=(NavigationInput){.zoom=5};editor_navigate(&s,f);
        assert(s.rack_width[0]==width && s.rack_view[0]==view && !s.active);
        f.blocked=0;f.pointer.x=130;editor_navigate(&s,f);assert(s.rack_width[0]==width);
        f.pointer.x=500;f.middle_pressed=f.middle_held=1;f.scroll=(NavigationInput){0};editor_navigate(&s,f);
        f.middle_pressed=0;f.pointer.x-=32;editor_navigate(&s,f);assert(s.drag==0 && s.pan_cursor && s.rack_view[0]>view);
        f.middle_held=0;f.blocked=1;editor_navigate(&s,f);assert(s.drag==-1);
        s.pattern=1;f.blocked=0;f.scroll=(NavigationInput){.zoom=1};editor_navigate(&s,f);assert(s.rack_width[0]==width && s.rack_width[1]>16);
    }
    float width=16,view=100;
    rack_zoom(&width,&view,574,10000,.5f);assert(width==128 && view>=0);
    rack_zoom(&width,&view,574,-10000,.5f);assert(width>=4 && width<=128 && isfinite(view));
    for(int step_width=4;step_width<=128;step_width*=2) {
        project_new(&project);project.channel_count=1;
        RackState s={.view=7.25f,.step_width=step_width,.paint_channel=-1};
        rack_paint(&s,&project,1600,360,200+(10.5f-s.view)*step_width,40);
        assert(note_at(&project,0,0,10,60));
        rack_paint(&s,&project,1600,360,200+(12.5f-s.view)*step_width,40);
        assert(note_at(&project,0,0,11,60) && note_at(&project,0,0,12,60));
        s.paint=1;s.paint_channel=-1;
        rack_paint(&s,&project,1600,360,200+(11.5f-s.view)*step_width,40);
        assert(!note_at(&project,0,0,11,60));
        assert(note_at(&project,0,0,10,60) && note_at(&project,0,0,12,60));
    }
    return 0;
}
