#ifndef _BREADMAKER_H_
#define _BREADMAKER_H_

#include <avr/io.h>

#define STATE_IDLE 0
#define STATE_TIMER 1
#define STATE_BAKING 2
#define STATE_WARMING 3
#define STATE_ERROR 4

#define MOTOR_STOPPED 0
#define MOTOR_IMPULSE 1
#define MOTOR_RUNNING 2

struct baking_stage
{
	uint8_t temperature;
	uint8_t motor;
	uint16_t duration;
};

struct baking_beep
{
	uint8_t stage;
	uint16_t time;
	uint8_t count;
};

extern volatile uint32_t seconds;
extern volatile uint32_t millis;
extern volatile uint8_t hour;
extern volatile uint8_t min;
extern volatile uint8_t sec;
extern volatile uint8_t half_sec;
extern volatile uint8_t motor_state;
extern volatile uint8_t current_state;
extern volatile uint8_t last_error;
extern volatile struct baking_stage baking_program[32];
extern volatile struct baking_beep baking_beeps[4];
extern volatile uint8_t baking_stage_count;
extern volatile uint8_t baking_current_stage;
extern volatile uint8_t baking_beeps_count;
extern volatile uint16_t current_stage_time;
extern volatile uint8_t program_number;
extern volatile uint8_t crust_number;
extern volatile uint32_t delayed_secs;
extern volatile uint32_t passed_secs;
extern volatile int8_t cmd_start;
extern volatile int8_t cmd_abort;
extern volatile int8_t cmd_abort_err;
extern volatile int8_t warming_temperature;
extern volatile int16_t warming_max_time;
extern volatile uint8_t max_temperature_before_timer;
extern volatile uint8_t max_temperature_before_baking;
extern volatile uint16_t program_duration[PROGRAM_COUNT];

void do_stuff();
void show_digit(uint8_t pos, uint8_t digit);
void show_number(uint16_t number);
uint8_t read_buttons();

extern volatile uint32_t millis;
extern volatile uint8_t display[5];

#endif
