// Partically based on temperature.cpp from Marlin project: https://github.com/MarlinFirmware/Marlin/

#include "defines.h"
#include "bits.h"
#include "temptable.h"
#include "temperature.h"
#include "breadmaker.h"
#include "usart.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/pgmspace.h>

volatile uint8_t soft_pwm = 0x00;
volatile uint8_t soft_pwm_timer = 0;
uint16_t temp_raw = 0;
uint32_t temp_res = 0;
float target_temperature = 0.0;
float current_temperature = 0.0;
static float pid_error = 0.0;
static bool pid_reset = 0;
static float temp_iState = 0.0;
static float temp_dState = 0.0;
static float iTerm = 0.0;
static float dTerm = 0.0;
static float pTerm = 0;

// Software very slow PWM
void update_pwm()
{
	soft_pwm_timer++;
	if (soft_pwm > soft_pwm_timer || soft_pwm == PID_MAX)
	{
		HEATER_ON;
	}
	else {
		HEATER_OFF;
	}
}

// Reads ADC
uint16_t get_raw_temp()
{
	unset_bit(ADCSRA, ADIF);
	set_bit(ADCSRA, ADSC);
	while (!(ADCSRA & (1 << ADIF)));
	return ADC;
}

// Reads and calculates current temperature
float get_temp()
{
	temp_raw = get_raw_temp();
	if (!TEMP_PULLUP_IS_ON)
		temp_res = (0x3FF - temp_raw) * 105000UL / temp_raw;
	else
		temp_res = (0x3FF - temp_raw) * 793UL / temp_raw;

	uint8_t tt = 0;
	float t;
	if (temp_res > temp_table[0])
	{
		t = 20.0;
	}
	else {
		while (temp_table[tt + 1] > temp_res) tt++;
		t = 20.0 + tt * 5.0 + (temp_table[tt] - temp_res) * 5.0 / (temp_table[tt] - temp_table[tt + 1]);
	}

	if (t >= 90)
		TEMP_PULLUP_ON;
	else if (t <= 80)
		TEMP_PULLUP_OFF;

	return t;
}

float constrain(float v, float min, float max)
{
	if (v < min) return min;
	if (v > max) return max;
	return v;
}

#ifdef ENABLE_AUTOTUNE
void PID_autotune(float temp, int ncycles)
{
	float input = 0.0;
	int cycles = 0;
	bool heating = true;

	uint32_t temp_ms = millis, t1 = temp_ms, t2 = temp_ms;
	long t_high = 0, t_low = 0;

	long bias, d;
	float max = 0, min = 10000;

	//printf("PID Autotune\r\n");

	soft_pwm = 0;

	soft_pwm = bias = d = BANG_MAX;

	current_temperature = get_temp();

	// PID Tuning loop
	while (1) {
		uint32_t ms = millis;

		_delay_ms(1000);
		current_temperature = get_temp();
		if (1) { // temp sample ready
			input = get_temp();

			NOLESS(max, input);
			NOMORE(min, input);

			if (heating && input > temp) {
				if (ELAPSED(ms, t2 + 5000UL)) {
					heating = false;
					soft_pwm = (bias - d) >> 1;
					t1 = ms;
					t_high = t1 - t2;
					max = temp;
				}
			}

			if (!heating && input < temp) {
				if (ELAPSED(ms, t1 + 5000UL)) {
					heating = true;
					t2 = ms;
					t_low = t2 - t1;
					if (cycles > 0) {
						long max_pow = PID_MAX;
						bias += (d * (t_high - t_low)) / (t_low + t_high);
						bias = constrain(bias, 20, max_pow - 20);
						d = (bias > max_pow / 2) ? max_pow - 1 - bias : bias;

						tx_str_C("STATS ");
						tx_str_C("\"cycle\": "); tx_d(cycles);
						tx_str_C(", \"bias\": "); tx_d(bias);
						tx_str_C(", \"d\": "); tx_d(d);
						tx_str_C(", \"min\": "); tx_f(min);
						tx_str_C(", \"max\": "); tx_f(max);
						tx_byte('\n');
					}
					soft_pwm = (bias + d) >> 1;
					cycles++;
					min = temp;
				}
			}
		}
		// Every 2 seconds...PID_MAX
		if (ELAPSED(ms, temp_ms + 2000UL)) {
			tx_str_C("STATS ");
			tx_str_C(" \"T\": "); tx_f(input);
			tx_str_C(", \"PWM\": "); tx_d(soft_pwm);
			tx_byte('\n');
			temp_ms = ms;
		} // every 2 seconds
		if (cycles > ncycles) {
			float Ku, Tu, workKp, workKi, workKd;
			Ku = (4.0 * d) / (M_PI * (max - min) * 0.5);
			Tu = ((float)(t_low + t_high) * 0.001);
			tx_str_C("STATS ");
			tx_str_C("\"Ku\": "); tx_f(Ku);
			tx_str_C(", \"Tu\": "); tx_f(Tu);
			workKp = 0.6 * Ku;
			workKi = 2 * workKp / Tu;
			workKd = workKp * Tu * 0.125;
			//printf(" Clasic PID \r\n");
			tx_str_C(", \"Kp\": "); tx_f(workKp);
			tx_str_C(", \"Ki\": "); tx_f(workKi);
			tx_str_C(", \"Kd\": "); tx_f(workKd);
			/*
			workKp = 0.33*Ku;
			workKi = workKp / Tu;
			workKd = workKp*Tu / 3;
			printf(" Some overshoot \r\n");
			printf("Kp = %d.%03ld\r\n", FL(workKp));
			printf("Ki = %d.%03ld\r\n", FL(workKi));
			printf("Kd = %d.%03ld\r\n", FL(workKd));
			workKp = 0.2*Ku;
			workKi = 2 * workKp / Tu;
			workKd = workKp*Tu / 3;
			printf(" No overshoot \r\n");
			printf("Kp = %d.%03ld\r\n", FL(workKp));
			printf("Ki = %d.%03ld\r\n", FL(workKi));
			printf("Kd = %d.%03ld\r\n", FL(workKd));
			*/
			tx_byte('\n');
			soft_pwm = 0;
			return;
		}
		do_stuff();
	}
}
#endif

// All heater stuff, must be called every second
void manage_heater()
{
	current_temperature = get_temp();

	float pid_output;
	pid_error = target_temperature - current_temperature;
	dTerm = K2 * PID_Kd * (current_temperature - temp_dState) + K1 * dTerm;
	temp_dState = current_temperature;
	if (pid_error > PID_FUNCTIONAL_RANGE /*|| ((pid_error > 0) && (target_temperature > PID_FUNCTIONAL_MAX_TEMP))*/) {
		pid_output = BANG_MAX;
		pid_reset = true;
	}
/*
	else if (pid_error >= -5 && target_temperature > PID_FUNCTIONAL_MAX_TEMP)
	{
		pid_output = BANG_HI;
		pid_reset = true;
	}
*/
	else if (pid_error < -(PID_FUNCTIONAL_RANGE) || target_temperature == 0) {
		pid_output = 0;
		pid_reset = true;
	}
	else {
		if (pid_reset) {
			temp_iState = 0.0;
			pid_reset = false;
		}
		pTerm = PID_Kp * pid_error;
		temp_iState += pid_error;
		iTerm = PID_Ki * temp_iState;

		pid_output = pTerm + iTerm - dTerm;

		if (pid_output > PID_MAX) {
			if (pid_error > 0) temp_iState -= pid_error; // conditional un-integration
			pid_output = PID_MAX;
		}
		else if (pid_output < 0) {
			if (pid_error < 0) temp_iState -= pid_error; // conditional un-integration
			pid_output = 0;
		}
		pid_output = (int)(pid_output * BANG_MAX / PID_MAX);
	}

	soft_pwm = current_temperature < MAX_TEMP ? pid_output : 0;
	if (soft_pwm > BANG_MAX) soft_pwm = BANG_MAX;
}
