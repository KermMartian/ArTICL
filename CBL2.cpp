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
	TILP()
{
	return;
}

// Constructor with custom communication lines. Fun
// fact: You can use this and multiple TILP objects to
// talk to multiple endpoints at the same time.
CBL2::CBL2(int tip, int ring) :
	TILP(tip, ring)
{
	return;
}

int CBL2::getFromCBL2(uint8_t type, uint8_t* header, uint8_t* data, int* datalength, int maxlength) {
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
}

int CBL2::eventLoopTick() {
	if (!callback_init)
		return -1;
		
	return -1;
}