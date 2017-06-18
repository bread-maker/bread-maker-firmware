#include "defines.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "usart.h"
#include "breadmaker.h"
#include "transmitter.h"
#include "temperature.h"

const char PROGMEM STR_STATE[] = "\"state\":\"";
const char PROGMEM STR_MOTOR_OFF[] = "off";
const char PROGMEM STR_MOTOR_ONOFF[] = "onoff";
const char PROGMEM STR_MOTOR_ON[] = "on";
const char* motor_state_names[] = { STR_MOTOR_OFF, STR_MOTOR_ONOFF, STR_MOTOR_ON };
const char PROGMEM STR_FALSE[] = "false";
const char PROGMEM STR_TRUE[] = "true";
const char* bool_names[] = { STR_FALSE, STR_TRUE };

// Sends current baking program
void send_program()
{
	tx_str_C("PROGR {\"program_id\":");
	tx_d(program_number);
	tx_str_C(",\"crust_id\":");
	tx_d(crust_number);
	tx_str_C(",\"max_temp_a\":");
	tx_d(max_temperature_before_timer);
	tx_str_C(",\"max_temp_b\":");
	tx_d(max_temperature_before_baking);
	tx_str_C(",\"stages\":[");
	uint8_t i;
	for (i = 0; i < baking_stage_count; i++)
	{
		struct baking_stage* cmd = (void*)&baking_program[i];
		if (i != 0) tx_byte(',');
		tx_str_C("{\"temp\":");
		tx_d(cmd->temperature);
		tx_str_C(",\"motor\":\"");
		tx_str_P((char*)motor_state_names[cmd->motor]);
		tx_str_C("\",\"duration\":");
		tx_d(cmd->duration);
		tx_str_C("}");
	}
	tx_str_C("],\"beeps\":[");
	for (i = 0; i < baking_beeps_count; i++)
	{
		struct baking_beep* b = (void*)&baking_beeps[i];
		if (i != 0) tx_byte(',');
		tx_str_C("{\"stage\":");
		tx_d(b->stage);
		tx_str_C(",\"time\":");
		tx_d(b->time);
		tx_str_C(",\"count\":");
		tx_d(b->count);
		tx_str_C("}");
	}
	tx_str_C("],\"warm_temp\":");
	tx_d(warming_temperature);
	tx_str_C(",\"max_warm_time\":");
	tx_d(warming_max_time);
	tx_str_C("}\n");
}

// Sends current stats
void send_stats()
{
	//printf("TIMER %02d:%02d:%02d\n", hour, min, sec);
	if (hour > 24 || ((seconds % 360) == 0))
		tx_str_C("TIME?\n");
	tx_str_C("STATS ");
	switch (current_state)
	{
	case STATE_IDLE:
		tx_str_P((char*)STR_STATE);
		tx_str_C("idle\"");
		break;
	case STATE_TIMER:
		tx_str_P((char*)STR_STATE);
		tx_str_C("timer\",\"passed\":");
		tx_d(passed_secs);
		tx_str_C(",\"left\":");
		tx_d(delayed_secs);
		break;
	case STATE_BAKING:
		tx_str_P((char*)STR_STATE);
		tx_str_C("baking\",\"program_id\":"); tx_d(program_number);
		tx_str_C(",\"crust_id\":"); tx_d(crust_number);
		tx_str_C(",\"stage\":"); tx_d(baking_current_stage);
		tx_str_C(",\"stage_time\":"); tx_d(current_stage_time);
		tx_str_C(",\"passed\":"); tx_d(passed_secs);
		tx_str_C(",\"left\":"); tx_d(delayed_secs);
		break;
	case STATE_WARMING:
		tx_str_P((char*)STR_STATE);
		tx_str_C("warming\",\"passed\":");
		tx_d(passed_secs);
		tx_str_C(",\"left\":");
		tx_d(delayed_secs);
		break;
	case STATE_ERROR:
		tx_str_P((char*)STR_STATE);
		tx_str_C("error\",\"error_code\":");
		tx_d(last_error);
		break;
	}
	tx_str_C(",\"target_temp\":"); tx_d(target_temperature);
	tx_str_C(",\"temp\":"); tx_f(current_temperature);
	tx_str_C(",\"motor\":\""); tx_str_P((char*)motor_state_names[motor_state]);
	tx_str_C("\",\"pullup\":"); tx_str_P((char*)bool_names[!!TEMP_PULLUP_IS_ON]);
	tx_str_C(",\"adc\":"); tx_d(temp_raw);
	tx_str_C(",\"res\":"); tx_d(temp_res);
	tx_str_C(",\"pwm\":"); tx_d(soft_pwm);
	tx_str_C(",\"heat\":"); tx_str_P((char*)bool_names[!!HEATER_IS_ON]);
	tx_byte('\n');
}
