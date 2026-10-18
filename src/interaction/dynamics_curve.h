// SPDX-License-Identifier: GPL-3.0-only
#ifndef DYNAMICS_CURVE_EDIT_H
#define DYNAMICS_CURVE_EDIT_H
#include "dynamics.h"
float dynamics_level_position(float db);
float dynamics_level_db(float position);
int dynamics_curve_insert(DynamicsBand *band,float input,float output);
int dynamics_curve_delete(DynamicsBand *band,int point);
void dynamics_curve_move(DynamicsBand *band,int point,float input,float output,int lock_x,int lock_y);
void dynamics_curve_bend(DynamicsBand *band,int segment,float bend);
#endif
