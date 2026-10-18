// SPDX-License-Identifier: GPL-3.0-only
/* Concrete2D renderer implementation. No SDL types leak into interaction or
   engine headers. UI clipping is explicit replacement, never a widget stack. */
#include "draw.h"
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
struct DrawContext { SDL_Renderer *renderer; float scale,x,y; };
struct DrawTexture { SDL_Texture *texture; int width,height; };
static SDL_FColor color(DrawColor c) { return (SDL_FColor){c.r/255.f,c.g/255.f,c.b/255.f,c.a/255.f}; }
static SDL_FPoint point(DrawContext *d,DrawPoint p) { return (SDL_FPoint){p.x*d->scale+d->x,p.y*d->scale+d->y}; }
static SDL_FRect rect(DrawContext *d,DrawRect r) { return (SDL_FRect){r.x*d->scale+d->x,r.y*d->scale+d->y,r.width*d->scale,r.height*d->scale}; }
DrawContext *draw_create(void *renderer) {
    if(!renderer)return NULL;DrawContext *d=calloc(1,sizeof *d);if(!d)return NULL;
    d->renderer=renderer;d->scale=1;SDL_SetRenderDrawBlendMode(d->renderer,SDL_BLENDMODE_BLEND);return d;
}
void draw_destroy(DrawContext *d) { free(d); }
void draw_transform(DrawContext *d,float scale,float x,float y) { d->scale=scale;d->x=x;d->y=y; }
void draw_clear(DrawContext *d,DrawColor c) { SDL_SetRenderDrawColor(d->renderer,c.r,c.g,c.b,c.a);SDL_RenderClear(d->renderer); }
void draw_rect(DrawContext *d,DrawRect r,DrawColor c) { SDL_FRect area=rect(d,r);SDL_SetRenderDrawColor(d->renderer,c.r,c.g,c.b,c.a);SDL_RenderFillRect(d->renderer,&area); }
void draw_border(DrawContext *d,DrawRect r,float width,DrawColor c) {
    /* Original DrawRectangleLinesEx clamps only beyond the full dimension,
       not at half its size; preserve overlapping-border cases. */
    if(width>r.width || width>r.height) {
        if(r.width>=r.height)width=r.height/2;
        else if(r.width<=r.height)width=r.width/2;
    }
    draw_rect(d,(DrawRect){r.x,r.y,r.width,width},c);
    draw_rect(d,(DrawRect){r.x,r.y+r.height-width,r.width,width},c);
    draw_rect(d,(DrawRect){r.x,r.y+width,width,r.height-2*width},c);
    draw_rect(d,(DrawRect){r.x+r.width-width,r.y+width,width,r.height-2*width},c);
}
void draw_line(DrawContext *d,DrawPoint a,DrawPoint b,float width,DrawColor c) {
    float dx=b.x-a.x,dy=b.y-a.y,length=sqrtf(dx*dx+dy*dy);
    if(length<=0 || width<=0)return;
    float k=width/(2*length);DrawPoint n={-k*dy,k*dx};
    DrawVertex v[]={{{a.x+n.x,a.y+n.y},0,0,c},{{a.x-n.x,a.y-n.y},0,0,c},{{b.x+n.x,b.y+n.y},0,0,c},
                    {{b.x+n.x,b.y+n.y},0,0,c},{{a.x-n.x,a.y-n.y},0,0,c},{{b.x-n.x,b.y-n.y},0,0,c}};
    draw_triangles(d,NULL,v,6);
}
void draw_hairline(DrawContext *d,DrawPoint a,DrawPoint b,DrawColor c) {
    SDL_FPoint from=point(d,a),to=point(d,b);SDL_SetRenderDrawColor(d->renderer,c.r,c.g,c.b,c.a);
    SDL_RenderLine(d->renderer,from.x,from.y,to.x,to.y);
}
void draw_clip(DrawContext *d,const DrawRect *r) {
    if(!r){SDL_SetRenderClipRect(d->renderer,NULL);return;}
    SDL_Rect clip={(int)r->x,(int)r->y,(int)r->width,(int)r->height};SDL_SetRenderClipRect(d->renderer,&clip);
}
DrawTexture *draw_texture_create(DrawContext *d,int width,int height,const void *rgba,int target) {
    if(width<=0 || height<=0)return NULL;DrawTexture *t=calloc(1,sizeof *t);if(!t)return NULL;
    t->texture=SDL_CreateTexture(d->renderer,SDL_PIXELFORMAT_RGBA32,target?SDL_TEXTUREACCESS_TARGET:SDL_TEXTUREACCESS_STATIC,width,height);
    if(!t->texture){free(t);return NULL;}t->width=width;t->height=height;
    SDL_SetTextureBlendMode(t->texture,SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(t->texture,SDL_SCALEMODE_NEAREST);
    if(rgba && !SDL_UpdateTexture(t->texture,NULL,rgba,width*4)){SDL_DestroyTexture(t->texture);free(t);return NULL;}
    return t;
}
void draw_texture_destroy(DrawTexture *t) { if(t){SDL_DestroyTexture(t->texture);free(t);} }
void draw_texture_filter(DrawTexture *t,int linear) { if(t)SDL_SetTextureScaleMode(t->texture,linear?SDL_SCALEMODE_LINEAR:SDL_SCALEMODE_NEAREST); }
void draw_texture(DrawContext *d,DrawTexture *t,DrawRect source,DrawRect destination,DrawColor c) {
    if(!t)return;SDL_FRect src={source.x,source.y,fabsf(source.width),fabsf(source.height)},dst=rect(d,destination);
    SDL_FlipMode flip=(source.width<0?SDL_FLIP_HORIZONTAL:0)|(source.height<0?SDL_FLIP_VERTICAL:0);
    SDL_SetTextureColorMod(t->texture,c.r,c.g,c.b);SDL_SetTextureAlphaMod(t->texture,c.a);
    SDL_RenderTextureRotated(d->renderer,t->texture,&src,&dst,0,NULL,flip);
}
void draw_texture_pro(DrawContext *d,DrawTexture *t,DrawRect source,DrawRect destination,DrawPoint origin,float rotation,DrawColor c) {
    /* Origin/rotation arithmetic adapted from pinned rtextures.c; retained
       zlib notice in docs/licenses/raylib.txt. Target textures use SDL's
       top-origin storage; callers do not add a legacy GL framebuffer flip. */
    if(!t)return;
    DrawPoint top_left,top_right,bottom_left,bottom_right;
    if(rotation==0) {
        float x=destination.x-origin.x,y=destination.y-origin.y;
        top_left=(DrawPoint){x,y};top_right=(DrawPoint){x+destination.width,y};
        bottom_left=(DrawPoint){x,y+destination.height};bottom_right=(DrawPoint){x+destination.width,y+destination.height};
    }else {
        float radians=rotation*(3.14159265358979323846f/180.0f),sin_rotation=sinf(radians),cos_rotation=cosf(radians);
        float x=destination.x,y=destination.y,dx=-origin.x,dy=-origin.y;
        top_left=(DrawPoint){x+dx*cos_rotation-dy*sin_rotation,y+dx*sin_rotation+dy*cos_rotation};
        top_right=(DrawPoint){x+(dx+destination.width)*cos_rotation-dy*sin_rotation,y+(dx+destination.width)*sin_rotation+dy*cos_rotation};
        bottom_left=(DrawPoint){x+dx*cos_rotation-(dy+destination.height)*sin_rotation,y+dx*sin_rotation+(dy+destination.height)*cos_rotation};
        bottom_right=(DrawPoint){x+(dx+destination.width)*cos_rotation-(dy+destination.height)*sin_rotation,y+(dx+destination.width)*sin_rotation+(dy+destination.height)*cos_rotation};
    }
    float left=source.x/t->width,right=(source.x+fabsf(source.width))/t->width,top=source.y/t->height,bottom=(source.y+fabsf(source.height))/t->height;
    if(source.width<0){float swap=left;left=right;right=swap;}if(source.height<0){float swap=top;top=bottom;bottom=swap;}
    DrawVertex vertices[]={ {top_left,left,top,c},{bottom_left,left,bottom,c},{bottom_right,right,bottom,c},
        {top_left,left,top,c},{bottom_right,right,bottom,c},{top_right,right,top,c} };
    draw_triangles(d,t,vertices,6);
}
void draw_triangles(DrawContext *d,DrawTexture *texture,const DrawVertex *vertices,int count) {
    SDL_Vertex batch[1536];
    for(int at=0;at<count;) {
        int n=count-at;if(n>1536)n=1536;
        for(int i=0;i<n;i++){DrawVertex v=vertices[at+i];batch[i]=(SDL_Vertex){point(d,v.point),color(v.color),{v.u,v.v}};}
        SDL_RenderGeometry(d->renderer,texture?texture->texture:NULL,batch,n,NULL,0);at+=n;
    }
}
void draw_target(DrawContext *d,DrawTexture *t) { SDL_SetRenderTarget(d->renderer,t?t->texture:NULL); }
int draw_read_rgba(DrawContext *d,void *rgba,int width,int height) {
    SDL_Surface *surface=SDL_RenderReadPixels(d->renderer,NULL);if(!surface)return 0;
    SDL_Surface *converted=SDL_ConvertSurface(surface,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(surface);if(!converted)return 0;
    int ok=converted->w==width && converted->h==height;
    if(ok)for(int y=0;y<height;y++)memcpy((unsigned char *)rgba+y*width*4,(unsigned char *)converted->pixels+y*converted->pitch,(size_t)width*4);
    SDL_DestroySurface(converted);return ok;
}
void draw_flush(DrawContext *d) { SDL_FlushRenderer(d->renderer); }

int draw_present(DrawContext *draw){return SDL_RenderPresent(draw->renderer);}

void draw_texture_tiled_pixels(DrawContext *d,DrawTexture *texture,DrawRect source,DrawRect destination,DrawColor color){
    if(!d || !texture || source.width<1 || source.height<1 || destination.width<=0 || destination.height<=0)return;
    float scale=d->scale,ox=d->x,oy=d->y;
    int left=(int)ceilf(destination.x*scale+ox),top=(int)ceilf(destination.y*scale+oy);
    int right=(int)ceilf((destination.x+destination.width)*scale+ox),bottom=(int)ceilf((destination.y+destination.height)*scale+oy);
    int tile_w=(int)source.width,tile_h=(int)source.height;
    d->scale=1;d->x=d->y=0;
    for(int y=top;y<bottom;){
        int sy=((y%tile_h)+tile_h)%tile_h,h=bottom-y;if(h>tile_h-sy)h=tile_h-sy;
        for(int x=left;x<right;){
            int sx=((x%tile_w)+tile_w)%tile_w,w=right-x;if(w>tile_w-sx)w=tile_w-sx;
            draw_texture(d,texture,(DrawRect){source.x+sx,source.y+sy,w,h},(DrawRect){x,y,w,h},color);
            x+=w;
        }y+=h;
    }
    d->scale=scale;d->x=ox;d->y=oy;
}
