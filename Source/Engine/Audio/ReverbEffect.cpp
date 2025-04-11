// ReverbEffect.cpp
#include "ReverbEffect.h"
#include "Engine/Core/Log.h"

void ReverbEffect::OnParamsChanged()
{
    // Recalculate reverb parameters based on room size and decay
    const uint32 delaySize = (uint32)(44100 * _roomSize * 0.1f); // Assuming 44.1kHz sample rate
    
    if (_delayLine.Count() != delaySize)
    {
        _delayLine.Resize(Math::Max(1u, delaySize));
        Platform::MemoryClear(_delayLine.Get(), _delayLine.Count() * sizeof(float));    
        _delayLinePos = 0;
    }
    
    _feedback = 0.2f + _decay * 0.6f;
    _damping = 0.1f + (1.0f - _decay) * 0.2f;
}

void ReverbEffect::Process(const float* inputBuffer, float* outputBuffer, uint32 numSamples, const AudioDataInfo& format)
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
    
    if (_delayLine.IsEmpty())
        OnParamsChanged();
    
    const uint32 delaySize = _delayLine.Count();
    
    for (uint32 i = 0; i < numSamples; i++)
    {
        const float inputSample = inputBuffer[i];
        
        // Get sample from delay line
        const float delaySample = _delayLine[_delayLinePos];
        
        // Apply feedback and damping
        float newSample = inputSample + delaySample * _feedback;
        newSample = newSample * (1.0f - _damping) + delaySample * _damping;
        
        // Write back to delay line
        _delayLine[_delayLinePos] = Math::Clamp(newSample, -1.0f, 1.0f);
        
        // Move delay line position
        _delayLinePos = (_delayLinePos + 1) % delaySize;
        
        // Mix wet and dry signals
        outputBuffer[i] = inputSample * dryMix + delaySample * wetMix;
    }
}
