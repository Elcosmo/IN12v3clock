#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ds3231.h"
#include "i2c.h"

#define MAX_OPS 128
#define MAX_ACKS 16
#define MAX_READ_BYTES 16

typedef enum {
	OP_INIT,
	OP_BUSY_CHECK,
	OP_SLAVE_UNLOCK,
	OP_START,
	OP_STOP,
	OP_SHIFT,
	OP_ACK
} OpType;

typedef struct {
	OpType type;
	uint8_t arg;
	uint8_t ret;
} Op;

static Op ops[MAX_OPS];
static unsigned op_count;
static uint8_t busy_result;
static uint8_t ack_values[MAX_ACKS];
static unsigned ack_count;
static unsigned ack_index;
static uint8_t read_values[MAX_READ_BYTES];
static unsigned read_count;
static unsigned read_index;
static uint8_t last_shift_was_read;

static void reset_mock(void)
{
	memset(ops,0,sizeof(ops));
	op_count = 0;
	busy_result = OK;
	memset(ack_values,ACK,sizeof(ack_values));
	ack_count = 0;
	ack_index = 0;
	memset(read_values,0,sizeof(read_values));
	read_count = 0;
	read_index = 0;
	last_shift_was_read = 0;
}

static void add_op(OpType type, uint8_t arg, uint8_t ret)
{
	if (op_count >= MAX_OPS) {
		printf("FAIL mock operation overflow\n");
		return;
	}
	ops[op_count].type = type;
	ops[op_count].arg = arg;
	ops[op_count].ret = ret;
	op_count++;
}

static void set_acks(const uint8_t *values, unsigned count)
{
	unsigned i;
	ack_count = count;
	for (i = 0; i < count; i++) {
		ack_values[i] = values[i];
	}
}

static void set_reads(const uint8_t *values, unsigned count)
{
	unsigned i;
	read_count = count;
	for (i = 0; i < count; i++) {
		read_values[i] = values[i];
	}
}

void i2c_init(void)
{
	add_op(OP_INIT,0,0);
}

uint8_t i2c_busy_check(void)
{
	add_op(OP_BUSY_CHECK,0,busy_result);
	return busy_result;
}

void i2c_slave_unlock(void)
{
	add_op(OP_SLAVE_UNLOCK,0,0);
}

void i2c_start(void)
{
	add_op(OP_START,0,0);
}

void i2c_stop(void)
{
	add_op(OP_STOP,0,0);
}

uint8_t i2c_shift(uint8_t b)
{
	uint8_t ret = 0;
	if ((b == 0xFF) && (read_index < read_count)) {
		ret = read_values[read_index++];
		last_shift_was_read = 1;
	} else {
		last_shift_was_read = 0;
	}
	add_op(OP_SHIFT,b,ret);
	return ret;
}

uint8_t i2c_ack(uint8_t b)
{
	uint8_t ret = ACK;
	if (!last_shift_was_read) {
		if (ack_index < ack_count) {
			ret = ack_values[ack_index++];
		}
	}
	add_op(OP_ACK,b,ret);
	last_shift_was_read = 0;
	return ret;
}

static int expect_op(unsigned index, OpType type, uint8_t arg)
{
	if (index >= op_count) {
		printf("FAIL missing op index=%u\n", index);
		return 0;
	}
	if ((ops[index].type != type) || (ops[index].arg != arg)) {
		printf("FAIL op index=%u expected type=%u arg=0x%02X actual type=%u arg=0x%02X\n",
			index,
			(unsigned)type,
			(unsigned)arg,
			(unsigned)ops[index].type,
			(unsigned)ops[index].arg);
		return 0;
	}
	return 1;
}

static int expect_op_count(unsigned expected)
{
	if (op_count != expected) {
		printf("FAIL op count expected=%u actual=%u\n", expected, op_count);
		return 0;
	}
	return 1;
}

static int verify_read_sequence(void)
{
	return expect_op_count(18) &&
		expect_op(0,OP_BUSY_CHECK,0) &&
		expect_op(1,OP_START,0) &&
		expect_op(2,OP_SHIFT,DS3231_ADDRESS) &&
		expect_op(3,OP_ACK,READ_ANSWER) &&
		expect_op(4,OP_SHIFT,SECONDS_ADDR) &&
		expect_op(5,OP_ACK,READ_ANSWER) &&
		expect_op(6,OP_START,0) &&
		expect_op(7,OP_SHIFT,DS3231_ADDRESS | 0x01) &&
		expect_op(8,OP_ACK,READ_ANSWER) &&
		expect_op(9,OP_SHIFT,0xFF) &&
		expect_op(10,OP_ACK,SET_ACK) &&
		expect_op(11,OP_SHIFT,0xFF) &&
		expect_op(12,OP_ACK,SET_ACK) &&
		expect_op(13,OP_SHIFT,0xFF) &&
		expect_op(14,OP_ACK,SET_ACK) &&
		expect_op(15,OP_SHIFT,0xFF) &&
		expect_op(16,OP_ACK,SET_NAK) &&
		expect_op(17,OP_STOP,0);
}

static int expect_safe_time(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t weekday)
{
	if ((seconds != 0) || (minutes != 0) || (hours != 0) || (weekday != 1)) {
		printf("FAIL safe time seconds=0x%02X minutes=0x%02X hours=0x%02X weekday=%u\n",
			(unsigned)seconds,
			(unsigned)minutes,
			(unsigned)hours,
			(unsigned)weekday);
		return 0;
	}
	return 1;
}

static int test_read_time(uint8_t rtc_seconds,
			  uint8_t rtc_minutes,
			  uint8_t rtc_hours,
			  uint8_t rtc_weekday,
			  uint8_t expected_valid)
{
	uint8_t ack_ok[] = {ACK,ACK,ACK};
	uint8_t reads[] = {rtc_seconds,rtc_minutes,rtc_hours,rtc_weekday};
	uint8_t seconds = 0;
	uint8_t minutes = 0;
	uint8_t hours = 0;
	uint8_t weekday = 0;
	uint8_t valid;

	reset_mock();
	set_acks(ack_ok,3);
	set_reads(reads,4);
	valid = ds3231_read_time(&seconds,&minutes,&hours,&weekday);

	if (valid != expected_valid) {
		printf("FAIL read validity expected=%u actual=%u seconds=0x%02X minutes=0x%02X hours=0x%02X weekday=%u\n",
			(unsigned)expected_valid,
			(unsigned)valid,
			(unsigned)rtc_seconds,
			(unsigned)rtc_minutes,
			(unsigned)rtc_hours,
			(unsigned)rtc_weekday);
		return 0;
	}
	if (expected_valid) {
		if ((seconds != rtc_seconds) || (minutes != rtc_minutes) || (hours != rtc_hours) || (weekday != rtc_weekday)) {
			printf("FAIL read values seconds=0x%02X minutes=0x%02X hours=0x%02X weekday=%u\n",
				(unsigned)seconds,
				(unsigned)minutes,
				(unsigned)hours,
				(unsigned)weekday);
			return 0;
		}
	} else if (!expect_safe_time(seconds,minutes,hours,weekday)) {
		return 0;
	}
	if (!verify_read_sequence()) {return 0;}
	return 1;
}

static int test_validation_function(void)
{
	if (!ds3231_time_valid(0x00,0x00,0x00,1)) {
		printf("FAIL validation rejected 00:00:00 weekday 1\n");
		return 0;
	}
	if (!ds3231_time_valid(0x59,0x59,0x23,7)) {
		printf("FAIL validation rejected 23:59:59 weekday 7\n");
		return 0;
	}
	if (ds3231_time_valid(0x00,0x99,0x12,1)) {
		printf("FAIL validation accepted minute 0x99\n");
		return 0;
	}
	if (ds3231_time_valid(0x00,0x00,0x28,1)) {
		printf("FAIL validation accepted hour 0x28\n");
		return 0;
	}
	if (ds3231_time_valid(0x00,0x00,0x52,1)) {
		printf("FAIL validation accepted 12h format\n");
		return 0;
	}
	if (ds3231_time_valid(0x6A,0x00,0x12,1)) {
		printf("FAIL validation accepted invalid BCD 0x6A\n");
		return 0;
	}
	if (ds3231_time_valid(0x00,0x00,0x12,0)) {
		printf("FAIL validation accepted weekday 0\n");
		return 0;
	}
	if (ds3231_time_valid(0x00,0x00,0x12,8)) {
		printf("FAIL validation accepted weekday 8\n");
		return 0;
	}
	return 1;
}

static int verify_write_weekday(uint8_t weekday)
{
	uint8_t ack_ok[] = {ACK,ACK,ACK};

	reset_mock();
	set_acks(ack_ok,3);
	ds3231_write_weekday(weekday);

	return expect_op_count(9) &&
		expect_op(0,OP_BUSY_CHECK,0) &&
		expect_op(1,OP_START,0) &&
		expect_op(2,OP_SHIFT,DS3231_ADDRESS) &&
		expect_op(3,OP_ACK,READ_ANSWER) &&
		expect_op(4,OP_SHIFT,DAY_ADDR) &&
		expect_op(5,OP_ACK,READ_ANSWER) &&
		expect_op(6,OP_SHIFT,weekday) &&
		expect_op(7,OP_ACK,READ_ANSWER) &&
		expect_op(8,OP_STOP,0);
}

static int verify_write_time(uint8_t seconds, uint8_t minutes, uint8_t hours)
{
	uint8_t ack_ok[] = {ACK,ACK,ACK,ACK,ACK};

	reset_mock();
	set_acks(ack_ok,5);
	ds3231_write_time(&seconds,&minutes,&hours);

	return expect_op_count(13) &&
		expect_op(0,OP_BUSY_CHECK,0) &&
		expect_op(1,OP_START,0) &&
		expect_op(2,OP_SHIFT,DS3231_ADDRESS) &&
		expect_op(3,OP_ACK,READ_ANSWER) &&
		expect_op(4,OP_SHIFT,SECONDS_ADDR) &&
		expect_op(5,OP_ACK,READ_ANSWER) &&
		expect_op(6,OP_SHIFT,seconds) &&
		expect_op(7,OP_ACK,READ_ANSWER) &&
		expect_op(8,OP_SHIFT,minutes) &&
		expect_op(9,OP_ACK,READ_ANSWER) &&
		expect_op(10,OP_SHIFT,hours) &&
		expect_op(11,OP_ACK,READ_ANSWER) &&
		expect_op(12,OP_STOP,0);
}

static int test_invalid_writes(void)
{
	reset_mock();
	ds3231_write_weekday(0);
	if (!expect_op_count(0)) {return 0;}
	reset_mock();
	ds3231_write_weekday(8);
	if (!expect_op_count(0)) {return 0;}
	return 1;
}

static int test_busy_and_nak(void)
{
	uint8_t ack_nak_first[] = {NAK};
	uint8_t ack_nak_second[] = {ACK,NAK};
	uint8_t seconds = 0xAA;
	uint8_t minutes = 0xBB;
	uint8_t hours = 0xCC;
	uint8_t weekday = 0xDD;
	uint8_t valid;

	reset_mock();
	busy_result = BUSY;
	valid = ds3231_read_time(&seconds,&minutes,&hours,&weekday);
	if (!(expect_op_count(2) && expect_op(0,OP_BUSY_CHECK,0) && expect_op(1,OP_SLAVE_UNLOCK,0))) {return 0;}
	if (valid || !expect_safe_time(seconds,minutes,hours,weekday)) {
		printf("FAIL busy read did not return invalid safe time\n");
		return 0;
	}

	reset_mock();
	busy_result = BUSY;
	ds3231_write_weekday(3);
	if (!(expect_op_count(2) && expect_op(0,OP_BUSY_CHECK,0) && expect_op(1,OP_SLAVE_UNLOCK,0))) {return 0;}

	reset_mock();
	set_acks(ack_nak_first,1);
	ds3231_write_weekday(3);
	if (!(expect_op_count(5) &&
		expect_op(0,OP_BUSY_CHECK,0) &&
		expect_op(1,OP_START,0) &&
		expect_op(2,OP_SHIFT,DS3231_ADDRESS) &&
		expect_op(3,OP_ACK,READ_ANSWER) &&
		expect_op(4,OP_STOP,0))) {return 0;}

	reset_mock();
	set_acks(ack_nak_second,2);
	seconds = 0xAA;
	minutes = 0xBB;
	hours = 0xCC;
	weekday = 0xDD;
	valid = ds3231_read_time(&seconds,&minutes,&hours,&weekday);
	if (!(expect_op_count(7) &&
		expect_op(0,OP_BUSY_CHECK,0) &&
		expect_op(1,OP_START,0) &&
		expect_op(2,OP_SHIFT,DS3231_ADDRESS) &&
		expect_op(3,OP_ACK,READ_ANSWER) &&
		expect_op(4,OP_SHIFT,SECONDS_ADDR) &&
		expect_op(5,OP_ACK,READ_ANSWER) &&
		expect_op(6,OP_STOP,0))) {return 0;}
	if (valid || !expect_safe_time(seconds,minutes,hours,weekday)) {
		printf("FAIL nak read did not return invalid safe time\n");
		return 0;
	}

	return 1;
}

int main(void)
{
	uint8_t weekday;

	reset_mock();
	ds3231Init();
	if (!(expect_op_count(1) && expect_op(0,OP_INIT,0))) {return 1;}
	if (!test_validation_function()) {return 1;}

	for (weekday = 1; weekday <= 7; weekday++) {
		if (!test_read_time(0x12,0x34,0x23,weekday,1)) {return 1;}
		if (!verify_write_weekday(weekday)) {return 1;}
	}
	if (!verify_write_time(0x00,0x30,0x07)) {return 1;}

	if (!test_read_time(0x12,0x99,0x23,1,0)) {return 1;}
	if (!test_read_time(0x12,0x34,0x28,1,0)) {return 1;}
	if (!test_read_time(0x12,0x34,0x52,1,0)) {return 1;}
	if (!test_read_time(0x6A,0x34,0x23,1,0)) {return 1;}
	if (!test_read_time(0x12,0x34,0x23,0,0)) {return 1;}
	if (!test_read_time(0x12,0x34,0x23,8,0)) {return 1;}
	if (!test_invalid_writes()) {return 1;}
	if (!test_busy_and_nak()) {return 1;}

	printf("PASS ds3231 rtc validation and protocol tests\n");
	return 0;
}
