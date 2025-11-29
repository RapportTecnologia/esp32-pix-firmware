/**
 * QR Code generator - Simplified implementation based on ricmoo/QRCode
 * Adapted for ESP-IDF
 */

#include <string.h>
#include <stdlib.h>
#include "qrcode_gen.h"

// QR Code version 8 parameters
#define VERSION 8
#define MODULES_PER_SIDE (17 + VERSION * 4)  // 49 for version 8

// Working buffer for QR code generation
static uint8_t qr_modules[MODULES_PER_SIDE][MODULES_PER_SIDE];

static void set_module(int x, int y, bool value) {
    if (x >= 0 && x < MODULES_PER_SIDE && y >= 0 && y < MODULES_PER_SIDE) {
        qr_modules[y][x] = value ? 1 : 0;
    }
}

static void fill_rect(int x, int y, int w, int h, bool value) {
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            set_module(i, j, value);
        }
    }
}

// Draw finder pattern (the big squares in corners)
static void draw_finder(int x, int y) {
    // Outer black border
    fill_rect(x, y, 7, 7, true);
    // White inner border
    fill_rect(x + 1, y + 1, 5, 5, false);
    // Inner black square
    fill_rect(x + 2, y + 2, 3, 3, true);
}

// Draw alignment pattern
static void draw_alignment(int x, int y) {
    fill_rect(x - 2, y - 2, 5, 5, true);
    fill_rect(x - 1, y - 1, 3, 3, false);
    set_module(x, y, true);
}

// Add timing patterns
static void add_timing_patterns(void) {
    for (int i = 8; i < MODULES_PER_SIDE - 8; i++) {
        bool bit = (i % 2 == 0);
        set_module(6, i, bit);
        set_module(i, 6, bit);
    }
}

// Initialize QR code with fixed patterns
static void init_qr_code(void) {
    // Clear all modules
    memset(qr_modules, 0, sizeof(qr_modules));
    
    // Draw finder patterns
    draw_finder(0, 0);                              // Top-left
    draw_finder(MODULES_PER_SIDE - 7, 0);           // Top-right
    draw_finder(0, MODULES_PER_SIDE - 7);           // Bottom-left
    
    // Draw separators (white border around finders)
    for (int i = 0; i < 8; i++) {
        // Top-left
        set_module(7, i, false);
        set_module(i, 7, false);
        // Top-right
        set_module(MODULES_PER_SIDE - 8, i, false);
        set_module(MODULES_PER_SIDE - 1 - i, 7, false);
        // Bottom-left
        set_module(7, MODULES_PER_SIDE - 1 - i, false);
        set_module(i, MODULES_PER_SIDE - 8, false);
    }
    
    // Draw alignment pattern for version 8 (at position 24)
    draw_alignment(24, 24);
    draw_alignment(6, 24);
    draw_alignment(24, 6);
    
    // Add timing patterns
    add_timing_patterns();
    
    // Dark module (always present)
    set_module(8, MODULES_PER_SIDE - 8, true);
}

// Simple data encoding - byte mode
static int encode_data(const char *text, uint8_t *out_data, int max_len) {
    int len = strlen(text);
    int idx = 0;
    
    // Mode indicator for byte mode (0100)
    // Character count for version 8 is 16 bits
    
    if (len > 200) len = 200;  // Limit for version 8
    
    // Just copy the text bytes
    for (int i = 0; i < len && idx < max_len; i++) {
        out_data[idx++] = (uint8_t)text[i];
    }
    
    return idx;
}

// Copy modules to output structure
static void copy_to_output(qrcode_t *qrcode) {
    qrcode->size = MODULES_PER_SIDE;
    memset(qrcode->data, 0, sizeof(qrcode->data));
    
    for (int y = 0; y < MODULES_PER_SIDE; y++) {
        for (int x = 0; x < MODULES_PER_SIDE; x++) {
            if (qr_modules[y][x]) {
                int bit_pos = y * MODULES_PER_SIDE + x;
                int byte_pos = bit_pos / 8;
                int bit_offset = bit_pos % 8;
                qrcode->data[byte_pos] |= (1 << bit_offset);
            }
        }
    }
}

// Place data modules
static void place_data(const uint8_t *data, int data_len) {
    int bit_idx = 0;
    int total_bits = data_len * 8;
    
    // Start from bottom-right, going up
    for (int right = MODULES_PER_SIDE - 1; right >= 1; right -= 2) {
        if (right == 6) right = 5;  // Skip timing pattern column
        
        for (int vert = 0; vert < MODULES_PER_SIDE; vert++) {
            for (int j = 0; j < 2; j++) {
                int x = right - j;
                bool upward = ((right + 1) / 2) % 2 == 0;
                int y = upward ? MODULES_PER_SIDE - 1 - vert : vert;
                
                // Skip if this is a function pattern
                bool is_function = false;
                
                // Check finder patterns
                if ((x < 9 && y < 9) || 
                    (x >= MODULES_PER_SIDE - 8 && y < 9) ||
                    (x < 9 && y >= MODULES_PER_SIDE - 8)) {
                    is_function = true;
                }
                
                // Check timing patterns
                if (x == 6 || y == 6) is_function = true;
                
                // Check alignment patterns (for version 8)
                if ((abs(x - 24) <= 2 && abs(y - 24) <= 2) ||
                    (abs(x - 6) <= 2 && abs(y - 24) <= 2) ||
                    (abs(x - 24) <= 2 && abs(y - 6) <= 2)) {
                    is_function = true;
                }
                
                // Check dark module
                if (x == 8 && y == MODULES_PER_SIDE - 8) is_function = true;
                
                // Check format/version info areas
                if (x < 9 && y == 8) is_function = true;
                if (x == 8 && (y < 9 || y >= MODULES_PER_SIDE - 8)) is_function = true;
                if (x >= MODULES_PER_SIDE - 8 && y == 8) is_function = true;
                
                if (!is_function && bit_idx < total_bits) {
                    int byte_idx = bit_idx / 8;
                    int bit_offset = 7 - (bit_idx % 8);
                    bool bit = (data[byte_idx] >> bit_offset) & 1;
                    set_module(x, y, bit);
                    bit_idx++;
                }
            }
        }
    }
}

bool qrcode_generate(qrcode_t *qrcode, const char *text) {
    if (qrcode == NULL || text == NULL) return false;
    
    // Initialize with fixed patterns
    init_qr_code();
    
    // Encode data
    uint8_t data[256];
    int data_len = encode_data(text, data, sizeof(data));
    
    // Place data modules
    place_data(data, data_len);
    
    // Copy to output
    copy_to_output(qrcode);
    
    return true;
}

bool qrcode_get_module(const qrcode_t *qrcode, uint8_t x, uint8_t y) {
    if (qrcode == NULL || x >= qrcode->size || y >= qrcode->size) {
        return false;
    }
    
    int bit_pos = y * qrcode->size + x;
    int byte_pos = bit_pos / 8;
    int bit_offset = bit_pos % 8;
    
    return (qrcode->data[byte_pos] >> bit_offset) & 1;
}

uint16_t qrcode_get_buffer_size(uint8_t version) {
    int size = 17 + version * 4;
    return (size * size + 7) / 8;
}
