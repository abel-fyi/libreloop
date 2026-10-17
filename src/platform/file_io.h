// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_FILE_IO_H
#define LIBRELOOP_FILE_IO_H
#include <stdio.h>
/* These pathname operations require UTF-16 on Windows. Ordinary portable
   stream operations remain C library calls; POSIX keeps the original calls. */
#ifdef _WIN32
FILE *platform_file_open(const char *path,const char *mode);
int platform_file_remove(const char *path);
int platform_file_rename(const char *source,const char *destination);
#else
#define platform_file_open fopen
#define platform_file_remove remove
#define platform_file_rename rename
#endif
#endif
