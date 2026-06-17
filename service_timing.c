#include "STM8S003F3.h"
#include "service_timing.h"
#include "ds3231.h"
#include "keyboard.h"
#include "keyevents.h"
#include "macro.h"
#include "iface.h"

extern volatile uint8_t flag10ms;
static uint8_t mutex = 0;

static void timer20msProc(void);

void service_timing_init (void)
{
	
}

void set_service_timing_mutex (void)
{
	mutex = 1;
}

void reset_service_timing_mutex (void)
{
	mutex = 0;
}

void service_timing_10ms_proc (void) 
{
	static uint8_t quere10ms = 0;
	
	if (flag10ms){
		if(!mutex){
			quere10ms++;
			
			switch(quere10ms){
				case 1:	break;
				case 2: iface_10ms_proc_en();		break;
				case 3: timer20msProc(); 
					flag10ms = 0;
					quere10ms = 0;
					break;
					
				default:
					flag10ms = 0;
					quere10ms = 0;
				break;
			}
		}
	}
}

static void timer20msProc(void)
{
	static uint8_t counter20ms;
	if (++counter20ms > (2-1)){
		counter20ms = 0;
		
		keyboard_refresh();
	}
}
