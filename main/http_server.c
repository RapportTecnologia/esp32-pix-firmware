#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"

#include "http_server.h"
#include "wifi_manager.h"

static const char *TAG = "http_server";

// HTTP server handle
static httpd_handle_t s_server = NULL;

// API key storage
static char s_api_key[API_KEY_MAX_LEN] = {0};

// NVS namespace for storing API key
#define NVS_NAMESPACE "esp_pix"
#define NVS_KEY_API   "api_key"

/**
 * @brief Load API key from NVS
 */
static esp_err_t load_api_key_from_nvs(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS namespace not found, API key not set");
        return err;
    }

    size_t required_size = sizeof(s_api_key);
    err = nvs_get_str(nvs_handle, NVS_KEY_API, s_api_key, &required_size);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "API key loaded from NVS (length: %d)", strlen(s_api_key));
    } else {
        ESP_LOGW(TAG, "API key not found in NVS");
    }

    nvs_close(nvs_handle);
    return err;
}

/**
 * @brief Save API key to NVS
 */
static esp_err_t save_api_key_to_nvs(const char *key)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(nvs_handle, NVS_KEY_API, key);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write API key: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "API key saved to NVS");
    }

    nvs_close(nvs_handle);
    return err;
}

/**
 * @brief Handler for GET / (root page)
 */
static esp_err_t root_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET /");
    
    static const char *html_page = 
        "<!DOCTYPE html>"
        "<html lang=\"pt-BR\">"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "<title>ESP32-PIX</title>"
        "<style>"
        "*{margin:0;padding:0;box-sizing:border-box;}"
        "body{font-family:system-ui,-apple-system,sans-serif;background:linear-gradient(135deg,#1a1a2e 0%,#16213e 100%);min-height:100vh;display:flex;align-items:center;justify-content:center;color:#fff;}"
        ".container{max-width:600px;padding:40px;text-align:center;}"
        "h1{font-size:2.5rem;margin-bottom:10px;background:linear-gradient(90deg,#00d4ff,#00ff88);-webkit-background-clip:text;-webkit-text-fill-color:transparent;}"
        ".subtitle{color:#888;margin-bottom:30px;font-size:1.1rem;}"
        ".card{background:rgba(255,255,255,0.05);border-radius:16px;padding:30px;margin:20px 0;border:1px solid rgba(255,255,255,0.1);}"
        ".card h2{color:#00d4ff;margin-bottom:15px;}"
        ".card p{color:#aaa;line-height:1.6;}"
        ".endpoints{text-align:left;margin-top:20px;}"
        ".endpoint{background:rgba(0,212,255,0.1);padding:12px 16px;border-radius:8px;margin:8px 0;font-family:monospace;font-size:0.9rem;}"
        ".endpoint span{color:#00ff88;}"
        "a{color:#00d4ff;text-decoration:none;}"
        "a:hover{text-decoration:underline;}"
        ".footer{margin-top:30px;color:#666;font-size:0.85rem;}"
        "</style>"
        "</head>"
        "<body>"
        "<div class=\"container\">"
        "<h1>ESP32-PIX</h1>"
        "<p class=\"subtitle\">Terminal PIX com ESP32</p>"
        "<div class=\"card\">"
        "<h2>Sobre o Projeto</h2>"
        "<p>ESP32-PIX é um terminal de pagamentos PIX baseado em ESP32. "
        "Permite receber pagamentos instantâneos via QR Code PIX com display integrado.</p>"
        "</div>"
        "<div class=\"card\">"
        "<h2>Endpoints Disponíveis</h2>"
        "<div class=\"endpoints\">"
        "<div class=\"endpoint\"><span>GET</span> /status - Status do dispositivo</div>"
        "<div class=\"endpoint\"><span>GET</span> /addapikey?key=KEY - Configurar API Key</div>"
        "</div>"
        "</div>"
        "<div class=\"card\">"
        "<h2>Obtenha o Projeto</h2>"
        "<p>Código fonte disponível em:<br>"
        "<a href=\"https://github.com/RapportTecnologia/esp32-pix-firmware\" target=\"_blank\">github.com/RapportTecnologia/esp32-pix-firmware</a></p>"
        "</div>"
        "<div class=\"footer\">"
        "<p><a href=\"https://rapport.tec.br\" target=\"_blank\">rapport.tec.br</a></p>"
        "<p>E-mail: <a href=\"mailto:admin@rapport.tec.br\">admin@rapport.tec.br</a></p>"
        "<p>WhatsApp: <a href=\"https://wa.me/5585985205490\" target=\"_blank\">(+55 85) 98520-5490</a></p>"
        "<p style=\"margin-top:15px;\">&copy; 2024 ESP32-PIX Project</p>"
        "</div>"
        "</div>"
        "</body>"
        "</html>";
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    
    return ESP_OK;
}

/**
 * @brief Handler for GET /status endpoint
 */
static esp_err_t status_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET /status");
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "online");
    cJSON_AddStringToObject(root, "device", "ESP32-PIX");
    cJSON_AddBoolToObject(root, "api_key_set", strlen(s_api_key) > 0);
    
    char *json_str = cJSON_PrintUnformatted(root);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free(json_str);
    cJSON_Delete(root);
    
    return ESP_OK;
}

/**
 * @brief Handler for GET /addapikey endpoint
 */
static esp_err_t addapikey_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET /addapikey");
    
    // Get query string length
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    
    if (buf_len <= 1) {
        ESP_LOGW(TAG, "No query parameters provided");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, "{\"success\":false,\"error\":\"Missing key parameter\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    char *buf = malloc(buf_len);
    if (buf == NULL) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    esp_err_t err = httpd_req_get_url_query_str(req, buf, buf_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get query string");
        free(buf);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Query string: %s", buf);
    
    // Extract key parameter
    char key_value[API_KEY_MAX_LEN] = {0};
    err = httpd_query_key_value(buf, "key", key_value, sizeof(key_value));
    
    free(buf);
    
    if (err != ESP_OK || strlen(key_value) == 0) {
        ESP_LOGW(TAG, "Key parameter not found or empty");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, "{\"success\":false,\"error\":\"Invalid or empty key\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // Save API key
    strncpy(s_api_key, key_value, sizeof(s_api_key) - 1);
    s_api_key[sizeof(s_api_key) - 1] = '\0';
    
    // Persist to NVS
    err = save_api_key_to_nvs(s_api_key);
    
    ESP_LOGI(TAG, "API key set successfully (length: %d)", strlen(s_api_key));
    
    // Send response
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "success", true);
    cJSON_AddStringToObject(root, "message", "API key saved successfully");
    cJSON_AddNumberToObject(root, "key_length", strlen(s_api_key));
    
    char *json_str = cJSON_PrintUnformatted(root);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free(json_str);
    cJSON_Delete(root);
    
    return ESP_OK;
}

/**
 * @brief URI handlers registration
 */
static const httpd_uri_t uri_root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t uri_status = {
    .uri       = "/status",
    .method    = HTTP_GET,
    .handler   = status_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t uri_addapikey = {
    .uri       = "/addapikey",
    .method    = HTTP_GET,
    .handler   = addapikey_handler,
    .user_ctx  = NULL
};

esp_err_t http_server_start(void)
{
    if (s_server != NULL) {
        ESP_LOGW(TAG, "Server already running");
        return ESP_OK;
    }

    // Load API key from NVS
    load_api_key_from_nvs();

    // Get and print IP address
    char ip_str[16];
    if (wifi_manager_get_ip(ip_str, sizeof(ip_str)) == ESP_OK) {
        ESP_LOGI(TAG, "============================================");
        ESP_LOGI(TAG, "HTTP Server starting...");
        ESP_LOGI(TAG, "IP Address: %s", ip_str);
        ESP_LOGI(TAG, "Endpoints:");
        ESP_LOGI(TAG, "  - http://%s/", ip_str);
        ESP_LOGI(TAG, "  - http://%s/status", ip_str);
        ESP_LOGI(TAG, "  - http://%s/addapikey?key=YOUR_KEY", ip_str);
        ESP_LOGI(TAG, "============================================");
    } else {
        ESP_LOGW(TAG, "Could not get IP address");
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.server_port = 80;

    ESP_LOGI(TAG, "Starting server on port: %d", config.server_port);
    
    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start server: %s", esp_err_to_name(err));
        return err;
    }

    // Register URI handlers
    httpd_register_uri_handler(s_server, &uri_root);
    httpd_register_uri_handler(s_server, &uri_status);
    httpd_register_uri_handler(s_server, &uri_addapikey);

    ESP_LOGI(TAG, "HTTP server started successfully");
    
    return ESP_OK;
}

esp_err_t http_server_stop(void)
{
    if (s_server == NULL) {
        return ESP_OK;
    }

    esp_err_t err = httpd_stop(s_server);
    if (err == ESP_OK) {
        s_server = NULL;
        ESP_LOGI(TAG, "HTTP server stopped");
    }
    
    return err;
}

const char* http_server_get_api_key(void)
{
    return s_api_key;
}

bool http_server_validate_api_key(const char *key)
{
    if (key == NULL || strlen(s_api_key) == 0) {
        return false;
    }
    return strcmp(key, s_api_key) == 0;
}
