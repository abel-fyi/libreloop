// SPDX-License-Identifier: GPL-3.0-only
#include "browser_files.h"
#include "files.h"
#include <stdio.h>
#define fixture_make_directory platform_browser_make_directory
#define fixture_directory_exists platform_browser_directory
#define fixture_file_exists platform_path_exists
#include "directory_sequence.h"
int main(void){FILE *f=fopen("directories.txt","w");if(!f)return 1;int result=directory_sequence(f);return fclose(f) || result;}
