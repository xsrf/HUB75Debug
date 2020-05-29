HUB75 Debug Instructions
========================
This short program will run two basic tests on a typical HUB75 LED matrix panel.
It is meant for running with *Arduino Framework* on an *ESP8266* or *ESP32* but will probably also work on other microcontrollers.

Test 1
------
This test will help you identify how the panel addresses its rows and also how many rows need to be addressed.
When the first test runs, you should see white lines on your panel. 
How many rows are they apart?
If every N-th row is lit, you have a 1/N panel. e.g. if every 4th row is lit, it's a 1/4 panel.
Also, which line is lit? Note that the Top-Side is usually marked on the backside of the panel.
* If the FIRST row is lit, you have a STRAIGHT Mux.
* If the SECOND row is lit, you have a BINARY Mux.
* If the THIRD row is lit, you have a SHIFTREG_ABC Mux with LATCH on PIN_B.
* If the FOURTH row is lit, you have a SHIFTREG_ABC Mux without LATCH and Shift on falling edge.
* If the FIFTH row is lit, you have a SHIFTREG_ABC Mux without LATCH and Shift on rising edge.

Test 2
------
This test will help you identify how the incoming data is mapped to the LEDs of each scan-line.
Pixel by pixel will be enabled to show the mapping pattern evolving.
The test will run twice - first fast than slow.
You can skip this test by pressing any key (via Serial connection).

Notes
-----
See [src/hub75debug.cpp](src/hub75debug.cpp) for complete instructions.

Please use libraries like [PxMatrix](https://github.com/2dom/PxMatrix) to actually drive your panel.