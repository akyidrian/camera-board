// #####################################
// File     : camera.c
// Author   : Aydin Arik and Chris Meaclem
// Date     : 
// Release  : Intermediate
// Purpose  : 
// Notes    : 
// #####################################

/*Includes:*/
#include "camera.h"

#define READ_MASK 0xFF
	//The mask for reading data from the USART. use 0x1FF to include parity.

/*END - Includes:*/
/*Definitions:*/


/*END - Definitions:*/
/*Prototypes:*/

/*END - Prototypes*/
/*Globals:*/
extern char mmc_buffer[BUFFER_SIZE];// external buffer which is use to read/write in MMC card
unsigned int imgOffset = 0;//The block offset for writing multiple images to the SD
unsigned char pictureNumber = 0;

/*END - Globals:*/


int camera_reset(void) //Reset the camera
{
	char cmd[] = CMD_RESET;
	camera_sendCommand(cmd, CMD_RESET_LEN);
	
	//Check return then:
	char rtnValue[] = CMD_RESET_RETURN;
	bool success = confirmReturn(rtnValue, CMD_RESET_RETURN_LEN);
	if(success)		
		return SUCCESS;
	else	
		return NOT_SUCCESS;
}

int camera_takePicture(void)
{
	char cmd[] = CMD_TAKE_PICTURE;
	camera_sendCommand(cmd, CMD_TAKE_PICTURE_LEN);
	
	//Check return then:
	char rtnValue[] = CMD_TAKE_PICTURE_RETURN;
	bool success = confirmReturn(rtnValue, CMD_TAKE_PICTURE_RETURN_LEN);
	if(success)
		return SUCCESS;
	else	
		return NOT_SUCCESS;
}

void camera_sendCommand(char* cmd, int length) //Send command to camera
{
	int i;
	for(i=0; i<length; i++)
	{
		write_char_USART0(cmd[i]);
	}
}


int camera_readJPEGSize(void)
{
	char cmd[] = CMD_GET_SIZE;
	camera_sendCommand(cmd, CMD_GET_SIZE_LEN);
	
	//Check return then:
	char rtnValue[] = CMD_GET_SIZE_RETURN;
	char data[2] = {0};//cvm initilised data to zero
	getJpegSize (rtnValue, CMD_GET_SIZE_RETURN_LEN, data);
	int size = (data[0]<<8)+data[1]; //0xXXYY, where XX is data[0] and YY is data[1]
	return size;
	/*bool success = true;
	//bool success = confirmReturn(rtnValue, CMD_TAKE_PICTURE_RETURN_LEN);
	if(success)	
		
		return SUCCESS;
	else	
		return NOT_SUCCESS;*/
}

int camera_stopPicture(void)
{
	char cmd[] = CMD_STOP_PICTURE;
	camera_sendCommand(cmd, CMD_STOP_PICTURE_LEN);
	
	//Check return then:
	char rtnValue[] = CMD_STOP_PICTURE_RETURN;
	bool success = confirmReturn(rtnValue, CMD_STOP_PICTURE_RETURN_LEN);
	if(success)	
		
		return SUCCESS;
	else	
		return NOT_SUCCESS;
}

int camera_setResolution(void)
{
	unsigned char size = 0x22;
	char cmd[] = CMD_2_SET_PIC_SIZE(size);//0x00 = biggest, 640*480
	camera_sendCommand(cmd, CMD_2_SET_PIC_SIZE_LEN);
	
	//Check return then:
	char rtnValue[] = CMD_2_SET_PIC_SIZE_RETURN;
	bool success = confirmReturn(rtnValue, CMD_2_SET_PIC_SIZE_RETURN_LEN);
	if(success)	
		
		return SUCCESS;
	else	
		return NOT_SUCCESS;
}

int camera_setCompression(unsigned char compression)//Bigger=More Compressed.
{
	//Typical for compression to be 0x36 (Datasheet)
	char cmd[] = CMD_SET_COMPRESSION(compression);
	camera_sendCommand(cmd, CMD_SET_COMPRESSION_LEN);
	
	//Check return then:
	char rtnValue[] = CMD_SET_COMPRESSION_RETURN;
	bool success = confirmReturn(rtnValue, CMD_SET_COMPRESSION_RETURN_LEN);
	if(success)
		return SUCCESS;
	else	
		return NOT_SUCCESS;
}

int camera_setPowerSaving(bool state)
{
	
	if(state)
	{
		char cmd[] = CMD_PWR_SAVING(0x01);//Enable
		camera_sendCommand(cmd, CMD_PWR_SAVING_LEN);
	}
	else
	{
		char cmd[] = CMD_PWR_SAVING(0x00);//Disable
		camera_sendCommand(cmd, CMD_PWR_SAVING_LEN);
	}
	
	//Check return then:
	char rtnValue[] = CMD_PWR_SAVING_RETURN;
	bool success = confirmReturn(rtnValue, CMD_PWR_SAVING_RETURN_LEN);
	if(success)
		return SUCCESS;
	else	
		return NOT_SUCCESS;
}

int camera_setBaudRate(enum baudRates rate)
{
	if(rate == baud_9600)
	{
		char cmd[] = CMD_SET_BAUD(0xAE,0xC8);
		camera_sendCommand(cmd, CMD_SET_BAUD_LEN);
	}
	else if(rate == baud_19200)
	{
		char cmd[] = CMD_SET_BAUD(0x56, 0xE4);
		camera_sendCommand(cmd, CMD_SET_BAUD_LEN);
	}
	else if(rate == baud_38400)
	{
		char cmd[] = CMD_SET_BAUD(0x2A, 0xF2);
		camera_sendCommand(cmd, CMD_SET_BAUD_LEN);
	}
	else if(rate == baud_57600)
	{
		char cmd[] = CMD_SET_BAUD(0x1C, 0x4C);
		camera_sendCommand(cmd, CMD_SET_BAUD_LEN);
	}
	else if(rate == baud_115200)
	{
		char cmd[] = CMD_SET_BAUD(0x0D, 0xA6);
		camera_sendCommand(cmd, CMD_SET_BAUD_LEN);
		camera_sendCommand(cmd, CMD_SET_BAUD_LEN);
	}
	else//Fail due to invalid argument
	{
		return NOT_SUCCESS;
	}
	
	//Check return then:
	char rtnValue[] = CMD_SET_BAUD_RETURN;
	bool success = confirmReturn(rtnValue, CMD_SET_BAUD_RETURN_LEN);
	if(success)
		return SUCCESS;
	else	
		return NOT_SUCCESS;
}

int camera_getPicture(void)
{	
	int pictureStartPstn = imgOffset;
	
	char data[BUFFER_SIZE];//Create a buffer the same size as the USART buffer
	ledSet(LED1);//Indicate we are taking photo.
	
	camera_takePicture(); 
	
	memset(&mmc_buffer,0xEE,BUFFER_SIZE);//Init USART TX buffer with a known sequence.
	memset(data,0xEE,BUFFER_SIZE);//Init USART RX buffer with a known sequence.
	
//Setup Variables
	unsigned int imgPstn = 0;//The byte pstn in the image as a part of the total size given by imgSize
	bool rxEndFlag = false;//Flag to indicate if we receive end of file(0xFFD9) from camera.
	//const unsigned char sizeHigh=GET_MSBYTE(BUFFER_SIZE), sizeLow=GET_LSBYTE(BUFFER_SIZE);//Based on the SD block size.
	const unsigned int transferBlockSize = BUFFER_SIZE;
	
	const unsigned char sizeHigh=GET_MSBYTE(transferBlockSize), sizeLow=GET_LSBYTE(transferBlockSize);//Based on the SD block size.
	
	
	unsigned int subPstn = 0;//Pstn in the received data. This is a subset of the BUFFER_SIZE.
	unsigned int numBlocks = 0;
	while(!rxEndFlag)//While waiting for whole image. Get data till FF D9. Starts with FF D8
	{
		ledSet(LED2);//Indicate we are waiting for packets.
		
		subPstn = 0;
		char result;//Data received on USART
		bool rxStartSeq = false;//A flag to indicate if we have reveived the stat of packet sequence from camera.
	//Send command to get next BUFFER_SIZE bytes from camera.
		char cmd[] = CMD_GET_DATA(GET_MSBYTE(imgPstn), GET_LSBYTE(imgPstn),sizeHigh,sizeLow,0x00,0x0A);//#define CMD_GET_DATA(mh,ml,kh,kl,x1,x2)
		camera_sendCommand(cmd, CMD_GET_DATA_LEN);
		
	//Wait for start of return then save data:
		result = READ_MASK & read_char_USART0();//mask with FF to remove parity.
		while(result != 0x76)
		{
			result = READ_MASK & read_char_USART0();//Keep refreshing until read 0x76
		}
		
		data[0] = result;//Store the start of packet at pstn 0
		subPstn = 1;//increment to one.
		
	//Continue to store rx data until we reach the end of the buffer:
		while(subPstn<(transferBlockSize))
		{	
			data[subPstn] = READ_MASK & read_char_USART0();//Store data
			
			//Remove the start bits:
			//If we have received the start sequence, move the index back to zero so to overwrite it with the JPEG data.
			if(subPstn>=4){//Must have atleast 5 chars in the buffer to have received the start sequence.
				if(data[subPstn-4] == 0x76){
					if(data[subPstn-3] == 0x00){
						if(data[subPstn-2] == 0x32){
							if(data[subPstn-1] == 0x00){
								if(data[subPstn] == 0x00){
									subPstn = 0;
									rxStartSeq = true;
			}}}}}}//Dirty but hopefully faster (less cases to check)
			
			//Check for end of file:
			if(subPstn>=1)//Need to have received atleast 2 chars first.
			{
				if(data[subPstn] == 0xD9 && data[subPstn-1] == 0xFF)
				{
					rxEndFlag = true;//break if we are at end of file.
					camera_stopPicture();
					camera_reset();
					//memset((data+subPstn+1), 0xEE, (BUFFER_SIZE-subPstn-1));
					break;
				}
			}
			
			if(rxStartSeq)//If we received the start seq we need to start at subPstn=0.
			{
				subPstn = 0;
				rxStartSeq = false;
			}
			else
			{
				subPstn++;
			}
			
			ledClear(LED2);
		}
		
	//Buffer is now full so write the black to SD
		memcpy(&mmc_buffer,data,BUFFER_SIZE);
		mmcWriteBlock(imgPstn+imgOffset*BUFFER_SIZE);
		numBlocks++;//Increment the number of blocks used in this image
		memset(data,0xEE,BUFFER_SIZE);//Reset data to a known value.

		imgPstn += BUFFER_SIZE;
	}
	
	//Make it easier to find end of file in WinHex and use time for debugging:
		char timeString[] = __TIME__;
		char endString[] = "endOfJPEG.CompiledAt:";//21
		char picNumberText[] = ". Picture Number:";//Size 17
		memset(&mmc_buffer,0xEE,BUFFER_SIZE);
		
		memcpy(&mmc_buffer,endString,21);
		memcpy(&mmc_buffer[21],timeString,8);//Length of __TIME__ is 8
		memcpy(&mmc_buffer[29],picNumberText,15);//Lengthc is 17
		memcpy(&mmc_buffer[44],(char)pictureNumber,1);//Lengthc is 8
		
		
		mmcWriteBlock(imgPstn+imgOffset*BUFFER_SIZE);
		numBlocks++;//Increment the number of blocks used in this image
		
	pictureNumber++;
	imgOffset += numBlocks;//The next address for the SD of the next photo.
	ledClear(LED1);//Finished taking photo.
	
	return pictureStartPstn;
}

int confirmReturn (char *returnValue, int size) 
{
	int i;
	char data[size];

	getCamResponse(data, size, returnValue);
	
	for (i = 0; i < size; i++)
	{
		//if data observed is not the same as the expected returnValue
		if (data[i] != returnValue[i])
		{
			return 0; //return is not the same proper return
		}
	}

	return 1; //if all return values match what is expected.
}

void getCamResponse(char* data, int size, char *returnValue)//Gets data from the return line and saved it to SD
{
	int j, i;
	for(j=0; j<size; j++)
	{
		data[j] = 0xEE;//Init data to help with debug on SD card
	}
	
	char result;
	result = READ_MASK & read_char_USART0();//mask with FF to remove parity.
	
	//Wait for the start of the return 
	while(result != returnValue[0])
	{
		result = READ_MASK & read_char_USART0();
	}
	data[0] = result;//Store the start of packet at pstn 0
	i = 1;//increment to one.
	
	//Store the remaining packets
	while(true)
	{	
		result = READ_MASK & read_char_USART0();
		data[i] = result;//Store data
	
		i++;
		if(i>(size-1))//if i is greater than the size of the expected returnValue
		{
			break;
		}
	}
}

int getJpegSize (char *returnValue, int size, char* data) //data points to an area where we can save returned data.
{
	int i;
	char dataRx[size];//All the received data from camera
	getCamResponse(dataRx, size, returnValue);
	
	data[0] = dataRx[7];
	data[1] = dataRx[8];
	
	//Validating return as the expected returnValue
	for (i = 0; i < size; i++)
	{
		if (data[i] != returnValue[i])
		{
			return 0; //return is not the same proper return
		}
	}

	return 1; //if all return values match what is expected.
}

//EOF
