#pragma once
#include <stdint.h>

#define LCD_W 160
#define LCD_H 128

// RGB565, byte-swapped: the ESP32 is little-endian but the ST7735
// expects the high byte first on the wire.
static inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    return (uint16_t)((c >> 8) | (c << 8));
}

void      lcd_init(void);
uint16_t *lcd_backbuffer(void); // the buffer to draw into this frame
void      lcd_present(void);    // start DMA of the back buffer, then swap
