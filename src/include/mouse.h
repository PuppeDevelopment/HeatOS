#ifndef MOUSE_H
#define MOUSE_H

#include "types.h"

#define MOUSE_BTN_LEFT   0x01
#define MOUSE_BTN_RIGHT  0x02
#define MOUSE_BTN_MIDDLE 0x04

int mouse_init(void);
void mouse_get_screen_pos(int *x, int *y);
uint8_t mouse_get_buttons(void);
bool mouse_has_event(void);
void mouse_clear_event(void);
void mouse_get_delta(int *dx, int *dy);
void mouse_set_screen_pos(int x, int y);
void mouse_poll(void);

/* Internal feed hook for keyboard driver AUX bytes. */
void mouse_handle_aux_byte(uint8_t data);

#endif