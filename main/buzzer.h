#ifndef BUZZER_H
#define BUZZER_H

#include "esp_err.h"

/**
 * @brief Initialize buzzer
 * @return ESP_OK on success
 */
esp_err_t buzzer_init(void);

/**
 * @brief Play a tone
 * @param frequency Frequency in Hz
 * @param duration Duration in milliseconds
 */
void buzzer_tone(int frequency, int duration);

/**
 * @brief Stop the tone
 */
void buzzer_no_tone(void);

/**
 * @brief Beep n times
 * @param times Number of beeps
 * @param duration Duration of each beep in ms
 * @param frequency Frequency in Hz
 */
void buzzer_beep(int times, int duration, int frequency);

#endif // BUZZER_H
