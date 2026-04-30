#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <stdint.h>
#include "driver/ledc.h" // Required for ledc_channel_t

// Motor A
#define MOTOR_A_IN1  18
#define MOTOR_A_IN2  19
#define MOTOR_A_ENA  5

// Motor B
#define MOTOR_B_IN3  22
#define MOTOR_B_IN4  23
#define MOTOR_B_ENB  4

// PWM Settings
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES   LEDC_TIMER_13_BIT // 0 to 8191
#define LEDC_FREQUENCY  5000              

// Function Prototypes
void init_motors(void);
void set_motor_speed(ledc_channel_t channel, uint32_t duty); 
void motor_a_forward_xspeed(unsigned int x);
void motor_b_forward_xspeed(unsigned int x);
void motor_a_reverse_xspeed(unsigned int x);
void motor_b_reverse_xspeed(unsigned int x);
void motors_stop(void);
void handle_controller(int turn, unsigned int reverse, unsigned int forward);

#endif