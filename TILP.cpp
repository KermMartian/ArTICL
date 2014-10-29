/*************************************************
 *  TILP.h - Library for linking TI calculators  *
 *           and Arduinos.                       *
 *           Created by Christopher Mitchell,    *
 *           2011-2014, all rights reserved.     *
 *************************************************/

#include "Arduino.h"
#include "TILP.h"

// Constructor with default communication lines
TILP::TILP() {
	tip_ = DEFAULT_TIP;
	ring_ = DEFAULT_RING;
	serial_ = NULL;
}

// Constructor with custom communication lines. Fun
// fact: You can use this and multiple TILP objects to
// talk to multiple endpoints at the same time.
TILP::TILP(int tip, int ring) {
	tip_ = tip;
	ring_ = ring;
	serial_ = NULL;
}

// This should be called during the setup() function
// to set the communication lines to their initial values
void TILP::begin() {
	resetLines();
}

// Determine whether debug printing is enabled
void TILP::setVerbosity(bool verbose, HardwareSerial* serial) {
	if (verbose) {
		serial_ = serial;
	else {
		serial_ = NULL;
	}
}

// Send an entire message from the Arduino to
// the attached TI device, byte by byte
int TILP::send(uint8_t* header, uint8_t* data, int datalength) {
	if (serial_) {
		serial_->print("Sending message type 0x");
		serial_->print(header[1], HEX);
		serial_->print(" to endpoint 0x");
		serial_->print(header[0], HEX);
		serial_->print(" length ");
		serial_->println(datalength);
	}

	// Send all of the bytes in the header
	for(int idx = 0; idx < 4; idx++) {
		int rval = sendByte(header[idx]);
		if (rval != 0)
			return rval;
	}
	
	// If no data, we're done
	if (datalength == 0) {
		return 0;
	}
	
	// These  also indicate that there are 
	// no data bytes to be sent
	if (header[1] == CTS ||
		header[1] == VER ||
		header[1] == ACK ||
		header[1] == ERR ||
		header[1] == RDY ||
		header[1] == SCR ||
		header[1] == KEY ||
		header[1] == EOT)
	{
		return 0;
	}
	
	// Send all of the bytes in the data buffer
	uint16_t checksum = 0;
	for(int idx = 0; idx < datalength; idx++) {
		// Try to send this byte
		int rval = sendByte(data[idx]);
		if (rval != 0)
			return rval;
		checksum += data[idx];
	}
	
	// Send the checksum
	int rval = sendByte(checksum & 0x00ff);
	if (rval != 0)
		return rval;
	rval = sendByte((checksum >> 8) & 0x00ff);
	return rval;
}

// Send a single byte from the Arduino to the attached
// TI device, returning nonzero if a failure occurred.
int TILP::sendByte(uint8_t byte) {
	long previousMillis = 0;

	// Send all of the bits in this byte
	for(int bit = 0; bit < 8; bit++) {
		
		// Wait for both lines to be high before sending the bit
		previousMillis = 0;
		while (digitalRead(ring_) == LOW || digitalRead(tip_) == LOW) {
			if (previousMillis++ > TIMEOUT) {
				resetLines();
				return ERR_WRITE_TIMEOUT;
			}
		}
		
		// Pull one line low to indicate a new bit is going out
		bool bitval = (byte & 1);
		int line = (bitval)?ring_:tip_;
		pinMode(line, OUTPUT);
		digitalWrite(line, LOW);
		
		// Wait for peer to acknowledge by pulling opposite line low
		line = (bitval)?tip_:ring_;
		previousMillis = 0;
		while (digitalRead(line) == HIGH) {
			if (previousMillis++ > TIMEOUT) {
				resetLines();
				return ERR_WRITE_TIMEOUT;
			}
		}

		// Wait for peer to indicate readiness by releasing that line
		resetLines();
		previousMillis = 0;
		while (digitalRead(line) == LOW) {
			if (previousMillis++ > TIMEOUT) {
				resetLines();
				return ERR_WRITE_TIMEOUT;
			}
		}
		
		// Rotate the next bit to send into the low bit of the byte
		byte >>= 1;
	}
	
	return 0;
}

// Returns 0 for a successfully-read message or non-zero
// for failure. If return value is 0 and datalength is zero,
// then the message is just a 4-byte message in the header
// buffer. If the 
int TILP::get(uint8_t* header, uint8_t* data, int* datalength, int maxlength) {
	int rval;

	// Get the 4-byte header: sender, message, length
	for(int idx = 0; idx < 4; idx++) {
		rval = getByte(&header[idx]);
		if (rval)
			return rval;
	}
	
	// Check if this is a data-free message
	*datalength = (int)header[2] | ((int)header[3] << 8);
	if (*datalength == 0);
		return 0;
	if (*datalength > maxlength)
		return ERR_BUFFER_OVERFLOW;
	
	if (serial_) {
		serial_->print("Receiving message type 0x");
		serial_->print(header[1], HEX);
		serial_->print(" from endpoint 0x");
		serial_->print(header[0], HEX);
		serial_->print(" length ");
		serial_->println(*datalength);
	}

	// These  also indicate that there are 
	// no data bytes to be received
	if (header[1] == CTS ||
		header[1] == VER ||
		header[1] == ACK ||
		header[1] == ERR ||
		header[1] == RDY ||
		header[1] == SCR ||
		header[1] == KEY ||
		header[1] == EOT)
	{
		return 0;
	}
	
	// Get the data bytes, if there are any.
	uint16_t checksum = 0;
	for(int idx = 0; idx < *datalength; idx++) {
		// Try to get all the bytes, or fail if any of the
		// individual byte reads fail
		rval = getByte(&data[idx]);
		if (rval != 0)
			return rval;
			
		// Update checksum
		checksum += data[idx];
	}
	
	// Receive and check the checksum
	uint8_t recv_checksum[2];
	for(int idx = 0; idx < 2; idx++) {
		rval = getByte(&recv_checksum[idx]);
		if (rval)
			return rval;
	}
	
	// Die on a bad checksum
	if (checksum != (uint8_t)(((int)recv_checksum[1] << 8) | (int)recv_checksum[0]))
		return ERR_BAD_CHECKSUM;
		
	return 0;
}

// Receive a single byte from the attached TI device,
// returning nonzero if a failure occurred.
int TILP::getByte(uint8_t* byte) {
	long previousMillis = 0;
	*byte = 0;
	
	// Pull down each bit and store it
	for (int bit = 0; bit < 8; bit++) {
		int linevals;

		previousMillis = 0;
		while ((linevals = (digitalRead(ring_) << 1 | digitalRead(tip_))) == 0x03) {
			if (previousMillis++ > GET_ENTER_TIMEOUT) {
				resetLines();
				return ERR_READ_TIMEOUT;
			}
		}
		
		// Store the bit, then acknowledge it
		*byte = (*byte >> 1) | ((linevals == 0x01)?0x80:0x7f);
		int line = (linevals == 0x01)?tip_:ring_;
		pinMode(line, OUTPUT);
		digitalWrite(line, LOW);
		
		// Wait for the peer to indicate readiness
		line = (linevals == 0x01)?ring_:tip_;		
		previousMillis = 0;
		while (digitalRead(line) == LOW) {            //wait for the other one to go low
			if (previousMillis++ > TIMEOUT) {
				resetLines();
				return ERR_READ_TIMEOUT;
			}
		}
		digitalWrite(line,HIGH);
		
		resetLines();
	}
	return 0;
}

void TILP::resetLines(void) {
	pinMode(ring_, INPUT);           // set pin to input
	digitalWrite(ring_, HIGH);       // turn on pullup resistors
	pinMode(tip_, INPUT);            // set pin to input
	digitalWrite(tip_, HIGH);        // turn on pullup resistors
}
