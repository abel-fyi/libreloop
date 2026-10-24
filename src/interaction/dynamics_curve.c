// SPDX-License-Identifier: GPL-3.0-only
#include "dynamics_curve.h"
#include <math.h>
#include <string.h>
static float clamp(float x,float a,float b){return fmaxf(a,fminf(b,x));}
/* A soft-log amplitude axis keeps unity near the center and quiet levels
   near the origin, leaving useful room for bends above ordinary thresholds. */
float dynamics_level_position(float db){return log1pf(2*powf(10,clamp(db,-72,12)/20))/log1pf(2*powf(10,12.f/20));}
float dynamics_level_db(float position){float gain=expm1f(clamp(position,0,1)*log1pf(2*powf(10,12.f/20)))/2;return clamp(20*log10f(fmaxf(gain,1e-12f)),-72,12);}
float dynamics_monitor_speed(float control){
    if(control<=-1)return .25f;if(control>=1)return 4;
    return .85f*powf(control<0?.85f/.25f:4/.85f,control);
}
void dynamics_band_bounds(float low,float high,float lowcut,float bounds[4]){
    float cut=clamp(logf(fmaxf(20,lowcut)/20)/logf(1000),0,1);
    bounds[0]=cut;bounds[1]=fmaxf(cut,clamp(logf(low/20)/logf(1000),0,1));
    bounds[2]=fmaxf(bounds[1],clamp(logf(high/20)/logf(1000),0,1));bounds[3]=1;
}
float dynamics_monitor_position(unsigned age,float control){return 1-age*dynamics_monitor_speed(control)/255.f;}
float dynamics_recent_input_envelope(const DynamicsMonitor *monitor,int band,unsigned samples){
    if(!monitor || band<0 || band>=4 || !monitor->count || !samples)return -72;
    unsigned first=monitor->count>samples?monitor->count-samples:0;
    float peak=-72;
    /* Each monitor entry spans 256 frames. Decay old peaks by about 2 dB per
       entry so a click remains visible for a few UI frames without freezing. */
    for(unsigned i=first;i<monitor->count;i++)peak=fmaxf(peak,monitor->input[band][i]-2.f*(monitor->count-1-i));
    return peak;
}
float dynamics_meter_height(const DynamicsBand *band,float input_db,float position){
    if(input_db<=-71.5f || position<0 || position>dynamics_level_position(input_db))return 0;
    return dynamics_level_position(dynamics_curve(band,dynamics_level_db(position)));
}
float dynamics_curve_handle_input(const DynamicsBand *band,int segment){
    if(segment<0 || segment>=(int)band->count-1)return -72;
    return dynamics_level_db((dynamics_level_position(band->points[segment].x)+dynamics_level_position(band->points[segment+1].x))*.5f);
}
int dynamics_curve_insert(DynamicsBand *b,float x,float y){int n=(int)b->count;if(n>=DYNAMICS_POINTS || !isfinite(x) || !isfinite(y) || x<=-72 || x>=12)return -1;int at=1;while(at<n && b->points[at].x<x)at++;if(x-b->points[at-1].x<.25f || b->points[at].x-x<.25f)return -1;memmove(&b->points[at+1],&b->points[at],(n-at)*sizeof *b->points);b->points[at]=(DynamicsPoint){x,clamp(y,-72,12),0,0};b->count++;return at;}
int dynamics_curve_delete(DynamicsBand *b,int p){int n=(int)b->count;if(p<=0 || p>=n-1)return 0;memmove(&b->points[p],&b->points[p+1],(n-p-1)*sizeof *b->points);b->points[n-1]=(DynamicsPoint){0};b->count--;return 1;}
void dynamics_curve_move(DynamicsBand *b,int p,float x,float y,int lock_x,int lock_y){int n=(int)b->count;if(p<0 || p>=n || !isfinite(x) || !isfinite(y))return;if(!lock_x && p>0 && p<n-1)b->points[p].x=clamp(x,b->points[p-1].x+.25f,b->points[p+1].x-.25f);if(!lock_y)b->points[p].y=clamp(y,-72,12);}
void dynamics_curve_bend(DynamicsBand *b,int p,float bend){if(p>=0 && p<(int)b->count-1 && isfinite(bend))b->points[p].bend=clamp(bend,-1,1);}

/* Shared by grid drawing and magnet editing; distance is measured in display
   space because the dB axes are nonlinear. The quiet boundary is a grid edge. */
const float *dynamics_grid_values(unsigned *count){static const float values[]={-72,-18,-12,-9,-6,-3,0,3,6,9,12};*count=sizeof values/sizeof *values;return values;}
static float snap_between(float db,float low,float high,float fallback){
    unsigned count;const float *grid=dynamics_grid_values(&count);float position=dynamics_level_position(db),distance=2,best=fallback;
    for(unsigned i=0;i<count;i++)if(grid[i]>=low && grid[i]<=high){float d=fabsf(dynamics_level_position(grid[i])-position);if(d<distance){best=grid[i];distance=d;}}
    return best;
}
float dynamics_grid_snap(float db){return snap_between(db,-72,12,db);}
void dynamics_curve_move_grid(DynamicsBand *b,int point,float input,float output,int lock_x,int lock_y,int magnet){
    if(!magnet){dynamics_curve_move(b,point,input,output,lock_x,lock_y);return;}
    int count=(int)b->count;if(point<0 || point>=count || !isfinite(input) || !isfinite(output))return;
    if(!lock_x && point>0 && point<count-1)b->points[point].x=snap_between(input,b->points[point-1].x+.25f,b->points[point+1].x-.25f,b->points[point].x);
    if(!lock_y)b->points[point].y=dynamics_grid_snap(output);
}
