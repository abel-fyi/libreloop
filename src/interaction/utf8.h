// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_UTF8_H
#define LIBRELOOP_UTF8_H
int utf8_next(const char *text,int *bytes);
const char *utf8_encode(int codepoint,int *bytes);
#endif
