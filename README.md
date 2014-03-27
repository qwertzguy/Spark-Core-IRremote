Spark-Core-IRremote
===================

A modified version of [Arduino-IRremote](https://github.com/shirriff/Arduino-IRremote) for the Spark Core v1.0 and compatible with the Spark Core web IDE.

Modifications
-------------

* Trimmed out IRrecv (removed the decoding of IR signals. Thus you can only send IR codes with this lib. If you make the IRrecv part of the code copatible, please send a pull-request :) )
* Removed the use of PWM. It now only uses digitalWrite() and delayMicroseconds() to make the IR pulses. Thus the maximum frequency of pulses is 166khz. (Most IR codes are 38khz so it's not a problem)
* Allows to set the pin of the IR led. (any A or D pin should work on the Spark Core)
* Allows to make multiple instance of the IRsend class with different pins to control multiple IR leds.
