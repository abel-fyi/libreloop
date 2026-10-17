// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_TUBE_H
#define LIBRELOOP_TUBE_H
/* Tube-inspired saturation, fixed 48 kHz, fixed storage and no render allocation. */
typedef struct {float drive,tone,output;} TubeSettings; /* dB, Hz, dB */
typedef struct {double x,integral;} TubeStage;
typedef struct {
    TubeStage stage[2][2];
    float pre[2],inter[2],tone_state[2],dc_input[2],dc_output[2],envelope[2];
    float drive,tone,output,wet,gain,makeup,tone_coefficient;
    float gain_step,makeup_step,tone_step;
    unsigned tick;int ready;
} Tube;
TubeSettings tube_default(void);
int tube_valid(TubeSettings settings);
void tube_reset(Tube *tube);
void tube_process(Tube *tube,TubeSettings settings,float wet,float stereo[2]);
#endif
