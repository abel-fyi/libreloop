// SPDX-License-Identifier: GPL-3.0-only
/* Real chooser/filesystem, controlled I/O result, clock and drawing boundary. */
#include <sys/stat.h>
#include <unistd.h>
static char fixture_root[4096],committed_path[4096];
static int fixture_keys[512],fixture_chars[16],fixture_char_count,fixture_char_read;
static int fixture_pressed,fixture_down,io_ok,io_calls,replacement_calls;
static float fixture_wheel;
static double fixture_time;
static const char *fixture_home(const char *key){return !strcmp(key,"HOME")?fixture_root:NULL;}
int project_file_commit(const char *path){io_calls++;snprintf(committed_path,sizeof committed_path,"%s",path);snprintf(ui.status,sizeof ui.status,io_ok?"Saved %s":"Fixture I/O failure: %s",path);return io_ok;}
void replacement_saved(void){replacement_calls++;ui.document.pending_action=0;}
int hover(float x,float y,float w,float h){return ui.input_enabled && ui.mouse.x>=x && ui.mouse.x<x+w && ui.mouse.y>=y && ui.mouse.y<y+h;}
int button(const char *text,int x,int y,int w,int h,int active){(void)text;(void)active;return hover(x,y,w,h) && fixture_pressed;}
void label(const char *text,float x,float y,int size,FIXTURE_COLOR color){(void)text;(void)x;(void)y;(void)size;(void)color;}
int text_width(const char *text,int size){(void)size;return (int)strlen(text)*6;}
const char *fit_text(const char *text,int width,int size){(void)width;(void)size;return text;}
int shortcut_down(void){return 0;}
int command_down(void){return 0;}
static int entry_row(const char *name){for(int i=0;i<ui.file_picker.count;i++)if(!strcmp(name,ui.file_picker.entries[i].name))return i;return -1;}
static void click_row(const char *name){int row=entry_row(name);ui.mouse.x=140;ui.mouse.y=80+66+(row-ui.file_picker.scroll)*24+12;fixture_pressed=fixture_down=1;}
static void trace_text(FILE *out,const char *text){
    const char *p=text;size_t n=strlen(fixture_root);
    while(*p){if(!strncmp(p,fixture_root,n)){fputs("<root>",out);p+=n;}else{fprintf(out,"%02x",(unsigned char)*p);p++;}}
    fputc('|',out);
}
static int touch_fixture(const char *path){FILE *f=fopen(path,"wb");return f && !fclose(f);}
static int file_picker_sequence(FILE *out){
    char template[]="/tmp/libreloop-picker-flow-XXXXXX",path[4096],subfolder[4096],empty[4096],initial[4096],config[4096],locations[4096];
    if(!mkdtemp(template) || !realpath(template,fixture_root))return 1;
    snprintf(subfolder,sizeof subfolder,"%s/Subfolder",fixture_root);snprintf(empty,sizeof empty,"%s/Empty",fixture_root);
    if(mkdir(subfolder,0700) || mkdir(empty,0700))return 1;
    for(int i=0;i<22;i++){snprintf(path,sizeof path,"%s/Song %02d.llp",fixture_root,i);if(!touch_fixture(path))return 1;}
    snprintf(path,sizeof path,"%s/.hidden.llp",fixture_root);if(!touch_fixture(path))return 1;
    snprintf(path,sizeof path,"%s/Audio.wav",fixture_root);if(!touch_fixture(path))return 1;
    snprintf(path,sizeof path,"%s/Nested.llp",subfolder);if(!touch_fixture(path))return 1;
    snprintf(initial,sizeof initial,"%s/Song 01.llp",fixture_root);
    snprintf(config,sizeof config,"%s/folders.txt",fixture_root);snprintf(locations,sizeof locations,"%s/chooser-folders.txt",fixture_root);
    /* Application layout scale; framebuffer DPI/native delivery aren't exercised. */
    for(int dpi=1;dpi<=2;dpi++)for(int mode=0;mode<24;mode++){
        remove(locations);file_chooser_locations(config);memset(&ui,0,sizeof ui);
        io_ok=mode!=10;io_calls=replacement_calls=0;committed_path[0]=0;
        ui.input_enabled=1;ui.pattern_popup=11;ui.document.pending_action=3;
        ui.file_action=mode>=9 && mode<=12?mode==10?6:3:mode==18?7:mode==19?8:mode==20?5:4;
        int save=ui.file_action!=4 && ui.file_action!=7;
        const char *extension=mode==18 || mode==20?"wav":mode==19?"llpreset":"llp";
        int purpose=mode==18?FILE_SAMPLE:mode==19?FILE_PRESET_SAVE:mode==20?FILE_EXPORT:save?FILE_SAVE:FILE_OPEN;
        if(!file_chooser_begin_recent(&ui.file_picker,initial,extension,save,purpose))return 1;
        file_picker_sync();ui.file_field=save?2:0;ui.rename_select_all=save;
        for(int frame=0;frame<3;frame++){
            memset(fixture_keys,0,sizeof fixture_keys);memset(fixture_chars,0,sizeof fixture_chars);fixture_char_count=fixture_char_read=0;
            fixture_pressed=fixture_down=0;fixture_wheel=0;ui.mouse=(FIXTURE_POINT){0,0};fixture_time=10+frame*.1;
            if(mode<=3 && frame<2){click_row(mode==3 && frame==1?"Song 02.llp":"Song 01.llp");if(mode==1)fixture_time=frame==0?0:.35;else if(frame==1)fixture_time=mode==2?9:10.1;}
            if(mode==4 && frame<2)click_row("Subfolder");
            if(mode==5 && frame<2){if(frame==0)ui.file_picker.selected=entry_row("Song 00.llp");fixture_keys[frame==0?265:257]=1;}
            if(mode==6 && frame==0){ui.file_field=2;ui.rename_select_all=1;const char *s="Song 01.llp";while(*s)fixture_chars[fixture_char_count++]=*s++;fixture_keys[257]=1;}
            if((mode==7 || mode==8) && frame==0){ui.file_field=1;snprintf(ui.file_directory,sizeof ui.file_directory,"%s/%s",fixture_root,mode==7?"Subfolder":"Missing");fixture_keys[257]=1;}
            if(mode>=9 && mode<=11){if(frame==0)fixture_keys[257]=1;if(frame==1){if(mode==11){ui.mouse=(FIXTURE_POINT){120+125,80+127};fixture_pressed=fixture_down=1;}else fixture_keys[257]=1;}}
            if(mode==12 && frame==0){ui.mouse=(FIXTURE_POINT){120+620-180,80+420-30};fixture_pressed=fixture_down=1;}
            if(mode==13){ui.mouse=(FIXTURE_POINT){140,160};fixture_wheel=frame==0?-2.25f:frame==1?.25f:100;}
            if(mode==14 && frame<2){ui.mouse=(FIXTURE_POINT){120+620-18,frame==0?80+66+220:80+66+20};fixture_pressed=frame==0;fixture_down=1;}
            if(mode==15 && frame==0){ui.mouse=(FIXTURE_POINT){120+620-70,80+40};fixture_pressed=fixture_down=1;}
            if(mode==16 && frame==0){ui.file_field=2;strcpy(ui.file_picker.name,"Subfolder");fixture_keys[257]=1;}
            if(mode==17 && frame==0){ui.file_field=2;strcpy(ui.file_picker.name,"Missing.llp");fixture_keys[257]=1;}
            if(mode==18 && frame==0){click_row("Audio.wav");fixture_keys[257]=1;}
            if((mode==19 || mode==20) && frame==0){ui.file_field=2;strcpy(ui.file_picker.name,"New Take");fixture_keys[257]=1;}
            if(mode==21 && frame==0){if(!file_chooser_folder(&ui.file_picker,subfolder))return 1;file_picker_sync();ui.mouse=(FIXTURE_POINT){120+20,80+40};fixture_pressed=fixture_down=1;}
            if(mode==22 && frame==0){if(!file_chooser_folder(&ui.file_picker,subfolder))return 1;file_picker_sync();ui.mouse=(FIXTURE_POINT){120+70,80+40};fixture_pressed=fixture_down=1;}
            if(mode==23 && frame==0){if(!file_chooser_folder(&ui.file_picker,empty))return 1;file_picker_sync();fixture_keys[264]=1;}
            fixture_frame_input();
            if(ui.pattern_popup==11)draw_file_picker(120,80,620,420,(float)dpi);
            fprintf(out,"%d %d %d %.17g 120 80 620 420 |%d %d %d %d %d %d %d %d %d %d %d %d %.17g %d %d|",dpi,mode,frame,fixture_time,ui.pattern_popup,ui.file_confirm,ui.file_field,ui.rename_select_all,ui.file_scroll_drag,ui.file_picker.count,ui.file_picker.selected,ui.file_picker.scroll,ui.file_picker.hidden,ui.file_picker.remember,ui.document.pending_action,ui.file_last_row,ui.file_last_click,io_calls,replacement_calls);
            trace_text(out,ui.file_picker.directory);trace_text(out,ui.file_directory);trace_text(out,ui.file_picker.name);trace_text(out,ui.file_pending);trace_text(out,ui.file_picker.error);trace_text(out,ui.status);trace_text(out,committed_path);fputc('\n',out);
            /* The carried original main loop closes a dismissed chooser here. */
            if(ui.pattern_popup!=11 && ui.file_picker.entries)file_chooser_close(&ui.file_picker);
        }
        file_chooser_close(&ui.file_picker);
        FILE *saved=fopen(locations,"r");fprintf(out,"remember %d %d|",dpi,mode);
        if(saved){char line[4096];while(fgets(line,sizeof line,saved))trace_text(out,line);fclose(saved);}fputc('\n',out);
    }
    remove(locations);for(int i=0;i<22;i++){snprintf(path,sizeof path,"%s/Song %02d.llp",fixture_root,i);remove(path);}
    snprintf(path,sizeof path,"%s/.hidden.llp",fixture_root);remove(path);snprintf(path,sizeof path,"%s/Audio.wav",fixture_root);remove(path);snprintf(path,sizeof path,"%s/Nested.llp",subfolder);remove(path);
    return rmdir(subfolder) || rmdir(empty) || rmdir(fixture_root);
}
