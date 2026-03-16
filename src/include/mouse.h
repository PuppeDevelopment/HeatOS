#ifndef MOUSE_H
#define MOUSE_H

#include "types.h"

typedef struct {
    int16_t dx;
    int16_t dy;
    bool left;
    bool right;
    bool middle;
} mouse_packet_t;

bool mouse_init(void);
bool mouse_is_ready(void);
void mouse_set_reporting(bool enabled);
bool mouse_poll(mouse_packet_t *out);

#endif
