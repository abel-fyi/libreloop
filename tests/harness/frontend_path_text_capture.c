// SPDX-License-Identifier: GPL-3.0-only
#include "../../src/ui/frontend/host.h"
#include <stdio.h>
#include <string.h>
#define fixture_stem host_get_file_name_without_ext
#define fixture_extension host_is_file_extension
#define fixture_format host_text_format
#include "path_text_sequence.h"
int main(void){return path_text_sequence();}
