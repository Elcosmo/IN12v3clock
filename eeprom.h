#ifndef _EEPROM_H_
#define	_EEPROM_H_

#ifdef	__cplusplus
extern "C" {
#endif

enum eeprom_adreses{
	ZERO_ADDR,
	F1224_ADDR,
	BRIGHT_ADDR,
	NIGHT_BR_ADDR,
	NIGHT_BR_EN_ADDR,
	NIGHT_BR_START_H_ADDR,
	NIGHT_BR_START_M_ADDR,
	NIGHT_BR_STOP_H_ADDR,
	NIGHT_BR_STOP_M_ADDR,
	NIGHT_RGB_EN_ADDR,
	ANTIPOISONING_AT_NIGHT_ONLY_ADDR,
	R_ADDR,
	G_ADDR,
	B_ADDR,
	ANTIPOISONING_EFFECT_ADDR,
	RGB_EN_ADDR,
	COLON_BLINKING_TYPE
};


typedef struct {
	uint8_t		zeroEn;
	uint8_t		f1224;
	uint8_t		bright;
	uint8_t		nBright;
	uint8_t		nBrightEn;
	uint8_t		nBrightStartH;
	uint8_t		nBrightStartM;
	uint8_t		nBrightEndH;
	uint8_t		nBrightEndM;
	uint8_t		rgbAtNightEn;
	uint8_t		antipoisoningAtNihgtOnly;
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