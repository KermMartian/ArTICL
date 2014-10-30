/*************************************************
 *  CBL2.h - Library for emulating CBL2 devices  *
 *           or CBL-connected calculators with   *
 *           Arduinos.                           *
 *           Created by Christopher Mitchell,    *
 *           2011-2014, all rights reserved.     *
 *************************************************/

#include "Arduino.h"
#include "CBL2.h"

// Constructor with default communication lines
CBL2::CBL2() :
	TICL()
{
	return;
}

// Constructor with custom communication lines.
CBL2::CBL2(int tip, int ring) :
	TICL(tip, ring)
{
	return;
}

int CBL2::getFromCBL2(uint8_t type, uint8_t* header, uint8_t* data, int* datalength, int maxlength) {
	// Step 1: Send REQ, wait for ACK and VAR
	
	// Step 2: ACK VAR, send CTS
	
	// Step 3: Receive CTS ACK and DATA
	
	// Step 4: ACK DATA (do NOT perform EOT)
	return -1;
}

int CBL2::sendToCBL2(uint8_t type, uint8_t* header, uint8_t* data, int datalength) {
	return -1;
}

int CBL2::setupCallbacks(uint8_t* header, uint8_t* data, int maxlength,
				   int (*get_callback)(uint8_t, int),
				   int (*send_callback)(uint8_t, int*))
{
	header_ = header;
	data_ = data;
	maxlength_ = maxlength;
	get_callback_ = get_callback;
	send_callback_ = send_callback;
	callback_init = true;
	return 0;
}

int CBL2::eventLoopTick() {
	uint8_t msg_header[4];
	int length;
	int rval;
	int endpoint = 0x12;

	if (!callback_init)
		return -1;
	
	// See if there's a message coming
	rval = get(msg_header, data_, &length, maxlength_);
	if (rval) {
		if (serial_) {
			serial_->print("No incoming message: code ");
			serial_->println(rval);
		}
		return 0;			// No message coming
	}

	// Deduce what kind of operation is happening
	// CBL2 responds to TI-82 as 0x12, "0x95" endpoint as 0x15
	endpoint = (msg_header[0] == CALC82)?0x12:0x15;
	
	// Now deal with the message
	switch(msg_header[1]) {
		case ACK:
			break;						// Drop ACKs on the floor

		case RTS:
			memcpy(header_, data_, length);		// Save the variable header
			
			// Send an ACK
			msg_header[0] = endpoint;
			msg_header[1] = ACK;
			msg_header[2] = msg_header[3] = 0x00;
			send(msg_header, NULL, 0);
			
			// Send a CTS
			msg_header[0] = endpoint;
			msg_header[1] = CTS;
			msg_header[2] = msg_header[3] = 0x00;
			send(msg_header, NULL, 0);
			
			break;
		
		case DATA:
			// Send an ACK
			msg_header[0] = endpoint;
			msg_header[1] = ACK;
			msg_header[2] = msg_header[3] = 0x00;
			send(msg_header, NULL, 0);
			
			// Deliver the data to the callback
			rval = get_callback_(header_[3], length);		// Ignore rval for now	
			break;
	
		case EOT:
			// Send an ACK
			msg_header[0] = endpoint;
			msg_header[1] = ACK;
			msg_header[2] = msg_header[3] = 0x00;
			send(msg_header, NULL, 0);
			break;
		
		case REQ:
			// Send an ACK
			msg_header[0] = endpoint;
			msg_header[1] = ACK;
			msg_header[2] = msg_header[3] = 0x00;
			send(msg_header, NULL, 0);
			
			// Get the header and data from the callback
			send_callback_(header_[3], &datalength_);
			
			// Send the VAR message
			msg_header[0] = endpoint;
			msg_header[1] = VAR;
			msg_header[2] = 0x0B;
			msg_header[3] = 0x00;
			send(msg_header, header_, 0x0B);
			
			break;
			
		case CTS:
			// Send an ACK
			msg_header[0] = endpoint;
			msg_header[1] = ACK;
			msg_header[2] = msg_header[3] = 0x00;
			send(msg_header, NULL, 0);
			
			// Send the DATA
			msg_header[0] = endpoint;
			msg_header[1] = VAR;
			msg_header[2] = (datalength_ & 0x00ff);
			msg_header[3] = (datalength_ >> 8);
			send(msg_header, data_, datalength_);
			
			break;
	}
			
	return 0;
}