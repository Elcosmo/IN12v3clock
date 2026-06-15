#include <stdint.h>
#include <stdio.h>
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

static uint8_t oracle_enabled(uint8_t weekday, uint16_t minutes)
{
	switch (weekday) {
		case 1:
			return (minutes < 60) ||
				((minutes >= 420) && (minutes < 540)) ||
				((minutes >= 1020) && (minutes < 1440));
		case 2:
		case 3:
		case 4:
		case 5:
			return ((minutes >= 420) && (minutes < 540)) ||
				((minutes >= 1020) && (minutes < 1440));
		case 6:
			return ((minutes >= 420) && (minutes < 720)) ||
				((minutes >= 1140) && (minutes < 1440));
		case 7:
			return (minutes < 60) ||
				((minutes >= 420) && (minutes < 720)) ||
				((minutes >= 1140) && (minutes < 1440));
		default:
			return 0;
	}
}

static void print_time(uint16_t minute_of_week)
{
	uint8_t weekday = (uint8_t)(minute_of_week / MINUTES_PER_DAY) + 1;
	uint16_t minutes = minute_of_week % MINUTES_PER_DAY;

	printf("weekday=%u %02u:%02u", (unsigned)weekday, (unsigned)(minutes / 60), (unsigned)(minutes % 60));
}

int main(void)
{
	uint16_t minute_of_week;
	uint16_t active_minutes = 0;
	uint16_t transition_count = 0;
	uint8_t previous = oracle_enabled(1,0);
	size_t transition_index = 0;

	for (minute_of_week = 0; minute_of_week < MINUTES_PER_WEEK; minute_of_week++) {
		uint8_t weekday = (uint8_t)(minute_of_week / MINUTES_PER_DAY) + 1;
		uint16_t minutes = minute_of_week % MINUTES_PER_DAY;
		uint8_t expected = oracle_enabled(weekday,minutes);
		uint8_t actual = schedule_display_enabled(weekday,minutes);

		if (actual != expected) {
			printf("FAIL first mismatch at minute %u (", (unsigned)minute_of_week);
			print_time(minute_of_week);
			printf("): expected=%u actual=%u\n", expected, actual);
			return 1;
		}

		if (actual) {active_minutes++;}

		if (minute_of_week > 0) {
			if (actual != previous) {
				if (transition_index >= ARRAY_COUNT(expected_transitions)) {
					printf("FAIL unexpected extra transition at minute %u (", (unsigned)minute_of_week);
					print_time(minute_of_week);
					printf("): state=%u\n", actual);
					return 1;
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
					return 1;
				}
				transition_index++;
				transition_count++;
			}
		}
		previous = actual;
	}

	for (minute_of_week = 0; minute_of_week < MINUTES_PER_DAY; minute_of_week++) {
		if (schedule_display_enabled(0,minute_of_week) != 0) {
			printf("FAIL invalid weekday 0 enabled at %02u:%02u\n", (unsigned)(minute_of_week / 60), (unsigned)(minute_of_week % 60));
			return 1;
		}
		if (schedule_display_enabled(8,minute_of_week) != 0) {
			printf("FAIL invalid weekday 8 enabled at %02u:%02u\n", (unsigned)(minute_of_week / 60), (unsigned)(minute_of_week % 60));
			return 1;
		}
	}

	if (active_minutes != EXPECTED_ACTIVE_MINUTES) {
		printf("FAIL active minutes: expected=%u actual=%u\n", (unsigned)EXPECTED_ACTIVE_MINUTES, (unsigned)active_minutes);
		return 1;
	}

	if (transition_index != ARRAY_COUNT(expected_transitions)) {
		printf("FAIL transition count: expected=%u actual=%u\n",
			(unsigned)ARRAY_COUNT(expected_transitions),
			(unsigned)transition_count);
		return 1;
	}

	printf("PASS minutes_tested=%u invalid_weekday_minutes_tested=%u active_minutes=%u transitions=%u\n",
		(unsigned)MINUTES_PER_WEEK,
		(unsigned)(2 * MINUTES_PER_DAY),
		(unsigned)active_minutes,
		(unsigned)transition_count);

	return 0;
}
