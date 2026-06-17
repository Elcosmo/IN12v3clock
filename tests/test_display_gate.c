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

#define DOT_CALL_MS 30
#define DOT_PULSE_LUT_SIZE 24U
#define DOT_PULSE_PHASES (DOT_PULSE_LUT_SIZE * 2U)
#define DOT_PULSE_DIVIDER 2U
#define DOT_PULSE_CYCLE_MS (DOT_PULSE_PHASES * DOT_PULSE_DIVIDER * DOT_CALL_MS)
#define DOT_PULSE_MAX_STEP_PERCENT 6U

static const uint8_t dot_pulse_lut[DOT_PULSE_LUT_SIZE] = {
	24, 24, 25, 27, 29, 32, 35, 39,
	43, 48, 53, 58, 62, 67, 72, 77,
	81, 85, 88, 91, 93, 95, 96, 96
};

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

static uint16_t expected_scaled(uint8_t value)
{
	return (uint16_t)(((int32_t)value * 10000) / 255);
}

static uint16_t expected_dot_pwm(uint8_t percent)
{
	uint16_t value;

	if (percent == 0) {
		return 0;
	}
	value = (uint16_t)(((int32_t)percent * 10000) / 100);
	if (value < DOT_MIN_BRIGHT) {
		value = DOT_MIN_BRIGHT;
	}
	return value;
}

static uint16_t actual_dot_pwm(void)
{
	if (!sfr_TIM2.IER.CC2IE) {
		return 0;
	}
	return (((uint16_t)sfr_TIM2.CCR2H.byte) << 8) | sfr_TIM2.CCR2L.byte;
}

static uint8_t expected_default_dot_percent(uint8_t phase)
{
	if (phase >= DOT_PULSE_LUT_SIZE) {
		phase = (DOT_PULSE_PHASES - 1U) - phase;
	}
	return dot_pulse_lut[phase];
}

static void expect_dot_pwm(const char *label, uint16_t expected)
{
	uint16_t got = actual_dot_pwm();

	if (got != expected) {
		printf("FAIL %s dot_pwm=%u expected=%u\n", label, got, expected);
		failures++;
	}
}

static void test_default_dot_pulse_cycle(void)
{
	uint8_t step;
	uint8_t repeat;
	uint16_t pwm[DOT_PULSE_PHASES + 1U];
	uint16_t prev_pwm = 0;
	uint8_t have_prev = 0;

	e.colonBlinkingType = 0;
	displayDotGateSet(0);
	displayDotGateSet(1);

	for (step = 0; step < DOT_PULSE_PHASES; step++) {
		for (repeat = 0; repeat < DOT_PULSE_DIVIDER; repeat++) {
			uint16_t got;
			uint16_t expected;
			uint16_t diff;

			displayDotPulseProc();
			got = actual_dot_pwm();
			expected = expected_dot_pwm(expected_default_dot_percent(step));
			expect_dot_pwm("default dot pulse", expected);
			if (got == 0) {
				fail("default dot pulse reached zero while gate is open");
			}
			if (have_prev) {
				diff = (got > prev_pwm) ? (got - prev_pwm) : (prev_pwm - got);
				if (diff > expected_dot_pwm(DOT_PULSE_MAX_STEP_PERCENT)) {
					fail("default dot pulse step jump is too large");
				}
			}
			prev_pwm = got;
			have_prev = 1;
		}
		pwm[step] = actual_dot_pwm();
	}
	displayDotPulseProc();
	pwm[DOT_PULSE_PHASES] = actual_dot_pwm();

	for (step = 1; step < DOT_PULSE_LUT_SIZE; step++) {
		if (pwm[step] < pwm[step - 1]) {
			fail("default dot pulse rise is not monotone");
		}
	}
	for (step = DOT_PULSE_LUT_SIZE + 1U; step < DOT_PULSE_PHASES; step++) {
		if (pwm[step] > pwm[step - 1]) {
			fail("default dot pulse fall is not monotone");
		}
	}
	if ((pwm[0] != pwm[DOT_PULSE_PHASES - 1U]) ||
	    (pwm[0] != pwm[DOT_PULSE_PHASES])) {
		fail("default dot pulse cycle is not continuous at wrap");
	}
	if (pwm[0] <= DOT_MIN_BRIGHT) {
		fail("default dot pulse minimum is not above clamp threshold");
	}
	if ((DOT_PULSE_CYCLE_MS < 2700U) || (DOT_PULSE_CYCLE_MS > 3100U)) {
		fail("default dot pulse cycle duration is outside range");
	}
}

static void test_dot_modes_unchanged(void)
{
	uint8_t step;

	displayDotGateSet(1);
	e.colonBlinkingType = 1;
	for (step = 0; step < 60; step++) {
		displayDotPulseProc();
		expect_dot_pwm("colon always on", expected_dot_pwm(200));
	}

	e.colonBlinkingType = 2;
	for (step = 0; step < 60; step++) {
		displayDotPulseProc();
		expect_dot_pwm("colon always off", 0);
	}
}

static void test_dot_gate_closed(void)
{
	uint8_t step;

	displayDotGateSet(0);
	e.colonBlinkingType = 0;
	for (step = 0; step < 40; step++) {
		displayDotPulseProc();
		expect_dot_pwm("closed dot gate default", 0);
	}
	e.colonBlinkingType = 1;
	for (step = 0; step < 40; step++) {
		displayDotPulseProc();
		expect_dot_pwm("closed dot gate always on", 0);
	}
	displayDotGateSet(1);
	e.colonBlinkingType = 0;
	displayDotPulseProc();
	expect_dot_pwm("dot gate reopen starts at minimum",
		       expected_dot_pwm(expected_default_dot_percent(0)));
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
	if (sfr_TIM2.CR1.CEN) {
		fail("timer running before HC595 zero latch");
	}
	if (lenght != 5) {
		printf("FAIL hc595 zero length=%u\n", (unsigned)lenght);
		failures++;
	}
	hc595_zero_latched = 1;
	for (k = 0; k < lenght; k++) {
		if (data[k] != 0) {
			printf("FAIL hc595 init data[%u]=0x%02X\n", (unsigned)k, (unsigned)data[k]);
			failures++;
			hc595_zero_latched = 0;
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
	displayDot(1);
	if (sfr_TIM2.IER.CC2IE == 0) {
		fail("dot did not enable while gate open");
	}

	displayDotGateSet(0);
	expect_dot_off("dot gate close");

	displayDot(1);
	expect_dot_off("dot request while closed");

	displayDotPulseProc();
	expect_dot_off("dot pulse while closed");

	displayDotGateSet(1);
	test_default_dot_pulse_cycle();
	test_dot_modes_unchanged();
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
