// SpatializerEffect.cpp
#include "SpatializerEffect.h"

void SpatializerEffect::UpdateEarParameters()
{
    // Calculate distance between listener and source
    const float distance = Vector3::Distance(_listenerPosition, _sourcePosition);
    
    // Inverse-square law distance attenuation
    _distanceAttenuation = _sourceRadius / Math::Max(_sourceRadius, distance);
    
    // Calculate direction from listener to source
    Vector3 listenerToSource = _sourcePosition - _listenerPosition;
    listenerToSource.Normalize();
    
    // Transform to listener's local space
    const Transform listenerTransform(_listenerPosition, _listenerOrientation);
    const Vector3 localDirection = listenerTransform.WorldToLocal(listenerToSource);
    
    // Approximate head width (in engine units)
    const float headWidth = 0.2f * 100.0f; // 20cm in Flax units
    
    // Calculate ear positions relative to head center
    const Vector3 leftEarOffset(-headWidth / 2.0f, 0.0f, 0.0f);
    const Vector3 rightEarOffset(headWidth / 2.0f, 0.0f, 0.0f);
    
    // Calculate distance and delay for each ear
    const float leftDist = Vector3::Distance(leftEarOffset, localDirection * headWidth);
    const float rightDist = Vector3::Distance(rightEarOffset, localDirection * headWidth);
    
    // Convert distance to delay time (assuming 343 m/s speed of sound)
    const float speedOfSound = 343.0f * 100.0f; // m/s to Flax units
    _leftEar.Delay = leftDist / speedOfSound;
    _rightEar.Delay = rightDist / speedOfSound;
    
    // Calculate gain for each ear based on direction
    _leftEar.Gain = Math::Lerp(0.2f, 1.0f, Math::Saturate(-localDirection.X + 0.5f));
    _rightEar.Gain = Math::Lerp(0.2f, 1.0f, Math::Saturate(localDirection.X + 0.5f));
    
    // Scale gains by distance attenuation
    _leftEar.Gain *= _distanceAttenuation;
    _rightEar.Gain *= _distanceAttenuation;
    
    // Calculate delay line sizes (assuming 44.1kHz sample rate)
    // We'll adjust for actual sample rate during processing
    const int32 maxDelaySamples = (int32)(0.01f * 44100.0f); // 10ms max delay
    
    if (_leftEar.DelayLine.Count() != maxDelaySamples)
    {
        _leftEar.DelayLine.Resize(maxDelaySamples);
        _rightEar.DelayLine.Resize(maxDelaySamples);
        Platform::MemoryClear(_leftEar.DelayLine.Get(), maxDelaySamples * sizeof(float));
        Platform::MemoryClear(_rightEar.DelayLine.Get(), maxDelaySamples * sizeof(float));

        _leftEar.DelayLinePos = 0;
        _rightEar.DelayLinePos = 0;
    }
}

void SpatializerEffect::ProcessChannel(const float* input, float* output, uint32 numSamples, uint32 sampleRate, HRTFFilter& filter)
{
    const int32 delayLineSize = filter.DelayLine.Count();
    const int32 delaySamples = Math::Min((int32)(filter.Delay * sampleRate), delayLineSize - 1);
    
    for (uint32 i = 0; i < numSamples; i++)
    {
        // Write input to delay line
        filter.DelayLine[filter.DelayLinePos] = input[i];
        
        // Get delayed sample
        const int32 readPos = (filter.DelayLinePos - delaySamples + delayLineSize) % delayLineSize;
        const float delayedSample = filter.DelayLine[readPos];
        
        // Apply gain and write to output
        output[i] = delayedSample * filter.Gain;
        
        // Advance delay line position
        filter.DelayLinePos = (filter.DelayLinePos + 1) % delayLineSize;
    }
}

void SpatializerEffect::Process(const float* inputBuffer, float* outputBuffer, uint32 numSamples, const AudioDataInfo& format)
{
    const float wetMix = GetWetDryMix();
    const float dryMix = 1.0f - wetMix;
    
    // We need to know if we're processing mono or stereo
    const uint32 channels = format.NumChannels;
    
    // Clear output buffer
    Platform::MemoryClear(outputBuffer, numSamples * channels * sizeof(float));
    
    // For mono input to stereo output (most common case)
    if (channels == 2)
    {
        // Ensure temp buffer is large enough
        if (_tempBuffer.Count() < (int32)numSamples)
            _tempBuffer.Resize(numSamples);
        
        float* leftChannel = _tempBuffer.Get();
        float* rightChannel = _tempBuffer.Get() + numSamples / 2;
        
        // Extract mono input if needed
        const float* monoInput = inputBuffer;
        if (channels == 2)
        {
            // Downmix stereo to mono
            for (uint32 i = 0; i < numSamples / 2; i++)
            {
                _tempBuffer[i] = (inputBuffer[i * 2] + inputBuffer[i * 2 + 1]) * 0.5f;
            }
            monoInput = _tempBuffer.Get();
        }
        
        // Process HRTF for left and right ears
        ProcessChannel(monoInput, leftChannel, numSamples / 2, format.SampleRate, _leftEar);
        ProcessChannel(monoInput, rightChannel, numSamples / 2, format.SampleRate, _rightEar);
        
        // Interleave back to stereo output with wet/dry mix
        for (uint32 i = 0; i < numSamples / 2; i++)
        {
            // Mix dry signal
            if (channels == 2)
            {
                // Original stereo
                outputBuffer[i * 2] = inputBuffer[i * 2] * dryMix + leftChannel[i] * wetMix;
                outputBuffer[i * 2 + 1] = inputBuffer[i * 2 + 1] * dryMix + rightChannel[i] * wetMix;
            }
            else
            {
                // Original mono
                outputBuffer[i * 2] = monoInput[i] * dryMix + leftChannel[i] * wetMix;
                outputBuffer[i * 2 + 1] = monoInput[i] * dryMix + rightChannel[i] * wetMix;
            }
        }
    }
    else
    {
        // Just apply simple panning for non-stereo formats
        for (uint32 i = 0; i < numSamples; i++)
        {
            outputBuffer[i] = inputBuffer[i] * dryMix + inputBuffer[i] * _distanceAttenuation * wetMix;
        }
    }
}
