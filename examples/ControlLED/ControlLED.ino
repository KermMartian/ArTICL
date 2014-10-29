#include "CBL2.h"

CBL2* cbl;
int lineRed = 7;
int lineWhite = 6;
int ledPin = 13;

#define MAXDATALEN 255
uint8_t header[16];
uint8_t data[MAXDATALEN];

void setup() {
  Serial.begin(9600);
  cbl = new CBL2(lineRed, lineWhite);
  cbl->resetLines();
  cbl->setupCallbacks(header, data, MAXDATALEN,
                      onGetAsCBL2, onSendAsCBL2);
}

void loop() {
  int rval;
  if (rval = cbl->eventLoopTick()) {
    Serial.print("Failed to run eventLoopTick: code ");
    Serial.println(rval);
  }
}

int onGetAsCBL2(uint8_t type, int datalen) {
  Serial.print("Got variable of type ");
  Serial.print(type);
  Serial.println("from calculator.");
  return 0;
}

int onSendAsCBL2(uint8_t type, int* datalen) {
  Serial.print("Got request for variable of type ");
  Serial.print(type);
  Serial.println("from calculator.");
  return -1;
}