// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_SCROLL_INPUT_H
#define LIBRELOOP_SCROLL_INPUT_H
#include "navigation.h"
typedef struct { NavigationInput wheel,native; } ScrollInput;
void scroll_input_begin(ScrollInput *input);
void scroll_input_wheel(ScrollInput *input,double x,double y);
void scroll_input_native(ScrollInput *input,double x,double y,int precise);
void scroll_input_magnify(ScrollInput *input,double magnification);
NavigationInput scroll_input_navigation(const ScrollInput *input);
float scroll_input_scalar(const ScrollInput *input);
#endif
