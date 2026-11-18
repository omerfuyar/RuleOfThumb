#include "lcd.h"

#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_heap_caps.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

// ---- wiring ---------------------------------------------------------------
#define PIN_SCK   18
#define PIN_MOSI  23 // labelled "SDA" on the module
#define PIN_CS    5
#define PIN_DC    2  // labelled "A0" / "RS" / "DC"
// RST is tied to 3.3 V; we use the software reset command instead.

#define LCD_HOST     SPI3_HOST          // VSPI: pins 18/23 are its native pins
#define LCD_CLOCK_HZ (26 * 1000 * 1000) // drop to 10 MHz if you see garbage

// ---- panel variant knobs (see notes at the bottom) --------------------------
#define MADCTL   0xA8 // landscape + BGR. Try 0x68 if mirrored/upside down; clear 0x08 if red/blue swapped
#define OFFSET_X 0    // "green tab" modules often need 1 or 2 here
#define OFFSET_Y 0

static const char *TAG = "lcd";

static esp_lcd_panel_io_handle_t io;
static uint16_t                 *fb[2];
static int                       back;
static SemaphoreHandle_t         xfer_done;

// Called from the SPI interrupt when a whole frame has finished sending.
static bool on_color_done(esp_lcd_panel_io_handle_t h, esp_lcd_panel_io_event_data_t *e, void *ctx)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(xfer_done, &woken);
    return woken == pdTRUE;
}

typedef struct {
    uint8_t  cmd;
    uint8_t  len;
    uint8_t  data[16];
    uint16_t delay_ms;
} init_cmd_t;

// ST7735S init sequence (same values the common Adafruit driver uses).
static const init_cmd_t init_seq[] = {
    {0x01, 0, {0}, 150},                                   // SWRESET
    {0x11, 0, {0}, 150},                                   // SLPOUT (needs >=120 ms)
    {0xB1, 3, {0x01, 0x2C, 0x2D}, 0},                      // FRMCTR1 frame rate
    {0xB2, 3, {0x01, 0x2C, 0x2D}, 0},                      // FRMCTR2
    {0xB3, 6, {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D}, 0},    // FRMCTR3
    {0xB4, 1, {0x07}, 0},                                  // INVCTR
    {0xC0, 3, {0xA2, 0x02, 0x84}, 0},                      // PWCTR1 power
    {0xC1, 1, {0xC5}, 0},                                  // PWCTR2
    {0xC2, 2, {0x0A, 0x00}, 0},                            // PWCTR3
    {0xC3, 2, {0x8A, 0x2A}, 0},                            // PWCTR4
    {0xC4, 2, {0x8A, 0xEE}, 0},                            // PWCTR5
    {0xC5, 1, {0x0E}, 0},                                  // VMCTR1
    {0x20, 0, {0}, 0},                                     // INVOFF
    {0x36, 1, {MADCTL}, 0},                                // MADCTL orientation
    {0x3A, 1, {0x05}, 0},                                  // COLMOD: 16-bit RGB565
    {0xE0, 16, {0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D,
                0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10}, 0}, // gamma +
    {0xE1, 16, {0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D,
                0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10}, 0}, // gamma -
    {0x13, 0, {0}, 10},                                    // NORON
    {0x29, 0, {0}, 100},                                   // DISPON
};

static void send(uint8_t cmd, const uint8_t *data, size_t len)
{
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io, cmd, data, len));
}

void lcd_init(void)
{
    // 1. The SPI peripheral itself. MISO is unused: the display is write-only.
    spi_bus_config_t bus = {
        .sclk_io_num     = PIN_SCK,
        .mosi_io_num     = PIN_MOSI,
        .miso_io_num     = -1,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = LCD_W * LCD_H * 2,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus, SPI_DMA_CH_AUTO));

    // 2. An "LCD IO" on top of it: knows to drive DC low for commands, high for data.
    xfer_done = xSemaphoreCreateBinary();
    xSemaphoreGive(xfer_done); // nothing in flight yet

    esp_lcd_panel_io_spi_config_t cfg = {
        .cs_gpio_num         = PIN_CS,
        .dc_gpio_num         = PIN_DC,
        .spi_mode            = 0,
        .pclk_hz             = LCD_CLOCK_HZ,
        .trans_queue_depth   = 4,
        .lcd_cmd_bits        = 8,
        .lcd_param_bits      = 8,
        .on_color_trans_done = on_color_done,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &cfg, &io));

    // 3. Wake the controller up and configure it.
    for (size_t i = 0; i < sizeof(init_seq) / sizeof(init_seq[0]); i++) {
        send(init_seq[i].cmd, init_seq[i].len ? init_seq[i].data : NULL, init_seq[i].len);
        if (init_seq[i].delay_ms)
            vTaskDelay(pdMS_TO_TICKS(init_seq[i].delay_ms));
    }

    // 4. Drawing window = whole screen. Set once; every RAMWR restarts at its top-left.
    uint16_t x0 = OFFSET_X, x1 = OFFSET_X + LCD_W - 1;
    uint16_t y0 = OFFSET_Y, y1 = OFFSET_Y + LCD_H - 1;
    send(0x2A, (uint8_t[]){x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF}, 4); // CASET
    send(0x2B, (uint8_t[]){y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF}, 4); // RASET

    // 5. Two framebuffers in DMA-capable internal RAM (40 KB each).
    for (int i = 0; i < 2; i++) {
        fb[i] = heap_caps_calloc(1, LCD_W * LCD_H * 2, MALLOC_CAP_DMA);
        if (!fb[i]) {
            ESP_LOGE(TAG, "framebuffer allocation failed");
            abort();
        }
    }
    ESP_LOGI(TAG, "ST7735 ready, %dx%d @ %d MHz", LCD_W, LCD_H, LCD_CLOCK_HZ / 1000000);
}

uint16_t *lcd_backbuffer(void) { return fb[back]; }

void lcd_present(void)
{
    // Wait until the previous frame has left the other buffer, then send this one.
    // tx_color returns immediately; DMA streams the pixels while we draw the next frame.
    xSemaphoreTake(xfer_done, portMAX_DELAY);
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_color(io, 0x2C /* RAMWR */, fb[back], LCD_W * LCD_H * 2));
    back ^= 1;
}

/*
 * Panel variant notes — cheap 1.8" modules differ:
 *   - Red and blue swapped           -> toggle the 0x08 (BGR) bit in MADCTL
 *   - Mirrored or upside down        -> try MADCTL 0x68, 0xE8, 0x28
 *   - 1-2 px garbage line on an edge -> set OFFSET_X / OFFSET_Y to 1 or 2
 *   - Colours look inverted          -> change 0x20 (INVOFF) to 0x21 (INVON)
 */
