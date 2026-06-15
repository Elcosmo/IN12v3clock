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

static int test_read_weekday(uint8_t rtc_weekday, uint8_t expected_weekday)
{
	uint8_t ack_ok[] = {ACK,ACK,ACK};
	uint8_t reads[] = {0x12,0x34,0x56,rtc_weekday};
	uint8_t seconds = 0;
	uint8_t minutes = 0;
	uint8_t hours = 0;
	uint8_t weekday = 0;

	reset_mock();
	set_acks(ack_ok,3);
	set_reads(reads,4);
	ds3231_read_time(&seconds,&minutes,&hours,&weekday);

	if ((seconds != 0x12) || (minutes != 0x34) || (hours != 0x56) || (weekday != expected_weekday)) {
		printf("FAIL read values rtc_weekday=%u seconds=0x%02X minutes=0x%02X hours=0x%02X weekday=%u\n",
			(unsigned)rtc_weekday,
			(unsigned)seconds,
			(unsigned)minutes,
			(unsigned)hours,
			(unsigned)weekday);
		return 0;
	}
	if (!verify_read_sequence()) {return 0;}
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

	reset_mock();
	busy_result = BUSY;
	ds3231_read_time(&seconds,&minutes,&hours,&weekday);
	if (!(expect_op_count(2) && expect_op(0,OP_BUSY_CHECK,0) && expect_op(1,OP_SLAVE_UNLOCK,0))) {return 0;}
	if ((seconds != 0xAA) || (minutes != 0xBB) || (hours != 0xCC) || (weekday != 0xDD)) {
		printf("FAIL busy read modified output variables\n");
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
	ds3231_read_time(&seconds,&minutes,&hours,&weekday);
	if (!(expect_op_count(7) &&
		expect_op(0,OP_BUSY_CHECK,0) &&
		expect_op(1,OP_START,0) &&
		expect_op(2,OP_SHIFT,DS3231_ADDRESS) &&
		expect_op(3,OP_ACK,READ_ANSWER) &&
		expect_op(4,OP_SHIFT,SECONDS_ADDR) &&
		expect_op(5,OP_ACK,READ_ANSWER) &&
		expect_op(6,OP_STOP,0))) {return 0;}

	return 1;
}

int main(void)
{
	uint8_t weekday;

	reset_mock();
	ds3231Init();
	if (!(expect_op_count(1) && expect_op(0,OP_INIT,0))) {return 1;}

	for (weekday = 1; weekday <= 7; weekday++) {
		if (!test_read_weekday(weekday,weekday)) {return 1;}
		if (!verify_write_weekday(weekday)) {return 1;}
	}

	if (!test_read_weekday(0,1)) {return 1;}
	if (!test_read_weekday(8,1)) {return 1;}
	if (!test_invalid_writes()) {return 1;}
	if (!test_busy_and_nak()) {return 1;}

	printf("PASS ds3231 read/write weekday protocol tests\n");
	return 0;
}
