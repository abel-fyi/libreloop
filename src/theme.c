// SPDX-License-Identifier: GPL-3.0-only
#include "theme.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

Theme ui_theme;
static char config[PATH_MAX];
static const Theme themes[]={
    {
        .background={36,44,48,255}, .surface={62,74,79,255},
        .control={73,86,92,255}, .text={229,233,235,255},
        .secondary={177,188,193,255}, .highlight={159,218,76,255},
        .hover={89,102,108,255}, .active_hover={180,234,102,255},
        .selected_text={31,43,30,255}, .border={99,112,119,255},
        .title={78,88,94,255}, .title_focus={88,101,108,255},
        .disabled={66,76,81,255}, .step_alt={103,87,88,255},
        .track={49,63,67,255}, .clip={{105,121,128,255},{120,109,128,255}},
        .note_drag={191,231,150,255}, .piano_white={194,204,209,255},
        .patt={235,155,61,255}, .grid_major={79,94,102,255},
        .grid_minor={56,69,75,255},
        .signal={166,213,128,255}, .loop={225,77,72,255}, .meter_low={150,227,69,255},
        .meter_mid={255,174,40,255}, .meter_high={255,48,48,255},
        .rack={114,124,129,255}, .mixer={62,78,85,255}, .effects={99,116,121,255},
        .browser={32,41,46,255}, .note={151,207,170,255},
        .step_on={{184,203,212,255},{209,154,151,255}}, .waveform={222,204,181,255},
        .knob={128,255,0,255}, .swing={255,153,0,255},
        .pan_left={255,204,0,255}, .pan_right={255,51,0,255},
        .stereo={0,191,255,255}, .mono={153,0,255,255},
        .transparent=1
    },
    {
        .background={214,219,220,255}, .surface={235,238,238,255},
        .control={203,211,213,255}, .text={39,49,53,255},
        .secondary={85,99,105,255}, .highlight={143,190,83,255},
        .hover={186,199,203,255}, .active_hover={164,208,106,255},
        .selected_text={30,47,29,255}, .border={142,157,164,255},
        .title={193,204,207,255}, .title_focus={178,194,199,255},
        .disabled={220,225,226,255}, .step_alt={205,187,185,255},
        .track={224,230,231,255}, .clip={{175,192,196,255},{201,186,205,255}},
        .note_drag={164,200,124,255}, .piano_white={247,249,248,255},
        .patt={227,164,83,255}, .grid_major={161,176,181,255},
        .grid_minor={204,215,218,255},
        .signal={71,125,48,255}, .loop={190,53,46,255}, .meter_low={78,148,40,255},
        .meter_mid={198,125,18,255}, .meter_high={211,46,36,255},
        .rack={214,221,223,255}, .mixer={196,207,210,255}, .effects={219,226,227,255},
        .browser={231,235,236,255}, .note={127,181,145,255},
        .step_on={{111,146,162,255},{175,114,111,255}}, .waveform={94,86,70,255},
        .knob={71,153,0,255}, .swing={217,119,0,255},
        .pan_left={181,138,0,255}, .pan_right={224,51,0,255},
        .stereo={0,139,204,255}, .mono={136,0,221,255},
        .light=1, .transparent=1
    }
};

void theme_init(const char *browser_config) {
    ui_theme=themes[0]; config[0]=0;
    const char *slash=strrchr(browser_config,'/');
    if(!slash || slash-browser_config+11>=(int)sizeof config) return;
    snprintf(config,sizeof config,"%.*s/theme.txt",(int)(slash-browser_config),browser_config);
    FILE *f=fopen(config,"r"); int light,transparent;
    if(f) {
        int read=fscanf(f,"%d %d",&light,&transparent);
        if(read>=1 && (light==0 || light==1)) ui_theme=themes[light];
        if(read==2 && (transparent==0 || transparent==1)) ui_theme.transparent=transparent;
        fclose(f);
    }
}
static int save(void) {
    FILE *f=config[0]?fopen(config,"w"):NULL;
    if(!f) return 0;
    int ok=fprintf(f,"%d %d\n",ui_theme.light,ui_theme.transparent)>0;
    if(fclose(f)) ok=0;
    return ok;
}
int theme_select(int light) {
    int transparent=ui_theme.transparent;
    ui_theme=themes[!!light]; ui_theme.transparent=transparent;
    return save();
}
int theme_transparency(int enabled) { ui_theme.transparent=!!enabled; return save(); }
Color ui_glass(Color color,int opacity) { if(ui_theme.transparent) color.a=opacity; return color; }
void ui_surface(Rectangle rect,Color color) { DrawRectangleRec(rect,color); }
void ui_frame(Rectangle rect) {
    ui_surface(rect,ui_glass(ui_theme.surface,160));
    DrawRectangleLinesEx(rect,1,ui_theme.border);
}
