// SPDX-License-Identifier: GPL-3.0-only
#include "../../src/ui/frontend/ui_internal.h"
#include "files.h"
#include "text_legacy.h"
UiState ui;
static void file_picker_submit_direct(void);
#include "project_commit_sequence.h"
void host_wake(void){}
int host_thread_join(PlatformThread *thread,void **result){(void)thread;(void)result;return 0;}
char *host_realpath(const char *path,char *out){return platform_canonical_path(path,out,LL_PATH_CAPACITY)?out:NULL;}
const char *host_get_file_name(const char *path){return legacy_file_name(path);}
const char *host_get_file_name_without_ext(const char *path){return legacy_file_stem(path);}
#include "../../src/ui/frontend/app_project.c"
int file_picker_commit(void){if(!project_file_commit(ui.file_pending))return 0;file_chooser_remember(&ui.file_picker,ui.file_pending);return 1;}
static void file_picker_submit_direct(void){
    ui.pattern_popup=11;ui.file_confirm=1;ui.project.master=.7f;
    if(file_picker_commit()){ui.pattern_popup=0;if(ui.file_action==2 || ui.file_action==3 || ui.file_action==6)replacement_saved();}
    else{ui.file_confirm=0;snprintf(ui.file_picker.error,sizeof ui.file_picker.error,"%.159s",ui.status);}
}
int main(int argc,char **argv){if(argc!=2)return 1;int mode=atoi(argv[1]);if(mode<0 || mode>11)return 1;return project_commit_sequence(mode);}
