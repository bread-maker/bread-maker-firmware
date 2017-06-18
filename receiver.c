#include "defines.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "breadmaker.h"
#include "usart.h"

volatile char command_buffer[16];
volatile uint8_t command_buffer_pos = 0;

void add_stage(uint8_t temperature, uint8_t motor, uint16_t duration)
{
	if (motor > 2) motor = 0;
	if (baking_stage_count >= sizeof(baking_program) / sizeof(struct baking_stage)) return;
	baking_program[baking_stage_count].temperature = temperature;
	baking_program[baking_stage_count].motor = motor;
	baking_program[baking_stage_count].duration = duration;
	baking_stage_count++;
}

void add_beep(uint8_t stage, uint16_t time, uint8_t count)
{
	if (baking_beeps_count >= sizeof(baking_beeps) / sizeof(struct baking_beep)) return;
	baking_beeps[baking_beeps_count].stage = stage;
	baking_beeps[baking_beeps_count].time = time;
	baking_beeps[baking_beeps_count].count = count;
	baking_beeps_count++;
}

ISR(USART_RXC_vect)
{
	char b;
	while (UCSRA & (1 << RXC))
	{
		b = UDR;
		if (b == 10 || b == 13)
		{
			// Parsing command
			uint8_t i, argc = 0;
			uint32_t argv[6] = { 0, 0, 0, 0, 0, 0 };

			for (i = 0; i <= command_buffer_pos && argc < sizeof(argv) / sizeof(uint32_t); i++)
			{
				char ch = command_buffer[i];
				if (i == command_buffer_pos || ch == ' ')
				{
					command_buffer[i] = 0;
					argc++;
				}
				else if (ch >= '0' && ch <= '9')
				{
					argv[argc] *= 10;
					argv[argc] += ch - '0';
				}
			}

			// Setting time
			if ((strcmp_P((char*)command_buffer, PSTR("TIME")) == 0) && (argc >= 4))
			{
				hour = argv[1];
				min = argv[2];
				sec = argv[3];
			}
			// New baking program
			else if (strcmp_P((char*)command_buffer, PSTR("NEW")) == 0)
			{
				if (current_state != STATE_IDLE) cmd_abort = 1;
				baking_stage_count = baking_beeps_count = 0;
			}
			// New baking stage
			else if ((strcmp_P((char*)command_buffer, PSTR("STAGE")) == 0) && (argc >= 4))
			{
				if (current_state != STATE_IDLE) cmd_abort = 1;
				add_stage(argv[1], argv[2], argv[3]);
			}
			// New beep
			else if ((strcmp_P((char*)command_buffer, PSTR("BEEP")) == 0) && (argc >= 4))
			{
				if (current_state != STATE_IDLE) cmd_abort = 1;
				add_beep(argv[1], argv[2], argv[3]);
			}
			// Maximum temperature before timer set
			else if ((strcmp_P((char*)command_buffer, PSTR("MAXTEMPA")) == 0) && (argc >= 2))
			{
				max_temperature_before_timer = argv[1];
			}
			// Maximum temperature before baking
			else if ((strcmp_P((char*)command_buffer, PSTR("MAXTEMPB")) == 0) && (argc >= 2))
			{
				max_temperature_before_baking = argv[1];
			}
			// Warming temperature
			else if ((strcmp_P((char*)command_buffer, PSTR("WARMTEMP")) == 0) && (argc >= 2))
			{
				warming_temperature = argv[1];
			}
			// Maximum warming time
			else if ((strcmp_P((char*)command_buffer, PSTR("WARMTIME")) == 0) && (argc >= 2))
			{
				warming_max_time = argv[1];
			}
			// Baking start
			else if ((strcmp_P((char*)command_buffer, PSTR("RUN")) == 0) && (argc >= 4))
			{
				if (current_state != STATE_IDLE) cmd_abort = 1;
				program_number = argv[1];
				crust_number = argv[2];
				delayed_secs = argv[3];
				cmd_start = 1;
			}
			// Baking aboirt
			else if (strcmp_P((char*)command_buffer, PSTR("ABORT")) == 0)
			{
				if (current_state != STATE_IDLE) cmd_abort = 1;
			}
			// Dismiss error
			else if (strcmp_P((char*)command_buffer, PSTR("NOERR")) == 0)
			{
				cmd_abort_err = 1;
			}
			command_buffer_pos = 0;
		}
		else if (command_buffer_pos < sizeof(command_buffer))
		{
			command_buffer[command_buffer_pos++] = b;
		}
	}
}
