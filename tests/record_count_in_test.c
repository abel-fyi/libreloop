// SPDX-License-Identifier: GPL-3.0-only
#include "record_count_in.h"
#include <math.h>
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void) {
    RecordCountIn c={0};CHECK(record_count_in_remaining(&c,0)==0);
    CHECK(record_count_in_begin(&c,4,120,10));CHECK(c.end==10.5);
    CHECK(record_count_in_remaining(&c,10)==4 && record_count_in_remaining(&c,10.125)==3);
    CHECK(record_count_in_remaining(&c,10.499)==1 && record_count_in_remaining(&c,10.5)==0);
    CHECK(record_count_in_remaining(&c,11)==0 && record_count_in_remaining(&c,9)==4);
    CHECK(record_count_in_begin(&c,7,90,20));CHECK(fabs(c.end-20-7*15.0/90)<1e-12);
    CHECK(!record_count_in_begin(&c,0,120,0) && !record_count_in_begin(&c,4097,120,0));
    CHECK(!record_count_in_begin(&c,1,NAN,0) && !record_count_in_begin(&c,1,0,0));
    CHECK(!record_count_in_begin(&c,1,120,INFINITY));c.active=0;CHECK(!record_count_in_remaining(&c,20));
    return 0;
}
