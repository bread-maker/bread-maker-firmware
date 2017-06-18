#ifndef _TEMPERATURE_H
#define _TEMPERATURE_H

#define PID_MAX 0xFF

#define M_PI 3.14159265358979323846
#define FL(n) (int)n,((long int)(n*1000L) % 1000L)
// Macros to contrain values
#define NOLESS(v,n) do{ if (v < n) v = n; }while(0)
#define NOMORE(v,n) do{ if (v > n) v = n; }while(0)
// For time
#define PENDING(NOW,SOON) ((long)(NOW-(SOON))<0)
#define ELAPSED(NOW,SOON) (!PENDING(NOW,SOON))
#ifndef bool
#define bool uint8_t
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

void update_pwm();
#ifdef ENABLE_AUTOTUNE
void PID_autotune(float temp, int ncycles);
#endif
float get_temp();
void manage_heater();

extern volatile uint8_t soft_pwm;
extern float target_temperature;
extern float current_temperature;
extern uint16_t temp_raw;
extern uint32_t temp_res;

#endif
