#include "blink.h"
#include "state.h"
#include "leds.h"

bool blink(uint32_t elapsed)
{
  if(elapsed < 3882)
    occlude_eye(0);
  else if(elapsed < 3882)
    occlude_eye(1);
  else if(elapsed < 4329)
    occlude_eye(2);
  else if(elapsed < 4329)
    occlude_eye(3);
  else if(elapsed < 5125)
    occlude_eye(4);
  else if(elapsed < 5722)
    occlude_eye(5);
  else
    occlude_eye(6);

  if(elapsed < 3882)
    occlude_lid(4);
  else
    occlude_lid(5);

  leds_refresh();

  return false;
}
