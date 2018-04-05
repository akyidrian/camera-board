// #####################################
// File     : camera.h
// Author   : Aydin Arik and Chris Meaclem
// Date     : 29/07/2011
// Release  : Intermediate
// Purpose  : Provides an interface for the Linksprite Y201 Camera
// Notes    : Review LS-Y201 Datasheet for CMD's and etc as reference.
// #####################################
#ifndef __CAMERA_H__
#define __CAMERA_H__

/*Includes:*/
#include "boolean.h"
#include <usart/usart.h>
#include <led/led.h>
#include <mmc/mmc.h>

enum successFlag {SUCCESS = 0, NOT_SUCCESS};
enum baudRates {baud_9600 = 0, baud_19200, baud_38400, baud_57600, baud_115200};

/*END - Includes:*/
/*Definitions:*/

//Camera Commands:
	#define CMD_RESET 			{0x56, 0x00, 0x26, 0x00}
	#define CMD_RESET_LEN			4
	#define CMD_TAKE_PICTURE 	{0x56, 0x00, 0x36, 0x01, 0x00}
	#define CMD_TAKE_PICTURE_LEN	5
	#define CMD_GET_SIZE  		{0x56, 0x00, 0x34, 0x01, 0x00}
	#define CMD_GET_SIZE_LEN		5
	#define CMD_STOP_PICTURE 	{0x56, 0x00, 0x36, 0x01, 0x03}
	#define CMD_STOP_PICTURE_LEN	5
	
//Camera Command Recieved 
	#define CMD_RESET_RETURN 	{0x76, 0x00, 0x26, 0x00}
	#define CMD_RESET_RETURN_LEN 4
	#define CMD_TAKE_PICTURE_RETURN	{0x76, 0x00, 0x36, 0x00, 0x00}
	#define CMD_TAKE_PICTURE_RETURN_LEN	5
	#define CMD_STOP_PICTURE_RETURN {0x76, 0x00, 0x36, 0x00, 0x00}
	#define CMD_STOP_PICTURE_RETURN_LEN 5
	#define CMD_GET_SIZE_RETURN {0x76, 0x00, 0x34, 0x00, 0x04, 0x00, 0x00}
		//x1, x2: Length of the picture file.
		//Has two chars on end. Len = 7+2=9
		#define CMD_GET_SIZE_RETURN_LEN	9
		
	#define CMD_SET_COMPRESSION_RETURN {0x76, 0x00, 0x31, 0x00, 0x00}
	#define CMD_SET_COMPRESSION_RETURN_LEN	5
	
	#define CMD_1_SET_PIC_SIZE_RETURN {0x76, 0x00, 0x31, 0x00, 0x00}
	#define CMD_1_SET_PIC_SIZE_RETURN_LEN	5
	#define CMD_2_SET_PIC_SIZE_RETURN {0x76, 0x00, 0x54, 0x00, 0x00}
	#define CMD_2_SET_PIC_SIZE_RETURN_LEN	5
	#define CMD_PWR_SAVING_RETURN {0x76, 0x00, 0x3E, 0x00, 0x00}
	#define CMD_PWR_SAVING_RETURN_LEN 5
	#define CMD_SET_BAUD_RETURN {0x76, 0x00, 0x24, 0x00, 0x00}
	#define CMD_SET_BAUD_RETURN_LEN 5
	#define CMD_GET_DATA_RETURN {0x76, 0x00, 0x32, 0x00, 0x00}//THIS MAY NOT BE COMPLETELY RIGHT.
	

//Partial Camera Commands (Require additional dada to be sent at end):
	#define CMD_GET_DATA(mh,ml,kh,kl,x1,x2) \
		{0x56, 0x00, 0x32, 0x0C, 0x00, 0x0A, 0x00, 0x00, mh, ml, 0x00, 0x00, kh, kl, x1, x2}
	#define CMD_GET_DATA_LEN 16
		//mh,ml = address. kh,kl = length. x1,x2 = interval time
	#define CMD_SET_COMPRESSION(x) \
		{0x56, 0x00, 0x31, 0x05, 0x01, 0x01, 0x12, 0x04, x}
		//x = compression ratio, 0->FF
	#define CMD_SET_COMPRESSION_LEN 9
	#define CMD_1_SET_PIC_SIZE(x)	{0x56, 0x00, 0x31, 0x05, 0x04, 0x01, 0x00, 0x19, x}
		//x = 0x00 (640*480), 0x11 (320*240), 0x22 (160*120), 
		//NON-VOLATILE: WHEN IT'S SET IT STAYS REGARDLESS OF POWER OFF.
		//NEED TO PERFORM RESET AFTER USING THIS FOR CHANGES TO HAPPEN.
	#define CMD_2_SET_PIC_SIZE(x)	{0x56, 0x00, 0x54, 0x01, x}
		//x = 0x00 (640*480), 0x11 (320*240), 0x22 (160*120)
		//VOLATILE: CHANGES RETURN TO 320*240 BY DEFAULT ON POWER OFF.
	#define CMD_2_SET_PIC_SIZE_LEN 5
	#define CMD_PWR_SAVING(x)	{0x56, 0x00, 0x3E, 0x03, 0x00, 0x01, x}
		//x = 0x00 or 0x01
	#define CMD_PWR_SAVING_LEN	7
	#define CMD_SET_BAUD(x,y) {0x56, 0x00, 0x24, 0x03, 0x01, x, y}
		//x, y See datasheet for values 
	#define CMD_SET_BAUD_LEN 7
	
	
#define CMD_INIT_RETURN {0x36, 0x32, 0x35, 0x0D, 0x0A, 0x49, 0x6E, 0x69, 0x74, 0x20, 0x65, 0x6E, 0x64, 0x0D, 0x0A}
#define CMD_INIT_RETURN_LEN 15

#define BLOCK_SIZE 32
	//The size of data to read in from camera. EG 32 bits
#define INTERVAL_TIME {0x00, 0x0A}
	//Data transfer speed = INTERVAL_TIME*0.01m seconds. 0x000A is typical (Datasheet)
	
	
#define GET_MSBYTE(x) (char)(x>>8)
#define GET_LSBYTE(x) (char)(x&0xFF)


/*END - Definitions:*/
/*Prototypes:*/
int camera_reset(void);
int camera_takePicture(void);
void camera_sendCommand(char* cmd, int length);
int camera_readJPEGSize(void);
int camera_stopPicture(void);
int confirmReturn (char *returnValue, int size);
void getCamResponse(char* data, int size, char *returnValue);
int getJpegSize (char *returnValue, int size, char* data);
int camera_getPicture(void);

/*END - Prototypes:*/
/*Globals:*/


/*END - Globals:*/

#endif // #ifndef __CAMERA_H__

