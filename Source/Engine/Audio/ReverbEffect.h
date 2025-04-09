// ReverbEffect.h
#pragma once

#include "AudioEffect.h"

/// <summary>
/// Simple reverb audio effect
/// </summary>
API_CLASS() class FLAXENGINE_API ReverbEffect : public AudioEffect
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(ReverbEffect);

private:
    float _decay;
    float _roomSize;
    Array<float> _delayLine;
    uint32 _delayLinePos;
    float _feedback;
    float _damping;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ReverbEffect"/> class.
    /// </summary>
    /// <param name="params">The object initialization parameters.</param>
    ReverbEffect(const SpawnParams& params)
        : AudioEffect(params)
        , _decay(0.5f)
        , _roomSize(0.5f)
        , _delayLinePos(0)
        , _feedback(0.3f)
        , _damping(0.2f)
    {
    }

    /// <summary>
    /// Gets the decay time.
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetDecay() const
    {
        return _decay;
    }

    /// <summary>
    /// Sets the decay time.
    /// </summary>
    API_PROPERTY() void SetDecay(float value)
    {
        _decay = Math::Clamp(value, 0.0f, 0.99f);
        OnParamsChanged();
    }

    /// <summary>
    /// Gets the room size.
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetRoomSize() const
    {
        return _roomSize;
    }

    /// <summary>
    /// Sets the room size.
    /// </summary>
    API_PROPERTY() void SetRoomSize(float value)
    {
        _roomSize = Math::Clamp(value, 0.0f, 1.0f);
        OnParamsChanged();
    }

    // [AudioEffect]
    void Process(const float* inputBuffer, float* outputBuffer, uint32 numSamples, const AudioDataInfo& format) override;

    static ReverbEffect* Spawn()
    {
        SpawnParams params(Guid::New(), TypeInitializer);
        return New<ReverbEffect>(params);
    }

protected:
    // [AudioEffect]
    void OnParamsChanged() override;
};
