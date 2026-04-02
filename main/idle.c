#include "idle.h"
#include "state.h"
#include "esp_log.h"

static bool s_logged = false;

bool idle(uint32_t elapsed_ms) {
    if (!s_logged) {
        ESP_LOGI("idle", "waiting %lums before next movement", (unsigned long)state.state_duration_ms);
        s_logged = true;
    }
    if (elapsed_ms >= state.state_duration_ms) {
        s_logged = false;
        return true;
    }
    return false;
}
