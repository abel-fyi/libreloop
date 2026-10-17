// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_SHORTCUTS_H
#define LIBRELOOP_SHORTCUTS_H
#include "input.h"
enum { SHORTCUT_UNDO,SHORTCUT_REDO,SHORTCUT_SAVE,SHORTCUT_SAVE_AS,SHORTCUT_OPEN,
    SHORTCUT_SELECTION,SHORTCUT_OCTAVE,SHORTCUT_NUDGE,SHORTCUT_TRANSPORT };
typedef struct {int kind,argument;} ShortcutAction;
typedef struct {ShortcutAction actions[8];int count;} ShortcutActions;
typedef struct {int mac,modal,captured,dragging,browser_focus,editor,piano_tool;} ShortcutContext;
/* Concrete original main-loop bindings, in original branch order. */
ShortcutActions shortcut_actions(const AppInput *input,ShortcutContext context);
#endif
