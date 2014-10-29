ArduinoTILP
===========

TI Link Protocol library for the Arduino development platform

For now, see http://www.cemetech.net/forum/viewtopic.php?t=4771 for details.

Verbose Output
--------------
You can make the TILP library dump details of every packet successfully received
and every packet that is attempted to be sent. Add the following code to your
setup() function, assuming tilp is a TILP object.

```
  Serial.begin(9600);
  tilp.setVerbosity(true, &Serial);
```