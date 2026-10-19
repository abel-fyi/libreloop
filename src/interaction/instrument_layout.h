// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_INSTRUMENT_LAYOUT_H
#define LIBRELOOP_INSTRUMENT_LAYOUT_H
/* The compact sampler puts its waveform below the last control row at 280.
   A 420-unit outer frame leaves 110 units of waveform after the studio
   caption translation (6) and bottom margin (24), at every supported width. */
enum { SAMPLER_COMPACT_WIDTH=520, INSTRUMENT_ENVELOPE_HEIGHT=160 };
static inline float instrument_envelope_top(float height){return height-42-INSTRUMENT_ENVELOPE_HEIGHT;}
static inline float sampler_min_height(int studio) { return studio?420:300; }
static inline int sampler_wave_top(float width,int studio) { return studio && width<SAMPLER_COMPACT_WIDTH?280:252; }
/* Concrete six-op control rows in the carried editor's content coordinates.
   No device/render types; geometry can be supplied to interaction replays. */
typedef struct {
    int spacious;
    float preset,tabs,heading,head,pages,row[2],graph,graph_height,hint;
} SixOpLayout;
static inline SixOpLayout sixop_layout(float width,float height,int studio) {
    SixOpLayout l={.preset=52,.tabs=78,.heading=113,.head=136,.pages=184,
        .row={226,292},.graph=336,.graph_height=height-378,.hint=344};
    if(studio){l.graph=instrument_envelope_top(height);l.graph_height=INSTRUMENT_ENVELOPE_HEIGHT;}
    if(!studio || width<360 || height<640)return l;
    /* 90s.png: 376x654 outer frame, 640-unit content frame after the caption
       translation. Extra height is distributed across real controls rather
       than assigned entirely to the graph. */
    float scale=(height-94)/546;
    l.spacious=1;
    l.preset=52+20*scale;l.tabs=52+56*scale;
    l.heading=52+91*scale;l.head=52+132*scale;l.pages=52+192*scale;
    l.row[0]=52+246*scale;l.row[1]=52+318*scale;
    l.graph=instrument_envelope_top(height);l.graph_height=INSTRUMENT_ENVELOPE_HEIGHT;l.hint=l.graph+8;
    return l;
}
#endif
