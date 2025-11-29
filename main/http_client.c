#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"

#include "http_client.h"

static const char *TAG = "http_client";

// Certificado CA embarcado (ISRG Root X1 - Let's Encrypt)
extern const char isrg_root_x1_pem_start[] asm("_binary_isrg_root_x1_pem_start");
extern const char isrg_root_x1_pem_end[]   asm("_binary_isrg_root_x1_pem_end");

#define MAX_HTTP_OUTPUT_BUFFER 1024

// Buffer for HTTP response
static char response_buffer[MAX_HTTP_OUTPUT_BUFFER];
static int response_len = 0;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (response_len + evt->data_len < MAX_HTTP_OUTPUT_BUFFER) {
                memcpy(response_buffer + response_len, evt->data, evt->data_len);
                response_len += evt->data_len;
                response_buffer[response_len] = 0;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

esp_err_t http_create_charge(float amount, const char *description, payment_response_t *response)
{
    if (response == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(response, 0, sizeof(payment_response_t));
    response_len = 0;
    memset(response_buffer, 0, sizeof(response_buffer));

    // Build URL
    char url[256];
    snprintf(url, sizeof(url), "%s/create_payment", CONFIG_ESP_PIX_BACKEND_URL);

    // Build JSON body
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "amount", amount);
    cJSON_AddStringToObject(root, "description", description);
    char *post_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (post_data == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sending: %s", post_data);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
        .cert_pem = isrg_root_x1_pem_start,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %" PRId64,
                 status_code, esp_http_client_get_content_length(client));

        if (status_code == 200) {
            // Parse JSON response
            cJSON *json = cJSON_Parse(response_buffer);
            if (json != NULL) {
                cJSON *payment_id = cJSON_GetObjectItem(json, "paymentId");
                cJSON *qr_code = cJSON_GetObjectItem(json, "qrCode");
                cJSON *amount_json = cJSON_GetObjectItem(json, "amount");

                if (cJSON_IsString(payment_id)) {
                    strncpy(response->payment_id, payment_id->valuestring, sizeof(response->payment_id) - 1);
                }
                if (cJSON_IsString(qr_code)) {
                    strncpy(response->qr_code, qr_code->valuestring, sizeof(response->qr_code) - 1);
                }
                if (cJSON_IsNumber(amount_json)) {
                    response->amount = (float)amount_json->valuedouble;
                }
                response->success = true;

                cJSON_Delete(json);
            } else {
                ESP_LOGE(TAG, "Failed to parse JSON response");
                err = ESP_FAIL;
            }
        } else {
            ESP_LOGE(TAG, "HTTP error: %d", status_code);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    free(post_data);
    esp_http_client_cleanup(client);

    return err;
}

payment_status_t http_check_payment_status(const char *payment_id)
{
    if (payment_id == NULL || strlen(payment_id) == 0) {
        return PAYMENT_STATUS_ERROR;
    }

    response_len = 0;
    memset(response_buffer, 0, sizeof(response_buffer));

    // Build URL
    char url[256];
    snprintf(url, sizeof(url), "%s/status/%s", CONFIG_ESP_PIX_BACKEND_URL, payment_id);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
        .cert_pem = isrg_root_x1_pem_start,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    payment_status_t status = PAYMENT_STATUS_UNKNOWN;

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGD(TAG, "HTTP GET Status = %d", status_code);

        if (status_code == 200) {
            cJSON *json = cJSON_Parse(response_buffer);
            if (json != NULL) {
                cJSON *status_json = cJSON_GetObjectItem(json, "status");
                if (cJSON_IsString(status_json)) {
                    const char *status_str = status_json->valuestring;
                    if (strcmp(status_str, "APPROVED") == 0) {
                        status = PAYMENT_STATUS_APPROVED;
                    } else if (strcmp(status_str, "PENDING") == 0) {
                        status = PAYMENT_STATUS_PENDING;
                    } else if (strcmp(status_str, "REJECTED") == 0) {
                        status = PAYMENT_STATUS_REJECTED;
                    }
                }
                cJSON_Delete(json);
            }
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        status = PAYMENT_STATUS_ERROR;
    }

    esp_http_client_cleanup(client);
    return status;
}
