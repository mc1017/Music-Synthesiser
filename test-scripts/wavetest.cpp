#include <cmath>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>

const std::uint32_t MAX_UINT32 = 4294967295;
const std::uint32_t stepSizes [] = {25538028, 27056599, 28665468, 30370005, 32175899, 34089178, 36116226, 38263809, 40539093, 42949673, 45503593, 48209378, 51076057, 54113197, 57330935, 60740010, 64351799, 68178356, 72232452, 76527617, 81078186, 85899346, 91007187, 96418756, 102152113, 108226394, 114661870, 121480020, 128703598, 136356712, 144464904, 153055234, 162156372, 171798692, 182014374, 192837512};
volatile std::uint32_t currentStepSize;
volatile uint32_t currentVolLFOStepSize;
volatile uint32_t postLFOStepSize;
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
  double sine[] = {0, 22, 44, 64, 82, 98, 111, 120, 126, 128, 126, 120, 111, 98, 82, 64, 44, 22, 0, -22, -44, -64, -82, -98, -111, -120, -126, -128, -126, -120, -111, -98, -82, -64, -44, -22};
  phaseAcc += currentStepSize;
  int32_t Vout = (phaseAcc >> 24) ;
  WaveFile << sine[int(Vout/5.33)]+128 << std::endl;
}

void sampleISR_LFO()
{
    static uint32_t phaseAcc = 0;
    
    static uint32_t VolLFOphaseAcc = 0;

    currentVolLFOStepSize = 780903;
    VolLFOphaseAcc += currentVolLFOStepSize; 

    postLFOStepSize = currentStepSize;

    float LFOvolamt = 1; //placeholder volume automation
    float LFOpitchamt = 0.0833333333; //placeholder pitch automation
    float VoutModifier = 0;
    float stepModifier = 0;
    
    if ((VolLFOphaseAcc >> 24) < 128){
         VoutModifier = 1 - LFOvolamt * 1.9 * (static_cast<float>(VolLFOphaseAcc)/static_cast<float>(MAX_UINT32));
         stepModifier = 1 - LFOpitchamt * 1.9 * (static_cast<float>(VolLFOphaseAcc)/static_cast<float>(MAX_UINT32));
     }
     else{
         VoutModifier = 1 - LFOvolamt * 1.9 * (1 - static_cast<float>(VolLFOphaseAcc)/static_cast<float>(MAX_UINT32));
         stepModifier = 1 - LFOpitchamt * 1.9 * (1 - static_cast<float>(VolLFOphaseAcc)/static_cast<float>(MAX_UINT32));
     }
    
    postLFOStepSize = static_cast<int>(stepModifier * static_cast<float>(currentStepSize));
    phaseAcc += postLFOStepSize;

    int32_t Vout = (phaseAcc >> 24) - 128;

    Vout = static_cast<int>(static_cast<float>(Vout)*VoutModifier);
    //Vout = Vout >> (8 - knob3rotation);

    WaveFile << Vout;

    //WaveFile << "VolLFOphaseAcc    " << VolLFOphaseAcc << "    postLFOStepSize    " << postLFOStepSize << "    currentStepSize    " << currentStepSize << "    stepModifier    " << stepModifier <<  std::endl;
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
        std::cout<<"ISR";
        for (int i = 0; i < 1000; i++)
        {
            sampleISR();
        }
    }

    else if (type == "triangle")

    {
        std::cout<<"Tri";
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
        std::cout<<"Sine";
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