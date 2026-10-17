// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_MENU_NAVIGATION_H
#define LIBRELOOP_MENU_NAVIGATION_H
#include "input.h"
typedef struct {float x,y,width,height;} MenuBounds;
typedef struct {
    int id,selected,count,items,active,enter;
    InputPoint pointer;
    MenuBounds bounds[128];
} MenuNavigation;
typedef struct {InputPoint pointer;int mouse_pressed,next,previous,enter;} MenuNavigationInput;
void menu_navigation_begin(MenuNavigation *menu,int id,int opened,int default_item,
    int *scroll,int total,MenuNavigationInput input);
void menu_navigation_end(MenuNavigation *menu);
#endif
