// SPDX-License-Identifier: GPL-3.0-only
#include "windows_path_math.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Windows path check%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void) {
    char out[256];
    CHECK(windows_path_absolute("C:/samples/tone.wav"));CHECK(windows_path_absolute("\\\\server\\share\\tone.wav"));
    CHECK(!windows_path_absolute("C:relative.wav"));CHECK(!windows_path_absolute("samples/tone.wav"));
    CHECK(windows_path_parent("C:/samples/tone.wav",out,sizeof out) && !strcmp(out,"C:/samples"));
    CHECK(windows_path_parent("C:/samples",out,sizeof out) && !strcmp(out,"C:/"));
    CHECK(windows_path_parent("C:/",out,sizeof out) && !strcmp(out,"C:/"));
    CHECK(windows_path_parent("//server/share/tone.wav",out,sizeof out) && !strcmp(out,"//server/share"));
    CHECK(windows_path_parent("//server/share",out,sizeof out) && !strcmp(out,"//server/share"));
    CHECK(windows_path_relative("C:/projects/song","C:/projects/song/assets/tone.wav",out,sizeof out) && !strcmp(out,"assets/tone.wav"));
    CHECK(windows_path_relative("C:/projects/song","C:/samples/tone.wav",out,sizeof out) && !strcmp(out,"../../samples/tone.wav"));
    CHECK(windows_path_relative("C:/projects/song","D:/samples/tone.wav",out,sizeof out) && !strcmp(out,"D:/samples/tone.wav"));
    CHECK(windows_path_relative("//server/share","//server/share/tone.wav",out,sizeof out) && !strcmp(out,"tone.wav"));
    CHECK(windows_path_relative("//server/share/","//server/share/tone.wav",out,sizeof out) && !strcmp(out,"tone.wav"));
    CHECK(windows_path_relative("//server/share/a","//other/share/tone.wav",out,sizeof out) && !strcmp(out,"//other/share/tone.wav"));
    CHECK(windows_path_relative("C:/a","c:/a/tone.wav",out,sizeof out) && !strcmp(out,"tone.wav"));
    CHECK(windows_path_relative("C:/a","C:/A/tone.wav",out,sizeof out) && !strcmp(out,"../A/tone.wav"));
    CHECK(!windows_path_relative("C:/a","C:/long-file.wav",out,3));
    CHECK(!strcmp(windows_path_basename("C:\\sounds\\tone.wav"),"tone.wav"));
    CHECK(windows_path_join("C:/","tone.wav",out,sizeof out) && !strcmp(out,"C:/tone.wav"));
    puts("Windows drive/UNC roots, cross-volume assets, separators and bounds checked");return 0;
}
