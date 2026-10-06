// SPDX-License-Identifier: GPL-3.0-only
#ifndef FILE_CHOOSER_H
#define FILE_CHOOSER_H
#include <limits.h>
#include <stddef.h>
typedef struct { char *name; int directory; } FileEntry;
typedef struct {
    char directory[PATH_MAX],name[PATH_MAX],extension[16],error[160];
    FileEntry *entries;
    int count,selected,scroll,save,hidden,purpose,remember;
} FileChooser;
enum { FILE_OPEN, FILE_SAVE, FILE_EXPORT, FILE_SAMPLE, FILE_PRESET_OPEN, FILE_PRESET_SAVE, FILE_PURPOSES };
/* Optional persistent locations, stored beside the Browser configuration. */
void file_chooser_locations(const char *browser_config);
int file_chooser_begin_recent(FileChooser *c,const char *initial,const char *extension,int save,int purpose);
void file_chooser_remember(FileChooser *c,const char *chosen);
/* Initialize FileChooser to zero before the first begin call. */
int file_chooser_begin(FileChooser *c,const char *initial,const char *extension,int save);
int file_chooser_folder(FileChooser *c,const char *path);
int file_chooser_parent(FileChooser *c);
/* -1 invalid, 0 navigated into a directory, 1 ready, 2 needs overwrite confirmation. */
int file_chooser_path(FileChooser *c,char *path,size_t capacity);
void file_chooser_close(FileChooser *c);
#endif
