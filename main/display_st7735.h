#ifndef DISPLAY_ST7735_H
#define DISPLAY_ST7735_H

#include <stdint.h>
#include "esp_err.h"

// Colors (RGB565)
#define ST7735_BLACK   0x0000
#define ST7735_WHITE   0xFFFF
#define ST7735_RED     0xF800
#define ST7735_GREEN   0x07E0
#define ST7735_BLUE    0x001F
#define ST7735_YELLOW  0xFFE0
#define ST7735_CYAN    0x07FF
#define ST7735_MAGENTA 0xF81F
#define ST7735_ORANGE  0xFD20

/**
 * @brief Initialize the ST7735 display
 * @return ESP_OK on success
 */
esp_err_t display_init(void);

/**
 * @brief Fill the entire screen with a color
 * @param color RGB565 color
 */
void display_fill_screen(uint16_t color);

/**
 * @brief Fill a rectangle with a color
 * @param x X position
 * @param y Y position
 * @param w Width
 * @param h Height
 * @param color RGB565 color
 */
void display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

/**
 * @brief Draw a pixel
 * @param x X position
 * @param y Y position
 * @param color RGB565 color
 */
void display_draw_pixel(int16_t x, int16_t y, uint16_t color);

/**
 * @brief Set text color
 * @param color RGB565 color
 */
void display_set_text_color(uint16_t color);

/**
 * @brief Set cursor position
 * @param x X position
 * @param y Y position
 */
void display_set_cursor(int16_t x, int16_t y);

/**
 * @brief Set text size (scale factor)
 * @param size Size multiplier (1, 2, 3...)
 */
void display_set_text_size(uint8_t size);

/**
 * @brief Print text at cursor position
 * @param text Text to print
 */
void display_print(const char *text);

/**
 * @brief Get display width
 * @return Width in pixels
 */
int16_t display_get_width(void);

/**
 * @brief Get display height
 * @return Height in pixels
 */
int16_t display_get_height(void);

/**
 * @brief Show a message with title
 * @param title Title text
 * @param msg Message text
 * @param color Text color
 */
void display_show_message(const char *title, const char *msg, uint16_t color);

/**
 * @brief Display a QR code
 * @param data QR code data buffer
 * @param size QR code size (modules)
 * @param amount Amount to display
 */
void display_show_qrcode(const uint8_t *data, uint8_t size, float amount);

#endif // DISPLAY_ST7735_H
