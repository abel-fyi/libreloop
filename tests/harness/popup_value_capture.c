// SPDX-License-Identifier: GPL-3.0-only
#include "text_edit.h"
#include <stdio.h>
#include <string.h>
#define popup_number text_number_parse
#define popup_name text_rename_name
#include "popup_value_sequence.h"
int main(void){FILE *f=fopen("values.txt","w");if(!f)return 1;popup_value_sequence(f);return fclose(f)!=0;}
