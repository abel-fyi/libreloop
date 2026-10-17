// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_TRANSPORT_READOUT_H
#define LIBRELOOP_TRANSPORT_READOUT_H
#include <stdint.h>
#include <stdio.h>
/* Sample-position time, independent of BPM and UI frame rate. Integer
   remainder avoids floating point drift and multiplication overflow. */
static inline void transport_seconds(char *out,size_t capacity,uint64_t frames,unsigned rate) {
    if(!rate){snprintf(out,capacity,"0.000");return;}
    snprintf(out,capacity,"%llu.%03llu",(unsigned long long)(frames/rate),
        (unsigned long long)((frames%rate)*1000/rate));
}
#endif
