#include "input.h"
#include "driver/gpio.h"

// Index i in this table becomes bit (1 << i) in input_read().
// Order matches BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT — swap numbers to remap.
static const gpio_num_t pins[] = {GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_4, GPIO_NUM_15};
#define NPINS (sizeof(pins) / sizeof(pins[0]))

void input_init(void)
{
    uint64_t mask = 0;
    for (int i = 0; i < NPINS; i++)
        mask |= 1ULL << pins[i];

    gpio_config_t cfg = {
        .pin_bit_mask = mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, // ~45 kOhm internal pull-up: idle = 1
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE, // we poll once per frame
    };
    gpio_config(&cfg);
}

uint32_t input_read(void)
{
    uint32_t held = 0;
    for (int i = 0; i < NPINS; i++)
        if (gpio_get_level(pins[i]) == 0) // active-low: pressed pulls the pin to GND
            held |= 1u << i;
    return held;
}
