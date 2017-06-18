#ifndef _BUTTONS_H_
#define _BUTTONS_H_

// Button codes
#define BTN_MENU 0x01
#define BTN_START 0x02
#define BTN_STOP 0x04
#define BTN_CRUST 0x08
#define BTN_PLUS 0x10
#define BTN_MINUS 0x20

uint8_t read_buttons();
void check_cancel_hold(uint8_t btn);
void switch_display_mode(uint8_t btn);

#endif
