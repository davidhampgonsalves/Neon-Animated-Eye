#pragma once
#include <stdint.h>
#include <stdbool.h>

void leds_init(void);
void leds_off(void);
void occlude_eye(int count);
void occlude_lid(int count);
void leds_refresh(void);
