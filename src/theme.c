// SPDX-License-Identifier: GPL-3.0-only
#include "theme.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

Theme ui_theme;
static char config[PATH_MAX];
static unsigned accent_choice;
static const Theme dark_theme={
        .background={33,33,33,255}, .surface={84,84,84,255},
        .control={48,48,48,255}, .text={232,232,232,255},
        .secondary={186,186,186,255}, .highlight={239,168,80,255},
        .hover={98,98,98,255}, .active_hover={250,186,109,255},
        .selected_text={35,30,25,255}, .border={110,110,110,255},
        .title={84,84,84,255}, .title_focus={94,94,94,255},
        .disabled={74,74,74,255}, .step_alt={100,91,83,255},
        .track={65,65,65,255}, .clip={{118,118,118,255},{113,113,113,255}},
        .note_drag={231,214,190,255}, .piano_white={195,195,195,255},
        .patt={239,168,80,255}, .grid_major={43,43,43,255},
        .grid_minor={55,55,55,255},
        .piano_black={74,74,74,255}, .piano_c={167,167,167,255},
        .piano_row={{65,65,65,255},{59,59,59,255}},
        .signal={239,168,80,255}, .loop={225,77,72,255}, .meter_low={151,181,130,255},
        .meter_mid={221,180,106,255}, .meter_high={224,83,75,255},
        .rack={84,84,84,255}, .mixer={84,84,84,255}, .effects={84,84,84,255},
        .browser={38,38,38,255}, .note={190,190,190,255},
        .step_on={{190,190,190,255},{201,174,149,255}}, .waveform={206,206,206,255},
        .fader={195,195,195,255}, .fader_mark={33,33,33,255},
        .knob={206,206,206,255}, .swing={206,206,206,255},
        .pan_left={204,164,110,255}, .pan_right={205,117,105,255},
        .stereo={143,174,192,255}, .mono={173,150,187,255},
};

/* Derive light chrome from the same hierarchy so surfaces cannot drift apart. */
static Theme palette(int light) {
    Theme theme=dark_theme; theme.light=!!light;
    if(light) {
        Color *inverted[]={&theme.background,&theme.surface,&theme.control,&theme.text,
            &theme.secondary,&theme.hover,&theme.border,&theme.title,&theme.title_focus,
            &theme.disabled,&theme.track,&theme.grid_major,&theme.grid_minor,
            &theme.piano_row[0],&theme.piano_row[1],&theme.rack,&theme.mixer,
            &theme.effects,&theme.browser,&theme.note,&theme.waveform,&theme.swing,
            &theme.fader,&theme.fader_mark};
        for(unsigned i=0;i<sizeof inverted/sizeof *inverted;i++) {
            Color *color=inverted[i]; color->r=255-color->r; color->g=255-color->g; color->b=255-color->b;
        }
        Color *indicators[]={&theme.pan_left,&theme.pan_right,&theme.stereo,&theme.mono,
            &theme.meter_low,&theme.meter_mid,&theme.meter_high};
        for(unsigned i=0;i<sizeof indicators/sizeof *indicators;i++) *indicators[i]=ColorBrightness(*indicators[i],-.45f);
        /* Piano key identities and semantic hues survive a theme switch. */
        theme.piano_white=(Color){235,235,235,255}; theme.piano_black=(Color){64,64,64,255};
        theme.piano_c=(Color){207,207,207,255}; theme.step_alt=(Color){163,163,163,255};
        theme.step_on[0]=(Color){65,65,65,255}; theme.step_on[1]=(Color){106,81,54,255};
    }
    return theme;
}

static void apply_accent(void) {
    unsigned rgb=accent_choice?accent_choice:0x516389;
    Color color={rgb>>16,(rgb>>8)&255,rgb&255,255};
    ui_theme.highlight=color;
    ui_theme.knob=ui_theme.light?ColorBrightness(color,-.5f):color;
    ui_theme.active_hover=ColorBrightness(color,.12f);
    ui_theme.signal=ui_theme.light?ColorBrightness(color,-.45f):color;
    ui_theme.patt=color;
    ui_theme.note_drag=ColorBrightness(color,.15f);
    ui_theme.selected_text=theme_foreground(color);
}
void theme_init(const char *browser_config) {
    ui_theme=palette(0); config[0]=0; accent_choice=0;
    apply_accent();
    const char *slash=strrchr(browser_config,'/');
    if(!slash || slash-browser_config+11>=(int)sizeof config) return;
    snprintf(config,sizeof config,"%.*s/theme.txt",(int)(slash-browser_config),browser_config);
    FILE *f=fopen(config,"r"); int light;
    if(f) {
        unsigned accent=0;
        int read=fscanf(f,"%d %x",&light,&accent);
        if(read>=1 && (light==0 || light==1)) ui_theme=palette(light);
        /* Old files may contain the removed transparency flag as 0 or 1. */
        if(read==2 && accent>1 && accent<=0xffffff) accent_choice=accent;
        apply_accent();
        fclose(f);
    }
}
static int save(void) {
    FILE *f=config[0]?fopen(config,"w"):NULL;
    if(!f) return 0;
    int ok=fprintf(f,"%d %06x\n",ui_theme.light,accent_choice)>0;
    if(fclose(f)) ok=0;
    return ok;
}
int theme_select(int light) {
    ui_theme=palette(light);
    apply_accent();
    return save();
}
int theme_accent(unsigned rgb) {
    if(rgb>0xffffff) return 0;
    accent_choice=rgb; ui_theme=palette(ui_theme.light); apply_accent();
    return save();
}
void ui_surface(Rectangle rect,Color color) { DrawRectangleRec(rect,color); }
void ui_frame(Rectangle rect) {
    ui_surface(rect,ui_theme.surface);
    DrawRectangleLinesEx(rect,1,ui_theme.border);
}

Color theme_foreground(Color background) {
    float channels[]={background.r/255.f,background.g/255.f,background.b/255.f};
    for(int i=0;i<3;i++) channels[i]=channels[i]<=.04045f?channels[i]/12.92f:powf((channels[i]+.055f)/1.055f,2.4f);
    float luminance=.2126f*channels[0]+.7152f*channels[1]+.0722f*channels[2];
    return (luminance+.05f)/.05f >= 1.05f/(luminance+.05f)?BLACK:WHITE;
}
void ui_fader_handle(Rectangle rect,int selected) {
    DrawRectangleRec(rect,selected?ui_theme.highlight:ui_theme.fader);
    DrawLine(rect.x+2,rect.y+rect.height/2,rect.x+rect.width-2,rect.y+rect.height/2,
        selected?ui_theme.selected_text:ui_theme.fader_mark);
}
