// Copyright (c) 2025 Robert Valentine. All rights reserved.

#include "AudioDSPChain.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Memory/Memory.h"
#include "AudioSource.h"

AudioSource* AudioDSPChain::_source = nullptr;
bool AudioDSPChain::_isEnabled = true;
int32 AudioDSPChain::_lastSampleRate = 48000;
Array<AudioDSPEffect*> AudioDSPChain::_effects;
Array<float> AudioDSPChain::_mixBuffer;
CriticalSection AudioDSPChain::_locker;

AudioDSPChain::AudioDSPChain(AudioSource* source)
{
    _source = source;
    _isEnabled = true;
    _lastSampleRate = 48000;

    // Initialize mix buffer with some reasonable size
    _mixBuffer.Resize(1024);
}

AudioDSPChain::~AudioDSPChain()
{
    Clear();
}

void AudioDSPChain::SetEnabled(bool value)
{
    ScopeLock lock(_locker);
    _isEnabled = value;
}

void AudioDSPChain::AddEffect(AudioDSPEffect* effect)
{
    if (!effect)
        return;

    ScopeLock lock(_locker);

    // Check if effect already exists
    if (_effects.Contains(effect))
        return;

    _effects.Add(effect);

    LOG(Info, "AudioDSPChain: Added effect type {0} to source {1}", (int)effect->GetType(), _source->GetNamePath());
}

void AudioDSPChain::RemoveEffect(AudioDSPEffect* effect)
{
    if (!effect)
        return;

    ScopeLock lock(_locker);
    _effects.Remove(effect);
}

Array<AudioDSPEffect*> AudioDSPChain::GetEffects() const
{
    ScopeLock lock(_locker);
    // Create a new array as a copy of the effects array
    return Array<AudioDSPEffect*>(_effects);
}

AudioDSPEffect* AudioDSPChain::GetEffectByType(AudioDSPEffect::EffectType type) const
{
    ScopeLock lock(_locker);

    for (AudioDSPEffect* effect : _effects)
    {
        if (effect->GetType() == type)
            return effect;
    }

    return nullptr;
}

void AudioDSPChain::Process(float* buffer, int32 sampleCount, int32 channels, int32 sampleRate)
{
    ScopeLock lock(_locker);

    if (!_isEnabled || _effects.IsEmpty())
        return;

    _lastSampleRate = sampleRate;

    // Resize mix buffer if needed
    const int32 bufferSize = sampleCount * channels;
    if (_mixBuffer.Count() < bufferSize)
        _mixBuffer.Resize(bufferSize);

    // Process each effect in chain
    float* input = buffer;
    float* output = _mixBuffer.Get();

    for (int i = 0; i < _effects.Count(); i++)
    {
        AudioDSPEffect* effect = _effects[i];

        if (effect->IsEnabled())
        {
            effect->Process(input, output, sampleCount, channels, sampleRate);

            // Swap buffers for next effect
            float* temp = input;
            input = output;
            output = (output == buffer) ? _mixBuffer.Get() : buffer;
        }
    }

    // If the final output is not in the original buffer, copy it back
    if (input != buffer)
        Memory::CopyItems(buffer, input, bufferSize * sizeof(float));
}

void AudioDSPChain::Clear()
{
    ScopeLock lock(_locker);

    for (AudioDSPEffect* effect : _effects)
    {
        Delete(effect);
    }

    _effects.Clear();
}
