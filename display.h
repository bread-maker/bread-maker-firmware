#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <avr/io.h>

#define DISPLAY_RAW 0
#define DISPLAY_TIME 1
#define DISPLAY_TEMPERATURE 2
#define DISPLAY_TIME_LEFT 3
#define DISPLAY_TIME_PASSED 4

extern volatile uint8_t display_mode;
extern volatile uint8_t display[5];
extern volatile uint8_t display_col;
extern const uint8_t digit_codes[];

void show_digit(uint8_t pos, uint8_t digit);
void show_number(uint16_t number);
void show_temperature();
void show_hour_min(uint8_t h, uint8_t m, uint8_t d);
void show_time();
void show_time_left();
void show_time_passed();
void update_display();

#endif
