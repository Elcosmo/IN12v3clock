#include "STM8S003F3.h"
#include "eeprom.h"

Edata e;

static uint8_t *EEPROM_scheduleData(void)
{
	return (uint8_t *)&e.schedule;
}

static void EEPROM_readSchedule(void)
{
	uint8_t k;
	uint8_t *data = EEPROM_scheduleData();

	for (k = 0; k < SCHEDULE_CONFIG_COUNT; k++){
		data[k] = EEPROM_readByte(SCHEDULE_WEEKDAY1_START_ADDR + k);
	}
}

static uint8_t EEPROM_scheduleValid(void)
{
	uint8_t k;
	uint8_t *data = EEPROM_scheduleData();

	for (k = 0; k < SCHEDULE_CONFIG_COUNT; k++){
		if (data[k] > 23) {return 0;}
	}
	return 1;
}

static void EEPROM_writeSchedule(void)
{
	uint8_t k;
	uint8_t *data = EEPROM_scheduleData();

	for (k = 0; k < SCHEDULE_CONFIG_COUNT; k++){
		EEPROM_writeByte(SCHEDULE_WEEKDAY1_START_ADDR + k,data[k]);
	}
}

static void EEPROM_writeScheduleDefaults(void)
{
	e.schedule.weekday1_start = SCHEDULE_DEFAULT_WEEKDAY1_START;
	e.schedule.weekday1_end = SCHEDULE_DEFAULT_WEEKDAY1_END;
	e.schedule.weekday2_start = SCHEDULE_DEFAULT_WEEKDAY2_START;
	e.schedule.weekday2_end = SCHEDULE_DEFAULT_WEEKDAY2_END;
	e.schedule.weekend1_start = SCHEDULE_DEFAULT_WEEKEND1_START;
	e.schedule.weekend1_end = SCHEDULE_DEFAULT_WEEKEND1_END;
	e.schedule.weekend2_start = SCHEDULE_DEFAULT_WEEKEND2_START;
	e.schedule.weekend2_end = SCHEDULE_DEFAULT_WEEKEND2_END;
	EEPROM_writeSchedule();
}

void EEPROM_dataInit(void)
{
	e.rgbGlobalEn = EEPROM_readByte(RGB_EN_ADDR);
	if (e.rgbGlobalEn > 1) {
		e.rgbGlobalEn = 0;
		EEPROM_writeByte(RGB_EN_ADDR,e.rgbGlobalEn);
	}

	e.zeroEn = EEPROM_readByte(ZERO_ADDR);
	if (e.zeroEn > 1) {
		e.zeroEn = 0;
		EEPROM_writeByte(ZERO_ADDR,e.zeroEn);
	}

	e.f1224 = EEPROM_readByte(F1224_ADDR);
	if (e.f1224) {													//disable, 12h time display is not working yet!
		e.f1224 = 0;
		EEPROM_writeByte(F1224_ADDR,e.f1224);
	}

	e.bright = EEPROM_readByte(BRIGHT_ADDR);
	if ((e.bright > 100)||(e.bright < 5)) {
		e.bright = 100;
		EEPROM_writeByte(BRIGHT_ADDR,e.bright);
	}

	EEPROM_readSchedule();
	if (EEPROM_readByte(CONFIG_VERSION_ADDR) != EEPROM_CONFIG_VERSION){
		EEPROM_writeScheduleDefaults();
		EEPROM_writeByte(CONFIG_VERSION_ADDR,EEPROM_CONFIG_VERSION);
	} else if (!EEPROM_scheduleValid()){
		EEPROM_writeScheduleDefaults();
	}

	e.antipoisoningEffect = EEPROM_readByte(ANTIPOISONING_EFFECT_ADDR);
	if (e.antipoisoningEffect!= 1) {
		e.antipoisoningEffect = 1;	// todo temporally it's only 1
		EEPROM_writeByte(ANTIPOISONING_EFFECT_ADDR, e.antipoisoningEffect);
	}

	e.colonBlinkingType = EEPROM_readByte(COLON_BLINKING_TYPE);
	if (e.colonBlinkingType > 2){
		e.colonBlinkingType = 0;
		EEPROM_writeByte(COLON_BLINKING_TYPE,e.colonBlinkingType);
	}
}

#ifndef HOST_TEST
/**
  \fn uint8_t EEPROM_writeByte(uint16_t logAddr, uint8_t data)

  \brief write 1B to D-flash / EEPROM

  \param[in] logAddr    logical address to write to (starting from EEPROM_ADDR_START)
  \param[in] data       byte to program

  \return write successful(=1) or error(=0)

  write single byte to logical address in D-flash / EEPROM
*/
uint8_t EEPROM_writeByte(uint16_t logAddr, uint8_t data) {

  uint16_t   addr = EEPROM_ADDR_START + logAddr;  // physical address
  uint8_t    countTimeout;                        // timeout counter

  // address range check
  if (logAddr > EEPROM_SIZE)
    return(0);

  // begin critical cection (disable interrupts)
  DISABLE_INTERRUPTS();

  // unlock w/e access to EEPROM
  sfr_FLASH.DUKR.byte = 0xAE;
  sfr_FLASH.DUKR.byte = 0x56;

  // wait until access granted
  while(!sfr_FLASH.IAPSR.DUL);

  // write byte in 16-bit address range
  *((uint8_t*) addr) = data;

  // wait until done or timeout (normal flash write measured with 0 --> 100 is more than sufficient)
  countTimeout = 100;                                // ~0.95us/inc -> ~0.1ms
  while ((!sfr_FLASH.IAPSR.EOP) && (countTimeout--));

  // lock EEPROM again against accidental erase/write
  sfr_FLASH.IAPSR.DUL = 0;

  // critical section (enable interrupts)
  ENABLE_INTERRUPTS();

  // write successful -> return 1
  return(countTimeout != 0);

} // EEPROM_writeByte


/**
  \fn uint8_t EEPROM_readByte(uint16_t logAddr)

  \brief read 1B from D-flash / EEPROM

  \param[in] logAddr    logical address to read from (starting from EEPROM_ADDR_START)

  \return read byte (0xFF on error)

  read single byte to logical address in D-flash / EEPROM
*/
uint8_t EEPROM_readByte(uint16_t logAddr) {

  uint16_t   addr = EEPROM_ADDR_START + logAddr;  // physical address

  // address range check
  if (logAddr > EEPROM_SIZE)
    return(0xFF);

  // return read data
  return(*((uint8_t*) addr));

} // EEPROM_readByte
#endif
