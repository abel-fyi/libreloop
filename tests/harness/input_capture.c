// SPDX-License-Identifier: GPL-3.0-only
#include "input.h"
#include <stdio.h>
static AppInput input;
static FILE *out,*clicks;
static void apply(InputEvent event){input_event(&input,&event);}
#define CLICK_BEGIN() input_clicks_begin(&input)
#define CLICK(b) input_click(&input,b)
#define PENDING(b) input.pending_clicks[b]
#define POINT InputPoint
#define BEGIN(f) input_begin(&input,125000000+(f)*16666667)
#define KEY(k,a,l) apply((InputEvent){.kind=INPUT_KEY,.data.key={k,a,l}})
#define CHAR(c) apply((InputEvent){.kind=INPUT_TEXT,.data.codepoint=c})
#define BUTTON(b,a) apply((InputEvent){.kind=INPUT_BUTTON,.data.button={b,a}})
#define POINTER(x,y) apply((InputEvent){.kind=INPUT_POINTER,.data.pointer={x,y}})
#define WHEEL(x,y) apply((InputEvent){.kind=INPUT_WHEEL,.data.scroll={x,y,0}})
#define TRANSFORM(x,y,sx,sy) do{input.offset=(InputPoint){x,y};input.scale=(InputPoint){sx,sy};}while(0)
#define POSITION() input_pointer(&input)
#define DELTA() input_delta(&input)
#define WHEEL_VECTOR() ((InputPoint){input.scroll.wheel.x,input.scroll.wheel.y})
#define WHEEL_SCALAR() scroll_input_scalar(&input.scroll)
#define PRESSED(k) input_key_pressed(&input,k)
#define REPEAT(k) input_key_repeat(&input,k)
#define DOWN(k) input_key_down(&input,k)
#define RELEASED(k) input_key_released(&input,k)
#define UP(k) input_key_up(&input,k)
#define BPRESSED(b) input_button_pressed(&input,b)
#define BDOWN(b) input_button_down(&input,b)
#define BRELEASED(b) input_button_released(&input,b)
#define BUP(b) input_button_up(&input,b)
#define NEXT_KEY() input_next_key(&input)
#define NEXT_CHAR() input_next_char(&input)

static void run(void) {
    const int queries[]={-1,0,1,32,65,66,67,68,69,80,90,256,257,280,282,340,341,343,348,511,512};
    for(int frame=0;frame<9;frame++) {
        BEGIN(frame);
        if(frame==0) {
            POINTER(13.123456789,-8.987654321);POINTER(24.123456789,32.987654321);
            KEY(65,1,0);KEY(65,0,0);KEY(66,1,0);KEY(0,1,0);KEY(-1,1,0);
            for(int burst=0;burst<5;burst++){BUTTON(0,1);BUTTON(0,0);}BUTTON(2,1);WHEEL(4,1);WHEEL(-.25,.25);
            for(int c=0;c<20;c++)CHAR(32+c);
        }else if(frame==1) {
            KEY(66,2,0);KEY(66,2,0);KEY(67,2,0);
            POINTER(24.987654321,32.123456789);WHEEL(-.1,0);
        }else if(frame==2) {
            KEY(66,0,0);KEY(68,1,0);KEY(68,0,0);KEY(280,1,1);KEY(280,0,1);KEY(282,1,2);KEY(282,0,2);
            BUTTON(2,0);BUTTON(1,1);for(int k=0;k<20;k++)KEY(80+k,1,0);
        }else if(frame==3) {
            for(int k=80;k<100;k++)KEY(k,0,0);BUTTON(1,0);KEY(280,0,0);KEY(282,0,0);
            for(int c=0;c<20;c++)CHAR(0x3bb+c);WHEEL(100.000001,99.99999);WHEEL(.1,-.2);
        }else if(frame==4) {
            KEY(340,1,0);KEY(341,1,0);KEY(343,1,0);BUTTON(7,1);
            POINTER(500.125,1000.75);TRANSFORM(-13,8,.5f,2);
        }else if(frame==5) {
            KEY(340,0,0);KEY(341,0,0);KEY(343,0,0);BUTTON(7,0);
            POINTER(-100.5,500.25);TRANSFORM(0,0,1,1);
        }else if(frame==6) {
            KEY(511,1,0);KEY(511,2,0);KEY(1,1,0);CHAR(0x10ffff);CHAR(0);
            WHEEL(0,-1);WHEEL(0,0);BUTTON(0,1);
        }else if(frame==7) {
            KEY(511,0,0);KEY(511,1,0);BUTTON(0,0);BUTTON(0,1);
        }
        CLICK_BEGIN();fprintf(clicks,"frame %d",frame);for(int b=0;b<8;b++)fprintf(clicks," %d %d",CLICK(b),PENDING(b));fprintf(clicks,"\n");
        POINT p=POSITION(),d=DELTA(),w=WHEEL_VECTOR();
        fprintf(out,"frame %d pointer %.9g %.9g delta %.9g %.9g wheel %.9g %.9g scalar %.9g\n",frame,p.x,p.y,d.x,d.y,w.x,w.y,WHEEL_SCALAR());
        for(int k=0;k<21;k++){int key=queries[k];fprintf(out,"key %d %d %d %d %d %d\n",key,PRESSED(key),REPEAT(key),DOWN(key),RELEASED(key),UP(key));}
        for(int b=0;b<8;b++)fprintf(out,"button %d %d %d %d %d\n",b,BPRESSED(b),BDOWN(b),BRELEASED(b),BUP(b));
        fprintf(out,"queues ");for(int c=0;c<18;c++)fprintf(out,"%d ",NEXT_KEY());for(int c=0;c<18;c++)fprintf(out,"%d ",NEXT_CHAR());fprintf(out,"\n");
    }
}

int main(void){input_init(&input);out=fopen("input.txt","w");clicks=fopen("clicks.txt","w");if(!out || !clicks)return 1;run();fclose(out);fclose(clicks);return 0;}
