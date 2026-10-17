// SPDX-License-Identifier: GPL-3.0-only
#include "text_edit.h"
#include <stdio.h>
#include <string.h>
static struct {int rename_select_all;} ui;
static void edit_apply(char *text,size_t capacity);
#include "text_edit_sequence.h"
static void edit_apply(char *text,size_t capacity) {
    AppInput input;input_init(&input);
    input.char_count=edit_character_count;
    for(int i=0;i<edit_character_count;i++)input.char_queue[i]=edit_characters[i];
    text_edit(text,capacity,&ui.rename_select_all,(TextEditInput){
        .select_all=edit_shortcut && edit_keys['A'],.paste=edit_shortcut && edit_keys['V'],
        .clipboard=edit_clipboard,.backspace=edit_keys[259],.arrow=edit_keys[263] || edit_keys[262],
        .command_down=edit_command,.input=&input});
    edit_character_read=edit_character_count-input.char_count;
}
int main(void){FILE *f=fopen("text.txt","w");if(!f)return 1;text_edit_sequence(f);return fclose(f)!=0;}
