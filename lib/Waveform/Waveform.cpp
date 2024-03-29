#include <Arduino.h>
#include <Keyboard.h>
#include <Waveform.h>
#include <cmath>

uint8_t octave;
volatile uint32_t currentStepSize;
static volatile uint32_t currentVolLFOStepSize;
static volatile uint32_t postLFOStepSize;
static const std::uint32_t MAX_UINT32 = 4294967295;
volatile uint32_t phaseAcc[12] = {0};

const uint32_t stepSizes3[] = {
    25538028, 27056598, 28665467, 30370005, 32175899, 34089178, 36116226, 38263808, 40539093, 42949673, 45503593, 48209378 // G# / Ab (415.30 Hz)
};

const uint32_t stepSizes0[] = {
    51076057, 54113197, 57330935, 60740010, 64351799, 68178356, 72232452, 76527617, 81078186, 85899346, 91007187, 96418756 // G# / Ab (830.61 Hz)
};
const uint32_t stepSizes1[] = {
    102152114, 108226394, 114661870, 121480020, 128703598, 136356712, 144464904, 153055234, 162156372, 171798692, 182014374, 192837512 // G# / Ab (1661.22 Hz)
};
const uint32_t stepSizes2[] = {
    204304228, 216452788, 229323740, 242960040, 257407196, 272713424, 288929808, 306110468, 324312744, 343597384, 364028748, 385675024 // G# / Ab (3322.44 Hz)
};

const uint32_t stepSizes4[] = {
    408608456, 432905576, 458647480, 485920080, 514814392, 545426848, 577859616, 612220936, 648625488, 687194768, 728057496, 771350048 // G# / Ab (6644.88 Hz)
};
const uint32_t *const stepSizeList[] = { stepSizes3, stepSizes0, stepSizes1, stepSizes2, stepSizes4,};



void sawTooth(int32_t &Vout, uint8_t &activeNotes, int index)
{
    const uint32_t *stepSizes = stepSizeList[octave];
    phaseAcc[index] += stepSizes[index];
    int32_t noteVout = (phaseAcc[index] >> 24) - 128;
    noteVout = noteVout >> (8 - knob[3].getRotation());
    Vout += noteVout;
    ++activeNotes;
}

void triangle(int32_t &Vout, uint8_t &activeNotes, int index)
{
    const uint32_t *stepSizes = stepSizeList[octave];
    phaseAcc[index] += stepSizes[index];
    int32_t noteVout = 0;
    if ((phaseAcc[index] >> 24) < 128)
    {
        noteVout = (phaseAcc[index] >> 24) - 128;
    }
    else
    {
        noteVout = 255 - (phaseAcc[index] >> 24);
    }
    noteVout = noteVout >> (8 - knob[3].getRotation());
    Vout += noteVout;
    ++activeNotes;
}

void square(int32_t &Vout, uint8_t &activeNotes, int index)
{
    const uint32_t *stepSizes = stepSizeList[octave];
    phaseAcc[index] += stepSizes[index];
    int32_t noteVout = 0;
    if ((phaseAcc[index] >> 24) < 128)
    {
        noteVout = 0 - 128;
    }
    else
    {
        noteVout = 255 - 128;
    }
    noteVout = noteVout >> (8 - knob[3].getRotation());
    Vout += noteVout;
    ++activeNotes;
}

// Sine Waveform
void sine(int32_t &Vout, uint8_t &activeNotes, int index)
{
    const uint32_t *stepSizes = stepSizeList[octave];
    phaseAcc[index] += stepSizes[index];
    int32_t noteVout = 0;
    float angle = 2.0 * 3.1416 * (static_cast<float>(phaseAcc[index]) / static_cast<float>(MAX_UINT32));
    noteVout = 150 * sin(angle);
    float knobPosition = static_cast<float>(knob[3].getRotation()) / 8.0;
    float attenuationFactor = pow(knobPosition, 2);
    noteVout = static_cast<int32_t>(static_cast<float>(noteVout) * attenuationFactor);



    Vout += noteVout;
    ++activeNotes;
}

void LFO(int32_t &Vout, uint8_t &activeNotes, int index)
{
    static uint32_t VolLFOphaseAcc = 0;
    const uint32_t *stepSizes = stepSizeList[octave];
    

    currentVolLFOStepSize = 780903;
    VolLFOphaseAcc += currentVolLFOStepSize;

    postLFOStepSize = stepSizes[index];

    float LFOvolamt = 1;              // placeholder volume automation
    float LFOpitchamt = 0.0833333333; // placeholder pitch automation
    float VoutModifier = 0;
    float stepModifier = 0;

    if ((VolLFOphaseAcc >> 24) < 128)
    {
        VoutModifier = 1 - LFOvolamt * 1.9 * (static_cast<float>(VolLFOphaseAcc) / static_cast<float>(MAX_UINT32));
        stepModifier = 1 - LFOpitchamt * 1.9 * (static_cast<float>(VolLFOphaseAcc) / static_cast<float>(MAX_UINT32));
    }
    else
    {
        VoutModifier = 1 - LFOvolamt * 1.9 * (1 - static_cast<float>(VolLFOphaseAcc) / static_cast<float>(MAX_UINT32));
        stepModifier = 1 - LFOpitchamt * 1.9 * (1 - static_cast<float>(VolLFOphaseAcc) / static_cast<float>(MAX_UINT32));
    }

    postLFOStepSize = static_cast<int>(stepModifier * static_cast<float>(currentStepSize));
    phaseAcc[index] += postLFOStepSize;

    Vout = (phaseAcc[index] >> 24) - 128;

    Vout =(Vout * VoutModifier);
}


void waveforms(int32_t &Vout, uint8_t &activeNotes, int index)
{
    
    switch (knob[0].getRotation())
    {
    case 0:
        sawTooth(Vout, activeNotes, index);
        break;
    case 1:
        triangle(Vout, activeNotes, index);
        break;
    case 2:
        square(Vout, activeNotes, index);
        break;
    case 3:
        sine(Vout, activeNotes, index);
        break;
    case 4:
        LFO(Vout, activeNotes, index);
        break;
    
    default:
        sawTooth(Vout, activeNotes, index);
        break;
    }
    
    
}