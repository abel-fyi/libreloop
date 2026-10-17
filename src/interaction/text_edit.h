// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_TEXT_EDIT_H
#define LIBRELOOP_TEXT_EDIT_H
#include "input.h"
#include <stddef.h>
typedef struct {
    int select_all,paste,backspace,arrow,command_down;
    const char *clipboard;
    AppInput *input;
} TextEditInput;
/* Original popup text editing: selection is whole-string, with no caret. */
void text_edit(char *text,size_t capacity,int *selected,TextEditInput event);
void text_backspace(char *text);
/* Original Apply validation. Failed parsing leaves the target untouched. */
int text_number_parse(const char *text,float low,float high,int integer,float *target);
/* Trims ASCII spaces in-place; returns the first name byte, or NULL if empty. */
const char *text_rename_name(char *text);
#endif
