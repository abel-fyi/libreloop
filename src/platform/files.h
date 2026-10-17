// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_FILES_H
#define LIBRELOOP_FILES_H
#include <stddef.h>
#include "path.h"
enum { PLATFORM_OTHER,PLATFORM_FILE,PLATFORM_DIRECTORY };
typedef struct PlatformDirectory PlatformDirectory;
typedef struct { char name[LL_PATH_CAPACITY]; int kind; } PlatformEntry;
int platform_canonical_path(const char *path,char *out,size_t capacity);
int platform_absolute_path(const char *path);
const char *platform_basename(const char *path);
int platform_join_path(const char *directory,const char *name,char *out,size_t capacity);
int platform_parent_path(const char *path,char *out,size_t capacity);
int platform_relative_path(const char *directory,const char *absolute,char *out,size_t capacity);
int platform_working_directory(char *out,size_t capacity);
int platform_file_kind(const char *path,int *kind);
int platform_path_access(const char *path,int writable);
/* Existence only, including directories; original raylib FileExists/F_OK. */
int platform_path_exists(const char *path);
PlatformDirectory *platform_directory_open(const char *path);
/*1 item,0 end,-1 read error. Unavailable metadata is skipped as in original. */
int platform_directory_next(PlatformDirectory *directory,PlatformEntry *entry);
void platform_directory_close(PlatformDirectory *directory);
/* Expands a ...XXXXXX asset-directory template in place. */
int platform_asset_directory(char *path);
void platform_remove_directory(const char *path);
#endif
