/*************************************************
 * tivar.h - Library for converting TI-OS var    *
 *           types to/from POSIX var type.       *
 *           Part of the ArTICL linking library. *
 *           Created by Christopher Mitchell,    *
 *           2011-2014, all rights reserved.     *
 *************************************************/

#include "Arduino.h"

class TIVar {
  public:
	static float realToFloat8x(uint8_t* real);
};
