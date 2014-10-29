/*************************************************
 *  TILP.h - Library for linking TI calculators  *
 *           and Arduinos.                       *
 *           Created by Christopher Mitchell,    *
 *           2011-2014, all rights reserved.     *
 *************************************************/

#ifndef TILP_H
#define TILP_H

#include "Arduino.h"
#include "HardwareSerial.h"

#define DEFAULT_TIP		2			// Tip = red wire
#define	DEFAULT_RING	3			// Ring = white wire

#define TIMEOUT 4000
#define GET_ENTER_TIMEOUT 30000

enum TILPErrors {
	ERR_READ_TIMEOUT = -1,
	ERR_WRITE_TIMEOUT = -2,
	ERR_BAD_CHECKSUM = -3,
	ERR_BUFFER_OVERFLOW = -4,
	ERR_INVALID = -5,
};

enum Endpoint {
	COMP82	= 0x02,
	COMP83	= 0x03,
	COMP83P	= 0x23,
	CALC83P = 0x73,
	CALC82	= 0x82,
	CALC83	= 0x83,
};

enum CommandID {
	VAR		= 0x06,
	CTS		= 0x09,
	DATA	= 0x15,
	VER		= 0x2D,
	SKIP	= 0x36,
	EXIT	= 0x36,
	ACK		= 0x56,
	ERR		= 0x5A,
	RDY		= 0x68,
	SCR		= 0x6D,
	KEY		= 0x87,
	DEL		= 0x88,
	EOT		= 0x92,
	REQ		= 0xA2,
	RTS		= 0xC9,
};

class TILP {
	public:
		TILP();
		TILP(int tip, int ring);
		void begin();
		void setVerbosity(bool verbose, HardwareSerial* serial = NULL);

		int send(uint8_t* header, uint8_t* data, int datalength);
		int get(uint8_t* header, uint8_t* data, int* datalength, int maxlength);
		void resetLines();

	protected:
		HardwareSerial* serial_;

	private:
		int sendByte(uint8_t byte);
		int getByte(uint8_t* byte);

		int tip_;
		int ring_;
};

#endif	// TILP_H