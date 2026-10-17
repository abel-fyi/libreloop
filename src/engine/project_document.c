// SPDX-License-Identifier: GPL-3.0-only
#include "project_document.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>
int project_document_dirty(const ProjectDocument *document,const Project *project) {
    size_t head=offsetof(Project,audio_seconds),tail=head+sizeof project->audio_seconds;
    return memcmp(project,&document->saved,head) ||
        memcmp((const char *)project+tail,(const char *)&document->saved+tail,sizeof *project-tail);
}
int project_document_request(ProjectDocument *document,const Project *project,int action) {
    document->pending_action=action;
    if(project_document_dirty(document,project)) return 1;
    document->pending_action=0;
    if(action==REPLACE_QUIT) document->quit=1;
    return 0;
}
void project_document_saved(ProjectDocument *document,const Project *project,const char *path) {
    document->saved=*project;
    if(path) { snprintf(document->path,sizeof document->path,"%s",path); document->saved_on_disk=1; }
}
