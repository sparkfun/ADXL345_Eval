
/*
 * Copyright (c) 2006-2009 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <ctype.h>
#include <string.h>

//#include <avr/sleep.h>

#include "fat.h"
#include "fat_config.h"
#include "partition.h"
#include "sd_raw.h"
#include "sd_raw_config.h"
#include "main.h"
#include "ADXL345.h"

#define DEBUG 0

uint8_t find_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name, struct fat_dir_entry_struct* dir_entry);
struct fat_file_struct* open_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name); 

char init_filesystem(void);
struct fat_file_struct* create_log_file(void);
void save_data(struct fat_file_struct **file_handle, const char * const buf);

void ioinit(void);
char get_command(char * command);
char execute_command(char * command);

struct fat_dir_struct* dd;
struct fat_dir_entry_struct directory;
struct fat_fs_struct* fs;
struct partition_struct* partition;

struct fat_file_struct * log_file_handle;
char low_byte=0;
char high_byte=0;
int x_value=0, y_value=0, z_value=0;
char ascii_value[6];
char data_buffer[512];

int main(void)
{
	char command_buffer[6];
	char continue_char=0;
	char enabled_interrupts=0, interrupt_map=0, interrupt_source=0;
	char led_2_timer=0, led_3_timer=0;
	char sd_init_tries=0, sd_init_success=0;
	char logging_enabled=0;
	
    /* setup uart */
    //uart_init();

	ioinit();	//Setup the AVR I/O for data logging
	UNSELECT_ACCEL();
	
	//Flash lights to indicate succesfull startup
	LED1_ON();
	delay_ms(100);		
	LED1_OFF();
	
	LED2_ON();
	delay_ms(100);		
	LED2_OFF();
	
	LED3_ON();
	delay_ms(100);		
	LED3_OFF();	
	delay_ms(100);

	PORTC |= 0x20;
	delay_ms(100);
	PORTC &= ~(0x20);
	delay_ms(100);
	
	PORTC |= 0x10;
	delay_ms(100);
	PORTC &= ~(0x10);
	delay_ms(100);
	
	PORTB |= 0x02;
	delay_ms(100);
	PORTB &= ~(0x02);
	delay_ms(100);

	PORTB |= 0x01;
	delay_ms(100);
	PORTB &= ~(0x01);
	delay_ms(100);	

	//Initialize SD and FAT32
	//while(!init_filesystem());
	sd_init_success=init_filesystem();
	//while(!sd_init_success && (sd_init_tries++ < 3))
	//{
	//	sd_init_success = init_filesystem();
	//}
	if(sd_init_success)logging_enabled =1;

	//Initialize system for file logging
	if(logging_enabled)log_file_handle=create_log_file();

	//Initialize Accelerometer to start logging data
	adxl345_defaults();
	
	//Flash all lights to indicate start of logging!
	for(int i=0; i<((logging_enabled+1)*2); i++){
		LED1_ON();
		LED2_ON();
		LED3_ON();
		delay_ms(100);
		LED1_OFF();
		LED2_OFF();
		LED3_OFF();
		delay_ms(100);
	}	
	
	
	while(1){
		//The enabled_interrupts variable is used to keep track of the enabled interrupts that can cause a status LED to turn on
		//(WATERMARK, OVERRUN AND DATA_READY are ignored because they generate interrupts regardless of if they are enabled or not)
		enabled_interrupts = adxl345_read(INT_ENABLE);
		enabled_interrupts &= ~(WATERMARK | OVERRUN | DATA_READY);
		//interrupt_map is used to keep track of which interrupts are going to the INT1 pin and which are going to the INT2 pin.
		//If the bit is 0, the interrupt is going to the INT1 pin; if the bit is 1, the interrupt is going to the INT2 pin.
		interrupt_map = adxl345_read(INT_MAP);
		while((UCSR0A & (1<<RXC0)) == 0){
			if((PIND & ACCEL_INT1) == 0){
				//Read the interrupt source register to clear any interrupts
				interrupt_source=adxl345_read(INT_SOURCE);
				if((interrupt_source & DATA_READY)==DATA_READY)
				{
					high_byte = adxl345_read(DATAX1);
					low_byte = adxl345_read(DATAX0);
					x_value = (high_byte << 8) | low_byte;
					
					high_byte = adxl345_read(DATAY1);
					low_byte = adxl345_read(DATAY0);
					y_value = (high_byte << 8) | low_byte;

					high_byte = adxl345_read(DATAZ1);
					low_byte = adxl345_read(DATAZ0);
					z_value = (high_byte << 8) | low_byte;

					sprintf(ascii_value, "%04x", x_value);
					strcat(data_buffer, ascii_value);
					strcat(data_buffer, ",");
					
					sprintf(ascii_value, "%04x", y_value);
					strcat(data_buffer, ascii_value);
					strcat(data_buffer, ",");
					
					sprintf(ascii_value, "%04x", z_value);
					strcat(data_buffer, ascii_value);
					strcat(data_buffer, "\n");

					printf("X:%04x\tY:%04x\tZ:%04x\r", x_value, y_value, z_value);
				}
				//Check to see if ther are any other active interrupts for the INT1 Interrupt Source from the ADXL345.
				if((interrupt_source & enabled_interrupts & ~(interrupt_map)) != 0)led_2_timer=BLINK_TIME;
			}
			if((PIND & ACCEL_INT2) == 0) led_3_timer=BLINK_TIME;

			if(led_2_timer > 0){
				LED2_ON();
				led_2_timer-=1;
			}
			else LED2_OFF();
			
			if(led_3_timer > 0){
				LED3_ON();
				led_3_timer-=1;
			}
			else LED3_OFF();

			if(strlen(data_buffer)>=495){
				LED1_ON();
				if(logging_enabled)save_data(&log_file_handle, data_buffer);
				*data_buffer='\0';
				LED1_OFF();
			}
		}
		//If we get here, we need to enter the command prompt
		//printf("Got a character!\n");
		if(logging_enabled)fat_close_file(log_file_handle);
		continue_char = UDR0;		//Read the character from the UART to clear the buffer
		delay_ms(50);

		//Put the ADXL345 into standby mode
		adxl345_write(POWER_CTL, ~MEASURE);
		while(continue_char !=2)
		{
			LED2_OFF();
			LED3_OFF();
			printf("\n>");
			//If we get a valid command length, than execute the command
			if(get_command(command_buffer))continue_char=execute_command(command_buffer);
		}
		printf("\n");
		//Put the ADXL345 back into measurement mode
		adxl345_write(POWER_CTL, MEASURE);
		if(logging_enabled)log_file_handle=create_log_file();
	}

    return 0;
}

uint8_t find_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name, struct fat_dir_entry_struct* dir_entry)
{
    while(fat_read_dir(dd, dir_entry))
    {
        if(strcmp(dir_entry->long_name, name) == 0)
        {
            fat_reset_dir(dd);
            return 1;
        }
    }

    return 0;
}

struct fat_file_struct* open_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name)
{
    struct fat_dir_entry_struct file_entry;
    if(!find_file_in_dir(fs, dd, name, &file_entry))
        return 0;

    return fat_open_file(fs, &file_entry);
}

//Function: ioinit
//Purpose:	Initialize AVR I/O, UART and Interrupts
//Inputs:	None
//Outputs:	None
void ioinit(void)
{
    //1 = output, 0 = input
	
	adxl345_hw_setup();
	
	DDRD |= (1<<TX_O)|(1<<STAT3)|(1<<STAT2)|(1<<STAT1);		//Define Port D Outputs
	PORTD |= (1<<RX_I);		//Use internal pull-ups on these inputs
    DDRD &= ~(1<<RX_I);	//Define Port D inputs
	
	//Set Extra Pins to Outputs for testing
	DDRC |= 0xF0;
	DDRB |= 0x03;
	
    UBRR0H = (MYUBRR >> 8) & 0x7F;	//Make sure highest bit(URSEL) is 0 indicating we are writing to UBRRH
	UBRR0L = MYUBRR;
    UCSR0A = (1<<U2X0);					//Double the UART Speed
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);		//Enable Rx and Tx in UART
    UCSR0C = (1<<UCSZ00)|(1<<UCSZ01);		//8-Bit Characters
    stdout = &mystdout; //Required for printf init
	
	// Init timer 2
	//Set Prescaler to 8. (Timer Frequency set to 1Mhz
	TCCR2B = (1<<CS21); 	//Divde clock by 8 for 1 Mhz Timer 2 Frequency
    	
	cli();
}

static int uart_putchar(char c, FILE *stream)
{
  if (c == '\n')
    uart_putchar('\r', stream);
  
  loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = c;
  return 0;
}

uint8_t uart_getchar(void)
{
    while( !(UCSR0A & (1<<RXC0)) );
	return(UDR0);
}

struct fat_file_struct* create_log_file(void)
{
	struct fat_dir_entry_struct file_entry;
	char name[32];
	int file_number=0;
	
	sprintf(name, "ADXL345_%03d.csv", file_number);
	while(find_file_in_dir(fs,dd,name,&directory)){
		file_number++;
		sprintf(name, "ADXL345_%03d.csv", file_number);
	}
	
#ifdef DEBUG
	if(!fat_create_file(dd, name, &file_entry))
	{
		printf_P(PSTR("No File Created!\n\r"));
	}
#endif
   /* search file in current directory and open it */
	return open_file_in_dir(fs, dd, name);

}

//Usage: saveData(FILE_NAME, log_data, log_data_index);
//Inputs: char *buf - character array to be saved
//		  int buf_size - size of character array
//Output: buffer array is saved to LOG_FILE
//Description: Saves the buf character array to the SD card.
//CONDITIONS: LOG_FILE must be initialized to the handle of an open file.
void save_data(struct fat_file_struct **file_handle, const char * const buf)
{
	fat_write_file(*file_handle, (unsigned char*)buf, strlen(buf));
	sd_raw_sync();
}

char init_filesystem(void)
{
	//setup sd card slot 
	if(!sd_raw_init())
	{
#if DEBUG
		printf_P(PSTR("MMC/SD initialization failed\n"));
#endif
		return 0;
	}

	//open first partition
	partition = partition_open(sd_raw_read,
									sd_raw_read_interval,
#if SD_RAW_WRITE_SUPPORT
									sd_raw_write,
									sd_raw_write_interval,
#else
									0,
									0,
#endif
									0
							   );

	if(!partition)
	{
		//If the partition did not open, assume the storage device
		//is a "superfloppy", i.e. has no MBR.
		partition = partition_open(sd_raw_read,
								   sd_raw_read_interval,
#if SD_RAW_WRITE_SUPPORT
								   sd_raw_write,
								   sd_raw_write_interval,
#else
								   0,
								   0,
#endif
								   -1
								  );
		if(!partition)
		{
#if DEBUG
			printf_P(PSTR("opening partition failed\n"));
#endif
			return 0;
		}
	}

	//Open file system
	//struct fat_fs_struct* fs = fat_open(partition);
	fs = fat_open(partition);
	if(!fs)
	{
#if DEBUG
		printf_P(PSTR("opening filesystem failed\n"));
#endif
		return 0;
	}

	//Open root directory
	//struct fat_dir_entry_struct directory;
	fat_get_dir_entry_of_path(fs, "/", &directory);

	//struct fat_dir_struct* dd = fat_open_dir(fs, &directory);
	dd=fat_open_dir(fs, &directory);
	if(!dd)
	{
#if DEBUG
		printf_P(PSTR("opening root directory failed\n"));
#endif
		return 0;
	}
	return 1;
}

//General short delays
void delay_ms(uint16_t x)
{
    for (; x > 0 ; x--)
        delay_us(1000);
}

//General short delays
void delay_us(uint16_t x)
{    
    while(x > 256)
    {
        TIFR2 = (1<<TOV2); //Clear any interrupt flags on Timer2
        TCNT2 = 0; //256 - 125 = 131 : Preload timer 2 for x clicks. Should be 1us per click
        while( (TIFR2 & (1<<TOV2)) == 0);
       
        x -= 256;
    }

    TIFR2 = (1<<TOV2); //Clear any interrupt flags on Timer2
    TCNT2= 256 - x; //256 - 125 = 131 : Preload timer 2 for x clicks. Should be 1us per click
    while( (TIFR2 & (1<<TOV2)) == 0);
} 

char execute_command(char * command)
{
	char address=0;
	char command_length=strlen(command);
	char address_string[2], register_string[2];
	char register_value=0;
	
	//Every command that has a payload has the address in the first two positions of the command payload
	sprintf(address_string, "%c%c", command[1], command[2]);
	address = (char)strtol(address_string, NULL, 16);	//Convert the address string to a hexadecimal value

	//Check to see if we received the read command
	if(command[0] == 'R' || command[0] == 'r')
	{
		//Make sure received at least enough characters for a valid command
		if(command_length != 3) return 0;	
	
		register_value=adxl345_read(address);
		printf("\nReg. Address: %02x\n", address);
		printf("Reg. Value: %02x", register_value);
		return 1;
	}
	else if(command[0]=='w' || command[0] == 'W')
	{
		//Make sure we received enough characters for this command
		if(command_length != 5) return 0;

		//Get the value to be written to the register
		sprintf(register_string, "%c%c", command[3], command[4]);
		register_value = (char)strtol(register_string, NULL, 16);
		//Now write the value to the register
		adxl345_write(address, register_value);
		//Now read back the reg to verify
		register_value=adxl345_read(address);
		printf("\nReg. Address: %x\n", address);
		printf("Reg. Value: %02x", register_value);
		return 1;
	}
	//Check to see if the address was given in 'Short-Hand' (Ctrl+'digit')
	else if(command[0] < 29)
	{
		if(command_length > 1) return 0;
		
		//Convert the character to the proper address value
		address = command[0]+29;
		register_value=adxl345_read(address);
		//For Debugging Purposes...
		//printf("\nReg. Address: %x\n", address);
		//printf("Reg. Value: %02x", register_value);
		printf("%02x", register_value);
		return 1;
	}
	else if(command[0]==30)
	{
		if(command_length > 1) return 0;
	
		for(address=29; address<58; address++)
		{
			printf("%02x,", adxl345_read(address));
		}
		return 1;
	}
	else if(command[0]=='s' || command[0] == 'S')return 2;
	else{
		printf("\nInvalid Command");
		return 0;
	}
}

char get_command(char * command)
{
	char receive_char=0, command_buffer_count=0;
	
	//Get a command from the prompt (A command can have a maximum of MAX_COMMAND_LENGTH characters). Command is ended with a carriage return ('Enter' key)
	receive_char = uart_getchar();
	while(receive_char != '\r'){
		*command=receive_char;
		printf("%c", *command++);
		if((command_buffer_count == 0) || (command_buffer_count == 2) || (command_buffer_count == 4))printf(" ");
		command_buffer_count++;
		if(command_buffer_count == MAX_COMMAND_LENGTH)break; 	//If we've added MAX_COMMAND_LENGTH characters to the buffer, get out of this loop!
		receive_char =  uart_getchar();
	}
	*command='\0';	//Terminate the command string with a NULL character. This is so command_buffer[] can be treated as a string.
	if((command_buffer_count == 0) || (command_buffer_count == 2) || (command_buffer_count == 4))return 0;
	return 1;
}