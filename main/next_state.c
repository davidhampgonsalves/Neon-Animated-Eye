#include "state.h"

anim_state_t next_state(anim_state_t current)
{
    (void)current;
    state.state_duration_ms = 0;
    return STATE_IDLE_OPEN;
}
