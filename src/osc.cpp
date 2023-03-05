#include <vector>
#include <iostream>
#include <math.h>

double pi = 22/7;

class Waveform {
    public:
        std::vector<double> wave;
        double frequency = 440;
        double samples = 22000/frequency;

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
        double envAtt = 0;
        double envDel = 0;
        double envSus = 0;
        double envRel = 0;
        Waveform LFOWaveform;
};

class Filter {
    public:
        double prevInput = 0;
        double thisInput = 0;
        double prevOutput = 0;
        double thisOutput = 0;
        double R = 1000;
        double C = 0.001;
        double tau = R*C;
        double dt = 1/22000;

        double lowPass(double input){
            thisOutput = (tau*prevOutput + dt*thisInput) / (tau + dt);
            return thisOutput;
        }

        double highPass(double input){
            thisOutput = (1 - (tau/(2*dt) - 1)/(tau/(2*dt) + 1)*(prevOutput + input - prevInput));
            return thisOutput;
        }
};

int main()
{
    std::cout << "test";
}