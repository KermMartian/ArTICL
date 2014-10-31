/***************************************************
 * tivar.cpp - Library for converting TI-OS var    *
 *             types to/from POSIX var type.       *
 *             Part of the ArTICL linking library. *
 *             Created by Christopher Mitchell,    *
 *             2011-2014, all rights reserved.     *
 ***************************************************/

#include "TIVar.h"

float TIVar::realToFloat8x(uint8_t* real) {
    const float ieee_lut[10] = {0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f};
    int16_t dec_exp;
	float ieee_acc = 0;
    
	// Convert the exponent
	dec_exp = ((int16_t)real[1] - 0x80) - 13;		// decimal point is followed by 13 digits

	// Convert the mantissa
	for(uint8_t i = 0; i < 14; i++) {
		float digit = ieee_lut[0x0f & (real[2 + (i >> 1)] >> ((i & 0x01)?0:4))];
		ieee_acc = (10 * ieee_acc) + digit;
	}
	
	// Raise mantissa to a positive exponent
	while(dec_exp > 0) {
		ieee_acc *= 10;
		dec_exp--;
	}
	
	// Lower mantissa to a negative exponent
	while(dec_exp < 0) {
		ieee_acc *= 0.1f;
		dec_exp++;
	}
	
	// Negate the number, if necessary
	if (real[0] & 0x80) {
		ieee_acc *= -1;
	}
	
	return ieee_acc;
}

