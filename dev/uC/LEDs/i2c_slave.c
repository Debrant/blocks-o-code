/* 
Jacob Mickiewicz 3/25/15
a program for an attiny461 to be an I2C slave device to work with 'A Block of Code'
*/

/*
List of current states
#	read				write				notes
0	block function		state				default
1	block function		block function
2	block function		i2c address
3	block function		reset				not implemented
4	Y position			state				may not be used
5	X position			state				may not be used
6	error led			error led
7	status led			status led
*/

/*
to do
error light led - needs to turn on or off
status light led - needs to turn on or off

might like
rename 'state' to be more clear
*/

#include "usiTwiSlave.h"
#include <inttypes.h>
#include <avr/interrupt.h>

#include "i2c_slave.h"

//#define I2C_SLAVE_ADDRESS 42 //in use it'll be set by another program (local bus code) can also be set over the i2c bus
//#define BLOCK_FUNCTION 36 //in phase 2 will be set by another program (adc ->function code) can also be set over the i2c bus
// these are now passed in to this file


volatile uint8_t State_i2c; //this keeps track of the state or mode of the block
volatile uint8_t Block_Function; //= BLOCK_FUNCTION; //this is the lexical function 
volatile uint8_t X_position;
volatile uint8_t Y_position;
volatile uint8_t Error_led;
volatile uint8_t Status_led;


//void Demo_Function_Select();
//void WDT_on();
//void requestEvent_i2c();
//void receiveEvent_i2c(uint8_t Howmany);
//void I2C_setup(uint8_t slave_add);
//void setup_i2c(uint8_t slave_add);
//int loop_i2c();

void Demo_Function_Select(void)
{
	Block_Function = ((PINA >>1) & 0b00000111); //port A inputs 1,2,3
	
	
}

void WDT_on(void) //force a reset in about 16ms
{
	//_WDR();
	/* Clear WDRF in MCUSR */
	MCUSR = 0x00;
	/* Write logical one to WDCE and WDE */
	WDTCR |= (1<<WDCE) | (1<<WDE);
	/* Turn on WDT */
	WDTCR = 0x08;
}

void requestEvent_i2c(void)  //this runs when a read is detected for address
{  
  switch (State_i2c)
  {
  case 7:
	usiTwiTransmitByte(Status_led);
 	State_i2c = 0;
	break;
  case 6:
	usiTwiTransmitByte(Error_led);
	State_i2c = 0;
	break;
  case 5:
	usiTwiTransmitByte(Y_position);
	State_i2c = 0;
    break;
  case 4:
	usiTwiTransmitByte(X_position);
	State_i2c = 0;
    break;
  case 0: //reading the function, default
    usiTwiTransmitByte(Block_Function);
    break;
/*
  default:
//	Demo_Function_Select();
//	_delay_ms(225);
//	_delay_ms(225);
//	_delay_ms(225);
//	_delay_ms(225);

    usiTwiTransmitByte(Block_Function);
    break;
 */
  }
}

void receiveEvent_i2c(uint8_t HowMany) //this runs when a write is detected for address 
{                                  //flow is slave_write then state_# then value. 3 bytes to changes something
    switch (State_i2c)
    {
	  case 7: //status led
		
		if((Status_led=usiTwiReceiveByte())==(0|1))
		{
			PORTA = (PORTA & 0b11011111) | (Status_led << 5); 
		}
		State_i2c = 0;
		break;
	  case 6: //error led
		
		if((Status_led=usiTwiReceiveByte())==(0|1))
		{
			PORTA = (PORTA & 0b11101111) | (Error_led << 4);
		}
		State_i2c = 0;
		break;
	  case 3: //reset
		
		usiTwiReceiveByte(); //trash 3rd byte
		State_i2c = 0;
		break;
      case 2: //changing slave address
        usiTwiSlaveInit(usiTwiReceiveByte());
        State_i2c = 0;
        break;
      case 1: //setting a new function
        Block_Function = usiTwiReceiveByte();
        State_i2c = 0;
        break;
      case 0: //setting a new state, default
      default:       
        State_i2c = usiTwiReceiveByte();
        break;
    }
}

void I2C_setup(uint8_t slave_add)
{
     usiTwiSlaveInit(slave_add);
}


void setup_i2c(uint8_t slave_add,uint8_t Function,uint8_t X,uint8_t Y) 
{
     I2C_setup(slave_add);
	 X_position = X;
	 Y_position = Y;
	 Block_Function = Function;
     usi_onReceiverPtr = receiveEvent_i2c;
     usi_onRequestPtr = requestEvent_i2c;

}

int loop_i2c(void)
{
	if(State_i2c==3)
	{
		State_i2c = 0;
		return(false);
	}
    {
		if (!usi_onReceiverPtr)
		{
			// no onReceive callback, nothing to do...
			return(true);
		}
		if (!(USISR & ( 1 << USIPF )))
		{
			// Stop not detected
			return(true);
		}
		uint8_t amount = usiTwiAmountDataInReceiveBuffer();
		if (amount == 0)
		{
			// no data in buffer
			return(true);
		}
		usi_onReceiverPtr(amount);
		return(true);
	}
}