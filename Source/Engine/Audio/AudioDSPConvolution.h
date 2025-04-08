// Copyright (c) 2025 Robert Valentine. All rights reserved.

#pragma once

#include "AudioDSPEffect.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Convolution effect for impulse response processing.
/// </summary>
class FLAXENGINE_API AudioDSPConvolution : public AudioDSPEffect
{
private:
    Array<float> _impulseResponse;
    int32 _irLength;
    Array<float> _buffer;
    int32 _bufferPosition;
    float _wetLevel;
    float _dryLevel;

public:
    /// <summary>
    /// Creates a new convolution effect.
    /// </summary>
    AudioDSPConvolution();

    /// <summary>
    /// Gets the impulse response data.
    /// </summary>
    Array<float> GetImpulseResponse() const;

    /// <summary>
    /// Sets the impulse response data.
    /// </summary>
    void SetImpulseResponse(const Array<float>& impulseResponse);

    /// <summary>
    /// Gets the wet level (processed signal amount).
    /// </summary>
    FORCE_INLINE float GetWetLevel() const
    {
        return _wetLevel;
    }

    /// <summary>
    /// Sets the wet level (processed signal amount).
    /// </summary>
    void SetWetLevel(float value);

    /// <summary>
    /// Gets the dry level (original signal amount).
    /// </summary>
    FORCE_INLINE float GetDryLevel() const
    {
        return _dryLevel;
    }

    /// <summary>
    /// Sets the dry level (original signal amount).
    /// </summary>
    void SetDryLevel(float value);

    // [AudioDSPEffect]
    void Process(const float* input, float* output, int32 sampleCount, int32 channels, int32 sampleRate) override;
};
