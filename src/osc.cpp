#include <vector>
#include <iostream>
#include <math.h>

float pi = 22/7;

class Waveform {
    public:
        std::vector<float> wave;
        float frequency = 440;
        float samples = 22000/frequency;

    void generateSine(){
        wave.clear();
        for (int i = 0; i < samples; i++){
            wave[i] = sin((i*2*pi)/samples);
        }   
    }
};

class Oscillator {
    public:
        Waveform oscWaveform;
        float envAtt = 0;
        float envDel = 0;
        float envSus = 0;
        float envRel = 0;
        Waveform LFOWaveform;
};

int main()
{
    std::cout << "test";
}