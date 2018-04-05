
//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include <board.h>
#include <pio/pio.h>
#include <aic/aic.h> 
#include <pio/pio_it.h> 
#include <led/led.h>
#include <spi/spi.h>
#include "boolean.h"
#include <mmc/mmc.h>
#include <usart/usart.h>
#include "camera.h"

//#define DEBUG


//------------------------------------------------------------------------------
//         Local definitions
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//         Local variables
//------------------------------------------------------------------------------

volatile unsigned int InterruptComms = false;

/// Pio pins to configure.
Pin int_pins[] = {
    PIN_COMMS_INT
    };
	
 Pin commsCanReadPin[] = {
	 SEND_COMMS_INT_PIN
 };
	
 #define DELAY 1000000 

AT91PS_PIO    m_pPio   = AT91C_BASE_PIOA;//Pointer to PIO
bool haveSPI = false;//Indicates if we have control of SPI or not.
extern char mmc_buffer[BUFFER_SIZE];// external buffer which is use to read/write in MMC card
extern AT91PS_SPI s_pSpi;

enum states{CAM_INIT_WAIT = 0, IDLE, TAKE_PHOTO, COMMS_CAN_READ}; //TAKE_PHOTO, etc

enum sdState {Present, Locked, NotPresent} sdState;//States for the SD card
	//Present = Inserted and not locked.
	//Locked = Inserted and locked.
	//NotPresent = Not Present. Dont care about lock.
typedef enum {
	redOn = 1,
	redOff,
	orangeOn,
	orangeOff,
	yellowOn,
	yellowOff,
	greenOn,
	greenOff
} cmd;

//------------------------------------------------------------------------------
/// FXN PROTOTYPES
//------------------------------------------------------------------------------
static void ISR_COMMS(void);//Interrupt Handler
void init(void);//Init the SAM7
int InitSD(void);//Init the SD card and associated periperials
void getSDState(void);
void heartbeat(void);//Heartbeat LED
void SPIControl(bool getControl);//Handles SPI control events
void sendPicture(int pictureStartPstn);//Sends pic stored on SD to comms board.
//------------------------------------------------------------------------------

static void ISR_COMMS(void)
{
    PIO_DisableIt(&int_pins[0]);//Disable interrupts
    InterruptComms = true;//Set Flag
    PIO_EnableIt(&int_pins[0]);//enable interrupts.
}

void init(void)
{
	PIO_Configure(int_pins, PIO_LISTSIZE(int_pins)); // Configure PA19 and PA20 as Peripheral B.
    
	ledInit();
    
	//Setup Interrupts:
		PIO_InitializeInterrupts(AT91C_AIC_PRIOR_HIGHEST | AT91C_AIC_SRCTYPE_POSITIVE_EDGE);
		PIO_ConfigureIt(&int_pins[0], (void (*)(const Pin *)) ISR_COMMS);
		PIO_EnableIt(&int_pins[0]);
		
		PIO_Configure(commsCanReadPin, PIO_LISTSIZE(commsCanReadPin));
		PIO_Clear(&commsCanReadPin[0]);
		
	InitUSART0();//initialising camera communications (UART)
	InitUSART1();//initialising Comms board communications (UART)
	
	SPIControl(true);//Notify comms we are taking control of SPI bus
	//Initialising sd card and doing a few checks.
	if(InitSD()) { ledSet(LED_RED); }//Indicate error with SD Card.
}

int InitSD(void)
{
	sdState = NotPresent;//Init state to not present.
	getSDState();
	
	Init_CP_WP();
	if (initMMC() == MMC_SUCCESS)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

void getSDState(void)
{
	int isLocked = 0, isPresent = 0;
	//wp:
	if(((m_pPio->PIO_PDSR) & BIT15)) { isLocked = 1;  }//High when locked
	//cd:
	if(!((m_pPio->PIO_PDSR) & BIT16)) { isPresent = 1; }//CD is low when chip present
	
	//Set state:
	if(!isPresent)//if not present
	{
		sdState = NotPresent;
	}
	else//Chip is inserted. Is it locked or not?
	{
		if(isLocked)
		{
			sdState = Locked;
		}
		else
		{
			sdState = Present;
		}
	}
	
}

void heartbeat(void)
{
	static int status = 0;//represent the status of the heartbeat led.
	if(status) 
	{ 
		ledClear(LED0); 
		status = 0;
	}
	else 
	{ 
		ledSet(LED0);
		status = 1;
	}
}

void SPIControl(bool getControl)
{
	if(getControl)
	{
		PIO_Clear(&commsCanReadPin[0]);//Notify comms we are taking control of SPI bus
		if(!haveSPI)
		{
			//setMasterMode();
			//InitSD();
			initMMC();
			haveSPI = true;
		}
	}
	else
	{
		if(haveSPI)
		{
			//setSlaveMode();
			haveSPI = false;
			PIO_Set(&commsCanReadPin[0]);//Notify comms we are ready to take photo and not using SPI
		}
	}
	
}


void sendPicture(int pictureStartPstn)
{
	bool endFlag = false;//break out of loop if we detect end of image. DDF9
	//blockPstn->sdBlock->Whole Image.
	int blockPstn = 0;//The pstn in the SD block. Range [0->511]
	int sdBlock = 0;//The SD Block, as a part of the total image. Increases until we detect FFD9      

	while(!endFlag)//While waiting for whole image. Get data till FF D9. Starts with FF D8
	{
		mmcReadBlock((sdBlock+pictureStartPstn)*BUFFER_SIZE, BUFFER_SIZE);//Read the block.

		while(blockPstn<BUFFER_SIZE)
		{	
			write_char_USART1(mmc_buffer[blockPstn]);
			
			//Check for end of file:
			if(blockPstn>1)//Prevent index issues.
			{
				if(mmc_buffer[blockPstn] == 0xD9 && mmc_buffer[blockPstn-1] == 0xFF)
				{
					endFlag = true;//break if we are at end of file.
					break;
				}
			}
			
			blockPstn++;//Progress to next char in block.
		}
		blockPstn = 0;//Begin next loop at start of new block.
		sdBlock++;//Progress to net block.
	}
}

void sendPictureLimitedSize(void)
{
//Sends only four blocks followed by FFD9
//Total Bytes = 2050 = 4*512 + 2

	bool endFlag = false;//break out of loop if we detect end of image. DDF9
	//blockPstn->sdBlock->Whole Image.
	int blockPstn = 0;//The pstn in the SD block. Range [0->511]
	int sdBlock = 0;//The SD Block, as a part of the total image. Increases until we detect FFD9      

	while(!endFlag)//While waiting for whole image. Get data till FF D9. Starts with FF D8
	{
		mmcReadBlock(sdBlock*BUFFER_SIZE, BUFFER_SIZE);//Read the block.

		while(blockPstn<BUFFER_SIZE)
		{	
			write_char_USART1(mmc_buffer[blockPstn]);
			
			//Small wait between bytes
			volatile int w = 0;
			while (w < 100) {
				w++;
			}
			
			if(sdBlock>3)//Break when we have read out enough blocks.
			{
				write_char_USART1(0xFF);
				write_char_USART1(0xD9);
				endFlag = true;//break if we are at end of file.
				break;
			}
			
			blockPstn++;//Progress to next char in block.
		}
		blockPstn = 0;//Begin next loop at start of new block.
		sdBlock++;//Progress to net block.
		
		//Wait between blocks.
		volatile int w = 0;
		while (w < 1000) {
			w++;
		}
	}
}

/*
Untested.
A fxn to send the picture from SD to Comms via USART
*/
/*
void sendPicture(void)
{
	bool endFlag = false;
	int blockPstn = 0;//The pstn in the SD block
	int sdBlock = 0;//The SD Block         

	int i = 0; 
	while(!endFlag)//While waiting for whole image. Get data till FF D9. Starts with FF D8
	{
		mmcReadBlock(i*BUFFER_SIZE, BUFFER_SIZE);//Read the block.
		
		//mmcWriteBlock((i+26)*BUFFER_SIZE);
		
		
		while(blockPstn<(BUFFER_SIZE))
		{	
			write_char_USART1(mmc_buffer[blockPstn]);
			blockPstn++;//Progress to next char in block.
			
			volatile int w = 0;
			while (w < 100) {
				w++;
			}
		}
		blockPstn = 0;
		volatile int w = 0;
		while (w < 1000) {
			w++;
		}
		
		
		//i++;//Read the next block.
		//if(i>3)
		//if(i>25) 
		//{
		//	write_char_USART1(0xff);
		//	write_char_USART1(0xd9);
		//	break;
		//}
		
		//Check for end of file:
			if(blockPstn>1)//Prevent index issues.
			{
				if(mmc_buffer[blockPstn] == 0xD9 && mmc_buffer[blockPstn-1] == 0xFF)
				{
					endFlag = true;//break if we are at end of file.
					//break;
				}
			}
			
	}
	
	//Receive echo from comms board and write back to SD card at offset (26)
	//
	//int k = 0;
	//while(k<512)
	//{
	//	mmc_buffer[k] = read_char_USART1();
	//	k++;
	//}
	//mmcWriteBlock(26*BUFFER_SIZE);
	//ledSet(LED_ORANGE);
	//
}*/
/*void sendPicture(void)
{
	bool endFlag = false;
	int blockPstn = 0;//The pstn in the SD block
	int sdBlock = 0;//The SD Block          
	while(!endFlag)//While waiting for whole image. Get data till FF D9. Starts with FF D8
	{
		mmcReadBlock(sdBlock*BUFFER_SIZE, BUFFER_SIZE);//Read the block.
		
		//Transfer block
		while(blockPstn<(BUFFER_SIZE))
		{	
			//write_char_USART1(mmc_buffer[blockPstn]);
			mmcWriteBlock((sdBlock+26)*BUFFER_SIZE);
			blockPstn++;//Progress to next char in block.
			
			volatile int w = 0;
			while (w < 1000) {
				w++;
			}
			
			
			//Check for end of file:
			if(blockPstn>1)//Prevent index issues.
			{
				if(mmc_buffer[blockPstn] == 0xD9 && mmc_buffer[blockPstn-1] == 0xFF)
				{
					endFlag = true;//break if we are at end of file.
					break;
				}
			}
		}
		
		blockPstn = 0; //back to the start of the next new block.
		sdBlock++;//Read the next block.
	}
}*/

 int main(void)
 {
	init();//Initialising leds and interupts
 	//setHighImpedance();
	 
	 volatile int wait=0; //TODO remove this...
	 int state = CAM_INIT_WAIT; 
     while(1)//INFINITE LOOP.
     {
		
		if(sdState != Present) { ledSet(LED_RED); }//Indicate SD card error with LED3.
		 // Go into the desired state.
		 int pictureStartPstn = 0;
		 switch (state)
		 {
		  case CAM_INIT_WAIT:
			//Runs once at reset. Delays startup so CAMERA can initilise.
			state = IDLE;
			
			SPIControl(true);//Notify comms we are taking control of SPI bus
			
			#ifndef DEBUG
				//Flash lights when not in debug mode.
				volatile int delayTime1 = 0;
				volatile int delayTime = 0;
				while (delayTime < DELAY)
				{
					ledFlash();//Flash LED;s to show we are in startup sequence.
					while (delayTime1 < 0.1*DELAY)
					{
						delayTime1++;
					}
					delayTime+= delayTime1;//approximately 2 second, most likely more.
					delayTime1 = 0;
				}
				delayTime = 0;
			#endif
			ledSetAll(0);//Clear all LED's
			break;
			
		 case IDLE: //Waiting for comms board to say they want to take a photo...
			SPIControl(false);
			
			if (InterruptComms == true)//Received interrupt to take photo.
			{
				state = TAKE_PHOTO;//Next state after interrupt is to take photo.
				ledSet(LED_ORANGE);
				InterruptComms = false; //reset interrupt flag for next future interupts.
			}
			
			break;
			
		  case TAKE_PHOTO:
			//Assume Comms have disabled their SPI peripheral.
			SPIControl(true);//Notify comms we are taking control of SPI bus
			ledClear(LED_ORANGE);
			pictureStartPstn = camera_getPicture();//Get pic from camera and store on SD card
			sendPicture(pictureStartPstn);//Send pic on SD card to comms board.
			
			state = COMMS_CAN_READ;//Done with SPI and photo taken
			break;
			
		  case COMMS_CAN_READ://comms must realise they cannot ask us to take a photo and read from the card at the same time.
			SPIControl(false);//Hand control of SPI back to Comms
			state = IDLE;//After telling comms they can read, wait in IDLE state.
			break;
		 }
		 
		heartbeat();//Flash heartbeat LED.
		wait=0;
		while(wait<DELAY)	wait++;
     }

	 return 0;
 }
