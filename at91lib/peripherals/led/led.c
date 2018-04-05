#include "led.h"

#define INIT_STATE 0
	//Define the initial state for the LED's. 0=off
#define NUM_LEDS 4
	//Num of LED's used by the board.

Pin board_leds[] = {
    PIN_LED_BOARD6,
	PIN_LED_BOARD5,
	PIN_LED_BOARD4,
	PIN_LED_BOARD3
    };

void ledInit(void)
{
	PIO_Configure(board_leds, PIO_LISTSIZE(board_leds)); // Configure LED's
	
	int i = 0;
	for(i=0; i<NUM_LEDS; i++)
	{
		#if INIT_STATE == 0
			ledClear(LED0);
			ledClear(LED1);
			ledClear(LED2);
			ledClear(LED3);
		#else
			ledSet(LED0);
			ledSet(LED1);
			ledSet(LED2);
			ledSet(LED3);
		#endif
	}
	
}

void ledSet(int value)
{
	PIO_Set(&board_leds[value]);
}

void ledClear(int value)
{
	PIO_Clear(&board_leds[value]);
}

void ledSetAll(int state)
{
	if(state)
	{
		ledSet(0);
		ledSet(1);
		ledSet(2);
		ledSet(3);
	}
	else
	{
		ledClear(0);
		ledClear(1);
		ledClear(2);
		ledClear(3);
	}
	
}

void ledFlash(void)//Call repeatidly for flashing.
{
	static int led = 0;//LED Counter.
	ledClear(led);
	led++;
	if(led>3)
	{
		led = 0;
	}
	
	ledSet(led);
}
