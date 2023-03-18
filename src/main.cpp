#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <ES_CAN.h>
#include <vector>

// Constants
const uint32_t interval = 100;               // Display update interval
const std::uint32_t MAX_UINT32 = 4294967295; // max value -1 of uint32_t

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

// Look up current step size
volatile uint32_t currentStepSize;
volatile uint8_t keyArray[7];
SemaphoreHandle_t keyArrayMutex;
uint8_t knob0rotation;
uint8_t knob1rotation;
uint8_t knob2rotation;
uint8_t knob3rotation;
uint8_t previousDelta;

// Handshake out signals (default to high on startup)
volatile uint8_t OutPin[7] = {0, 0, 0, 1, 1, 1, 1};

// Handshake device ID array
std::vector<uint32_t> moduleID = {};

// Global variable to hold the device's position
uint8_t position;

// Unique device ID
uint32_t deviceID;

// CAN queue
QueueHandle_t msgInQ;
QueueHandle_t msgOutQ;
SemaphoreHandle_t CAN_TX_Semaphore;

// CAN Headers
const uint32_t ID_MODULE_INFO = 0x111;

uint8_t east;
uint8_t west;

// Display driver object
U8G2_SSD1305_128X32_NONAME_F_HW_I2C u8g2(U8G2_R0);

/*
void sampleISR()
{
    static uint32_t phaseAcc = 0;
    phaseAcc += currentStepSize;
    int32_t Vout = (phaseAcc >> 24) - 128;
    analogWrite(OUTR_PIN, Vout + 128);
}
*/

// ALTERNATE WAVEFORM(Triangle)
/*
void sampleISR(){
  static uint32_t phaseAcc = 0;
  phaseAcc += currentStepSize;
  int32_t Vout = 0;
  if ((phaseAcc >> 24) < 128){
    Vout = (phaseAcc >> 24) - 128;
  }
  else{
    Vout = 255 - (phaseAcc >> 24);
  }
  analogWrite(OUTR_PIN, Vout + 128);
}
*/

// class Knob
// {
// public:
//     Knob(int minValue, int maxValue)
//         : minValue(minValue), maxValue(maxValue) {}

//     uint8_t rotationDecoder(uint8_t currentA, uint8_t currentB, uint8_t rotationIndex)
//     {
//         // Decode knob 3 into rotation variable
//         int delta = 0;
//         uint8_t *rotation;
//         uint8_t *prevA;
//         uint8_t *prevB;
//         if (rotationIndex == 0)
//         {
//             rotation = &knob0rotation;
//             prevA = &prev0A;
//             prevB = &prev0B;
//         }
//         else if (rotationIndex == 1)
//         {
//             rotation = &knob1rotation;
//             prevA = &prev1A;
//             prevB = &prev1B;
//         }
//         else if (rotationIndex == 2)
//         {
//             rotation = &knob2rotation;
//             prevA = &prev2A;
//             prevB = &prev2B;
//         }
//         else if (rotationIndex == 3)
//         {
//             rotation = &knob3rotation;
//             prevA = &prev3A;
//             prevB = &prev3B;
//         }
//         if (*prevA == 1 && *prevB == 1 && currentA == 0 && currentB == 1)
//         {
//             delta = 1;
//         }
//         else if (*prevA == 1 && *prevB == 0 && currentB == 0 && currentA == 0)
//         {
//             delta = -1;
//         }
//         else if (*prevA == 0 && *prevB == 0 && currentA == 1 && currentB == 0)
//         {
//             delta = 1;
//         }
//         else if (*prevA == 0 && *prevB == 1 && currentB == 1 && currentA == 1)
//         {
//             delta = -1;
//         }

//         uint8_t tempRotation = *rotation + delta;

//         // Check that the new rotation value is within permitted bounds
//         if (tempRotation >= minValue && tempRotation <= maxValue)
//         {
//             // Update knob3Rotation variable atomically
//             *rotation = tempRotation;
//         }

//         // Store current state as previous state
//         *prevA = currentA;
//         *prevB = currentB;
//         return *rotation;
//     }

//     uint8_t getRotation(uint8_t knobIndex) const
//     {
//         if (knobIndex == 0)
//         {
//             return knob0rotation;
//         }
//         else if (knobIndex == 1)
//         {
//             return knob1rotation;
//         }
//         else if (knobIndex == 2)
//         {
//             return knob2rotation;
//         }
//         else
//         {
//             return knob3rotation;
//         }
//     }

//     void setLimits(int minVal, int maxVal)
//     {
//         minValue = minVal;
//         maxValue = maxVal;
//     }

// private:
//     uint8_t prev0A = 0;
//     uint8_t prev0B = 0;
//     uint8_t prev1A = 0;
//     uint8_t prev1B = 0;
//     uint8_t prev2A = 0;
//     uint8_t prev2B = 0;
//     uint8_t prev3A = 0;
//     uint8_t prev3B = 0;
//     uint8_t minValue;
//     uint8_t maxValue;
//     uint8_t knob0rotation;
//     uint8_t knob1rotation;
//     uint8_t knob2rotation;
//     uint8_t knob3rotation;
// };

// ALTERNATE WAVEFORM(Square)
void sampleISR()
{
    static uint32_t phaseAcc = 0;
    phaseAcc += currentStepSize;
    int32_t Vout = 0;
    if ((phaseAcc >> 24) < 128)
    {
        Vout = 0 - 128;
    }
    else
    {
        Vout = 255 - 128;
    }
    Vout = Vout >> (8 - knob3rotation);
    analogWrite(OUTR_PIN, Vout + 128);
}

// ALTERNATE WAVEFORM(Sine, to be optimised)
/*
void sampleISR(){
  static uint32_t phaseAcc = 0;
  phaseAcc += currentStepSize;
  double radians = phaseAcc/MAX_UINT32;
  double sineScaled = std::sin(radians)*2147483646 + 0.5;
  int32_t sineScaledFixed = static_cast<int32_t>(sineScaled);

  int32_t Vout = (sineScaledFixed >> 24) - 128;
  analogWrite(OUTR_PIN, Vout + 128);
}
*/

uint8_t getDeviceID()
{
    uint32_t uid = HAL_GetUIDw0();
    // Create a hash function to process UID
    return (uid >> 16) ^ (uid >> 8) ^ uid;
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
uint8_t rotationDecoder(uint8_t &prevA, uint8_t &prevB, uint8_t currentA, uint8_t currentB, uint8_t rotation)
{
    // Decode knob 3 into rotation variable
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
    if (tempRotation >= 0 && tempRotation <= 8)
    {
        // Update knob3Rotation variable atomically
        rotation = tempRotation;
    }

    // Store current state as previous state
    prevA = currentA;
    prevB = currentB;
    return rotation;
}

void sendCAN_ModuleInfo(uint8_t position, uint32_t ID)
{
    uint8_t TX_Message[8] = {0};
    TX_Message[0] = position;
    TX_Message[1] = (uint8_t)(ID >> 24); // Most significant byte
    TX_Message[2] = (uint8_t)(ID >> 16);
    TX_Message[3] = (uint8_t)(ID >> 8);
    TX_Message[4] = (uint8_t)(ID); // Least significant byte

    // Send CAN Message
    CAN_TX(ID_MODULE_INFO, TX_Message);
}

void sendCAN_HSEnd()
{
    uint8_t TX_Message[8] = {0};
    TX_Message[0] = 'E';

    // Send CAN Message
    CAN_TX(ID_MODULE_INFO, TX_Message);
}

void scanKeysTask(void *pvParameters)
{
    const TickType_t xFrequency = 20 / portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    // Knob knob = Knob(0, 8);
    uint8_t knobIndex = 0;
    uint8_t knobValue;
    uint8_t prevA3;
    uint8_t prevB3;
    uint8_t prevA2;
    uint8_t prevB2;
    uint8_t prevA1;
    uint8_t prevB1;
    uint8_t prevA0;
    uint8_t prevB0;
    while (1)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        for (uint8_t i = 0; i < 7; i++)
        {
            setRow(i);
            delayMicroseconds(3);
            digitalWrite(OUT_PIN, OutPin[i]);
            digitalWrite(REN_PIN, HIGH);
            xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
            keyArray[i] = readCols();
            // Set row select enable pin (REN) high
            if (i == 3)
            {
                uint8_t currentA3 = keyArray[i] & 0b00000001;
                uint8_t currentB3 = (keyArray[i] & 0b00000010) >> 1;
                knob3rotation = rotationDecoder(prevA3, prevB3, currentA3, currentB3, knob3rotation);
                u8g2.setCursor(70, 20);
                u8g2.print(knob3rotation);
                knobIndex = 2;
                uint8_t currentA2 = (keyArray[i] & 0b00000100) >> 2;
                uint8_t currentB2 = (keyArray[i] & 0b00001000) >> 3;
                knob2rotation = rotationDecoder(prevA2, prevB2, currentA2, currentB2, knob2rotation);
                u8g2.setCursor(60, 20);
                u8g2.print(knob2rotation);
            }
            else if (i == 4)
            {
                knobIndex = 1;
                uint8_t currentA1 = keyArray[i] & 0b00000001;
                uint8_t currentB1 = (keyArray[i] & 0b00000010) >> 1;
                knob1rotation = rotationDecoder(prevA1, prevB1, currentA1, currentB1, knob1rotation);
                u8g2.setCursor(50, 20);
                u8g2.print(knob1rotation);
                knobIndex = 0;
                uint8_t currentA0 = (keyArray[i] & 0b00000100) >> 2;
                uint8_t currentB0 = (keyArray[i] & 0b00001000) >> 3;
                knob0rotation = rotationDecoder(prevA0, prevB0, currentA0, currentB0, knob0rotation);
                u8g2.setCursor(40, 20);
                u8g2.print(knob0rotation);
            }
            else if (i == 5)
            {
                uint8_t currentWestHS = (keyArray[i] & 0b00001000) >> 3;
                u8g2.setCursor(80, 20);
                u8g2.print(currentWestHS);
            }
            else if (i == 6)
            {
                uint8_t currentEastHS = (keyArray[i] & 0b00001000) >> 3;
                u8g2.setCursor(90, 20);
                u8g2.print(currentEastHS);
            }
            else
            {
                u8g2.setCursor(2 + 10 * i, 20);
                u8g2.print(keyArray[i], HEX);
            }

            for (size_t i = 0; i < moduleID.size(); i++)
            {
                u8g2.setCursor(20 * i, 30);
                u8g2.print(moduleID[i], HEX);
            }

            u8g2.setCursor(50, 30);
            u8g2.print("position");
            u8g2.print(position);

            // u8g2.setCursor(50, 30);
            // u8g2.print(deviceID, HEX);

            xSemaphoreGive(keyArrayMutex);
        }
    }
}

void update_display()
{
    uint32_t ID;
    uint8_t RX_Message[8] = {0};
    xQueueReceive(msgInQ, RX_Message, portMAX_DELAY);
    u8g2.setCursor(66, 30);
    u8g2.print(RX_Message[0]);
    u8g2.print(RX_Message[1]);
    u8g2.print(RX_Message[2]);
    u8g2.print(RX_Message[3]);
    u8g2.print(RX_Message[4]);
    // transfer internal memory to the display
}

void updateDisplayTask(void *pvParameters)
{
    int highestBit = -1;
    const uint32_t stepSizes[] = {
        51076057, 54113197, 57330935, 60740010, 64351799, 68178356, 72232452, 76527617, 81078186, 85899346, 91007187, 96418756 // G# / Ab (830.61 Hz)
    };

    const TickType_t xFrequency = 100 / portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        u8g2.clearBuffer();                 // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        // Update display
        xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
        highestBit = highest_unset_bit(keyArray);
        xSemaphoreGive(keyArrayMutex);
        currentStepSize = (highestBit < 0) ? 0 : stepSizes[highestBit];
        u8g2.setCursor(2, 10);
        // update_display();
        u8g2.print(currentStepSize);
        // u8g2.print(currentStepSize, HEX);
        u8g2.sendBuffer(); // transfer internal memory to the display

        // Toggle LED
        digitalToggle(LED_BUILTIN);
    }
}

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

bool handshakeRoutine(uint8_t &position)
{
    bool westMost = false;
    bool eastMost = false;
    uint8_t eastHS = 1;
    uint8_t westHS = 1;
    bool detect = false;
    uint8_t RX_Message[8] = {0};

    // Set East & West HS output signals
    setRow(5);
    delayMicroseconds(5);
    digitalWrite(OUT_PIN, 1);
    digitalWrite(REN_PIN, 1);
    delayMicroseconds(5);

    setRow(6);
    delayMicroseconds(5);
    digitalWrite(OUT_PIN, 1);
    digitalWrite(REN_PIN, 1);
    delayMicroseconds(5);

    delay(100);

    // Get current East & West HS input signals
    setRow(5);
    delayMicroseconds(5);
    digitalWrite(REN_PIN, 1);
    westHS = (readCols() & 0b00001000) >> 3;
    delayMicroseconds(5);

    setRow(6);
    delayMicroseconds(5);
    digitalWrite(REN_PIN, 1);
    eastHS = (readCols() & 0b00001000) >> 3;
    delayMicroseconds(5);

    u8g2.clearBuffer(); // clear the internal memory

    u8g2.setFont(u8g2_font_t0_11_tf);

    u8g2.setCursor(20, 20);
    u8g2.print(westHS);
    u8g2.setCursor(40, 20);
    u8g2.print(eastHS);

    u8g2.sendBuffer();

    delay(100);

    if (westHS && eastHS)
    {
        delay(100);

        setRow(5);
        delayMicroseconds(5);
        digitalWrite(REN_PIN, 1);
        westHS = (readCols() & 0b00001000) >> 3;
        delayMicroseconds(5);

        setRow(6);
        delayMicroseconds(5);
        digitalWrite(REN_PIN, 1);
        eastHS = (readCols() & 0b00001000) >> 3;
        delayMicroseconds(5);

        u8g2.clearBuffer(); // clear the internal memory

        u8g2.setFont(u8g2_font_t0_11_tf);

        u8g2.setCursor(20, 20);
        u8g2.print(westHS);
        u8g2.setCursor(40, 20);
        u8g2.print(eastHS);

        u8g2.sendBuffer();

        delay(100);
    }

    // Initial detection
    if (westHS && !eastHS)
    {
        // West-most Module
        westMost = true;

        delay(500);

        // Turn East HS signal off
        setRow(6);
        delayMicroseconds(5);
        digitalWrite(OUT_PIN, 0);
        digitalWrite(REN_PIN, 1);
        delayMicroseconds(5);

        setRow(5);
        delayMicroseconds(5);
        digitalWrite(OUT_PIN, 0);
        digitalWrite(REN_PIN, 1);
        delayMicroseconds(5);

        u8g2.clearBuffer(); // clear the internal memory

        u8g2.setFont(u8g2_font_t0_11_tf);

        u8g2.setCursor(20, 20);
        u8g2.print("set low");

        u8g2.sendBuffer();

        delay(200);

        // Set position
        position = 0;

        // broadcast CAN signal
        sendCAN_ModuleInfo(position, deviceID);

        // Go to end of function to wait
        detect = true;
    }

    if (westHS && eastHS)
    {
        // Only Module
        moduleID.push_back(deviceID);
        position = 10;
        return false;

        u8g2.clearBuffer(); // clear the internal memory

        u8g2.setFont(u8g2_font_t0_11_tf);

        u8g2.setCursor(20, 20);
        u8g2.print("only");

        u8g2.sendBuffer();

        delay(100);
    }

    if (eastHS)
    {
        // East-most Module
        eastMost = true;
    }

    while (!detect)
    {
        setRow(5);
        delayMicroseconds(5);
        digitalWrite(REN_PIN, 1);
        delayMicroseconds(5);
        westHS = (readCols() & 0b00001000) >> 3;

        u8g2.clearBuffer(); // clear the internal memory

        u8g2.setFont(u8g2_font_t0_11_tf);

        u8g2.setCursor(20, 20);
        u8g2.print("detecting");

        u8g2.setCursor(80, 20);
        u8g2.print(westHS);

        u8g2.setCursor(90, 20);
        u8g2.print(millis());

        if (eastMost)
        {
            u8g2.setCursor(20, 30);
            u8g2.print("eastmost");
        }

        u8g2.sendBuffer();

        if (westHS != 0)
        {
            // Recieve CAN signal
            uint32_t ID;
            while (CAN_CheckRXLevel())
                CAN_RX(ID, RX_Message);

            // Increment position
            position = RX_Message[0] + 1;

            sendCAN_ModuleInfo(position, deviceID);

            // Turn East HS signal off
            setRow(6);
            delayMicroseconds(3);
            digitalWrite(OUT_PIN, LOW);
            digitalWrite(REN_PIN, HIGH);

            detect = true;
        }
    }

    if (eastMost)
    {
        // TODO:  Send CAN signal to end handshake process
        sendCAN_HSEnd();

        u8g2.clearBuffer(); // clear the internal memory

        u8g2.setFont(u8g2_font_t0_11_tf);

        u8g2.setCursor(20, 20);
        u8g2.print("exiting");

        u8g2.sendBuffer();

        delay(100);

        return true;
    }

    bool waitMode = true;
    while (waitMode)
    {
        u8g2.clearBuffer(); // clear the internal memory

        u8g2.setFont(u8g2_font_t0_11_tf);

        u8g2.setCursor(20, 20);
        u8g2.print("waiting");

        u8g2.sendBuffer();
        // TODO: recieve CAN signal from other modules
        uint32_t ID;
        while (CAN_CheckRXLevel())
            CAN_RX(ID, RX_Message);

        if (RX_Message[0] != 'E' && ID == ID_MODULE_INFO)
        {
            uint32_t moduleID_Recieve = ((uint32_t)RX_Message[1] << 24) | ((uint32_t)RX_Message[2] << 16) | ((uint32_t)RX_Message[3] << 8) | (uint32_t)RX_Message[4];
            u8g2.clearBuffer(); // clear the internal memory

            u8g2.setFont(u8g2_font_t0_11_tf);

            u8g2.setCursor(20, 20);
            u8g2.print(moduleID_Recieve);

            u8g2.sendBuffer();
            delay(3000);

            moduleID.push_back(moduleID_Recieve);
        }
        else if (RX_Message[0] == 'E' && ID == ID_MODULE_INFO)
        {
            return true;
        }
    }

    u8g2.clearBuffer(); // clear the internal memory

    u8g2.setFont(u8g2_font_t0_11_tf);

    u8g2.setCursor(20, 20);
    u8g2.print("erroring");

    u8g2.sendBuffer();

    delay(100);

    // Default to return false
    return false;
}

void CAN_RX_ISR(void)
{
    uint8_t RX_Message_ISR[8];
    uint32_t ID;
    CAN_RX(ID, RX_Message_ISR);
    xQueueSendFromISR(msgInQ, RX_Message_ISR, NULL);
}

void CAN_TX_ISR(void)
{
    xSemaphoreGiveFromISR(CAN_TX_Semaphore, NULL);
}

void canBusInitRoutine()
{
    // Initialise CAN TX semaphore
    CAN_TX_Semaphore = xSemaphoreCreateCounting(3, 3);

    // Initialize CAN bus and disable looping
    uint32_t status = CAN_Init(false);

    // Set filters for CAN signals
    status = setCANFilter(ID_MODULE_INFO, 0x7ff); // Send Handshake Device Info
    // status = setCANFilter(0x222, 0x7ff); // End Handshake Sequence

    CAN_RegisterRX_ISR(CAN_RX_ISR);
    CAN_RegisterTX_ISR(CAN_TX_ISR);

    status = CAN_Start();

    msgInQ = xQueueCreate(36, 8);
    msgOutQ = xQueueCreate(36, 8);
}

void setup()
{
    // put your setup code here, to run once:

    // Set pin directions
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

    // Initialise display
    setOutMuxBit(DRST_BIT, LOW); // Assert display logic reset
    delayMicroseconds(2);
    setOutMuxBit(DRST_BIT, HIGH); // Release display logic reset
    u8g2.begin();
    setOutMuxBit(DEN_BIT, HIGH); // Enable display power supply

    // Get unique ID
    deviceID = HAL_GetUIDw0();

    // Initialize CAN bus
    canBusInitRoutine();

    // Perform Handshake Routinue
    handshakeRoutine(position);

    TIM_TypeDef *Instance = TIM1;
    HardwareTimer *sampleTimer = new HardwareTimer(Instance);
    sampleTimer->setOverflow(22000, HERTZ_FORMAT);
    sampleTimer->attachInterrupt(sampleISR);
    sampleTimer->resume();

    keyArrayMutex = xSemaphoreCreateMutex();
    TaskHandle_t scanKeysHandle = NULL;
    xTaskCreate(
        scanKeysTask, /* Function that implements the task */
        "scanKeys",   /* Text name for the task */
        200,          /* Stack size in words, not bytes */
        NULL,         /* Parameter passed into the task */
        2,            /* Task priority */
        &scanKeysHandle);

    TaskHandle_t updateDisplayHandle = NULL;
    xTaskCreate(
        updateDisplayTask, /* Function that implements the task */
        "updateDisplay",   /* Text name for the task */
        200,               /* Stack size in words, not bytes */
        NULL,              /* Parameter passed into the task */
        1,                 /* Task priority */
        &scanKeysHandle);

    vTaskStartScheduler();
}

void loop()
{
}