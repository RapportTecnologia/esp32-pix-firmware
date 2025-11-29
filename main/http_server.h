#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Maximum length for API key
 */
#define API_KEY_MAX_LEN 128

/**
 * @brief Initialize and start the HTTP REST server
 * @return ESP_OK on success
 */
esp_err_t http_server_start(void);

/**
 * @brief Stop the HTTP server
 * @return ESP_OK on success
 */
esp_err_t http_server_stop(void);

/**
 * @brief Get the current API key
 * @return Pointer to the stored API key (empty string if not set)
 */
const char* http_server_get_api_key(void);

/**
 * @brief Check if API key is valid
 * @param key API key to validate
 * @return true if valid
 */
bool http_server_validate_api_key(const char *key);

#endif // HTTP_SERVER_H
