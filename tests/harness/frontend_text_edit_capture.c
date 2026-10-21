// SPDX-License-Identifier: GPL-3.0-only
#include "../../src/ui/frontend/ui_internal.h"
UiState ui;
static AppInput frame;
static void edit_apply(char *text,size_t capacity);
#include "text_edit_sequence.h"
int shortcut_down(void){return edit_shortcut;}
int command_down(void){return edit_command;}
int host_is_key_pressed(int key){return edit_keys[key];}
const char *host_get_clipboard_text(void){return edit_clipboard;}
AppInput *host_input(void){return &frame;}
#include "../../src/ui/frontend/ui_menus.c"
static void edit_apply(char *text,size_t capacity) {
    input_init(&frame);frame.char_count=edit_character_count;
    for(int i=0;i<edit_character_count;i++)frame.char_queue[i]=edit_characters[i];
    text_input(text,capacity);
    edit_character_read=edit_character_count-frame.char_count;
}
int main(void){FILE *f=fopen("text.txt","wb");if(!f)return 1;text_edit_sequence(f);return fclose(f)!=0;}
