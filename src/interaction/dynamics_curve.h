// SPDX-License-Identifier: GPL-3.0-only
#ifndef DYNAMICS_CURVE_EDIT_H
#define DYNAMICS_CURVE_EDIT_H
#include "dynamics.h"
/* Display-only: bipolar speed -1..1, center = 0.85x; newest history at right. */
float dynamics_monitor_speed(float control);
void dynamics_band_bounds(float low,float high,float lowcut,float bounds[4]);
float dynamics_monitor_position(unsigned age,float control);
float dynamics_recent_input_envelope(const DynamicsMonitor *monitor,int band,unsigned samples);
float dynamics_meter_height(const DynamicsBand *band,float input_db,float position);
float dynamics_level_position(float db);
float dynamics_level_db(float position);
float dynamics_curve_handle_input(const DynamicsBand *band,int segment);
int dynamics_curve_insert(DynamicsBand *band,float input,float output);
int dynamics_curve_delete(DynamicsBand *band,int point);
void dynamics_curve_move(DynamicsBand *band,int point,float input,float output,int lock_x,int lock_y);
const float *dynamics_grid_values(unsigned *count);
float dynamics_grid_snap(float db);
void dynamics_curve_move_grid(DynamicsBand *band,int point,float input,float output,int lock_x,int lock_y,int magnet);
void dynamics_curve_bend(DynamicsBand *band,int segment,float bend);
#endif
