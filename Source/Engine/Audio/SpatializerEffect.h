// SpatializerEffect.h
#pragma once

#include "AudioEffect.h"

/// <summary>
/// A more advanced 3D spatialization effect
/// </summary>
API_CLASS() class FLAXENGINE_API SpatializerEffect : public AudioEffect
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(SpatializerEffect);

private:
    Vector3 _listenerPosition;
    Quaternion _listenerOrientation;
    Vector3 _sourcePosition;
    float _sourceRadius;
    float _distanceAttenuation;
    float _directionAttenuation;
    float _reverbMix;
    
    // Head-related transfer function (HRTF) simulation
    struct HRTFFilter
    {
        float Delay;
        float Gain;
        Array<float> DelayLine;
        int32 DelayLinePos;
    };
    
    HRTFFilter _leftEar;
    HRTFFilter _rightEar;
    Array<float> _tempBuffer;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="SpatializerEffect"/> class.
    /// </summary>
    /// <param name="params">The object initialization parameters.</param>
    SpatializerEffect(const SpawnParams& params)
        : AudioEffect(params)
        , _listenerPosition(Vector3::Zero)
        , _listenerOrientation(Quaternion::Identity)
        , _sourcePosition(Vector3::Zero)
        , _sourceRadius(1.0f)
        , _distanceAttenuation(1.0f)
        , _directionAttenuation(1.0f)
        , _reverbMix(0.3f)
    {
        _leftEar.DelayLinePos = 0;
        _rightEar.DelayLinePos = 0;
        UpdateEarParameters();
    }

    /// <summary>
    /// Gets the listener position.
    /// </summary>
    API_PROPERTY() FORCE_INLINE Vector3 GetListenerPosition() const
    {
        return _listenerPosition;
    }

    /// <summary>
    /// Sets the listener position.
    /// </summary>
    API_PROPERTY() void SetListenerPosition(const Vector3& value)
    {
        _listenerPosition = value;
        UpdateEarParameters();
    }

    /// <summary>
    /// Gets the listener orientation.
    /// </summary>
    API_PROPERTY() FORCE_INLINE Quaternion GetListenerOrientation() const
    {
        return _listenerOrientation;
    }

    /// <summary>
    /// Sets the listener orientation.
    /// </summary>
    API_PROPERTY() void SetListenerOrientation(const Quaternion& value)
    {
        _listenerOrientation = value;
        UpdateEarParameters();
    }

    /// <summary>
    /// Gets the source position.
    /// </summary>
    API_PROPERTY() FORCE_INLINE Vector3 GetSourcePosition() const
    {
        return _sourcePosition;
    }

    /// <summary>
    /// Sets the source position.
    /// </summary>
    API_PROPERTY() void SetSourcePosition(const Vector3& value)
    {
        _sourcePosition = value;
        UpdateEarParameters();
    }

    /// <summary>
    /// Gets the source radius.
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetSourceRadius() const
    {
        return _sourceRadius;
    }

    /// <summary>
    /// Sets the source radius.
    /// </summary>
    API_PROPERTY() void SetSourceRadius(float value)
    {
        _sourceRadius = Math::Max(0.1f, value);
        UpdateEarParameters();
    }

    /// <summary>
    /// Gets the reverb mix level.
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetReverbMix() const
    {
        return _reverbMix;
    }

    /// <summary>
    /// Sets the reverb mix level.
    /// </summary>
    API_PROPERTY() void SetReverbMix(float value)
    {
        _reverbMix = Math::Saturate(value);
    }

    // [AudioEffect]
    void Process(const float* inputBuffer, float* outputBuffer, uint32 numSamples, const AudioDataInfo& format) override;

    static SpatializerEffect* Spawn()
    {
        SpawnParams params(Guid::New(), TypeInitializer);
        return New<SpatializerEffect>(params);
    }

private:
    void UpdateEarParameters();
    void ProcessChannel(const float* input, float* output, uint32 numSamples, uint32 sampleRate, HRTFFilter& filter);
};
