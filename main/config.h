#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct { uint8_t r, g, b; } rgb_t;

static inline rgb_t rgb_make(uint8_t r, uint8_t g, uint8_t b) {
    rgb_t c = {r, g, b}; return c;
}
static inline rgb_t rgb_scale(rgb_t c, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    rgb_t out = {
        (uint8_t)(c.r * t),
        (uint8_t)(c.g * t),
        (uint8_t)(c.b * t),
    };
    return out;
}

typedef struct {
    float    homing_peak_hysteresis;
    uint32_t estimated_cycle_ms;

    uint32_t blink_pause_min_ms;
    uint32_t blink_pause_max_ms;

    float    ease_in_fraction;
    float    ease_out_fraction;

    float    occlusion_start_phase;
    float    occlusion_end_phase;

    int      eyeball_top_first;
    int      eyeball_top_last;
    int      eyeball_bot_first;
    int      eyeball_bot_last;

    int      lid_wiper_offset;
    int      lid_end_occlusion;

    rgb_t    eyelid_colour;
    rgb_t    eyeball_colour;

    float    flame_probability;
    uint32_t flame_flicker_rate_ms;
} config_t;

static const config_t CONFIG_DEFAULT = {
    .homing_peak_hysteresis    = 0.15f,
    .estimated_cycle_ms        = 7500,

    .blink_pause_min_ms        = 10000,
    .blink_pause_max_ms        = 60000,

    .ease_in_fraction          = 0.20f,
    .ease_out_fraction         = 0.20f,

    .occlusion_start_phase     = 0.35f,
    .occlusion_end_phase       = 0.85f,

    .eyeball_top_first         = 8,
    .eyeball_top_last          = 9,
    .eyeball_bot_first         = 19,
    .eyeball_bot_last          = 20,

    .lid_wiper_offset          = 0,
    .lid_end_occlusion         = 10,

    .eyelid_colour             = {255, 200, 160},
    .eyeball_colour            = {0,   180, 255},

    .flame_probability         = 0.10f,
    .flame_flicker_rate_ms     = 80,
};
