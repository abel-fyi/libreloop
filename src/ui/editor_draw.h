// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_EDITOR_DRAW_H
#define LIBRELOOP_EDITOR_DRAW_H
#include "render.h"
#include "editor_windows.h"
typedef struct { const Windows *windows;int id,hover_slot,fm_instrument; } EditorDrawing;
void editor_draw_begin(UIRender *render,WindowGeometry geometry,const EditorDrawing *state);
void editor_draw_end(UIRender *render,const EditorDrawing *state);
#endif
