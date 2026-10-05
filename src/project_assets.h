// SPDX-License-Identifier: GPL-3.0-only
#ifndef PROJECT_ASSETS_H
#define PROJECT_ASSETS_H
#include "engine.h"
/* Resolve relative references against the project, independent of launch directory. */
int project_sample_path(const char *project_path,const char *reference,char *out,size_t capacity);
/* Successful collection rebinds references to the new companion files for later saves. */
int project_save_assets(const char *path,Project *project,const Sample originals[CHANNELS],int collect);
typedef int (*ProjectSampleLoad)(const char *path,Sample *sample);
/* Prepares a replacement without changing the current project; absent audio retains its reference. */
int project_load_assets(const char *path,Project *project,Sample originals[CHANNELS],Sample processed[CHANNELS],ProjectSampleLoad load,int *missing);
#endif
