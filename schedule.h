#ifndef _SCHEDULE_H_
#define _SCHEDULE_H_

#include <stdint.h>

typedef struct {
	uint8_t weekday1_start;
	uint8_t weekday1_end;
	uint8_t weekday2_start;
	uint8_t weekday2_end;
	uint8_t weekend1_start;
	uint8_t weekend1_end;
	uint8_t weekend2_start;
	uint8_t weekend2_end;
} ScheduleConfig;

enum schedule_config_index{
	SCHEDULE_WEEKDAY1_START_INDEX,
	SCHEDULE_WEEKDAY1_END_INDEX,
	SCHEDULE_WEEKDAY2_START_INDEX,
	SCHEDULE_WEEKDAY2_END_INDEX,
	SCHEDULE_WEEKEND1_START_INDEX,
	SCHEDULE_WEEKEND1_END_INDEX,
	SCHEDULE_WEEKEND2_START_INDEX,
	SCHEDULE_WEEKEND2_END_INDEX,
	SCHEDULE_CONFIG_COUNT
};

enum schedule_defaults{
	SCHEDULE_DEFAULT_WEEKDAY1_START = 7,
	SCHEDULE_DEFAULT_WEEKDAY1_END = 9,
	SCHEDULE_DEFAULT_WEEKDAY2_START = 17,
	SCHEDULE_DEFAULT_WEEKDAY2_END = 0,
	SCHEDULE_DEFAULT_WEEKEND1_START = 7,
	SCHEDULE_DEFAULT_WEEKEND1_END = 12,
	SCHEDULE_DEFAULT_WEEKEND2_START = 19,
	SCHEDULE_DEFAULT_WEEKEND2_END = 1
};

#ifdef SCHEDULE_DEFAULTS_IMPLEMENTATION
static void schedule_config_defaults(ScheduleConfig *config)
{
	config->weekday1_start = SCHEDULE_DEFAULT_WEEKDAY1_START;
	config->weekday1_end = SCHEDULE_DEFAULT_WEEKDAY1_END;
	config->weekday2_start = SCHEDULE_DEFAULT_WEEKDAY2_START;
	config->weekday2_end = SCHEDULE_DEFAULT_WEEKDAY2_END;
	config->weekend1_start = SCHEDULE_DEFAULT_WEEKEND1_START;
	config->weekend1_end = SCHEDULE_DEFAULT_WEEKEND1_END;
	config->weekend2_start = SCHEDULE_DEFAULT_WEEKEND2_START;
	config->weekend2_end = SCHEDULE_DEFAULT_WEEKEND2_END;
}

#endif /* SCHEDULE_DEFAULTS_IMPLEMENTATION */

#ifdef SCHEDULE_IMPLEMENTATION
static uint8_t schedule_interval_current(uint8_t current_hour, uint8_t start_hour, uint8_t end_hour)
{
	if ((current_hour > 23)||(start_hour > 23)||(end_hour > 23)) {return 0;}
	if (start_hour == end_hour) {return 0;}
	if (start_hour < end_hour) {return ((current_hour >= start_hour)&&(current_hour < end_hour));}
	return (current_hour >= start_hour);
}

static uint8_t schedule_interval_previous(uint8_t current_hour, uint8_t start_hour, uint8_t end_hour)
{
	if ((current_hour > 23)||(start_hour > 23)||(end_hour > 23)) {return 0;}
	if (start_hour <= end_hour) {return 0;}
	return (current_hour < end_hour);
}

static uint8_t schedule_day_enabled(const ScheduleConfig *config, uint8_t weekday, uint8_t current_hour)
{
	if (config == 0) {return 0;}
	if ((weekday < 1)||(weekday > 7)) {return 0;}
	if (weekday >= 6){
		if (schedule_interval_current(current_hour,config->weekend1_start,config->weekend1_end)) {return 1;}
		if (schedule_interval_current(current_hour,config->weekend2_start,config->weekend2_end)) {return 1;}
	} else {
		if (schedule_interval_current(current_hour,config->weekday1_start,config->weekday1_end)) {return 1;}
		if (schedule_interval_current(current_hour,config->weekday2_start,config->weekday2_end)) {return 1;}
	}
	return 0;
}

static uint8_t schedule_previous_day_enabled(const ScheduleConfig *config, uint8_t weekday, uint8_t current_hour)
{
	if (weekday >= 6){
		if (schedule_interval_previous(current_hour,config->weekend1_start,config->weekend1_end)) {return 1;}
		if (schedule_interval_previous(current_hour,config->weekend2_start,config->weekend2_end)) {return 1;}
	} else {
		if (schedule_interval_previous(current_hour,config->weekday1_start,config->weekday1_end)) {return 1;}
		if (schedule_interval_previous(current_hour,config->weekday2_start,config->weekday2_end)) {return 1;}
	}
	return 0;
}

static uint8_t schedule_display_enabled(const ScheduleConfig *config, uint8_t weekday, uint8_t current_hour)
{
	uint8_t previous_weekday;
	if ((weekday < 1)||(weekday > 7)) {return 0;}
	if (schedule_day_enabled(config,weekday,current_hour)) {return 1;}
	if (weekday == 1) {previous_weekday = 7;}
	else {previous_weekday = weekday - 1;}
	return schedule_previous_day_enabled(config,previous_weekday,current_hour);
}

#endif /* SCHEDULE_IMPLEMENTATION */

#endif /* _SCHEDULE_H_ */
