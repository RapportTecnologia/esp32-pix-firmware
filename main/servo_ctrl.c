#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"

#include "servo_ctrl.h"

static const char *TAG = "servo_ctrl";

// Servo PWM configuration
#define SERVO_LEDC_TIMER        LEDC_TIMER_1
#define SERVO_LEDC_MODE         LEDC_LOW_SPEED_MODE
#define SERVO_LEDC_CHANNEL      LEDC_CHANNEL_1
#define SERVO_LEDC_DUTY_RES     LEDC_TIMER_13_BIT
#define SERVO_FREQ_HZ           50

// Servo pulse width range (in microseconds)
#define SERVO_MIN_PULSEWIDTH_US 500
#define SERVO_MAX_PULSEWIDTH_US 2500
#define SERVO_MAX_DEGREE        180

static bool servo_initialized = false;
static bool servo_attached = false;

static inline uint32_t angle_to_duty(int angle)
{
    // Calculate pulse width for the angle
    uint32_t pulse_width = SERVO_MIN_PULSEWIDTH_US + 
        ((SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) * angle) / SERVO_MAX_DEGREE;
    
    // Convert pulse width to duty cycle
    // Period = 1000000 / 50 = 20000 us
    // Duty = (pulse_width / period) * max_duty
    uint32_t max_duty = (1 << SERVO_LEDC_DUTY_RES) - 1;
    return (pulse_width * max_duty * SERVO_FREQ_HZ) / 1000000;
}

esp_err_t servo_init(void)
{
    // Configure LEDC timer for servo
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = SERVO_LEDC_MODE,
        .timer_num        = SERVO_LEDC_TIMER,
        .duty_resolution  = SERVO_LEDC_DUTY_RES,
        .freq_hz          = SERVO_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = SERVO_LEDC_MODE,
        .channel        = SERVO_LEDC_CHANNEL,
        .timer_sel      = SERVO_LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = CONFIG_ESP_PIX_SERVO_GPIO,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    servo_initialized = true;
    servo_attached = false;

    ESP_LOGI(TAG, "Servo initialized on GPIO %d", CONFIG_ESP_PIX_SERVO_GPIO);

    // Set initial position to 90 degrees
    servo_attach();
    servo_set_angle(90);
    vTaskDelay(pdMS_TO_TICKS(50));
    servo_detach();

    return ESP_OK;
}

esp_err_t servo_attach(void)
{
    if (!servo_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    servo_attached = true;
    return ESP_OK;
}

esp_err_t servo_detach(void)
{
    if (!servo_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Set duty to 0 to stop the signal
    ledc_set_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL, 0);
    ledc_update_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL);
    
    servo_attached = false;
    return ESP_OK;
}

esp_err_t servo_set_angle(int angle)
{
    if (!servo_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (angle < 0) angle = 0;
    if (angle > SERVO_MAX_DEGREE) angle = SERVO_MAX_DEGREE;

    uint32_t duty = angle_to_duty(angle);
    
    ledc_set_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL, duty);
    ledc_update_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL);

    return ESP_OK;
}

void servo_dispense(void)
{
    ESP_LOGI(TAG, "Dispensing product...");
    
    servo_attach();

    // Move from 0 to 90 degrees
    for (int pos = 0; pos <= 90; pos++) {
        servo_set_angle(pos);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelay(pdMS_TO_TICKS(200));

    // Return to 90 degrees
    servo_set_angle(90);
    vTaskDelay(pdMS_TO_TICKS(200));

    servo_detach();

    ESP_LOGI(TAG, "Dispense complete, servo stopped.");
}
