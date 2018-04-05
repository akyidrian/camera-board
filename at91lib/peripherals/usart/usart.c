//usart.c
#include "usart.h"
//#include "AT91SAM7S64.h"
#include "board.h"

// Pointers
AT91PS_PIO    u_pPio    = AT91C_BASE_PIOA;
AT91PS_PMC    u_pPMC    = AT91C_BASE_PMC;
AT91PS_USART  u_pUSART0 = AT91C_BASE_US0;
AT91PS_USART  u_pUSART1 = AT91C_BASE_US1;
AT91PS_PDC    u_pPDC0   = AT91C_BASE_PDC_US0;
AT91PS_PDC    u_pPDC1   = AT91C_BASE_PDC_US1;
AT91PS_MC     u_pMC     = AT91C_BASE_MC;
AT91PS_AIC    u_pAic    = AT91C_BASE_AIC;

void InitUSART0(void)
{

  u_pPio->PIO_PDR = BIT5 | BIT6 | BIT21 | BIT22; //Disables the PIO from controlling the corresponding pin (enables peripheral control of the pin).
  u_pPio->PIO_ASR = BIT5 | BIT6 | BIT21 | BIT22; //Assigns the I/O line to the Peripheral A function.
  u_pPio->PIO_BSR = 0;                           //Assigns the I/O line to the Peripheral B function.


  //enable the clock of USART
  u_pPMC->PMC_PCER = 1<<AT91C_ID_US0;

  //set baud rate divisor register
  //u_pUSART0->US_BRGR = 313; //((48000000)/9600x16)
  u_pUSART0->US_BRGR = 78; //((48000000)/38400x16) cvm

  //write the Timeguard Register
  u_pUSART0->US_TTGR = 0;

  //Set the USART mode
  u_pUSART0->US_MR = 0x08c0;

  //Enable the RX and TX PDC transfer requests
  u_pPDC0->PDC_PTCR = AT91C_PDC_TXTEN | AT91C_PDC_RXTEN;

  //Enable usart
  u_pUSART0->US_CR = 0x50;

}

void InitUSART1(void)
{

  u_pPio->PIO_PDR = BIT5 | BIT6 | BIT21 | BIT22; //Disables the PIO from controlling the corresponding pin (enables peripheral control of the pin).
  u_pPio->PIO_ASR = BIT5 | BIT6 | BIT21 | BIT22; //Assigns the I/O line to the Peripheral A function.
  u_pPio->PIO_BSR = 0;                           //Assigns the I/O line to the Peripheral B function.


  //enable the clock of USART
  u_pPMC->PMC_PCER = 1<<AT91C_ID_US1;

  //set baud rate divisor register
  //u_pUSART1->US_BRGR = 313; //((48000000)/9600x16)
  u_pUSART1->US_BRGR = 78; //((48000000)/9600x16)

  //write the Timeguard Register
  u_pUSART1->US_TTGR = 0;

  //Set the USART mode
  u_pUSART1->US_MR = 0x08c0;

  //Enable the RX and TX PDC transfer requests
  u_pPDC1->PDC_PTCR = AT91C_PDC_TXTEN | AT91C_PDC_RXTEN;

  //Enable usart
  u_pUSART1->US_CR = 0x50;

}

//write char to UART0
void write_char_USART0(unsigned char ch)
{
  while (!(u_pUSART0->US_CSR&AT91C_US_TXRDY)==1);
  u_pUSART0->US_THR = ((ch & 0x1FF));
}

//read char from UART0
unsigned char read_char_USART0(void)
{
  while (!(u_pUSART0->US_CSR&AT91C_US_RXRDY)==1);
  return((u_pUSART0->US_RHR) & 0x1FF);
}

//write char from UART0 (non stop)
unsigned char read_char_USART0_nonstop(void)
{
  if((u_pUSART0->US_CSR&AT91C_US_RXRDY)==1)
    return((u_pUSART0->US_RHR) & 0x1FF);
  else return 0;
}

//write char to UART1
void write_char_USART1(unsigned char ch)
{
  while (!(u_pUSART1->US_CSR&AT91C_US_TXRDY)==1);
  u_pUSART1->US_THR = ((ch & 0x1FF));
}

//read char from UART1
unsigned char read_char_USART1(void)
{
  while (!(u_pUSART1->US_CSR&AT91C_US_RXRDY)==1);
  return((u_pUSART1->US_RHR) & 0x1FF);
}

//read char from UART1 (non stop)
unsigned char read_char_USART1_nonstop(void)
{
  if ((u_pUSART1->US_CSR&AT91C_US_RXRDY)==1)
    return((u_pUSART1->US_RHR) & 0x1FF);
  else
    return 0;
}


void write_str_USART0(unsigned char* buff) {

  unsigned int i = 0x0;

  while(buff[i] != '\0') {
    write_char_USART0(buff[i]);
    i++;
  }

}


void write_str_USART1(unsigned char* buff) {

  unsigned int i = 0x0;

  while(buff[i] != '\0') {
    write_char_USART1(buff[i]);
    i++;
  }
}


