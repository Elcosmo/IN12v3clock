#ifndef _IFACE_H_
#define	_IFACE_H_

#ifdef	__cplusplus
extern "C" {
#endif

enum setup_states{
	SETUP_ZERO,
	SETUP_F1224,
	SETUP_BRIGHT,
	SETUP_NIGHT_BR,
	SETUP_NIGHT_BR_EN,
	SETUP_NIGHT_BR_START_H,
	SETUP_NIGHT_BR_START_M,
	SETUP_NIGHT_BR_END_H,
	SETUP_NIGHT_BR_END_M,
	SETUP_NIGHT_RGB_EN,
	SETUP_ANTIPOISONING_AT_NIGHT_ONLY,
	SETUP_COLON_BLINKING_TYPE,
	SETUP_R,
	SETUP_G,
	SETUP_B,
	SETUP_HOURS,
	SETUP_MINUTES,
	SETUP_NO = 255
};

typedef struct  
{
	uint16_t 	timeSetupCounter;
	uint8_t		display[4];
	uint8_t		flag10ms;
	uint8_t		display_state;
	uint8_t		seconds;
	uint8_t		minutes;
	uint8_t		hours;
	uint8_t		setupValue;
	uint8_t 	minutesSetupValue;
	uint8_t 	hoursSetupValue;
	uint8_t 	counter1s;
	uint8_t		flag05s;
	uint8_t		flag100ms;
	uint8_t		minutesOld;
	uint8_t		hoursOld;
	uint8_t		apOld[4];
	uint8_t		apNew[4];
	uint8_t		apFlagEn[4];
	uint8_t		counterAp100ms;
	uint8_t 	antipoisoningEn;
	uint8_t 	antipoisoningOldDigit;
	uint8_t 	antipoisoningCurrentDigit;
}Iface;


void iface_init( void );
void iface_proc ( void ) ;
void iface_10ms_proc_en (void);
void iface_flag05sReset(void);
uint8_t bcd_to_decimal(uint8_t bcd);
uint8_t decimal_to_bcd(uint8_t decimal);
uint16_t time_to_minutes(uint8_t hour, uint8_t minute);
uint8_t is_time_in_interval(uint16_t current_minutes, uint16_t start_minutes, uint16_t end_minutes);
void iface_start_antipoisoning(void);
void RGBtoggle ( void );

#ifdef	__cplusplus
}
#endif

#endif	/* _IFACE_H_ */