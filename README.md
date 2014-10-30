ArTICL (Arduino TI Calculator Linking Library)
==============================================
TI Link Protocol library for the Arduino development platform

For now, see http://www.cemetech.net/forum/viewtopic.php?t=4771 for details.

Using the Library
-----------------
If you want to be able to simply `send()` and `get()` packets to and from a
calculator or other TI link protocol-speaking device, create a TICL object and
use it. For higher-level management that can be used with the calculator's
`Send(` and `Get(` commands, make a CBL2 object instead. See the ControlLED
example for a demonstration of using the CBL2 class.

Verbose Output
--------------
You can make the TICL class dump details of every packet successfully received
and every packet that is attempted to be sent. Add the following code to your
setup() function, assuming ticl is a TICL object or CBL2 object.

```
  Serial.begin(9600);
  ticl.setVerbosity(true, &Serial);
```