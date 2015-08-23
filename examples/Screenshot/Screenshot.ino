/*************************************************
 *  Screenshot.ino                               *
 *  Example from the ArTICL library              *
 *           Created by Christopher Mitchell,    *
 *           2011-2015, all rights reserved.     *
 *                                               *
 *  This demo communicates use the silent-       *
 *  linking commands to dump a screenshot from   *
 *  a connected calculator to the Arduino's      *
 *  serial console.                              *
 *************************************************/

#include <TICL.h>

#if defined(__MSP432P401R__)		// MSP432 target
#define TRIGGER_PRESSED LOW
#define TRIGGER_BUTTON 73
#else								// Arduino target
#define TRIGGER_PRESSED LOW
#define TRIGGER_BUTTON 4
#endif

TICL ticl = TICL(DEFAULT_RING, DEFAULT_TIP);

void setup() {
  pinMode(TRIGGER_BUTTON, INPUT_PULLUP);
  Serial.begin(9600);
  ticl.resetLines();
  ticl.setVerbosity(true, &Serial);
}

void loop() {
  if (TRIGGER_PRESSED == digitalRead(TRIGGER_BUTTON)) {
    Serial.println("Starting transfer...");
    uint8_t screen[768 + 2];
    int rlen = 0, rval = 0;
	
    // Request the screen image
    uint8_t msg[4] = {COMP83P, SCR, 0x00, 0x00};
    rval = ticl.send(msg, NULL, 0);
	if (rval) {
      Serial.print("Failed to send SCR request: ");
	  Serial.println(rval);
	  return;
	}
	
    // Wait for ack
    rval = ticl.get(msg, NULL, &rlen, 0);
	if (rval) {
      Serial.print("Failed to get SCR ack: ");
	  Serial.println(rval);
	  return;
	}
	
    // Wait for screen image
    rval == ticl.get(NULL, screen, &rlen, 768+2);
	if (rval) {
      Serial.print("Failed to get SCR: ");
	  Serial.println(rval);
	  return;
	}
	
    // Send an ack
    rval = ticl.send(msg, NULL, 0);
	if (rval) {
      Serial.print("Failed to send ack: ");
	  Serial.println(rval);
	  return;
	}
	
    // Dump the screen to the serial console
    for (int i = 0; i < 768; i++) {
      for (int j = 7; j >= 0; j--) {
        if (screen[i + 4] & (1 << j)) {
          Serial.write('#');
        } else {
          Serial.write('.');
        }
      }
      if (i % 12 == 0) {
        Serial.println();
      }
    }
  }
}