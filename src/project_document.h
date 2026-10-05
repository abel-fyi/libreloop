// SPDX-License-Identifier: GPL-3.0-only
#ifndef PROJECT_DOCUMENT_H
#define PROJECT_DOCUMENT_H
#include "engine.h"
enum { REPLACE_NEW=1,REPLACE_DEMO,REPLACE_OPEN,REPLACE_PATH,REPLACE_QUIT };
typedef struct {
    Project saved;
    char path[PATH_MAX],export_path[PATH_MAX],replacement_path[PATH_MAX];
    int saved_on_disk,pending_action,quit;
} ProjectDocument;
int project_document_dirty(const ProjectDocument *document,const Project *project);
/* Returns 1 when the UI must prompt. The pending action survives failed saves. */
int project_document_request(ProjectDocument *document,const Project *project,int action);
void project_document_saved(ProjectDocument *document,const Project *project,const char *path);
#endif
