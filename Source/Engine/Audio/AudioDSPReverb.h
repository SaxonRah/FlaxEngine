// Copyright (c) 2025 Robert Valentine. All rights reserved.

#pragma once

#include "AudioDSPEffect.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Reverb effect for simulating acoustic spaces.
/// </summary>
class FLAXENGINE_API AudioDSPReverb : public AudioDSPEffect
{
private:
    float _roomSize;
    float _damping;
    float _wetLevel;
    float _dryLevel;
    float _width;

    int32 _lastSampleRateChecked = 0;

    // Comb and allpass filters for reverb algorithm
    static const int NUM_COMBS = 8;
    static const int NUM_ALLPASSES = 4;

    struct CombFilter {
        Array<float> buffer;
        int bufferSize;
        int bufferIndex;
        float feedback;
        float filterStore;
        float damp1, damp2;

        CombFilter() : bufferSize(0), bufferIndex(0), feedback(0), filterStore(0), damp1(0), damp2(0) {}
    };

    struct AllpassFilter {
        Array<float> buffer;
        int bufferSize;
        int bufferIndex;
        float feedback;

        AllpassFilter() : bufferSize(0), bufferIndex(0), feedback(0) {}
    };

    CombFilter _combFilters[NUM_COMBS];
    AllpassFilter _allpassFilters[NUM_ALLPASSES];

public:
    /// <summary>
    /// Creates a new reverb effect.
    /// </summary>
    AudioDSPReverb();

    /// <summary>
    /// Gets the room size parameter.
    /// </summary>
    FORCE_INLINE float GetRoomSize() const
    {
        return _roomSize;
    }

    /// <summary>
    /// Sets the room size parameter.
    /// </summary>
    void SetRoomSize(float value);

    /// <summary>
    /// Gets the damping parameter.
    /// </summary>
    FORCE_INLINE float GetDamping() const
    {
        return _damping;
    }

    /// <summary>
    /// Sets the damping parameter.
    /// </summary>
    void SetDamping(float value);

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

    /// <summary>
    /// Gets the stereo width parameter.
    /// </summary>
    FORCE_INLINE float GetWidth() const
    {
        return _width;
    }

    /// <summary>
    /// Sets the stereo width parameter.
    /// </summary>
    void SetWidth(float value);

    // [AudioDSPEffect]
    void Process(const float* input, float* output, int32 sampleCount, int32 channels, int32 sampleRate) override;

private:
    void Initialize(int32 sampleRate);
    float ProcessComb(CombFilter& filter, float input);
    float ProcessAllpass(AllpassFilter& filter, float input);
    void UpdateParameters();
};
