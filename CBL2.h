/*************************************************
 *  CBL2.h - Library for emulating CBL2 devices  *
 *           or CBL-connected calculators with   *
 *           Arduinos.                           *
 *           Created by Christopher Mitchell,    *
 *           2011-2014, all rights reserved.     *
 *************************************************/

#ifndef TILP_H
#define TILP_H

#include "Arduino.h"
#include "TILP.h"

enum VarTypes {
	VarReal = 0,
	VarRList = 1,
	VarMatrix = 2,
	VarYVar = 3,
	VarString = 4,
	VarProgram = 5,
	VarProtProg = 6,
	VarPic = 7,
	VarGDB = 8,
	VarWindow = 0x0b,
	VarComplex = 0x0c,
	
class CBL2: public TILP {
	public:
		CBL2();
		CBL2(int tip, int ring);
		int getFromCBL2(uint8_t type, uint8_t* header, uint8_t* data, int* datalength, int maxlength);
		int sendToCBL2(uint8_t type, uint8_t* header, uint8_t* data, int datalength);
};

#endif	// TILP_H