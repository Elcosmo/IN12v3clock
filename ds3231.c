#include "STM8S003F3.h"
#include "ds3231.h"
#include "i2c.h"

void ds3231Init(void)
{
	i2c_init();
	// дописать инициализацию, особенно часов, чтобы был только 24 часа формат времени
}

static uint8_t ds3231_bcd_00_59_valid(uint8_t value)
{
	if (value & 0x80) {return 0;}
	if ((value & 0x0F) > 9) {return 0;}
	if (((value >> 4) & 0x07) > 5) {return 0;}
	return 1;
}

static void ds3231_set_safe_time(uint8_t *seconds, uint8_t *minutes, uint8_t *hours, uint8_t *weekday)
{
	*seconds = 0;
	*minutes = 0;
	*hours = 0;
	*weekday = 1;
}

uint8_t ds3231_time_valid(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t weekday)
{
	uint8_t hour_decimal;

	if (!ds3231_bcd_00_59_valid(seconds)) {return 0;}
	if (!ds3231_bcd_00_59_valid(minutes)) {return 0;}
	if (hours & 0xC0) {return 0;}
	if ((hours & 0x0F) > 9) {return 0;}
	hour_decimal = ((hours >> 4) * 10) + (hours & 0x0F);
	if (hour_decimal > 23) {return 0;}
	if ((weekday < 1)||(weekday > 7)) {return 0;}
	return 1;
}

uint8_t ds3231_read_time(uint8_t *seconds, uint8_t *minutes, uint8_t *hours, uint8_t *weekday)
{
	if (i2c_busy_check() == OK){
		i2c_start();
		i2c_shift(DS3231_ADDRESS);
		if (i2c_ack(READ_ANSWER)==ACK)
		{
			i2c_shift(SECONDS_ADDR);
			if (i2c_ack(READ_ANSWER)==ACK)
			{
				i2c_start();
				i2c_shift(DS3231_ADDRESS | 0x01);
				if (i2c_ack(READ_ANSWER)==ACK)
				{
					uint8_t rtc_seconds = i2c_shift(0xFF); i2c_ack(SET_ACK);
					uint8_t rtc_minutes = i2c_shift(0xFF); i2c_ack(SET_ACK);
					uint8_t rtc_hours = i2c_shift(0xFF); 	i2c_ack(SET_ACK);
					uint8_t rtc_weekday = i2c_shift(0xFF); i2c_ack(SET_NAK);
					i2c_stop();
					if (ds3231_time_valid(rtc_seconds,rtc_minutes,rtc_hours,rtc_weekday)){
						*seconds = rtc_seconds;
						*minutes = rtc_minutes;
						*hours = rtc_hours;
						*weekday = rtc_weekday;
						return 1;
					}
					ds3231_set_safe_time(seconds,minutes,hours,weekday);
					return 0;
				}
			}
		}
	}else{
		i2c_slave_unlock();
		ds3231_set_safe_time(seconds,minutes,hours,weekday);
		return 0;
	}
	i2c_stop();
	ds3231_set_safe_time(seconds,minutes,hours,weekday);
	return 0;
}

void ds3231_write_time(uint8_t *seconds, uint8_t *minutes, uint8_t *hours)
{
	if (i2c_busy_check() == OK){
		i2c_start();
		i2c_shift(DS3231_ADDRESS);
		if (i2c_ack(READ_ANSWER)==ACK)
		{
			i2c_shift(SECONDS_ADDR);
			if (i2c_ack(READ_ANSWER)==ACK)
			{
				i2c_shift(*seconds); 
				if (i2c_ack(READ_ANSWER)==ACK)
				{
					i2c_shift(*minutes); 
					if (i2c_ack(READ_ANSWER)==ACK)
					{
						i2c_shift(*hours); 
						if (i2c_ack(READ_ANSWER)==ACK)
						{
							i2c_stop();
							return;
						}
					}
				}
			}
		}
	}else{
		i2c_slave_unlock();
		return;
	}
	i2c_stop();
}

void ds3231_write_weekday(uint8_t weekday)
{
	if ((weekday < 1)||(weekday > 7)) {return;}
	if (i2c_busy_check() == OK){
		i2c_start();
		i2c_shift(DS3231_ADDRESS);
		if (i2c_ack(READ_ANSWER)==ACK)
		{
			i2c_shift(DAY_ADDR);
			if (i2c_ack(READ_ANSWER)==ACK)
			{
				i2c_shift(weekday);
				if (i2c_ack(READ_ANSWER)==ACK)
				{
					i2c_stop();
					return;
				}
			}
		}
	}else{
		i2c_slave_unlock();
		return;
	}
	i2c_stop();
}