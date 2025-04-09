// EQEffect.h
#pragma once

#include "AudioEffect.h"

/// <summary>
/// Simple 3-band EQ effect
/// </summary>
API_CLASS() class FLAXENGINE_API EQEffect : public AudioEffect
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(EQEffect);

private:
    float _lowGain;
    float _midGain;
    float _highGain;
    float _lowFreq;
    float _highFreq;
    
    // State variables for filters
    float _lowPass1, _lowPass2;
    float _bandPass1, _bandPass2;
    float _highPass1, _highPass2;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="EQEffect"/> class.
    /// </summary>
    /// <param name="params">The object initialization parameters.</param>
    EQEffect(const SpawnParams& params)
        : AudioEffect(params)
        , _lowGain(1.0f)
        , _midGain(1.0f)
        , _highGain(1.0f)
        , _lowFreq(200.0f)
        , _highFreq(2000.0f)
        , _lowPass1(0.0f)
        , _lowPass2(0.0f)
        , _bandPass1(0.0f)
        , _bandPass2(0.0f)
        , _highPass1(0.0f)
        , _highPass2(0.0f)
    {
    }

    /// <summary>
    /// Gets the low frequency gain.
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetLowGain() const
    {
        return _lowGain;
    }

    /// <summary>
    /// Sets the low frequency gain.
    /// </summary>
    API_PROPERTY() void SetLowGain(float value)
    {
        _lowGain = Math::Clamp(value, 0.0f, 2.0f);
    }

    /// <summary>
    /// Gets the mid frequency gain.
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetMidGain() const
    {
        return _midGain;
    }

    /// <summary>
    /// Sets the mid frequency gain.
    /// </summary>
    API_PROPERTY() void SetMidGain(float value)
    {
        _midGain = Math::Clamp(value, 0.0f, 2.0f);
    }

    /// <summary>
    /// Gets the high frequency gain.
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetHighGain() const
    {
        return _highGain;
    }

    /// <summary>
    /// Sets the high frequency gain.
    /// </summary>
    API_PROPERTY() void SetHighGain(float value)
    {
        _highGain = Math::Clamp(value, 0.0f, 2.0f);
    }

    // [AudioEffect]
    void Process(const float* inputBuffer, float* outputBuffer, uint32 numSamples, const AudioDataInfo& format) override;

    static EQEffect* Spawn()
    {
        SpawnParams params(Guid::New(), TypeInitializer);
        return New<EQEffect>(params);
    }
};
