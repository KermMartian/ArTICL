/*************************************************
 *  CalcCam.ino                                  *
 *  Example from the ArTICL library              *
 *           Created by Christopher Mitchell,    *
 *           2011-2014, all rights reserved.     *
 *                                               *
 *  This demo lets you capture pictures using a  *
 *  graphing calculator and a Gameboy camera.    *
 *  It also lets a connected calculator get or   *
 *  set the Gameboy camera's control registers,  *
 *  to control things like gain, exposure time,  *
 *  and more. The following commands are         *
 *  interpreted:                                 *
 *  - GetCalc(Pic1): Returns a 265x165 15-color  *
 *    image in Pic1-Pic9 or Pic0 to a TI-84+CSE, *
 *    or a 96x64 monochrome image to a TI-83+/   *
 *    TI-84+.                                    *
 *  - Send(L1): Sends an 8-element list          *
 *    containing the camera settings. Reset to   *
 *    defaults if the Arduino loses power.       *
 *  - Get(L1): Gets the 8-element list           *
 *    containing the camera settings.            *
 *                                               *
 *  The original Gameboy camera (M64282FP) code  *
 *  used in this demo was created by Laurent     *
 *  Saint-Marcel (lstmarcel@yahoo.fr), released  *
 *  2005-07-05. Adapted to the Arduino by Google *
 *  Code user shimniok@gmail.com, and further    *
 *  modified by Christopher Mitchell.            *
 *************************************************/

/* Camera connections:
    READ  -- D8,  PB0
    XCK   -- D9,  PB1
    XRST  -- D10, PB2
    LOAD  -- D11, PB3
    SIN   -- D12, PB4
    START -- D13, PB5
    VOUT  -- A3,  PC3
*/

// Includes
#include <Wire.h>

#include <avr/interrupt.h>
#include <avr/io.h>
#include <compat/deprecated.h>

#include "CBL2.h"
#include "TIVar.h"

// Defines
#define CAM_DATA_PORT     PORTB
#define CAM_DATA_DDR      DDRB
#define CAM_READ_PIN      8       // Arduino D8

#define CAM_LED_DDR       DDRB
#define CAM_LED_PORT      PORTB

// CAM_DATA_PORT
#define CAM_START_BIT 5
#define CAM_SIN_BIT   4
#define CAM_LOAD_BIT  3
#define CAM_RESET_BIT 2
#define CAM_CLOCK_BIT 1
#define CAM_READ_BIT  0
// CAM_LED_PORT
#define CAM_LED_BIT   4
// PORT C: Analogic/digital converter
#define CAM_ADC_PIN   3

#define TWI_CAMERA 0x01 // TWI (I2C) address of the camera

// modes of the camera
enum CamMode {
	CAM_MODE_STANDARD,
	CAM_MODE_DOWNSCALE
};

/* ------------------------------------------------------------------------ */
/* GLOBALS                                                                  */
/* ------------------------------------------------------------------------ */

// ArTICL-related
CBL2 cbl;
int lineRed = 7;
int lineWhite = 6;

#define MAXDATALEN 255
uint8_t header[16];
uint8_t data[MAXDATALEN];

// default value for registers
// 155 1 0 30 1 0 1 7 
// no edge detection, exposure=0,30, offset=-27, vref=+1.0, gain = 1
unsigned char camReg[8]={ 155, 1, 0, 30, 1, 0, 1, 7 };

CamMode camMode             = CAM_MODE_STANDARD;
unsigned char camClockSpeed = 0x07; // was 0x0A

int x, y;                // the current x,y coordinate we're working on
int cx, cy;

/* ------------------------------------------------------------------------ */
/* MACROS                                                                   */
/* ------------------------------------------------------------------------ */

#define Serialwait()   while (Serial.available() == 0) ;

int dataIn;
int dataOut;
boolean dataReady;
unsigned char reg;

/* ------------------------------------------------------------------------ */
/* Initialize all components                                                */
/* ------------------------------------------------------------------------ */
void setup()
{
  dataReady = false;
  Serial.begin(38400);
  //Wire.begin(TWI_CAMERA);
  //Wire.onReceive(recvByte);
  //Wire.onRequest(sendByte);
  camInit();

  /* enable interrupts */
  sei();

  cbl.setLines(lineRed, lineWhite);
  cbl.resetLines();
  //cbl.setVerbosity(true, &Serial);			// Comment this in for message information
  cbl.setupCallbacks(header, data, MAXDATALEN,
                      onGetAsCBL2, onSendAsCBL2);

  Serial.println("Ready.");
}

/* ------------------------------------------------------------------------ */
/* Program entry point                                                      */
/* ------------------------------------------------------------------------ */
void loop() { 
  int rval = 0;
  rval = cbl.eventLoopTick();
  if (rval && rval != ERR_READ_TIMEOUT) {
    Serial.print("Failed to run eventLoopTick: code ");
    Serial.println(rval);
  }  
} // loop

///////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// TWI HANDLERS
void recvByte(int howMany) {
  dataIn = Wire.read();
  dataReady = true;
}

void sendByte(void) {
  Wire.write(dataOut);
}

///////////////////////////////////////////////////////////////////////////
// CAM TIMING AND CONTROL

// cbi(port, bitId) = clear bit(port, bitId) = Set the signal Low
// sbi(port, bitId) = set bit(port, bitId) = Set the signal High

// Delay used between each signal sent to the AR (four per xck cycle).
void camStepDelay() {
  	unsigned char u=camClockSpeed;
	while(u--) {__asm__ __volatile__ ("nop");}
}
// Set the clock signal Low
inline void camClockL()
{
	cbi(CAM_DATA_PORT, CAM_CLOCK_BIT);
}
// Set the clock signal High
inline void camClockH()
{
	sbi(CAM_DATA_PORT, CAM_CLOCK_BIT);
}


// Initialise the IO ports for the camera
void camInit()
{
  pinMode(8, INPUT);   // READ
  pinMode(9, OUTPUT);  // XCK
  pinMode(10, OUTPUT);  // XRST
  pinMode(11, OUTPUT); // LOAD
  pinMode(12, OUTPUT); // SIN
  pinMode(13, OUTPUT); // START
  
  cbi(CAM_DATA_PORT, CAM_CLOCK_BIT);
  sbi(CAM_DATA_PORT, CAM_RESET_BIT);  // Reset is active low
  cbi(CAM_DATA_PORT, CAM_LOAD_BIT);
  cbi(CAM_DATA_PORT, CAM_START_BIT);
  cbi(CAM_DATA_PORT, CAM_SIN_BIT);
}


// Sends a 'reset' pulse to the AR chip.
// START:  XCK Rising Edge 
// FINISH: XCK Just before Falling Edge
void camReset()
{
  camClockH(); // clock high
  camStepDelay();
  camStepDelay();
 
  camClockL(); // clock low
  camStepDelay();
  cbi(CAM_DATA_PORT, CAM_RESET_BIT);
  camStepDelay();

  camClockH(); // clock high
  camStepDelay();
  sbi(CAM_DATA_PORT, CAM_RESET_BIT);
  camStepDelay();
}


// locally set the value of a register but do not set it in the AR chip. You 
// must run camSendRegisters1 to write the register value in the chip
void camStoreReg(unsigned char reg, unsigned char data) 
{
  camReg[reg] = data;
}

// Reset the camera and set the camera's 8 registers
// from the locally stored values (see camStoreReg)
void camSetRegisters(void)
{
  for(reg=0; reg<8; ++reg) {
    camSetReg(reg, camReg[reg]);
  }
}

// Sets one of the 8 8-bit registers in the AR chip.
// START:  XCK Falling Edge 
// FINISH: XCK Just before Falling Edge
void camSetReg(unsigned char regaddr, unsigned char regval)
{
  unsigned char bitmask;

  // Write 3-bit address.
  for(bitmask = 0x4; bitmask >= 0x1; bitmask >>= 1){
    camClockL();
    camStepDelay();
    // ensure load bit is cleared from previous call
    cbi(CAM_DATA_PORT, CAM_LOAD_BIT);
    // Set the SIN bit
    if(regaddr & bitmask)
      sbi(CAM_DATA_PORT, CAM_SIN_BIT);
    else
      cbi(CAM_DATA_PORT, CAM_SIN_BIT);
    camStepDelay();

    camClockH();
    camStepDelay();
    // set the SIN bit low
    cbi(CAM_DATA_PORT, CAM_SIN_BIT);
    camStepDelay();
  }

  // Write 7 most significant bits of 8-bit data.
  for(bitmask = 128; bitmask >= 1; bitmask>>=1){
    camClockL();
    camStepDelay();
    // set the SIN bit
    if(regval & bitmask)
      sbi(CAM_DATA_PORT, CAM_SIN_BIT);
    else
      cbi(CAM_DATA_PORT, CAM_SIN_BIT);
    camStepDelay();
    // Assert load at rising edge of xck
    if (bitmask == 1)
      sbi(CAM_DATA_PORT, CAM_LOAD_BIT);
    camClockH();
    camStepDelay();
    // reset the SIN bit
    cbi(CAM_DATA_PORT, CAM_SIN_BIT);
    camStepDelay();
  }
}

// Take a picture, read it and send it trhough the serial port. 
//
// getPicture -- send the pixel data to the requester
//
// START:  XCK Falling Edge 
// FINISH: XCK Just before Rising Edge
void camStartPicture() {

  // Camera START sequence
  camClockL();
  camStepDelay();
  // ensure load bit is cleared from previous call
  cbi(CAM_DATA_PORT, CAM_LOAD_BIT);
  // START rises before xck
  sbi(CAM_DATA_PORT, CAM_START_BIT);
  camStepDelay();

  camClockH();
  camStepDelay();
  // start valid on rising edge of xck, so can drop now
  cbi(CAM_DATA_PORT, CAM_START_BIT);
  camStepDelay();

  camClockL();
  camStepDelay();
  camStepDelay();
 
  // Wait for READ to go high
  while (1) {
    camClockH();
    camStepDelay();
    // READ goes high with rising XCK
    //    if ( inp(CAM_READ_PIN) & (1 << CAM_READ_BIT) )
    if (digitalRead(CAM_READ_PIN) == HIGH) // CAM pin on PB0/D8
      break;
    camStepDelay();

    camClockL();
    camStepDelay();
    camStepDelay();
  }
  
  //sbi(CAM_LED_PORT, CAM_LED_BIT);

  camStepDelay();
   
  // Read pixels from cam until READ signal is low again
  // Set registers while reading the first 11-ish pixels
  // The camera seems to be spitting out 128x128 even though the final 5 rows are junk

  cx = 0;
  cy = 0;
}

inline uint8_t camGetPixel(void) {
  uint8_t pixel;
  camClockL();
  camStepDelay();
  // get the next pixel, buffer it, and send it out to the attached calculator
  pixel = analogRead(CAM_ADC_PIN) >> 2;

  camClockH();
  camStepDelay();
  camStepDelay();

  cx++;
  if (cx == 128) {
    cx = 0;
    cy++;
  }
      
  if (cy >= 128) {
    camClockL();
    camStepDelay();
    camStepDelay();
  }
  
  return pixel;
}

uint8_t sendPicDataByte(int idx) {
  uint8_t outbyte;
  if (x < 0) {
    uint16_t length = (camMode == CAM_MODE_STANDARD)?21945:756;
	if (x == -2) {
	  outbyte = (uint8_t)(length & 0x0ff);
	} else {
	  outbyte = (uint8_t)(length >> 8);
	}
	x++;
	return outbyte;
  }
  if (camMode == CAM_MODE_STANDARD) {
    outbyte = 0xbb;    // all white
    if (y >= 17 && y < 145 && x >= 68 && x < 196) {
      for(uint8_t i = 0; i < 2; i++) {
        uint8_t pixel = camGetPixel();
        outbyte <<= 4;
        if (pixel < 25) {
        outbyte |= 0x03;
        } else if (pixel < 76) {
          outbyte |= 0x0f;
        } else if (pixel < 127) {
          outbyte |= 0x0e;
        } else if (pixel < 178) {
          outbyte |= 0x0d;
        } else if (pixel < 229) {
          outbyte |= 0x0c;
        } else {
          outbyte |= 0x0b;
        }
      }
    }
    x += 2;
    // Update coordinates
    if (x >= 266) {
      x = 0;
      y++;
    }
  } else {
    outbyte = 0x00;	// 8 white pixels;
    if (x >= 16 && x < 80) {
      for(uint8_t i = 0; i < 8; i++) {
        uint8_t pixel = camGetPixel();
        outbyte <<= 1;
        outbyte |= (pixel >= 128)?0:1;
        camGetPixel();			// Skip one pixel
      }
    }

    x += 8;
    if (x >= 96) {
      x = 0;
      y++;
      for(uint8_t i = 0; i < 128; i++) {
        camGetPixel();		// Throw out one row
      }
	}
  }
  
  return outbyte;
}

int onGetAsCBL2(uint8_t type, enum Endpoint model, int datalen) {
  //Serial.print("Got variable of type 0x");
  //Serial.print(header[2], HEX);
  //Serial.print(" from endpoint of type 0x");
  //Serial.println((int)model, HEX);

  if (header[2] != 0x01) {
    // Only accept a list
    return -1;
  }
  if (8 != TIVar::sizeWordToInt(&data[0])) {
    // Only accept an 8-element list
    return -1;
  }
  for(int i=0; i<8; i++) {
    int value = (int)TIVar::realToFloat8x(&data[2 + TIVar::sizeOfReal(model) * i], model);  
    //Serial.print("Element ");
    //Serial.print(i);
    //Serial.print(" has value ");
    //Serial.println(value, HEX);
    camStoreReg(i, value);
  }
  return 0;
}

int onSendAsCBL2(uint8_t type, enum Endpoint model, int* headerlen,
                 int* datalen, data_callback* data_callback)
{
  Serial.print("Req for var 0x");
  Serial.print(type, HEX);
  Serial.print(" from EP 0x");
  Serial.println((int)model, HEX);
  
  if (type == 0x01 || type == 0x5D) {
    // Return the register values. First compose header...
    *headerlen = 11;
    *datalen = 2 + 8 * TIVar::sizeOfReal(model);
    if (*datalen > MAXDATALEN) {
      // Too big.
      return -1;
    }
    TIVar::intToSizeWord(*datalen, &header[0]);
    // Do not change the rest of the header
    
    // ... then compose body
    TIVar::intToSizeWord(8, &data[0]);
    int offset = 2;
    for(int i = 0; i < 8; i++) {
  	// Convert the value, get the length of the inserted data or -1 for failure
  	int rval = TIVar::longToReal8x(camReg[i], &data[offset], model);
  	if (rval < 0) {
  		return -1;
  	}
  	offset += rval;
    }
    return 0;
    
  } else if (type == 0x07 || type == 0x60) {
	// TI-84+CSE or TI-83+/TI-84+ picture
	
    // Compose the VAR header
    if (*headerlen == 13 && header[11] == 0x0A) {  
      *datalen = 21945 + 2;                             // 165 * 266 / 2 (4 bits per pixel)
      // Leave the pic portion of the header as-is
      header[11] = 0x0A;      // Version
      header[12] = 0x80;      // Archived
      camMode = CAM_MODE_STANDARD;
    } else {
      *datalen = 756 + 2;
      TIVar::intToSizeWord(*datalen, &header[0]);	// Two bytes for the element count, 6 Reals
	  header[2] = 0x07;		// Because the TI-OS makes no sense
      camMode = CAM_MODE_DOWNSCALE;
    }
	
	// Initialize the camera
    x = -2;
    y = 0;
    camReset();
    camSetRegisters();
    camStartPicture();

    // Do not compose the body of the variable!
    *data_callback = sendPicDataByte;
    return 0;
  } else {
    // Unknown type requested
    return -1;
  }
}
