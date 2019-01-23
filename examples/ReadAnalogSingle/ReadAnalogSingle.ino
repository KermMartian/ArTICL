/*************************************************
 *  ReadAnalogSingle.ino                         *
 *  Example from the ArTICL library              *
 *           Created by Christopher Mitchell,    *
 *           2011-2019, all rights reserved.     *
 *************************************************/
#include "CBL2.h"
#include "TIVar.h"

#define MAXDATALEN 255

uint8_t header[16];
uint8_t data[MAXDATALEN];

CBL2 cbl;
const int lineRed = 7;
const int lineWhite = 6;

// Forward declaration of onRequest() and onReceived() functions
int onReceived(uint8_t type, enum Endpoint model, int datalen);
int onRequest(uint8_t type, enum Endpoint model, int* headerlen,
              int* datalen, data_callback* data_callback);

void setup() {
    Serial.begin(9600);
    cbl.setLines(lineRed, lineWhite);
    cbl.resetLines();
    //cbl.setVerbosity(true, &Serial);      // Comment this in for message information
    cbl.setupCallbacks(header, data, MAXDATALEN, onReceived, onRequest);
}

void loop() {
    cbl.eventLoopTick();
}

int onReceived(uint8_t type, enum Endpoint model, int datalen) {
    // do nothing
    return 0;
}

// This is triggered when the attached calculator calls Get()
int onRequest(uint8_t type, enum Endpoint model, int* headerlen,
              int* datalen, data_callback* data_callback)
{
    if (type != VarTypes82::VarReal) {
        Serial.println("Received request for invalid data type");
        return -1; // Can only return a real
    }

	// This is the value that we're going to send back
    int val = analogRead(0);
	// Convert it into a TI-formatted real number for the correct type of calculator,
	// and get back how many bytes that real number takes up. We convert it directly
	// into the body of the packet we'll be returning to the calculator (data).
    *datalen = TIVar::longToReal8x((long long int)val, data, model);

	// Clear the header to be sent, then fill in the size of the real number
    memset(header, 0, sizeof(header));
    TIVar::intToSizeWord(*datalen, header);
	// Adjust the variable type and name
    header[2] = VarTypes82::VarReal; // Variable type
    header[3] = 'A'; // variable A
    header[4] = 0x00; // pointless zero termination
    *headerlen = 13;

	// ArTICL will take it from here.
    return 0;
}