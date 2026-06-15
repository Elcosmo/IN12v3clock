#ifndef _DISPLAY_H_
#define	_DISPLAY_H_

#ifdef	__cplusplus
extern "C" {
#endif

#include "STM8S003F3.h"

#define DOT_PIN						sfr_PORTD.ODR.ODR6
#define NIXIE_COUNT				4
#define DOT_MIN_BRIGHT		100
#define NIXIE_MIN_BRIGHT	100
#define NIXIE_OFF					10


void displayInit ( void );
void displaySetBright(uint8_t bright);
void displayNixie(uint8_t *data, uint8_t full_bright_bitmask);
void displayDot (uint8_t state);
void displayDotGateSet (uint8_t state);
void displayDotPulse (void);
void displayDotPulseProc (void);

void displayRGBset (uint8_t state);
void displayRGBGateSet (uint8_t state);
void displayRset(uint8_t value);
void displayGset(uint8_t value);
void displayBset(uint8_t value);

ISR_HANDLER (TIM2_UPD_ISR, _TIM2_OVR_UIF_VECTOR_);
ISR_HANDLER (TIM2_CAP_ISR, _TIM2_CAPCOM_CC1IF_VECTOR_);

#ifdef	__cplusplus
}
#endif

#endif	/* _DISPLAY_H_ */
