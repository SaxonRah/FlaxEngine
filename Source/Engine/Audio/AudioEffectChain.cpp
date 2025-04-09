// AudioEffectChain.cpp
#include "AudioEffectChain.h"

void AudioEffectChain::AddEffect(AudioEffect* effect)
{
    if (effect == nullptr || _effects.Contains(effect))
        return;
    
    _effects.Add(effect);
    MarkDirty();
}

void AudioEffectChain::RemoveEffect(AudioEffect* effect)
{
    if (effect == nullptr)
        return;
    
    if (_effects.Remove(effect))
        MarkDirty();
}

void AudioEffectChain::MoveEffect(AudioEffect* effect, int32 newIndex)
{
    if (effect == nullptr)
        return;
    
    const int32 oldIndex = _effects.Find(effect);
    if (oldIndex != -1 && oldIndex != newIndex)
    {
        _effects.RemoveAt(oldIndex);
        _effects.Insert(Math::Clamp(newIndex, 0, _effects.Count()), effect);
        MarkDirty();
    }
}

void AudioEffectChain::Clear()
{
    if (_effects.IsEmpty())
        return;
    
    _effects.Clear();
    MarkDirty();
}

void AudioEffectChain::Process(const float* inputBuffer, float* outputBuffer, uint32 numSamples, const AudioDataInfo& format)
{
    if (_effects.IsEmpty() || numSamples == 0)
    {
        // If no effects, just copy input to output
        if (inputBuffer != outputBuffer)
            Platform::MemoryCopy(outputBuffer, inputBuffer, numSamples * sizeof(float));
        return;
    }

    // Ensure processing buffer is large enough
    const uint32 bufferSize = numSamples * sizeof(float);
    if (_processingBuffer.Count() < (int32)numSamples)
        _processingBuffer.Resize(numSamples);
    
    // Copy input to temp buffer
    Platform::MemoryCopy(_processingBuffer.Get(), inputBuffer, bufferSize);
    
    // Chain processing
    float* sourceBuffer = _processingBuffer.Get();
    float* targetBuffer = outputBuffer;
    
    // If only one effect, process directly to output
    if (_effects.Count() == 1 && _effects[0]->GetIsEnabled())
    {
        _effects[0]->Process(sourceBuffer, targetBuffer, numSamples, format);
        return;
    }
    
    // Process multiple effects
    bool outputIsTarget = true;
    for (int32 i = 0; i < _effects.Count(); i++)
    {
        if (!_effects[i]->GetIsEnabled())
            continue;
        
        // On last effect, make sure to output to the target buffer
        if (i == _effects.Count() - 1 && !outputIsTarget)
        {
            _effects[i]->Process(sourceBuffer, targetBuffer, numSamples, format);
        }
        // Toggle between source and temp buffer
        else if (outputIsTarget)
        {
            // Output back to source buffer
            _effects[i]->Process(sourceBuffer, _processingBuffer.Get(), numSamples, format);
            sourceBuffer = _processingBuffer.Get();
            outputIsTarget = false;
        }
        else
        {
            // Output to target buffer
            _effects[i]->Process(sourceBuffer, targetBuffer, numSamples, format);
            sourceBuffer = targetBuffer;
            outputIsTarget = true;
        }
    }
    
    // If we ended up with results in the source buffer, copy to output
    if (!outputIsTarget)
        Platform::MemoryCopy(targetBuffer, sourceBuffer, bufferSize);
}
