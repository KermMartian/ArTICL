/*************************************************
 *  WhackAMole.ino                               *
 *  Example from the ArTICL library              *
 *           Created by Timothy Keller,    *
 *           2011-2014, all rights reserved.     *
 *                                               *
 *  This demo allows interfacing with a MSP432   *
 *  device designed as a Whack A Mole Board, by  *
 *  emulating a CBL2 Style API.                  *
 *  Pins 2 and 23-30 are connected to CDS cells  *
 *  that act as our "buttons" for this demo.     *
 *
 *************************************************/
 
#include <CBL2.h>
#include <TIVar.h>


CBL2* cbl;
const int lineRed = DEFAULT_TIP;
const int lineWhite = DEFAULT_RING;

#define ANALOG_PIN_COUNT 9
const int analogPins[ANALOG_PIN_COUNT] = {30, 29, 28, 27, 26, 25, 24, 23, 2};

#define RGB_Red 40
#define RGB_Green 39
#define RGB_Blue 38
#define Multiplexer_PIN_COUNT 9
const int MultiPlexerPins[Multiplexer_PIN_COUNT] = {37, 36, 35, 34, 33, 32, 31, 11, 12};

#define MAXDATALEN 255
uint8_t header[16];
uint8_t data[MAXDATALEN];

void setup()
{

  Serial.begin(9600);
  cbl = new CBL2(lineRed, lineWhite);
  cbl->resetLines();
  cbl->setVerbosity(true, &Serial);  // Comment this in for message information
  cbl->setupCallbacks(header, data, MAXDATALEN,
                      onGetAsCBL2, onSendAsCBL2);
                      
}

void loop()
{
  int rval;
  rval = cbl->eventLoopTick();
  if (rval && rval != ERR_READ_TIMEOUT) {
    Serial.print("Failed to run eventLoopTick: code ");
    Serial.println(rval);
  }
  
}
void SetRgbActive(int Led){

    TurnOffAllRGBLeds();
    digitalWrite(MultiPlexerPins[Led],LOW);

}

void TurnOffAllRGBLeds(){
    for(int i = 0; i < Multiplexer_PIN_COUNT; i++) {
        digitalWrite(MultiPlexerPins[i],HIGH);
    }        
}
int onGetAsCBL2(uint8_t type, enum Endpoint model, int datalen) {
  Serial.print("Got variable of type ");
  Serial.print(type);
  Serial.print(" from endpoint of type ");
  Serial.println((int)model);
  
  if (type != VarTypes82::VarRList)
    return -1;  //If we are not a list we do not want you ABORT
    
  int list_len = data[0] | (data[1] << 8);
  switch(list_len){
    case 1:{  //If we are a control structure
        int value = (int)TIVar::realToFloat8x(&data[2], model);  // Get element
        switch(value){
            case 0:{  // Turns off all LEDS
                TurnOffAllRGBLeds();
                return 0;
            }
            case 10:  // Unimplemented yet. But room to grow for single commands is here. 
            case 11:  //
            default:
                return -1;
        }
    }
    case 4:{  // RGB Control structure
             // {N,R,G,B} Where N is LED and RGB=Analog Values for each color
       
        TurnOffAllRGBLeds();  //First we turn off the multiplexer
        
        int Offset = 2;  //Start us after the size bytes
        int NewLed = (int)TIVar::realToFloat8x(&data[Offset], model);  // Get element
        
        Offset += TIVar::sizeOfReal(model);
        analogWrite(RGB_Red,(int)TIVar::realToFloat8x(&data[Offset], model));
        Offset += TIVar::sizeOfReal(model);
        analogWrite(RGB_Green,(int)TIVar::realToFloat8x(&data[Offset], model));
        Offset += TIVar::sizeOfReal(model);
        analogWrite(RGB_Blue,(int)TIVar::realToFloat8x(&data[Offset], model));
        
        SetRgbActive(NewLed);  //Turn on the new LED
        return 0;
    }    
    default:{  // Gotta Catch Em All!
        return -1;
    }    
  }
  
  return -1;
}

int onSendAsCBL2(uint8_t type, enum Endpoint model, int* headerlen,
                 int* datalen, data_callback* data_callback)
{
  Serial.print("Got request for variable of type ");
  Serial.print(type);
  Serial.print(" from endpoint of type ");
  Serial.println((int)model);

  if (type != VarTypes82::VarRList)
    return -1; //If we are not a list we do not want you ABORT

     // Compose the VAR header
  *datalen = 2 + TIVar::sizeOfReal(model) * ANALOG_PIN_COUNT;
  TIVar::intToSizeWord(*datalen, &header[0]);	// Two bytes for the element count, ANALOG_PIN_COUNT Reals
                                                // This sets header[0] and header[1]
  header[2] = 0x04;				// RealList (if you're a TI-85. Bleh.)
  header[3] = 0x01;				// Name length
  header[4] = 0x41;				// "A", as per "standard" See http://www.cemetech.net/forum/viewtopic.php?p=224739#224739
  header[5] = 0x00;				// Zero terminator (remainder of header is ignored)
  *headerlen = 11;
  
  // Compose the body of the variable
  data[0] = ANALOG_PIN_COUNT;
  data[1] = 0;
  int offset = 2;
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