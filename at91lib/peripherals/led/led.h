#ifndef LED_H_
#define LED_H_

#include <pio/pio.h>
#include <board.h>

#define LED0 0
#define LED1 1
#define LED2 2
#define LED3 3

#define LED_GREEN 0
#define LED_ORANGE 1
#define LED_YELLOW 2
#define LED_RED 3

void ledInit(void);

void ledSet(int value);

void ledClear(int value);

void ledSetAll(int state);

void ledFlash(void);

#endif /* LED_H_ */

