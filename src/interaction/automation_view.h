// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_AUTOMATION_VIEW_H
#define LIBRELOOP_AUTOMATION_VIEW_H
#include "engine.h"
typedef struct { float shown[AUTOMATIONS];uint8_t visible[AUTOMATIONS]; } AutomationView;
void automation_view_update(AutomationView *view,const Project *project,int playing,int song,double step);
float automation_displayed_value(const AutomationView *view,const Project *project,int playing,int song,const void *pointer,float manual);
#endif
