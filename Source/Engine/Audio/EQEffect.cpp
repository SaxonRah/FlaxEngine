// EQEffect.cpp
#include "EQEffect.h"
#include "Engine/Core/Log.h"

void EQEffect::Process(const float* inputBuffer, float* outputBuffer, uint32 numSamples, const AudioDataInfo& format)
{
    static bool logged = false;
    if (!logged) {
        LOG(Info, "Processing EQEffect with {0} samples", numSamples);
        logged = true;
    }
    const float wetMix = GetWetDryMix();
    const float dryMix = 1.0f - wetMix;
    
    // Calculate filter coefficients based on sample rate
    const float sampleRate = (float)format.SampleRate;
    const float lowCoeff = Math::Exp(-2.0f * PI * _lowFreq / sampleRate);
    const float highCoeff = Math::Exp(-2.0f * PI * _highFreq / sampleRate);
    
    for (uint32 i = 0; i < numSamples; i++)
    {
        const float inputSample = inputBuffer[i];
        
        // Low-pass filter (for low frequencies)
        _lowPass1 = _lowPass1 + (inputSample - _lowPass1) * (1.0f - lowCoeff);
        _lowPass2 = _lowPass2 + (_lowPass1 - _lowPass2) * (1.0f - lowCoeff);
        
        // High-pass filter (for high frequencies)
        _highPass1 = inputSample - _lowPass1;
        _highPass2 = _highPass1 - _bandPass1;
        
        // Band-pass filter (for mid frequencies)
        _bandPass1 = _lowPass1 - _lowPass2;
        _bandPass2 = _highPass1 - _highPass2;
        
        // Apply gains to each band
        const float lowOut = _lowPass2 * _lowGain;
        const float midOut = _bandPass2 * _midGain;
        const float highOut = _highPass2 * _highGain;
        
        // Mix wet (EQ) and dry signals
        const float wetOut = lowOut + midOut + highOut;
        outputBuffer[i] = inputSample * dryMix + wetOut * wetMix;
    }
}
