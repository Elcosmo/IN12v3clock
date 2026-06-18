#ifndef _EEPROM_H_
#define	_EEPROM_H_

#include "schedule.h"

#ifdef	__cplusplus
extern "C" {
#endif

enum eeprom_adreses{
	ZERO_ADDR,
	F1224_ADDR,
	BRIGHT_ADDR,
	SCHEDULE_WEEKDAY1_START_ADDR,
	SCHEDULE_WEEKDAY1_END_ADDR,
	SCHEDULE_WEEKDAY2_START_ADDR,
	SCHEDULE_WEEKDAY2_END_ADDR,
	SCHEDULE_WEEKEND1_START_ADDR,
	SCHEDULE_WEEKEND1_END_ADDR,
	SCHEDULE_WEEKEND2_START_ADDR,
	SCHEDULE_WEEKEND2_END_ADDR,
	R_ADDR,
	G_ADDR,
	B_ADDR,
	ANTIPOISONING_EFFECT_ADDR,
	RGB_EN_ADDR,
	COLON_BLINKING_TYPE,
	CONFIG_VERSION_ADDR
};

enum eeprom_constants{
	EEPROM_CONFIG_VERSION = 1
};


typedef struct {
	uint8_t		zeroEn;
	uint8_t		f1224;
	uint8_t		bright;
	ScheduleConfig	schedule;
	uint8_t 	rgbGlobalEn;
	uint8_t		antipoisoningEffect;
	uint8_t		colonBlinkingType;
}Edata;

extern Edata e;


void EEPROM_dataInit(void);
uint8_t EEPROM_writeByte(uint16_t logAddr, uint8_t data);
uint8_t EEPROM_readByte(uint16_t logAddr);


#ifdef	__cplusplus
}
#endif

#endif	/* _EEPROM_H_ */
