#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "display.h"
#include "eeprom.h"

MockPort sfr_PORTD;
MockPort sfr_PORTC;
MockTim2 sfr_TIM2;
Edata e;

static uint8_t mock_eeprom[COLON_BLINKING_TYPE + 1];
static uint16_t rgb_r;
static uint16_t rgb_g;
static uint16_t rgb_b;
static unsigned eeprom_read_count;
static unsigned eeprom_write_count;
static unsigned hc595_init_count;
static unsigned hc595_shift_count;
static unsigned hc595_enable_count;
static uint8_t hc595_zero_latched;
static unsigned failures;

static void fail(const char *message)
{
	printf("FAIL %s\n", message);
	failures++;
}

static void expect_u16(const char *label, uint16_t got, uint16_t expected)
{
	if (got != expected) {
		printf("FAIL %s got=%u expected=%u\n", label, got, expected);
		failures++;
	}
}

static void expect_zero_rgb(const char *context)
{
	expect_u16(context, rgb_r, 0);
	expect_u16(context, rgb_g, 0);
	expect_u16(context, rgb_b, 0);
}

static void expect_dot_off(const char *context)
{
	if (sfr_TIM2.IER.CC2IE != 0 || sfr_PORTD.ODR.ODR6 != 0) {
		printf("FAIL %s CC2IE=%u DOT=%u\n",
		       context,
		       (unsigned)sfr_TIM2.IER.CC2IE,
		       (unsigned)sfr_PORTD.ODR.ODR6);
		failures++;
	}
}

static void expect_dot_on(const char *context)
{
	if (sfr_TIM2.IER.CC2IE != 0 || sfr_PORTD.ODR.ODR6 != 1) {
		printf("FAIL %s CC2IE=%u DOT=%u\n",
		       context,
		       (unsigned)sfr_TIM2.IER.CC2IE,
		       (unsigned)sfr_PORTD.ODR.ODR6);
		failures++;
	}
}

static uint16_t expected_scaled(uint8_t value)
{
	return (uint16_t)(((int32_t)value * 10000) / 255);
}

static void test_dot_output_levels(void)
{
	displayDotGateSet(1);
	sfr_TIM2.IER.CC2IE = 1;
	sfr_TIM2.SR1.CC2IF = 1;
	displayDotDigital(1);
	expect_dot_on("digital dot on");
	expect_u16("digital dot clears CC2IF", sfr_TIM2.SR1.CC2IF, 0);

	sfr_TIM2.SR1.CC2IF = 1;
	TIM2_CAP_ISR();
	expect_dot_on("digital dot remains on after CC2 ISR");
	expect_u16("CC2 ISR clears pending flag", sfr_TIM2.SR1.CC2IF, 0);

	TIM2_UPD_ISR();
	expect_dot_on("digital dot remains on after update ISR");

	displayDotDigital(0);
	expect_dot_off("digital dot off");

	sfr_TIM2.IER.CC2IE = 1;
	sfr_TIM2.SR1.CC2IF = 1;
	displayDot(1);
	expect_dot_on("legacy dot wrapper is digital");
	expect_u16("legacy dot wrapper clears CC2IF", sfr_TIM2.SR1.CC2IF, 0);
}

static void test_dot_gate_closed(void)
{
	displayDotGateSet(0);
	expect_dot_off("dot gate close");
	sfr_TIM2.IER.CC2IE = 1;
	sfr_TIM2.SR1.CC2IF = 1;
	displayDotDigital(1);
	expect_dot_off("dot request while closed");
	expect_u16("closed gate clears CC2IF", sfr_TIM2.SR1.CC2IF, 0);
	displayDotGateSet(1);
	displayDotDigital(1);
	expect_dot_on("dot gate reopen accepts on");
}

static void expect_saved_config_unchanged(uint8_t colon_type,
					  uint8_t red,
					  uint8_t green,
					  uint8_t blue)
{
	if (e.colonBlinkingType != colon_type ||
	    mock_eeprom[R_ADDR] != red ||
	    mock_eeprom[G_ADDR] != green ||
	    mock_eeprom[B_ADDR] != blue) {
		fail("saved configuration changed");
	}
}

void hc595Init(void)
{
	hc595_init_count++;
}

void hc595ChainShiftOut(uint8_t *data, uint8_t lenght)
{
	uint8_t k;

	hc595_shift_count++;
	if ((hc595_shift_count == 1) && sfr_TIM2.CR1.CEN) {
		fail("timer running before HC595 zero latch");
	}
	if (lenght != 5) {
		printf("FAIL hc595 zero length=%u\n", (unsigned)lenght);
		failures++;
	}
	if (hc595_shift_count == 1) {
		hc595_zero_latched = 1;
		for (k = 0; k < lenght; k++) {
			if (data[k] != 0) {
				printf("FAIL hc595 init data[%u]=0x%02X\n", (unsigned)k, (unsigned)data[k]);
				failures++;
				hc595_zero_latched = 0;
			}
		}
	}
}

void hc595OutputDisable(void)
{
}

void hc595OutputEnable(void)
{
	hc595_enable_count++;
	if (!hc595_zero_latched) {
		fail("HC595 output enabled before zero latch");
	}
	if (!sfr_TIM2.CR1.CEN) {
		fail("HC595 output enabled before timer start");
	}
}

void RGBinit(void)
{
}

void RGBsetR(uint16_t value)
{
	rgb_r = value;
}

void RGBsetG(uint16_t value)
{
	rgb_g = value;
}

void RGBsetB(uint16_t value)
{
	rgb_b = value;
}

uint8_t EEPROM_writeByte(uint16_t logAddr, uint8_t data)
{
	(void)logAddr;
	(void)data;
	eeprom_write_count++;
	return 0;
}

uint8_t EEPROM_readByte(uint16_t logAddr)
{
	eeprom_read_count++;
	if (logAddr < sizeof(mock_eeprom)) {
		return mock_eeprom[logAddr];
	}
	return 0;
}

int main(void)
{
	const uint8_t saved_colon_type = 1;
	const uint8_t saved_red = 51;
	const uint8_t saved_green = 102;
	const uint8_t saved_blue = 204;

	memset(&sfr_PORTD, 0, sizeof(sfr_PORTD));
	memset(&sfr_PORTC, 0, sizeof(sfr_PORTC));
	memset(&sfr_TIM2, 0, sizeof(sfr_TIM2));
	memset(&e, 0, sizeof(e));
	memset(mock_eeprom, 0, sizeof(mock_eeprom));

	e.rgbGlobalEn = 1;
	e.colonBlinkingType = saved_colon_type;
	mock_eeprom[R_ADDR] = saved_red;
	mock_eeprom[G_ADDR] = saved_green;
	mock_eeprom[B_ADDR] = saved_blue;

	displayInit();
	expect_zero_rgb("init rgb gate closed");
	if (hc595_init_count != 1 || hc595_shift_count != 1 || hc595_enable_count != 1) {
		printf("FAIL hc595 init=%u shift=%u enable=%u\n",
		       hc595_init_count,
		       hc595_shift_count,
		       hc595_enable_count);
		failures++;
	}

	displayRGBGateSet(1);
	expect_u16("rgb gate reopen red", rgb_r, expected_scaled(saved_red));
	expect_u16("rgb gate reopen green", rgb_g, expected_scaled(saved_green));
	expect_u16("rgb gate reopen blue", rgb_b, expected_scaled(saved_blue));

	displayRGBGateSet(0);
	expect_zero_rgb("rgb gate close");

	displayRGBset(1);
	expect_zero_rgb("rgb request while closed");

	displayRset(255);
	displayGset(128);
	displayBset(64);
	expect_zero_rgb("direct rgb setters while closed");

	displayRGBGateSet(1);
	expect_u16("rgb requested state restored red", rgb_r, expected_scaled(saved_red));
	expect_u16("rgb requested state restored green", rgb_g, expected_scaled(saved_green));
	expect_u16("rgb requested state restored blue", rgb_b, expected_scaled(saved_blue));

	displayDotGateSet(1);
	displayDotDigital(1);
	expect_dot_on("dot did not turn on while gate open");

	displayDotGateSet(0);
	expect_dot_off("dot gate close");

	displayDotDigital(1);
	expect_dot_off("dot request while closed");

	displayDotGateSet(1);
	test_dot_output_levels();
	test_dot_gate_closed();
	e.colonBlinkingType = saved_colon_type;
	displayDotGateSet(1);
	expect_saved_config_unchanged(saved_colon_type, saved_red, saved_green, saved_blue);

	if (eeprom_write_count != 0) {
		printf("FAIL eeprom writes=%u\n", eeprom_write_count);
		failures++;
	}

	if (eeprom_read_count == 0) {
		fail("expected RGB apply to read EEPROM color values");
	}

	if (failures) {
		printf("FAIL display gate tests failures=%u\n", failures);
		return 1;
	}

	printf("PASS display gate tests eeprom_reads=%u eeprom_writes=%u\n",
	       eeprom_read_count,
	       eeprom_write_count);
	return 0;
}
