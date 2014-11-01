/*************************************************
 * tivar.h - Library for converting TI-OS var    *
 *           types to/from POSIX var type.       *
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
	static double realToFloat8x(uint8_t* real, uint8_t model = CBL82);
	static int longToReal8x(long int n, uint8_t* real, uint8_t model = CBL85);
	static int floatToReal8x(double f, uint8_t* real, uint8_t model = CBL85);
	static uint16_t sizeWordToInt(uint8_t* ptr);
	static void intToSizeWord(uint16_t size, uint8_t* ptr);

  private:
	static RealType modelToType(uint8_t model);
};
