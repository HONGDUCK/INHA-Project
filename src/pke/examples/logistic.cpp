#define PROFILE

#include <iostream>
#include <string>
#include<math.h>
#include <fstream>
#include <vector>
#include "openfhe.h"
using namespace std;
using namespace lbcrypto;

void InputData();

int main(){

    InputData();

    return 0;
}

void InputData(){
    ifstream file;
    file.open("Iris.csv"); // input file name
    string line;
    vector<vector<double>> inputVector;
    vector<double> outputVector;


    while (getline(file, line)){
        int i = 0;
        while (line[i] != ','){
            i++;
        }
        i++; // skip ID and comma

        vector<double> inputs;
        inputs.clear();

        for(int token=0; token<4; token++){ // number : 4 is rows of dataset
            double value;
            string str_value = "";
            while (line[i] != ','){
                str_value += line[i];
                i++;
            }
            i++; // skip comma
            value = stod(str_value);
            inputs.push_back(value);
        }
        i++; // skip comma

        string outputValue(1, line[line.size()-1]);
        inputVector.push_back(inputs);
        outputVector.push_back(stod(outputValue));
    }

    /* Print */
    cout << "Print Input Values\n";
    for(size_t i=0; i<inputVector.size(); i++){
        cout << "index : " << i << " : ";
        for(size_t j=0; j<inputVector[i].size(); j++){
            cout << inputVector[i][j] << ", ";
        }
        cout << "output value : " << outputVector[i] << "\n";
    }
}