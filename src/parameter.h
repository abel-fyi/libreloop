// SPDX-License-Identifier: GPL-3.0-only
#ifndef PARAMETER_H
#define PARAMETER_H
/* Saved IDs, independent of UI addresses. The device range is reserved for future built-in processors. */
enum { PARAM_CHANNEL_VOLUME=1, PARAM_CHANNEL_PAN, PARAM_CHANNEL_PITCH, PARAM_INSERT_VOLUME, PARAM_INSERT_PAN, PARAM_INSERT_WIDTH, PARAM_MASTER_VOLUME, PARAM_MASTER_WIDTH, PARAM_MASTER_PITCH, PARAM_CHANNEL_MUTE, PARAM_INSERT_MUTE, PARAM_MASTER_MUTE, PARAM_SWING, PARAM_PITCH_RANGE, PARAM_PLUGIN=1024, PARAM_CHORUS_RATE=1025, PARAM_CHORUS_DEPTH, PARAM_EFFECT_MIX, PARAM_FM_RATIO=1101, PARAM_FM_DEPTH, PARAM_FM_ATTACK, PARAM_FM_DECAY, PARAM_FM_SUSTAIN, PARAM_FM_RELEASE, PARAM_EQ_FIRST=1201, PARAM_EQ_LAST=1221 };
typedef struct { unsigned parameter,owner,slot; } ParameterTarget;
/* Common control metadata; independent of project layout and UI addresses. */
enum { PARAMETER_CONTINUOUS, PARAMETER_INTEGER, PARAMETER_TOGGLE, PARAMETER_LOGARITHMIC };
typedef struct {
    unsigned id;
    const char *name;
    float low,high,initial;
    int kind;
} ParameterDescriptor;
const ParameterDescriptor *parameter_descriptor(unsigned id);
#endif
