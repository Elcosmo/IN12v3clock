#ifndef _SCHEDULE_H_
#define _SCHEDULE_H_

#include <stdint.h>

enum schedule_minutes{
	SCHEDULE_DAY_MINUTES = 1440,
	SCHEDULE_MORNING_START = 420,
	SCHEDULE_WEEKDAY_MORNING_END = 540,
	SCHEDULE_WEEKEND_MORNING_END = 720,
	SCHEDULE_WEEKDAY_EVENING_START = 1020,
	SCHEDULE_WEEKEND_EVENING_START = 1140,
	SCHEDULE_WEEKDAY_EVENING_END = 1440,
	SCHEDULE_WEEKEND_EVENING_END = 1500
};

static uint8_t schedule_day_enabled(uint8_t weekday, uint16_t current_minutes)
{
	if ((weekday < 1)||(weekday > 7)) {return 0;}
	if (weekday >= 6){
		if ((current_minutes >= SCHEDULE_MORNING_START)&&(current_minutes < SCHEDULE_WEEKEND_MORNING_END)) {return 1;}
		if ((current_minutes >= SCHEDULE_WEEKEND_EVENING_START)&&(current_minutes < SCHEDULE_WEEKEND_EVENING_END)) {return 1;}
	} else {
		if ((current_minutes >= SCHEDULE_MORNING_START)&&(current_minutes < SCHEDULE_WEEKDAY_MORNING_END)) {return 1;}
		if ((current_minutes >= SCHEDULE_WEEKDAY_EVENING_START)&&(current_minutes < SCHEDULE_WEEKDAY_EVENING_END)) {return 1;}
	}
	return 0;
}

static uint8_t schedule_display_enabled(uint8_t weekday, uint16_t current_minutes)
{
	uint8_t previous_weekday;
	if ((weekday < 1)||(weekday > 7)) {return 0;}
	if (schedule_day_enabled(weekday,current_minutes)) {return 1;}
	if (weekday == 1) {previous_weekday = 7;}
	else {previous_weekday = weekday - 1;}
	return schedule_day_enabled(previous_weekday,current_minutes + SCHEDULE_DAY_MINUTES);
}

#endif /* _SCHEDULE_H_ */
