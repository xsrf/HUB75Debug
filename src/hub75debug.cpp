#include <Arduino.h>
#include <SPI.h>
#define PIN_NC 255

/* 
  ====================================================
  === MODIFY THIS BELOW TO FIT YOUR PANEL / WIRING === 
  ====================================================
*/

// Matrix Dimensions
#define MATRIX_WIDTH  64
#define MATRIX_HEIGHT 32

// PIN Wiring, set to PIN_NC if not connected
#define P_LAT  16 // D0
#define P_A     5 // D1
#define P_B     4 // D2
#define P_C    15 // D8
#define P_D    12 // D6 (Only for 1/16 or 1/32 panels)
#define P_E     0 // D3 (Only for 1/32 panles)
#define P_OE    2 // D4
#define P_CLK  14 // D5 (Documentation only, this Pin is hardwired and cannot be changed!)
#define P_DATA 13 // D7 (Documentation only, this Pin is hardwired and cannot be changed!)

/* 
  ====================================================
  === MODIFY THIS ABOVE TO FIT YOUR PANEL / WIRING === 
  ====================================================
*/


enum mux_patterns {BINARY, STRAIGHT, SHIFTREG_ABC};
uint32_t bpad = (MATRIX_WIDTH*MATRIX_HEIGHT*3)/2;
uint64_t lastmillis = 0;

void setup();
void loop();
void shiftABC(uint8_t rows, uint8_t level);
void selectDebugRow();
void selectRow(uint8_t row, uint8_t mux_pattern);
void strobe(uint32_t t_ms);
void latch();
void fillBuffer(uint16_t bits_high, uint16_t bits_low);

void setup() {
  Serial.begin(115200);
  Serial.println("HUB75 Matrix Debug");
  // Init SPI
  SPI.begin();
  SPI.setHwCs(false);
  SPI.setFrequency(1e6);
  // Init all Outputs/IOs
  pinMode(P_OE, OUTPUT);
  digitalWrite(P_OE, HIGH);
  pinMode(P_LAT, OUTPUT);
  if(P_A != PIN_NC) pinMode(P_A, OUTPUT);
  if(P_B != PIN_NC) pinMode(P_B, OUTPUT);
  if(P_C != PIN_NC) pinMode(P_C, OUTPUT);
  if(P_D != PIN_NC) pinMode(P_D, OUTPUT);
  if(P_E != PIN_NC) pinMode(P_E, OUTPUT);
  if(P_A != PIN_NC) digitalWrite(P_A, LOW );
  if(P_B != PIN_NC) digitalWrite(P_B, LOW );
  if(P_C != PIN_NC) digitalWrite(P_C, LOW );
  if(P_D != PIN_NC) digitalWrite(P_D, LOW );
  if(P_E != PIN_NC) digitalWrite(P_E, LOW );
}


void loop() {
  // Step 1: Get MUX-Pattern... 1/2, 1/4, 1/8, ...
  Serial.println("");
  Serial.println("Test 1: Mux/Scan");
  Serial.println("----------------");
  Serial.println("You should see white lines on your panel. How many rows are they apart?");
  Serial.println("If every N-th row is lit, you have a 1/N panel. e.g. if every 4th row is lit, it's a 1/4 panel.");
  Serial.println("Also, which line is lit? Note that the Top-Side is usually marked on the backside of the panel.");
  Serial.println("* If the FIRST row is lit, you have a STRAIGHT Mux.");
  Serial.println("* If the SECOND row is lit, you have a BINARY Mux.");
  Serial.println("* If the THIRD row is lit, you have a SHIFTREG_ABC Mux with LATCH on PIN_B.");
  Serial.println("* If the FOURTH row is lit, you have a SHIFTREG_ABC Mux without LATCH and Shift on falling edge.");
  Serial.println("* If the FIFTH row is lit, you have a SHIFTREG_ABC Mux without LATCH and Shift on rising edge.");
  lastmillis = millis();
  while(millis() - lastmillis < 5e3) {
    fillBuffer(bpad,0);
    latch();
    selectDebugRow();
    strobe(10);
  }

  // Step 2: Scan-Pattern
  Serial.println("");
  Serial.println("Test 2: Pattern");
  Serial.println("---------------");
  Serial.println("Pixel by pixel will be enabled now to show you how the data is mapped to the scan-lines.");
  Serial.println("You can slow down / skip this test by pressing any key.");

  delay(1e3);
  uint16_t cnt = 0;
  while (cnt < bpad) {
    fillBuffer(cnt,bpad-cnt);
    latch();
    selectDebugRow();
    strobe(5);
    if(millis() - lastmillis > 5) {
      lastmillis = millis();
      cnt++;
    }
    while(Serial.available()) {
      uint8_t dummy;
      Serial.readBytes(&dummy,1);
      cnt = bpad;
    }
  }

  delay(1e3);
  cnt = 0;
  while (cnt < bpad) {
    fillBuffer(cnt,bpad-cnt);
    latch();
    selectDebugRow();
    strobe(10);
    if(millis() - lastmillis > 100) {
      lastmillis = millis();
      cnt++;
    }
    while(Serial.available()) {
      uint8_t dummy;
      Serial.readBytes(&dummy,1);
      cnt = bpad;
    }
  }
  
  delay(1e3);
}







void shiftABC(uint8_t rows, uint8_t level = LOW) {
  if(P_C != PIN_NC) digitalWrite(P_C, level);
  delayMicroseconds(1);
  for(uint8_t i = 0; i<rows; i++) {
     digitalWrite(P_A, HIGH);
     delayMicroseconds(1);
     digitalWrite(P_A, LOW);
     delayMicroseconds(1);
  }
}

void selectDebugRow() {
  // This will help debugging the row selection mechanism
  // It will select:
  // - row 0 for STRAIGT
  // - row 1 for BINARY
  // - row 2 for SHIFTREG_ABC with Latch (not sure if that exists)
  // - row 3 for SHIFTREG_ABC without Latch and falling edge shift
  // - row 4 for SHIFTREG_ABC without Latch and rising edge shift
  if(P_A != PIN_NC) digitalWrite(P_A, LOW );
  if(P_B != PIN_NC) digitalWrite(P_B, LOW );
  if(P_C != PIN_NC) digitalWrite(P_C, LOW );
  if(P_D != PIN_NC) digitalWrite(P_D, LOW );
  if(P_E != PIN_NC) digitalWrite(P_E, LOW );
  // Shift out zeroes to clear SHIFTREG_ABC panels
  shiftABC(32);
  // Now shift out one 1 and shift it by 2 rows
  shiftABC(1, HIGH);
  shiftABC(2); // Now the third row (row 2) is selected
  // Now latch this, in case the Panel has a Latch on PIN B!
  if(P_B != PIN_NC) digitalWrite(P_B, HIGH );
  if(P_B != PIN_NC) digitalWrite(P_B, LOW );
  // Now shift once again, this selects row 3
  shiftABC(1);
  // Now activate A
  if(P_A != PIN_NC) digitalWrite(P_A, HIGH );
  // This will shift again by one if the shift register triggers on rising edge and select row 4
  // For STRAIGT, row 0 is selected
  // For BINARY, row 1 is selected
}

void selectRow(uint8_t row, uint8_t mux_pattern) {
  if(mux_pattern==BINARY) {
    if(P_A != PIN_NC) digitalWrite(P_A, (row>>0)&1 );
    if(P_B != PIN_NC) digitalWrite(P_B, (row>>1)&1 );
    if(P_C != PIN_NC) digitalWrite(P_C, (row>>2)&1 );
    if(P_D != PIN_NC) digitalWrite(P_D, (row>>3)&1 );
    if(P_E != PIN_NC) digitalWrite(P_E, (row>>4)&1 );
  }

  if(mux_pattern==STRAIGHT) {
    if(P_A != PIN_NC) digitalWrite(P_A, (row==0) );
    if(P_B != PIN_NC) digitalWrite(P_B, (row==1) );
    if(P_C != PIN_NC) digitalWrite(P_C, (row==2) );
    if(P_D != PIN_NC) digitalWrite(P_D, (row==3) );
    if(P_E != PIN_NC) digitalWrite(P_E, (row==4) );
  }

  if(mux_pattern==SHIFTREG_ABC) {
    if((P_A == PIN_NC) || (P_B == PIN_NC) || (P_C == PIN_NC)) return;
    // A,B,C are connected to a shift register Clock, Latch, Data
    uint32_t rowmask;
    rowmask = (1<<row);
    digitalWrite(P_A,LOW);
    digitalWrite(P_B,LOW);
    digitalWrite(P_C,LOW);
    shiftOut(P_C,P_A,MSBFIRST,rowmask>>24);
    shiftOut(P_C,P_A,MSBFIRST,rowmask>>16);
    shiftOut(P_C,P_A,MSBFIRST,rowmask>>8);
    shiftOut(P_C,P_A,MSBFIRST,rowmask);
    // Latch
    digitalWrite(P_B,HIGH);
    digitalWrite(P_B,LOW);
  }
}

void strobe(uint32_t t_ms) {
  uint32_t _s;
  _s = micros();
  while ( (micros()-_s)<t_ms*1e3 ) {
    digitalWrite(P_OE, LOW);
    delayMicroseconds(5);
    digitalWrite(P_OE, HIGH);
    delayMicroseconds(200);
  }  
  yield();
}

void latch() {
  digitalWrite(P_LAT, HIGH);
  digitalWrite(P_LAT, LOW);
}

void fillBuffer(uint16_t bits_high, uint16_t bits_low) {
  bits_low -= (bits_low+bits_high)%8;
  while (bits_low >= 8) {
    SPI.write(0x00);
    bits_low -= 8;
  }  
  if(bits_high%8 > 0) {
    uint8_t b=0;
    uint8_t bhr = bits_high%8;
    while (bhr > 0) {
      b |= (1<<(bhr-1));
      bhr--;
    }    
    SPI.write(b);
    bits_high -= bits_high%8;
  }
  while (bits_high >= 8) {
    SPI.write(0xFF);
    bits_high -= 8;
  }
}


