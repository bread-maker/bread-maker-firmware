#ifndef _USART_H
#define _USART_H

#include <avr/io.h>
#include <avr/pgmspace.h>

void USART_init(void);
void tx_byte(unsigned char data);
void tx_str(char* str);
void tx_str_P(void* str);
void tx_d(uint32_t d);
void tx_f(float f);

#define tx_str_C(str) tx_str_P((char*)PSTR(str));

#endif
