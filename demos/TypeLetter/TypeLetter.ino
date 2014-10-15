#include "TILP.h"

TILP* tilp;
int lineRed = 7;
int lineWhite = 6;

void setup() {
  Serial.begin(9600);
  tilp = new TILP(lineRed, lineWhite);
  tilp->resetLines();
}

void loop() {
  int rlen = 0;
  int rval = 0;
  uint8_t header[4] = {COMP83P, KEY, 0xA6, 0x00};
  rval = tilp->send(header, NULL, 0);              // Send KEY message
  if (rval != 0) {
    Serial.print("Send returned ");
    Serial.println(rval);
  } else {
    tilp->get(header, NULL, &rlen, 0);             // Get ACK
    if (rval != 0) {
      Serial.print("Get returned ");
      Serial.println(rval);
    }
    tilp->get(header, NULL, &rlen, 0);              // Get key process notification
    if (rval != 0) {
      Serial.print("Get returned ");
      Serial.println(rval);
    }
  }
  delay(500);      // 2 'M's per second7
}

