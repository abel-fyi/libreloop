// SPDX-License-Identifier: GPL-3.0-only
static int menu_pressed[512],menu_repeated[512],menu_mouse_pressed;
static void menu_navigation_sequence(FILE *out) {
    for(int mode=0;mode<32;mode++) {
        memset(menu_pressed,0,sizeof menu_pressed);memset(menu_repeated,0,sizeof menu_repeated);menu_mouse_pressed=0;
        menu_setup();
        int id=4,opened=0,scroll=2,total=9,*scroll_pointer=&scroll;
        if(mode<8) { menu_pressed[264]=1;ui.menu_keys.selected=mode%4; if(mode>=4)scroll_pointer=NULL; }
        else if(mode<16) { menu_pressed[265]=1;ui.menu_keys.selected=mode%4; if(mode>=12)scroll_pointer=NULL; }
        else if(mode==16) menu_pressed[264]=menu_pressed[265]=1;
        else if(mode==17) {menu_pressed[264]=1;opened=1;}
        else if(mode==18) {menu_pressed[264]=1;id=5;}
        else if(mode==19) {menu_repeated[264]=1;}
        else if(mode==20) menu_pressed['J']=1;
        else if(mode==21) menu_pressed['K']=1;
        else if(mode==22) menu_pressed['L']=1;
        else if(mode==23) menu_pressed['H']=1;
        else if(mode==24) menu_pressed[257]=1;
        else if(mode==25) menu_pressed[335]=1;
        else if(mode==26) menu_repeated[257]=1;
        else if(mode==27) ui.mouse=menu_point(10,40);
        else if(mode==28) ui.mouse=menu_point(90,40); /* exclusive right edge */
        else if(mode==29) {ui.menu_keys.pointer.x=10;ui.menu_keys.pointer.y=40;ui.mouse=menu_point(10,40);menu_mouse_pressed=1;}
        else if(mode==30) {menu_pressed[264]=1;ui.menu_keys.selected=3;scroll=5;}
        else {menu_pressed[265]=1;ui.menu_keys.selected=0;scroll=0;total=3;}
        menu_begin_fixture(id,opened,2,scroll_pointer,total);
        fprintf(out,"%d begin %d %d %d %d %d %d %d %.9g %.9g\n",mode,ui.menu_keys.id,ui.menu_keys.selected,ui.menu_keys.count,ui.menu_keys.items,ui.menu_keys.active,ui.menu_keys.enter,scroll,ui.menu_keys.pointer.x,ui.menu_keys.pointer.y);
        ui.menu_keys.items=mode==18?0:mode==19?129:mode==20?1:4;
        menu_end_fixture();
        fprintf(out,"%d end %d %d %d %d %d %d\n",mode,ui.menu_keys.id,ui.menu_keys.selected,ui.menu_keys.count,ui.menu_keys.items,ui.menu_keys.active,ui.menu_keys.enter);
    }
}
