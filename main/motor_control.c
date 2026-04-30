#include "motor_control.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

void init_motors() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL<<MOTOR_A_IN1) | (1ULL<<MOTOR_A_IN2) | 
                        (1ULL<<MOTOR_B_IN3) | (1ULL<<MOTOR_B_IN4),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t channel_a = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = MOTOR_A_ENA,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&channel_a);

    ledc_channel_config_t channel_b = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_1,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = MOTOR_B_ENB,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&channel_b);
}

void set_motor_speed(ledc_channel_t channel, uint32_t duty) {
    ledc_set_duty(LEDC_MODE, channel, duty);
    ledc_update_duty(LEDC_MODE, channel);
}

void motor_a_forward_xspeed(unsigned int x) {
    if (x > 100) x = 100;
    unsigned int speed = 81.91 * x; // Scale 0-100 to 0-8191
    gpio_set_level(MOTOR_A_IN1, 1);
    gpio_set_level(MOTOR_A_IN2, 0);
    set_motor_speed(LEDC_CHANNEL_0, speed);
}

void motor_b_forward_xspeed(unsigned int x) {
    if (x > 100) x = 100;
    unsigned int speed = 81.91 * x;
    gpio_set_level(MOTOR_B_IN3, 1);
    gpio_set_level(MOTOR_B_IN4, 0);
    set_motor_speed(LEDC_CHANNEL_1, speed);
}

void motor_a_reverse_xspeed(unsigned int x) {
    if (x > 100) x = 100;
    unsigned int speed = 81.91 * x;
    gpio_set_level(MOTOR_A_IN1, 0);
    gpio_set_level(MOTOR_A_IN2, 1);
    set_motor_speed(LEDC_CHANNEL_0, speed);
}

void motor_b_reverse_xspeed(unsigned int x) {
    if (x > 100) x = 100;
    unsigned int speed = 81.91 * x;
    gpio_set_level(MOTOR_B_IN3, 0);
    gpio_set_level(MOTOR_B_IN4, 1);
    set_motor_speed(LEDC_CHANNEL_1, speed);
}

void motors_stop(void) {
    // Setting both IN pins to HIGH acts as an electronic brake
    gpio_set_level(MOTOR_A_IN1, 1);
    gpio_set_level(MOTOR_A_IN2, 1);
    set_motor_speed(LEDC_CHANNEL_0, 0);
    gpio_set_level(MOTOR_B_IN3, 1);
    gpio_set_level(MOTOR_B_IN4, 1);
    set_motor_speed(LEDC_CHANNEL_1, 0);
}

void handle_controller(int turn, unsigned int reverse, unsigned int forward) {
    // Input Scaling & Clamping
    if (forward > 250) forward = 250;
    if (reverse > 250) reverse = 250;
    
    // sum_GAS range: -100 to 100
    int sum_GAS = (int)(forward - reverse) * 2 / 5; 

    if (turn > 125)  turn = 125;
    if (turn < -125) turn = -125;
    
    // turn_scaled range: -100 to 100
    int turn_scaled = turn * 4 / 5;


    // Prevents the motors from "humming" if the joystick doesn't perfectly center
    if (abs(sum_GAS) < 2 && abs(turn_scaled) < 8) {
        motors_stop();
        return;
    }


    // Left = Throttle + Turn | Right = Throttle - Turn
    int left_speed_raw  = sum_GAS + turn_scaled;
    int right_speed_raw = sum_GAS - turn_scaled;

    //Final Clamping (-100 to 100)
    if (left_speed_raw > 100)  left_speed_raw = 100;
    if (left_speed_raw < -100) left_speed_raw = -100;
    if (right_speed_raw > 100) right_speed_raw = 100;
    if (right_speed_raw < -100) right_speed_raw = -100;

    
    // Handle Left Motor (Motor A)
    if (left_speed_raw > 0) {
        motor_a_reverse_xspeed((unsigned int)left_speed_raw);
    } else if (left_speed_raw < 0) {
        motor_a_forward_xspeed((unsigned int)(-left_speed_raw));
    } else {
        // Stop Motor A specifically
        set_motor_speed(LEDC_CHANNEL_0, 0);
    }

    // Handle Right Motor (Motor B)
    if (right_speed_raw > 0) {
        motor_b_reverse_xspeed((unsigned int)right_speed_raw);
    } else if (right_speed_raw < 0) {
        motor_b_forward_xspeed((unsigned int)(-right_speed_raw));
    } else {
        set_motor_speed(LEDC_CHANNEL_1, 0);
    }

    // Debug output for the monitor
    //printf("GAS: %d | TURN: %d | L: %d | R: %d\n", sum_GAS, turn_scaled, left_speed_raw, right_speed_raw);
}