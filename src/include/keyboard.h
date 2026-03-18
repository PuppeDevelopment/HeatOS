#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

/* Special key codes (above ASCII range) */
#define KEY_UP        0x100
#define KEY_DOWN      0x101
#define KEY_LEFT      0x102
#define KEY_RIGHT     0x103
#define KEY_ENTER     0x104
#define KEY_BACKSPACE 0x105
#define KEY_ESCAPE    0x106
#define KEY_TAB       0x107
#define KEY_F1        0x110
#define KEY_F2        0x111
#define KEY_F3        0x112
#define KEY_F4        0x113
#define KEY_F10       0x119
#define KEY_HOME      0x120
#define KEY_END       0x121
#define KEY_DELETE    0x122
#define KEY_PAGEUP    0x123
#define KEY_PAGEDOWN  0x124

void keyboard_init(void);
int  keyboard_poll(void);     /* Returns 0 if no key, otherwise keycode */
int  keyboard_wait(void);     /* Blocks until key pressed */
bool keyboard_has_key(void);

#endif
