/*************************************************
 *  ReadAnalog.ino                               *
 *  Example from the ArTICL library              *
 *           Created by Christopher Mitchell,    *
 *           2011-2014, all rights reserved.     *
 *                                               *
 *  This demo reads the Arduino's six analog     *
 *  pins whenever the calculator requests a      *
 *  list, and returns the results as a six-      *
 *  element list with values between 0 and 1023. *
 *  On the MSP432 Launchpad, it returns the      *
 *  values of A0, A1, A3, A4, A5, A6 instead.    *
 *************************************************/

#include "CBL2.h"
#include "TIVar.h"

CBL2* cbl;
const int lineRed = DEFAULT_TIP;
const int lineWhite = DEFAULT_RING;

#define MAXDATALEN 255
uint8_t header[16];
uint8_t data[MAXDATALEN];

#if defined(__MSP432P401R__)		// MSP432 target
#define ANALOG_PIN_COUNT 6
const int analogPins[ANALOG_PIN_COUNT] = {30, 29, 12, 33, 13, 28};

#else								// Arduino target
#define ANALOG_PIN_COUNT 6
const int analogPins[ANALOG_PIN_COUNT] = {0, 1, 2, 3, 4, 5};

#endif

void setup() {
  Serial.begin(9600);
  cbl = new CBL2(lineRed, lineWhite);
  cbl->resetLines();
  cbl->setVerbosity(true, &Serial);			// Comment this in for mesage information
  cbl->setupCallbacks(header, data, MAXDATALEN,
                      onGetAsCBL2, onSendAsCBL2);
}

void loop() {
  int rval;
  rval = cbl->eventLoopTick();
  if (rval && rval != ERR_READ_TIMEOUT) {
    Serial.print("Failed to run eventLoopTick: code ");
    Serial.println(rval);
  }
}

int onGetAsCBL2(uint8_t type, enum Endpoint model, int datalen) {
  Serial.print("Got variable of type ");
  Serial.print(type);
  Serial.print(" from endpoint of type ");
  Serial.println((int)model);
  return 0;
}

int onSendAsCBL2(uint8_t type, enum Endpoint model, int* headerlen,
                 int* datalen, data_callback* data_callback)
{
  Serial.print("Got request for variable of type ");
  Serial.print(type);
  Serial.print(" from endpoint of type ");
  Serial.println((int)model);
  
  if (type != VarTypes82::VarRList)
    return -1;
  
  // Compose the VAR header
  *datalen = 2 + TIVar::sizeOfReal(model) * ANALOG_PIN_COUNT;
  TIVar::intToSizeWord(*datalen, &header[0]);	// Two bytes for the element count, ANALOG_PIN_COUNT Reals
                                                // This sets header[0] and header[1]
  header[2] = VarTypes85::VarRList;             // RealList (if you're a TI-85. Bleh.)
  header[3] = 0x01;	               // Name length
  header[4] = 0x41;                // "A", as per "standard" See http://www.cemetech.net/forum/viewtopic.php?p=224739#224739
  header[5] = 0x00;                // Zero terminator (remainder of header is ignored)
  *headerlen = 11;
  
  // Compose the body of the variable
  data[0] = ANALOG_PIN_COUNT;		// Little-endian word for number of
  data[1] = 0;						// elements in this list
  int offset = 2;					// Offset past the count word
  for(int i = 0; i < ANALOG_PIN_COUNT; i++) {
	long value = analogRead(analogPins[i]);
	// Convert the value, get the length of the inserted data or -1 for failure
	int rval = TIVar::longToReal8x(value, &data[offset], model);
	if (rval < 0) {
		return -1;
	}
	offset += rval;
  }
  for(int i = 0; i < *datalen; i++) {
    Serial.print(data[i], HEX);
	Serial.print(" ");
  }
  Serial.println("]]");

  return 0;
}