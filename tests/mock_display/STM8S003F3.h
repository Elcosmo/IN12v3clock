#ifndef _STM8S003F3_H_
#define _STM8S003F3_H_

#include <stdint.h>

#define _TIM2_OVR_UIF_VECTOR_ 0
#define _TIM2_CAPCOM_CC1IF_VECTOR_ 0
#define ISR_HANDLER(name, vector) void name(void)

typedef struct {
	uint8_t ODR2;
	uint8_t ODR3;
	uint8_t ODR5;
	uint8_t ODR6;
	uint8_t ODR7;
} MockPortOdr;

typedef struct {
	uint8_t DDR2;
	uint8_t DDR3;
	uint8_t DDR5;
	uint8_t DDR6;
	uint8_t DDR7;
} MockPortDdr;

typedef struct {
	uint8_t C12;
	uint8_t C13;
	uint8_t C15;
	uint8_t C16;
	uint8_t C17;
} MockPortCr1;

typedef struct {
	uint8_t C22;
	uint8_t C23;
	uint8_t C25;
	uint8_t C26;
	uint8_t C27;
} MockPortCr2;

typedef struct {
	MockPortOdr ODR;
	MockPortDdr DDR;
	MockPortCr1 CR1;
	MockPortCr2 CR2;
} MockPort;

typedef struct {
	uint8_t byte;
} MockByteReg;

typedef struct {
	uint8_t ARPE;
	uint8_t CEN;
} MockTimCr1;

typedef struct {
	uint8_t PSC;
} MockTimPscr;

typedef struct {
	uint8_t OC1PE;
} MockTimCcmr1;

typedef struct {
	uint8_t CC1E;
	uint8_t CC2E;
} MockTimCcer1;

typedef struct {
	uint8_t CC1IE;
	uint8_t CC2IE;
	uint8_t UIE;
} MockTimIer;

typedef struct {
	uint8_t UIF;
	uint8_t CC1IF;
	uint8_t CC2IF;
} MockTimSr1;

typedef struct {
	MockTimCr1 CR1;
	MockTimPscr PSCR;
	MockByteReg ARRH;
	MockByteReg ARRL;
	MockByteReg CCR1H;
	MockByteReg CCR1L;
	MockByteReg CCR2H;
	MockByteReg CCR2L;
	MockTimCcmr1 CCMR1;
	MockTimCcer1 CCER1;
	MockTimIer IER;
	MockTimSr1 SR1;
} MockTim2;

extern MockPort sfr_PORTC;
extern MockPort sfr_PORTD;
extern MockTim2 sfr_TIM2;

#endif /* _STM8S003F3_H_ */
