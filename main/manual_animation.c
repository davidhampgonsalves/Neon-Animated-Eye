#include "manual_animation.h"
#include "state.h"
#include "motor.h"
#include "hall.h"
#include "blink.h"


#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "manual";


static bool s_inited = false;
static float s_phase = 0.0f;
static float s_phase_step = 200.0f / 7500.0f;
static int s_step = 0;

void manual_animation_init(void)
{
    usb_serial_jtag_driver_config_t usb_cfg = {
        .rx_buffer_size = 256,
        .tx_buffer_size = 256,
    };
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_cfg));
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

        s_phase += s_phase_step;
        if (s_phase > 1.0f) s_phase = 1.0f;

        s_step++;
        int raw = hall_read();
        ESP_LOGI(TAG, "step=%d motor_time=%lu raw=%d",
                 s_step, (unsigned long)state.motor_time_ms, raw);
    }

    return false;
}
