// SPDX-License-Identifier: GPL-3.0-only
#include "theme.h"
#include <math.h>

Theme ui_theme;
/* Website colors: violet-black #09070f, lavender, pink and pale lime. */
static const Theme dark_theme={
        .background={9,7,15,255}, .surface={30,25,41,255},
        .control={20,16,29,255}, .text={238,238,255,255},
        .secondary={170,153,187,255}, .highlight={179,154,221,255},
        .hover={179,154,221,255}, .active_hover={204,187,238,255},
        .selected_text={35,24,49,255}, .border={119,102,136,255},
        .title={28,23,39,255}, .title_focus={43,34,57,255},
        .disabled={49,40,62,255}, .step_alt={43,34,57,255},
        .track={24,19,34,255}, .clip={{78,62,95,255},{69,55,87,255}},
        .note_drag={179,154,221,255}, .piano_white={218,204,234,255},
        .patt={179,154,221,255}, .grid_major={61,47,79,255},
        .grid_minor={40,31,53,255},
        .piano_black={49,40,62,255}, .piano_c={179,154,221,255},
        .piano_row={{23,19,33,255},{30,24,42,255}},
        .signal={187,255,85,255}, .loop={255,68,204,255},
        .meter_low={187,255,85,255}, .meter_mid={255,193,48,255}, .meter_high={255,74,100,255},
        .rack={30,25,41,255}, .mixer={30,25,41,255}, .effects={30,25,41,255},
        .browser={16,12,23,255}, .note={153,136,170,255},
        .step_on={{153,136,170,255},{140,160,122,255}}, .waveform={153,136,170,255},
        .fader={153,153,170,255}, .fader_mark={9,7,15,255},
        .knob={136,128,136,255}, .knob_track={112,104,112,255}, .swing={187,255,85,255},
        .pan_left={187,255,85,255}, .pan_right={255,68,204,255},
        .stereo={187,255,85,255}, .mono={255,68,204,255},
};

void theme_init(void) {
    ui_theme=dark_theme;
    ui_theme.selected_text=theme_foreground(ui_theme.highlight);
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
