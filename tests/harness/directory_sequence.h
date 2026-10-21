// SPDX-License-Identifier: GPL-3.0-only
#include <sys/stat.h>
#include <unistd.h>
static int is_directory(const char *path){struct stat s;return !stat(path,&s) && S_ISDIR(s.st_mode);}
static int directory_sequence(FILE *out){
    const char *paths[]={NULL,"",".","nested/A/B","slashes\\A\\B","blocked/A","trailing/","colon:/A","dot/../back","repeat//A///B","end\\"};
    const char *checks[]={"nested","nested/A","nested/A/B","slashes","slashes/A","slashes/A/B","slashes\\A\\B","blocked/A","trailing","colon:","colon:/A","dot","back","repeat","repeat/A","repeat/A/B","end","end\\"};
    FILE *blocked=fopen("blocked","wb");if(!blocked || fclose(blocked))return 1;
    for(int i=0;i<11;i++){
        int result=fixture_make_directory(paths[i]);fprintf(out,"%d %d",i,result);
        for(int j=0;j<18;j++)fprintf(out," %d",is_directory(checks[j]));fputc('\n',out);
    }
    /* Absolute nested paths are independent of temporary-root spelling. */
    char cwd[4096],path[4096];if(!getcwd(cwd,sizeof cwd))return 1;
    snprintf(path,sizeof path,"%s/absolute/A/B",cwd);fprintf(out,"absolute %d",fixture_make_directory(path));
    for(const char **p=(const char *[]) {"absolute","absolute/A","absolute/A/B",NULL};*p;p++)fprintf(out," %d",is_directory(*p));fputc('\n',out);
    if(mkdir("unreadable",0700) || chmod("unreadable",0000))return 1;
    fprintf(out,"metadata %d %d %d %d %d %d %d %d\n",
        fixture_directory_exists("nested"),fixture_directory_exists("blocked"),fixture_directory_exists("unreadable"),fixture_directory_exists("missing"),
        fixture_file_exists("nested"),fixture_file_exists("blocked"),fixture_file_exists("unreadable"),fixture_file_exists("missing"));
    return chmod("unreadable",0700)!=0;
}
