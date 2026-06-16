#include "STM8S003F3.h"
#include "hc595.h"

// HC595 pin definitions
#define CLOCK_PIN   						sfr_PORTD.ODR.ODR2   // Pin 19 (SRCLK)
#define LATCH_PIN   						sfr_PORTC.ODR.ODR7   // Pin 17 (RCLK)
#define	OE_PIN      						sfr_PORTD.ODR.ODR3   // Pin 20 (OE)
#define DATA_PIN    						sfr_PORTC.ODR.ODR5   // Pin 15 (PC5, DATA connected to DS of the first 74HC595)

void hc595Init (void) 
{
	// Initialize GPIOs for the pins used with the 74HC595

	CLOCK_PIN = 0;
	LATCH_PIN = 0;
	OE_PIN = 1;
	DATA_PIN = 0;
	
	// configure CLOCK_PIN as output
	sfr_PORTD.DDR.DDR2 = 1;     // input(=0) or output(=1)
	sfr_PORTD.CR1.C12  = 1;     // input: 0=float, 1=pull-up; output: 0=open-drain, 1=push-pull
	sfr_PORTD.CR2.C22  = 1;     // input: 0=no exint, 1=exint; output: 0=2MHz slope, 1=10MHz slope
  
  // configure LATCH_PIN as output
	sfr_PORTC.DDR.DDR7 = 1;     // input(=0) or output(=1)
	sfr_PORTC.CR1.C17  = 1;     // input: 0=float, 1=pull-up; output: 0=open-drain, 1=push-pull
	sfr_PORTC.CR2.C27  = 1;     // input: 0=no exint, 1=exint; output: 0=2MHz slope, 1=10MHz slope
		
	// configure OE_PIN as output
	sfr_PORTD.DDR.DDR3 = 1;     // input(=0) or output(=1)
	sfr_PORTD.CR1.C13  = 1;     // input: 0=float, 1=pull-up; output: 0=open-drain, 1=push-pull
	sfr_PORTD.CR2.C23  = 1;     // input: 0=no exint, 1=exint; output: 0=2MHz slope, 1=10MHz slope	
		
	// configure DATA_PIN as output
	sfr_PORTC.DDR.DDR5 = 1;     // input(=0) or output(=1)
	sfr_PORTC.CR1.C15  = 1;     // input: 0=float, 1=pull-up; output: 0=open-drain, 1=push-pull
	sfr_PORTC.CR2.C25  = 1;     // input: 0=no exint, 1=exint; output: 0=2MHz slope, 1=10MHz slope	
	
	hc595OutputDisable();
}

void hc595OutputDisable (void)
{
	OE_PIN = 1;
}

void hc595OutputEnable (void)
{
	// enable output for the first 595 in chain, all other 595 OE inputs is on ground.
	OE_PIN = 0;
}
/*
здесь нужно посмотреть на быстродействие этой фунции в прерывании,
и если нужно, то можно для ускорения передачи данных сделать 5 * 8 бит ( т.е по байту)
но потеряем в размере функции которая будет перекодировать 10 бит в 16-битном числе в конкретные биты в этом массиве.
Т.е. изначально будет массив 4 элемента на 10 бит ( 16 битное число) для отображение каждой лампы
которое будет мапится на массив 5 элемент на 8 бит ( 8 битное число) функция довольно объёмная. 
*/
/*
void hc595ChainShiftOut (uint16_t *data, uint8_t length) 
{
	uint8_t	r,b;
	
	for (r = 0; r < length; r++){
		for (b = 0; b < 10; b++) {
			// Write the MSB (bit 7) to the DATA pin
			if (*data & 0x1) {
					DATA_PIN = 1;
			} else {
					DATA_PIN = 0;
			}
			// Pulse the SRCLK (Shift Clock)
			CLOCK_PIN = 1;
			CLOCK_PIN = 0;
			// Shift to the next bit
			*data >>= 1;
		}
		data++;
	}
	LATCH_PIN = 1;
	LATCH_PIN = 0;
}
*/
void hc595ChainShiftOut (uint8_t *data, uint8_t length) 
{
	uint8_t	r,b,t;
	
	for (r = 0; r < length; r++){
		t = *data++;
		for (b = 0; b < 8; b++) {
			// Write the MSB (bit 7) to the DATA pin
			if (t & 0x80) {
					DATA_PIN = 1;
			} else {
					DATA_PIN = 0;
			}
			// Pulse the SRCLK (Shift Clock)
			CLOCK_PIN = 1;
			t <<= 1;
			CLOCK_PIN = 0;
			// Shift to the next bit
		}
	}
	LATCH_PIN = 1;
	LATCH_PIN = 0;
}
