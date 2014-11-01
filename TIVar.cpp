/***************************************************
 * tivar.cpp - Library for converting TI-OS var    *
 *             types to/from POSIX var type.       *
 *             Part of the ArTICL linking library. *
 *             Created by Christopher Mitchell,    *
 *             2011-2014, all rights reserved.     *
 ***************************************************/

#include "TIVar.h"

double TIVar::realToFloat8x(uint8_t* real) {
    const double ieee_lut[10] = {0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f};
    int32_t dec_exp;
	double ieee_acc = 0;
    
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

int floatToReal8x(double f, uint8_t* real) {
	int16_t exp = 0;
	
	// Set sign bit and get absolute value
	real[0] = (f >= 0)?0x00:0x80;
	f = (f > 0)?f:-f;
	
	// Bring large numbers down
	while(f != 0 && f >= 10.e14) {
		f /= 10.f;
	}
	
	// Bring small numbers up
	while(f != 0 && f < 1.e14) {
		f *= 10.f;
	}
	
	// Extract the digits
	for(uint8_t i=13; i >= 0; i--) {
		double digit = fmod(f, 10.);
		uint8_t cdigit = 0;
		while(digit > 0.5) {
			cdigit++;
			digit -= 1.f;
		}
		
		if ((i & 0x01) == 1) {
			real[2 + (i >> 2)] = cdigit;
		} else {
			real[2 + (i >> 2)] |= (cdigit << 4);
		}
		f = (f - digit) / 10.f;
	}
	
	// Set the exponent
	exp += 0x80;
	real[1] = (uint8_t)exp;

	return 0;		// Success
}
