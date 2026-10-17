// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_DRAW_CPU_STATS_H
#define LIBRELOOP_DRAW_CPU_STATS_H
#include "draw.h"
#include <stdint.h>
typedef struct {uint64_t presents,uploads,uploaded_pixels,compared_pixels;} DrawCpuStats;
/* Experiment-only diagnostics; not part of the UI/interaction interfaces. */
DrawCpuStats draw_cpu_stats(const DrawContext *draw);
#endif
