#include "blink.h"
#include "config.h"
#include "state.h"
#include "motor.h"
#include "leds.h"
#include "esp_log.h"
#include <stdlib.h>

extern const config_t *g_cfg;

static uint32_t s_half_cycle;
static uint32_t s_cycle;

#define START 0
#define LIDS_MEET (s_half_cycle - 1000)
#define EYE_CLOSED (s_half_cycle + (s_half_cycle/8))
#define LIDS_MEET_2 (EYE_CLOSED + (EYE_CLOSED - LIDS_MEET))
#define FINISH -1 // probably don't need

#define SPEED_RAMP_WINDOW 2000

static uint8_t get_motor_speed(uint32_t elapsed)
{
  int32_t dist1 = abs((int32_t)elapsed - (int32_t)LIDS_MEET);
  int32_t dist2 = abs((int32_t)elapsed - (int32_t)LIDS_MEET_2);
  int32_t dist = dist1 < dist2 ? dist1 : dist2;
  if (dist >= SPEED_RAMP_WINDOW) return MOTOR_SPEED;
  return MOTOR_SPEED + (MOTOR_SPEED_PEAK - MOTOR_SPEED) * (SPEED_RAMP_WINDOW - dist) / SPEED_RAMP_WINDOW;
}

uint32_t eye_schedule[] = { 3300, 3500, 3700, 3900, 4300, 5100, 5400 };
size_t eye_schedule_len = sizeof(eye_schedule) / sizeof(eye_schedule[0]);

uint32_t lid_schedule[] = { 500, 2000, 4500 };
size_t lid_schedule_len = sizeof(lid_schedule) / sizeof(lid_schedule[0]);

void animate_open_close(uint32_t elapsed) {
  elapsed = elapsed % s_cycle;
  if(elapsed > s_half_cycle) elapsed = s_half_cycle-(elapsed-s_half_cycle);

  if (elapsed < eye_schedule[0]) {
    occlude_eye(0, 100);
  } else {
    for (int count = 0; count < eye_schedule_len; count++) {
      uint32_t start = eye_schedule[count];
      uint32_t end = (count + 1) >= eye_schedule_len ? s_half_cycle : eye_schedule[count + 1];

      if (elapsed > end) continue;

      int percentage_complete = 100 - ((elapsed - start) * 100 / (end - start));
      occlude_eye(count, percentage_complete);

      break;
    }
  }

  for(int count=0 ; count < lid_schedule_len ; count++) {
    uint32_t start = lid_schedule[count];
    uint32_t end = (count + 1) >= lid_schedule_len ? s_half_cycle : lid_schedule[count+1];

    if(elapsed > end) continue;

    int percentage_complete = 100 - ((elapsed - start) * 100 / (end - start));

    occlude_lid(count, percentage_complete);
    break;
  }
}


#define LID_TRANSITION_END (LIDS_MEET + 500)
#define LID_TRANSITION_END_2 (LIDS_MEET_2 + 500)
void animate_lid_close(uint32_t elapsed) {
  lid_off();
  eye_off();

  if (elapsed < LID_TRANSITION_END) {
    int pct = (elapsed - LIDS_MEET) * 100 / (LID_TRANSITION_END - LIDS_MEET);
    transition_lids(2, pct);

  } else if (elapsed < LIDS_MEET_2) {
    uint32_t e = elapsed;
    if (e > EYE_CLOSED)
      e = EYE_CLOSED - (e - EYE_CLOSED);

    for (int count = 0; count < lid_schedule_len; count++) {
      uint32_t start = lid_schedule[count] + LID_TRANSITION_END;
      uint32_t end = (count + 1) >= lid_schedule_len ? EYE_CLOSED : lid_schedule[count + 1] + LID_TRANSITION_END;
      if (e > end) continue;
      int percentage_complete = 100 - ((e - start) * 100 / (end - start));
      occlude_bottom_lid(count, percentage_complete, false);
      break;
    }

  } else {
    int pct = (elapsed - LIDS_MEET_2) * 100 / (LID_TRANSITION_END_2 - LIDS_MEET_2);
    transition_lids(2, 100 - pct);
  }
}

static bool blink_one_cycle(uint32_t elapsed)
{
  if (elapsed < s_half_cycle)
    state.zero_detected = false;

  motor_drive(true);
  animate_open_close(elapsed);

  if (state.zero_detected) {
    state.zero_detected = false;
    motor_coast();
    return true;
  }

  leds_refresh();
  return false;
}

static bool blink_two_cycle(uint32_t elapsed)
{
  uint8_t speed = get_motor_speed(elapsed);

  if (elapsed < LIDS_MEET) {
    state.zero_detected = false;
    motor_drive_at_speed(true, speed);
    animate_open_close(elapsed);
  } else if (elapsed < LID_TRANSITION_END_2) {
    state.zero_detected = false;
    if (elapsed < EYE_CLOSED)
      motor_drive_at_speed(true, speed);
    else if (elapsed < EYE_CLOSED + 50)
      motor_coast();
    else if (elapsed < LIDS_MEET_2)
      motor_drive_at_speed(false, speed);
    else
      motor_drive_at_speed(true, speed);
    animate_lid_close(elapsed);
  } else {
    motor_drive_at_speed(true, speed);
    animate_open_close(elapsed - LID_TRANSITION_END_2 + s_cycle - LIDS_MEET);

    if (state.zero_detected) {
      state.zero_detected = false;
      motor_coast();
      return true;
    }
  }

  leds_refresh();
  return false;
}

bool blink(uint32_t elapsed)
{
  if (g_cfg->two_cycle_animation) {
    s_half_cycle = HALF_CYCLE_FULL_MODE;
    s_cycle = CYCLE_FULL_MODE;
    return blink_two_cycle(elapsed);
  }
  s_half_cycle = HALF_CYCLE_HALF_MODE;
  s_cycle = CYCLE_HALF_MODE;
  return blink_one_cycle(elapsed);
}
