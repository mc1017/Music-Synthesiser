#ifndef DISPLAY_H
#define DISPLAY_H
#include <Arduino.h>
#include <U8g2lib.h>
#include <Keyboard.h>

// Display driver object
extern U8G2_SSD1305_128X32_NONAME_F_HW_I2C u8g2;

void displayHandshake();

void displayOctave(uint8_t octave, bool drumEnable);

void displayWaveform(uint8_t waveform);

void displayMode(uint8_t mode);

void displayVolume(uint8_t volume);

void displayTXRX(bool transmitter, bool multipleModule, uint8_t position);

void displayKeys(uint8_t keyArray0, uint8_t keyArray1, uint8_t keyArray2);

#endif /* DISPLAY_H */