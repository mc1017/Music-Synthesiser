#ifndef DRUM_H
#define DRUM_H
#include <Arduino.h>
#include <U8g2lib.h>
#include <Keyboard.h>
#include <Waveform.h>
#include <cmath>

extern uint8_t snare[];

extern uint8_t CH[];

extern uint8_t kick[];


void drum_play(int32_t &Vout, uint8_t &activeNotes, int index);

void drum(int32_t &Vout, uint8_t &activeNotes, int index);

#endif /*DRUM_H */