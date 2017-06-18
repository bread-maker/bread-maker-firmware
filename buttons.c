#include "defines.h"
#include <avr/io.h>
#include <util/delay.h>
#include "breadmaker.h"
#include "bits.h"
#include "buttons.h"
#include "display.h"

// Read buttons states
uint8_t read_buttons()
{
	uint8_t result = 0;
	unset_bit(DDRA, 4);
	set_bit(DDRA, 3); // Buttons column 1 -> ground
	_delay_us(10);
	result |= (PINA & 7);
	unset_bit(DDRA, 3);
	set_bit(DDRA, 4); // Buttons column 2 -> ground
	_delay_us(10);
	result |= (PINA & 7) << 3;
	unset_bit(DDRA, 4);
	return ~result & 0x3F;
}

// Checks for cancel button
void check_cancel_hold(uint8_t btn)
{
	static uint16_t hold_time = 0;
	static uint32_t last_check = 0;
	if (last_check != millis / 100) // Every 100ms
	{
		if ((btn != 0xff ? btn : read_buttons()) == BTN_STOP)
		{
			hold_time++;
			if (hold_time > 15) cmd_abort = 1;
		} else hold_time = 0;
		last_check = millis / 100;
	}
}

// Switched display mode if need
void switch_display_mode(uint8_t btn)
{
	if (btn == 0xff) btn = read_buttons();
	switch (btn)
	{
		case BTN_PLUS:
			display_mode = DISPLAY_TIME_PASSED;
			show_time_passed();
			break;
		case BTN_MINUS:
			display_mode = DISPLAY_TIME_LEFT;
			show_time_left();
			break;
		case BTN_CRUST:
			display_mode = DISPLAY_TEMPERATURE;
			show_temperature();
			break;
		case BTN_MENU:
			display_mode = DISPLAY_TIME;
			show_time();
			break;
	}
}
