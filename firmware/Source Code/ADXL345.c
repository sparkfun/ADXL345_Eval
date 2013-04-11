/*
	ADXL345 Library
	
	This libary contains functions to interact with the ADXL345 Triple Axis Digital Accelerometer from Analog Devices written for the ATmega328p
	In order to use this libary, define the appropriate pins in the ADXL345.h file

	created 20 Aug 2009
	by Ryan Owens
	http://www.sparkfun.com
 
*/
#include "ADXL345.h"
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include "main.h"	//For inclusion of delay_us

void adxl345_hw_setup(void)
{
	ADXL_COM_DIRECTION |= (ACCEL_CS | ACCEL_SCK | ACCEL_DI); 	//Define Port C Outputs
	ADXL_COM_PORT |= ACCEL_DO;
	ADXL_COM_DIRECTION &= ~ACCEL_DO;								//Define Port C Inputs
	
	ADXL_INT_DIRECTION &= ~(ACCEL_INT1 | ACCEL_INT2);
	ADXL_INT_PORT |= (ACCEL_INT1 | ACCEL_INT2);
}

void adxl345_defaults(void){

	adxl345_write(DATA_FORMAT, INT_INVERT);	//Invert the interrupt bit (0 on active interrupt)
	adxl345_write(INT_ENABLE, DATA_READY);	//Activate the 'Data Ready' Interrupt
	adxl345_write(INT_MAP, 0<<7);			//Sent the Data Ready Interrupt to the INT1 pin
	adxl345_write(BW_RATE, 0x0A);			//Set Output Rate to 100 Hz
	adxl345_write(POWER_CTL, MEASURE);		//Put the Accelerometer into measurement mode	
}

char adxl345_read(char register_address){
	char read_address=0x80 | register_address;
	char register_value=0;
	
	CLOCK_HIGH();
	SELECT_ACCEL();	//Lower CS pin.
	
	for(int bit=7; bit>=0; bit--){
		CLOCK_LOW();
		
		if((read_address & (1<<bit))==(1<<bit))ADXL_COM_PORT |= ACCEL_DI;
		else ADXL_COM_PORT &= ~ACCEL_DI;
		delay_us(1);
		
		CLOCK_HIGH();
		delay_us(1);
	}
	
	for(int bit=7; bit>=0; bit--){
		CLOCK_LOW();
		delay_us(1);
		
		CLOCK_HIGH();
		delay_us(1);
		
		if((ADXL_COM_PIN & ACCEL_DO)==ACCEL_DO)register_value |= (1<<bit);
		else register_value &= ~(1<<bit);		
	}
	
	UNSELECT_ACCEL();
	
	return register_value;
}

void adxl345_write(char register_address, char register_value){
	CLOCK_HIGH();
	SELECT_ACCEL();	//Lower CS pin.
	
	for(int bit=7; bit>=0; bit--){
		CLOCK_LOW();
		
		if((register_address & (1<<bit))==(1<<bit))ADXL_COM_PORT |= ACCEL_DI;
		else ADXL_COM_PORT &= ~ACCEL_DI;
		delay_us(1);
		
		CLOCK_HIGH();
		delay_us(1);
	}
	
	for(int bit=7; bit>=0; bit--){
		CLOCK_LOW();
		
		if((register_value & (1<<bit))==(1<<bit))ADXL_COM_PORT |= ACCEL_DI;
		else ADXL_COM_PORT &= ~ACCEL_DI;	
		delay_us(1);
		
		CLOCK_HIGH();
		delay_us(1);		
	}
	
	UNSELECT_ACCEL();

}