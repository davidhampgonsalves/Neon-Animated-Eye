#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "led_strip.h"

void leds_init(void);
void leds_off(void);
void occlude_eye(int count);
void occlude_lid(int count, int percent_complete);
void leds_refresh(void);
void set_pixel(led_strip_handle_t strip, uint32_t idx, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness_pct);
