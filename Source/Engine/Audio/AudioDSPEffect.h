// Copyright (c) 2025 Robert Valentine. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Types/Span.h"
#include "Engine/Platform/CriticalSection.h"

/// <summary>
/// Base class for DSP effects that can process audio buffers.
/// </summary>
class FLAXENGINE_API AudioDSPEffect
{
public:
    /// <summary>
    /// Effect type identifiers
    /// </summary>
    enum class EffectType
    {
        /// <summary>
        /// Custom effect
        /// </summary>
        Custom = 0,

        /// <summary>
        /// Low-pass filter
        /// </summary>
        LowPass = 1,

        /// <summary>
        /// High-pass filter
        /// </summary>
        HighPass = 2,

        /// <summary>
        /// Reverb effect
        /// </summary>
        Reverb = 3,

        /// <summary>
        /// Convolution effect
        /// </summary>
        Convolution = 4,

        /// <summary>
        /// Delay effect
        /// </summary>
        Delay = 5
    };

protected:
    bool _isEnabled = true;
    EffectType _type;
    CriticalSection _locker;

public:
    /// <summary>
    /// Creates a new DSP effect instance.
    /// </summary>
    /// <param name="type">Effect type</param>
    AudioDSPEffect(EffectType type);

    /// <summary>
    /// Finalizes an instance of the <see cref="AudioDSPEffect"/> class.
    /// </summary>
    virtual ~AudioDSPEffect() = default;

    /// <summary>
    /// Gets the effect type.
    /// </summary>
    FORCE_INLINE EffectType GetType() const
    {
        return _type;
    }

    /// <summary>
    /// Gets the effect enabled state.
    /// </summary>
    FORCE_INLINE bool IsEnabled() const
    {
        return _isEnabled;
    }

    /// <summary>
    /// Sets the effect enabled state.
    /// </summary>
    virtual void SetEnabled(bool value);

    /// <summary>
    /// Processes audio samples.
    /// </summary>
    /// <param name="input">Input samples buffer</param>
    /// <param name="output">Output samples buffer</param>
    /// <param name="sampleCount">Samples count per channel</param>
    /// <param name="channels">Channels count</param>
    /// <param name="sampleRate">Audio sample rate (eg. 48000)</param>
    virtual void Process(const float* input, float* output, int32 sampleCount, int32 channels, int32 sampleRate)
    {
        // Default implementation just copies input to output
        if (input != output)
            Memory::CopyItems(output, input, sampleCount * channels * sizeof(float));
    }
};
