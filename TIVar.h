/*************************************************
 * tivar.h - Library for converting TI-OS var    *
 *           types to/from POSIX var types.      *
 *           Part of the ArTICL linking library. *
 *           Created by Christopher Mitchell,    *
 *           2011-2014, all rights reserved.     *
 *************************************************/

#include "Arduino.h"
#include "TICL.h"

// Used internally
enum RealType {
	REAL_INVALID = -1,
	REAL_82 = 1,
	REAL_83 = 1,
	REAL_85 = 2,
	REAL_86 = 2,
	REAL_89 = 3,			// Same for TI-92
};

class TIVar {
  public:
	static long long int realToLong8x(uint8_t* real, enum Endpoint model);
	static double realToFloat8x(uint8_t* real, enum Endpoint model = CBL82);
	static int longToReal8x(long long int n, uint8_t* real, enum Endpoint model = CBL85);
	static int floatToReal8x(double f, uint8_t* real, enum Endpoint model = CBL85);
	static int stringToStrVar8x(String s, uint8_t* strVar, enum Endpoint model = CBL85);
	static String strVarToString8x(uint8_t* strVar, enum Endpoint model = CBL85);
	static uint16_t sizeWordToInt(uint8_t* ptr);
	static void intToSizeWord(uint16_t size, uint8_t* ptr);
	static int sizeOfReal(enum Endpoint model);

  private:
	static bool isA2ByteTok(uint8_t a);
	static int32_t extractExponent(uint8_t* real, enum RealType type);
	static RealType modelToType(enum Endpoint model);
};
