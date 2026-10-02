// SPDX-License-Identifier: GPL-3.0-only
#ifndef BROWSER_H
#define BROWSER_H
#include "raylib.h"
#include <limits.h>
#define FOLDERS 8
typedef struct { char *path; int depth,dir,open; } BrowserNode;
typedef struct {
    char path[PATH_MAX],samples[PATH_MAX],folders[FOLDERS][PATH_MAX],config[PATH_MAX];
    BrowserNode *nodes;
    int count,items,scroll,selected,follow;
} Browser;
void browser_init(Browser *b,const char *samples);
int browser_toggle(Browser *b,int entry);
void browser_select(Browser *b,int entry);
void browser_left(Browser *b);
void browser_right(Browser *b);
int browser_add(Browser *b,const char *path);
void browser_close(Browser *b);
#endif
