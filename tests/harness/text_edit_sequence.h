// SPDX-License-Identifier: GPL-3.0-only
/* Shared stimulus only. Expected results are captured from frozen ui_menus.c. */
static int edit_keys[512],edit_characters[16],edit_character_count,edit_character_read;
static const char *edit_clipboard;
static int edit_shortcut,edit_command;
static void text_edit_sequence(FILE *out) {
    for(int mode=0;mode<24;mode++) {
        char text[64]="a\xc3\xa9\xf0\x9f\x8e\xb5";
        size_t capacity=sizeof text;
        memset(edit_keys,0,sizeof edit_keys);memset(edit_characters,0,sizeof edit_characters);
        edit_shortcut=edit_command=0;edit_clipboard=NULL;
        edit_character_count=edit_character_read=0;ui.rename_select_all=0;
        if(mode<8) {
            edit_keys[259]=(mode&1)!=0;edit_keys[263]=(mode&2)!=0;ui.rename_select_all=(mode&4)!=0;
        } else if(mode<16) {
            edit_shortcut=1;edit_keys['V']=1;ui.rename_select_all=(mode&1)!=0;
            const char *pastes[]={"hello","x\ny","\xc3\xa9\xf0\x9f\x8e\xb5","\xe2"};
            edit_clipboard=pastes[(mode-8)/2];
        } else if(mode==16) {
            edit_shortcut=1;edit_keys['A']=edit_keys['V']=1;edit_clipboard="paste";
            edit_keys[259]=1;
        } else if(mode==17) {
            edit_keys[262]=1;ui.rename_select_all=1;
        } else if(mode==18 || mode==19) {
            strcpy(text,"123");capacity=mode==18?4:6;
            edit_shortcut=1;edit_keys['V']=1;edit_clipboard="\xc3\xa9z";
        } else if(mode==20) {
            text[0]=0;capacity=1;edit_shortcut=1;edit_keys['V']=1;edit_clipboard="x";
        } else if(mode==21) {
            edit_shortcut=1;edit_keys['V']=1;edit_clipboard="\x7fstop";
        } else if(mode==22) {
            edit_shortcut=1;edit_keys['V']=1;ui.rename_select_all=1; /* null clipboard */
        } else {
            edit_command=1;ui.rename_select_all=1;
        }
        if(mode==3 || mode==7 || mode>=16) {
            int chars[]={10,127,'Q',0x00e9,0x1f3b5,0x110000};
            memcpy(edit_characters,chars,sizeof chars);edit_character_count=6;
        }
        edit_apply(text,capacity);
        fprintf(out,"%d %d %d ",mode,ui.rename_select_all,edit_character_read);
        for(size_t n=0;n<strlen(text);n++)fprintf(out,"%02x",(unsigned char)text[n]);
        fputc('\n',out);
    }
}
