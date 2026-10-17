// SPDX-License-Identifier: GPL-3.0-only
#include "text_legacy.h"
#include <stdio.h>
#include <string.h>
static const char *fixture_format(const char *format,...){va_list args;va_start(args,format);const char *text=legacy_text_formatv(format,args);va_end(args);return text;}
#define fixture_stem legacy_file_stem
#define fixture_extension legacy_file_extension
#include "path_text_sequence.h"
int main(void){return path_text_sequence();}
