#include "defines.h"
#include "breadmaker.h"
#include "beeper.h"
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>

// Sets PWM output frequency
void beeper_set_freq(int v)
{
	if (v > 0)
	{
		OCR1A = 500000UL / v - 1;
		TCCR1A = (1 << COM1B0);
		TCCR1B = (1 << WGM12) | (1 << CS11);
		TCNT1 = 0;
	}
	else {
		TCCR1A = TCCR1B = 0;
		TCNT0 = OCR1A = 0;
	}
}

// Notes
const uint16_t PROGMEM note_freqs[] = {
	0,
	NOTE_FREQ_B0,
	NOTE_FREQ_C1,
	NOTE_FREQ_C1S,
	NOTE_FREQ_D1,
	NOTE_FREQ_D1S,
	NOTE_FREQ_E1,
	NOTE_FREQ_F1,
	NOTE_FREQ_F1S,
	NOTE_FREQ_G1,
	NOTE_FREQ_G1S,
	NOTE_FREQ_A1,
	NOTE_FREQ_A1S,
	NOTE_FREQ_B1,
	NOTE_FREQ_C2,
	NOTE_FREQ_C2S,
	NOTE_FREQ_D2,
	NOTE_FREQ_D2S,
	NOTE_FREQ_E2,
	NOTE_FREQ_F2,
	NOTE_FREQ_F2S,
	NOTE_FREQ_G2,
	NOTE_FREQ_G2S,
	NOTE_FREQ_A2,
	NOTE_FREQ_A2S,
	NOTE_FREQ_B2,
	NOTE_FREQ_C3,
	NOTE_FREQ_C3S,
	NOTE_FREQ_D3,
	NOTE_FREQ_D3S,
	NOTE_FREQ_E3,
	NOTE_FREQ_F3,
	NOTE_FREQ_F3S,
	NOTE_FREQ_G3,
	NOTE_FREQ_G3S,
	NOTE_FREQ_A3,
	NOTE_FREQ_A3S,
	NOTE_FREQ_B3,
	NOTE_FREQ_C4,
	NOTE_FREQ_C4S,
	NOTE_FREQ_D4,
	NOTE_FREQ_D4S,
	NOTE_FREQ_E4,
	NOTE_FREQ_F4,
	NOTE_FREQ_F4S,
	NOTE_FREQ_G4,
	NOTE_FREQ_G4S,
	NOTE_FREQ_A4,
	NOTE_FREQ_A4S,
	NOTE_FREQ_B4,
	NOTE_FREQ_C5,
	NOTE_FREQ_C5S,
	NOTE_FREQ_D5,
	NOTE_FREQ_D5S,
	NOTE_FREQ_E5,
	NOTE_FREQ_F5,
	NOTE_FREQ_F5S,
	NOTE_FREQ_G5,
	NOTE_FREQ_G5S,
	NOTE_FREQ_A5,
	NOTE_FREQ_A5S,
	NOTE_FREQ_B5,
	NOTE_FREQ_C6,
	NOTE_FREQ_C6S,
	NOTE_FREQ_D6,
	NOTE_FREQ_D6S,
	NOTE_FREQ_E6,
	NOTE_FREQ_F6,
	NOTE_FREQ_F6S,
	NOTE_FREQ_G6,
	NOTE_FREQ_G6S,
	NOTE_FREQ_A6,
	NOTE_FREQ_A6S,
	NOTE_FREQ_B6,
	NOTE_FREQ_C7,
	NOTE_FREQ_C7S,
	NOTE_FREQ_D7,
	NOTE_FREQ_D7S,
	NOTE_FREQ_E7,
	NOTE_FREQ_F7,
	NOTE_FREQ_F7S,
	NOTE_FREQ_G7,
	NOTE_FREQ_G7S,
	NOTE_FREQ_A7,
	NOTE_FREQ_A7S,
	NOTE_FREQ_B7,
	NOTE_FREQ_C8,
	NOTE_FREQ_C8S,
	NOTE_FREQ_D8,
	NOTE_FREQ_D8S
};
 

// Mario main theme melody
const uint8_t PROGMEM melody[] = {
	NOTE_E5, NOTE_E5, 0, NOTE_E5,
	0, NOTE_C5, NOTE_E5, 0,
	NOTE_G5, 0, 0,	0,
	NOTE_G4, 0, 0, 0,
 
	NOTE_C5, 0, 0, NOTE_G4,
	0, 0, NOTE_E4, 0,
	0, NOTE_A4, 0, NOTE_B4,
	0, NOTE_A4S, NOTE_A4, 0,
 
	NOTE_G4, NOTE_E5, NOTE_G5,
	NOTE_A5, 0, NOTE_F5, NOTE_G5,
	0, NOTE_E5, 0, NOTE_C5,
	NOTE_D5, NOTE_B4, 0, 0,
 
	NOTE_C5, 0, 0, NOTE_G4,
	0, 0, NOTE_E4, 0,
	0, NOTE_A4, 0, NOTE_B4,
	0, NOTE_A4S, NOTE_A4, 0,
 
	NOTE_G4, NOTE_E5, NOTE_G5,
	NOTE_A5, 0, NOTE_F5, NOTE_G5,
	0, NOTE_E5, 0, NOTE_C5,
	NOTE_D5, NOTE_B4, 0, 0
};

// Mario main them tempo
const uint8_t PROGMEM tempo[] = {
	12, 12, 12, 12,
	12, 12, 12, 12,
	12, 12, 12, 12,
	12, 12, 12, 12,
 
	12, 12, 12, 12,
	12, 12, 12, 12,
	12, 12, 12, 12,
	12, 12, 12, 12,
 
	9, 9, 9,
	12, 12, 12, 12,
	12, 12, 12, 12,
	12, 12, 12, 12,
 
	12, 12, 12, 12,
	12, 12, 12, 12,
	12, 12, 12, 12,
	12, 12, 12, 12,
 
	9, 9, 9,
	12, 12, 12, 12,
	12, 12, 12, 12,
	12, 12, 12, 12,
};

static uint8_t delay(int t)
{
	while (t--)
	{
		_delay_ms(1);
		do_stuff();
		if (read_buttons()) return 1;
	}
	return 0;
}

void play_melody()
{
	int note;
	int size = sizeof(melody) / sizeof(uint8_t);
	for (note = 0; note < size; note++) { 
		// to calculate the note duration, take one second
		// divided by the note type.
		//e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
		int noteDuration = 600 / pgm_read_byte(&tempo[note]);
		beeper_set_freq(pgm_read_word(&note_freqs[pgm_read_byte(&melody[note])]));
		if (delay(noteDuration)) break; 
		beeper_set_freq(0);
		// to distinguish the notes, set a minimum time between them.
		// the note's duration + 30% seems to work well:
		int pauseBetweenNotes = noteDuration * 1.30;
		if (delay(pauseBetweenNotes)) break;
		//if (delay(noteDuration)) break; 
	}
	beeper_set_freq(0);
}
