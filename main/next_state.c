#include "state.h"
#include <stdint.h>
#include "esp_timer.h"

#define IDLE_MIN_MS 5000
#define IDLE_MAX_MS 30000

static anim_state_t s_sequence[] = {
    STATE_BLINKING,
    STATE_SQUINTING,
    STATE_NEON_FLICKER,
};
static int s_seq_idx = 0;
static const int s_seq_len = sizeof(s_sequence) / sizeof(s_sequence[0]);

anim_state_t next_state(anim_state_t current)
{
    if (current == STATE_IDLE_OPEN) {
        anim_state_t next = s_sequence[s_seq_idx];
        s_seq_idx = (s_seq_idx + 1) % s_seq_len;
        state.state_duration_ms = 0;
        return next;
    }

    state.state_duration_ms = IDLE_MIN_MS + (uint32_t)(esp_timer_get_time() % (IDLE_MAX_MS - IDLE_MIN_MS));
    return STATE_IDLE_OPEN;
}
