/* 
Jacob Mickiewicz 3/25/15
a program for an attiny85 to be an I2C slave device

On read returns function.
On write changes the current funtion or i2c adress.
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

uint8_t getDefaultData(uint8_t command);

uint8_t getDefaultData(uint8_t command) {
	return 42;
}

GetData_ptr get_data = getDefaultData;


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
	usiTwiTransmitByte(get_data(State_i2c));
	State_i2c = 0;
}

void receiveEvent_i2c(uint8_t HowMany) //this runs when a write is detected for address 
{                                  //flow is slave_write then state_# then value. 3 bytes to changes omething
    switch (State_i2c)
    {
	  case 3: //reset
		
		usiTwiReceiveByte(); //trash 3rd byte
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


void setup_i2c(uint8_t slave_add, GetData_ptr getDataFunc)
{
	I2C_setup(slave_add);
	get_data = getDataFunc;
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