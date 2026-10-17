// SPDX-License-Identifier: GPL-3.0-only
/* Original main.c shortcut branch order and modifier distinctions. */
#include "shortcuts.h"
static void push(ShortcutActions *out,int kind,int argument){out->actions[out->count++]=(ShortcutAction){kind,argument};}
static int pressed(const AppInput *i,int key){return input_key_pressed(i,key);}
static int repeated(const AppInput *i,int key){return pressed(i,key) || input_key_repeat(i,key);}
ShortcutActions shortcut_actions(const AppInput *i,ShortcutContext c) {
    ShortcutActions out={0};int shift=input_key_down(i,APP_KEY_LEFT_SHIFT) || input_key_down(i,APP_KEY_RIGHT_SHIFT);
    int shortcut=c.mac?(input_key_down(i,APP_KEY_LEFT_SUPER) || input_key_down(i,APP_KEY_RIGHT_SUPER)):
        (input_key_down(i,APP_KEY_LEFT_CONTROL) || input_key_down(i,APP_KEY_RIGHT_CONTROL));
    if(!c.modal && !c.captured && shortcut && pressed(i,'Z'))push(&out,shift?SHORTCUT_REDO:SHORTCUT_UNDO,0);
    if(!c.mac && !c.modal && !c.captured && shortcut && pressed(i,'Y'))push(&out,SHORTCUT_REDO,0);
    if(!c.modal && !c.captured && !c.dragging && shortcut && pressed(i,'S'))push(&out,shift?SHORTCUT_SAVE_AS:SHORTCUT_SAVE,0);
    if(!c.modal && !c.captured && !c.dragging && shortcut && pressed(i,'O'))push(&out,SHORTCUT_OPEN,0);
    if(!c.modal && !c.captured && !c.dragging && !c.browser_focus && shortcut) {
        if(c.editor==2 && c.piano_tool==0 && (pressed(i,APP_KEY_UP) || pressed(i,'K')))push(&out,SHORTCUT_OCTAVE,1);
        else if(c.editor==2 && c.piano_tool==0 && (pressed(i,APP_KEY_DOWN) || pressed(i,'J')))push(&out,SHORTCUT_OCTAVE,-1);
        else if(pressed(i,'X'))push(&out,SHORTCUT_SELECTION,0);
        else if(pressed(i,'C'))push(&out,SHORTCUT_SELECTION,1);
        else if(pressed(i,'V'))push(&out,SHORTCUT_SELECTION,2);
        else if(pressed(i,'A'))push(&out,SHORTCUT_SELECTION,3);
    }
    if(!c.modal && !c.captured && !c.dragging && !c.browser_focus && !shortcut &&
        !input_key_down(i,APP_KEY_LEFT_ALT) && !input_key_down(i,APP_KEY_RIGHT_ALT) && shift) {
        if(repeated(i,APP_KEY_LEFT))push(&out,SHORTCUT_NUDGE,0);
        else if(repeated(i,APP_KEY_RIGHT))push(&out,SHORTCUT_NUDGE,1);
        else if(repeated(i,APP_KEY_UP))push(&out,SHORTCUT_NUDGE,2);
        else if(repeated(i,APP_KEY_DOWN))push(&out,SHORTCUT_NUDGE,3);
    }
    if(!c.modal && pressed(i,APP_KEY_SPACE))push(&out,SHORTCUT_TRANSPORT,0);
    return out;
}
