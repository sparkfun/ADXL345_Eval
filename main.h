//*******************************************************
//					GPIO Definitions
//*******************************************************
//Pins 0-3 are defined in ADXL345.h
#define PC4				4		//Port C.4
#define PC5				5		//Port C.5
#define PC6				7

#define RX_I			0		//Port D.0
#define TX_O			1		//Port D.1
//Pins 2,3 are defined in ADXL345.h
//#define PD4			4		//Port D.4
#define STAT3			5		//Port D.5
#define STAT2			6		//Port D.6
#define STAT1			7		//Port D.7

#define	CS			2		//Port B.2
#define MOSI		3		//Port B.3
#define MISO		4		//Port B.4
#define SCK			5		//Port B.5
//*******************************************************
//						Macros
//*******************************************************
#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

#define	LED1_ON()	sbi(PORTD, STAT1)
#define	LED1_OFF()	cbi(PORTD, STAT1)

#define	LED2_ON()	sbi(PORTD, STAT2)
#define	LED2_OFF()	cbi(PORTD, STAT2)

#define	LED3_ON()	sbi(PORTD, STAT3)
#define	LED3_OFF()	cbi(PORTD, STAT3)

#define BLINK_TIME	100
//*******************************************************
//					General Definitions
//*******************************************************
//#define MYUBRR 8	//Used to set the AVR Baud Rate TO 115200 (internal 8 MHz Oscillator)
//#define MYUBRR 16	//Used to set the AVR Baud Rate TO 57600 (internal 8 MHz Oscillator)
#define MYUBRR 25	//Used to set the AVR Baud Rate TO 38400 (internal 8 MHz Oscillator)

#define MAX_COMMAND_LENGTH	5
//=======================================================
//					Function Definitions
//=======================================================
static int uart_putchar(char c, FILE *stream);
uint8_t uart_getchar(void);
static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
void ioinit(void);
void delay_ms(uint16_t x);
void delay_us(uint16_t x);