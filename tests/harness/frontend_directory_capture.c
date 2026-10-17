// SPDX-License-Identifier: GPL-3.0-only
#include "../../src/ui/frontend/host.h"
#include <stdio.h>
#define fixture_make_directory host_make_directory
#define fixture_directory_exists host_directory_exists
#define fixture_file_exists host_file_exists
#include "directory_sequence.h"
int main(void){FILE *f=fopen("directories.txt","w");if(!f)return 1;int result=directory_sequence(f);return fclose(f) || result;}
