#include "keyboard.h"
#include "io.h"

#define KB_DATA_PORT   0x60
#define KB_STATUS_PORT 0x64
#define KB_STATUS_OBF  0x01    /* Output buffer full */
#define KB_STATUS_AUX  0x20    /* Data from mouse (AUX) */

/* US QWERTY scancode set 1 -> ASCII (lowercase) */
static const char scancode_ascii[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', 8,  /* 00-0E */
    9,  'q','w','e','r','t','y','u','i','o','p','[',']', 13, 0,  /* 0F-1D */
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0, '\\',   /* 1E-2B */
    'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ', 0,  /* 2C-3A */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,              /* 3B-46 (F1-F10,Num,Scroll) */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 47-55 */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 56-64 */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 65-73 */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0                        /* 74-7F */
};

/* Shifted characters */
static const char scancode_shift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', 8,
    9,  'Q','W','E','R','T','Y','U','I','O','P','{','}', 13, 0,
    'A','S','D','F','G','H','J','K','L',':','"','~', 0, '|',
    'Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' ', 0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static bool shift_held = false;

void keyboard_init(void) {
    /* Flush the keyboard buffer */
    while (inb(KB_STATUS_PORT) & KB_STATUS_OBF)
        inb(KB_DATA_PORT);
}

bool keyboard_has_key(void) {
    uint8_t status = inb(KB_STATUS_PORT);
    return (status & KB_STATUS_OBF) != 0 && (status & KB_STATUS_AUX) == 0;
}

int keyboard_poll(void) {
    uint8_t status = inb(KB_STATUS_PORT);
    if (!(status & KB_STATUS_OBF))
        return 0;

    /* If controller output belongs to AUX (mouse), drain and ignore it here. */
    if (status & KB_STATUS_AUX) {
        return 0;
    }

    uint8_t sc = inb(KB_DATA_PORT);

    /* Handle extended scancodes (0xE0 prefix) */
    if (sc == 0xE0) {
        /* Wait for the actual keyboard scancode and skip AUX bytes. */
        uint32_t guard = 100000u;
        while (guard--) {
            status = inb(KB_STATUS_PORT);
            if (!(status & KB_STATUS_OBF)) continue;
            if (status & 0x20) {
                (void)inb(KB_DATA_PORT);
                continue;
            }
            sc = inb(KB_DATA_PORT);
            goto decode_extended;
        }
        return 0;

decode_extended:

        if (sc & 0x80) return 0;   /* extended key release */

        switch (sc) {
            case 0x48: return KEY_UP;
            case 0x50: return KEY_DOWN;
            case 0x4B: return KEY_LEFT;
            case 0x4D: return KEY_RIGHT;
        }
        return 0;
    }

    /* Key release (bit 7 set) */
    if (sc & 0x80) {
        uint8_t released = sc & 0x7F;
        if (released == 0x2A || released == 0x36) shift_held = false;
        return 0;
    }

    /* Shift press */
    if (sc == 0x2A || sc == 0x36) { shift_held = true; return 0; }

    /* Special keys */
    switch (sc) {
        case 0x01: return KEY_ESCAPE;
        case 0x0E: return KEY_BACKSPACE;
        case 0x0F: return KEY_TAB;
        case 0x1C: return KEY_ENTER;
        case 0x3B: return KEY_F1;
        case 0x3C: return KEY_F2;
        case 0x3D: return KEY_F3;
        case 0x3E: return KEY_F4;
        case 0x44: return KEY_F10;
        case 0x48: return KEY_UP;
        case 0x50: return KEY_DOWN;
    }

    /* Regular ASCII */
    if (sc < 128) {
        char c = shift_held ? scancode_shift[sc] : scancode_ascii[sc];
        if (c) return (int)(uint8_t)c;
    }
    return 0;
}

int keyboard_wait(void) {
    int key;
    do {
        key = keyboard_poll();
    } while (!key);
    return key;
}
