#include "blink.h"
#include "config.h"
#include "state.h"
#include "motor.h"
#include "leds.h"
#include "esp_log.h"
#include <stdlib.h>

extern const config_t *g_cfg;

static uint32_t (*s_eye_schedule)[2];
static size_t s_eye_schedule_len;
static uint32_t (*s_lid_schedule)[2];
static size_t s_lid_schedule_len;

#define TC_LIDS_MEET       1800
#define TC_LID_TRANS_END   1970
#define TC_EYE_CLOSED      2400
#define TC_LIDS_MEET_2     3000
#define TC_LID_TRANS_END_2 3170

#define SPEED_RAMP_WINDOW 690

static uint8_t get_motor_speed(uint32_t elapsed)
{
  int32_t dist1 = abs((int32_t)elapsed - (int32_t)TC_LIDS_MEET);
  int32_t dist2 = abs((int32_t)elapsed - (int32_t)TC_LIDS_MEET_2);
  int32_t dist = dist1 < dist2 ? dist1 : dist2;
  if (dist >= SPEED_RAMP_WINDOW) return MOTOR_SPEED_CLOSE;
  uint16_t speed = MOTOR_SPEED_CLOSE + (MOTOR_SPEED_PEAK - MOTOR_SPEED_CLOSE) * (SPEED_RAMP_WINDOW - dist) / SPEED_RAMP_WINDOW;
  return speed > 255 ? 255 : (uint8_t)speed;
}

uint32_t eye_schedule_half[][2] = {
  { 0,    0 },
  { 850,  1 },
  { 980, 2 },
  { 1100, 3 },
  { 1260, 4 },
  { 1500, 5 },
  { 1800, 6 },
  { 1900, 5 },
  { 2100, 4 },
  { 2350, 3 },
  { 2550, 2 },
  { 2650, 1 },
  { 2730, 0 },
};
size_t eye_schedule_half_len = sizeof(eye_schedule_half) / sizeof(eye_schedule_half[0]);

uint32_t eye_schedule_full[][2] = {
  { 0,    0 },
  { 850,  1 },
  { 980,  2 },
  { 1100, 3 },
  { 1260, 4 },
  { 1500, 5 },
  { 1800, 6 },
  { 1900, 5 },
  { 2100, 4 },
  { 2350, 3 },
  { 2550, 2 },
  { 2650, 1 },
  { 2730, 0 },
};
size_t eye_schedule_full_len = sizeof(eye_schedule_full) / sizeof(eye_schedule_full[0]);

uint32_t lid_schedule_half[][2] = {
  { 0,    0 },
  { 170,  0 },
  { 690,  1 },
  { 1550, 2 },
  { 2400, 2 },
  { 2800, 1 },
  { 3900, 0 },
};
size_t lid_schedule_half_len = sizeof(lid_schedule_half) / sizeof(lid_schedule_half[0]);

uint32_t lid_schedule_full[][2] = {
  { 0,   0 },
  { 50,  0 },
  { 150, 1 },
  { 350, 2 },
};
size_t lid_schedule_full_len = sizeof(lid_schedule_full) / sizeof(lid_schedule_full[0]);

static uint32_t s_anim_cycle;

void animate_open_close(uint32_t elapsed) {
  uint32_t eye_elapsed = elapsed % s_anim_cycle;
  uint32_t lid_elapsed = elapsed % s_anim_cycle;

  bool eye_set = false;
  for (int i = 0; i < s_eye_schedule_len; i++) {
    uint32_t start = s_eye_schedule[i][0];
    uint32_t end = (i + 1) >= s_eye_schedule_len ? s_anim_cycle : s_eye_schedule[i + 1][0];

    if (eye_elapsed > end) continue;

    int pct;
    int count = s_eye_schedule[i][1];
    int next_count = (i + 1) < s_eye_schedule_len ? s_eye_schedule[i + 1][1] : count;

    if (next_count > count)
      pct = 100 - ((eye_elapsed - start) * 100 / (end - start));
    else if (next_count < count)
      pct = (eye_elapsed - start) * 100 / (end - start);
    else
      pct = 100;

    occlude_eye(count, pct);
    eye_set = true;
    break;
  }
  if (!eye_set)
    occlude_eye(0, 100);

  bool lid_set = false;
  for (int i = 0; i < s_lid_schedule_len; i++) {
    uint32_t start = s_lid_schedule[i][0];
    uint32_t end = (i + 1) >= s_lid_schedule_len ? s_anim_cycle : s_lid_schedule[i + 1][0];

    if (lid_elapsed > end) continue;

    int pct;
    int lid_count = s_lid_schedule[i][1];
    int lid_next = (i + 1) < s_lid_schedule_len ? s_lid_schedule[i + 1][1] : lid_count;

    if (lid_next > lid_count) {
      pct = 100 - ((lid_elapsed - start) * 100 / (end - start));
      occlude_lid(lid_count, pct);
    } else if (lid_next < lid_count) {
      pct = (lid_elapsed - start) * 100 / (end - start);
      occlude_lid(lid_next, pct);
    } else {
      occlude_lid(lid_count, 100);
    }
    lid_set = true;
    break;
  }
  if (!lid_set)
    occlude_lid(0, 100);
}


void animate_lid_close(uint32_t elapsed) {
  lid_off();
  eye_off();

  if (elapsed < TC_LID_TRANS_END) {
    int pct = (elapsed - TC_LIDS_MEET) * 100 / (TC_LID_TRANS_END - TC_LIDS_MEET);
    transition_lids(2, pct);

  } else if (elapsed < TC_LIDS_MEET_2) {
    uint32_t e = elapsed;
    if (e > TC_EYE_CLOSED)
      e = TC_EYE_CLOSED - (e - TC_EYE_CLOSED);

    for (int i = 0; i < s_lid_schedule_len; i++) {
      uint32_t start = s_lid_schedule[i][0] + TC_LID_TRANS_END;
      uint32_t end = (i + 1) >= s_lid_schedule_len ? TC_EYE_CLOSED : s_lid_schedule[i + 1][0] + TC_LID_TRANS_END;
      if (e > end) continue;
      int percentage_complete = 100 - ((e - start) * 100 / (end - start));
      occlude_bottom_lid(s_lid_schedule[i][1], percentage_complete, false);
      break;
    }

  } else {
    int pct = (elapsed - TC_LIDS_MEET_2) * 100 / (TC_LID_TRANS_END_2 - TC_LIDS_MEET_2);
    transition_lids(2, 100 - pct);
  }
}

static bool s_logged_constants = false;

static bool blink_one_cycle(uint32_t elapsed)
{
  if (!s_logged_constants) {
    ESP_LOGI("blink", "ONE_CYCLE: half_cycle=%lu cycle=%lu",
             (unsigned long)HALF_CYCLE_HALF_MODE, (unsigned long)CYCLE_HALF_MODE);
    s_logged_constants = true;
  }

  if (elapsed < HALF_CYCLE_HALF_MODE)
    state.zero_detected = false;

  uint32_t phase = elapsed % CYCLE_HALF_MODE;
  if (phase < HALF_CYCLE_HALF_MODE)
    motor_drive_at_speed(true, MOTOR_SPEED_CLOSE);
  else
    motor_drive_at_speed(true, MOTOR_SPEED_OPEN);

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
  if (!s_logged_constants) {
    ESP_LOGI("blink", "TWO_CYCLE: half_cycle=%lu cycle=%lu lids_meet=%d eye_closed=%d lids_meet_2=%d lid_trans_end=%d lid_trans_end_2=%d",
             (unsigned long)HALF_CYCLE_HALF_MODE, (unsigned long)CYCLE_HALF_MODE,
             TC_LIDS_MEET, TC_EYE_CLOSED, TC_LIDS_MEET_2,
             TC_LID_TRANS_END, TC_LID_TRANS_END_2);
    s_logged_constants = true;
  }

  uint8_t speed = get_motor_speed(elapsed);

  if (elapsed < TC_LIDS_MEET) {
    state.zero_detected = false;
    motor_drive_at_speed(true, speed);
    animate_open_close(elapsed);
  } else if (elapsed < TC_LID_TRANS_END_2) {
    state.zero_detected = false;
    if (elapsed < TC_EYE_CLOSED)
      motor_drive_at_speed(true, speed);
    else if (elapsed < TC_EYE_CLOSED + 50)
      motor_coast();
    else if (elapsed < TC_LIDS_MEET_2)
      motor_drive_at_speed(false, speed);
    else
      motor_drive_at_speed(true, speed);
    animate_lid_close(elapsed);
  } else {
    motor_drive_at_speed(true, speed);
    animate_open_close(elapsed - (TC_LID_TRANS_END_2 - TC_LIDS_MEET));

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
  static uint32_t s_last_log = 0;
  if (elapsed - s_last_log > 500 || elapsed < s_last_log) {
    ESP_LOGI("blink", "elapsed=%lu", (unsigned long)elapsed);
    s_last_log = elapsed;
  }

  s_anim_cycle = CYCLE_HALF_MODE;

  if (g_cfg->two_cycle_animation) {
    s_eye_schedule = eye_schedule_full;
    s_eye_schedule_len = eye_schedule_full_len;
    s_lid_schedule = lid_schedule_full;
    s_lid_schedule_len = lid_schedule_full_len;
    return blink_two_cycle(elapsed);
  }
  s_eye_schedule = eye_schedule_half;
  s_eye_schedule_len = eye_schedule_half_len;
  s_lid_schedule = lid_schedule_half;
  s_lid_schedule_len = lid_schedule_half_len;
  return blink_one_cycle(elapsed);
}

#define SQUINT_CLOSE_MS   2100
#define SQUINT_HOLD_MS    1700
#define SQUINT_OPEN_MS    2400
#define SQUINT_TOTAL_MS   (SQUINT_CLOSE_MS + SQUINT_HOLD_MS + SQUINT_OPEN_MS)

uint32_t eye_schedule_squint[][2] = {
  { 0,    0 },
  { 700,  0 },
  { 1100, 1 },
  { 1500, 2 },
  { 2000, 3 },
  { 3800, 3 },
  { 4200, 2 },
  { 4800, 1 },
  { 5500, 0 },
};
size_t eye_schedule_squint_len = sizeof(eye_schedule_squint) / sizeof(eye_schedule_squint[0]);

uint32_t lid_schedule_squint[][2] = {
  { 0,    0 },
  { 500,  0 },
  { 1800, 1 },
  { 3800, 1 },
  { 5200, 0 },
};
size_t lid_schedule_squint_len = sizeof(lid_schedule_squint) / sizeof(lid_schedule_squint[0]);

bool squint(uint32_t elapsed)
{
  static uint32_t s_last_log = 0;
  if (elapsed - s_last_log > 500 || elapsed < s_last_log) {
    ESP_LOGI("squint", "elapsed=%lu", (unsigned long)elapsed);
    s_last_log = elapsed;
  }

  if (elapsed >= SQUINT_TOTAL_MS) {
    motor_coast();
    return true;
  }

  if (elapsed < SQUINT_CLOSE_MS)
    motor_drive_at_speed(true, MOTOR_SPEED_SQUINT_CLOSE);
  else if (elapsed < SQUINT_CLOSE_MS + SQUINT_HOLD_MS)
    motor_coast();
  else
    motor_drive_at_speed(false, MOTOR_SPEED_SQUINT_OPEN);

  s_anim_cycle = SQUINT_TOTAL_MS;
  s_eye_schedule = eye_schedule_squint;
  s_eye_schedule_len = eye_schedule_squint_len;
  s_lid_schedule = lid_schedule_squint;
  s_lid_schedule_len = lid_schedule_squint_len;

  animate_open_close(elapsed);

  leds_refresh();
  return false;
}
