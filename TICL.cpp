/*************************************************
 * TICL.cpp - Core of ArTICL library for linking *
 *            TI calculators and Arduinos.       *
 *            Created by Christopher Mitchell,   *
 *            2011-2014, all rights reserved.    *
 *************************************************/

#include "Arduino.h"
#include "TICL.h"

// Constructor with default communication lines
TICL::TICL() {
	tip_ = DEFAULT_TIP;
	ring_ = DEFAULT_RING;
	serial_ = NULL;
}

// Constructor with custom communication lines. Fun
// fact: You can use this and multiple TICL objects to
// talk to multiple endpoints at the same time.
TICL::TICL(int tip, int ring) {
	tip_ = tip;
	ring_ = ring;
	serial_ = NULL;
}

// This should be called during the setup() function
// to set the communication lines to their initial values
void TICL::begin() {
	resetLines();
}

// Determine whether debug printing is enabled
void TICL::setVerbosity(bool verbose, HardwareSerial* serial) {
	if (verbose) {
		serial_ = serial;
	} else {
		serial_ = NULL;
	}
}

// Send an entire message from the Arduino to
// the attached TI device, byte by byte
int TICL::send(uint8_t* header, uint8_t* data, int datalength, uint8_t(*data_callback)(int)) {
	if (serial_) {
		serial_->print("snd type 0x");
		serial_->print(header[1], HEX);
		serial_->print(" as EP 0x");
		serial_->print(header[0], HEX);
		serial_->print(" len ");
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
		uint8_t outbyte;
		// Get a byte if we need
		if (data_callback != NULL) {
			outbyte = data_callback(idx);
		} else {
			outbyte = data[idx];
		}
		// Try to send this byte
		int rval = sendByte(outbyte);
		if (rval != 0)
			return rval;
		checksum += outbyte;
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
int TICL::sendByte(uint8_t byte) {
	unsigned long previousMicros = 0;

	// Send all of the bits in this byte
	for(int bit = 0; bit < 8; bit++) {
		
		// Wait for both lines to be high before sending the bit
		previousMicros = micros();
		while (digitalRead(ring_) == LOW || digitalRead(tip_) == LOW) {
			if (micros() - previousMicros > TIMEOUT) {
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
		previousMicros = micros();
		while (digitalRead(line) == HIGH) {
			if (micros() - previousMicros > TIMEOUT) {
				resetLines();
				return ERR_WRITE_TIMEOUT;
			}
		}

		// Wait for peer to indicate readiness by releasing that line
		resetLines();
		previousMicros = micros();
		while (digitalRead(line) == LOW) {
			if (micros() - previousMicros > TIMEOUT) {
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
int TICL::get(uint8_t* header, uint8_t* data, int* datalength, int maxlength) {
	int rval;

	// Get the 4-byte header: sender, message, length
	for(int idx = 0; idx < 4; idx++) {
		rval = getByte(&header[idx]);
		if (rval)
			return rval;
	}
	*datalength = (int)header[2] | ((int)header[3] << 8);
	
	if (serial_) {
		serial_->print("Recv typ 0x");
		serial_->print(header[1], HEX);
		serial_->print(" from EP 0x");
		serial_->print(header[0], HEX);
		serial_->print(" len ");
		serial_->println(*datalength);
	}

	if (*datalength == 0)
		return 0;

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
	
	// Check if this is a data-free message
	if (*datalength > maxlength) {
		if (serial_) {
			serial_->print("Msg buf ovfl: ");
			serial_->print(*datalength);
			serial_->print(" > ");
			serial_->println(maxlength);
		}
		return ERR_BUFFER_OVERFLOW;
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
	if (checksum != (uint16_t)(((int)recv_checksum[1] << 8) | (int)recv_checksum[0]))
		return ERR_BAD_CHECKSUM;
	
	return 0;
}

// Receive a single byte from the attached TI device,
// returning nonzero if a failure occurred.
int TICL::getByte(uint8_t* byte) {
	unsigned long previousMicros = 0;
	*byte = 0;
	
	// Pull down each bit and store it
	for (int bit = 0; bit < 8; bit++) {
		int linevals;

		previousMicros = 0;
		while ((linevals = ((digitalRead(ring_) << 1) | digitalRead(tip_))) == 0x03) {
			if (micros() - previousMicros > GET_ENTER_TIMEOUT) {
				resetLines();
				if (serial_) { serial_->print("died waiting for bit "); serial_->println(bit); }
				return ERR_READ_ENTER_TIMEOUT;
			}
		}
		
		// Store the bit, then acknowledge it
		*byte = (*byte >> 1) | ((linevals == 0x01)?0x80:0x00);
		int line = (linevals == 0x01)?tip_:ring_;
		pinMode(line, OUTPUT);
		digitalWrite(line, LOW);
		
		// Wait for the peer to indicate readiness
		line = (linevals == 0x01)?ring_:tip_;		
		previousMicros = 0;
		while (digitalRead(line) == LOW) {            //wait for the other one to go high again
			if (micros() - previousMicros > TIMEOUT) {
				resetLines();
				if (serial_) { serial_->print("died waiting for bit ack "); serial_->println(bit); }
				return ERR_READ_TIMEOUT;
			}
		}

		// Now set them both high and to input
		resetLines();
	}
	if (serial_) { serial_->print("Got byte "); serial_->println(*byte); }
	return 0;
}

void TICL::resetLines(void) {
	pinMode(ring_, INPUT_PULLUP);           // set pin to input with pullups
	pinMode(tip_, INPUT_PULLUP);            // set pin to input with pullups
}
