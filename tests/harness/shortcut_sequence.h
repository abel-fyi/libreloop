// SPDX-License-Identifier: GPL-3.0-only
static int script_keys[512],script_previous[512],script_repeat[512];
static void script_press(int code){script_keys[code]=1;}
static void shortcut_sequence(int mode,int mac,int *modal,int *capture,int *drag,int *browser,int *editor,int *tool) {
    memset(script_keys,0,sizeof script_keys);memset(script_previous,0,sizeof script_previous);memset(script_repeat,0,sizeof script_repeat);
    *modal=*capture=*drag=*browser=0;*editor=2;*tool=0;
    script_press(mac?343:341);
    if(mode<6){const char *keys="ZYSOA ";for(const char *p=keys;*p;p++)script_press(*p);}
    if(mode==1)script_press(340);if(mode==2)*browser=1;if(mode==3)*modal=1;if(mode==4)*capture=1;if(mode==5)*drag=1;
    if(mode==6){script_press('X');script_press('C');script_press('V');script_press('A');}
    if(mode==7){script_press(265);script_press('K');script_press('J');script_press('C');}
    if(mode==8){script_press(264);script_press('J');script_press('V');}
    if(mode==9){*editor=0;script_press(265);script_press('A');}
    if(mode==10){*tool=2;script_press(265);script_press('C');}
    if(mode>=11){memset(script_keys,0,sizeof script_keys);script_press(340);script_press(32);script_press(263);script_press(262);script_press(265);}
    if(mode==12){script_previous[263]=1;script_repeat[263]=1;}
    if(mode==13)script_press(342);if(mode==14)*capture=1;if(mode==15)*drag=1;if(mode==16)*browser=1;
    if(mode==17)script_press(341);if(mode==18)script_press(343);if(mode==19)*modal=1;
}
