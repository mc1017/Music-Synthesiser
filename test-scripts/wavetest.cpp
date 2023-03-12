#include <cmath>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>

const std::uint32_t MAX_UINT32 = 4294967295;
const std::uint32_t stepSizes [] = {25538028, 27056599, 28665468, 30370005, 32175899, 34089178, 36116226, 38263809, 40539093, 42949673, 45503593, 48209378, 51076057, 54113197, 57330935, 60740010, 64351799, 68178356, 72232452, 76527617, 81078186, 85899346, 91007187, 96418756, 102152113, 108226394, 114661870, 121480020, 128703598, 136356712, 144464904, 153055234, 162156372, 171798692, 182014374, 192837512};
volatile std::uint32_t currentStepSize;
int Vout = 0;

std::ofstream WaveFile("waves.txt");

void sampleISR() {
  static uint32_t phaseAcc = 0;
  phaseAcc += currentStepSize;
  int32_t Vout = (phaseAcc >> 24) - 128;
  WaveFile << ( Vout + 128) << std::endl;
}

void sampleISR_triangle(){
  static uint32_t phaseAcc = 0;
  phaseAcc += currentStepSize;
  int32_t Vout = 0;
  if ((phaseAcc >> 24) < 128){
    Vout = (phaseAcc >> 24) - 128;
  }
  else{
    Vout = 255 - (phaseAcc >> 24);
  }
  WaveFile << ( Vout + 128) << std::endl;
}

void sampleISR_square(){
  static uint32_t phaseAcc = 0;
  phaseAcc += currentStepSize;
  int32_t Vout = 0;
  if ((phaseAcc >> 24) < 128){
    Vout = 0 - 128;
  }
  else{
    Vout = 255 - 128;
  }
  WaveFile << ( Vout + 128) << std::endl;
}

void sampleISR_sine(){
  static uint32_t phaseAcc = 0;
  phaseAcc += currentStepSize;
  double radians = phaseAcc/MAX_UINT32;
  double sineScaled = std::sin(radians)*2147483646 + 0.5;
  int32_t sineScaledFixed = static_cast<int32_t>(sineScaled);

  int32_t Vout = (sineScaledFixed >> 24) - 128;
  WaveFile << ( Vout + 128) << std::endl;
}

void sampleISR_LFO()
{
    static uint32_t phaseAcc = 0;
    
    static uint32_t LFOphaseAcc = 0;
    LFOphaseAcc += 780903; //LFO of 4Hz, placeholder lfo speed

    uint32_t LFOpitchamt = 255392044; //1 semitone, placeholder lfo pitch value, where 1 octave is represented by MAX_UINT32
    static uint32_t postLFOStepSize = currentStepSize;

    //uint32_t LFOvolamt = 0;
  
    postLFOStepSize = currentStepSize + LFOphaseAcc;

    /*
    if ((LFOphaseAcc >> 24) < 128){
        postLFOStepSize = currentStepSize * (1+LFOpitchamt/MAX_UINT32) * (LFOphaseAcc/MAX_UINT32-(1/4));
    }
    else{
        postLFOStepSize = currentStepSize * (1+LFOpitchamt/MAX_UINT32) * ((1/2)-LFOphaseAcc/MAX_UINT32);
    }
    */

    phaseAcc += postLFOStepSize;

    int32_t Vout = (phaseAcc >> 24) - 128;

    WaveFile << "LFOphaseAcc    " << LFOphaseAcc << "    postLFOStepSize    " << postLFOStepSize << "    currentStepSize    " << currentStepSize << std::endl;
}

int main(){
    int input;
    std::string type;
    std::cout << "specify step size (0-35)" << std::endl;
    std::cin >> input;
    std::cout << "specify waveform (saw, triangle, square, sine)" << std::endl;
    std::cin >> type;
    currentStepSize = stepSizes[input];

    if (type == "saw")
    {
        for (int i = 0; i < 1000; i++)
        {
            sampleISR();
        }
    }
    else if (type == "triangle")
    {
        for (int i = 0; i < 1000; i++)
        {
            sampleISR_triangle();
        }
    }
    else if (type == "square")
    {
        for (int i = 0; i < 1000; i++)
        {
            sampleISR_square();
        }
    }
    else if (type == "sine")
    {
        for (int i = 0; i < 1000; i++)
        {
            sampleISR_sine();
        }
    }
    else if (type == "LFO")
    {
        for (int i = 0; i < 1000; i++)
        {
            sampleISR_LFO();
        }
    }
    else
    {
        std::cout << "invalid input";
    }
    WaveFile.std::ofstream::close();
}