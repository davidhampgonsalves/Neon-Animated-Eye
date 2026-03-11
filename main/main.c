#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "config.h"
#include "state.h"
#include "motor.h"
#include "hall.h"
#include "leds.h"
#include "homing.h"
#include "idle.h"
#include "blink.h"
#include "manual_animation.h"

static const char *TAG = "main";

state_t state = {0};
const config_t *g_cfg = NULL;

void app_main(void)
{
    ESP_LOGI(TAG, "Kinetic Eye starting");

    static const config_t cfg = CONFIG_DEFAULT;
    g_cfg = &cfg;

    motor_init();
    leds_init();
    hall_init();

    state.zero_position_us = homing_run();
    state.state = STATE_MANUAL_ANIMATION;
    state.state_start_us = esp_timer_get_time();

    ESP_LOGI(TAG, "Entering main loop");

    for (;;) {
        hall_monitor();

        uint32_t elapsed_ms = (uint32_t)((esp_timer_get_time() - state.state_start_us) / 1000);
        bool complete = false;

        switch (state.state) {
            case STATE_IDLE_OPEN:
                complete = idle(elapsed_ms);
                break;
            case STATE_BLINKING:
                complete = blink(elapsed_ms);
                break;
            case STATE_MANUAL_ANIMATION:
                complete = manual_animation(elapsed_ms);
                break;
        }

        if (complete) {
            state.state = next_state(state.state);
            state.state_start_us = esp_timer_get_time();
        }

        vTaskDelay(1);
    }
}
