// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_PHASER_H
#define LIBRELOOP_PHASER_H
#include <stddef.h>
typedef struct {float rate,center,depth,feedback,stereo,stages;} PhaserSettings;
typedef struct {
    double phase;
    float state[2][12],last[2],dc_input[2],dc_output[2],coefficient[2];
    float rate,center,depth,feedback,stereo,stages,wet;
    unsigned ready;
} Phaser;
PhaserSettings phaser_default(void);
PhaserSettings phaser_factory(unsigned index);
const char *phaser_factory_name(unsigned index);
int phaser_valid(PhaserSettings settings);
const float *phaser_parameter(const PhaserSettings *settings,unsigned offset);
void phaser_reset(Phaser *phaser);
void phaser_process(Phaser *phaser,PhaserSettings settings,float wet,float stereo[2]);
#endif
