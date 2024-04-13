#include <lpc17xx.h>

#include <string.h>
#define LOW 0
#define HIGH 1
void printError(const char * str);
void response(unsigned int wTime, unsigned int mar, unsigned char pinVal);
char getDataBit(void);
unsigned int time;
char data;
unsigned char temp, msg1;
unsigned int k;
unsigned int i=0, flag1, j;
#define RDR         (1<<0)
#define THRE        (1<<5) //Transmit Holding Register Empty
#define MULVAL      15
#define DIVADDVAL   2
#define Ux_FIFO_EN  (1<<0)
#define Rx_FIFO_RST (1<<1)
#define Tx_FIFO_RST (1<<2)
#define DLAB_BIT    (1<<7)
#define LINE_FEED   0x0A //LF, For Linux, MAC and Windows Terminals  
#define CARRIAGE_RETURN 0x0D //CR, For Windows Terminals (CR+LF).

#define PRESCALE (25-1)

void initUART0(void);
void initTimer0(void);
void startTimer0(void);
unsigned int stopTimer0(void);
void delayUS(unsigned int microseconds);
void delayMS(unsigned int milliseconds);
void U0Write(char data);
char U0Read(void);

int fputc(int c)
{
	U0Write(c);
	return c; //return the character written to denote a successfull write
}

int fgetc()
{
	char c = U0Read();
	U0Write(c); //To echo Received characters back to serial Terminal
	return c;
}

void initUART0(void) //PCLK must be = 25Mhz!
{
	LPC_PINCON->PINSEL0 |= (1<<4) | (1<<6); //Select TXD0 and RXD0 function for P0.2 & P0.3!
	//LPC_SC->PCONP |= 1<<3; //Power up UART0 block. By Default it is enabled after RESET.
	LPC_UART0->LCR = 3 | DLAB_BIT ; /* 8 bits, no Parity, 1 Stop bit & DLAB set to 1  */
	LPC_UART0->DLL = 12;
	LPC_UART0->DLM = 0;
	LPC_UART0->FCR |= Ux_FIFO_EN | Rx_FIFO_RST | Tx_FIFO_RST;
	LPC_UART0->FDR = (MULVAL<<4) | DIVADDVAL; /* MULVAL=15(bits - 7:4) , DIVADDVAL=2(bits - 3:0)  */
	LPC_UART0->LCR &= ~(DLAB_BIT);
	//Baud= ~115200(114882). Now we can perform UART communication!
}

void initTimer0(void) //PCLK must be = 25Mhz!
{
	LPC_TIM0->CTCR = 0x0;
	LPC_TIM0->PR = PRESCALE; //Increment TC at every 24999+1 clock cycles
	LPC_TIM0->TCR = 0x02; //Reset Timer
}

void startTimer0(void)
{
	LPC_TIM0->TCR = 0x02; //Reset Timer
	LPC_TIM0->TCR = 0x01; //Enable timer
}

unsigned int stopTimer0(void)
{
	LPC_TIM0->TCR = 0x00; //Disable timer
	return LPC_TIM0->TC;
}

void delayUS(unsigned int microseconds) //Using Timer0
{
	LPC_TIM0->TCR = 0x02; //Reset Timer
	LPC_TIM0->TCR = 0x01; //Enable timer
	while(LPC_TIM0->TC < microseconds); //wait until timer counter reaches the desired delay
	LPC_TIM0->TCR = 0x00; //Disable timer
}

void delayMS(unsigned int milliseconds) //Using Timer0
{
	delayUS(milliseconds * 1000);
}

void U0Write(char cChar)
{
	while ( !(LPC_UART0->LSR & THRE) ); // wait till the THR is empty
	// now we can write to the queue
	if( cChar == '\n' ) //Send <CR+LF>
	{
		LPC_UART0->THR = CARRIAGE_RETURN;
		while( !(LPC_UART0->LSR & THRE )); 
		LPC_UART0->THR = LINE_FEED;
	}
	else
	{
		LPC_UART0->THR = cChar;
	}
}

char U0Read(void)
{
	while( !(LPC_UART0->LSR & RDR )); 
	return LPC_UART0->RBR; 
}

void delays(unsigned int k)
{
 for (j=0; j<k; j++);
}
void lcd_port()
{
 LPC_GPIO0->FIOPIN=msg1<<23;
 if (flag1==0)
 {
 LPC_GPIO0->FIOCLR=1<<27;
 }
 else
 {
 LPC_GPIO0->FIOSET=1<<27;
 }
 LPC_GPIO0->FIOSET=1<<28;
 delays(50);
 LPC_GPIO0->FIOCLR=1<<28;
 delays(1000000);
}
void lcd_write()
{
 msg1=temp&0xF0;
 msg1=msg1>>4;
 lcd_port();
 if ((i>3)|(flag1==1))
 {
 msg1=temp&0x0F;
 lcd_port();
 }
}
void SEND_STRING_DATA(char* msg){
 unsigned int i;
 unsigned char init[]={0x30, 0x30, 0x30, 0x20, 0x28, 0x0C, 0x01, 0x06, 0x80};
 LPC_PINCON->PINSEL1&=0xFC003FFF;
 LPC_GPIO0->FIODIR|=0X3F<<23;
 flag1=0;
 for (i=0; i<9; i++)
 {
 temp=init[i];
 lcd_write();
 }
 flag1=1;
 i=0;
 while(msg[i]!='\0')
 {
 temp=msg[i];
 lcd_write();
 i++;
 }
 
}
#define DATA_PIN (1<<4) 
char get_word(void)
{
 int time = 0;
 
 response(50,5,LOW);
 
 startTimer0();
 while(LPC_GPIO0->FIOPIN & DATA_PIN)
 {
 if(LPC_TIM0->TC > 75)
 {
 return 2;
 }
 }
 time = stopTimer0();
 
 if((time > (27-10)) && (time < (27+10))) 
 {
 return 0;
 }
 else if((time > (70-5)) && (time < (70+5)))
 {
 return 1;
 }
 else
 { 
 return 2;
 }
}


int main(void)
{
 unsigned char sWord[40] = {0};
 char word[5] = {0};
 SystemInit();
 SystemCoreClockUpdate();
 initTimer0();
 
 
 while(1)
 {
 LPC_PINCON->PINSEL0 &= 0<<8; //GPIO for P0.4
 LPC_GPIO0->FIODIR &= ~DATA_PIN;
 LPC_GPIO0->FIOCLR |= DATA_PIN;
 delayUS(18000);
 LPC_GPIO0->FIODIR &= ~(DATA_PIN);
 startTimer0();
 while((LPC_GPIO0->FIOPIN & DATA_PIN) != 0)
 {
 if(LPC_TIM0->TC > 40) 
break;
 }
 time = stopTimer0();
 if(time < 10 || time > 40) 
 { 
 SEND_STRING_DATA("Connection Failed");
 }
 response(80,5,LOW);
 response(80,5,HIGH);
 for(i=0; i < 40; i++)
 {
 data = get_word();
 if(data == 0 || data == 1)
 {
 sWord[i] = data;
 }
 else SEND_STRING_DATA("Error in receiving data");
 }
 
 data = 0;
 for(i=0; i<5; i++)
 {
 for(j=0; j<8; j++)
 {
 if( sWord[ 8*i + j ] )
 data |= (1<<(7-j));
 }
 word[i] = data;
 data = 0;
 } 
 strcat(word," H");
 SEND_STRING_DATA(word);
 delayUS(2000000); 
 }
}
void response(unsigned int wTime, unsigned int mar, unsigned char pinVal)
{
 int time = 0;
 int maxi = wTime + mar;
 
 startTimer0();
 if(pinVal)
 {
 while(LPC_GPIO0->FIOPIN & DATA_PIN)
 {
 if(LPC_TIM0->TC > (maxi)) break; 
 }
 }
 else
 {
 while( !(LPC_GPIO0->FIOPIN & DATA_PIN) )
 {
 if(LPC_TIM0->TC > (maxi)) break; 
 }
 }
 time = stopTimer0();
 
 if(time < (wTime-mar) || time > maxi) 
 {
 SEND_STRING_DATA("Out of Range");
 }
}
