/*************************************************
 *  CBL2.h - Library for emulating CBL2 devices  *
 *           or CBL-connected calculators with   *
 *           Arduinos.                           *
 *           Created by Christopher Mitchell,    *
 *           2011-2019, all rights reserved.     *
 *************************************************/

#include "Arduino.h"
#include "CBL2.h"
#include "TIVar.h"

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
	uint8_t msg_header[4];
	uint8_t endpoint = (type == 0x01)?CALC85b:CALC82;	// CALC82 for strings and other types, CALC85b for lists
	int length;
	int rval;
	
	// Step 1: Send REQ, wait for ACK and VAR
	// We will assume that the CBL2 can use 11-byte (TI-82/TI-83/TI-85-style)
	// variable headers when we send messages with a CALC82 endpoint
	msg_header[0] = endpoint;
	msg_header[1] = REQ;
	TIVar::intToSizeWord(11, &msg_header[2]);
	rval = send(msg_header, header, 11);
	
	if (rval || get(msg_header, NULL, &length, 0) || msg_header[1] != ACK) {
		// Either the message was not an ACK, or we didn't even get a message
		return -1;
	}
	
	if (get(msg_header, header, &length, 11) || msg_header[1] != VAR) {
		// Either the message was not a VAR, or we didn't even get a message
		return -1;
	}

	// Step 2: ACK VAR, send CTS
	msg_header[0] = endpoint;
	msg_header[1] = ACK;
	msg_header[2] = msg_header[3] = 0;
	rval = send(msg_header, NULL, 0);
	if (rval) {
		// The send did not complete successfully
		return -1;
	}
	
	msg_header[1] = CTS;
	rval = send(msg_header, NULL, 0);

	// Step 3: Receive CTS ACK and DATA
	if (rval || get(msg_header, NULL, &length, 0) || msg_header[1] != ACK) {
		// Either the message was not an ACK, or we didn't even get a message
		return -1;
	}

	if (get(msg_header, NULL, datalength, maxlength) || msg_header[1] != DATA) {
		// Either the message was not a DATA, or we didn't even get a message
		return -1;
	}
	
	// Step 4: ACK DATA (do NOT perform EOT)
	msg_header[0] = endpoint;
	msg_header[1] = ACK;
	msg_header[2] = msg_header[3] = 0;
	return send(msg_header, NULL, 0);
}

int CBL2::sendToCBL2(uint8_t type, uint8_t* header, uint8_t* data, int datalength) {
	uint8_t msg_header[4];
	uint8_t endpoint = (type == 0x01) ? CALC85b : CALC82;	// CALC82 for strings and other types, CALC85b for lists
	int length;
	int rval;

	// Step 1: Send RTS, wait for RTS ACK
	// We will assume that the CBL2 can use 11-byte (TI-82/TI-83/TI-85-style)
	// variable headers when we send messages with a CALC82 endpoint
	msg_header[0] = endpoint;
	msg_header[1] = RTS;
	msg_header[2] = 11;
	msg_header[3] = 0;
	rval = send(msg_header, header, 11);
	
	if (rval || (rval = get(msg_header, NULL, &length, 0)) || msg_header[1] != ACK) {
		// Either the message was not an ACK, or we didn't even get a message
		return -1;
	}

	// Step 2: Wait for CTS, send CTS ACK
	if (get(msg_header, NULL, &length, 0) || msg_header[1] != CTS) {
		// Either the message was not a CTS, or we didn't even get a message
		return -1;
	}
	
	msg_header[0] = endpoint;
	msg_header[1] = ACK;
	msg_header[2] = msg_header[3] = 0;
	rval = send(msg_header, NULL, 0);
	if (rval) {
		// The send did not complete successfully
		return -1;
	}

	// Step 3: Send DATA, wait for DATA ACK
	msg_header[0] = endpoint;
	msg_header[1] = DATA;
	TIVar::intToSizeWord(11, &msg_header[2]);
	rval = send(msg_header, data, datalength);
	
	if (rval || get(msg_header, NULL, &length, 0) || msg_header[1] != ACK) {
		// Either the message was not an ACK, or we didn't even get a message
		return -1;
	}

	// Step 4: Send EOT and wait for EOT ACK
	msg_header[0] = endpoint;
	msg_header[1] = EOT;
	msg_header[2] = msg_header[3] = 0;
	rval = send(msg_header, NULL, 0);
	
	if (rval || get(msg_header, NULL, &length, 0) || msg_header[1] != ACK) {
		// Either the message was not an ACK, or we didn't even get a message
		return -1;
	}
	return 0;
}

int CBL2::setupCallbacks(uint8_t* header, uint8_t* data, int maxlength,
				   int (*get_callback)(uint8_t, enum Endpoint, int),
				   int (*send_callback)(uint8_t, enum Endpoint, int*, int*, data_callback*))
{
	header_ = header;
	data_ = data;
	maxlength_ = maxlength;
	get_callback_ = get_callback;
	send_callback_ = send_callback;
	callback_init = true;
	return 0;
}

int CBL2::eventLoopTick(bool quick_fail) {
	uint8_t msg_header[4];
	int length;
	int rval;
	int endpoint = CBL82;

	if (!callback_init) {
		return -1;
	}
	
	// See if there's a message coming
	rval = get(msg_header, data_, &length, maxlength_, quick_fail ? TIMEOUT : GET_ENTER_TIMEOUT);
	if (rval) {
		if (serial_) {
			serial_->print("No msg: code ");
			serial_->println(rval);
		}
		return 0;			// No message coming
	}

	// Deduce what kind of operation is happening
	// CBL2 responds to TI-82 as 0x12, "0x95" endpoint as 0x15
	enum Endpoint model = (enum Endpoint)msg_header[0];
	switch(model) {
		case CALC82:
			endpoint = CBL82;
			break;
		case CALC85a:
		case CALC85b:
			endpoint = CBL85;
			break;
		case CALC89:
			endpoint = CBL89;
			break;
		case COMP83:
			endpoint = CALC83;
			break;
		case COMP83P:
			endpoint = CALC83P;
			break;
		default:
			return -1;				// Unknown endpoint
	};
	
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
			rval = send(msg_header, NULL, 0);
			if (rval) {
				// The send did not complete successfully
				break;
			}
			
			// Send a CTS
			msg_header[0] = endpoint;
			msg_header[1] = CTS;
			msg_header[2] = msg_header[3] = 0x00;
			rval = 10*send(msg_header, NULL, 0);
			
			break;
		
		case DATA:
			// Send an ACK
			msg_header[0] = endpoint;
			msg_header[1] = ACK;
			msg_header[2] = msg_header[3] = 0x00;
			rval = send(msg_header, NULL, 0);
			if (rval) {
				// The send did not complete successfully
				break;
			}
			
			// Deliver the data to the callback
			normalizeVariableHeader(model);			// Deal with all the wacky way headers can be constructed
			rval = get_callback_(header_[2], model, length);	// Ignore rval for now	
			break;
	
		case EOT:
			// Send an ACK
			msg_header[0] = endpoint;
			msg_header[1] = ACK;
			msg_header[2] = msg_header[3] = 0x00;
			rval = send(msg_header, NULL, 0);
			break;
		
		case REQ: {
			memcpy(header_, data_, length);		// Save the variable header

			// Send an ACK
			msg_header[0] = endpoint;
			msg_header[1] = ACK;
			msg_header[2] = msg_header[3] = 0x00;
			rval = send(msg_header, NULL, 0);
			if (rval) {
				// The send did not complete successfully
				break;
			}
			
			// Get the header and data from the callback
			data_callback_ = NULL;
			int headerlength = length;
			uint8_t tmp_header[16];
			normalizeVariableHeader(model);			// Deal with all the wacky way headers can be constructed
			memcpy(tmp_header, header_, 16);		// Save it...
			send_callback_(header_[2], model,
			               &headerlength, &datalength_, &data_callback_);
			// Copy in the size.
			tmp_header[0] = header_[0];
			tmp_header[1] = header_[1];
			memcpy(header_, tmp_header, 16);		// ...and restore it
			
			// Send the VAR message
			msg_header[0] = endpoint;
			msg_header[1] = VAR;
			msg_header[2] = headerlength;
			msg_header[3] = 0x00;
			rval = send(msg_header, header_, headerlength);
		  }
		  break;
			
		case CTS:
			// Send an ACK
			msg_header[0] = endpoint;
			msg_header[1] = ACK;
			msg_header[2] = msg_header[3] = 0x00;
			rval = send(msg_header, NULL, 0);
			if (rval) {
				// The send did not complete successfully
				break;
			}
			
			// Send the DATA
			msg_header[0] = endpoint;
			msg_header[1] = DATA;
			msg_header[2] = (datalength_ & 0x00ff);
			msg_header[3] = (datalength_ >> 8);
			rval = send(msg_header, data_, datalength_, data_callback_);
			
			break;
	}

	return rval;
}

void CBL2::normalizeVariableHeader(const int model) {
	if ((model == CALC82 || model == CALC85b) && header_[2] == VarTypes82::VarString && header_[3] == VarTypes82::VarRList) {
		// Real list from "TI-82" (could be TI-84+SE or TI-84+CSE , variable name encoded with some odd format
		header_[2] = VarTypes82::VarRList;
	} else if (model == CALC82 && header_[2] == VarTypes82::VarReal && header_[3] == 0xAA /* tVarStr */) {
		header_[2] = VarTypes82::VarString;
	} else if (model == CALC82 && header_[2] == VarTypes82::VarReal && header_[3] == 0x5E /* tVarYVar */) {
        header_[2] = VarTypes82::VarYVar;
    } else if (model == CALC82 && header_[2] == VarTypes82::VarReal && header_[3] == 0x60 /* tVarPic */) {
        header_[2] = VarTypes82::VarPic;
    }
}
