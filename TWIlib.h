#ifndef TWILIB_H_
#define TWILIB_H_
// TWI bit rate
#define TWI_FREQ 100000
// Get TWI status
#define TWI_STATUS	(TWSR & 0xF8) 
// Transmit buffer length
#define TXMAXBUFLEN 20
// Receive buffer length
#define RXMAXBUFLEN 20
// Global transmit buffer
#define MAXPACKETTYPE 20
uint8_t TWITransmitBuffer[TXMAXBUFLEN];
// Global receive buffer
volatile uint8_t TWIReceiveBuffer[RXMAXBUFLEN];
// Global receive handlers
void (*volatile handlers[MAXPACKETTYPE])(volatile uint8_t*, uint8_t);

// Buffer indexes
volatile int TXBuffIndex; // Index of the transmit buffer. Is volatile, can change at any time.
volatile int RXBuffIndex; // Current index in the receive buffer
// Buffer lengths
volatile int TXBuffLen; // The total length of the transmit buffer
volatile int RXBuffLen; // The total number of bytes to read (should be less than RXMAXBUFFLEN)

typedef enum {
	Ready,
	Initializing,
	RepeatedStartSent,
	MasterTransmitter,
	MasterReceiver,
	SlaveTransmitter,
	SlaveReciever
} TWIMode;

 typedef struct TWIInfoStruct{
	TWIMode mode;
	uint8_t errorCode;
	uint8_t repStart;	
}TWIInfoStruct;

TWIInfoStruct TWIInfo;


// TWI Status Codes
#define TWI_START_SENT			0x08 // Start sent
#define TWI_REP_START_SENT		0x10 // Repeated Start sent
// Master Transmitter Mode
#define TWI_MT_SLAW_ACK			0x18 // SLA+W sent and ACK received
#define TWI_MT_SLAW_NACK		0x20 // SLA+W sent and NACK received
#define TWI_MT_DATA_ACK			0x28 // DATA sent and ACK received
#define TWI_MT_DATA_NACK		0x30 // DATA sent and NACK received
// Master Receiver Mode
#define TWI_MR_SLAR_ACK			0x40 // SLA+R sent, ACK received
#define TWI_MR_SLAR_NACK		0x48 // SLA+R sent, NACK received
#define TWI_MR_DATA_ACK			0x50 // Data received, ACK returned
#define TWI_MR_DATA_NACK		0x58 // Data received, NACK returned
// Slave Receiver Mode
#define TWI_SR_SLAW_ACK			0x60 // Own SLA+W received and ACK Sent
#define TWI_SR_LOST_ARBIT		0x68 // Lost arbitration and own SLA+W received and ACK Sent
#define TWI_SR_GENERAL_CALL		0x70 // General call address received and ACK Sent
#define TWI_SR_LOST_ARBIT_GENERAL_CALL 0x78 //Lost arbitration and general call adress received and ACK Sent
#define TWI_SR_DATA_ACK			0x80 // Data received (Previously addressed) and ACK Sent
#define TWI_SR_DATA_NACK		0x88 // Data received (Previously addressed) and NACK Sent
#define TWI_SR_DATA_ACK_GENERAL	0x90 // Data received (Previously general call) and ACK Sent
#define TWI_SR_DATA_NACK_GENERAL 0x98 // Data received (Previously general call) and NACK Sent
#define TWI_SR_STOP_RECV		0xA0 //Stop or repeated STart condition received while addressed as SR
// Slave Transmitter Mode
#define  TWI_ST_SLAW_ACK		0xA8 //Own SLA+R received and ACK Sent
#define TWI_ST_LOST_ARBIT		0xB0 //Lost arbitration and own SLA+R received and ACK Sent
#define TWI_ST_DATA_ACK 		0xB8 //Data Sent and ACK received
#define  TWI_ST_DATA_NACK 		0xC0 //Data Sent and NACK received
#define TWI_ST_DATA_LAST_ACK	0xC8 //Last data byte sent ACK received		
// Miscellaneous States
#define TWI_LOST_ARBIT			0x38 // Arbitration has been lost
#define TWI_NO_RELEVANT_INFO	0xF8 // No relevant information available
#define TWI_ILLEGAL_START_STOP	0x00 // Illegal START or STOP condition has been detected
#define TWI_SUCCESS				0xFF // Successful transfer, this state is impossible from TWSR as bit2 is 0 and read only


#define TWISendStart()		(TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE)) // Send the START signal, enable interrupts and TWI, clear TWINT flag to resume transfer.
#define TWISendStop()		(TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN)|(1<<TWIE)) // Send the STOP signal, enable interrupts and TWI, clear TWINT flag.
#define TWISendTransmit()	(TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE)) // Used to resume a transfer, clear TWINT and ensure that TWI and interrupts are enabled.
#define TWISendACK()		(TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE)|(1<<TWEA)) // FOR MR mode. Resume a transfer, ensure that TWI and interrupts are enabled and respond with an ACK if the device is addressed as a slave or after it receives a byte.
#define TWISendNACK()		(TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE)) // FOR MR mode. Resume a transfer, ensure that TWI and interrupts are enabled but DO NOT respond with an ACK if the device is addressed as a slave or after it receives a byte.

// Function declarations
uint8_t installHandler(void (*handler)(volatile uint8_t*, uint8_t) ,uint8_t packetType);
uint8_t TWIMasterTransmitData(void *const TXdata, uint8_t dataLen, uint8_t repStart);
void TWIInitMaster(void);
uint8_t TWIMasterReadData(uint8_t TWIaddr, uint8_t bytesToRead, uint8_t repStart);
uint8_t isTWIReady(void);
uint8_t TWISlaveSendData(void *const TXdata, uint8_t dataLen);
void TWIInitSlave(uint8_t TWIaddr);
uint8_t getTWIErrorCode(void);



#endif // TWICOMMS_H_ 
