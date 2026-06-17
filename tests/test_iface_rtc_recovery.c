#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "iface.h"
#include "display.h"
#include "ds3231.h"
#include "eeprom.h"

extern Iface i;

Edata e;

static uint8_t mock_rtc_valid;
static uint8_t last_nixie[NIXIE_COUNT];
static uint8_t last_nixie_mask;
static uint8_t last_dot_request;
static uint8_t last_dot_gate = 1;
static uint8_t last_rgb_gate = 1;
static uint8_t last_rgb_state;
static uint8_t display_nixie_calls;
static unsigned rtc_reads;
static uint8_t mock_seconds = 0x02;
static uint8_t mock_minutes = 0x31;
static uint8_t mock_hours = 0x07;
static uint8_t mock_weekday = 1;
static unsigned failures;

static void fail(const char *message)
{
	printf("FAIL %s\n", message);
	failures++;
}

static void expect_u8(const char *label, uint8_t got, uint8_t expected)
{
	if (got != expected) {
		printf("FAIL %s got=%u expected=%u\n", label, (unsigned)got, (unsigned)expected);
		failures++;
	}
}

static void run_iface_events(unsigned count)
{
	unsigned k;

	for (k = 0; k < count; k++) {
		i.flag10ms = 1;
		iface_proc();
	}
}

static void run_display_period(void)
{
	run_iface_events(250);
}

static void set_mock_time(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t weekday)
{
	mock_seconds = seconds;
	mock_minutes = minutes;
	mock_hours = hours;
	mock_weekday = weekday;
}

static uint8_t nixie_all_off(void)
{
	uint8_t k;

	for (k = 0; k < NIXIE_COUNT; k++) {
		if (last_nixie[k] != NIXIE_OFF) {return 0;}
	}
	return 1;
}

static void expect_nixie_off(const char *label)
{
	if (!nixie_all_off()) {
		printf("FAIL %s nixie=%u,%u,%u,%u mask=0x%02X\n",
		       label,
		       (unsigned)last_nixie[0],
		       (unsigned)last_nixie[1],
		       (unsigned)last_nixie[2],
		       (unsigned)last_nixie[3],
		       (unsigned)last_nixie_mask);
		failures++;
	}
}

static void expect_nixie_digits(const char *label, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
{
	if ((last_nixie[0] != d0) ||
	    (last_nixie[1] != d1) ||
	    (last_nixie[2] != d2) ||
	    (last_nixie[3] != d3)) {
		printf("FAIL %s nixie=%u,%u,%u,%u expected=%u,%u,%u,%u\n",
		       label,
		       (unsigned)last_nixie[0],
		       (unsigned)last_nixie[1],
		       (unsigned)last_nixie[2],
		       (unsigned)last_nixie[3],
		       (unsigned)d0,
		       (unsigned)d1,
		       (unsigned)d2,
		       (unsigned)d3);
		failures++;
	}
}

uint8_t ds3231_read_time(uint8_t *seconds, uint8_t *minutes, uint8_t *hours, uint8_t *weekday)
{
	rtc_reads++;
	if (!mock_rtc_valid) {
		*seconds = 0;
		*minutes = 0;
		*hours = 0;
		*weekday = 1;
		return 0;
	}
	*seconds = mock_seconds;
	*minutes = mock_minutes;
	*hours = mock_hours;
	*weekday = mock_weekday;
	return 1;
}

uint8_t ds3231_time_valid(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t weekday)
{
	(void)seconds;
	(void)minutes;
	(void)hours;
	(void)weekday;
	return mock_rtc_valid;
}

void ds3231Init(void)
{
}

void ds3231_write_time(uint8_t *seconds, uint8_t *minutes, uint8_t *hours)
{
	(void)seconds;
	(void)minutes;
	(void)hours;
}

void ds3231_write_weekday(uint8_t weekday)
{
	(void)weekday;
}

void keyevents_proc(void)
{
}

void keyevents_counters(void)
{
}

void set_service_timing_mutex(void)
{
}

void reset_service_timing_mutex(void)
{
}

void displaySetBright(uint8_t bright)
{
	(void)bright;
}

void displayNixie(uint8_t *data, uint8_t full_bright_bitmask)
{
	memcpy(last_nixie,data,NIXIE_COUNT);
	last_nixie_mask = full_bright_bitmask;
	display_nixie_calls++;
}

void displayDot(uint8_t state)
{
	last_dot_request = state;
}

void displayDotGateSet(uint8_t state)
{
	last_dot_gate = state ? 1 : 0;
}

void displayRGBset(uint8_t state)
{
	last_rgb_state = state;
}

void displayRGBGateSet(uint8_t state)
{
	last_rgb_gate = state ? 1 : 0;
}

void displayRset(uint8_t value)
{
	(void)value;
}

void displayGset(uint8_t value)
{
	(void)value;
}

void displayBset(uint8_t value)
{
	(void)value;
}

uint8_t EEPROM_writeByte(uint16_t logAddr, uint8_t data)
{
	(void)logAddr;
	(void)data;
	fail("unexpected EEPROM write");
	return 0;
}

uint8_t EEPROM_readByte(uint16_t logAddr)
{
	(void)logAddr;
	return 0;
}

static void expect_colon_state(const char *label,
			       uint8_t seconds,
			       uint8_t minutes,
			       uint8_t hours,
			       uint8_t weekday,
			       uint8_t colon_type,
			       uint8_t expected_gate,
			       uint8_t expected_dot)
{
	set_mock_time(seconds,minutes,hours,weekday);
	e.colonBlinkingType = colon_type;
	i.display_state = SETUP_NO;
	mock_rtc_valid = 1;
	last_dot_request = 0xAA;
	last_dot_gate = 0xAA;
	run_display_period();
	expect_u8(label, last_dot_request, expected_dot);
	expect_u8("colon gate", last_dot_gate, expected_gate);
	run_display_period();
	expect_u8("colon stable in same second", last_dot_request, expected_dot);
}

int main(void)
{
	memset(&i,0,sizeof(i));
	memset(&e,0,sizeof(e));
	memset(last_nixie,NIXIE_OFF,sizeof(last_nixie));
	e.bright = 100;
	e.zeroEn = 0;
	e.rgbGlobalEn = 1;
	e.nBrightEn = 0;

	mock_rtc_valid = 0;
	set_mock_time(0x02,0x31,0x07,1);
	iface_init();
	expect_u8("rtc invalid after init", i.rtcValid, 0);
	expect_u8("safe seconds", i.seconds, 0);
	expect_u8("safe minutes", i.minutes, 0);
	expect_u8("safe hours", i.hours, 0);
	expect_u8("safe weekday", i.weekday, 1);

	i.antipoisoningEn = 1;
	i.apFlagEn[0] = 1;
	i.apOld[0] = 9;
	i.apNew[0] = 0;
	run_display_period();
	expect_nixie_off("normal invalid rtc display off");
	expect_u8("invalid rtc dot gate", last_dot_gate, 0);
	expect_u8("invalid rtc dot off", last_dot_request, 0);
	expect_u8("invalid rtc rgb gate", last_rgb_gate, 0);
	expect_u8("invalid rtc antipoisoning stopped", i.antipoisoningEn, 0);

	i.display_state = SETUP_HOURS;
	i.hoursSetupValue = 0x07;
	i.minutesSetupValue = 0x30;
	run_display_period();
	expect_nixie_digits("time setup visible invalid rtc", 0, 3, 7, 0);
	expect_u8("time setup keeps dot gate closed", last_dot_gate, 0);
	expect_u8("time setup keeps rgb gate closed", last_rgb_gate, 0);

	i.display_state = SETUP_ZERO;
	i.setupValue = 1;
	run_display_period();
	expect_nixie_digits("general menu visible invalid rtc", 1, NIXIE_OFF, NIXIE_OFF, SETUP_ZERO);

	i.display_state = SETUP_WEEKDAY;
	i.setupValue = 7;
	run_display_period();
	expect_nixie_digits("weekday menu visible invalid rtc", 7, NIXIE_OFF, 2, 1);

	i.display_state = SETUP_NO;
	mock_rtc_valid = 1;
	display_nixie_calls = 0;
	run_display_period();
	expect_u8("rtc recovered", i.rtcValid, 1);
	expect_u8("normal display called after recovery", display_nixie_calls > 0, 1);
	expect_nixie_digits("normal display recovered", 1, 3, 7, NIXIE_OFF);
	expect_u8("rgb desired restored after recovery", last_rgb_state, 1);
	expect_u8("rgb gate open after recovery", last_rgb_gate, 1);

	expect_colon_state("00 seconds colon on", 0x00, 0x30, 0x07, 1, 0, 1, 1);
	expect_colon_state("01 seconds colon off", 0x01, 0x30, 0x07, 1, 0, 1, 0);
	expect_colon_state("02 seconds colon on", 0x02, 0x30, 0x07, 1, 0, 1, 1);
	expect_colon_state("03 seconds colon off", 0x03, 0x30, 0x07, 1, 0, 1, 0);
	expect_colon_state("58 seconds colon on", 0x58, 0x30, 0x07, 1, 0, 1, 1);
	expect_colon_state("59 seconds colon off", 0x59, 0x30, 0x07, 1, 0, 1, 0);
	expect_colon_state("59 to 00 starts off", 0x59, 0x30, 0x07, 1, 0, 1, 0);
	expect_colon_state("59 to 00 then on", 0x00, 0x31, 0x07, 1, 0, 1, 1);
	expect_colon_state("schedule disabled colon off", 0x02, 0x00, 0x10, 1, 0, 0, 0);
	expect_colon_state("gate reopen follows even second", 0x02, 0x30, 0x07, 1, 0, 1, 1);
	expect_colon_state("menu 11 value 1 colon on", 0x03, 0x30, 0x07, 1, 1, 1, 1);
	expect_colon_state("menu 11 value 2 colon off", 0x02, 0x30, 0x07, 1, 2, 1, 0);

	if (failures) {
		printf("FAIL iface rtc recovery tests failures=%u\n", failures);
		return 1;
	}

	printf("PASS iface rtc invalid/menu/recovery tests\n");
	return 0;
}
