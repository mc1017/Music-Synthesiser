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
SemaphoreHandle_t minMaxMutex;
SemaphoreHandle_t rotationMutex;
SemaphoreHandle_t currentStepSizeMutex;

// Display driver object
U8G2_SSD1305_128X32_NONAME_F_HW_I2C u8g2(U8G2_R0);

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

// Transmitter / Reciever setting
bool transmitter = false;
bool multipleModule = false;

uint8_t RX_Message[8];
SemaphoreHandle_t RX_MessageMutex;
uint8_t CAN_Tx[8];
SemaphoreHandle_t CAN_TXSemaphore;
// CAN Headers
const uint32_t ID_MODULE_INFO = 0x111;

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

class Knob
{
public:
    Knob() {}

    Knob(uint8_t minValue, uint8_t maxValue)
        : minValue(minValue), maxValue(maxValue), rotation(0), prevA(0), prevB(0) {}

    void setLimits(uint8_t minValue, uint8_t maxValue)
    {
        xSemaphoreTake(minMaxMutex, portMAX_DELAY);
        this->minValue = minValue;
        this->maxValue = maxValue;
        xSemaphoreGive(minMaxMutex);
    }

    uint8_t getRotation() const
    {
        return rotation;
    }

    void updateRotation(uint8_t currentA, uint8_t currentB)
    {

        int delta = 0;
        xSemaphoreTake(rotationMutex, portMAX_DELAY);
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
        xSemaphoreGive(rotationMutex);
        // Store current state as previous state
        prevA = currentA;
        prevB = currentB;
    }

private:
    uint8_t prevA;
    uint8_t prevB;
    uint8_t minValue;
    uint8_t maxValue;
    uint8_t rotation;
};

Knob knob[4] = {Knob(0, 2), Knob(0, 3), Knob(0, 10), Knob(0, 8)};

void sampleISR()
{
    static uint32_t phaseAcc[12] = {0};
    const uint32_t stepSizes[] = {
        51076057, 54113197, 57330935, 60740010, 64351799, 68178356, 72232452, 76527617, 81078186, 85899346, 91007187, 96418756 // G# / Ab (830.61 Hz)
    };
    uint8_t activeNotes = 0;
    int32_t Vout = 0;
    // Calculate the output for each active note

    for (uint8_t i = 0; i < 3; ++i)
    {
        for (uint8_t j = 0; j < 4; ++j)
        {
            uint8_t key = ((keyArray[i] >> j) & 1);
            if (key == 0)
            {
                phaseAcc[i * 4 + j] += stepSizes[i * 4 + j];
                int32_t noteVout = (phaseAcc[i * 4 + j] >> 24) - 128;
                noteVout = noteVout >> (8 - knob[3].getRotation());
                Vout += noteVout;
                ++activeNotes;

                // xQueueSend( msgOutQ, TX_Message, portMAX_DELAY);
            }
        }
    }

    // Avoid Clipping
    if (activeNotes > 1)
    {
        Vout = Vout / activeNotes;
    }
    analogWrite(OUTR_PIN, Vout + 128);
}

// ALTERNATE WAVEFORM(Square)
// void sampleISR()
// {
//     static uint32_t phaseAcc = 0;
//     phaseAcc += currentStepSize;
//     int32_t Vout = 0;
//     if ((phaseAcc >> 24) < 128)
//     {
//         Vout = 0 - 128;
//     }
//     else
//     {
//         Vout = 255 - 128;
//     }
//     Vout = Vout >> (10 - knob[3].getRotation());
//     analogWrite(OUTR_PIN, Vout + 128);
// }

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

void CAN_RX_ISR(void)
{
    uint8_t RX_Message_ISR[8];
    uint32_t ID;
    CAN_RX(ID, RX_Message_ISR);
    xQueueSendFromISR(msgInQ, RX_Message_ISR, NULL);
}

void CAN_TX_ISR(void)
{
    xSemaphoreGiveFromISR(CAN_TXSemaphore, NULL);
}

void canBusInitHS()
{
    uint32_t status = CAN_Init(false);

    // Set filters for CAN signals
    status = setCANFilter(ID_MODULE_INFO, 0x7ff); // Send Handshake Device Info

    status = CAN_Start();
}

void canBusInitRoutine()
{

    // Initialize CAN bus and disable looping
    uint32_t status = CAN_Init(false);

    CAN_TXSemaphore = xSemaphoreCreateCounting(3, 3);

    // Set filters for CAN signals
    status = setCANFilter(ID_MODULE_INFO, 0x7ff); // Send Handshake Device Info
    // status = setCANFilter(0x222, 0x7ff); // End Handshake Sequence

    CAN_RegisterRX_ISR(CAN_RX_ISR);
    CAN_RegisterTX_ISR(CAN_TX_ISR);

    status = CAN_Start();

    msgInQ = xQueueCreate(36, 8);
    msgOutQ = xQueueCreate(36, 8);
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

void scanKeysTask(void *pvParameters)
{
    const TickType_t xFrequency = 20 / portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    uint8_t tmp_RX[8] = {0};
    uint8_t test_array[5] = {0};
    uint8_t TX_Message[8] = {0};

    while (1)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (!transmitter && multipleModule)
        {
            xSemaphoreTake(RX_MessageMutex, portMAX_DELAY);
            tmp_RX[0] = RX_Message[0];
            tmp_RX[1] = RX_Message[1];
            tmp_RX[2] = RX_Message[2];
            tmp_RX[4] = RX_Message[4];
            tmp_RX[3] = RX_Message[3];
            tmp_RX[5] = RX_Message[5];
            tmp_RX[6] = RX_Message[6];
            tmp_RX[7] = RX_Message[7];
            xSemaphoreGive(RX_MessageMutex);
        }
        // uint8_t TX_Message[8] = {0};
        for (uint8_t i = 0; i < 5; i++)
        {
            setRow(i);
            delayMicroseconds(3);
            xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
            keyArray[i] = readCols();
            TX_Message[i] = keyArray[i];
            if (i < 3)
            {
                if (!transmitter && multipleModule)
                    keyArray[i] = keyArray[i] & tmp_RX[i]; //~((keyArray[i])&(tmp_RX[i]))<<4;
                else
                    keyArray[i] = keyArray[i];
            }
            test_array[i] = keyArray[i];
            // TX_Message[i] = keyArray[i];
            // xQueueSend( msgOutQ, TX_Message, portMAX_DELAY);

            uint8_t currentA0 = (keyArray[i] & 0b00000100) >> 2;
            uint8_t currentB0 = (keyArray[i] & 0b00001000) >> 3;
            uint8_t currentA1 = keyArray[i] & 0b00000001;
            uint8_t currentB1 = (keyArray[i] & 0b00000010) >> 1;
            uint8_t currentA2 = (keyArray[i] & 0b00000100) >> 2;
            uint8_t currentB2 = (keyArray[i] & 0b00001000) >> 3;
            uint8_t currentA3 = keyArray[i] & 0b00000001;
            uint8_t currentB3 = (keyArray[i] & 0b00000010) >> 1;
            switch (i)
            {
            case 3:
                knob[3].updateRotation(currentA3, currentB3);
                u8g2.setCursor(80, 20);
                u8g2.print(knob[3].getRotation());
                knob[2].updateRotation(currentA2, currentB2);
                u8g2.setCursor(65, 20);
                u8g2.print(knob[2].getRotation());
                break;
            case 4:
                knob[1].updateRotation(currentA1, currentB1);
                u8g2.setCursor(50, 20);
                u8g2.print(knob[1].getRotation());
                knob[0].updateRotation(currentA0, currentB0);
                u8g2.setCursor(35, 20);
                u8g2.print(knob[0].getRotation());
                break;
            case 5:

            default:
                u8g2.setCursor(2 + 10 * i, 20);
                u8g2.print(keyArray[i], HEX);
                break;
            }
            xSemaphoreGive(keyArrayMutex);
        }
        TX_Message[5] = position;
        if (transmitter && multipleModule)
            xQueueSend(msgOutQ, TX_Message, portMAX_DELAY);

        if (!transmitter && multipleModule)
        {
            u8g2.setCursor(60, 10);
            u8g2.print(RX_Message[4]);
            u8g2.print(RX_Message[3]);
            u8g2.print(RX_Message[2]);
            u8g2.print(RX_Message[1]);
            u8g2.print(RX_Message[0]);
        }
    }
}

void update_display()
{
    uint32_t ID;
    xSemaphoreTake(RX_MessageMutex, portMAX_DELAY);
    u8g2.setCursor(60, 10);
    u8g2.print(RX_Message[6]);
    u8g2.print(RX_Message[5]);
    u8g2.print(RX_Message[4]);
    u8g2.print(RX_Message[3]);
    u8g2.print(RX_Message[2]);
    u8g2.print(RX_Message[1]);
    u8g2.print(RX_Message[0]);
    xSemaphoreGive(RX_MessageMutex);
    // xQueueReceive(msgInQ, RX_Message, portMAX_DELAY);

    // transfer internal memory to the display
}

void updateDisplayTask(void *pvParameters)
{
    int highestBit = -1;
    const uint32_t notes[] = {};
    const TickType_t xFrequency = 100 / portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // if (transmitter && multipleModule)
    //     uint8_t TX_Message[8] = {0};
    // TX_Message[0] = 7;
    // TX_Message[1] = 6;
    // TX_Message[2] = 4;
    // TX_Message[3] = 8;
    // TX_Message[4] = 9;
    // TX_Message[5] = 9;
    // TX_Message[6] = 9;
    // TX_Message[7] = 9;

    while (1)
    {
        u8g2.clearBuffer();                 // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
        const uint32_t stepSizes[] = {
            51076057, 54113197, 57330935, 60740010, 64351799, 68178356, 72232452, 76527617, 81078186, 85899346, 91007187, 96418756 // G# / Ab (830.61 Hz)
        };
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        // Update display
        xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
        highestBit = highest_unset_bit(keyArray);
        xSemaphoreGive(keyArrayMutex);
        __atomic_store_n(&currentStepSize, (highestBit < 0) ? 0 : stepSizes[highestBit], __ATOMIC_SEQ_CST);
        u8g2.setCursor(2, 10);
        u8g2.print(currentStepSize);
        // u8g2.print(currentStepSize, HEX);
        // if (!transmitter && multipleModule)
        //     update_display();

        u8g2.sendBuffer(); // transfer internal memory to the display

        // Toggle LED
        digitalToggle(LED_BUILTIN);
    }
}

// CAN Recieve Task
void CanComTask(void *pvParameters)
{
    uint8_t tmp_RX_Message[8] = {0};

    while (1)
    {
        xQueueReceive(msgInQ, tmp_RX_Message, portMAX_DELAY);
        xSemaphoreTake(RX_MessageMutex, portMAX_DELAY);
        RX_Message[0] = tmp_RX_Message[0];
        RX_Message[1] = tmp_RX_Message[1];
        RX_Message[2] = tmp_RX_Message[2];
        RX_Message[3] = tmp_RX_Message[3];
        RX_Message[4] = tmp_RX_Message[4];
        RX_Message[5] = tmp_RX_Message[5];
        RX_Message[6] = tmp_RX_Message[6];
        RX_Message[7] = tmp_RX_Message[7];
        xSemaphoreGive(RX_MessageMutex);
    }
}

// CAN send Task
void CAN_TX_Task(void *pvParameters)
{
    uint8_t msgOut[8];
    while (1)
    {
        xQueueReceive(msgOutQ, msgOut, portMAX_DELAY);
        xSemaphoreTake(CAN_TXSemaphore, portMAX_DELAY);
        CAN_TX(0x111, msgOut);
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

bool handshakeRoutine(uint8_t &position)
{
    bool westMost = false;
    bool eastMost = false;
    uint8_t eastHS = 1;
    uint8_t westHS = 1;
    bool detect = false;
    uint8_t Message[8] = {0};

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
                CAN_RX(ID, Message);

            // Increment position
            position = Message[0] + 1;

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
            CAN_RX(ID, Message);

        if (Message[0] == 'E' && ID == ID_MODULE_INFO)
        {
            return true;
        }
        else
        {
            moduleID.push_back(((uint32_t)Message[1] << 24) | ((uint32_t)Message[2] << 16) | ((uint32_t)Message[3] << 8) | (uint32_t)Message[4]);
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

    deviceID = HAL_GetUIDw0();

    // Initialise display
    setOutMuxBit(DRST_BIT, LOW); // Assert display logic reset
    delayMicroseconds(2);
    setOutMuxBit(DRST_BIT, HIGH); // Release display logic reset
    u8g2.begin();
    setOutMuxBit(DEN_BIT, HIGH); // Enable display power supply
    TIM_TypeDef *Instance = TIM1;
    HardwareTimer *sampleTimer = new HardwareTimer(Instance);
    sampleTimer->setOverflow(22000, HERTZ_FORMAT);
    sampleTimer->attachInterrupt(sampleISR);
    sampleTimer->resume();

    canBusInitRoutine();

    multipleModule = handshakeRoutine(position);

    // Resend Handshake End signal to ensure all modules exit the loop
    delay(100);
    sendCAN_HSEnd();

    setOutPin();

    minMaxMutex = xSemaphoreCreateMutex();
    keyArrayMutex = xSemaphoreCreateMutex();
    rotationMutex = xSemaphoreCreateMutex();
    currentStepSizeMutex = xSemaphoreCreateMutex();
    RX_MessageMutex = xSemaphoreCreateMutex();

    if (position == 0)
        transmitter = false;
    else
        transmitter = true;

    if (multipleModule && transmitter)
    {
        TaskHandle_t scanKeysHandle = NULL;
        xTaskCreate(
            scanKeysTask, /* Function that implements the task */
            "scanKeys",   /* Text name for the task */
            500,          /* Stack size in words, not bytes */
            NULL,         /* Parameter passed into the task */
            2,            /* Task priority */
            &scanKeysHandle);

        TaskHandle_t updateDisplayHandle = NULL;
        xTaskCreate(
            updateDisplayTask, /* Function that implements the task */
            "updateDisplay",   /* Text name for the task */
            500,               /* Stack size in words, not bytes */
            NULL,              /* Parameter passed into the task */
            1,                 /* Task priority */
            &scanKeysHandle);

        TaskHandle_t CanSendHandle = NULL;
        xTaskCreate(
            CAN_TX_Task, /* Function that implements the task */
            "CAN_TX",    /* Text name for the task */
            200,         /* Stack size in words, not bytes */
            NULL,        /* Parameter passed into the task */
            3,           /* Task priority */
            &scanKeysHandle);
    }
    else if (multipleModule && !transmitter)
    {
        TaskHandle_t scanKeysHandle = NULL;
        xTaskCreate(
            scanKeysTask, /* Function that implements the task */
            "scanKeys",   /* Text name for the task */
            500,          /* Stack size in words, not bytes */
            NULL,         /* Parameter passed into the task */
            2,            /* Task priority */
            &scanKeysHandle);

        TaskHandle_t updateDisplayHandle = NULL;
        xTaskCreate(
            updateDisplayTask, /* Function that implements the task */
            "updateDisplay",   /* Text name for the task */
            500,               /* Stack size in words, not bytes */
            NULL,              /* Parameter passed into the task */
            1,                 /* Task priority */
            &scanKeysHandle);

        TaskHandle_t CanComHandle = NULL;
        xTaskCreate(
            CanComTask, /* Function that implements the task */
            "CanCom",   /* Text name for the task */
            200,        /* Stack size in words, not bytes */
            NULL,       /* Parameter passed into the task */
            3,          /* Task priority */
            &scanKeysHandle);
    }
    else
    {
        TaskHandle_t scanKeysHandle = NULL;
        xTaskCreate(
            scanKeysTask, /* Function that implements the task */
            "scanKeys",   /* Text name for the task */
            500,          /* Stack size in words, not bytes */
            NULL,         /* Parameter passed into the task */
            2,            /* Task priority */
            &scanKeysHandle);

        TaskHandle_t updateDisplayHandle = NULL;
        xTaskCreate(
            updateDisplayTask, /* Function that implements the task */
            "updateDisplay",   /* Text name for the task */
            500,               /* Stack size in words, not bytes */
            NULL,              /* Parameter passed into the task */
            1,                 /* Task priority */
            &scanKeysHandle);
    }

    vTaskStartScheduler();
}

void loop()
{
}