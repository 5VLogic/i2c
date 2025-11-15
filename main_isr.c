#define F_CPU 8000000

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "../custom_usart/usart.h"

#define START 0x08
#define RE_START 0x10
#define SLA_R 0b10100001// slave address + read bit
#define SLA_W 0b10100000// slave address + read bit
#define MR_SLA_ACK 0x40// Transmitted SLA+R and ACK received
#define MT_SLA_ACK 0x18// Transmitted SLA+R and ACK received
#define MT_ADDRESS_ACK 0x28// Transmitted ADDRESS and ACK received
#define DATA_RX_ACK 0x50// Data has been received, and ACK returned
#define ADDRESS 0// Address to send

char string[4] = {'x', 'x', '0', 0};
uint8_t data[1024], nibble_high, nibble_low;
volatile uint8_t flag_end_read = 0;
uint16_t i = 0;

ISR(TWI0_vect)
{
	switch (TWSR0 & 0xF8) {
		case START:// Start condition successfully sent
				   // Load SLA_W in data reg, clear twint in control reg
			TWDR0 = SLA_W;
			TWCR0 = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
			break;

		case MT_SLA_ACK:// Received ACK after sending SLA_W
						// Load Data into DR, clear twint in CR, start tx
			TWDR0 = ADDRESS;
			TWCR0 = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
			break;

		case MT_ADDRESS_ACK:// Received ACK after sending addr
							// Send RE-START condition
			TWCR0 = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE);
			break;

		case RE_START:// Received ACK after RE-Start condition sent
					  // Load SLA_R in data reg, clear twint in CR
			TWDR0 = SLA_R;
			TWCR0 = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
			break;

		case MR_SLA_ACK:// Received ACK after sending SLA_R
						// Clear TWINT flag and send ACK back to EEPROM
			TWCR0 |= (1 << TWINT) | (1 << TWEN) | (1 << TWEA) | (1 << TWIE);
			break;

		case DATA_RX_ACK:// Received data from EEPROM
			data[i] = TWDR0;// Store byte into data array
			TWCR0 &= ~(1 << TWSTA) | ~(1 << TWSTO);//
			TWCR0 |= (1 << TWINT) | (1 << TWIE);
			if (i == 1023) {
				TWCR0 &= ~(1 << TWEA);
				TWCR0 = (1 << TWINT) | ( 1<< TWEN) | (1 << TWSTO) | (1 << TWIE);// Transmit STOP
				flag_end_read = 1;
			}
			else TWCR0 = (1 << TWEA) | (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
			i++;
	}
}


void important_task()
{
	PORTD ^= (1 << PD3);
}


void i2c_setup()
{
	// Set prescaler to TWI clock to get 100kHz when F_CPU is 8MHz
	TWBR0 = 80;

	// Disable power reduction to TWI0
	PRR0 &= ~(1 << PRTWI0);
}


void main()
{
	// Setup UART to transmit all data after reading from EEPROM
	usart_setup_tx();

	// Enable global interrupts
	sei();
	
	// Set Pin PD3 as output
	DDRD |= (1 << PD3);

	// Setup TWI0 peripheral before using it with baudrate = 100kHz
	i2c_setup();

	// Enable interrupt when flag is set by HW
	// ALWAYS DO THIS WHILE SETTING TWINT, OTHERWISE RESETTED!!!

	// Send START condition
	TWCR0 = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE);


	while (flag_end_read == 0)
		important_task();


	for(uint16_t i = 0; i < 1024; i++)
	{
		if ( (data[i] > 31) && (data[i] < 128) ) {
			string[0] = data[i];
			string[1] = 32;
		}
		else {
			nibble_high = data[i] >> 4;
			nibble_low = data[i] & 0x0f;
			string[0] = nibble_high + (nibble_high < 10 ? 48 : 55);
			string[1] = nibble_low + (nibble_low < 10 ? 48 : 55);
		}
		if ( (i % 16) == 15 ) string[2] = '\n';
		else string[2] = 32;// 32 is space char
		send(string);
	}

	while(1){
		PORTD ^= (1 << PD3);
		_delay_ms(100);
	}
}

