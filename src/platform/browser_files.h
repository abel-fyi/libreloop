// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_BROWSER_FILES_H
#define LIBRELOOP_BROWSER_FILES_H
#include <stddef.h>
typedef struct { unsigned int capacity,count; char **paths; } BrowserFileList;
int platform_browser_directory(const char *path);
int platform_browser_make_directory(const char *path);
void platform_browser_config_base(char *out,size_t capacity);
BrowserFileList platform_browser_files(const char *path);
void platform_browser_files_free(BrowserFileList files);
#endif
