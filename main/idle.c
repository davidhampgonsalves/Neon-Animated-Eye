#include "idle.h"
#include "state.h"

bool idle(uint32_t elapsed_ms)
{
    return elapsed_ms >= state.state_duration_ms;
}
