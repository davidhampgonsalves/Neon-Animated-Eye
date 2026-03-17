#include "blink.h"
#include "state.h"
#include "leds.h"
#include "esp_log.h"

#define HALF_CYCLE 7500
#define CYCLE (HALF_CYCLE * 2)

uint32_t lid_schedule[] = { 0, 2000, 4500, 6000 };
size_t lid_schedule_len = sizeof(lid_schedule) / sizeof(lid_schedule[0]);

bool blink(uint32_t elapsed)
{
  elapsed = elapsed % CYCLE;
  if(elapsed > HALF_CYCLE) elapsed = HALF_CYCLE-(elapsed-HALF_CYCLE);


  if(elapsed < 3700)
    occlude_eye(0);
  else if(elapsed < 3882)
    occlude_eye(1);
  else if(elapsed < 4000)
    occlude_eye(2);
  else if(elapsed < 4329)
    occlude_eye(3);
  else if(elapsed < 5125)
    occlude_eye(4);
  else if(elapsed < 5722)
    occlude_eye(5);
  else
    occlude_eye(6);

  for(int count=0 ; count < lid_schedule_len ; count++) {
    uint32_t start = lid_schedule[count];
    uint32_t end = (count + 1) >= lid_schedule_len ? HALF_CYCLE : lid_schedule[count+1];

    if(elapsed > end) continue;

    int percentage_complete = 100 - ((elapsed - start) * 100 / (end - start));

    // ESP_LOGI("blink", "elapsed=%lu, motor time=%lu, percent=%d", elapsed, state.motor_time_ms, percentage_complete);
    ESP_LOGI("blink", "elapsed=%lu, %=%d, start=%lu, end=%lu", elapsed, percentage_complete, start, end);
    occlude_lid(count, percentage_complete);

    break;
  }

  leds_refresh();

  return false;
}
