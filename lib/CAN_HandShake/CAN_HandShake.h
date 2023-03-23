#ifndef CAN_HANDSHAKE_H
#define CAN_HANDSHAKE_H
#include <Arduino.h>
#include <U8g2lib.h>
#include <Keyboard.h>
#include <ES_CAN.h>
#include <vector>
#include <Display.h>

extern std::vector<uint32_t> moduleID;

// Handshake out signals (default to high on startup)
extern volatile uint8_t OutPin[7];

// Transmitter / Reciever setting
extern bool transmitter;
extern bool multipleModule;

// Receiver / Transmitter Message
extern uint8_t RX_Message[8];
extern uint8_t RX_Message2[8];
extern uint8_t CAN_Tx[8];

// Global variable to hold the device's position
extern uint8_t position;
extern uint32_t deviceID;
extern const uint32_t ID_MODULE_INFO;

void sendCAN_HSEnd();

bool handshakeRoutine(uint8_t &position);

#endif /* CAN_HANDSHAKE_H */