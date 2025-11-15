#define F_CPU 8000000

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include "usart.h"

#define SLA 0b10100000

#define MASTER

uint8_t data[] = {0, 0, 0, 0, 0, 0, 0, 0};

// To send a START or RE_START condition
uint8_t i2c_Master_Start(uint8_t start_type)
{
	TWCR0 = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

	while ( !(TWCR0 & (1 << TWINT))) ;

	if ( (TWSR0 & 0xF8) != start_type )
		return 0;
	return 1;
}

// To send a STOP condition
void i2c_Master_Stop()
{
	TWCR0 = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

// Call the Slave device and expect an ACK in return
// rw is either TW_READ (1) or TW_WRITE (0)
uint8_t i2c_Master_SLA_Ack(uint8_t address, uint8_t rw)
{
	TWDR0 = ( rw ) ? ( address | 0x01 ) : ( address & 0xFE );
	TWCR0 = (1 << TWINT) | (1 << TWEN);

	while ( !(TWCR0 & (1 << TWINT))) ;

	if ( (TWSR0 & 0xF8) != ( rw ? TW_MR_SLA_ACK : TW_MT_SLA_ACK ) )
		return 1;
	return 0;
}

// Send or receive a byte of data as Master and send/expect ACK
uint8_t i2c_Master_Data_Ack(uint8_t* data, uint8_t rw)
{
	if ( rw ) { // TW_READ
		TWCR0 = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
	}
	else { // TW_WRITE
		TWDR0 = *data;
		TWCR0 = (1 << TWINT) | (1 << TWEN);
	}
	while ( !(TWCR0 & (1 << TWINT))) ;

	if ( (TWSR0 & 0xF8) != ( rw ? TW_MR_DATA_ACK : TW_MT_DATA_ACK ) )
		return 1;
	*data = TWDR0;
	return 0;
}

// Send or receive a byte of data as Master and send/expect NACK
uint8_t i2c_Master_Data_Nack(uint8_t* data, uint8_t rw)
{
	if ( rw ) { // TW_READ
		TWCR0 = (1 << TWINT) | (1 << TWEN);
	}
	else { // TW_WRITE
		TWDR0 = *data;
		TWCR0 = (1 << TWINT) | (1 << TWEN);
	}
	while ( !(TWCR0 & (1 << TWINT))) ;

	if ( (TWSR0 & 0xF8) != ( rw ? TW_MR_DATA_NACK : TW_MT_DATA_NACK ) )
		return 1;
	*data = TWDR0;
	return 0;
}

// Set Slave addr and wait indefinetly for a Master to call
uint8_t i2c_Slave_Listen(uint8_t address)
{
	TWAR0 = address;
	TWCR0 = (1 << TWEA) | (1 << TWEN) | (1 << TWINT);
	while ( !(TWCR0 & (1 << TWINT)) );
	if ( (TWSR0 & 0xF8) == TW_ST_SLA_ACK )
		return 0;
	return 1;
}

// Send a byte of data as Slave
uint8_t i2c_Slave_Transmit(uint8_t data)
{
	TWDR0 = data;
	TWCR0 = (1 << TWEA) | (1 << TWEN) | (1 << TWINT);
	while ( !(TWCR0 & (1 << TWINT)) );
	if ( (TWSR0 & 0xF8) == TW_ST_DATA_ACK )
		return 0;
	TWCR0 = (1 << TWINT);
	return 1;
}

// Setup the TWI peripheral
// Arg: frequency in kHz
void i2c_Setup(uint8_t frequency)
{
	// Set baud rate reg.
	TWBR0 = (uint32_t)F_CPU / ( (uint32_t)frequency * (uint32_t)1000 );
	PRR0 &= ~(1 << PRTWI0);// Disable power reduction to TWI0
}


void main()
{
	// Setup TWI0 peripheral before using it with baudrate = 100kHz
	i2c_Setup(100);

	// Set Pin PD3 as output
	DDRD |= (1 << PD3);

#ifdef MASTER

	usart_setup_tx();

	_delay_ms(10);

	i2c_Master_Start(TW_START);

	i2c_Master_SLA_Ack(SLA, TW_READ);

	for(uint8_t i = 0; i < 7; i++)
		i2c_Master_Data_Ack(&data[i], TW_READ);

	i2c_Master_Data_Nack(&data[7], TW_READ);

	i2c_Master_Stop();

	for(uint8_t i = 0; i < 8; i++) {
		send_byte_in_hex(data[i]);
		send(" ");
	}
	send("\n\n");

#else

	for(uint8_t i = 0; i < 8; i++)
		data[i] = i + 0xA0;

	i2c_Slave_Listen(SLA);

	uint8_t i = 0;
	while (1) {
		if ( i2c_Slave_Transmit(data[i]) )
			break;
		i = (i+1) % 8;
	}

#endif

	while(1){
		PORTD ^= (1 << PD3);
		_delay_ms(100);
	}
}
