#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "lcd.h"
#include "input.h"

static const char *TAG = "console";

static void fill(uint16_t *fb, uint16_t c)
{
    for (int i = 0; i < LCD_W * LCD_H; i++)
        fb[i] = c;
}

static void rect(uint16_t *fb, int x, int y, int w, int h, uint16_t c)
{
    int x0 = x < 0 ? 0 : x, x1 = x + w > LCD_W ? LCD_W : x + w;
    int y0 = y < 0 ? 0 : y, y1 = y + h > LCD_H ? LCD_H : y + h;
    for (int j = y0; j < y1; j++)
        for (int i = x0; i < x1; i++)
            fb[j * LCD_W + i] = c;
}

static void game_task(void *arg)
{
    int px = LCD_W / 2 - 8, py = LCD_H / 2 - 8;

    TickType_t last   = xTaskGetTickCount();
    int64_t    t_fps  = esp_timer_get_time();
    int        frames = 0;

    for (;;) {
        // --- input & update ---
        uint32_t b = input_read();
        if (b & BTN_UP)    py -= 2;
        if (b & BTN_DOWN)  py += 2;
        if (b & BTN_LEFT)  px -= 2;
        if (b & BTN_RIGHT) px += 2;
        if (px < 0) px = 0;
        if (py < 0) py = 0;
        if (px > LCD_W - 16) px = LCD_W - 16;
        if (py > LCD_H - 16) py = LCD_H - 16;

        // --- render ---
        uint16_t *fb = lcd_backbuffer();
        fill(fb, rgb(10, 10, 40));
        // Orientation / colour test: red top-left, green top-right, blue bottom-left.
        rect(fb, 0, 0, 10, 10, rgb(255, 0, 0));
        rect(fb, LCD_W - 10, 0, 10, 10, rgb(0, 255, 0));
        rect(fb, 0, LCD_H - 10, 10, 10, rgb(0, 0, 255));
        // The player: white, turns yellow while any button is held.
        rect(fb, px, py, 16, 16, b ? rgb(255, 200, 0) : rgb(255, 255, 255));

        lcd_present();

        // --- stats ---
        frames++;
        int64_t now = esp_timer_get_time();
        if (now - t_fps >= 1000000) {
            ESP_LOGI(TAG, "%d fps, buttons=0x%lx", frames, (unsigned long)b);
            frames = 0;
            t_fps  = now;
        }

        xTaskDelayUntil(&last, pdMS_TO_TICKS(16)); // ~60 Hz fixed step
    }
}

void app_main(void)
{
    lcd_init();
    input_init();
    // Core 1 for the game; Wi-Fi/ESP-NOW will live on core 0 later.
    xTaskCreatePinnedToCore(game_task, "game", 4096, NULL, 5, NULL, 1);
}
