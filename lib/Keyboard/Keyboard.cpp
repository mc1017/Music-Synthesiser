#include <Arduino.h>
#include <U8g2lib.h>
#include <Keyboard.h>
// Pin definitions
// Row select and enable
const int RA0_PIN = D3;
const int RA1_PIN = D6;
const int RA2_PIN = D12;
const int REN_PIN = A5;

// Matrix input and output
const int C0_PIN = A2;
const int C1_PIN = D9;
const int C2_PIN = A6;
const int C3_PIN = D1;
const int OUT_PIN = D11;

// Audio analogue out
const int OUTL_PIN = A4;
const int OUTR_PIN = A3;

// Joystick analogue in
const int JOYY_PIN = A0;
const int JOYX_PIN = A1;

// Output multiplexer bits
const int DEN_BIT = 3;
const int DRST_BIT = 4;
const int HKOW_BIT = 5;
const int HKOE_BIT = 6;

// Constants
const uint32_t interval = 100; // Display update interval
volatile uint8_t keyArray[7];
volatile uint8_t keyArray2[7] = {15,15,15,15,15,15,15};
volatile uint8_t keyArray3[7] = {15,15,15,15,15,15,15};

// Display driver object
U8G2_SSD1305_128X32_NONAME_F_HW_I2C u8g2(U8G2_R0);

Knob::Knob() {}

Knob::Knob(uint8_t minValue, uint8_t maxValue)
    : minValue(minValue), maxValue(maxValue), rotation(0), prevA(0), prevB(0) {}

void Knob::setLimits(uint8_t minValue, uint8_t maxValue)
{
    this->minValue = minValue;
    this->maxValue = maxValue;
}

uint8_t Knob::getRotation() const
{
    return rotation;
}

void Knob::updateRotation(uint8_t currentA, uint8_t currentB)
{

    int delta = 0;
    if (prevA == 1 && prevB == 1 && currentA == 0 && currentB == 1)
    {
        delta = 1;
    }
    else if (prevA == 1 && prevB == 0 && currentB == 0 && currentA == 0)
    {
        delta = -1;
    }
    else if (prevA == 0 && prevB == 0 && currentA == 1 && currentB == 0)
    {
        delta = 1;
    }
    else if (prevA == 0 && prevB == 1 && currentB == 1 && currentA == 1)
    {
        delta = -1;
    }

    uint8_t tempRotation = rotation + delta;

    // Check that the new rotation value is within permitted bounds
    if (tempRotation >= minValue && tempRotation <= maxValue)
    {
        __atomic_store_n(&rotation, tempRotation, __ATOMIC_RELAXED);
    }
    // Store current state as previous state
    prevA = currentA;
    prevB = currentB;
}

Knob knob[4] = {Knob(0, 4), Knob(0, 3), Knob(0, 2), Knob(0, 8)};

// Function to set outputs using key matrix
void setOutMuxBit(const uint8_t bitIdx, const bool value)
{
    digitalWrite(REN_PIN, LOW);
    digitalWrite(RA0_PIN, bitIdx & 0x01);
    digitalWrite(RA1_PIN, bitIdx & 0x02);
    digitalWrite(RA2_PIN, bitIdx & 0x04);
    digitalWrite(OUT_PIN, value);
    digitalWrite(REN_PIN, HIGH);
    delayMicroseconds(2);
    digitalWrite(REN_PIN, LOW);
}

void setRow(uint8_t rowIdx)
{
    // Set row select enable pin (REN) high
    digitalWrite(REN_PIN, LOW);
    digitalWrite(RA0_PIN, LOW);
    digitalWrite(RA1_PIN, LOW);
    digitalWrite(RA2_PIN, LOW);
    // Read inputs from column 0
    uint8_t bit_value0 = ((rowIdx >> 0) & 1);
    uint8_t bit_value1 = ((rowIdx >> 1) & 1);
    uint8_t bit_value2 = ((rowIdx >> 2) & 1);
    if (bit_value0)
    {
        digitalWrite(RA0_PIN, HIGH);
    }
    if (bit_value1)
    {
        digitalWrite(RA1_PIN, HIGH);
    }
    if (bit_value2)
    {
        digitalWrite(RA2_PIN, HIGH);
    }
    // Set row select enable pin (REN) high
    digitalWrite(REN_PIN, HIGH);
}
uint8_t readCols()
{

    uint8_t C0 = digitalRead(C0_PIN);
    uint8_t C1 = digitalRead(C1_PIN);
    uint8_t C2 = digitalRead(C2_PIN);
    uint8_t C3 = digitalRead(C3_PIN);

    digitalWrite(REN_PIN, LOW);

    // Concatenate the four bits into a single byte and return
    return (C3 << 3) | (C2 << 2) | (C1 << 1) | C0;
}

int highest_unset_bit(volatile uint8_t array[])
{
    uint16_t merged = ((uint16_t)array[2] & 0x0F) << 8 |
                      ((uint16_t)array[1] & 0x0F) << 4 |
                      ((uint16_t)array[0] & 0x0F);
    int index = 0;
    uint16_t mask = 0x800;
    int i;
    for (i = 11; i >= 0; i--)
    {
        if ((merged & mask) == 0)
        {
            break;
        }
        mask >>= 1;
    }
    return i;
}

void setOutPin()
{
    uint8_t OutPin[7] = {0, 0, 0, 1, 1, 1, 1};

    for (int i = 0; i < 7; i++)
    {
        setRow(i);                        // Set row address
        digitalWrite(OUT_PIN, OutPin[i]); // Set value to latch in DFF
        digitalWrite(REN_PIN, 1);         // Enable selected row
        delayMicroseconds(3);             // Wait for column inputs to stabilise
        digitalWrite(REN_PIN, 0);         // Disable selected row
    }
}