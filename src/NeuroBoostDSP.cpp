// DSP-only implementation of NeuroBoost
// This file provides the core audio processing functionality

#include "NeuroBoostDSP.h"
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <cstring>

// Test program for DSP functionality
#ifndef NEUROBOOST_DSP_LIB_ONLY

int main(int argc, char* argv[])
{
    std::cout << "NeuroBoost DSP Test" << std::endl;
    std::cout << "===================" << std::endl;
    
    // Parse command line arguments
    double gainPercent = 100.0; // Default: unity gain
    
    if (argc > 1) {
        gainPercent = std::atof(argv[1]);
        if (gainPercent < 0.0 || gainPercent > 200.0) {
            std::cerr << "Error: Gain must be between 0 and 200 percent" << std::endl;
            return 1;
        }
    }
    
    std::cout << "Gain setting: " << gainPercent << "%" << std::endl;
    
    // Create DSP processor
    NeuroBoostDSP dsp;
    dsp.SetSampleRate(44100.0);
    dsp.SetGain(gainPercent);
    
    // Create test signal (sine wave)
    const int numFrames = 256;
    const int numChannels = 2;
    const double frequency = 440.0; // A4
    const double sampleRate = 44100.0;
    
    // Allocate buffers
    sample inputBuffer[numChannels][numFrames];
    sample outputBuffer[numChannels][numFrames];
    sample* inputs[numChannels] = { inputBuffer[0], inputBuffer[1] };
    sample* outputs[numChannels] = { outputBuffer[0], outputBuffer[1] };
    
    // Generate test signal
    for (int s = 0; s < numFrames; s++) {
        double t = static_cast<double>(s) / sampleRate;
        sample value = static_cast<sample>(0.5 * std::sin(2.0 * M_PI * frequency * t));
        inputBuffer[0][s] = value;
        inputBuffer[1][s] = value;
    }
    
    // Copy input to output (for comparison)
    std::memcpy(outputBuffer, inputBuffer, sizeof(inputBuffer));
    
    // Process audio
    dsp.ProcessBlock(inputs, outputs, numFrames, numChannels);
    
    // Calculate RMS of input and output
    double inputRMS = 0.0;
    double outputRMS = 0.0;
    
    for (int s = 0; s < numFrames; s++) {
        inputRMS += inputBuffer[0][s] * inputBuffer[0][s];
        outputRMS += outputBuffer[0][s] * outputBuffer[0][s];
    }
    
    inputRMS = std::sqrt(inputRMS / numFrames);
    outputRMS = std::sqrt(outputRMS / numFrames);
    
    std::cout << std::endl;
    std::cout << "Test Results:" << std::endl;
    std::cout << "  Input RMS:  " << inputRMS << std::endl;
    std::cout << "  Output RMS: " << outputRMS << std::endl;
    std::cout << "  Expected ratio: " << (gainPercent / 100.0) << std::endl;
    std::cout << "  Actual ratio:   " << (inputRMS > 0 ? outputRMS / inputRMS : 0) << std::endl;
    
    // Verify gain is applied correctly
    double expectedRatio = gainPercent / 100.0;
    double actualRatio = inputRMS > 0 ? outputRMS / inputRMS : 0;
    double tolerance = 0.001;
    
    if (std::abs(actualRatio - expectedRatio) < tolerance) {
        std::cout << std::endl;
        std::cout << "SUCCESS: Gain processing is correct!" << std::endl;
        return 0;
    } else {
        std::cout << std::endl;
        std::cout << "FAILURE: Gain processing mismatch!" << std::endl;
        return 1;
    }
}

#endif // NEUROBOOST_DSP_LIB_ONLY
