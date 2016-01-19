ArTICL (Arduino TI Calculator Linking Library)
==============================================
TI Link Protocol library for the Arduino development platform

For more information and for questions, please refer to 
[this forum post](http://www.cemetech.net/forum/viewtopic.php?t=10809)
on Cemetech. An [introductory video](https://www.youtube.com/watch?v=-A14KrqVtt0)
contains a demo of the ControlLED example, and a
[how-to video](https://www.youtube.com/watch?v=gAUrIO3FTcQ) created by
@geekboy1011 and @KermMartian shows you how to set up ArTICL with your calculator.

Using the Library
-----------------
If you want to be able to simply `send()` and `get()` packets to and from a
calculator or other TI link protocol-speaking device, create a TICL object and
use it. For higher-level management that can be used with the calculator's
`Send(` and `Get(` commands, make a CBL2 object instead. See the ControlLED
example for a demonstration of using the CBL2 class.

Introductory video: https://www.youtube.com/watch?v=-A14KrqVtt0

How-to video: https://www.youtube.com/watch?v=gAUrIO3FTcQ

Hardware
--------
ArTICL works with the Arduino Duemilanove and newer Arduino boards, as well as
TI's MSP432 Launchpad.
### Arduino Hardware
To communicate with a calculator, cut open a 2.5mm calculator link cable, and
connect its three wires to the Arduino. The defaults are digital pin 2 = tip (red
wire), digital pin 3 = ring (white wire), gnd = base (copper wire). For the
screenshot example, short digital pin 4 to gnd to take a screenshot.
### MSP432 Launchpad Hardware
The Launchpad has weak pullup resistors, so additional connections are necessary.
Cut open a 2.5mm calculator link cable, and connect its three wires to the Launchpad.
The defaults are pin P5.7 = tip (red wire), P5.6 = ring (white wire), gnd = base
(copper wire). In addition, connect a 1Kohm resistor from P5.6 to +3.3V, and from
P5.7 to +3.3V.

![MSP432 Launchpad to calculator connections](/doc/articl_msp432.png)

Verbose Output
--------------
You can make the TICL class dump details of every packet successfully received
and every packet that is attempted to be sent. Add the following code to your
setup() function, assuming ticl is a TICL object or CBL2 object.

```
  Serial.begin(9600);
  ticl.setVerbosity(true, &Serial);
```
