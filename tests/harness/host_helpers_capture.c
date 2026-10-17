// SPDX-License-Identifier: GPL-3.0-only
#include "host.h"
#include <stdio.h>
int main(void) {
    const DrawPoint points[][4]={{{0,0},{2,4},{7,-2},{11,6}},{{100000,-100000},{-300000,2},{.25f,-.5f},{400001,500002}}};
    const float times[]={-1,0,.01f,.125f,.25f,.5f,.75f,.999f,1,1.25f,2};
    FILE *out=fopen("bezier.bin","wb");if(!out)return 1;
    for(unsigned i=0;i<sizeof points/sizeof *points;i++)for(unsigned j=0;j<sizeof times/sizeof *times;j++) {
        DrawPoint p=host_get_spline_point_bezier_cubic(points[i][0],points[i][1],points[i][2],points[i][3],times[j]);fwrite(&p,sizeof p,1,out);
    }
    fclose(out);out=fopen("names.txt","w");if(!out)return 1;
    const char *names[]={"tone.wav","/tmp/tone.ogg","C:\\samples\\tone.flac",".hidden",".hidden.wav","name.","multi.part.wav","plain","a/.name","a/."};
    for(unsigned i=0;i<sizeof names/sizeof *names;i++)fprintf(out,"%s\n",host_get_file_name_without_ext(names[i]));
    fclose(out);return 0;
}
