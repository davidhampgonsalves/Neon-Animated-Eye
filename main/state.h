#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    STATE_IDLE_OPEN,
    STATE_BLINKING,
    STATE_MANUAL_ANIMATION,
} anim_state_t;

typedef struct {
    anim_state_t state;
    int64_t      state_start_us;
    uint32_t     state_duration_ms;
    int64_t      zero_position_us;
} state_t;

extern state_t state;

anim_state_t next_state(anim_state_t current);
