// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_ICONS_H
#define LIBRELOOP_ICONS_H
#include "draw.h"
#include "widget_pixels.h"
enum { ICON_RACK,ICON_PLAYLIST,ICON_PIANO,ICON_MIXER,ICON_PENCIL,ICON_BRUSH,ICON_SELECT,ICON_METRO,ICON_PLAY,ICON_STOP,ICON_PAUSE,ICON_CLOSE,ICON_BACK,ICON_FOLLOW,ICON_KEYS,ICON_WAVE,ICON_AUTOMATION,ICON_CUT,ICON_STRETCH,ICON_ROW_ZOOM,ICON_COUNT };
/* Keep the original atlas count stable for frozen reference fixtures. */
enum { ICON_FILE=ICON_COUNT,ICON_VIEW,ICON_HELP,ICON_SAVE,ICON_NEW,ICON_UNDO,ICON_REDO,ICON_COPY,ICON_PASTE,ICON_DELETE,ICON_SETTINGS,ICON_CHECK,ICON_ADD,ICON_RESET,ICON_UP,ICON_HOME,ICON_FX,ICON_LIBRARY,ICON_SPEAKER,ICON_SOLO,ICON_CLASSIC_COUNT };
int icon_action(const char *caption);
void icon_shapes(DrawContext *draw,int id);
typedef struct { DrawTexture *texture;int large,medium,small; } UIIcons;
WidgetPixels icons_pack(const DrawColor *input,int high,int large,int medium,int small);
int icons_build(DrawContext *draw,float density,UIIcons *icons);
int icons_build_classic(DrawContext *draw,float density,UIIcons *icons);
void icons_destroy(UIIcons *icons);
#endif
