#include "TILP.h"

TILP* tilp;
int lineRed = 7;
int lineWhite = 6;
int ledPin = 13;

void setup() {
  tilp = new TILP(lineRed, lineWhite);
  tilp->resetLines();
}

void loop() {
}
