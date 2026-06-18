#include <stdint.h>
#include <stdio.h>
#define SCHEDULE_DEFAULTS_IMPLEMENTATION
#define SCHEDULE_IMPLEMENTATION
#include "../schedule.h"

#define WEEKDAYS 7
#define MINUTES_PER_DAY 1440
#define MINUTES_PER_WEEK (WEEKDAYS * MINUTES_PER_DAY)
#define EXPECTED_ACTIVE_MINUTES 4020
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
	uint16_t minute_of_week;
	uint8_t enabled;
} Transition;

static const Transition expected_transitions[] = {
	{60,0},
	{420,1},
	{540,0},
	{1020,1},
	{1440,0},
	{1860,1},
	{1980,0},
	{2460,1},
	{2880,0},
	{3300,1},
	{3420,0},
	{3900,1},
	{4320,0},
	{4740,1},
	{4860,0},
	{5340,1},
	{5760,0},
	{6180,1},
	{6300,0},
	{6780,1},
	{7200,0},
	{7620,1},
	{7920,0},
	{8340,1},
	{8700,0},
	{9060,1},
	{9360,0},
	{9780,1}
};

static uint8_t oracle_interval(uint16_t current_minutes, uint8_t start_hour, uint8_t end_hour)
{
	uint16_t start_minutes;
	uint16_t end_minutes;

	if ((start_hour > 23)||(end_hour > 23)) {return 0;}
	if (start_hour == end_hour) {return 0;}
	start_minutes = (uint16_t)start_hour * 60;
	end_minutes = (uint16_t)end_hour * 60;
	if (start_hour > end_hour) {end_minutes += MINUTES_PER_DAY;}
	return ((current_minutes >= start_minutes)&&(current_minutes < end_minutes));
}

static uint8_t oracle_day_enabled(const ScheduleConfig *config, uint8_t weekday, uint16_t minutes)
{
	if ((weekday < 1)||(weekday > 7)) {return 0;}
	if (weekday >= 6) {
		return oracle_interval(minutes,config->weekend1_start,config->weekend1_end) ||
			oracle_interval(minutes,config->weekend2_start,config->weekend2_end);
	}
	return oracle_interval(minutes,config->weekday1_start,config->weekday1_end) ||
		oracle_interval(minutes,config->weekday2_start,config->weekday2_end);
}

static uint8_t oracle_enabled(const ScheduleConfig *config, uint8_t weekday, uint16_t minutes)
{
	uint8_t previous_weekday;

	if ((weekday < 1)||(weekday > 7)) {return 0;}
	if (oracle_day_enabled(config,weekday,minutes)) {return 1;}
	previous_weekday = (weekday == 1) ? 7 : (uint8_t)(weekday - 1);
	return oracle_day_enabled(config,previous_weekday,minutes + MINUTES_PER_DAY);
}

static void print_time(uint16_t minute_of_week)
{
	uint8_t weekday = (uint8_t)(minute_of_week / MINUTES_PER_DAY) + 1;
	uint16_t minutes = minute_of_week % MINUTES_PER_DAY;

	printf("weekday=%u %02u:%02u", (unsigned)weekday, (unsigned)(minutes / 60), (unsigned)(minutes % 60));
}

static int expect_state(const char *label, const ScheduleConfig *config, uint8_t weekday, uint8_t hour, uint8_t minute, uint8_t expected)
{
	uint8_t actual = schedule_display_enabled(config,weekday,hour);

	if (actual != expected) {
		printf("FAIL %s weekday=%u %02u:%02u expected=%u actual=%u\n",
			label,
			(unsigned)weekday,
			(unsigned)hour,
			(unsigned)minute,
			(unsigned)expected,
			(unsigned)actual);
		return 0;
	}
	return 1;
}

static int test_default_week(void)
{
	ScheduleConfig config;
	uint16_t minute_of_week;
	uint16_t active_minutes = 0;
	uint16_t transition_count = 0;
	uint8_t previous;
	size_t transition_index = 0;

	schedule_config_defaults(&config);
	previous = oracle_enabled(&config,1,0);

	for (minute_of_week = 0; minute_of_week < MINUTES_PER_WEEK; minute_of_week++) {
		uint8_t weekday = (uint8_t)(minute_of_week / MINUTES_PER_DAY) + 1;
		uint16_t minutes = minute_of_week % MINUTES_PER_DAY;
		uint8_t expected = oracle_enabled(&config,weekday,minutes);
		uint8_t actual = schedule_display_enabled(&config,weekday,(uint8_t)(minutes / 60));

		if (actual != expected) {
			printf("FAIL first mismatch at minute %u (", (unsigned)minute_of_week);
			print_time(minute_of_week);
			printf("): expected=%u actual=%u\n", expected, actual);
			return 0;
		}

		if (actual) {active_minutes++;}

		if (minute_of_week > 0) {
			if (actual != previous) {
				if (transition_index >= ARRAY_COUNT(expected_transitions)) {
					printf("FAIL unexpected extra transition at minute %u (", (unsigned)minute_of_week);
					print_time(minute_of_week);
					printf("): state=%u\n", actual);
					return 0;
				}
				if ((expected_transitions[transition_index].minute_of_week != minute_of_week) ||
					(expected_transitions[transition_index].enabled != actual)) {
					printf("FAIL transition mismatch at index %u: expected minute=%u state=%u, actual minute=%u state=%u (",
						(unsigned)transition_index,
						(unsigned)expected_transitions[transition_index].minute_of_week,
						(unsigned)expected_transitions[transition_index].enabled,
						(unsigned)minute_of_week,
						(unsigned)actual);
					print_time(minute_of_week);
					printf(")\n");
					return 0;
				}
				transition_index++;
				transition_count++;
			}
		}
		previous = actual;
	}

	for (minute_of_week = 0; minute_of_week < MINUTES_PER_DAY; minute_of_week++) {
		if (schedule_display_enabled(&config,0,(uint8_t)(minute_of_week / 60)) != 0) {
			printf("FAIL invalid weekday 0 enabled at %02u:%02u\n", (unsigned)(minute_of_week / 60), (unsigned)(minute_of_week % 60));
			return 0;
		}
		if (schedule_display_enabled(&config,8,(uint8_t)(minute_of_week / 60)) != 0) {
			printf("FAIL invalid weekday 8 enabled at %02u:%02u\n", (unsigned)(minute_of_week / 60), (unsigned)(minute_of_week % 60));
			return 0;
		}
	}

	if (active_minutes != EXPECTED_ACTIVE_MINUTES) {
		printf("FAIL active minutes: expected=%u actual=%u\n", (unsigned)EXPECTED_ACTIVE_MINUTES, (unsigned)active_minutes);
		return 0;
	}

	if (transition_index != ARRAY_COUNT(expected_transitions)) {
		printf("FAIL transition count: expected=%u actual=%u\n",
			(unsigned)ARRAY_COUNT(expected_transitions),
			(unsigned)transition_count);
		return 0;
	}

	printf("PASS default schedule minutes_tested=%u invalid_weekday_minutes_tested=%u active_minutes=%u transitions=%u\n",
		(unsigned)MINUTES_PER_WEEK,
		(unsigned)(2 * MINUTES_PER_DAY),
		(unsigned)active_minutes,
		(unsigned)transition_count);
	return 1;
}

static int test_custom_schedules(void)
{
	ScheduleConfig config;

	schedule_config_defaults(&config);
	config.weekday1_start = 6;
	config.weekday1_end = 8;
	config.weekday2_start = 22;
	config.weekday2_end = 2;
	config.weekend1_start = 10;
	config.weekend1_end = 10;
	config.weekend2_start = 21;
	config.weekend2_end = 23;

	return expect_state("custom before weekday morning", &config, 1, 5, 59, 0) &&
		expect_state("custom weekday morning start", &config, 1, 6, 0, 1) &&
		expect_state("custom weekday morning end", &config, 1, 8, 0, 0) &&
		expect_state("custom weekday midnight span start", &config, 1, 23, 30, 1) &&
		expect_state("custom weekday midnight span previous day", &config, 2, 1, 30, 1) &&
		expect_state("custom weekday midnight span end", &config, 2, 2, 0, 0) &&
		expect_state("custom disabled weekend first slot", &config, 6, 10, 30, 0) &&
		expect_state("custom weekend evening", &config, 6, 22, 0, 1) &&
		expect_state("custom weekend evening end", &config, 6, 23, 0, 0);
}

static int test_boundaries(void)
{
	ScheduleConfig config;
	ScheduleConfig disabled;

	schedule_config_defaults(&config);
	disabled.weekday1_start = 7;
	disabled.weekday1_end = 7;
	disabled.weekday2_start = 17;
	disabled.weekday2_end = 17;
	disabled.weekend1_start = 7;
	disabled.weekend1_end = 7;
	disabled.weekend2_start = 19;
	disabled.weekend2_end = 19;

	return expect_state("friday late on", &config, 5, 23, 59, 1) &&
		expect_state("saturday after weekday boundary off", &config, 6, 0, 30, 0) &&
		expect_state("sunday previous saturday on", &config, 7, 0, 30, 1) &&
		expect_state("monday previous sunday on", &config, 1, 0, 30, 1) &&
		expect_state("tuesday after weekday boundary off", &config, 2, 0, 30, 0) &&
		expect_state("disabled weekday morning", &disabled, 1, 7, 30, 0) &&
		expect_state("disabled weekend previous day", &disabled, 7, 0, 30, 0);
}

int main(void)
{
	if (!test_default_week()) {return 1;}
	if (!test_custom_schedules()) {return 1;}
	if (!test_boundaries()) {return 1;}

	printf("PASS configurable schedule tests\n");
	return 0;
}
