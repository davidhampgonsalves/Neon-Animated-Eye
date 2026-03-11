#include "leds.h"
#include "config.h"

#include "esp_log.h"
#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern const config_t *g_cfg;

static const char *TAG = "leds";

#define PIN_EYELID    20
#define PIN_EYEBALL   21
#define NUM_EYELID    60
#define NUM_EYEBALL   22

static led_strip_handle_t s_eyelid;
static led_strip_handle_t s_eyeball;

static void off(led_strip_handle_t strip, int idx)
{
    led_strip_set_pixel(strip, idx, 0, 0, 0);
}

void leds_off(void)
{
    for (int i = 0; i < NUM_EYELID; i++)
        off(s_eyelid, i);
    led_strip_refresh(s_eyelid);
    for (int i = 0; i < NUM_EYEBALL; i++)
        off(s_eyeball, i);
    led_strip_refresh(s_eyeball);
}

void occlude_eye(int count)
{
    ESP_LOGI(TAG, "occlude_eye count=%d", count);
    const rgb_t c = g_cfg->eyeball_colour;
    for (int i = 0; i < NUM_EYEBALL; i++)
        led_strip_set_pixel(s_eyeball, i, c.r, c.g, c.b);

    for (int i = 0; i < count && i < 6; i++) {
        off(s_eyeball, 8 - i);
        off(s_eyeball, 9 + i);
        off(s_eyeball, (20 + i) % 22);
        off(s_eyeball, 19 - i);
    }
}

void occlude_lid(int count)
{
    ESP_LOGI(TAG, "occlude_lid count=%d", count);
    for (int i = 0; i < 30; i++)
        led_strip_set_pixel(s_eyelid, i, 255, 0, 0);
    for (int i = 30; i < NUM_EYELID; i++)
        led_strip_set_pixel(s_eyelid, i, 0, 0, 255);

    for (int i = 0; i < count && i < 30; i++) {
        off(s_eyelid, i);
        off(s_eyelid, 29 - i);
        off(s_eyelid, 30 + i);
        off(s_eyelid, 59 - i);
    }
}

void leds_refresh(void)
{
    led_strip_refresh(s_eyeball);
    vTaskDelay(1);
    led_strip_refresh(s_eyelid);
}

void leds_init(void)
{
    led_strip_config_t el_cfg = {
        .strip_gpio_num           = PIN_EYELID,
        .max_leds                 = NUM_EYELID,
        .led_model                = LED_MODEL_WS2812,
        .flags                    = { .invert_out = false },
    };
    led_strip_rmt_config_t el_rmt = {
        .clk_src       = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags         = { .with_dma = false },
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&el_cfg, &el_rmt, &s_eyelid));

    led_strip_config_t eb_cfg = {
        .strip_gpio_num           = PIN_EYEBALL,
        .max_leds                 = NUM_EYEBALL,
        .led_model                = LED_MODEL_WS2812,
        .flags                    = { .invert_out = false },
    };
    led_strip_rmt_config_t eb_rmt = {
        .clk_src       = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags         = { .with_dma = false },
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&eb_cfg, &eb_rmt, &s_eyeball));

    leds_off();
    ESP_LOGI(TAG, "LED strips ready");
}
