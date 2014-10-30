/*************************************************
 *  Screenshot.ino                               *
 *  Example from the ArTICL library              *
 *           Created by Christopher Mitchell,    *
 *           2011-2014, all rights reserved.     *
 *                                               *
 *  This demo communicates use the silent-       *
 *  linking commands to dump a screenshot from   *
 *  a connected calculator to the Arduino's      *
 *  serial console.                              *
 *************************************************/

#include <TICL.h>

TICL ticl = TICL(3, 2);

void setup() {
  pinMode(4, INPUT_PULLUP);
  Serial.begin(9600);
  ticl.resetLines();
  delay(1000);
}

void loop() {
  if (!digitalRead(4)) {
    Serial.println("Starting transfer...");
    uint8_t screen[768+2];
    uint8_t msg[4] = {0x73, 0x6D, 0x00, 0x00};
    ticl.send(msg, NULL, 0);
    Serial.println("Sent request");
    int rlen = 0;
    ticl.get(msg, NULL, &rlen, 0);
    Serial.println("Recieved acknowledgement");
    ticl.get(NULL, screen, &rlen, 768+2);
    Serial.println("Recieved screenshot");
    ticl.send(msg, NULL, 0);
    Serial.println("Sent acknowledgement");
    for (int i = 1; i <= 768; i++) {
      for (int j = 7; j > -1; j--) {
        if (screen[i+3] & (1 << j)) {
          Serial.write('#');
        } else {
          Serial.write('.');
        }
      }
      if (i%(12) == 0) {
        Serial.println();
      }
    }
  }
}