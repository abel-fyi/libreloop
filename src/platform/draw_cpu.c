// SPDX-License-Identifier: GPL-3.0-only
/* Phase5 experiment only: immediate CPU RGBA rasterization, SDL final blit.
   Sampling/raster fidelity is evaluated, not assumed equal to hardware SDL. */
#include "draw.h"
#include "draw_palette.h"
#include "draw_cpu_stats.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
struct DrawTexture {DrawColor *pixels;int width,height,linear;};
struct DrawContext {
    SDL_Renderer *renderer;SDL_Texture *present;
    DrawColor *frame,*previous;int width,height,have_previous;
    DrawTexture *target;float scale,x,y;int pattern_step,palette;
    int left,top,right,bottom,clipped;
    DrawCpuStats stats;
};
static DrawPoint point(DrawContext *d,DrawPoint p){return (DrawPoint){p.x*d->scale+d->x,p.y*d->scale+d->y};}
static int dimensions(DrawContext *d) {
    if(d->target)return 1;int w,h;if(!SDL_GetRenderOutputSize(d->renderer,&w,&h) || w<1 || h<1)return 0;
    if(w==d->width && h==d->height)return 1;
    DrawColor *frame=calloc((size_t)w*h,sizeof *frame),*previous=malloc((size_t)w*h*sizeof *previous);
    SDL_Texture *texture=SDL_CreateTexture(d->renderer,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING,w,h);
    if(!frame || !previous || !texture){free(frame);free(previous);if(texture)SDL_DestroyTexture(texture);return 0;}
    free(d->frame);free(d->previous);if(d->present)SDL_DestroyTexture(d->present);
    d->frame=frame;d->previous=previous;d->present=texture;d->width=w;d->height=h;d->have_previous=0;
    SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_NONE);SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_NEAREST);return 1;
}
static DrawColor *pixels(DrawContext *d,int *w,int *h) {
    if(!d->target && !d->frame && !dimensions(d))return NULL;
    *w=d->target?d->target->width:d->width;*h=d->target?d->target->height:d->height;
    return d->target?d->target->pixels:d->frame;
}
static void bounds(DrawContext *d,int w,int h,int *l,int *t,int *r,int *b) {
    *l=d->clipped?d->left:0;*t=d->clipped?d->top:0;*r=d->clipped?d->right:w;*b=d->clipped?d->bottom:h;
    if(*l<0)*l=0;if(*t<0)*t=0;if(*r>w)*r=w;if(*b>h)*b=h;
}
static DrawColor blend(DrawColor dst,DrawColor src) {
    if(!src.a)return dst;if(src.a==255)return src;
    unsigned a=src.a,inv=255-a;
    return (DrawColor){(unsigned char)((src.r*a+dst.r*inv)/255),(unsigned char)((src.g*a+dst.g*inv)/255),
        (unsigned char)((src.b*a+dst.b*inv)/255),(unsigned char)(a+dst.a*inv/255)};
}
DrawContext *draw_create(void *native){if(!native)return NULL;DrawContext *d=calloc(1,sizeof *d);if(!d)return NULL;d->renderer=native;d->scale=1;if(!dimensions(d)){draw_destroy(d);return NULL;}return d;}
void draw_destroy(DrawContext *d){if(d){free(d->frame);free(d->previous);if(d->present)SDL_DestroyTexture(d->present);free(d);}}
void draw_palette_enable(DrawContext *d,int enabled){d->palette=enabled;}
int draw_palette_enabled(const DrawContext *d){return d->palette;}
void draw_pattern_density(DrawContext *d,float density){d->pattern_step=isfinite(density)?(int)roundf(fmaxf(1,fminf(8,density))):1;}
int draw_pattern_pixel_size(const DrawContext *d){return d->pattern_step>0?d->pattern_step:1;}
void draw_transform(DrawContext *d,float scale,float x,float y){d->scale=scale;d->x=x;d->y=y;}
void draw_clear(DrawContext *d,DrawColor c){if(d->palette)c=luice_palette_map(c,0,0,0);if(!dimensions(d))return;int w,h;DrawColor *p=pixels(d,&w,&h);if(p)for(size_t i=0;i<(size_t)w*h;i++)p[i]=c;}
void draw_clip(DrawContext *d,const DrawRect *r){d->clipped=r!=NULL;if(r){d->left=(int)r->x;d->top=(int)r->y;d->right=d->left+(int)r->width;d->bottom=d->top+(int)r->height;}}
void draw_rect(DrawContext *d,DrawRect rect,DrawColor c) {
    if(d->palette)c=luice_palette_map(c,0,0,0);
    int w,h,l,t,r,b;DrawColor *p=pixels(d,&w,&h);if(!p || rect.width<=0 || rect.height<=0)return;
    bounds(d,w,h,&l,&t,&r,&b);DrawPoint a=point(d,(DrawPoint){rect.x,rect.y}),z=point(d,(DrawPoint){rect.x+rect.width,rect.y+rect.height});
    int left=(int)ceilf(a.x-.5f),top=(int)ceilf(a.y-.5f),right=(int)ceilf(z.x-.5f),bottom=(int)ceilf(z.y-.5f);
    if(left<l)left=l;if(top<t)top=t;if(right>r)right=r;if(bottom>b)bottom=b;
    for(int y=top;y<bottom;y++)for(int x=left;x<right;x++)p[(size_t)y*w+x]=blend(p[(size_t)y*w+x],c);
}
void draw_border(DrawContext *d,DrawRect r,float width,DrawColor c) {
    if(width>r.width || width>r.height)width=r.width>=r.height?r.height/2:r.width/2;
    draw_rect(d,(DrawRect){r.x,r.y,r.width,width},c);draw_rect(d,(DrawRect){r.x,r.y+r.height-width,r.width,width},c);
    draw_rect(d,(DrawRect){r.x,r.y+width,width,r.height-2*width},c);draw_rect(d,(DrawRect){r.x+r.width-width,r.y+width,width,r.height-2*width},c);
}
void draw_hairline(DrawContext *d,DrawPoint from,DrawPoint to,DrawColor c) {
    int w,h,l,t,r,b;DrawColor *p=pixels(d,&w,&h);if(!p)return;bounds(d,w,h,&l,&t,&r,&b);
    from=point(d,from);to=point(d,to);int x=(int)roundf(from.x),y=(int)roundf(from.y),xx=(int)roundf(to.x),yy=(int)roundf(to.y);
    int dx=abs(xx-x),sx=x<xx?1:-1,dy=-abs(yy-y),sy=y<yy?1:-1,error=dx+dy;
    for(;;){if(x>=l && x<r && y>=t && y<b)p[(size_t)y*w+x]=blend(p[(size_t)y*w+x],c);if(x==xx && y==yy)break;int e=2*error;if(e>=dy){error+=dy;x+=sx;}if(e<=dx){error+=dx;y+=sy;}}
}
static float edge(DrawPoint a,DrawPoint b,DrawPoint p){return (b.x-a.x)*(p.y-a.y)-(b.y-a.y)*(p.x-a.x);}
static int top_left(DrawPoint a,DrawPoint b){return b.y<a.y || (b.y==a.y && b.x>a.x);}
static unsigned char byte(float v){return (unsigned char)fmaxf(0,fminf(255,roundf(v)));}
static DrawColor sample(DrawTexture *t,float u,float v) {
    if(!t)return (DrawColor){255,255,255,255};
    float x=u*t->width,y=v*t->height;
    if(!t->linear){int xx=(int)floorf(x),yy=(int)floorf(y);xx=xx<0?0:xx>=t->width?t->width-1:xx;yy=yy<0?0:yy>=t->height?t->height-1:yy;return t->pixels[(size_t)yy*t->width+xx];}
    x-=.5f;y-=.5f;int x0=(int)floorf(x),y0=(int)floorf(y);float fx=x-x0,fy=y-y0;DrawColor c[4];
    for(int j=0;j<2;j++)for(int i=0;i<2;i++){int xx=x0+i,yy=y0+j;xx=xx<0?0:xx>=t->width?t->width-1:xx;yy=yy<0?0:yy>=t->height?t->height-1:yy;c[j*2+i]=t->pixels[(size_t)yy*t->width+xx];}
    DrawColor out;unsigned char *o=(unsigned char*)&out;
    for(int k=0;k<4;k++){float a=((unsigned char*)&c[0])[k]*(1-fx)+((unsigned char*)&c[1])[k]*fx,b=((unsigned char*)&c[2])[k]*(1-fx)+((unsigned char*)&c[3])[k]*fx;o[k]=byte(a*(1-fy)+b*fy);}return out;
}
void draw_triangles(DrawContext *d,DrawTexture *texture,const DrawVertex *vertices,int count) {
    int w,h,l,t,r,b;DrawColor *p=pixels(d,&w,&h);if(!p)return;bounds(d,w,h,&l,&t,&r,&b);
    for(int at=0;at+2<count;at+=3) {
        DrawVertex a=vertices[at],c=vertices[at+2],v=vertices[at+1];a.point=point(d,a.point);v.point=point(d,v.point);c.point=point(d,c.point);
        float area=edge(a.point,v.point,c.point);if(!isfinite(area) || area==0)continue;
        if(area<0){DrawVertex swap=v;v=c;c=swap;area=-area;}
        int left=(int)floorf(fminf(a.point.x,fminf(v.point.x,c.point.x))),right=(int)ceilf(fmaxf(a.point.x,fmaxf(v.point.x,c.point.x)));
        int top=(int)floorf(fminf(a.point.y,fminf(v.point.y,c.point.y))),bottom=(int)ceilf(fmaxf(a.point.y,fmaxf(v.point.y,c.point.y)));
        if(left<l)left=l;if(right>r)right=r;if(top<t)top=t;if(bottom>b)bottom=b;
        for(int y=top;y<bottom;y++)for(int x=left;x<right;x++) {
            DrawPoint q={x+.5f,y+.5f};float e0=edge(v.point,c.point,q),e1=edge(c.point,a.point,q),e2=edge(a.point,v.point,q);
            if(e0<0 || (e0==0 && !top_left(v.point,c.point)) || e1<0 || (e1==0 && !top_left(c.point,a.point)) || e2<0 || (e2==0 && !top_left(a.point,v.point)))continue;
            float f0=e0/area,f1=e1/area,f2=e2/area;DrawColor tex=sample(texture,a.u*f0+v.u*f1+c.u*f2,a.v*f0+v.v*f1+c.v*f2);
            DrawColor color={byte((a.color.r*f0+v.color.r*f1+c.color.r*f2)*tex.r/255),byte((a.color.g*f0+v.color.g*f1+c.color.g*f2)*tex.g/255),byte((a.color.b*f0+v.color.b*f1+c.color.b*f2)*tex.b/255),byte((a.color.a*f0+v.color.a*f1+c.color.a*f2)*tex.a/255)};
            if(d->palette)color=luice_palette_map(color,x,y,1);
            p[(size_t)y*w+x]=blend(p[(size_t)y*w+x],color);
        }
    }
}
void draw_line(DrawContext *d,DrawPoint a,DrawPoint b,float width,DrawColor c) {
    float dx=b.x-a.x,dy=b.y-a.y,length=sqrtf(dx*dx+dy*dy);if(length<=0 || width<=0)return;float k=width/(2*length);DrawPoint n={-k*dy,k*dx};
    DrawVertex v[]={{{a.x+n.x,a.y+n.y},0,0,c},{{a.x-n.x,a.y-n.y},0,0,c},{{b.x+n.x,b.y+n.y},0,0,c},{{b.x+n.x,b.y+n.y},0,0,c},{{a.x-n.x,a.y-n.y},0,0,c},{{b.x-n.x,b.y-n.y},0,0,c}};draw_triangles(d,NULL,v,6);
}
DrawTexture *draw_texture_create(DrawContext *d,int w,int h,const void *rgba,int target){(void)d;(void)target;if(w<1 || h<1)return NULL;DrawTexture *t=calloc(1,sizeof *t);if(!t)return NULL;t->pixels=calloc((size_t)w*h,sizeof *t->pixels);if(!t->pixels){free(t);return NULL;}t->width=w;t->height=h;if(rgba){memcpy(t->pixels,rgba,(size_t)w*h*4);if(d->palette)for(int y=0;y<h;y++)for(int x=0;x<w;x++)t->pixels[y*w+x]=luice_palette_map(t->pixels[y*w+x],x,y,1);}return t;}
void draw_texture_destroy(DrawTexture *t){if(t){free(t->pixels);free(t);}}
void draw_texture_filter(DrawTexture *t,int linear){if(t)t->linear=linear;}
void draw_target(DrawContext *d,DrawTexture *t){d->target=t;d->clipped=0;}
int draw_read_rgba(DrawContext *d,void *rgba,int width,int height){int w,h;DrawColor *p=pixels(d,&w,&h);if(!p || w!=width || h!=height)return 0;memcpy(rgba,p,(size_t)w*h*4);return 1;}
void draw_flush(DrawContext *d){(void)d;/* CPU commands are immediate; no pending texture references. */}
int draw_present(DrawContext *d) {
    if(d->target || !dimensions(d))return 0;d->stats.presents++;d->stats.compared_pixels+=(uint64_t)d->width*d->height;int l=d->width,t=d->height,r=0,b=0;
    for(int y=0;y<d->height;y++)for(int x=0;x<d->width;x++) {
        size_t i=(size_t)y*d->width+x;if(!d->have_previous || memcmp(&d->frame[i],&d->previous[i],4)) {
            if(x<l)l=x;if(x+1>r)r=x+1;if(y<t)t=y;if(y+1>b)b=y+1;
        }
    }
    if(r>l && b>t){SDL_Rect dirty={l,t,r-l,b-t};if(!SDL_UpdateTexture(d->present,&dirty,d->frame+(size_t)t*d->width+l,d->width*4))return 0;d->stats.uploads++;d->stats.uploaded_pixels+=(uint64_t)(r-l)*(b-t);memcpy(d->previous,d->frame,(size_t)d->width*d->height*4);d->have_previous=1;}
    return SDL_RenderTexture(d->renderer,d->present,NULL,NULL) && SDL_RenderPresent(d->renderer);
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

void draw_texture(DrawContext *d,DrawTexture *t,DrawRect source,DrawRect dest,DrawColor color){draw_texture_pro(d,t,source,dest,(DrawPoint){0},0,color);}

DrawCpuStats draw_cpu_stats(const DrawContext *draw){return draw->stats;}

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

void draw_material_gradient(DrawContext *d,DrawRect area,DrawColor a,DrawColor b,int horizontal){
    int w,h,l,t,r,bottom;DrawColor *p=pixels(d,&w,&h);if(!p)return;
    DrawPoint from=point(d,(DrawPoint){area.x,area.y}),to=point(d,(DrawPoint){area.x+area.width,area.y+area.height});
    int left=(int)ceilf(from.x),top=(int)ceilf(from.y),right=(int)ceilf(to.x),end=(int)ceilf(to.y);
    bounds(d,w,h,&l,&t,&r,&bottom);
    for(int y=top>t?top:t;y<end && y<bottom;y++)for(int x=left>l?left:l;x<right && x<r;x++){
        int length=horizontal?right-left:end-top;float f=length>1?(horizontal?x-left:y-top)/(float)(length-1):0;
        DrawColor c={a.r+(b.r-a.r)*f,a.g+(b.g-a.g)*f,a.b+(b.b-a.b)*f,a.a+(b.a-a.a)*f};
        p[(size_t)y*w+x]=blend(p[(size_t)y*w+x],luice_palette_map(c,x,y,1));
    }
}
