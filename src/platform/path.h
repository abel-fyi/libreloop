// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_PATH_H
#define LIBRELOOP_PATH_H
/* Preserve original platform capacities; project-format sample paths stay1024. */
#ifdef __APPLE__
#define LL_PATH_CAPACITY 1024
#else
#define LL_PATH_CAPACITY 4096
#endif
#endif
