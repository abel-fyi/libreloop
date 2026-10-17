// SPDX-License-Identifier: GPL-3.0-only
#include "../../src/ui/frontend/ui_internal.h"
UiState ui;
static void menu_setup(void);
#define menu_point(x,y) ((DrawPoint){x,y})
#define menu_begin_fixture menu_keys_begin
#define menu_end_fixture menu_keys_end
#include "menu_navigation_sequence.h"
int host_is_key_pressed(int k){return menu_pressed[k];}
int host_is_key_pressed_repeat(int k){return menu_repeated[k];}
int mouse_pressed(int button){return button==0 && menu_mouse_pressed;}
#include "../../src/ui/frontend/ui_widgets.c"
static void menu_setup(void){memset(&ui.menu_keys,0,sizeof ui.menu_keys);ui.menu_keys.id=4;ui.menu_keys.count=4;ui.menu_keys.selected=1;ui.mouse=(DrawPoint){0,0};for(int i=0;i<4;i++)ui.menu_keys.bounds[i]=(MenuBounds){10,20+i*10,80,10};}
int main(void){FILE *f=fopen("menu.txt","w");if(!f)return 1;menu_navigation_sequence(f);return fclose(f)!=0;}
