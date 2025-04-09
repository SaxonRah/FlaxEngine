// AcousticMaterialEffect.cpp
#include "AcousticMaterialEffect.h"

void AcousticMaterialEffect::UpdateCoefficients()
{
    // Calculate reflection coefficient (1 - absorption)
    _reflection = 1.0f - _absorption;
    
    // Set frequency-dependent coefficients
    _lowCoeff = 1.0f - _lowFreqAbsorption;
    _midCoeff = 1.0f - _midFreqAbsorption;
    _highCoeff = 1.0f - _highFreqAbsorption;
}

void AcousticMaterialEffect::Process(const float* inputBuffer, float* outputBuffer, uint32 numSamples, const AudioDataInfo& format)
{
    const float wetMix = GetWetDryMix();
    const float dryMix = 1.0f - wetMix;
    
    // Apply frequency-dependent absorption
    for (uint32 i = 0; i < numSamples; i++)
    {
        const float inputSample = inputBuffer[i];
        
        // Simple 3-band filter
        // Low frequencies
        _lowPass = _lowPass + (1.0f - _lowPass) * 0.1f;
        const float lowOutput = _lowPass * inputSample * _lowCoeff;
        
        // Mid frequencies
        _bandPass = (_bandPass + inputSample - _lowPass) * 0.5f;
        const float midOutput = _bandPass * _midCoeff;
        
        // High frequencies
        _highPass = inputSample - _lowPass - _bandPass;
        const float highOutput = _highPass * _highCoeff;
        
        // Combine bands
        const float processedSample = lowOutput + midOutput + highOutput;
        
        // Simulate scattering with a bit of noise
        float noise = Math::Sin(i * 0.1f) * 0.01f; // Simple deterministic noise
        
        // Mix the filtered signal with noise based on scattering coefficient
        const float scatteredSample = processedSample * (1.0f - _scattering) + noise * _scattering;
        
        // Mix dry and wet signals
        outputBuffer[i] = inputSample * dryMix + scatteredSample * wetMix * _reflection;
    }
}