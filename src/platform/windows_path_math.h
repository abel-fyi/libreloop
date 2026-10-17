// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_WINDOWS_PATH_MATH_H
#define LIBRELOOP_WINDOWS_PATH_MATH_H
#include <stddef.h>
int windows_path_absolute(const char *path);
const char *windows_path_basename(const char *path);
int windows_path_parent(const char *path,char *out,size_t capacity);
int windows_path_relative(const char *directory,const char *absolute,char *out,size_t capacity);
int windows_path_join(const char *directory,const char *name,char *out,size_t capacity);
#endif
