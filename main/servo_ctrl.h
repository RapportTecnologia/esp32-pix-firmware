#ifndef SERVO_CTRL_H
#define SERVO_CTRL_H

#include "esp_err.h"

/**
 * @brief Initialize servo motor
 * @return ESP_OK on success
 */
esp_err_t servo_init(void);

/**
 * @brief Set servo angle
 * @param angle Angle in degrees (0-180)
 * @return ESP_OK on success
 */
esp_err_t servo_set_angle(int angle);

/**
 * @brief Attach servo (enable PWM output)
 * @return ESP_OK on success
 */
esp_err_t servo_attach(void);

/**
 * @brief Detach servo (disable PWM output)
 * @return ESP_OK on success
 */
esp_err_t servo_detach(void);

/**
 * @brief Dispense product animation
 */
void servo_dispense(void);

#endif // SERVO_CTRL_H
