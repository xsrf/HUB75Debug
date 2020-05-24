#include <Arduino.h>
#include <SPI.h>

#define PIN_NC 255

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

enum mux_patterns {BINARY, STRAIGHT, SHIFTREG_ABC};

void setup() {
  Serial.begin(115200);
  Serial.println("HUB75 Matrix Debug");
  // Init SPI
  SPI.begin();
  SPI.setHwCs(false);
  SPI.setFrequency(1e6);
  // Enable Outputs
  pinMode(P_OE, OUTPUT);
  digitalWrite(P_OE, HIGH);
  pinMode(P_LAT, OUTPUT);
  if(P_A != PIN_NC) pinMode(P_A, OUTPUT);
  if(P_B != PIN_NC) pinMode(P_B, OUTPUT);
  if(P_C != PIN_NC) pinMode(P_C, OUTPUT);
  if(P_D != PIN_NC) pinMode(P_D, OUTPUT);
  if(P_E != PIN_NC) pinMode(P_E, OUTPUT);
}


void selectDebugRow() {
  // This will help debugging the row selection mechanism
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
    delayMicroseconds(10);
    digitalWrite(P_OE, HIGH);
    delayMicroseconds(100);
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

void loop() {
  uint32_t bpad = (MATRIX_WIDTH*MATRIX_HEIGHT*3)/2;
  fillBuffer(8,bpad);
  latch();
  selectRow(0, BINARY);
  strobe(100);
  delay(500);
}

