// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_FRAME_WAIT_H
#define LIBRELOOP_FRAME_WAIT_H
/* UI-thread limiter: original configured raylib wait, with busy wait disabled. */
void platform_frame_wait(double seconds);
#endif
