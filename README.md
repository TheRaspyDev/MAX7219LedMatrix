# MAX7219LedMatrix
Library for Arduino/ESP8266 for displaying text on one or multiple MAX7219 8x8 led matrices.

This library displays text and sets specific pixels on one or multiple 8x8 led matrices with a MAX7219 driver chip controlled through the SPI interface.
These modules are relatively cheep and can be daisy chained which makes it easy to get a led text bar up and running.

For details about the MAX7219 theory, wiring, schematic, etc. there's a great post by Nick Gammon: http://www.gammon.com.au/forum/?id=11516 
  
Currently this library supports the following operations:

- set pixels
- write text with a simple font
- scroll text left or right 
- oscillate text between the two ends
- Rotate characters for those displays with 90 Deg rotated LED matrices
- 4x faster SPI transfer compared to original code



## Example

```
#include <SPI.h>
#include "LedMatrix.h"

#define NUMBER_OF_DEVICES 1
#define CS_PIN 2
LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CS_PIN);

void setup() {
  Serial.begin(115200); // For debugging output
  ledMatrix.init();
  ledMatrix.setIntensity(4); // range is 0-15
  ledMatrix.setText("MAX7219 Demo");
}

void loop() {
  ledMatrix.clear();
  ledMatrix.oscillateText();
  ledMatrix.drawText();
  ledMatrix.commit(); // commit transfers the byte buffer to the displays
  delay(100);
}
```


## Connecting the module(s) 

|LED Matrix |	ESP8266                     |
|-----------|-----------------------------|
|VCC        |	+3.3V                       |
|GND	      | GND                         |
|DIN	      |GPIO13 (HSPID)               |
|CS	        |Choose free GPIO, e.g. GPIO2 |
|CLK	      |GPIO14 (HSPICLK)             |


|LED Matrix |	Arduino Mini Pro            |
|-----------|-----------------------------|
|VCC        |	+5V                       |
|GND	      | GND                         |
|DIN	      |D11 (MOSI)                   |
|CS	        |Choose free GPIO, D2         |
|CLK	      |D13 (SCK)                    |
