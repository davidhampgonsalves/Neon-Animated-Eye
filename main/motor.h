#pragma once
#include <stdint.h>
#include <stdbool.h>

void motor_init(void);
void motor_drive(bool forward);
void motor_drive_fast(bool forward);
void motor_drive_homing(void);
void motor_coast(void);
