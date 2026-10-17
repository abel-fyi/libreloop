// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_EFFECT_CHAIN_H
#define LIBRELOOP_EFFECT_CHAIN_H
#include "engine.h"
int effect_slot_after_move(int slot,int from,int to);
int effect_chain_move(Project *project,int bus,int from,int to);
#endif
