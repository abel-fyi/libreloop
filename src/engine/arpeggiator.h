// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_ARPEGGIATOR_H
#define LIBRELOOP_ARPEGGIATOR_H
#include <stdint.h>
enum { ARP_OFF,ARP_UP,ARP_DOWN,ARP_UP_DOWN,ARP_BOUNCE,ARP_RANDOM,ARP_DIRECTIONS };
enum { ARP_AUTO_SUSTAIN,ARP_AUTO,ARP_OCTAVES,ARP_MAJOR,ARP_MINOR,ARP_MAJOR7,ARP_MINOR7,ARP_DOM7,ARP_SUS2,ARP_SUS4,ARP_DIM,ARP_AUG,ARP_CHORDS };
typedef struct { float direction,time,gate,range,repeat,chord,slide; } ArpSettings;
typedef struct { uint32_t id;int channel,pitch,lane,pattern,note_id;float velocity;double remaining;uint64_t order;int active; } ArpHeld;
typedef struct { double phase;uint64_t step;uint32_t random;int running,slot,single,pitch,pending;uint64_t root_order; } ArpChannel;
typedef struct { ArpHeld held[128];ArpChannel channels[32];int count,counts[32]; } ArpState;
ArpSettings arp_default(void);
int arp_valid(ArpSettings settings);
const char *arp_direction_name(unsigned direction);
const char *arp_chord_name(unsigned chord);
#endif
