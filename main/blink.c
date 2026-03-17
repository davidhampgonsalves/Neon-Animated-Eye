#include "blink.h"
#include "state.h"
#include "leds.h"
#include "esp_log.h"

#define HALF_CYCLE 6687
#define CYCLE (HALF_CYCLE * 2)

uint32_t eye_schedule[] = { 3700, 3882, 4000, 4329, 5125, 5722 };
size_t eye_schedule_len = sizeof(eye_schedule) / sizeof(eye_schedule[0]);

uint32_t lid_schedule[] = { 0, 2000, 4500, 6000 };
size_t lid_schedule_len = sizeof(lid_schedule) / sizeof(lid_schedule[0]);

bool blink(uint32_t elapsed)
{
  elapsed = elapsed % CYCLE;
  if(elapsed > HALF_CYCLE) elapsed = HALF_CYCLE-(elapsed-HALF_CYCLE);

  ESP_LOGI("blink", "elapsed=%lu, motor time=%lu", elapsed, state.motor_time_ms);

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

  leds_refresh();

  return false;
}
