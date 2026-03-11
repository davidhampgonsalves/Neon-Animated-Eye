#include "motor.h"
#include "state.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

static const char *TAG = "motor";

#define MOTOR_PIN_AIN1   7
#define MOTOR_PIN_AIN2   8
#define MOTOR_PIN_PWMA   9
#define MOTOR_PIN_STBY   6

#define LEDC_TIMER       LEDC_TIMER_0
#define LEDC_CHANNEL     LEDC_CHANNEL_0
#define LEDC_DUTY_RES    LEDC_TIMER_8_BIT
#define LEDC_FREQ_HZ     5000
#define MOTOR_SPEED_HOMING 255
#define MOTOR_SPEED      160

static bool    s_driving = false;
static int64_t s_drive_start_us = 0;

static void motor_set_speed(uint8_t speed)
{
    uint32_t duty = speed;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
}

static void motor_drive_at(uint8_t speed)
{
    if (!s_driving) {
        s_drive_start_us = esp_timer_get_time();
        s_driving = true;
    }
    gpio_set_level((gpio_num_t)MOTOR_PIN_STBY, 1);
    gpio_set_level((gpio_num_t)MOTOR_PIN_AIN1, 1);
    gpio_set_level((gpio_num_t)MOTOR_PIN_AIN2, 0);
    motor_set_speed(speed);
}

void motor_drive(void)
{
    motor_drive_at(MOTOR_SPEED);
}

void motor_drive_homing(void)
{
    motor_drive_at(MOTOR_SPEED_HOMING);
}

void motor_coast(void)
{
    if (s_driving) {
        int64_t now_us = esp_timer_get_time();
        uint32_t delta_ms = (uint32_t)((now_us - s_drive_start_us) / 1000);
        state.motor_time_ms += delta_ms;
        s_driving = false;
    }
    gpio_set_level((gpio_num_t)MOTOR_PIN_AIN1, 0);
    gpio_set_level((gpio_num_t)MOTOR_PIN_AIN2, 0);
    motor_set_speed(0);
    gpio_set_level((gpio_num_t)MOTOR_PIN_STBY, 0);
}

void motor_init(void)
{
    ESP_LOGI(TAG, "Init: AIN1=%d AIN2=%d PWM=%d STBY=%d",
             MOTOR_PIN_AIN1, MOTOR_PIN_AIN2, MOTOR_PIN_PWMA, MOTOR_PIN_STBY);

    gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << MOTOR_PIN_AIN1) |
                        (1ULL << MOTOR_PIN_AIN2) |
                        (1ULL << MOTOR_PIN_STBY),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_cfg));
    gpio_set_level((gpio_num_t)MOTOR_PIN_STBY, 1);

    ledc_timer_config_t timer_cfg = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num       = LEDC_TIMER,
        .freq_hz         = LEDC_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t ch_cfg = {
        .gpio_num   = MOTOR_PIN_PWMA,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_cfg));

    ESP_LOGI(TAG, "TB6612 via LEDC ready");
}
