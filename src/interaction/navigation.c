// SPDX-License-Identifier: GPL-3.0-only
#include "navigation.h"
NavigationMotion navigation_motion(NavigationInput input,int shift,int zoom_modifier,float scale) {
    NavigationMotion motion={.zoom=input.zoom};
    if(zoom_modifier) { motion.zoom+=input.y*(input.precise?.05f:1); return motion; }
    float distance=input.precise?1/scale:48;
    motion.x=-input.x*distance;
    if(shift) motion.x-=input.y*distance;
    else motion.y=-input.y*distance;
    return motion;
}
