#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "display_st7735.h"

static const char *TAG = "display_st7735";

// ST7735 commands
#define ST7735_NOP      0x00
#define ST7735_SWRESET  0x01
#define ST7735_SLPOUT   0x11
#define ST7735_NORON    0x13
#define ST7735_INVOFF   0x20
#define ST7735_INVON    0x21
#define ST7735_DISPOFF  0x28
#define ST7735_DISPON   0x29
#define ST7735_CASET    0x2A
#define ST7735_RASET    0x2B
#define ST7735_RAMWR    0x2C
#define ST7735_MADCTL   0x36
#define ST7735_COLMOD   0x3A
#define ST7735_FRMCTR1  0xB1
#define ST7735_FRMCTR2  0xB2
#define ST7735_FRMCTR3  0xB3
#define ST7735_INVCTR   0xB4
#define ST7735_PWCTR1   0xC0
#define ST7735_PWCTR2   0xC1
#define ST7735_PWCTR3   0xC2
#define ST7735_PWCTR4   0xC3
#define ST7735_PWCTR5   0xC4
#define ST7735_VMCTR1   0xC5
#define ST7735_GMCTRP1  0xE0
#define ST7735_GMCTRN1  0xE1

// Display dimensions
#define ST7735_WIDTH  128
#define ST7735_HEIGHT 160

static spi_device_handle_t spi_handle;
static int16_t cursor_x = 0;
static int16_t cursor_y = 0;
static uint16_t text_color = ST7735_WHITE;
static uint8_t text_size = 1;

// Basic 5x7 font
static const uint8_t font5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // (space)
    0x00, 0x00, 0x5F, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x14, 0x7F, 0x14, 0x7F, 0x14, // #
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
    0x23, 0x13, 0x08, 0x64, 0x62, // %
    0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x1C, 0x22, 0x41, 0x00, // (
    0x00, 0x41, 0x22, 0x1C, 0x00, // )
    0x08, 0x2A, 0x1C, 0x2A, 0x08, // *
    0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x00, 0x50, 0x30, 0x00, 0x00, // ,
    0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x00, 0x08, 0x14, 0x22, 0x41, // <
    0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x41, 0x22, 0x14, 0x08, 0x00, // >
    0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x32, 0x49, 0x79, 0x41, 0x3E, // @
    0x7E, 0x11, 0x11, 0x11, 0x7E, // A
    0x7F, 0x49, 0x49, 0x49, 0x36, // B
    0x3E, 0x41, 0x41, 0x41, 0x22, // C
    0x7F, 0x41, 0x41, 0x22, 0x1C, // D
    0x7F, 0x49, 0x49, 0x49, 0x41, // E
    0x7F, 0x09, 0x09, 0x01, 0x01, // F
    0x3E, 0x41, 0x41, 0x51, 0x32, // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, // H
    0x00, 0x41, 0x7F, 0x41, 0x00, // I
    0x20, 0x40, 0x41, 0x3F, 0x01, // J
    0x7F, 0x08, 0x14, 0x22, 0x41, // K
    0x7F, 0x40, 0x40, 0x40, 0x40, // L
    0x7F, 0x02, 0x04, 0x02, 0x7F, // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, // O
    0x7F, 0x09, 0x09, 0x09, 0x06, // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, // R
    0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x01, 0x01, 0x7F, 0x01, 0x01, // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, // V
    0x7F, 0x20, 0x18, 0x20, 0x7F, // W
    0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x03, 0x04, 0x78, 0x04, 0x03, // Y
    0x61, 0x51, 0x49, 0x45, 0x43, // Z
    0x00, 0x00, 0x7F, 0x41, 0x41, // [
    0x02, 0x04, 0x08, 0x10, 0x20, // backslash
    0x41, 0x41, 0x7F, 0x00, 0x00, // ]
    0x04, 0x02, 0x01, 0x02, 0x04, // ^
    0x40, 0x40, 0x40, 0x40, 0x40, // _
    0x00, 0x01, 0x02, 0x04, 0x00, // `
    0x20, 0x54, 0x54, 0x54, 0x78, // a
    0x7F, 0x48, 0x44, 0x44, 0x38, // b
    0x38, 0x44, 0x44, 0x44, 0x20, // c
    0x38, 0x44, 0x44, 0x48, 0x7F, // d
    0x38, 0x54, 0x54, 0x54, 0x18, // e
    0x08, 0x7E, 0x09, 0x01, 0x02, // f
    0x08, 0x14, 0x54, 0x54, 0x3C, // g
    0x7F, 0x08, 0x04, 0x04, 0x78, // h
    0x00, 0x44, 0x7D, 0x40, 0x00, // i
    0x20, 0x40, 0x44, 0x3D, 0x00, // j
    0x00, 0x7F, 0x10, 0x28, 0x44, // k
    0x00, 0x41, 0x7F, 0x40, 0x00, // l
    0x7C, 0x04, 0x18, 0x04, 0x78, // m
    0x7C, 0x08, 0x04, 0x04, 0x78, // n
    0x38, 0x44, 0x44, 0x44, 0x38, // o
    0x7C, 0x14, 0x14, 0x14, 0x08, // p
    0x08, 0x14, 0x14, 0x18, 0x7C, // q
    0x7C, 0x08, 0x04, 0x04, 0x08, // r
    0x48, 0x54, 0x54, 0x54, 0x20, // s
    0x04, 0x3F, 0x44, 0x40, 0x20, // t
    0x3C, 0x40, 0x40, 0x20, 0x7C, // u
    0x1C, 0x20, 0x40, 0x20, 0x1C, // v
    0x3C, 0x40, 0x30, 0x40, 0x3C, // w
    0x44, 0x28, 0x10, 0x28, 0x44, // x
    0x0C, 0x50, 0x50, 0x50, 0x3C, // y
    0x44, 0x64, 0x54, 0x4C, 0x44, // z
};

static void spi_write_cmd(uint8_t cmd)
{
    gpio_set_level(CONFIG_ESP_PIX_TFT_DC_GPIO, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_polling_transmit(spi_handle, &t);
}

static void spi_write_data(const uint8_t *data, size_t len)
{
    if (len == 0) return;
    gpio_set_level(CONFIG_ESP_PIX_TFT_DC_GPIO, 1);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_polling_transmit(spi_handle, &t);
}

static void spi_write_data_byte(uint8_t data)
{
    spi_write_data(&data, 1);
}

static void set_addr_window(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    uint8_t data[4];
    
    spi_write_cmd(ST7735_CASET);
    data[0] = 0;
    data[1] = x0;
    data[2] = 0;
    data[3] = x1;
    spi_write_data(data, 4);
    
    spi_write_cmd(ST7735_RASET);
    data[0] = 0;
    data[1] = y0;
    data[2] = 0;
    data[3] = y1;
    spi_write_data(data, 4);
    
    spi_write_cmd(ST7735_RAMWR);
}

esp_err_t display_init(void)
{
    // Configure GPIO for DC and RST
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_ESP_PIX_TFT_DC_GPIO) | (1ULL << CONFIG_ESP_PIX_TFT_RST_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = CONFIG_ESP_PIX_TFT_MOSI_GPIO,
        .miso_io_num = -1,
        .sclk_io_num = CONFIG_ESP_PIX_TFT_SCK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = ST7735_WIDTH * ST7735_HEIGHT * 2 + 8,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // Configure SPI device
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = CONFIG_ESP_PIX_TFT_CS_GPIO,
        .queue_size = 7,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle));

    // Hardware reset
    gpio_set_level(CONFIG_ESP_PIX_TFT_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(CONFIG_ESP_PIX_TFT_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(CONFIG_ESP_PIX_TFT_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    // Software reset
    spi_write_cmd(ST7735_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));

    // Exit sleep mode
    spi_write_cmd(ST7735_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Frame rate control
    spi_write_cmd(ST7735_FRMCTR1);
    spi_write_data_byte(0x01);
    spi_write_data_byte(0x2C);
    spi_write_data_byte(0x2D);

    spi_write_cmd(ST7735_FRMCTR2);
    spi_write_data_byte(0x01);
    spi_write_data_byte(0x2C);
    spi_write_data_byte(0x2D);

    spi_write_cmd(ST7735_FRMCTR3);
    spi_write_data_byte(0x01);
    spi_write_data_byte(0x2C);
    spi_write_data_byte(0x2D);
    spi_write_data_byte(0x01);
    spi_write_data_byte(0x2C);
    spi_write_data_byte(0x2D);

    // Display inversion control
    spi_write_cmd(ST7735_INVCTR);
    spi_write_data_byte(0x07);

    // Power control
    spi_write_cmd(ST7735_PWCTR1);
    spi_write_data_byte(0xA2);
    spi_write_data_byte(0x02);
    spi_write_data_byte(0x84);

    spi_write_cmd(ST7735_PWCTR2);
    spi_write_data_byte(0xC5);

    spi_write_cmd(ST7735_PWCTR3);
    spi_write_data_byte(0x0A);
    spi_write_data_byte(0x00);

    spi_write_cmd(ST7735_PWCTR4);
    spi_write_data_byte(0x8A);
    spi_write_data_byte(0x2A);

    spi_write_cmd(ST7735_PWCTR5);
    spi_write_data_byte(0x8A);
    spi_write_data_byte(0xEE);

    // VMCTR1
    spi_write_cmd(ST7735_VMCTR1);
    spi_write_data_byte(0x0E);

    // Inversion off
    spi_write_cmd(ST7735_INVOFF);

    // Memory data access control - rotation 0
    spi_write_cmd(ST7735_MADCTL);
    spi_write_data_byte(0x00);

    // Color mode - 16bit/pixel
    spi_write_cmd(ST7735_COLMOD);
    spi_write_data_byte(0x05);

    // Gamma adjustment positive
    spi_write_cmd(ST7735_GMCTRP1);
    uint8_t gamma_pos[] = {0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D,
                           0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10};
    spi_write_data(gamma_pos, 16);

    // Gamma adjustment negative
    spi_write_cmd(ST7735_GMCTRN1);
    uint8_t gamma_neg[] = {0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D,
                           0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10};
    spi_write_data(gamma_neg, 16);

    // Normal display mode on
    spi_write_cmd(ST7735_NORON);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Display on
    spi_write_cmd(ST7735_DISPON);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Clear screen
    display_fill_screen(ST7735_BLACK);

    ESP_LOGI(TAG, "Display initialized");
    return ESP_OK;
}

void display_fill_screen(uint16_t color)
{
    display_fill_rect(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
}

void display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT) return;
    if (x + w > ST7735_WIDTH) w = ST7735_WIDTH - x;
    if (y + h > ST7735_HEIGHT) h = ST7735_HEIGHT - y;

    set_addr_window(x, y, x + w - 1, y + h - 1);

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    gpio_set_level(CONFIG_ESP_PIX_TFT_DC_GPIO, 1);

    // Send color data in chunks
    uint8_t line_buf[ST7735_WIDTH * 2];
    for (int i = 0; i < w; i++) {
        line_buf[i * 2] = hi;
        line_buf[i * 2 + 1] = lo;
    }

    spi_transaction_t t = {
        .length = w * 16,
        .tx_buffer = line_buf,
    };

    for (int row = 0; row < h; row++) {
        spi_device_polling_transmit(spi_handle, &t);
    }
}

void display_draw_pixel(int16_t x, int16_t y, uint16_t color)
{
    if (x < 0 || x >= ST7735_WIDTH || y < 0 || y >= ST7735_HEIGHT) return;

    set_addr_window(x, y, x, y);
    uint8_t data[2] = {color >> 8, color & 0xFF};
    spi_write_data(data, 2);
}

void display_set_text_color(uint16_t color)
{
    text_color = color;
}

void display_set_cursor(int16_t x, int16_t y)
{
    cursor_x = x;
    cursor_y = y;
}

void display_set_text_size(uint8_t size)
{
    text_size = (size > 0) ? size : 1;
}

static void draw_char(int16_t x, int16_t y, unsigned char c, uint16_t color, uint8_t size)
{
    if (c < 32 || c > 122) return;  // Limited character set
    
    int idx = (c - 32) * 5;
    if (idx >= (int)sizeof(font5x7)) return;
    
    for (int i = 0; i < 5; i++) {
        uint8_t line = font5x7[idx + i];
        for (int j = 0; j < 8; j++) {
            if (line & (1 << j)) {
                if (size == 1) {
                    display_draw_pixel(x + i, y + j, color);
                } else {
                    display_fill_rect(x + i * size, y + j * size, size, size, color);
                }
            }
        }
    }
}

void display_print(const char *text)
{
    while (*text) {
        if (*text == '\n') {
            cursor_y += 8 * text_size;
            cursor_x = 0;
        } else {
            draw_char(cursor_x, cursor_y, *text, text_color, text_size);
            cursor_x += 6 * text_size;
            if (cursor_x > ST7735_WIDTH - 6 * text_size) {
                cursor_x = 0;
                cursor_y += 8 * text_size;
            }
        }
        text++;
    }
}

int16_t display_get_width(void)
{
    return ST7735_WIDTH;
}

int16_t display_get_height(void)
{
    return ST7735_HEIGHT;
}

void display_show_message(const char *title, const char *msg, uint16_t color)
{
    // Cafe Expresso theme: yellow background, orange frame,
    // brown title and white body text
    (void)color; // keep signature but ignore dynamic color

    // Background
    display_fill_screen(ST7735_YELLOW);

    // Outer frame (top/bottom/left/right borders)
    display_fill_rect(0, 0, ST7735_WIDTH, 4, ST7735_ORANGE);
    display_fill_rect(0, ST7735_HEIGHT - 4, ST7735_WIDTH, 4, ST7735_ORANGE);
    display_fill_rect(0, 0, 4, ST7735_HEIGHT, ST7735_ORANGE);
    display_fill_rect(ST7735_WIDTH - 4, 0, 4, ST7735_HEIGHT, ST7735_ORANGE);

    // Title - centered at top in brown
    display_set_text_size(2);
    int16_t title_len = strlen(title) * 12;  // 6 * 2 = 12 pixels per char
    int16_t title_x = (ST7735_WIDTH - title_len) / 2;
    if (title_x < 0) title_x = 0;
    display_set_text_color(ST7735_BROWN);
    display_set_cursor(title_x, 24);
    display_print(title);

    // Message - centered in middle in white
    display_set_text_size(1);
    int16_t msg_len = strlen(msg) * 6;  // 6 pixels per char
    int16_t msg_x = (ST7735_WIDTH - msg_len) / 2;
    if (msg_x < 0) msg_x = 0;
    display_set_text_color(ST7735_WHITE);
    display_set_cursor(msg_x, 80);
    display_print(msg);
}

void display_show_qrcode(const uint8_t *data, uint8_t size, float amount)
{
    // Cafe Expresso theme background
    display_fill_screen(ST7735_YELLOW);

    int scale = 2;
    int16_t offsetX = (ST7735_WIDTH - size * scale) / 2;
    int16_t offsetY = 20;

    // Draw QR code modules
    for (uint8_t y = 0; y < size; y++) {
        for (uint8_t x = 0; x < size; x++) {
            // Calculate bit position
            int bit_pos = y * size + x;
            int byte_pos = bit_pos / 8;
            int bit_offset = bit_pos % 8;
            
            bool is_black = (data[byte_pos] >> bit_offset) & 1;
            uint16_t color = is_black ? ST7735_BLACK : ST7735_WHITE;
            display_fill_rect(offsetX + x * scale, offsetY + y * scale, scale, scale, color);
        }
    }

    // Display amount in brown text
    char amount_str[32];
    snprintf(amount_str, sizeof(amount_str), "%.2f R$", amount / 100.0f);

    display_set_text_color(ST7735_BROWN);
    display_set_text_size(1);
    display_set_cursor(10, ST7735_HEIGHT - 30);
    display_print(amount_str);
}
