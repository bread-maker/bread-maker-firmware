#include "defines.h"
#include <avr/io.h>
#include <avr/pgmspace.h>

void tx_byte(unsigned char data)
{
	/* Wait for empty transmit buffer */
	while (!(UCSRA & (1 << UDRE)));
	/* Put data into buffer, sends the data */
	UDR = data;
}

void tx_str(char* str)
{
	while (*str) tx_byte(*(str++));
}

void tx_str_P(char* str)
{
	char ch;
	while ((ch = pgm_read_byte(str++)) != 0) tx_byte(ch);
}

void tx_d(uint32_t d)
{
	uint8_t started = 0;
	uint32_t r = 1000000000;
	while (r > 0)
	{
		uint8_t n = (d / r) % 10;
		if (n || r == 1) started = 1;
		if (started)
			tx_byte('0' + n);
		r /= 10;
	}
}

void tx_f(float f)
{
	uint32_t d = (uint32_t)(f * 1000);
	uint8_t started = 0;
	uint32_t r = 1000000000;
	while (r > 0)
	{
		uint8_t n = (d / r) % 10;
		if (n || r == 1000) started = 1;
		if (started)
			tx_byte('0' + n);
		if (r == 1000) tx_byte('.');
		r /= 10;
	}
}

void USART_init(void)
{
	unsigned int bd = (F_CPU / (16UL * UART_BAUD)) - 1;
	UBRRL = bd & 0xFF;
	UBRRH = bd >> 8;

	UCSRB = _BV(TXEN) | _BV(RXEN) | _BV(RXCIE); /* tx/rx enable */
	UCSRC |= _BV(UMSEL);
}
