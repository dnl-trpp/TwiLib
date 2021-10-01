#include <avr/io.h>
#include <avr/interrupt.h>
#include "TWIlib.h"
#include "util/delay.h"

void TWIInitMaster()
{
	TWIInfo.mode = Ready;
	TWIInfo.errorCode = 0xFF;
	TWIInfo.repStart = 0;
	// No pre-scaling
	TWSR = 0;
	// Set bit rate
	TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;
	// Enable TWI and interrupt
	TWCR = (1 << TWIE) | (1 << TWEN);
}

void TWIInitSlave(uint8_t TWIaddr)
{
	TWIInfo.mode = Ready;
	TWIInfo.errorCode = 0xFF;
	TWIInfo.repStart = 0;
	// No pre-scaling
	TWSR = 0;
	// Set bit rate
	TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;
	//Set address and enable general call
	TWAR = (TWIaddr<<1) | 0x01;
	// Clear Buffers
	RXBuffIndex = 0;
	RXBuffLen = 0;
	TXBuffLen = 0;
	TXBuffIndex = 0;
	TWISendACK(); //Get ready to repond with ACK;

}

uint8_t isTWIReady()
{
	if ( (TWIInfo.mode == Ready) | (TWIInfo.mode == RepeatedStartSent) )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

uint8_t getTWIErrorCode(){
	return TWIInfo.errorCode;
}

uint8_t TWIMasterTransmitData(void *const TXdata, uint8_t dataLen, uint8_t repStart)
{
	if (dataLen <= TXMAXBUFLEN)
	{
		// Wait until ready
		while (!isTWIReady()) {_delay_us(1);}
		TWIInfo.repStart = repStart;
		// Copy data into the transmit buffer
		uint8_t *data = (uint8_t *)TXdata;
		for (int i = 0; i < dataLen; i++)
		{
			TWITransmitBuffer[i] = data[i];
		}
		// Copy transmit info to global variables
		TXBuffLen = dataLen;
		TXBuffIndex = 0;
		
		// If a repeated start has been sent, then devices are already listening for an address
		// and another start does not need to be sent. 
		if (TWIInfo.mode == RepeatedStartSent)
		{
			TWIInfo.mode = Initializing;
			TWDR = TWITransmitBuffer[TXBuffIndex++]; // Load data to transmit buffer
			TWISendTransmit(); // Send the data
		}
		else // Otherwise, just send the normal start signal to begin transmission.
		{
			TWIInfo.mode = Initializing;
			TWISendStart();
		}
		
	}
	else
	{
		return 1; //Return error
	}
	return 0;
}

uint8_t TWIMasterReadData(uint8_t TWIaddr, uint8_t bytesToRead, uint8_t repStart)
{
	// Check if number of bytes to read can fit in the RXbuffer
	if (bytesToRead < RXMAXBUFLEN)
	{
		// Reset buffer index and set RXBuffLen to the number of bytes to read
		RXBuffIndex = 0;
		RXBuffLen = bytesToRead;
		// Create the one value array for the address to be transmitted
		uint8_t TXdata[1];
		// Shift the address and AND a 1 into the read write bit (set to write mode)
		TXdata[0] = (TWIaddr << 1) | 0x01;
		// Use the TWITransmitData function to initialize the transfer and address the slave
		TWIMasterTransmitData(TXdata, 1, repStart);
	}
	else
	{
		return 0;
	}
	return 1;
}

uint8_t installHandler(void (*handler)(volatile uint8_t*, uint8_t) ,uint8_t packetType){
	if(packetType>MAXPACKETTYPE) return 0;
	handlers[packetType] = handler;
	return 1;
}

//Get ready to transmit something
uint8_t TWISlaveSendData(void *const TXdata, uint8_t dataLen){
	
	if(dataLen <= TXMAXBUFLEN)
	{			
		// Copy data into the transmit buffer
		uint8_t *data = (uint8_t *)TXdata;
		for (int i = 0; i < dataLen; i++)
		{
			TWITransmitBuffer[i] = data[i];
		}
		// Copy transmit info to global variables
		TXBuffLen = dataLen;
		TXBuffIndex = 0;
	    //TWIInfo.mode = Initializing;
		//TWISendACK();
	}else
	{
		return 0;
	}
	return 1;
}

ISR (TWI_vect)
{
	switch (TWI_STATUS)
	{
		// -------- MASTER TRANSMITTER OR WRITING ADDRESS --------  //
		case TWI_MT_SLAW_ACK: // SLA+W transmitted and ACK received
		// Set mode to Master Transmitter
		TWIInfo.mode = MasterTransmitter;
		case TWI_START_SENT: // Start condition has been transmitted
		case TWI_MT_DATA_ACK: // Data byte has been transmitted, ACK received
			if (TXBuffIndex < TXBuffLen) // If there is more data to send
			{
				TWDR = TWITransmitBuffer[TXBuffIndex++]; // Load data to transmit buffer
				TWIInfo.errorCode = TWI_NO_RELEVANT_INFO;
				TWISendTransmit(); // Send the data
			}
			// This transmission is complete however do not release bus yet
			else if (TWIInfo.repStart)
			{
				TWIInfo.errorCode = 0xFF;
				TWISendStart();
			}
			// All transmissions are complete, exit
			else
			{
				TWIInfo.mode = Ready;
				TWIInfo.errorCode = 0xFF;
				TWISendStop();
			}
			break;


		// -------- MASTER RECEIVER --------  //		
		case TWI_MR_SLAR_ACK: // SLA+R has been transmitted, ACK has been received
			// Switch to Master Receiver mode
			TWIInfo.mode = MasterReceiver;
			// If there is more than one byte to be read, receive data byte and return an ACK
			if (RXBuffIndex < RXBuffLen-1)
			{
				TWIInfo.errorCode = TWI_NO_RELEVANT_INFO;
				TWISendACK();
			}
			// Otherwise when a data byte (the only data byte) is received, return NACK
			else
			{
				TWIInfo.errorCode = TWI_NO_RELEVANT_INFO;
				TWISendNACK();
			}
			break;
		
		case TWI_MR_DATA_ACK: // Data has been received, ACK has been transmitted.
		
			/// -- HANDLE DATA BYTE --- ///
			TWIReceiveBuffer[RXBuffIndex++] = TWDR;
			// If there is more than one byte to be read, receive data byte and return an ACK
			if (RXBuffIndex < RXBuffLen-1)
			{
				TWIInfo.errorCode = TWI_NO_RELEVANT_INFO;
				TWISendACK();
			}
			// Otherwise when a data byte (the only data byte) is received, return NACK
			else
			{
				TWIInfo.errorCode = TWI_NO_RELEVANT_INFO;
				TWISendNACK();
			}
			break;
		
		case TWI_MR_DATA_NACK: // Data byte has been received, NACK has been transmitted. End of transmission.
		
			/// -- HANDLE DATA BYTE --- ///
			TWIReceiveBuffer[RXBuffIndex++] = TWDR;	
			// This transmission is complete however do not release bus yet
			if (TWIInfo.repStart)
			{
				TWIInfo.errorCode = 0xFF;
				TWISendStart();
			}
			// All transmissions are complete, exit
			else
			{
				TWIInfo.mode = Ready;
				TWIInfo.errorCode = 0xFF;
				TWISendStop();
			}
			break;
		

		// -------- MT and MR common -------- //		
		case TWI_MR_SLAR_NACK: // SLA+R transmitted, NACK received
		case TWI_MT_SLAW_NACK: // SLA+W transmitted, NACK received
		case TWI_MT_DATA_NACK: // Data byte has been transmitted, NACK received
		case TWI_LOST_ARBIT: // Arbitration has been lost
			// Return error and send stop and set mode to ready
			if (TWIInfo.repStart)
			{				
				TWIInfo.errorCode = TWI_STATUS;
				TWISendStart();
			}
			// All transmissions are complete, exit
			else
			{
				TWIInfo.mode = Ready;
				TWIInfo.errorCode = TWI_STATUS;
				TWISendStop();
			}
			break;
		case TWI_REP_START_SENT: // Repeated start has been transmitted
			// Set the mode but DO NOT clear TWINT as the next data is not yet ready
			TWIInfo.mode = RepeatedStartSent;
			break;
		

		// -------- SLAVE RECEIVER --------  //		
		case TWI_SR_LOST_ARBIT:
		case TWI_SR_LOST_ARBIT_GENERAL_CALL:
		case TWI_SR_GENERAL_CALL:
		case TWI_SR_SLAW_ACK: //Handles recieved slaw and and general call in the same way
			TWIInfo.mode = SlaveReciever;
			if (RXBuffIndex < RXMAXBUFLEN)
			{
				TWIInfo.errorCode = TWI_NO_RELEVANT_INFO;
				TWISendACK();
			}
			else
			{ 
				TWIInfo.errorCode = TWI_NO_RELEVANT_INFO;
				TWISendNACK();
			}
			break;

		case TWI_SR_DATA_ACK: // Data received (Previously addressed) and ACK Sent
		case TWI_SR_DATA_ACK_GENERAL: // Data received (Previously general call) and ACK Sent
			TWIReceiveBuffer[RXBuffIndex++] = TWDR;
			RXBuffLen++;
			// If there is more than one byte to be read, receive next data byte and return an ACK
			if (RXBuffIndex < RXMAXBUFLEN)
			{
				TWIInfo.errorCode = TWI_NO_RELEVANT_INFO;
				TWISendACK();
			}
			// Otherwise when no more data is expected, return NACK
			else
			{
				TWIInfo.errorCode = TWI_NO_RELEVANT_INFO;
				TWISendNACK();
			}
			break;


		case TWI_SR_DATA_NACK:
		case TWI_SR_DATA_NACK_GENERAL:  //No more data expected(Buffer full Give error)
			TWIInfo.errorCode = TWI_STATUS;
			RXBuffLen=0;
			RXBuffIndex=0;
			TWIInfo.mode = Ready;
			TWISendACK();
			break;



		case TWI_SR_STOP_RECV: //Stop 
			TWIInfo.errorCode = 0xff; //Success
			if(handlers[TWIReceiveBuffer[0]] != 0){ //If a handler for this packet Type is installed
				(handlers[TWIReceiveBuffer[0]])(TWIReceiveBuffer,RXBuffLen); //Then call it!
			}
			RXBuffLen=0;
			RXBuffIndex=0;
			TWIInfo.mode = Ready;
			TWISendACK(); //Clear TWINT but jump out of TWI
			break;
		

		// -------- SLAVE TRANSMITTER --------  //		
		case TWI_ST_LOST_ARBIT:
		case TWI_ST_SLAW_ACK: //Own SLAR received ACK Transmitted
			TWIInfo.mode= SlaveTransmitter;
		case TWI_ST_DATA_ACK: //Data byte transmitted ACK Recieved
			if (TXBuffIndex < TXBuffLen-1){
				TWDR = TWITransmitBuffer[TXBuffIndex++];
				TWIInfo.errorCode = TWI_NO_RELEVANT_INFO;
				TWISendACK(); //Send and expect ACK
			}else
			{
				TWDR = TWITransmitBuffer[TXBuffIndex++];
				TWIInfo.errorCode = TWI_NO_RELEVANT_INFO;
				TWISendNACK(); //For last byte, send and expect NACK

			}
			break;


		case TWI_ST_DATA_NACK: //Data transmitted and NACK received (last byte or not acknowledged)
		case TWI_ST_DATA_LAST_ACK:
			TWIInfo.errorCode = 0xff;
			TWIInfo.mode = Ready;
			TWISendACK(); //Clear TWINT but jump out of TWI
			break;
		
		
		// -------- MISCELLANEOUS STATES --------  //
		case TWI_NO_RELEVANT_INFO: // It is not really possible to get into this ISR on this condition
								   // Rather, it is there to be manually set between operations
			break;
		case TWI_ILLEGAL_START_STOP: // Illegal START/STOP, abort and return error
			TWIInfo.errorCode = TWI_ILLEGAL_START_STOP;
			TWIInfo.mode = Ready;
			TWISendStop();
			break;
	}
	
}
