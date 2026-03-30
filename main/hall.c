#include "hall.h"
#include "state.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "hall";

#define HALL_ADC_UNIT    ADC_UNIT_1
#define HALL_ADC_CHANNEL ADC_CHANNEL_1

#define SAMPLE_INTERVAL_US  50000
#define RISE_THRESHOLD   50
#define FALL_THRESHOLD   50
#define BASELINE_NEAR    20

typedef enum { HALL_IDLE, HALL_RISING, HALL_FALLING } hall_state_t;

static adc_oneshot_unit_handle_t s_adc;

static int64_t     s_last_sample_us;
static hall_state_t s_hall_state;
static int         s_baseline;
static int         s_peak_val;
static int64_t     s_peak_time_us;

void hall_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = HALL_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, HALL_ADC_CHANNEL, &chan_cfg));

    s_last_sample_us = 0;
    s_hall_state = HALL_IDLE;
    s_baseline = 0;
    s_peak_val = 0;

    ESP_LOGI(TAG, "ADC ready (unit %d, ch %d)", HALL_ADC_UNIT, HALL_ADC_CHANNEL);
}

void hall_reset(void)
{
    s_hall_state = HALL_IDLE;
    s_baseline = 0;
    s_peak_val = 0;
}

int hall_read(void)
{
    int raw = 0;
    adc_oneshot_read(s_adc, HALL_ADC_CHANNEL, &raw);
    return raw;
}

void hall_monitor(void)
{
    int64_t now_us = esp_timer_get_time();
    if (now_us - s_last_sample_us < SAMPLE_INTERVAL_US)
        return;
    s_last_sample_us = now_us;

    int raw = hall_read();

    switch (s_hall_state) {
    case HALL_IDLE:
        if (s_baseline == 0)
            s_baseline = raw;
        else
            s_baseline = (s_baseline * 7 + raw) / 8;

        if (raw > s_baseline + RISE_THRESHOLD) {
            s_hall_state = HALL_RISING;
            s_peak_val = raw;
            s_peak_time_us = now_us;
            uint32_t elapsed_ms = (uint32_t)((now_us - state.state_start_us) / 1000);
            ESP_LOGI(TAG, "rise detected, raw=%d baseline=%d elapsed=%lu", raw, s_baseline, (unsigned long)elapsed_ms);
        }
        break;

    case HALL_RISING:
        if (raw > s_peak_val) {
            s_peak_val = raw;
            s_peak_time_us = now_us;
        }
        if (raw < s_peak_val - FALL_THRESHOLD) {
            s_hall_state = HALL_FALLING;
            state.zero_position_us = s_peak_time_us;
            state.zero_detected = true;
            uint32_t elapsed_ms = (uint32_t)((now_us - state.state_start_us) / 1000);
            ESP_LOGI(TAG, "fall detected, peak=%d elapsed=%lu", s_peak_val, (unsigned long)elapsed_ms);
        }
        break;

    case HALL_FALLING:
        if (raw < s_baseline + BASELINE_NEAR) {
            s_hall_state = HALL_IDLE;
            s_peak_val = 0;
        }
        break;
    }
}
