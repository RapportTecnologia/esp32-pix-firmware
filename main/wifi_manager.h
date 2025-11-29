#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Initialize WiFi in station mode and connect
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Check if WiFi is connected
 * @return true if connected
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Wait for WiFi connection with timeout
 * @param timeout_ms Timeout in milliseconds
 * @return ESP_OK if connected, ESP_ERR_TIMEOUT if timed out
 */
esp_err_t wifi_manager_wait_connected(uint32_t timeout_ms);

#endif // WIFI_MANAGER_H
