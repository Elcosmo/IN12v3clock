#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "iface.h"
#include "display.h"
#include "ds3231.h"
#define SCHEDULE_DEFAULTS_IMPLEMENTATION
#include "eeprom.h"

extern Edata e;
Iface i;

static uint8_t mock_eeprom[CONFIG_VERSION_ADDR + 1];
static unsigned writes_total;
static unsigned writes_by_addr[CONFIG_VERSION_ADDR + 1];
static unsigned failures;

static const uint8_t default_schedule[SCHEDULE_CONFIG_COUNT] = {7,9,17,0,7,12,19,1};
static const uint8_t custom_schedule[SCHEDULE_CONFIG_COUNT] = {6,8,20,23,9,13,21,2};

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

static void reset_counters(void)
{
	writes_total = 0;
	memset(writes_by_addr,0,sizeof(writes_by_addr));
}

static void set_valid_base(void)
{
	memset(mock_eeprom,0,sizeof(mock_eeprom));
	mock_eeprom[BRIGHT_ADDR] = 80;
	mock_eeprom[ANTIPOISONING_EFFECT_ADDR] = 1;
	mock_eeprom[RGB_EN_ADDR] = 1;
	mock_eeprom[COLON_BLINKING_TYPE] = 2;
}

static uint8_t *schedule_bytes(void)
{
	return (uint8_t *)&e.schedule;
}

static void store_schedule(const uint8_t *values)
{
	uint8_t k;

	for (k = 0; k < SCHEDULE_CONFIG_COUNT; k++){
		mock_eeprom[SCHEDULE_WEEKDAY1_START_ADDR + k] = values[k];
	}
}

static void expect_schedule(const char *label, const uint8_t *values)
{
	uint8_t k;
	uint8_t *actual = schedule_bytes();

	for (k = 0; k < SCHEDULE_CONFIG_COUNT; k++){
		if (actual[k] != values[k]){
			printf("FAIL %s ram[%u]=%u expected=%u\n", label, (unsigned)k, (unsigned)actual[k], (unsigned)values[k]);
			failures++;
		}
		if (mock_eeprom[SCHEDULE_WEEKDAY1_START_ADDR + k] != values[k]){
			printf("FAIL %s eeprom[%u]=%u expected=%u\n", label, (unsigned)k, (unsigned)mock_eeprom[SCHEDULE_WEEKDAY1_START_ADDR + k], (unsigned)values[k]);
			failures++;
		}
	}
}

uint8_t EEPROM_writeByte(uint16_t logAddr, uint8_t data)
{
	if (logAddr >= sizeof(mock_eeprom)){
		fail("EEPROM write out of range");
		return 0;
	}
	mock_eeprom[logAddr] = data;
	writes_by_addr[logAddr]++;
	writes_total++;
	return 1;
}

uint8_t EEPROM_readByte(uint16_t logAddr)
{
	if (logAddr >= sizeof(mock_eeprom)){
		return 0xFF;
	}
	return mock_eeprom[logAddr];
}

uint8_t keyboard_get_key(void)
{
	return 0xFF;
}

void iface_flag05sReset(void)
{
}

void iface_start_antipoisoning(void)
{
}

void RGBtoggle(void)
{
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

void displayRGBset(uint8_t state)
{
	(void)state;
}

void displaySetBright(uint8_t bright)
{
	(void)bright;
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

void event_single_key1(void);
void event_single_key2(void);

static void test_migration_defaults(void)
{
	uint8_t k;

	set_valid_base();
	for (k = 0; k < SCHEDULE_CONFIG_COUNT; k++){
		mock_eeprom[SCHEDULE_WEEKDAY1_START_ADDR + k] = 0xEE;
	}
	mock_eeprom[CONFIG_VERSION_ADDR] = 0xFF;
	reset_counters();
	EEPROM_dataInit();
	expect_schedule("old format migration", default_schedule);
	expect_u8("config version written", mock_eeprom[CONFIG_VERSION_ADDR], EEPROM_CONFIG_VERSION);
	expect_u8("old format write count", (uint8_t)writes_total, 9);
	expect_u8("brightness preserved", mock_eeprom[BRIGHT_ADDR], 80);
	expect_u8("rgb preserved", mock_eeprom[RGB_EN_ADDR], 1);

	reset_counters();
	EEPROM_dataInit();
	expect_schedule("second boot defaults", default_schedule);
	expect_u8("second boot write count", (uint8_t)writes_total, 0);
}

static void test_invalid_hour_resets_all(void)
{
	set_valid_base();
	store_schedule(custom_schedule);
	mock_eeprom[CONFIG_VERSION_ADDR] = EEPROM_CONFIG_VERSION;
	mock_eeprom[SCHEDULE_WEEKEND1_END_ADDR] = 24;
	reset_counters();
	EEPROM_dataInit();
	expect_schedule("invalid hour reset", default_schedule);
	expect_u8("invalid hour write count", (uint8_t)writes_total, SCHEDULE_CONFIG_COUNT);
}

static void test_custom_power_cycle(void)
{
	set_valid_base();
	store_schedule(custom_schedule);
	mock_eeprom[CONFIG_VERSION_ADDR] = EEPROM_CONFIG_VERSION;
	reset_counters();
	EEPROM_dataInit();
	expect_schedule("custom power cycle", custom_schedule);
	expect_u8("custom power cycle write count", (uint8_t)writes_total, 0);

	reset_counters();
	EEPROM_dataInit();
	expect_schedule("custom second boot", custom_schedule);
	expect_u8("custom second boot write count", (uint8_t)writes_total, 0);
}

static void test_menu_schedule_save(void)
{
	uint8_t k;
	uint8_t values[SCHEDULE_CONFIG_COUNT] = {1,2,3,4,5,6,7,8};

	schedule_config_defaults(&e.schedule);
	for (k = 0; k < SCHEDULE_CONFIG_COUNT; k++){
		i.display_state = SETUP_WEEKDAY1_START + k;
		i.setupValue = values[k];
		event_single_key2();
		expect_u8("menu save EEPROM", mock_eeprom[SCHEDULE_WEEKDAY1_START_ADDR + k], values[k]);
		expect_u8("menu save RAM", schedule_bytes()[k], values[k]);
	}

	i.display_state = SETUP_WEEKDAY1_START;
	i.setupValue = 23;
	event_single_key1();
	expect_u8("menu hour wrap to zero", i.setupValue, 0);
	event_single_key1();
	expect_u8("menu hour increment after wrap", i.setupValue, 1);
}

int main(void)
{
	test_migration_defaults();
	test_invalid_hour_resets_all();
	test_custom_power_cycle();
	test_menu_schedule_save();

	if (failures) {
		printf("FAIL schedule EEPROM/menu tests failures=%u\n", failures);
		return 1;
	}

	printf("PASS schedule EEPROM/menu tests writes=%u\n", writes_total);
	return 0;
}
