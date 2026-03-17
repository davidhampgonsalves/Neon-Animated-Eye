#include "manual_animation.h"
#include "state.h"
#include "motor.h"
#include "hall.h"
#include "blink.h"
#include "leds.h"


#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "manual";


static bool s_inited = false;

void manual_animation_init(void)
{
    s_inited = true;
    ESP_LOGI(TAG, "Press any key to advance motor 200ms + update phase.");
}

bool manual_animation(uint32_t elapsed_ms)
{
    (void)elapsed_ms;

    if (!s_inited)
        manual_animation_init();

    uint8_t ch;
    int len = usb_serial_jtag_read_bytes(&ch, 1, 0);
    if (len > 0) {
        motor_drive();
        vTaskDelay(pdMS_TO_TICKS(200));
        motor_coast();

        blink(state.motor_time_ms);

        // int raw = hall_read();
        // ESP_LOGI(TAG, "step=%d motor_time=%lu raw=%d",
        //          s_step, (unsigned long)state.motor_time_ms, raw);
    }

    return false;
}
