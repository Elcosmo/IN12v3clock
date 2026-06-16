#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "STM8S003F3.h"
#include "hc595.h"

MockPort sfr_PORTC;
MockPort sfr_PORTD;
MockTim2 sfr_TIM2;

static unsigned failures;

static void expect_u8(const char *label, uint8_t got, uint8_t expected)
{
	if (got != expected) {
		printf("FAIL %s got=%u expected=%u\n", label, (unsigned)got, (unsigned)expected);
		failures++;
	}
}

int main(void)
{
	uint8_t zeros[5] = {0,0,0,0,0};

	memset(&sfr_PORTC, 0xAA, sizeof(sfr_PORTC));
	memset(&sfr_PORTD, 0xAA, sizeof(sfr_PORTD));
	memset(&sfr_TIM2, 0, sizeof(sfr_TIM2));

	hc595Init();

	expect_u8("clock pin safe low", sfr_PORTD.ODR.ODR2, 0);
	expect_u8("latch pin safe low", sfr_PORTC.ODR.ODR7, 0);
	expect_u8("data pin safe low", sfr_PORTC.ODR.ODR5, 0);
	expect_u8("oe disabled after init", sfr_PORTD.ODR.ODR3, 1);

	expect_u8("clock ddr", sfr_PORTD.DDR.DDR2, 1);
	expect_u8("oe ddr", sfr_PORTD.DDR.DDR3, 1);
	expect_u8("data ddr", sfr_PORTC.DDR.DDR5, 1);
	expect_u8("latch ddr", sfr_PORTC.DDR.DDR7, 1);

	hc595ChainShiftOut(zeros,sizeof(zeros));

	expect_u8("oe remains disabled while zero latched", sfr_PORTD.ODR.ODR3, 1);
	expect_u8("clock low after shift", sfr_PORTD.ODR.ODR2, 0);
	expect_u8("latch low after shift", sfr_PORTC.ODR.ODR7, 0);
	expect_u8("data low after zero shift", sfr_PORTC.ODR.ODR5, 0);

	hc595OutputEnable();
	expect_u8("oe enabled", sfr_PORTD.ODR.ODR3, 0);

	hc595OutputDisable();
	expect_u8("oe disabled", sfr_PORTD.ODR.ODR3, 1);

	if (failures) {
		printf("FAIL hc595 init tests failures=%u\n", failures);
		return 1;
	}

	printf("PASS hc595 init tests\n");
	return 0;
}
