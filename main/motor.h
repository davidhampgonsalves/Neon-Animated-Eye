#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MOTOR_SPEED       200
#define MOTOR_SPEED_CLOSE 205
#define MOTOR_SPEED_OPEN  255
#define MOTOR_SPEED_PEAK  255

void motor_init(void);
void motor_drive(bool forward);
void motor_drive_at_speed(bool forward, uint8_t speed);
void motor_drive_homing(void);
void motor_coast(void);
