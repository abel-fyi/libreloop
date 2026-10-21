// SPDX-License-Identifier: GPL-3.0-only
#define _XOPEN_SOURCE 700
#include "../../src/ui/frontend/ui_internal.h"
UiState ui;
#define FIXTURE_COLOR DrawColor
#define FIXTURE_POINT DrawPoint
static void fixture_frame_input(void);
#include "file_picker_sequence.h"
static AppInput frame_input;
static void fixture_frame_input(void){input_init(&frame_input);frame_input.char_count=fixture_char_count;for(int i=0;i<fixture_char_count;i++)frame_input.char_queue[i]=fixture_chars[i];}
AppInput *host_input(void){return &frame_input;}
int host_is_key_pressed(int key){return fixture_keys[key];}
int mouse_pressed(int button){return button==0 && fixture_pressed;}
int host_is_mouse_button_down(int button){return button==0 && fixture_down;}
float host_get_mouse_wheel_move(void){return fixture_wheel;}
double host_get_time(void){return fixture_time;}
const char *host_get_clipboard_text(void){return NULL;}
#define host_draw_rectangle(...) ((void)0)
#define host_draw_rectangle_rec(...) ((void)0)
#define host_draw_rectangle_lines(...) ((void)0)
#define host_draw_rectangle_lines_ex(...) ((void)0)
#define host_begin_scissor_mode(...) ((void)0)
#define ui_surface(...) ((void)0)
#define ui_text_well(...) ((void)0)
#define ui_folder_icon(...) ((void)0)
#define ui_scrollbar(...) ((void)0)
#define ui_bevel(...) ((void)0)
int button_color(const char *text,int x,int y,int w,int h,int active,DrawColor base){(void)base;return button(text,x,y,w,h,active);}
#define getenv fixture_home
#include "../../src/ui/frontend/ui_menus.c"
int main(void){FILE *f=fopen("picker.txt","wb");if(!f)return 1;int result=file_picker_sequence(f);return fclose(f) || result;}
