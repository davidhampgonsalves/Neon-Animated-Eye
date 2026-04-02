#include "neon_flicker.h"
#include "leds.h"
#include "esp_timer.h"

#define FLICKER_DURATION 2000

static const uint32_t flicker_pattern[][2] = {
    { 0,    1 },
    { 300,  0 },
    { 380,  1 },
    { 900,  0 },
    { 980,  1 },
    { 1500, 0 },
    { 1540, 1 },
    { 2000, 1 },
};
static const size_t flicker_len = sizeof(flicker_pattern) / sizeof(flicker_pattern[0]);

static int s_target = -1;

static bool lookup(const uint32_t schedule[][2], size_t len, uint32_t elapsed)
{
    bool on = true;
    for (int i = len - 1; i >= 0; i--) {
        if (elapsed >= schedule[i][0]) {
            on = schedule[i][1];
            break;
        }
    }
    return on;
}

bool neon_flicker(uint32_t elapsed)
{
    if (elapsed >= FLICKER_DURATION) {
        s_target = -1;
        return true;
    }

    if (s_target < 0)
        s_target = (int)(esp_timer_get_time() % 3);

    bool on = lookup(flicker_pattern, flicker_len, elapsed);

    leds_set_bottom_lid(s_target == 0 ? on : true);
    leds_set_eyeball(s_target == 1 ? on : true);
    leds_set_top_lid(s_target == 2 ? on : true);
    leds_refresh();

    return false;
}
