#include "blink.h"
#include "config.h"
#include "state.h"
#include "motor.h"
#include "leds.h"
#include "esp_log.h"

#define CYCLE_HALF (CYCLE + HALF_CYCLE)
#define TWO_CYCLE (CYCLE * 2)

#define START 0
#define LIDS_MEET (HALF_CYCLE - 1000)
#define EYE_CLOSED (HALF_CYCLE + (HALF_CYCLE/4))
#define LIDS_MEET_2 (EYE_CLOSED + (EYE_CLOSED - LIDS_MEET))
#define FINISH -1 // probably don't need

uint32_t eye_schedule[] = { 3700, 3882, 4000, 4329, 5125, 5722 };
size_t eye_schedule_len = sizeof(eye_schedule) / sizeof(eye_schedule[0]);

uint32_t lid_schedule[] = { 0, 2000, 4500 };
size_t lid_schedule_len = sizeof(lid_schedule) / sizeof(lid_schedule[0]);

void animate_open_close(uint32_t elapsed) {
  elapsed = elapsed % CYCLE;
  if(elapsed > HALF_CYCLE) elapsed = HALF_CYCLE-(elapsed-HALF_CYCLE);

  if (elapsed < eye_schedule[0]) {
    occlude_eye(0, 100);
  } else {
    for (int count = 0; count < eye_schedule_len; count++) {
      uint32_t start = eye_schedule[count];
      uint32_t end = (count + 1) >= eye_schedule_len ? HALF_CYCLE : eye_schedule[count + 1];

      if (elapsed > end) continue;

      int percentage_complete = 100 - ((elapsed - start) * 100 / (end - start));
      occlude_eye(count, percentage_complete);

      break;
    }
  }

  for(int count=0 ; count < lid_schedule_len ; count++) {
    uint32_t start = lid_schedule[count];
    uint32_t end = (count + 1) >= lid_schedule_len ? HALF_CYCLE : lid_schedule[count+1];

    if(elapsed > end) continue;

    int percentage_complete = 100 - ((elapsed - start) * 100 / (end - start));

    // ESP_LOGI("blink", "elapsed=%lu, %=%d, start=%lu, end=%lu", elapsed, percentage_complete, start, end);

    occlude_lid(count, percentage_complete);
    break;
  }
}


#define LID_TRANSITION_END (LIDS_MEET + 500)
#define LID_TRANSITION_END_2 (LIDS_MEET_2 + 500)
#define LID_TRANSITION_DURATION (LID_TRANSITION_END - LIDS_MEET)
void animate_lid_close(uint32_t elapsed) {
  lid_off();
  eye_off();
  if(elapsed > CYCLE) elapsed = CYCLE-(elapsed-CYCLE);

  if(elapsed < LID_TRANSITION_END || (elapsed > LIDS_MEET_2 && elapsed < LID_TRANSITION_END_2)) {
    motor_drive_fast(true);
    int transition_percent = (elapsed - LIDS_MEET) * 100 / (LID_TRANSITION_END - LIDS_MEET);
    if(elapsed < LID_TRANSITION_DURATION)
      transition_percent = 100 - transition_percent;

    ESP_LOGI("animate_lid_close", "transition elapsed=%lu, percent=%d", (unsigned long)elapsed, transition_percent);
    transition_lids(2, transition_percent);
  } else {
    ESP_LOGI("animate_lid_close", "open/close elapsed=%lu", (unsigned long)elapsed);
    for(int count=0 ; count < lid_schedule_len ; count++) {
      uint32_t start = lid_schedule[count] + HALF_CYCLE;
      uint32_t end = (count + 1) >= lid_schedule_len ? CYCLE : lid_schedule[count+1] + HALF_CYCLE;

      if(elapsed > end) continue;

      int percentage_complete = 100 - ((elapsed - start) * 100 / (end - start));
      occlude_bottom_lid(count, percentage_complete, false);

      if(elapsed < EYE_CLOSED)
        motor_drive(true);
      else if(elapsed > EYE_CLOSED && elapsed < EYE_CLOSED + 50)
        motor_coast();
      else
        motor_drive(false);

      break;
    }
  }
}

bool blink(uint32_t elapsed)
{
  if(elapsed < LIDS_MEET) {
    motor_drive(true);
    animate_open_close(elapsed);
  } else if(elapsed < CYCLE_HALF)
    animate_lid_close(elapsed);
  else if(elapsed < TWO_CYCLE) {
    motor_drive(true);
    animate_open_close(elapsed);
  } else {
    motor_coast();
    return true;
  }

  leds_refresh();

  return false;
}
