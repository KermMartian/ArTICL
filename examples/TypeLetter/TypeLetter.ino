/*************************************************
 *  TypeLetter.ino                               *
 *  Example from the ArTICL library              *
 *           Created by Christopher Mitchell,    *
 *           2011-2015, all rights reserved.     *
 *                                               *
 *  This demo communicates use the silent-       *
 *  linking commands to type the letter M on a   *
 *  connected calculator.                        *
 *************************************************/

#include "TICL.h"

TICL* ticl;
int lineRed = DEFAULT_TIP;
int lineWhite = DEFAULT_RING;

void setup() {
  Serial.begin(9600);
  ticl = new TICL(lineRed, lineWhite);
  ticl->resetLines();
  ticl->setVerbosity(true, &Serial);
}

void loop() {
  int rlen = 0;
  int rval = 0;
  uint8_t header[4] = {COMP83P, KEY, 0xA6, 0x00};
  rval = ticl->send(header, NULL, 0);              // Send KEY message
  if (rval != 0) {
    Serial.print("Send returned ");
    Serial.println(rval);
  } else {
    rval = ticl->get(header, NULL, &rlen, 0);             // Get ACK
    if (rval != 0) {
      Serial.print("Get returned ");
      Serial.println(rval);
    } else {
      rval = ticl->get(header, NULL, &rlen, 0);              // Get key process notification
      if (rval != 0) {
        Serial.print("Get returned ");
        Serial.println(rval);
      }
	}
  }
  delay(500);      // 2 'M's per second
}

