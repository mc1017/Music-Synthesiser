#include <Arduino.h>
#include <U8g2lib.h>
#include <Keyboard.h>
#include <ES_CAN.h>
#include <CAN_HandShake.h>
#include <vector>

// Handshake device ID array
std::vector<uint32_t> moduleID = {};

// Global variable to hold the device's position
uint8_t position;
uint32_t deviceID;
uint32_t deviceIDs[3];

// Transmitter / Reciever setting
bool transmitter = false;
bool multipleModule = false;

// Handshake out signals (default to high on startup)
volatile uint8_t OutPin[7] = {0, 0, 0, 1, 1, 1, 1};

// CAN Headers
const uint32_t ID_MODULE_INFO = 0x111;

uint8_t RX_Message[8];
uint8_t CAN_Tx[8];

void sendCAN_HSEnd()
{
    uint8_t TX_Message[8] = {0};
    TX_Message[0] = 'E';

    // Send CAN Message
    CAN_TX(ID_MODULE_INFO, TX_Message);
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

void debugPrintEastWest(uint8_t eastHS, uint8_t westHS)
{

    u8g2.clearBuffer(); // clear the internal memory
    u8g2.setFont(u8g2_font_t0_11_tf);
    u8g2.setCursor(20, 20);
    u8g2.print(westHS);
    u8g2.setCursor(40, 20);
    u8g2.print(eastHS);
    u8g2.sendBuffer();
}

void debugPrintStatement(char *statement)
{

    u8g2.clearBuffer(); // clear the internal memory
    u8g2.setFont(u8g2_font_t0_11_tf);
    u8g2.setCursor(20, 20);
    u8g2.print(statement);
    u8g2.sendBuffer();
}

bool waitMode(uint8_t Message[])
{
    while (true)
    {

        debugPrintStatement("Waiting");

        setRow(6);
        delayMicroseconds(5);
        u8g2.print(digitalRead(OUT_PIN));
        digitalWrite(REN_PIN, 1);

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
        delay(100);
    }
    debugPrintStatement("Erroring");
    return false;
}

void getDeviceIDs()
{
    deviceIDs[0] = HAL_GetUIDw0();
    deviceIDs[1] = HAL_GetUIDw1();
    deviceIDs[2] = HAL_GetUIDw2();
}

bool handshakeRoutine(uint8_t &position)
{
    bool westMost = false;
    bool eastMost = false;
    uint8_t eastHS = 1;
    uint8_t westHS = 1;
    bool detect = false;
    uint8_t Message[8] = {0};
    getDeviceIDs();

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
    int max_tries = 5;
    for (int i = 0; i < max_tries; i++)
    {
        if (westHS)
        {
            setRow(5);
            westHS = (readCols() & 0b00001000) >> 3;
            delayMicroseconds(5);
        }
        if (eastHS)
        {
            setRow(6);
            eastHS = (readCols() & 0b00001000) >> 3;
            delayMicroseconds(5);
        }
        if (!westHS && !eastHS)
        {
            break;
        }

        debugPrintEastWest(eastHS, westHS);
        delay(100);
    }

    if (westHS && eastHS)
    {
        // Only Module
        moduleID.push_back(deviceID);
        position = 10;
        return false;
    }
    else if (westHS && !eastHS)
    {
        // West-most Module
        westMost = true;

        // Turn East HS signal off
        setRow(5);
        delayMicroseconds(5);
        digitalWrite(OUT_PIN, 0);
        setRow(6);
        delayMicroseconds(5);
        digitalWrite(OUT_PIN, 0);

        digitalWrite(REN_PIN, 1);
        delayMicroseconds(5);
        debugPrintStatement("set low");
        // Set position
        position = 0;

        // broadcast CAN signal
        sendCAN_ModuleInfo(position, deviceIDs[position]);
    }
    else
    {
        if (!westHS && eastHS)
        {
            eastMost = true;
        }
        while (!westHS)
        {
            setRow(5);
            delayMicroseconds(5);
            digitalWrite(REN_PIN, 1);
            delayMicroseconds(5);
            westHS = (readCols() & 0b00001000) >> 3;
        }
        // Recieve CAN signal
        uint32_t ID;
        while (CAN_CheckRXLevel())
            CAN_RX(ID, Message);

        // Increment position
        position = Message[0] + 1;

        delay(100);

        // Turn East HS signal off
        setRow(6);
        delayMicroseconds(3);
        digitalWrite(OUT_PIN, 0);
        digitalWrite(REN_PIN, 1);
        delayMicroseconds(3);

        sendCAN_ModuleInfo(position, deviceIDs[position]);
    }
    if (eastMost)
    {
        sendCAN_HSEnd();
    }
    // Default to return false
    return waitMode(Message);
}