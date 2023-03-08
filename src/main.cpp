#include <Arduino.h>
#include <U8g2lib.h>
#include <cmath>

//Constants
  const uint32_t interval = 100; //Display update interval
  const uint32_t MAX_UINT32 = 4294967295;
  const uint32_t stepSizes [] = {32832933, 30990162, 29250818, 27609095, 26059516, 24596908, 23216389, 21913354, 20683452, 19522579, 18426860, 17392640, 16416466, 15495081, 14625409, 13804548, 13029758, 12298454, 11608195, 10956677, 10341726, 9761289, 9213430, 8696320, 8208233, 7747540, 7312704, 6902274, 6514879, 6149227, 5804097, 5478338, 5170863, 4880645, 4606715, 4348160};
  volatile uint32_t currentStepSize;

//Pin definitions
  //Row select and enable
  const int RA0_PIN = D3;
  const int RA1_PIN = D6;
  const int RA2_PIN = D12;
  const int REN_PIN = A5;

  //Matrix input and output
  const int C0_PIN = A2;
  const int C1_PIN = D9;
  const int C2_PIN = A6;
  const int C3_PIN = D1;
  const int OUT_PIN = D11;

  //Audio analogue out
  const int OUTL_PIN = A4;
  const int OUTR_PIN = A3;

  //Joystick analogue in
  const int JOYY_PIN = A0;
  const int JOYX_PIN = A1;

  //Output multiplexer bits
  const int DEN_BIT = 3;
  const int DRST_BIT = 4;
  const int HKOW_BIT = 5;
  const int HKOE_BIT = 6;

//Display driver object
U8G2_SSD1305_128X32_NONAME_F_HW_I2C u8g2(U8G2_R0);

//Generate a sawtooth
void sampleISR() {
  static uint32_t phaseAcc = 0;
  phaseAcc += currentStepSize;
  int32_t Vout = (phaseAcc >> 24) - 128;
  analogWrite(OUTR_PIN, Vout + 128);
}

void sampleISR_triangle(){
  static uint32_t phaseAcc = 0;
  phaseAcc += currentStepSize;
  if ((phaseAcc >> 24) < 128){
    int32_t Vout = (phaseAcc >> 24) - 128;
  }
  else{
    int32_t Vout = 255 - (phaseAcc >> 24);
  }
  analogWrite(OUTR_PIN, Vout + 128);
}

void sampleISR_square(){
  static uint32_t phaseAcc = 0;
  phaseAcc += currentStepSize;
  if ((phaseAcc >> 24) < 128){
    int32_t Vout = 0 - 128;
  }
  else{
    int32_t Vout = 255 - 128;
  }
  analogWrite(OUTR_PIN, Vout + 128);
}

void sampleISR_sine(){
  static uint32_t phaseAcc = 0;
  phaseAcc += currentStepSize;
  double radians = phaseAcc/MAX_UINT32;
  double sineScaled = std::sin(radians)*2147483646 + 0.5;
  int32_t sineScaledFixed = static_cast<int32_t>(sineScaled);

  int32_t Vout = (sineScaledFixed >> 24) - 128;
  analogWrite(OUTR_PIN, Vout + 128);
}

//Function to set outputs using key matrix
void setOutMuxBit(const uint8_t bitIdx, const bool value) {
      digitalWrite(REN_PIN,LOW);
      digitalWrite(RA0_PIN, bitIdx & 0x01);
      digitalWrite(RA1_PIN, bitIdx & 0x02);
      digitalWrite(RA2_PIN, bitIdx & 0x04);
      digitalWrite(OUT_PIN,value);
      digitalWrite(REN_PIN,HIGH);
      delayMicroseconds(2);
      digitalWrite(REN_PIN,LOW);
}

void setup() {
  // put your setup code here, to run once:

  //Set pin directions
  pinMode(RA0_PIN, OUTPUT);
  pinMode(RA1_PIN, OUTPUT);
  pinMode(RA2_PIN, OUTPUT);
  pinMode(REN_PIN, OUTPUT);
  pinMode(OUT_PIN, OUTPUT);
  pinMode(OUTL_PIN, OUTPUT);
  pinMode(OUTR_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(C0_PIN, INPUT);
  pinMode(C1_PIN, INPUT);
  pinMode(C2_PIN, INPUT);
  pinMode(C3_PIN, INPUT);
  pinMode(JOYX_PIN, INPUT);
  pinMode(JOYY_PIN, INPUT);

  //Initialise display
  setOutMuxBit(DRST_BIT, LOW);  //Assert display logic reset
  delayMicroseconds(2);
  setOutMuxBit(DRST_BIT, HIGH);  //Release display logic reset
  u8g2.begin();
  setOutMuxBit(DEN_BIT, HIGH);  //Enable display power supply

  //Initialise UART
  Serial.begin(9600);
  Serial.println("Hello World");
}

void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t next = millis();
  static uint32_t count = 0;

  while (millis() < next);  //Wait for next interval

  next += interval;

  //Update display
  u8g2.clearBuffer();         // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
  u8g2.drawStr(2,10,"Helllo World!");  // write something to the internal memory
  u8g2.setCursor(2,20);
  u8g2.print(count++);
  u8g2.sendBuffer();          // transfer internal memory to the display

  //Toggle LED
  digitalToggle(LED_BUILTIN);
  
}