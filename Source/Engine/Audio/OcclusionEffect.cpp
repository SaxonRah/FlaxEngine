// OcclusionEffect.cpp
#include "OcclusionEffect.h"
#include "Engine/Core/Log.h"

void OcclusionEffect::Process(const float* inputBuffer, float* outputBuffer, uint32 numSamples, const AudioDataInfo& format)
{

    static bool logged = false;
    if (!logged) {
        LOG(Info, "Effect {0} processing {1} samples, wet/dry = {2}",
            GetType().ToString(), numSamples, GetWetDryMix());

        // Log the first few samples for debugging
        String inputSamplesStr;
        String outputSamplesStr;
        for (uint32 i = 0; i < Math::Min(10u, numSamples); i++) {
            inputSamplesStr += String::Format(TEXT("{0:.3f} "), inputBuffer[i]);
        }

        // Process a few samples
        const float wetMix = GetWetDryMix();
        const float dryMix = 1.0f - wetMix;
        for (uint32 i = 0; i < Math::Min(10u, numSamples); i++) {
            // Example processing (modify based on your actual effect)
            outputBuffer[i] = inputBuffer[i] * dryMix + (inputBuffer[i] * 0.5f) * wetMix;
            outputSamplesStr += String::Format(TEXT("{0:.3f} "), outputBuffer[i]);
        }

        LOG(Info, "Input samples: {0}", inputSamplesStr);
        LOG(Info, "Output samples: {0}", outputSamplesStr);

        logged = true;
    }

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
