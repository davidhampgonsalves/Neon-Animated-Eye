#include "hall.h"
#include "state.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "hall";

#define HALL_ADC_UNIT    ADC_UNIT_1
#define HALL_ADC_CHANNEL ADC_CHANNEL_1

#define WINDOW_SAMPLES   5
#define SAMPLE_INTERVAL_US  50000
#define RISE_THRESHOLD   50

static adc_oneshot_unit_handle_t s_adc;

static int      s_window[WINDOW_SAMPLES];
static int      s_window_idx;
static int64_t  s_last_sample_us;
static int      s_window_first;
static int      s_peak_val;
static int64_t  s_peak_time_us;
static uint32_t s_peak_motor_time_ms;
static bool     s_rise_detected;

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

    s_window_idx = 0;
    s_last_sample_us = 0;
    s_rise_detected = false;
    s_peak_val = 0;
    s_window_first = 0;

    ESP_LOGI(TAG, "ADC ready (unit %d, ch %d)", HALL_ADC_UNIT, HALL_ADC_CHANNEL);
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

    if (s_window_idx == 0) {
        if (!s_rise_detected) {
            s_window_first = raw;
            if (s_peak_val == 0) s_peak_val = raw;
        }
    }

    s_window[s_window_idx] = raw;

    if (raw > s_peak_val) {
        s_peak_val = raw;
        s_peak_time_us = now_us;
        s_peak_motor_time_ms = state.motor_time_ms;
    }

    s_window_idx++;
    if (s_window_idx < WINDOW_SAMPLES)
        return;

    s_window_idx = 0;

    int wmin = s_window[0], wmax = s_window[0];
    for (int i = 1; i < WINDOW_SAMPLES; i++) {
        if (s_window[i] < wmin) wmin = s_window[i];
        if (s_window[i] > wmax) wmax = s_window[i];
    }
    if (wmax - wmin < RISE_THRESHOLD) {
        s_window_first = raw;
        return;
    }

    if (!s_rise_detected) {
        int above = 0;
        for (int i = 0; i < WINDOW_SAMPLES; i++) {
            if (s_window[i] > s_window_first + RISE_THRESHOLD) above++;
        }
        if (above > WINDOW_SAMPLES / 2) {
            s_rise_detected = true;
            ESP_LOGI(TAG, "monitor: rise detected, peak=%d", s_peak_val);
        }
        s_window_first = raw;
    } else {
        int below = 0;
        for (int i = 0; i < WINDOW_SAMPLES; i++) {
            if (s_window[i] < s_peak_val) below++;
        }
        if (below > WINDOW_SAMPLES / 2) {
            ESP_LOGI(TAG, "monitor: fall detected, peak=%d, updating ******** zero_position *********", s_peak_val);
            state.zero_position_us = s_peak_time_us;
            state.motor_time_ms = state.motor_time_ms - s_peak_motor_time_ms;
            s_rise_detected = false;
            s_peak_val = 0;
        }
    }
}
