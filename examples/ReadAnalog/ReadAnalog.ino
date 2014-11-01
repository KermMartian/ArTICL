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
 *************************************************/

#include "CBL2.h"
#include "TIVar.h"

CBL2* cbl;
int lineRed = 7;
int lineWhite = 6;

#define MAXDATALEN 255
uint8_t header[16];
uint8_t data[MAXDATALEN];

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

int onGetAsCBL2(uint8_t type, int datalen) {
  Serial.print("Got variable of type ");
  Serial.print(type);
  Serial.println(" from calculator.");
  return 0;
}

int onSendAsCBL2(uint8_t type, int* datalen) {
  Serial.print("Got request for variable of type ");
  Serial.print(type);
  Serial.println(" from calculator.");
  
  if (type != 0x01)
    return -1;
  
  // Compose the VAR header
  *datalen = 2 + 9 * 6;
  header[0] = *datalen;			// Two bytes for the element count, 6 9-byte reals
  header[1] = 0;
  header[2] = 0x04;
  header[3] = 0x01;
  header[4] = 0x41;				// See http://www.cemetech.net/forum/viewtopic.php?p=224739#224739
  header[5] = 0x00;
  
  // Compose the body of the variable
  data[0] = 6;
  data[1] = 0;
  for(int i = 0; i < 6; i++) {
	float value = analogRead(i);
	TIVar::floatToReal8x(value, &data[2 + 9 * i]);
  }
  for(int i = 0; i < *datalen; i++) {
    Serial.print(data[i], HEX);
	Serial.print(" ");
  }
  Serial.println("]]");

  return 0;
}