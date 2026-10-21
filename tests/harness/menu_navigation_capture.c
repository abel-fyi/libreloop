// SPDX-License-Identifier: GPL-3.0-only
#include "menu_navigation.h"
#include <stdio.h>
#include <string.h>
static struct {MenuNavigation menu_keys;InputPoint mouse;} ui;
static void menu_setup(void);
static void menu_begin_fixture(int id,int opened,int default_item,int *scroll,int total);
static void menu_end_fixture(void){menu_navigation_end(&ui.menu_keys);}
#define menu_point(x,y) ((InputPoint){x,y})
#include "menu_navigation_sequence.h"
static int key(int k){return menu_pressed[k] || menu_repeated[k];}
static void menu_setup(void){memset(&ui.menu_keys,0,sizeof ui.menu_keys);ui.menu_keys.id=4;ui.menu_keys.count=4;ui.menu_keys.selected=1;ui.mouse=(InputPoint){0,0};for(int i=0;i<4;i++)ui.menu_keys.bounds[i]=(MenuBounds){10,20+i*10,80,10};}
static void menu_begin_fixture(int id,int opened,int default_item,int *scroll,int total) {
    menu_navigation_begin(&ui.menu_keys,id,opened,default_item,scroll,total,(MenuNavigationInput){
        .pointer=ui.mouse,.mouse_pressed=menu_mouse_pressed,
        .next=key(264) || key('J') || key(262) || key('L'),
        .previous=key(265) || key('K') || key(263) || key('H'),
        .enter=menu_pressed[257] || menu_pressed[335]});
}
int main(void){FILE *f=fopen("menu.txt","wb");if(!f)return 1;menu_navigation_sequence(f);return fclose(f)!=0;}
