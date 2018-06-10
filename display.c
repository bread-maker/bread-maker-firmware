#include "defines.h"
#include <avr/io.h>
#include "breadmaker.h"
#include "display.h"
#include "temperature.h"

volatile uint8_t display_mode = DISPLAY_TIME;
volatile uint8_t display[5] = { 0, 0, 0, 0, 0 };
volatile uint8_t display_col = 0;
const uint8_t digit_codes[] = { 0xFC, 0x60, 0xDA, 0xF2, 0x66, 0xB6, 0xBE, 0xE0, 0xFE, 0xF6, 0xEE, 0x3E, 0x9C, 0x7A, 0x9E, 0x8E };

// Shows one digit
void show_digit(uint8_t pos, uint8_t digit)
{
	display[pos] = (display[pos] & 1) | digit_codes[digit % 16];
}

// Shows number
void show_number(uint16_t number)
{
	uint8_t i;
	for (i = 0; i < 4; i++)
	{
		uint8_t digit = number % 10;
		if (number)
			show_digit(3 - i, digit);
		else
			display[3 - i] = 0;
		number /= 10;
	}
	display[1] &= ~1;
	display[2] &= ~1;
}

// Shows temperature
void show_temperature()
{
	uint8_t i;
	uint8_t number = current_temperature;
	for (i = 1; i < 4; i++)
	{
		uint8_t digit = number % 10;
		if (number)
			show_digit(3 - i, digit);
		else
			display[3 - i] = 0;
		number /= 10;
	}
	display[1] &= ~1;
	display[2] &= ~1;
	display[3] = 0xC6;
}

// Shows specified time
void show_hour_min(uint8_t h, uint8_t m, uint8_t d)
{
	show_digit(0, h / 10);
	show_digit(1, h % 10);
	show_digit(2, m / 10);
	show_digit(3, m % 10);
	if (!d)
	{
		display[1] |= 1;
		display[2] |= 1;
	}
	else {
		display[1] &= ~1;
		display[2] &= ~1;
	}
}

// Shows specified time, seconds only
void show_sec(uint8_t s, uint8_t d)
{
	display[0] &= 1;
	display[1] &= 1;
	show_digit(2, s / 10);
	show_digit(3, s % 10);
	if (!d)
	{
		display[1] |= 1;
		display[2] |= 1;
	}
	else {
		display[1] &= ~1;
		display[2] &= ~1;
	}
}

// Shows current time
void show_time()
{
	if (hour < 24) // Часы установлены? Показываем время
	{
		show_hour_min(hour, min, half_sec);
	}
	else { // Часы не установлены, показываем какую-нибудь фигню, но неправильное время нам не нужно
		uint8_t i;
		for (i = 0; i < 4; i++)
			display[i] = 1 << (((millis / 50 + i) % 6) + 2);
	}
}

// Shows time left
void show_time_left()
{
	if (delayed_secs >= 60)
		show_hour_min(delayed_secs / 3600, (delayed_secs / 60) % 60, half_sec);
	else
		show_sec(delayed_secs, half_sec);
}

// Shows time passed
void show_time_passed()
{
	show_hour_min(passed_secs / 3600, (passed_secs / 60) % 60, half_sec);
}

// Must be called from timer interrupt, every 2ms
void update_display()
{
	if ((display_mode == DISPLAY_TIME) && ((millis % 50) == 0)) show_time();
	if ((display_mode == DISPLAY_TIME_LEFT) && ((millis % 50) == 0)) show_time_left();
	if ((display_mode == DISPLAY_TIME_PASSED) && ((millis % 50) == 0)) show_time_passed();
	if ((display_mode == DISPLAY_TEMPERATURE) && ((millis % 50) == 0)) show_temperature();
	PORTB = (PORTB & ~0x1F) | (1 << display_col);
	PORTC = ~display[display_col];
	display_col = (display_col + 1) % 5;
}
