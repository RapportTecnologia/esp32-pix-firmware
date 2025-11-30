/**
 * ESP-PIX - ESP-IDF 5.5.0 PIX Payment System
 * 
 * Main application file
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "wifi_manager.h"
#include "http_client.h"
#include "http_server.h"
#include "display_st7735.h"
#include "qrcode_gen.h"
#include "servo_ctrl.h"
#include "buzzer.h"

static const char *TAG = "esp-pix";

// Global state
static char g_payment_id[64] = {0};
static char g_qr_data[512] = {0};
static float g_amount = 0;
static bool g_system_active = false;
static int64_t g_last_poll_time = 0;
static int64_t g_qr_start_time = 0;

// Forward declarations
static void cancel_charge(void);

// ==========================================================
// Button handling
typedef struct {
    int64_t press_start;
    bool holding;
} button_state_t;

static button_state_t g_button = {0, false};

static bool handle_button(void)
{
    bool pressed = gpio_get_level(CONFIG_ESP_PIX_BUTTON_GPIO) == 0;
    int64_t now = esp_timer_get_time() / 1000;  // Convert to ms

    if (pressed && g_button.press_start == 0) {
        g_button.press_start = now;
    }

    // Long press to cancel (3 seconds)
    if (pressed && (now - g_button.press_start > 3000) && g_system_active && !g_button.holding) {
        g_button.holding = true;
        ESP_LOGI(TAG, "Cancelando cobranca...");
        display_show_message("Cancelando", "Aguarde...", ST7735_RED);
        cancel_charge();
    }

    if (!pressed && g_button.press_start > 0) {
        int64_t press_time = now - g_button.press_start;
        g_button.press_start = 0;
        g_button.holding = false;

        // Short press to start
        if (press_time < 800 && !g_system_active) {
            return true;
        }
    }

    return false;
}

// ==========================================================
// Create charge
static void create_charge(float amount, const char *description)
{
    if (!wifi_manager_is_connected()) {
        ESP_LOGW(TAG, "WiFi desconectado!");
        buzzer_beep(3, 150, 500);
        display_show_message("Erro", "Sem WiFi!", ST7735_RED);
        return;
    }

    payment_response_t response;
    esp_err_t err = http_create_charge(amount, description, &response);

    if (err == ESP_OK && response.success) {
        strlcpy(g_payment_id, response.payment_id, sizeof(g_payment_id));
        strlcpy(g_qr_data, response.qr_code, sizeof(g_qr_data));
        g_amount = response.amount;

        // Generate and display QR code
        qrcode_t qrcode;
        if (qrcode_generate(&qrcode, g_qr_data)) {
            display_show_qrcode(qrcode.data, qrcode.size, g_amount);
            buzzer_beep(2, 150, 1500);
            g_system_active = true;
            g_qr_start_time = esp_timer_get_time() / 1000;
        } else {
            ESP_LOGE(TAG, "Falha ao gerar QR Code");
            display_show_message("Erro", "QR Code falhou", ST7735_RED);
            buzzer_beep(3, 200, 500);
        }
    } else {
        ESP_LOGE(TAG, "Erro ao criar cobranca");
        display_show_message("Erro", "Criar cobranca", ST7735_RED);
        buzzer_beep(3, 200, 500);
    }
}

// ==========================================================
// Cancel charge
static void cancel_charge(void)
{
    servo_detach();
    
    if (strlen(g_payment_id) > 0) {
        ESP_LOGI(TAG, "Cobranca cancelada!");
        memset(g_payment_id, 0, sizeof(g_payment_id));
    }
    
    g_system_active = false;
    gpio_set_level(CONFIG_ESP_PIX_LED_GPIO, 0);
    buzzer_beep(2, 150, 600);
    display_show_message("Cancelado", "Pressione o botao", ST7735_WHITE);
}

// ==========================================================
// Check payment status
static bool check_payment_status(void)
{
    if (strlen(g_payment_id) == 0 || !wifi_manager_is_connected()) {
        return false;
    }

    payment_status_t status = http_check_payment_status(g_payment_id);
    
    if (status == PAYMENT_STATUS_APPROVED) {
        buzzer_beep(2, 150, 2000);
        return true;
    }
    
    return false;
}

// ==========================================================
// Dispense product
static void dispense(void)
{
    ESP_LOGI(TAG, "Pagamento confirmado!");
    display_show_message("Pagamento", "Confirmado!", ST7735_GREEN);
    buzzer_beep(3, 150, 1800);
    
    servo_dispense();

    display_show_message("Liberado", "Retire o produto", ST7735_WHITE);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    display_show_message("Obrigado!", "Volte sempre!", ST7735_GREEN);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Blink LED
    for (int i = 0; i < 5; i++) {
        gpio_set_level(CONFIG_ESP_PIX_LED_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(150));
        gpio_set_level(CONFIG_ESP_PIX_LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(150));
    }

    memset(g_payment_id, 0, sizeof(g_payment_id));
    g_system_active = false;
    display_show_message("Pronto", "Pressione o botao", ST7735_WHITE);
}

// ==========================================================
// Show countdown
static void show_countdown(void)
{
    if (!g_system_active) return;

    int64_t now = esp_timer_get_time() / 1000;
    int64_t elapsed = now - g_qr_start_time;
    
    if (elapsed >= CONFIG_ESP_PIX_PAYMENT_TIMEOUT_MS) {
        ESP_LOGI(TAG, "Tempo expirado!");
        display_show_message("Expirado", "Cobranca cancelada", ST7735_RED);
        cancel_charge();
        return;
    }

    int remaining = (CONFIG_ESP_PIX_PAYMENT_TIMEOUT_MS - elapsed) / 1000;
    
    // Update countdown on display
    char countdown_str[32];
    snprintf(countdown_str, sizeof(countdown_str), "Tempo: %ds", remaining);
    
    // Clear countdown area and display new value
    display_fill_rect(0, 155, 160, 10, ST7735_WHITE);
    display_set_text_color(ST7735_BLACK);
    display_set_cursor(10, 140);
    display_set_text_size(1);
    display_print(countdown_str);
}

// ==========================================================
// Main application
void app_main(void)
{
    ESP_LOGI(TAG, "ESP-PIX iniciando...");

    // Configure LED GPIO
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << CONFIG_ESP_PIX_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);

    // Configure Button GPIO
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << CONFIG_ESP_PIX_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_conf);

    // Initialize peripherals
    buzzer_init();
    servo_init();
    display_init();

    // Show welcome message with Cafe Expresso branding
    display_show_message("Caf\xC3\xA9 Expresso", "Sistema Cognitivo de Cobranca Embarcada", ST7735_WHITE);

    // Initialize WiFi
    ESP_LOGI(TAG, "Conectando ao WiFi...");
    wifi_manager_init();
    
    // Wait for WiFi connection
    while (!wifi_manager_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(CONFIG_ESP_PIX_LED_GPIO, !gpio_get_level(CONFIG_ESP_PIX_LED_GPIO));
    }
    
    gpio_set_level(CONFIG_ESP_PIX_LED_GPIO, 1);
    ESP_LOGI(TAG, "WiFi conectado!");
    display_show_message("WiFi", "Conectado!", ST7735_GREEN);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Start HTTP REST server
    ESP_LOGI(TAG, "Iniciando servidor HTTP...");
    if (http_server_start() == ESP_OK) {
        char ip_str[16];
        if (wifi_manager_get_ip(ip_str, sizeof(ip_str)) == ESP_OK) {
            ESP_LOGI(TAG, "Servidor HTTP disponivel em: http://%s", ip_str);
            display_show_message("HTTP Server", ip_str, ST7735_CYAN);
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    } else {
        ESP_LOGE(TAG, "Falha ao iniciar servidor HTTP");
    }
    
    display_show_message("Pronto", "Pressione o botao", ST7735_WHITE);
    buzzer_beep(1, 200, 1500);

    // Main loop
    while (1) {
        // Check button
        if (handle_button()) {
            gpio_set_level(CONFIG_ESP_PIX_LED_GPIO, 1);
            buzzer_beep(1, 150, 1500);
            display_show_message("Gerando PIX", "Aguarde...", ST7735_YELLOW);
            create_charge(0.50, "Produto teste");
        }

        // Active payment session
        if (g_system_active && strlen(g_payment_id) > 0) {
            show_countdown();
            
            // Check payment status every 5 seconds
            int64_t now = esp_timer_get_time() / 1000;
            if (now - g_last_poll_time > 5000) {
                g_last_poll_time = now;
                if (check_payment_status()) {
                    dispense();
                } else {
                    ESP_LOGI(TAG, "Aguardando pagamento...");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms loop delay
    }
}
