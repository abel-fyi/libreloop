// SPDX-License-Identifier: GPL-3.0-only
/* Original ui_widgets.c menu_keys_begin/end, with input supplied by the caller. */
#include "menu_navigation.h"
#include <math.h>
void menu_navigation_begin(MenuNavigation *menu,int id,int opened,int default_item,
    int *scroll,int total,MenuNavigationInput input) {
    if(menu->id!=id || opened) {
        menu->id=id;menu->selected=default_item;menu->count=0;menu->pointer=input.pointer;
    }
    int moved=input.pointer.x!=menu->pointer.x || input.pointer.y!=menu->pointer.y;
    if(moved || input.mouse_pressed)
        for(int i=0;i<menu->count;i++) {
            MenuBounds r=menu->bounds[i];
            if(input.pointer.x>=r.x && input.pointer.x<r.x+r.width && input.pointer.y>=r.y && input.pointer.y<r.y+r.height) {
                menu->selected=i;break;
            }
        }
    menu->pointer=input.pointer;menu->active=1;menu->items=0;menu->enter=0;
    int next=input.next,previous=input.previous;
    if(!opened && menu->count>0) {
        if(next!=previous) {
            if(scroll && next && menu->selected==menu->count-1 && *scroll+menu->count<total) ++*scroll;
            else if(scroll && previous && menu->selected==0 && *scroll>0) --*scroll;
            else {
                if(scroll && next && menu->selected==menu->count-1) *scroll=0;
                if(scroll && previous && menu->selected==0) *scroll=fmaxf(0,total-menu->count);
                menu->selected=(menu->selected+(next?1:menu->count-1))%menu->count;
            }
        }
        menu->enter=input.enter;
    }
}
void menu_navigation_end(MenuNavigation *menu) {
    menu->count=menu->items<128?menu->items:128;menu->active=0;menu->enter=0;
    if(menu->count>0 && menu->selected>=menu->count) menu->selected=menu->count-1;
}
