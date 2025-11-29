#ifndef QRCODE_GEN_H
#define QRCODE_GEN_H

#include <stdint.h>
#include <stdbool.h>

// QR Code version 8 has size 49x49
#define QRCODE_VERSION 8
#define QRCODE_MAX_SIZE 49

/**
 * @brief QR Code structure
 */
typedef struct {
    uint8_t size;
    uint8_t data[QRCODE_MAX_SIZE * QRCODE_MAX_SIZE / 8 + 1];
} qrcode_t;

/**
 * @brief Generate QR code from text
 * @param qrcode Pointer to QR code structure
 * @param text Text to encode
 * @return true on success
 */
bool qrcode_generate(qrcode_t *qrcode, const char *text);

/**
 * @brief Check if a module is black
 * @param qrcode Pointer to QR code structure
 * @param x X coordinate
 * @param y Y coordinate
 * @return true if module is black
 */
bool qrcode_get_module(const qrcode_t *qrcode, uint8_t x, uint8_t y);

/**
 * @brief Get buffer size needed for QR code
 * @param version QR code version
 * @return Buffer size in bytes
 */
uint16_t qrcode_get_buffer_size(uint8_t version);

#endif // QRCODE_GEN_H
