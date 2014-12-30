dvopus
======

Opus digital voice over KISS TNC

Requires PulseAudio

To test, ensure TNC on-air baud rate is set to 9600 baud
and are already in KISS mode:

./dvopus-tx > /dev/ttyUSB0
./dvopus-rx < /dev/ttyUSB0

Testing was done with a TH-D7 transmitting to a TH-D700.
Given the many small frames and the fact that KISS mode on
these radios is undocumented, no voice ever really got through.
The serial buffers would fill and the radio would eventually
seize.

try2.txt has some notes on how to possibly make this work, but
I'm mostly chalking this up to a learning experience.

