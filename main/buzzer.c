#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"

#include "buzzer.h"

static const char *TAG = "buzzer";

#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_DUTY_RES       LEDC_TIMER_8_BIT
#define LEDC_DUTY           (127)  // 50% duty cycle

static bool buzzer_initialized = false;

esp_err_t buzzer_init(void)
{
    // Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = 1000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = CONFIG_ESP_PIX_BUZZER_GPIO,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    buzzer_initialized = true;
    ESP_LOGI(TAG, "Buzzer initialized on GPIO %d", CONFIG_ESP_PIX_BUZZER_GPIO);

    return ESP_OK;
}

void buzzer_tone(int frequency, int duration)
{
    if (!buzzer_initialized) {
        return;
    }

    // Set frequency
    ledc_set_freq(LEDC_MODE, LEDC_TIMER, frequency);
    
    // Set duty to 50%
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

    // Wait for duration
    vTaskDelay(pdMS_TO_TICKS(duration));
}

void buzzer_no_tone(void)
{
    if (!buzzer_initialized) {
        return;
    }

    // Set duty to 0 to stop the tone
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

void buzzer_beep(int times, int duration, int frequency)
{
    for (int i = 0; i < times; i++) {
        buzzer_tone(frequency, duration);
        buzzer_no_tone();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
