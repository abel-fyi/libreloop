// SPDX-License-Identifier: GPL-3.0-only
/* Adapted pinned rshapes circle geometry; zlib notice: docs/licenses/raylib.txt. */
#include "shapes.h"
#include <math.h>
#define PI 3.14159265358979323846f
#define DEG2RAD (PI/180.0f)
void shape_circle(DrawContext *draw,int x,int y,float radius,DrawColor color) {
    if(radius<=0)radius=.1f;float angle=0,stepLength=360/36.f;DrawVertex vertices[108];int count=0;
    for(int i=0;i<36/2;i++) {
        DrawPoint p[]={ {x,y},{x+cosf(DEG2RAD*(angle+stepLength*2.0f))*radius,y+sinf(DEG2RAD*(angle+stepLength*2.0f))*radius},
            {x+cosf(DEG2RAD*(angle+stepLength))*radius,y+sinf(DEG2RAD*(angle+stepLength))*radius},
            {x+cosf(DEG2RAD*angle)*radius,y+sinf(DEG2RAD*angle)*radius}};
        const int order[]={0,1,2,0,2,3};for(int j=0;j<6;j++)vertices[count++]=(DrawVertex){p[order[j]],0,0,color};angle+=stepLength*2.0f;
    }
    draw_triangles(draw,NULL,vertices,count);
}
void shape_circle_lines(DrawContext *draw,int x,int y,float radius,DrawColor color) {
    for(int i=0;i<360;i+=10)draw_hairline(draw,(DrawPoint){x+cosf(DEG2RAD*i)*radius,y+sinf(DEG2RAD*i)*radius},(DrawPoint){x+cosf(DEG2RAD*(i+10))*radius,y+sinf(DEG2RAD*(i+10))*radius},color);
}
