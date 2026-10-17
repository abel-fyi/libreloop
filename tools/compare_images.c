// SPDX-License-Identifier: GPL-3.0-only
/* Image differences are measurements, not acceptance thresholds. */
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
int main(int argc,char **argv) {
    if(argc!=3)return 1;int w,h,c,ww,hh,cc;unsigned char *a=stbi_load(argv[1],&w,&h,&c,4),*b=stbi_load(argv[2],&ww,&hh,&cc,4);
    if(!a || !b || w!=ww || h!=hh){fprintf(stderr,"Image load/dimensions differ\n");free(a);free(b);return 2;}
    uint64_t different=0,total_error=0;unsigned maximum=0;
    for(size_t p=0;p<(size_t)w*h;p++){int changed=0;for(int k=0;k<4;k++){unsigned x=a[p*4+k],y=b[p*4+k],error=x>y?x-y:y-x;changed|=error!=0;total_error+=error;if(error>maximum)maximum=error;}different+=changed;}
    printf("{\"width\":%d,\"height\":%d,\"different_pixels\":%llu,\"total_pixels\":%llu,\"max_channel_error\":%u,\"mean_absolute_channel_error\":%.8f}\n",w,h,(unsigned long long)different,(unsigned long long)((uint64_t)w*h),maximum,total_error/(double)((uint64_t)w*h*4));
    free(a);free(b);return 0;
}
