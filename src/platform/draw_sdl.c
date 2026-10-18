// SPDX-License-Identifier: GPL-3.0-only
/* Concrete2D renderer implementation. No SDL types leak into interaction or
   engine headers. UI clipping is explicit replacement, never a widget stack. */
#include "draw.h"
#include "draw_palette.h"
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
typedef struct { DrawTexture *texture;DrawColor a,b;int width,height,left,top,horizontal; } MaterialTile;
struct DrawContext { SDL_Renderer *renderer; float scale,x,y;int pattern_step,palette;unsigned material_next;size_t material_bytes;MaterialTile materials[128]; };
struct DrawTexture { SDL_Texture *texture; int width,height; };
static SDL_FColor color(DrawColor c) { return (SDL_FColor){c.r/255.f,c.g/255.f,c.b/255.f,c.a/255.f}; }
static SDL_FPoint point(DrawContext *d,DrawPoint p) { return (SDL_FPoint){p.x*d->scale+d->x,p.y*d->scale+d->y}; }
static SDL_FRect rect(DrawContext *d,DrawRect r) { return (SDL_FRect){r.x*d->scale+d->x,r.y*d->scale+d->y,r.width*d->scale,r.height*d->scale}; }
DrawContext *draw_create(void *renderer) {
    if(!renderer)return NULL;DrawContext *d=calloc(1,sizeof *d);if(!d)return NULL;
    d->renderer=renderer;d->scale=1;SDL_SetRenderDrawBlendMode(d->renderer,SDL_BLENDMODE_BLEND);return d;
}
void draw_destroy(DrawContext *d) { for(int i=0;i<128;i++)draw_texture_destroy(d->materials[i].texture);free(d); }
void draw_palette_enable(DrawContext *d,int enabled){d->palette=enabled;}
int draw_palette_enabled(const DrawContext *d){return d->palette;}
static DrawColor paint(DrawContext *d,DrawColor c){return d->palette?luice_palette_map(c,0,0,0):c;}
void draw_pattern_density(DrawContext *d,float density){d->pattern_step=isfinite(density)?(int)roundf(fmaxf(1,fminf(8,density))):1;}
int draw_pattern_pixel_size(const DrawContext *d){return d->pattern_step>0?d->pattern_step:1;}
void draw_transform(DrawContext *d,float scale,float x,float y) { d->scale=scale;d->x=x;d->y=y; }
void draw_clear(DrawContext *d,DrawColor c) { c=paint(d,c); SDL_SetRenderDrawColor(d->renderer,c.r,c.g,c.b,c.a);SDL_RenderClear(d->renderer); }
void draw_rect(DrawContext *d,DrawRect r,DrawColor c) { c=paint(d,c); SDL_FRect area=rect(d,r);SDL_SetRenderDrawColor(d->renderer,c.r,c.g,c.b,c.a);SDL_RenderFillRect(d->renderer,&area); }
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
    c=paint(d,c);SDL_FPoint from=point(d,a),to=point(d,b);SDL_SetRenderDrawColor(d->renderer,c.r,c.g,c.b,c.a);
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
    DrawColor *mapped=NULL;
    if(rgba && d->palette){
        mapped=malloc((size_t)width*height*sizeof *mapped);
        if(!mapped){draw_texture_destroy(t);return NULL;}
        const DrawColor *src=rgba;
        for(int y=0;y<height;y++)for(int x=0;x<width;x++)mapped[y*width+x]=luice_palette_map(src[y*width+x],x,y,1);
        rgba=mapped;
    }
    if(rgba && !SDL_UpdateTexture(t->texture,NULL,rgba,width*4)){free(mapped);SDL_DestroyTexture(t->texture);free(t);return NULL;}
    free(mapped);return t;
}
void draw_texture_destroy(DrawTexture *t) { if(t){SDL_DestroyTexture(t->texture);free(t);} }
void draw_texture_filter(DrawTexture *t,int linear) { if(t)SDL_SetTextureScaleMode(t->texture,linear?SDL_SCALEMODE_LINEAR:SDL_SCALEMODE_NEAREST); }
void draw_texture(DrawContext *d,DrawTexture *t,DrawRect source,DrawRect destination,DrawColor c) {
    if(!t)return;c=paint(d,c);SDL_FRect src={source.x,source.y,fabsf(source.width),fabsf(source.height)},dst=rect(d,destination);
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
        for(int i=0;i<n;i++){DrawVertex v=vertices[at+i];batch[i]=(SDL_Vertex){point(d,v.point),color(paint(d,v.color)),{v.u,v.v}};}
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

/* Native-pixel cached ordered gradients: no framebuffer readback or postfilter. */
void draw_material_gradient(DrawContext *d,DrawRect area,DrawColor a,DrawColor b,int horizontal){
    SDL_FRect dst=rect(d,area);int left=(int)ceilf(dst.x),top=(int)ceilf(dst.y);
    int w=(int)ceilf(dst.x+dst.w)-left,h=(int)ceilf(dst.y+dst.h)-top;
    if(w<1 || h<1)return;
    int tw=horizontal?w:64,th=horizontal?64:h,phase_x=left&3,phase_y=top&3;
    MaterialTile *tile=NULL;
    for(int i=0;i<128;i++){MaterialTile *m=&d->materials[i];
        if(m->texture && m->width==tw && m->height==th && m->left==phase_x && m->top==phase_y && m->horizontal==horizontal && !memcmp(&m->a,&a,sizeof a) && !memcmp(&m->b,&b,sizeof b)){tile=m;break;}}
    if(!tile){
        size_t bytes=(size_t)tw*th*sizeof(DrawColor);
        if(bytes>8*1024*1024){draw_rect(d,area,a);return;}
        DrawColor *pixels=malloc(bytes);if(!pixels){draw_rect(d,area,a);return;}
        for(int y=0;y<th;y++)for(int x=0;x<tw;x++){
            float t=horizontal?(w>1?x/(float)(w-1):0):(h>1?y/(float)(h-1):0);
            DrawColor c={a.r+(b.r-a.r)*t,a.g+(b.g-a.g)*t,a.b+(b.b-a.b)*t,a.a+(b.a-a.a)*t};
            pixels[y*tw+x]=luice_palette_map(c,x+phase_x,y+phase_y,1);
        }
        DrawTexture *texture=draw_texture_create(d,tw,th,pixels,0);free(pixels);
        if(!texture){draw_rect(d,area,a);return;}
        tile=&d->materials[d->material_next++%128];
        if(tile->texture){d->material_bytes-=(size_t)tile->width*tile->height*sizeof(DrawColor);draw_texture_destroy(tile->texture);tile->texture=NULL;}
        while(d->material_bytes+bytes>8*1024*1024){
            MaterialTile *old=&d->materials[d->material_next++%128];
            if(old->texture){d->material_bytes-=(size_t)old->width*old->height*sizeof(DrawColor);draw_texture_destroy(old->texture);old->texture=NULL;}
        }
        d->material_bytes+=bytes;
        *tile=(MaterialTile){texture,a,b,tw,th,phase_x,phase_y,horizontal};
    }
    float scale=d->scale,ox=d->x,oy=d->y;d->scale=1;d->x=d->y=0;
    for(int y=0;y<h;y+=th)for(int x=0;x<w;x+=tw){int cw=w-x<tw?w-x:tw,ch=h-y<th?h-y:th;
        draw_texture(d,tile->texture,(DrawRect){0,0,cw,ch},(DrawRect){left+x,top+y,cw,ch},(DrawColor){255,255,255,255});}
    d->scale=scale;d->x=ox;d->y=oy;
}
