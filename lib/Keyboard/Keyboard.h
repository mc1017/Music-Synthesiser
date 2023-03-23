#ifndef KEYBOARD_INTERFACE_H
#define KEYBOARD_INTERFACE_H
#include <Arduino.h>
#include <U8g2lib.h>

// Pin definitions
// Row select and enable
extern const int RA0_PIN;
extern const int RA1_PIN;
extern const int RA2_PIN;
extern const int REN_PIN;

// Matrix input and output
extern const int C0_PIN;
extern const int C1_PIN;
extern const int C2_PIN;
extern const int C3_PIN;
extern const int OUT_PIN;

// Audio analogue out
extern const int OUTL_PIN;
extern const int OUTR_PIN;

// Joystick analogue in
extern const int JOYY_PIN;
extern const int JOYX_PIN;

// Output multiplexer bits
extern const int DEN_BIT;
extern const int DRST_BIT;
extern const int HKOW_BIT;
extern const int HKOE_BIT;

// Constants
extern const uint32_t interval;        // Display update interval
extern const std::uint32_t MAX_UINT32; // max value -1 of uint32_t

extern volatile uint32_t currentStepSize;
extern volatile uint8_t keyArray[7];
extern uint8_t octave;

extern const uint32_t stepSizes0[];

extern const uint32_t stepSizes1[];

extern const uint32_t stepSizes2[];

extern const uint32_t *const stepSizeList[];

// Display driver object
extern U8G2_SSD1305_128X32_NONAME_F_HW_I2C u8g2;

void setOutMuxBit(const uint8_t bitIdx, const bool value);

void setOutPin();

// Set the row to be read
void setRow(uint8_t rowIdx);

// Read the column contents
uint8_t readCols();

// Find the highest note being pressed
int highest_unset_bit(volatile uint8_t array[]);

#endif /* KEYBOARD_INTERFACE_H */