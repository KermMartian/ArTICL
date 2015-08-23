/*************************************************
 *  TICL.h - Core of ArTICL library for linking  *
 *           TI calculators and Arduinos.        *
 *           Created by Christopher Mitchell,    *
 *           2011-2015, all rights reserved.     *
 *************************************************/

#ifndef TICL_H
#define TICL_H

#include "Arduino.h"
#include "HardwareSerial.h"

#define TIMEOUT 1000				// microseconds (1ms)
#define GET_ENTER_TIMEOUT 1000000	// microseconds (1s)

#if defined(__MSP432P401R__)		// MSP432 target
#define DEFAULT_TIP		17			// Tip = red wire (GPIO 5.7)
#define	DEFAULT_RING	37			// Ring = white wire (GPIO 5.6)
#else								// Arduino target
#define DEFAULT_TIP		2			// Tip = red wire
#define	DEFAULT_RING	3			// Ring = white wire
#endif

enum TICLErrors {
	ERR_READ_TIMEOUT = -1,
	ERR_WRITE_TIMEOUT = -2,
	ERR_BAD_CHECKSUM = -3,
	ERR_BUFFER_OVERFLOW = -4,
	ERR_INVALID = -5,
	ERR_READ_ENTER_TIMEOUT = -6
};

enum Endpoint {
	COMP82	= 0x02,
	COMP83	= 0x03,
	COMP85  = 0x05,
	COMP86  = 0x06,
	COMP89  = 0x09,
	COMP92  = 0x09,
	CBL82   = 0x12,
	CBL85   = 0x15,
	CBL89   = 0x19,
	CBL92   = 0x19,
	COMP83P	= 0x23,
	CALC83P = 0x73,
	CALC82	= 0x82,
	CALC83	= 0x83,
	CALC85a = 0x85,
	CALC89  = 0x89,
	CALC92  = 0x89,
	CALC85b = 0x95,
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

class TICL {
	public:
		TICL();
		TICL(int tip, int ring);
		void begin();
		void setVerbosity(bool verbose, HardwareSerial* serial = NULL);

		int send(uint8_t* header, uint8_t* data, int datalength, uint8_t(*data_callback)(int) = NULL);
		int get(uint8_t* header, uint8_t* data, int* datalength, int maxlength);
		void resetLines();

	protected:
		HardwareSerial* serial_;

	private:
		int sendByte(uint8_t byte);
		int getByte(uint8_t* byte);
		int digitalSafeRead(int pin);

		int tip_;
		int ring_;
};

#endif	// TICL_H