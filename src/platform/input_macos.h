// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_INPUT_MACOS_H
#define LIBRELOOP_INPUT_MACOS_H
#include "input.h"
typedef struct NativeInputMac NativeInputMac;
NativeInputMac *platform_macos_input_create(void *ns_window,unsigned int wake_event);
void platform_macos_input_begin(NativeInputMac *monitor);
ScrollInput platform_macos_input_poll(NativeInputMac *monitor);
void platform_macos_input_destroy(NativeInputMac *monitor);
#endif
