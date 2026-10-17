// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_TEXT_LEGACY_H
#define LIBRELOOP_TEXT_LEGACY_H
#include <stdarg.h>
const char *legacy_file_name(const char *path);
const char *legacy_file_stem(const char *path);
int legacy_file_extension(const char *path,const char *extensions);
const char *legacy_text_formatv(const char *format,va_list args);
#endif
