/*************************************************
 *  SimpleIO.ino                                 *
 *  Example from the ArTICL library              *
 *           Created by Christopher Mitchell,    *
 *           2011-2016, all rights reserved.     *
 *                                               *
 *  This demo communicates as if it was a CBL2   *
 *  device. Use Send({0}) to send a 1-element    *
 *  list to the Arduino and control the state    *
 *  of digital output lines. You can expand      *
 *  this demo to read or write any GPIO lines.   *
 *                                               *
 *  In its current state, takes a 4-element list,*
 *  the elements of which respectively turn a    *
 *  red, green, and blue LED on and off (0 or 1),*
 *  and set a motor's speed (0-255). A 2-element *
 *  list can be requested from the Arduino, ind- *
 *  icating the state of two digital inputs,     *
 *  meant to be connected to a pushbutton and an *
 *  SPST switch, respectively.                   *
 *                                               *
 *  This example is intended to function out of  *
 *  the box with rfdave's Arduino globalCALCnet  *
 *  shield; see https://www.cemetech.net/forum/  *
 *  viewtopic.php?t=10694 . If you're using this *
 *  example with another shield or without a     *
 *  shield, remember to adjust lineRed/lineWhite.*
 *************************************************/

#include "CBL2.h"
#include "TIVar.h"

CBL2 cbl;
const int lineRed = 7;
const int lineWhite = 6;

// Specify the pins for input and output
// 
#if defined(__MSP432P401R__)    // MSP432 target
#define LED_PIN_R 75
#define LED_PIN_G 76
#define LED_PIN_B 77
#define MOTOR_PIN 11
#define BUTTON_PIN 73
#define SWITCH_PIN 12
#else               // Arduino target
#define LED_PIN_R 8
#define LED_PIN_G 9
#define LED_PIN_B 10
#define MOTOR_PIN 11
#define BUTTON_PIN 12
#define SWITCH_PIN 5
#endif

// Lists are 2 + (9 * dimension) bytes,
// so incidentally a 255-byte max data length
// limits this demo's lists to 28 elements.
#define MAXDATALEN 255

uint8_t header[16];
uint8_t data[MAXDATALEN];

int onSendAsCBL2(uint8_t type, enum Endpoint model, int* headerlen,
                 int* datalen, data_callback* data_callback);
int onGetAsCBL2(uint8_t type, enum Endpoint model, int datalen);

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_PIN_R, OUTPUT);
  pinMode(LED_PIN_G, OUTPUT);
  pinMode(LED_PIN_B, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(SWITCH_PIN, INPUT);

  digitalWrite(LED_PIN_R, LOW);
  digitalWrite(LED_PIN_G, LOW);
  digitalWrite(LED_PIN_B, LOW);
  analogWrite(MOTOR_PIN, 0);
  digitalWrite(BUTTON_PIN, HIGH);   // Pull-up resistor
  digitalWrite(SWITCH_PIN, HIGH);   // Pull-up resistor

  Serial.begin(9600);                           // Used for debugging
  cbl.setLines(lineRed, lineWhite);
  cbl.resetLines();
  // cbl.setVerbosity(true, &Serial);      // Comment this in for verbose message information
  
  // The following registers buffers for exchanging data, the maximum
  // allowed data length, and functions to call on Get() and Send().
  cbl.setupCallbacks(header, data, MAXDATALEN,
                      onGetAsCBL2, onSendAsCBL2);
}

// Repeatedly check to see if the calculator has initiated a Get()
// or a Send() operation yet. If it has, then onGetAsCBL2() or
// onSendAsCBL2() will be invoked, since they were registered in
// setup() above.
void loop() {
  int rval;
  rval = cbl.eventLoopTick();
  if (rval && rval != ERR_READ_TIMEOUT) {
    Serial.print("Failed to run eventLoopTick: code ");
    Serial.println(rval);
  }
}

// Callback when the CBL2 class has successfully received a variable
// from the attached calculator.
int onGetAsCBL2(uint8_t type, enum Endpoint model, int datalen) {
  Serial.print("Got variable of type ");
  Serial.print(type);
  Serial.print(" from endpoint of type ");
  Serial.println((int)model);
  
  // We only want to handle lists.
  if (type != VarTypes82::VarRList && type != VarTypes82::VarURList &&
      type != VarTypes84PCSE::VarRList)
  {
    return -1;
  }

  // Turn the LEDs and motor on or off
  uint16_t list_len = TIVar::sizeWordToInt(&(data[0]));    // Convert 2-byte size word to int
  if (list_len == 4) {
    // It is indeed a 4-element list
    int size_of_real = TIVar::sizeOfReal(model);
    int val_red   = TIVar::realToLong8x(&data[size_of_real * 0 + 2], model); // First list element starts after 2-byte size word
    int val_green = TIVar::realToLong8x(&data[size_of_real * 1 + 2], model);
    int val_blue  = TIVar::realToLong8x(&data[size_of_real * 2 + 2], model);
    int val_motor = TIVar::realToLong8x(&data[size_of_real * 3 + 2], model);

    digitalWrite(LED_PIN_R, val_red);
    digitalWrite(LED_PIN_G, val_green);
    digitalWrite(LED_PIN_B, val_blue);
    analogWrite(MOTOR_PIN, val_motor);
  }
  return 0;
}

// Callback when the CBL2 class notices the attached calculator
// wants to start a Get() exchange. The CBL2 class needs to get
// any data to send before continuing the exchange.
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
  *datalen = 2 + TIVar::sizeOfReal(model) * 2;
  TIVar::intToSizeWord(*datalen, &header[0]);  // Two bytes for the element count, ANALOG_PIN_COUNT Reals
                                                // This sets header[0] and header[1]
  header[2] = VarTypes85::VarRList;             // RealList (if you're a TI-85. Bleh.)
  header[3] = 0x01;                // Name length
  header[4] = 0x41;                // "A", as per "standard" See http://www.cemetech.net/forum/viewtopic.php?p=224739#224739
  header[5] = 0x00;                // Zero terminator (remainder of header is ignored)
  *headerlen = 11;
  
  // Compose the body of the variable
  data[0] = 2;   // Little-endian word for number of
  data[1] = 0;            // elements in this list
  int offset = 2;         // Offset past the count word

  // Convert the value, get the length of the inserted data or -1 for failure
  int rval;
  rval = TIVar::longToReal8x(digitalRead(BUTTON_PIN), &data[offset], model);
  if (rval < 0) {
    return -1;
  }
  offset += rval;
  rval = TIVar::longToReal8x(digitalRead(SWITCH_PIN), &data[offset], model);
  if (rval < 0) {
    return -1;
  }
  offset += rval;

  return 0;
}