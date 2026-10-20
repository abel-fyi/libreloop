// SPDX-License-Identifier: GPL-3.0-only
#include "../../src/ui/frontend/ui_internal.h"
#include <errno.h>
UiState ui;
#include "recording_fixture_services.h"
int host_make_directory(const char *p){return directory_fixture(p);}
char *host_realpath(const char *p,char *out){return canonical_fixture(p,out);}
#include "../../src/platform/recording.c"
#include "../../src/ui/frontend/app_recording.c"
/* Frozen replays cover the original Song take/error paths. Explicitly select
   Song and press Record then Play now that Record alone only arms. */
static void replay_record_and_play(void){int song=ui.song;ui.song=1;recording_start();recording_play();if(!recording_active())ui.song=song;}
#define recording_start replay_record_and_play
#include "recording_sequence.h"
int main(int argc,char **argv){if(argc!=2)return 1;int n=atoi(argv[1]);if(n<0 || n>15)return 1;return run_recording(n);}
