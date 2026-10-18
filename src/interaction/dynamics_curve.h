// SPDX-License-Identifier: GPL-3.0-only
#ifndef DYNAMICS_CURVE_EDIT_H
#define DYNAMICS_CURVE_EDIT_H
#include "dynamics.h"
/* Display-only: bipolar speed -1..1, center = normal; newest history at right. */
float dynamics_monitor_speed(float control);
float dynamics_monitor_position(unsigned age,float control);
float dynamics_meter_height(const DynamicsBand *band,float input_db,float position);
float dynamics_level_position(float db);
float dynamics_level_db(float position);
int dynamics_curve_insert(DynamicsBand *band,float input,float output);
int dynamics_curve_delete(DynamicsBand *band,int point);
void dynamics_curve_move(DynamicsBand *band,int point,float input,float output,int lock_x,int lock_y);
void dynamics_curve_bend(DynamicsBand *band,int segment,float bend);
#endif
