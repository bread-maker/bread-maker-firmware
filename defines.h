#ifndef _DEFINES_H_
#define _DEFINES_H_

#define F_CPU 8000000UL
#define UART_BAUD 38400UL

// Hardware defines
#define MOTOR_ON set_bit(PORTD, 5)
#define MOTOR_OFF unset_bit(PORTD, 5)
#define HEATER_ON set_bit(PORTD, 6)
#define HEATER_OFF unset_bit(PORTD, 6)
#define HEATER_IS_ON ((PORTD>>6)&1)
#define TEMP_PULLUP_ON set_bit(PORTD, 3)
#define TEMP_PULLUP_OFF unset_bit(PORTD, 3)
#define TEMP_PULLUP_IS_ON ((PORTD>>3)&1)
#define CLOCK ((PINB>>7)&1)

// Temperature control
#define K1 0.95
#define K2 (1.0-K1)
#define PID_FUNCTIONAL_RANGE 15
#define BANG_MAX 0xE0
#define PID_Kp 48.512
#define PID_Ki 0.210
#define PID_Kd 2794.069
#define MAX_TEMP 135
//#define ENABLE_AUTOTUNE

// Motor control
// Impulse mode
#define MOTOR_PULSE_ON_TIME 200
#define MOTOR_PULSE_OFF_TIME 1000

#define CRUST_DISPLAY_TIMEOUT 3000
#define PROGRAM_SELECT_TIMEOUT 30000
#define PROGRAM_COUNT 7
#define CRUST_COUNT 3
#define MAX_DELAY_HOURS 15
#define BUTTON_REPEAT_AFTER 250
#define BUTTON_REPEAT_INTERVAL 10

#define ERROR_RESET_COLD_START 1
#define ERROR_RESET_BROWN_OUT 1
#define ERROR_RESET_WATCHDOG 2
#define ERROR_RESET_EXTERNAL 3
#define ERROR_NO_TICK 4
#define ERROR_NO_PROGRAM 5
#define ERROR_NO_THERMISTOR 6
#define ERROR_TOO_HOT 7

#define EEPROM_ADDR_WAS_IDLE 0

#endif
