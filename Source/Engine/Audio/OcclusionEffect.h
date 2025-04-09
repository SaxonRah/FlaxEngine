// OcclusionEffect.h
#pragma once

#include "AudioEffect.h"

/// <summary>
/// Audio effect that simulates sound occlusion and obstruction
/// </summary>
API_CLASS() class FLAXENGINE_API OcclusionEffect : public AudioEffect
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(OcclusionEffect);

private:
    float _occlusionLevel;
    float _lowPassCoeff;
    float _volumeReduction;
    
    // Filter state
    float _lastSample;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="OcclusionEffect"/> class.
    /// </summary>
    /// <param name="params">The object initialization parameters.</param>
    OcclusionEffect(const SpawnParams& params)
        : AudioEffect(params)
        , _occlusionLevel(0.0f)
        , _lowPassCoeff(0.0f)
        , _volumeReduction(0.0f)
        , _lastSample(0.0f)
    {
    }

    /// <summary>
    /// Gets the occlusion level (0 = no occlusion, 1 = fully occluded).
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetOcclusionLevel() const
    {
        return _occlusionLevel;
	}
	
    /// <summary>
    /// Sets the occlusion level (0 = no occlusion, 1 = fully occluded).
    /// </summary>
    API_PROPERTY() void SetOcclusionLevel(float value)
    {
        _occlusionLevel = Math::Saturate(value);
        UpdateParameters();
    }

    /// <summary>
    /// Updates the internal effect parameters based on occlusion level.
    /// </summary>
    void UpdateParameters()
    {
        // Calculate low-pass filter coefficient based on occlusion
        // Higher occlusion = more filtering of high frequencies
        _lowPassCoeff = 0.2f + 0.7f * _occlusionLevel;
        
        // Calculate volume reduction based on occlusion
        // Higher occlusion = more volume reduction
        _volumeReduction = 1.0f - (_occlusionLevel * 0.8f);
    }

    // [AudioEffect]
    void Process(const float* inputBuffer, float* outputBuffer, uint32 numSamples, const AudioDataInfo& format) override;

    static OcclusionEffect* Spawn()
    {
        SpawnParams params(Guid::New(), TypeInitializer);
        return New<OcclusionEffect>(params);
    }

protected:
    // [AudioEffect]
    void OnParamsChanged() override
    {
        UpdateParameters();
    }
};
