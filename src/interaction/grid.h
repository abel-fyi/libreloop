// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_GRID_H
#define LIBRELOOP_GRID_H
#include "engine.h"
enum { GRID_AUTO,GRID_NONE,GRID_BAR,GRID_HALF,GRID_QUARTER,GRID_EIGHTH,GRID_SIXTEENTH,GRID_32,GRID_64,GRID_EIGHTH_TRIPLET,GRID_SIXTEENTH_TRIPLET,GRID_CHOICES };
const char *grid_choice_name(int choice);
float grid_choice_interval(int choice,float pixels_per_step);
TimelineGrid grid_choice_layout(int choice,float pixels_per_step);
#endif
