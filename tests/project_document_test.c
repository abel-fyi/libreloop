// SPDX-License-Identifier: GPL-3.0-only
#include "project_document.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project project;
static ProjectDocument document;
int main(void) {
    project_new(&project); project_document_saved(&document,&project,"song.llp");
    CHECK(!project_document_dirty(&document,&project));
    project.audio_seconds[0]=2; CHECK(!project_document_dirty(&document,&project));
    project.bpm=130; CHECK(project_document_request(&document,&project,REPLACE_QUIT));
    CHECK(!document.quit && document.pending_action==REPLACE_QUIT);
    /* Cancel and retry; a failed save never marks the document clean or quits. */
    document.pending_action=0;
    CHECK(project_document_request(&document,&project,REPLACE_QUIT)); CHECK(!document.quit);
    project_document_saved(&document,&project,NULL);
    CHECK(document.pending_action==REPLACE_QUIT && !project_document_dirty(&document,&project));
    CHECK(!project_document_request(&document,&project,REPLACE_QUIT) && document.quit);
    CHECK(document.saved_on_disk && !strcmp(document.path,"song.llp"));
    puts("Close requests require a decision for dirty projects and preserve pending actions until saved or canceled."); return 0;
}
