#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MOTOR_SPEED      180
#define MOTOR_SPEED_PEAK 250

void motor_init(void);
void motor_drive(bool forward);
void motor_drive_at_speed(bool forward, uint8_t speed);
void motor_drive_homing(void);
void motor_coast(void);
