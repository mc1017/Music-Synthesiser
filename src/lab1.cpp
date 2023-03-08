#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>

void calculate(){
    double intlim = pow(2,32);
    std::vector<double> eqtemperament(36);
    eqtemperament[21] = 440;
    double twelfthrootoftwo = pow(2, 1.0/12.0);
    std::vector<std::string> notenames{"C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3", "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4", "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5"};
    std::vector<int> accsteps(36);

    std::ofstream CalcFile("calcs.txt");

    for (int i = 20; i >= 0; i -= 1)
    {
        eqtemperament[i] = eqtemperament[i+1]/twelfthrootoftwo;
    }
    for (int i = 22; i < 36; i += 1)
    {
        eqtemperament[i] = eqtemperament[i-1]*twelfthrootoftwo;
    }
    for (int i = 0; i < 36; i++)
    {
        accsteps[i] = int(intlim/eqtemperament[i] + 0.5);
    }

    for (int i = 0; i < 36; i++)
    {
        CalcFile << notenames[i] << std::endl << eqtemperament [i] << std::endl << accsteps[i] << std::endl;
    }

    CalcFile.std::ofstream::close();
}


int main(){
    calculate();
}