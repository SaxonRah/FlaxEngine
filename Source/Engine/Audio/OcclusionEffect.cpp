// OcclusionEffect.cpp
#include "OcclusionEffect.h"

void OcclusionEffect::Process(const float* inputBuffer, float* outputBuffer, uint32 numSamples, const AudioDataInfo& format)
{
    const float wetMix = GetWetDryMix();
    const float dryMix = 1.0f - wetMix;
    
    // Simple low-pass filter to simulate occlusion
    for (uint32 i = 0; i < numSamples; i++)
    {
        const float inputSample = inputBuffer[i];
        
        // Apply low-pass filter based on occlusion level
        _lastSample = _lastSample + (_lowPassCoeff * (inputSample - _lastSample));
        
        // Apply volume reduction
        const float occludedSample = _lastSample * _volumeReduction;
        
        // Mix dry and wet signals
        outputBuffer[i] = inputSample * dryMix + occludedSample * wetMix;
    }
}