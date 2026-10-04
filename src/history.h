// SPDX-License-Identifier: GPL-3.0-only
#ifndef HISTORY_H
#define HISTORY_H
#include "engine.h"
#define HISTORY_LIMIT 64
#define HISTORY_BYTES ((size_t)256*1024*1024)
typedef struct HistoryEntry HistoryEntry;
typedef struct { HistoryEntry *entries[HISTORY_LIMIT]; int count,cursor; size_t bytes; } History;
/* Source stamps identify immutable original PCM, even if an allocator reuses its address. */
int history_capture(History *h,const Project *p,const Sample sources[CHANNELS],const uint64_t stamps[CHANNELS]);
int history_peek(const History *h,int direction,Project *p,Sample sources[CHANNELS]);
int history_source_matches(const History *h,int direction,int channel,Sample source,uint64_t stamp);
void history_rebind(History *h,const Sample sources[CHANNELS],const uint64_t stamps[CHANNELS]);
void history_step(History *h,int direction);
void history_clear(History *h);
#endif
