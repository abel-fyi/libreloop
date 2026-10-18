// SPDX-License-Identifier: GPL-3.0-only
#include "studio_layout.h"
#include "mixer_view.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static int overlap(Rect a,Rect b){return a.x<b.x+b.w && b.x<a.x+a.w && a.y<b.y+b.h && b.y<a.y+a.h;}
int main(void) {
    Windows w;windows_init(&w,1448,1086);studio_workspace_init(&w,1448,1086,238);
    CHECK(w.editors[1].visible && !w.editors[0].visible && !w.editors[2].visible && !w.editors[4].visible);
    CHECK(w.editors[1].rect.h==534 && w.editors[3].rect.h==462);
    /* Every supported startup size preserves legible meter travel, including
       the three-row FX shelf. Geometry is allowed to consume more of a small
       arrangement viewport to keep the default mixer readable. */
    for(int height=562;height<=1200;height+=11)for(int rows=0;rows<=3;rows++){
        Windows startup;windows_init(&startup,900,height);studio_workspace_init(&startup,900,height,238);
        startup.workspace.mixer_fx_rows=rows;studio_workspace_layout(&startup,900,height,238);
        MixerView v=mixer_view_rows(startup.editors[3].rect.w,startup.editors[3].rect.h-6,8,0,0,startup.workspace.mixer_fx_rows);
        CHECK(v.fader_bottom-v.fader_top>=60-.001f);
        if(height>=740)CHECK(v.fader_bottom-v.fader_top>=244-.001f);
        CHECK(startup.editors[1].rect.h>=122 && !overlap(startup.editors[1].rect,startup.editors[3].rect));
    }
    Rect empty_mixer=w.editors[3].rect;
    for(int rows=1;rows<=3;rows++) {
        w.workspace.mixer_fx_rows=rows;studio_workspace_layout(&w,1448,1086,238);
        CHECK(w.editors[3].rect.h==empty_mixer.h+rows*22 && w.editors[3].rect.y==empty_mixer.y-rows*22);
    }
    w.workspace.mixer_fx_rows=0;studio_workspace_layout(&w,1448,1086,238);
    Rect page=w.editors[1].rect,desktop={244,60,1200,1002};
    windows_focus(&w,0);CHECK(w.focused==2 && w.editors[2].visible && !w.editors[1].visible && !w.editors[0].visible);
    CHECK(!memcmp(&page,&w.editors[2].rect,sizeof page));
    CHECK(studio_workspace_split(&w,page.x+100,page.y-3)==-1);
    studio_workspace_hide(&w,2);studio_workspace_maximize(&w,2);CHECK(w.editors[2].visible && !w.editors[2].maximized);
    windows_focus(&w,1);CHECK(w.editors[1].visible && !w.editors[2].visible);
    CHECK(!memcmp(&page,&w.editors[1].rect,sizeof page));
    windows_focus(&w,4);CHECK(w.editors[4].visible && w.workspace.floating[4]);Rect instrument=w.editors[4].rect;
    /* Move the floating tool away from the divider before testing resize. */
    w.editors[4].rect.x=800;w.editors[4].rect.y=80;w.editors[4].rect.h=w.editors[4].minh;
    instrument=w.editors[4].rect;float y=w.editors[3].rect.y-3;
    CHECK(studio_workspace_split(&w,500,y)==3);
    windows_update(&w,desktop,500,y,1,1,1);windows_update(&w,desktop,500,y-100,0,1,1.1);
    CHECK(w.editors[3].rect.h==562 && w.editors[1].rect.h==434);
    CHECK(!memcmp(&instrument,&w.editors[4].rect,sizeof instrument));
    windows_update(&w,desktop,500,y-100,0,0,1.2);CHECK(w.workspace.drag==-1);
    y=w.editors[3].rect.y-3;
    windows_update(&w,desktop,500,y,1,1,3);windows_update(&w,desktop,500,y,0,0,3.05);
    windows_update(&w,desktop,500,y,1,1,3.2);CHECK(w.editors[3].rect.h==462);
    windows_update(&w,desktop,500,y,0,0,3.3);
    /* Only close remains; former minimize/maximize positions do not hide/zoom. */
    studio_workspace_maximize(&w,4);CHECK(!w.editors[4].maximized);
    windows_focus(&w,5);CHECK(w.editors[5].visible && w.editors[4].visible);
    windows_focus(&w,2);CHECK(w.focused==2 && w.order[WINDOW_CAPACITY-1]>=4);
    studio_workspace_hide(&w,5);CHECK(!w.editors[5].visible && w.editors[4].visible);
    studio_workspace_hide(&w,4);
    for(int width=900;width<=1920;width+=102)for(int height=562;height<=1086;height+=131)for(int id=1;id<=2;id++) {
        studio_workspace_layout(&w,width,height,238);windows_focus(&w,id);
        CHECK(w.editors[id].visible && !w.editors[id==1?2:1].visible && !w.editors[0].visible);
        Rect r=w.editors[id].rect;CHECK(r.x==244 && r.y==60 && r.w==width-248 && r.h>=122);
        CHECK(!overlap(r,w.editors[3].rect));
        CHECK(r.y+r.h<=w.editors[3].rect.y-6+.01f);
    }
    studio_workspace_hide(&w,3);CHECK(!w.editors[3].visible);
    windows_focus(&w,3);CHECK(w.editors[3].visible);
    /* Floating tools normalize stale saved sizes and never capture corners. */
    studio_workspace_layout(&w,1448,1086,238);windows_focus(&w,4);
    w.editors[4].rect=(Rect){800,80,600,700};
    windows_update(&w,desktop,810,120,0,0,8);
    CHECK(w.editors[4].rect.w==440 && w.editors[4].rect.h==420);
    Rect fixed=w.editors[4].rect;
    windows_update(&w,desktop,fixed.x+fixed.w-3,fixed.y+fixed.h-3,1,1,8.1);
    CHECK(w.grab==-1 && !w.resize);
    windows_update(&w,desktop,fixed.x+fixed.w+50,fixed.y+fixed.h+50,0,1,8.2);
    CHECK(!memcmp(&fixed,&w.editors[4].rect,sizeof fixed));
    windows_update(&w,desktop,810,120,0,0,8.3);
    /* Caption sizing changes geometry, not the legacy controller coordinates. */
    Windows chrome;windows_init(&chrome,1448,1086);
    CHECK(windows_title_height(&chrome)==18);
    Rect original=chrome.editors[4].rect,content=windows_content_frame(&chrome,4);
    CHECK(!memcmp(&original,&content,sizeof content));
    studio_workspace_init(&chrome,1448,1086,238);windows_focus(&chrome,4);
    original=chrome.editors[4].rect;content=windows_content_frame(&chrome,4);
    CHECK(windows_title_height(&chrome)==24 && content.y==original.y+6 && content.h==original.h-6);
    CHECK(content.y+content.h==original.y+original.h && content.w==original.w);
    CHECK(!windows_chrome_hit(&chrome,4,original.x+80,original.y+24,1).title);
    WindowChromeHit hit=windows_chrome_hit(&chrome,4,original.x+80,original.y+20,1);
    CHECK(hit.title && hit.slot==-1);
    windows_update(&chrome,desktop,original.x+80,original.y+20,1,1,10);
    CHECK(chrome.grab==4 && !chrome.resize);
    windows_update(&chrome,desktop,original.x+80,original.y+20,0,0,10.1);
    hit=windows_chrome_hit(&chrome,4,original.x+original.w-20,original.y+20,1);
    CHECK(hit.title && hit.slot==2);
    windows_update(&chrome,desktop,original.x+original.w-20,original.y+20,1,1,11);
    CHECK(!chrome.editors[4].visible);
    puts("Exclusive Arrangement/Pattern pages, docked Mixer, floating editors and resize invariants passed.");return 0;
}
