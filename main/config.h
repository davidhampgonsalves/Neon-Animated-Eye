#pragma once
#include <stdint.h>
#include <stdbool.h>

#define HALF_CYCLE_HALF_MODE 2300
#define CYCLE_HALF_MODE (HALF_CYCLE_HALF_MODE * 2)

#define HALF_CYCLE_SQUINT 1300
#define CYCLE_SQUINT (HALF_CYCLE_SQUINT * 2)

typedef struct { uint8_t r, g, b; } rgb_t;

typedef struct {
    rgb_t    eyeball_colour;
    bool     two_cycle_animation;
} config_t;

static const config_t CONFIG_DEFAULT = {
    .eyeball_colour            = {0, 180, 255},
    .two_cycle_animation       = false,
};
