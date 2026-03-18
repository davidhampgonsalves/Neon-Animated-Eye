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
#define NUM_EYELID    61
#define NUM_EYEBALL   22

static led_strip_handle_t s_eyelid;
static led_strip_handle_t s_eyeball;

void set_pixel(led_strip_handle_t strip, uint32_t idx, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness_pct)
{
    if(brightness_pct == 100) {
      led_strip_set_pixel(strip, idx, r, g, b);
      return;
    }

    uint8_t br = brightness_pct > 100 ? 100 : brightness_pct;
    uint32_t scale = br * br;
    led_strip_set_pixel(strip, idx, r * scale / 10000, g * scale / 10000, b * scale / 10000);
}

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

void lid_off(void)
{
  for (int i = 0; i < NUM_EYELID; i++)
      off(s_eyelid, i);
}

void eye_off(void)
{
  for (int i = 0; i < NUM_EYEBALL; i++)
      off(s_eyeball, i);
}

void occlude_eye(int count, int percent_complete)
{
    const rgb_t c = g_cfg->eyeball_colour;
    for (int i = 0; i < NUM_EYEBALL; i++)
        set_pixel(s_eyeball, i, c.r, c.g, c.b, 100);

    for (int i = 0; i < count && i < 6; i++) {
        off(s_eyeball, 8 - i);
        off(s_eyeball, 9 + i);
        off(s_eyeball, (20 + i) % 22);
        off(s_eyeball, 19 - i);
    }

    if (count < 6) {
        int fade = percent_complete;
        set_pixel(s_eyeball, 8 - count, c.r, c.g, c.b, fade);
        set_pixel(s_eyeball, 9 + count, c.r, c.g, c.b, fade);
        set_pixel(s_eyeball, (20 + count) % 22, c.r, c.g, c.b, fade);
        set_pixel(s_eyeball, 19 - count, c.r, c.g, c.b, fade);
    }
}

#define START_TOP  4
#define END_TOP  26
#define START_BOTTOM  32
#define END_BOTTOM  54
void occlude_lid(int count, int percent_complete)
{
  occlude_top_lid(count, percent_complete);
  occlude_bottom_lid(count, percent_complete, true);
}

void occlude_top_lid(int count, int percent_complete)
{
  for(int i=START_TOP ; i < END_TOP; i++) off(s_eyelid, i);

  int start_top = START_TOP + count;
  int end_top = END_TOP - count;

  set_pixel(s_eyelid, start_top, 255, 0, 0, percent_complete);
  set_pixel(s_eyelid, end_top, 255, 0, 0, percent_complete);

  for (int i = start_top+1; i < end_top; i++)
    set_pixel(s_eyelid, i, 255, 0, 0, 100);
}

void occlude_bottom_lid(int count, int percent_complete, bool blue)
{
  for(int i=START_BOTTOM ; i < END_BOTTOM; i++) off(s_eyelid, i);

  int start_bottom = START_BOTTOM + count;
  int end_bottom = END_BOTTOM - count;

  uint8_t r = blue ? 0 : 255;
  uint8_t b = blue ? 255 : 0;

  set_pixel(s_eyelid, start_bottom, r, 0, b, percent_complete);
  set_pixel(s_eyelid, end_bottom, r, 0, b, percent_complete);

  for (int i = start_bottom+1; i < end_bottom; i++)
    set_pixel(s_eyelid, i, r, 0, b, 100);
}

void transition_lids(int count, int percent_complete) {
  for(int i=0 ; i < NUM_EYELID ; i++) off(s_eyelid, i);

  int start_top = START_TOP + count;
  int end_top = END_TOP - count;
  int start_bottom = START_BOTTOM + count;
  int end_bottom = END_BOTTOM - count;

  percent_complete = 100 - percent_complete;

  for (int i = start_top+1; i < end_top; i++)
    set_pixel(s_eyelid, i, 255, 0, 0, percent_complete);

  for (int i = start_bottom+1; i < end_bottom; i++)
    set_pixel(s_eyelid, i, 255 * percent_complete / 100, 0, 255 * (100 - percent_complete) / 100, 100);
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
