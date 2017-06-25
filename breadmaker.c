#include "defines.h"
#include "usart.h"
#include "bits.h"
#include "temperature.h"
#include "breadmaker.h"
#include "beeper.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include "transmitter.h"
#include "display.h"
#include "buttons.h"

volatile uint32_t seconds = 0; // Seconds since boot, high precision
volatile uint32_t millis = 0; // Milliseconds since boot, low precision
volatile uint8_t hour = 0xff;
volatile uint8_t min = 0;
volatile uint8_t sec = 0;
volatile uint8_t half_sec = 1;
volatile uint8_t motor_state = MOTOR_STOPPED;
volatile uint8_t current_state = STATE_IDLE;
volatile uint8_t last_error = 0;
volatile struct baking_stage baking_program[32];
volatile struct baking_beep baking_beeps[4];
volatile uint8_t baking_stage_count = 0;
volatile uint8_t baking_current_stage= 0;
volatile uint8_t baking_beeps_count = 0;
volatile uint16_t current_stage_time = 0;
volatile uint32_t last_stuff_time = 0;
volatile uint8_t program_number = 0;
volatile uint8_t crust_number = 0;
volatile uint32_t delayed_secs = 0;
volatile uint32_t passed_secs = 0;
volatile int8_t cmd_start = 0;
volatile int8_t cmd_abort = 0;
volatile int8_t cmd_abort_err = 0;
volatile int8_t warming_temperature = 50;
volatile int16_t warming_max_time = 10800;
volatile uint8_t max_temperature_before_timer = 40;
volatile uint8_t max_temperature_before_baking = 40;

// Must be called at least every second
void do_stuff()
{
	if (seconds - last_stuff_time <= 0) return;
	wdt_reset();
#ifndef ENABLE_AUTOTUNE
	manage_heater();
	send_stats();
#endif
	last_stuff_time = seconds;
}

// Motor control
void update_motor()
{
	if (motor_state == MOTOR_IMPULSE) {
		if ((millis % (MOTOR_PULSE_ON_TIME + MOTOR_PULSE_OFF_TIME)) < MOTOR_PULSE_ON_TIME)
			MOTOR_ON;
		else
			MOTOR_OFF;
	}
	else if (motor_state == MOTOR_RUNNING)
		MOTOR_ON;
	else
		MOTOR_OFF;
}

// Updates time
void update_time()
{
	uint8_t c_now = CLOCK;
	if (half_sec == c_now) return;
	half_sec = c_now;
	if (half_sec) seconds++; // Seconds since boot
	if (hour != 0xff) // Time is not set?
	{
		if (half_sec)
		{
			sec++;
			if (current_state == STATE_TIMER ||
				current_state == STATE_BAKING ||
				current_state == STATE_WARMING)
			{
				if (delayed_secs) delayed_secs--;
				passed_secs++;
			}
		}
		else return;
		if (sec >= 60)
		{
			min++;
			sec = 0;
		}
		if (min >= 60)
		{
			hour++;
			min = 0;
		}
		if (hour >= 24)
			hour = 0;
	}
}

// Executed every ~2 milliseconds
ISR(TIMER0_COMP_vect)
{
	millis += 2;
	if (seconds - last_stuff_time < 10)
	{
		if ((millis % 100) == 0) update_pwm();
	}
	else {
		HEATER_OFF; // EMERGENCY!
		motor_state = MOTOR_STOPPED;
	}
	if ((millis % 100) == 0) update_motor();
	if ((millis % 100) == 0) update_time();
	update_display();
}

// Shows error
void show_error(uint8_t code)
{
	current_state = STATE_ERROR;
	last_error = code;
	motor_state = MOTOR_STOPPED;
	target_temperature = 0;
	MOTOR_OFF;
	HEATER_OFF;
	wdt_reset();
	display_mode = DISPLAY_RAW;
	do_stuff();
	eeprom_write_byte(EEPROM_ADDR_WAS_IDLE, 0xff);
	MCUCSR = 0;
	display[0] = 0x9E;
	display[1] = display[2] = 0x0A;
	show_digit(3, code);
	display[4] = 0;
	beeper_set_freq(1000);
	_delay_ms(200);
	beeper_set_freq(500);
	_delay_ms(200);
	beeper_set_freq(200);
	_delay_ms(500);
	beeper_set_freq(0);
	while (read_buttons()) { _delay_ms(1); wdt_reset(); }
	cmd_abort_err = 0;
	while (!cmd_abort_err)
	{
		uint16_t i;
		for (i = 0; i < 1000 && !cmd_abort_err; i++)
		{
			_delay_ms(1);
			if (read_buttons()) cmd_abort_err = 1;
			wdt_reset();
		}
		send_stats();
	}
	PORTC = 0xFF;
	wdt_enable(WDTO_500MS);
	cli();
	while (1); // reboot
}

// Program selection routine
void select_program()
{
	display_mode = DISPLAY_RAW;
	display[0] = display[1] = display[2] = display[3] = display[4] = 0;
	uint8_t active = 0;
	int8_t delayed_hours = 0;
	int8_t delayed_mins = 0;
	uint8_t show_time = 0;
	uint8_t btn;
	uint8_t holding = 0;
	uint32_t timeout = PROGRAM_SELECT_TIMEOUT;
	while (1)
	{
		if (cmd_start) return;
		if (cmd_abort) return;
		btn = read_buttons();
		if (active)
		{
			if (btn == BTN_STOP)
				return;
			if (btn == BTN_MENU)
			{
				program_number++;
				show_time = 0;
			}
			if (program_number >= PROGRAM_COUNT)
				program_number = 0;
			if (btn == BTN_CRUST)
			{
				crust_number++;
				show_time = 0;
			}
			if (crust_number >= CRUST_COUNT)
				crust_number = 0;
			if (btn == BTN_PLUS)
			{
				delayed_mins += 1;
				if (delayed_mins >= 60)
				{
					delayed_hours++;
					delayed_mins = 0;
				}
				if (delayed_hours >= MAX_DELAY_HOURS)
				{
					delayed_hours = MAX_DELAY_HOURS - 1;
					delayed_mins = 59;
				}
				show_time = 1;
			}
			if (btn == BTN_MINUS)
			{
				delayed_mins -= 1;
				if (delayed_mins < 0)
				{
					delayed_hours--;
					delayed_mins = 59;
				}
				if (delayed_hours < 0)
				{
					delayed_hours = delayed_mins = 0;
				}
				show_time = 1;
			}
			if (btn == BTN_START)
				break;
		} // Active

		// Shows time or mode
		if (!show_time)
		{
			display[0] = 0;
			show_digit(1, program_number + 1);
			show_digit(2, crust_number + 10);
			display[3] = 0;
		}
		else {
			show_digit(0, delayed_hours / 10);
			show_digit(1, delayed_hours % 10);
			show_digit(2, delayed_mins / 10);
			show_digit(3, delayed_mins % 10);
		}

		display[1] |= 1;
		display[2] |= 1;
		display[4] = 1 << program_number;

		if (btn)
		{
			uint16_t t = 0;
			timeout = PROGRAM_SELECT_TIMEOUT;
			while ((btn = read_buttons()) != 0)
			{
				if ((btn == BTN_MINUS || btn == BTN_PLUS) && (t >= BUTTON_REPEAT_INTERVAL) && holding)
					break;
				if ((btn == BTN_MINUS || btn == BTN_PLUS) && (t >= BUTTON_REPEAT_AFTER))
				{
					holding = 1;
					break;
				}
				do_stuff();
				_delay_ms(1);
				t++;
			}
		}
		else {
			holding = 0;
			if (!--timeout) return;
		}
		active = 1;
	 	do_stuff();
		_delay_ms(1);
	}

	baking_stage_count = 0;
	baking_current_stage= 0;
	cmd_start = 0;
	delayed_secs = delayed_hours * 3600 + delayed_mins * 60;

	tx_str_C("SELCT ");
	tx_d(program_number);
	tx_byte(' ');
	tx_d(crust_number);
	tx_byte(' ');
	tx_d(delayed_secs);
	tx_str_C("\n");
	display[0] = display[1] = display[2] = display[3] = 0;

	timeout = 200;
	while (!cmd_start)
	{
		if (cmd_abort) return;
		_delay_ms(25);
		wdt_reset();
		if (!--timeout)
			show_error(ERROR_NO_PROGRAM);
	}
}

// Baking
void baking()
{
	uint8_t i;
	uint32_t total_time = 0;
	baking_current_stage = 0;
	if (temp_res >= 150000) show_error(ERROR_NO_THERMISTOR);
	if (current_temperature > max_temperature_before_timer) show_error(ERROR_TOO_HOT);
	display_mode = DISPLAY_RAW;
	display[0] = display[1] = display[2] = display[3] = 0;
	beeper_set_freq(1000); // Long beep after program start
	wdt_reset();
	_delay_ms(500);
	beeper_set_freq(0);	
	send_program();
	if (delayed_secs)
	{
		current_state = STATE_TIMER;
		display_mode = DISPLAY_TIME_LEFT;
		passed_secs = 0;
		send_stats();
		while (delayed_secs)
		{
			if (cmd_abort) return;
			int8_t btn = read_buttons();
			switch_display_mode(btn);
			check_cancel_hold(btn);
			display[4] = half_sec ? 0 : (1 << program_number);
			do_stuff();
			_delay_ms(1);
		}
		beeper_set_freq(1000); // Short beep after timer
		wdt_reset();
		_delay_ms(200);
		beeper_set_freq(0);	
	}
	display[4] = 1 << program_number;
	for (i = 0; i < baking_stage_count; i++)
		total_time += baking_program[i].duration;
	delayed_secs = total_time;
	passed_secs = 0;
	current_state = STATE_BAKING;
	display_mode = DISPLAY_TIME_LEFT;
	send_stats();
	if (current_temperature > max_temperature_before_baking) show_error(ERROR_TOO_HOT);	
	while (delayed_secs)
	{
		if (cmd_abort) return;
		uint32_t time_passed = total_time - delayed_secs;
		for (i = 0; (i < baking_stage_count) && (time_passed >= baking_program[i].duration); i++)
		{
			time_passed -= baking_program[i].duration;
		}
		if (i < baking_stage_count)
		{
			baking_current_stage= i;
			current_stage_time = time_passed;
			target_temperature = baking_program[i].temperature;
			motor_state = baking_program[i].motor;
			// Beeps
			for (i = 0; i < baking_beeps_count; i++)
			{
				struct baking_beep* b = (void*)&baking_beeps[i];
				if ((baking_current_stage == b->stage)
					&& (current_stage_time == b->time))
				{
					b->stage = 0xff;
					uint8_t j, n;
					for (n = 0; n < b->count; n++)
					{
					beeper_set_freq(500);						
					for (j = 0; j < 150; j++)
					{
						wdt_reset();
						_delay_ms(4);
					}
					beeper_set_freq(0);
					for (j = 0; j < 150; j++)
					{
						wdt_reset();
						_delay_ms(2);
					}
					}
				}
			}
		}
		int8_t btn = read_buttons();
		switch_display_mode(btn);
		check_cancel_hold(btn);
		do_stuff();
		_delay_ms(1);
	}
	//display_mode = DISPLAY_TIME;
	//display_mode = DISPLAY_TIME_PASSED;
	display_mode = DISPLAY_TEMPERATURE;
	target_temperature = warming_temperature;
	motor_state = MOTOR_STOPPED;
	delayed_secs = warming_max_time;
	passed_secs = 0;
	current_state = STATE_WARMING;
	tx_str_C("BAKED\n");
	play_melody();
	while (delayed_secs)
	{
		int8_t btn = read_buttons();
		switch_display_mode(btn);
		if (cmd_abort) return;
		if (btn == BTN_STOP)
		{
			beeper_set_freq(1000);
			_delay_ms(100);
			beeper_set_freq(0);
			return;
		}
		do_stuff();
		_delay_ms(1);
	}
}

int main(void)
{
	wdt_enable(WDTO_2S);
	sei();
	USART_init();
	//set_bit(PORTD, 2); unset_bit(DDRD, 2); // Phase in
	unset_bit(PORTD, 3); set_bit(DDRD, 3); // Thermistor pull-up
	unset_bit(PORTD, 4); set_bit(DDRD, 4); // Buzzer
	unset_bit(PORTD, 5); set_bit(DDRD, 5); // Motor
	unset_bit(PORTD, 6); set_bit(DDRD, 6); // Heater
	unset_bit(PORTA, 7); unset_bit(DDRA, 7); // Thermistor (ADC7)
 
	set_bit(PORTA, 0); unset_bit(DDRA, 0); // Buttons row 1
	set_bit(PORTA, 1); unset_bit(DDRA, 1); // Buttons row 2
	set_bit(PORTA, 2); unset_bit(DDRA, 2); // Buttons row 3
	unset_bit(PORTA, 3); unset_bit(DDRA, 3); // Buttons column 1
	unset_bit(PORTA, 4); unset_bit(DDRA, 4); // Buttons column 2

	ADMUX = (1 << REFS0) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0); // ref. AVCC, ADC7
	set_bit(ADCSRA, ADEN);

	DDRC |= 0xFF; // Leds rows
	DDRB = 0x1F; // Leds cols => output

	unset_bit(DDRB, 7); set_bit(PORTB, 7); // Clock input
	//DDRB |= 0xE0; // sck, mosi, miso => output
	//PORTB &= ~0xE0; // sck, mosi, miso => low

	TCCR0 = (1 << CS00) | (1 << CS02) | (1 << WGM01); // Enable Timer0, prescalex /1024, CTC
	OCR0 = 15; // Every ~2ms
	TIMSK |= (1 << OCIE0); //	Timer/Counter0 Compare-Match Interrupt Enable

	tx_str_C("BOOT0\n");
	uint16_t timeout = 2000;
	uint8_t p = 0;
	display_mode = DISPLAY_RAW;
	while (seconds < 5)
	{
		wdt_reset();
		display[p] = 0;
		p = (p + 1) % 4;
		display[p] = 2;
		_delay_ms(50);
		if (!--timeout)
			show_error(ERROR_NO_TICK);
	}
	tx_str_C("BOOT1\n");
	
	if (!eeprom_read_byte(EEPROM_ADDR_WAS_IDLE))
	{
		if (MCUCSR & (1<<WDRF))
			show_error(ERROR_RESET_WATCHDOG);
		else if (MCUCSR & (1<<BORF))
			show_error(ERROR_RESET_BROWN_OUT);
		else if (MCUCSR & (1<<EXTRF))
			show_error(ERROR_RESET_EXTERNAL);
		else if (MCUCSR & (1<<PORF))
			show_error(ERROR_RESET_COLD_START);
	}
	
	timeout = 25;
	while (read_buttons() == BTN_STOP)
	{
		display[0] = display[1] = display[2] = display[3] = 2;
		beeper_set_freq(500);
		_delay_ms(100);		
		display[0] = display[1] = display[2] = display[3] = 0;
		beeper_set_freq(100);
		_delay_ms(100);
		wdt_reset();
		if (!--timeout)
		{
			beeper_set_freq(0);
			wdt_reset();
			_delay_ms(1000);
			beeper_set_freq(1000);
			wdt_reset();
			_delay_ms(1000);
			beeper_set_freq(0);
			tx_str_C("RESET\n");
			break;
		}
	}

#ifdef ENABLE_AUTOTUNE
	display_mode = DISPLAY_TEMPERATURE;
	eeprom_write_byte(EEPROM_ADDR_WAS_IDLE, 0);
	PID_autotune(70, 8);
	eeprom_write_byte(EEPROM_ADDR_WAS_IDLE, 0xff);
	while (1) wdt_reset();
#endif

	uint8_t btn = 0;
	while (1)
	{
		if (btn) while (read_buttons())
		{
			do_stuff();
			_delay_ms(1);
		}
		btn = read_buttons();
		if (btn && btn != BTN_STOP) select_program();
		if (cmd_start)
		{
			cmd_abort = 0;
			cmd_start = 0;
			eeprom_write_byte(EEPROM_ADDR_WAS_IDLE, 0);
			baking();
			eeprom_write_byte(EEPROM_ADDR_WAS_IDLE, 0xff);
		}
		target_temperature = 0;
		motor_state = MOTOR_STOPPED;
		if (cmd_abort)
		{
			cmd_abort = 0;
			wdt_reset();
			beeper_set_freq(1000);
			_delay_ms(200);
			beeper_set_freq(200);
			_delay_ms(500);
			beeper_set_freq(0);
		}
		current_state = STATE_IDLE;
		display_mode = DISPLAY_TIME;
		display[4] = 0;
		do_stuff();
		_delay_ms(1);
	}
	return 0;
}

