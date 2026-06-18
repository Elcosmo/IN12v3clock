#ifndef _IFACE_H_
#define	_IFACE_H_

#ifdef	__cplusplus
extern "C" {
#endif

enum setup_states{
	SETUP_ZERO,
	SETUP_F1224,
	SETUP_BRIGHT,
	SETUP_WEEKDAY1_START,
	SETUP_WEEKDAY1_END,
	SETUP_WEEKDAY2_START,
	SETUP_WEEKDAY2_END,
	SETUP_WEEKEND1_START,
	SETUP_WEEKEND1_END,
	SETUP_WEEKEND2_START,
	SETUP_WEEKEND2_END,
	SETUP_COLON_BLINKING_TYPE,
	SETUP_WEEKDAY,
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
	uint8_t		weekday;
	uint8_t		rtcValid;
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
void iface_start_antipoisoning(void);
void RGBtoggle ( void );

#ifdef	__cplusplus
}
#endif

#endif	/* _IFACE_H_ */
