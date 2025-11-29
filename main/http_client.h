#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Payment response structure
 */
typedef struct {
    char payment_id[64];
    char qr_code[512];
    float amount;
    bool success;
} payment_response_t;

/**
 * @brief Payment status
 */
typedef enum {
    PAYMENT_STATUS_UNKNOWN,
    PAYMENT_STATUS_PENDING,
    PAYMENT_STATUS_APPROVED,
    PAYMENT_STATUS_REJECTED,
    PAYMENT_STATUS_ERROR
} payment_status_t;

/**
 * @brief Create a new PIX charge
 * @param amount Amount in BRL (e.g., 0.50)
 * @param description Description of the charge
 * @param response Pointer to store the response
 * @return ESP_OK on success
 */
esp_err_t http_create_charge(float amount, const char *description, payment_response_t *response);

/**
 * @brief Check payment status
 * @param payment_id Payment ID to check
 * @return Payment status
 */
payment_status_t http_check_payment_status(const char *payment_id);

#endif // HTTP_CLIENT_H
