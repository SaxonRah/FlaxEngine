// Copyright (c) 2025 Robert Valentine. All rights reserved.

#pragma once

#include "AudioDSPEffect.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/CriticalSection.h"

class AudioSource;

/// <summary>
/// Manages audio DSP processing chain for an audio source.
/// </summary>
class FLAXENGINE_API AudioDSPChain
{
private:
    static AudioSource* _source;
    static Array<AudioDSPEffect*> _effects;
    static Array<float> _mixBuffer;
    static CriticalSection _locker;
    static bool _isEnabled;
    static int32 _lastSampleRate;

public:
    /// <summary>
    /// Creates a new DSP chain for an audio source.
    /// </summary>
    /// <param name="source">Audio source to process</param>

    AudioDSPChain(AudioSource* source);

    /// <summary>
    /// Finalizes an instance of the <see cref="AudioDSPChain"/> class.
    /// </summary>
    ~AudioDSPChain();

    /// <summary>
    /// Gets the audio source this chain is attached to.
    /// </summary>
    FORCE_INLINE AudioSource* GetSource() const
    {
        return _source;
    }

    /// <summary>
    /// Gets the DSP chain enabled state.
    /// </summary>
    FORCE_INLINE bool IsEnabled() const
    {
        return _isEnabled;
    }

    /// <summary>
    /// Sets the DSP chain enabled state.
    /// </summary>
    static void SetEnabled(bool value);

    /// <summary>
    /// Adds a DSP effect to the chain.
    /// </summary>
    /// <param name="effect">Effect to add</param>
    static void AddEffect(AudioDSPEffect* effect);

    /// <summary>
    /// Removes a DSP effect from the chain.
    /// </summary>
    /// <param name="effect">Effect to remove</param>
    void RemoveEffect(AudioDSPEffect* effect);

    /// <summary>
    /// Gets the effects in this chain.
    /// </summary>
    Array<AudioDSPEffect*> GetEffects() const;

    /// <summary>
    /// Gets an effect by type.
    /// </summary>
    /// <param name="type">Effect type to find</param>
    AudioDSPEffect* GetEffectByType(AudioDSPEffect::EffectType type) const;

    /// <summary>
    /// Processes audio samples.
    /// </summary>
    /// <param name="buffer">Audio buffer to process</param>
    /// <param name="sampleCount">Samples count per channel</param>
    /// <param name="channels">Channels count</param>
    /// <param name="sampleRate">Audio sample rate (eg. 48000)</param>
    static void Process(float* buffer, int32 sampleCount, int32 channels, int32 sampleRate);

    /// <summary>
    /// Clears all effects from this chain.
    /// </summary>
    void Clear();
};
