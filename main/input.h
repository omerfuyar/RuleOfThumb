#pragma once
#include <stdint.h>

enum {
    BTN_UP    = 1 << 0,
    BTN_DOWN  = 1 << 1,
    BTN_LEFT  = 1 << 2,
    BTN_RIGHT = 1 << 3,
};

void     input_init(void);
uint32_t input_read(void); // bitmask of buttons currently held
