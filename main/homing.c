#include "homing.h"
#include "state.h"
#include "hall.h"
#include "motor.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "homing";

#define WINDOW_SAMPLES  5
#define WINDOW_TICK     5
#define RISE_THRESHOLD  50


int64_t homing_run(void)
{
    motor_drive_homing();

    ESP_LOGI(TAG, "Homing — waiting for baseline...");
    for (;;) {
        vTaskDelay(WINDOW_TICK);
        int raw = hall_read();
        if (raw < 2440)
            break;
    }

    ESP_LOGI(TAG, "Homing — looking for rise...");

    int adc_raw = hall_read();
    int window_first = adc_raw;
    int peak_val = adc_raw;
    int64_t peak_time_us = esp_timer_get_time();

    for (;;) {
        int above = 0;
        int samples[WINDOW_SAMPLES];
        for (int i = 0; i < WINDOW_SAMPLES; i++) {
            vTaskDelay(WINDOW_TICK);
            adc_raw = hall_read();
            samples[i] = adc_raw;
            if (adc_raw > peak_val) {
                peak_val = adc_raw;
                peak_time_us = esp_timer_get_time();
            }
            if (adc_raw > window_first + RISE_THRESHOLD) above++;
        }

        ESP_LOGI(TAG, "rise: [%d %d %d %d %d] first=%d peak=%d above=%d",
                 samples[0], samples[1], samples[2], samples[3], samples[4],
                 window_first, peak_val, above);

        if (above > WINDOW_SAMPLES / 2) {
            ESP_LOGI(TAG, "Rise detected. peak=%d", peak_val);
            break;
        }
        window_first = adc_raw;
    }

    ESP_LOGI(TAG, "Looking for fall...");

    for (;;) {
        int below = 0;
        int samples[WINDOW_SAMPLES];
        for (int i = 0; i < WINDOW_SAMPLES; i++) {
            vTaskDelay(WINDOW_TICK);
            adc_raw = hall_read();
            samples[i] = adc_raw;
            if (adc_raw > peak_val) {
                peak_val = adc_raw;
                peak_time_us = esp_timer_get_time();
            }
            if (adc_raw < peak_val) below++;
        }

        ESP_LOGI(TAG, "fall: [%d %d %d %d %d] peak=%d below=%d",
                 samples[0], samples[1], samples[2], samples[3], samples[4],
                 peak_val, below);

        if (below > WINDOW_SAMPLES / 2) {
            ESP_LOGI(TAG, "Fall detected. peak=%d raw=%d", peak_val, adc_raw);
            break;
        }
    }

    motor_coast();

    int64_t now_us = esp_timer_get_time();
    uint32_t overshoot_ms = (uint32_t)((now_us - peak_time_us) / 1000);

    ESP_LOGI(TAG, "Homed. peak=%d overshoot=%lums",
             peak_val, (unsigned long)overshoot_ms);

    return peak_time_us;
}
