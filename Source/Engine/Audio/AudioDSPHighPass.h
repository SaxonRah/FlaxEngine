// Copyright (c) 2025 Robert Valentine. All rights reserved.

#pragma once

#include "AudioDSPEffect.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// High-pass filter effect.
/// </summary>
class FLAXENGINE_API AudioDSPHighPass : public AudioDSPEffect
{
private:
    float _cutoffFrequency;
    float _resonance;

    // State variables
    Array<float> _x1, _x2, _y1, _y2;
    float _a0, _a1, _a2, _b1, _b2;

public:
    /// <summary>
    /// Creates a new high-pass filter effect.
    /// </summary>
    AudioDSPHighPass();

    /// <summary>
    /// Gets the cutoff frequency in Hz.
    /// </summary>
    FORCE_INLINE float GetCutoffFrequency() const
    {
        return _cutoffFrequency;
    }

    /// <summary>
    /// Sets the cutoff frequency in Hz.
    /// </summary>
    void SetCutoffFrequency(float value);

    /// <summary>
    /// Gets the resonance (Q factor).
    /// </summary>
    FORCE_INLINE float GetResonance() const
    {
        return _resonance;
    }

    /// <summary>
    /// Sets the resonance (Q factor).
    /// </summary>
    void SetResonance(float value);

    // [AudioDSPEffect]
    void Process(const float* input, float* output, int32 sampleCount, int32 channels, int32 sampleRate) override;

private:
    void UpdateCoefficients(int32 sampleRate);
};
