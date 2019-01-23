/***************************************************
 * tivar.cpp - Library for converting TI-OS var    *
 *             types to/from POSIX var types.      *
 *             Part of the ArTICL linking library. *
 *             Created by Christopher Mitchell,    *
 *             2011-2015, all rights reserved.     *
 ***************************************************/

#include "TIVar.h"

// Convert a TI real variable into a long long int
long long int TIVar::realToLong8x(uint8_t* real, enum Endpoint model) {
	long long int rval = 0;
    int32_t dec_exp;
	
	// Figure out what type it is
	enum RealType type = modelToType(model);
	if (type == REAL_89) {
		return NAN;			// TI-89/TI-92 not yet implemented! TODO
    }

	// Convert the exponent
	dec_exp = TIVar::extractExponent(real, type) + 14;
	
	// Now extract the number
	const uint8_t mantissa_offset = (type == REAL_82)?2:3;
	for(int i = 0; i < dec_exp; i++) {
		rval *= 10;
		rval += 0x0f & (real[mantissa_offset + (i >> 1)] >> (4 - (4 * (i % 2))));
	}
	
	// Negate the number, if necessary
	if (real[0] & 0x80) {
		rval = 0 - rval;
	}
	
	return rval;
}

// Convert a TI real variable into a double
double TIVar::realToFloat8x(uint8_t* real, enum Endpoint model) {
    const double ieee_lut[10] = {0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f};
    int32_t dec_exp;
	double ieee_acc = 0;
	
	// Figure out what type it is
	enum RealType type = modelToType(model);
	if (type == REAL_89) {
		return NAN;			// TI-89/TI-92 not yet implemented! TODO
    }

	// Convert the exponent
	dec_exp = TIVar::extractExponent(real, type);

	// Convert the mantissa
	const uint8_t mantissa_offset = (type == REAL_82)?2:3;
	for(uint8_t i = 0; i < 14; i++) {
		float digit = ieee_lut[0x0f & (real[mantissa_offset + (i >> 1)] >> ((i & 0x01)?0:4))];
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

// Convert a long long signed integer into a TI real variable
int TIVar::longToReal8x(long long int n, uint8_t* real, enum Endpoint model) {
	int16_t exp = 13;

	// Figure out what type it is
	enum RealType type = modelToType(model);
	if (type == REAL_89)
		return -1;			// TI-89/TI-92 not yet implemented! TODO
    
	// Set sign bit and get absolute value
	real[0] = (n >= 0)?0x00:0x80;
	n = (n > 0)?n:-n;
	
	// Bring large numbers down
	while(n != 0 && n >= 10e13) {
		n /= 10;
		exp += 1;
	}
	
	// Bring small numbers up
	while(n != 0 && n < 1e13) {
		n *= 10;
		exp -= 1;
	}

	// Extract the digits
	const uint8_t mantissa_offset = (type == REAL_82)?2:3;
	for(int8_t i=13; i >= 0; i--) {
		uint8_t cdigit = (uint8_t)(n % 10);

		if ((i & 0x01) == 1) {
			real[mantissa_offset + (i >> 1)] = cdigit;
		} else {
			real[mantissa_offset + (i >> 1)] |= (cdigit << 4);
		}
		n /= 10;
	}
	
	// Set the exponent
	if (type == REAL_82) {
		exp += 0x80;
		real[1] = (uint8_t)exp;

	} else if (type == REAL_85) {
		int32_t temp_exp = (int32_t)exp;
		temp_exp += 0x00fc00;
		real[1] = (uint8_t)(temp_exp & 0x00ff);
		real[2] = (uint8_t)((temp_exp >> 8) & 0x00ff);
	}

	return TIVar::sizeOfReal(model);		// Success: inserted data length
}

// Convert a double into a TI real variable
int TIVar::floatToReal8x(double f, uint8_t* real, enum Endpoint model) {
	int16_t exp = 13;
	
	// Figure out what type it is
	enum RealType type = modelToType(model);
	if (type == REAL_89) {
		return -1;			// TI-89/TI-92 not yet implemented! TODO
    }

	// Set sign bit and get absolute value
	real[0] = (f >= 0)?0x00:0x80;
	f = (f > 0)?f:-f;
	
	// Bring large numbers down
	while(f != 0 && f >= 10.e13) {
		f *= 0.1f;
		exp += 1;
	}
	
	// Bring small numbers up
	while(f != 0 && f < 1.e13) {
		f *= 10.f;
		exp -= 1;
	}
	
	// Extract the digits
	const uint8_t mantissa_offset = (type == REAL_82)?2:3;
	for(int8_t i=13; i >= 0; i--) {
        double digit, odigit;
        digit = odigit = fmod(f, 10.);
		uint8_t cdigit = 0;
		while(digit > 0.5) {
			cdigit++;
			digit -= 1.f;
		}
		
		if ((i & 0x01) == 1) {
			real[mantissa_offset + (i >> 1)] = cdigit;
		} else {
			real[mantissa_offset + (i >> 1)] |= (cdigit << 4);
		}
		f = (f - odigit) / 10.f;
	}
	
	// Set the exponent
	if (type == REAL_82) {
		exp += 0x80;
		real[1] = (uint8_t)exp;

	} else if (type == REAL_85) {
		int32_t temp_exp = (int32_t)exp;
		temp_exp += 0x00fc00;
		real[1] = (uint8_t)(temp_exp & 0x00ff);
		real[2] = (uint8_t)((temp_exp >> 8) & 0x00ff);
	}

	return TIVar::sizeOfReal(model);		// Success: inserted data length
}

// Convert a printable 7-bit ASCII String into a TI string variable
int TIVar::stringToStrVar8x(String s, uint8_t* strVar, enum Endpoint model) {
	uint16_t tokenlen = 0;

	enum StringType type = modelToTypeStr(model);
	int pos = 2; // Leave room for the length word prefix
	if (type == STR_89) {
		pos = 1;
	} else if (type == STR_92) {
		pos = 3;
	}

	for (int i = 0; i < s.length(); i++) {
		uint8_t c = s[i];
		uint16_t t;

		if (c < 0x20 || c >= 0x7f) {
			// Ignore control characters and 8-bit codes
			continue;
		}

		if (type == STR_83) {
			if ((c >= '0' && c <= '9') ||
					   (c >= 'A' && c <= 'Z')) {
				// Map basic characters (0-9, A-Z) directly
				t = c;
			} else if (c >= 'a' && c <= 'k') {
				// Map lowercase letters (group 1)
				t = c - 'a' + 0xbbb0;
			} else if (c >= 'l' && c <= 'z') {
				// Map lowercase letters (group 2)
				t = c - 'l' + 0xbbbc;
			} else {
				// Map punctuation
				switch (c) {
					case ' ':	t = 0x29; break;
					case '!':	t = 0x2d; break;
					case '\"':	t = 0x2a; break;
					case '#':	t = 0xbbd2; break;
					case '$':	t = 0xbbd3; break;
					case '%':	t = 0xbbda; break;
					case '&':	t = 0xbbd4; break;
					case '\'':	t = 0xae; break;
					case '(':	t = 0x10; break;
					case ')':	t = 0x11; break;
					case '*':	t = 0x82; break;
					case '+':	t = 0x70; break;
					case ',':	t = 0x2b; break;
					case '-':	t = 0x71; break;
					case '.':	t = 0x3a; break;
					case '/':	t = 0x83; break;
					case ':':	t = 0x3e; break;
					case ';':	t = 0xbbd6; break;
					case '<':	t = 0x6b; break;
					case '=':	t = 0x6a; break;
					case '>':	t = 0x6c; break;
					case '?':	t = 0xaf; break;
					case '@':	t = 0xbbd1; break;
					case '[':	t = 0x06; break;
					case '\\':	t = 0xbbd7; break;
					case ']':	t = 0x07; break;
					case '^':	t = 0xf0; break;
					case '_':	t = 0xbbd9; break;
					case '`':	t = 0xbbd5; break;
					case '{':	t = 0x08; break;
					case '|':	t = 0xbbd8; break;
					case '}':	t = 0x09; break;
					case '~':	t = 0xbbcf; break;
				}
			}
		} else { // Non-83-type mapping
			// Map all printable characters directly
			t = c;
		}

		// Append the token
		if (t & 0xff00) {
			strVar[pos++] = (t & 0xff00) >> 8;
		}
		strVar[pos++] = (t & 0xff);
		tokenlen++;
	}

	if (type == STR_89) {
		strVar[0] = '\0';
		strVar[pos++] = '\0';
		strVar[pos++] = 0x2d;
	} else if (type == STR_92) {
		TIVar::intToSizeWord(tokenlen + 2, strVar);
		strVar[2] = '\0';
		strVar[pos++] = '\0';
		strVar[pos++] = 0x2d;
	} else {
		TIVar::intToSizeWord(tokenlen, strVar);
	}
	return pos; // Equivalent to the variable's length in bytes
}

// Convert a TI string variable into a printable 7-bit ASCII String
String TIVar::strVarToString8x(uint8_t* strVar, enum Endpoint model) {
	String s;

	enum StringType type = modelToTypeStr(model);
	if (type == STR_89 || type == STR_92) {
		int i = (type == STR_89) ? 1 : 3;
		while (strVar[i]) {
			s.concat(strVar[i]);
			i++;
		}
		return s;
	}

	uint16_t tokenlen = sizeWordToInt(strVar);
	int pos = 2;
	
	for (int i = 0; i < tokenlen; i++) {
		uint8_t c;
		if (type == STR_85) {
			c = s[i];
		} else {
			uint16_t t;
			if (isA2ByteTok(strVar[pos])) {
				t  = strVar[pos++] << 8;
				t |= strVar[pos++];
			} else {
				t  = strVar[pos++];
			}

			if ((t >= 0x30 && t <= 0x39) ||
				(t >= 0x41 && t <= 0x5a)) {
				// Map basic tokens (0-9, A-Z) directly
				c = t;
			} else if (t >= 0xbbb0 && t <= 0xbbba) {
				// Map lowercase letters (group 1)
				c = t + 'a' - 0xbbb0;
			} else if (t >= 0xbbbc && t <= 0xbbca) {
				// Map lowercase letters (group 2)
				c = t + 'l' - 0xbbbc;
			} else {
				// Map punctuation
				switch (t) {
					case 0x29:		c = ' '; break;
					case 0x2d:		c = '!'; break;
					case 0x2a:		c = '\"'; break;
					case 0xbbd2:	c = '#'; break;
					case 0xbbd3:	c = '$'; break;
					case 0xbbda:	c = '%'; break;
					case 0xbbd4:	c = '&'; break;
					case 0xae:		c = '\''; break;
					case 0x10:		c = '('; break;
					case 0x11:		c = ')'; break;
					case 0x82:		c = '*'; break;
					case 0x70:		c = '+'; break;
					case 0x2b:		c = ','; break;
					case 0x71:		c = '-'; break;
					case 0x3a:		c = '.'; break;
					case 0x83:		c = '/'; break;
					case 0x3e:		c = ':'; break;
					case 0xbbd6:	c = ';'; break;
					case 0x6b:		c = '<'; break;
					case 0x6a:		c = '='; break;
					case 0x6c:		c = '>'; break;
					case 0xaf:		c = '?'; break;
					case 0xbbd1:	c = '@'; break;
					case 0x06:		c = '['; break;
					case 0xbbd7:	c = '\\'; break;
					case 0x07:		c = ']'; break;
					case 0xf0:		c = '^'; break;
					case 0xbbd9:	c = '_'; break;
					case 0xbbd5:	c = '`'; break;
					case 0x08:		c = '{'; break;
					case 0xbbd8:	c = '|'; break;
					case 0x09:		c = '}'; break;
					case 0xbbcf:	c = '~'; break;
					default:		c = '?'; break; // Non-ASCII tokens
				}
			}
		}
		s.concat(c);
	}

	return s;
}

bool TIVar::isA2ByteTok(uint8_t a) {
	return (
		a == 0x5c ||
		a == 0x5d ||
		a == 0x5e ||
		a == 0x60 ||
		a == 0x61 ||
		a == 0x62 ||
		a == 0x63 ||
		a == 0x7e ||
		a == 0xaa ||
		a == 0xbb ||
		a == 0xef);
}

// Return the type of real variable used on each model
enum RealType TIVar::modelToType(enum Endpoint model) {
	switch(model) {
		case COMP82:
		case CBL82:
		case CALC82:
			return REAL_82;
			break;
		case COMP83:
		case COMP83P:
		case CALC83P:
		case CALC83:
			return REAL_83;
			break;
		case COMP85:
		case CBL85:
		case CALC85a:
		case CALC85b:
			return REAL_85;
			break;
		case COMP86:
			return REAL_86;
			break;
		case COMP89:
		case CBL89:
		case CALC89:
			return REAL_89;
			break;
		default:
			return REAL_INVALID;
			break;
	}
}

// Return the type of string variable used on each model
enum StringType TIVar::modelToTypeStr(enum Endpoint model) {
	switch(model) {
		case COMP83:
		case COMP83P:
		case CALC83P:
		case CALC83:
			return STR_83;
			break;
		case COMP85:
		case CBL85:
		case CALC85a:
		case CALC85b:
			return STR_85;
			break;
		case COMP86:
			return STR_86;
			break;
		case COMP89:
		case CBL89:
		case CALC89:
			return STR_89;
			break;
		// TODO: The machine ID bytes for 89 are incorrect
		// and causing these cases to not compile.
		// case COMP92:
		// case CBL92:
		// case CALC92:
		// 	return STR_92;
		// 	break;
		default:
			return STR_INVALID;
			break;
	}
}

// Extract the exponent from a real
int32_t TIVar::extractExponent(uint8_t* real, enum RealType type) {
	if (type == REAL_82) {
		return ((int16_t)real[1] - 0x80) - 13;		// decimal point is followed by 13 digits
	} else if (type == REAL_85) {
		int32_t raw_exp = (int32_t)TIVar::sizeWordToInt(&real[1]);
		raw_exp -= 0x00fc00;
		return (int16_t)raw_exp;
	}
}

uint16_t TIVar::sizeWordToInt(uint8_t* ptr) {
	return ((uint16_t)ptr[0]) | (((uint16_t)ptr[1]) << 8);
}

void TIVar::intToSizeWord(uint16_t size, uint8_t* ptr) {
	ptr[0] = (uint8_t)(size & 0x00ff);
	ptr[1] = (uint8_t)(size >> 8);
	return;
}

// Real variable length on each model
// REAL_89 is currently not supported by ArTICL.
int TIVar::sizeOfReal(enum Endpoint model) {
	enum RealType type = modelToType(model);
	switch(type) {
		case REAL_82:
			return 9;
			break;
		case REAL_85:
			return 10;
			break;
		case REAL_89:
		case REAL_INVALID:
			return -1;
			break;
	}
	return -1;
}