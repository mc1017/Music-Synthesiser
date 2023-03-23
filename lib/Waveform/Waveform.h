#ifndef WAVEFORM_H
#define WAVEFORM_H
#include <Arduino.h>
#include <U8g2lib.h>
#include <Keyboard.h>
#include <Waveform.h>
#include <cmath>

extern uint8_t octave;
extern volatile uint32_t currentStepSize;

void sawTooth(int32_t &Vout, uint8_t &activeNotes, int index);

void triangle(int32_t &Vout, uint8_t &activeNotes, int index);

void square(int32_t &Vout, uint8_t &activeNotes, int index);

void sine(int32_t &Vout, uint8_t &activeNotes, int index);

void LFO();

void waveforms(int32_t &Vout, uint8_t &activeNotes, int index);

#endif /* WAVEFORM_H */