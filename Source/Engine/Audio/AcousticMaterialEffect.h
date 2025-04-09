// AcousticMaterialEffect.h
#pragma once

#include "AudioEffect.h"

/// <summary>
/// Audio effect that simulates acoustic material properties for ray-traced sound
/// </summary>
API_CLASS() class FLAXENGINE_API AcousticMaterialEffect : public AudioEffect
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(AcousticMaterialEffect);

private:
    float _absorption;     // Overall sound absorption coefficient
    float _reflection;     // Overall reflection coefficient
    float _transmission;   // Sound transmission coefficient
    float _scattering;     // Sound scattering coefficient
    float _lowFreqAbsorption;
    float _midFreqAbsorption;
    float _highFreqAbsorption;
    
    // Filter coefficients
    float _lowCoeff, _midCoeff, _highCoeff;
    
    // Filter states
    float _lowPass, _bandPass, _highPass;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="AcousticMaterialEffect"/> class.
    /// </summary>
    /// <param name="params">The object initialization parameters.</param>
    AcousticMaterialEffect(const SpawnParams& params)
        : AudioEffect(params)
        , _absorption(0.5f)
        , _reflection(0.5f)
        , _transmission(0.2f)
        , _scattering(0.3f)
        , _lowFreqAbsorption(0.2f)
        , _midFreqAbsorption(0.5f)
        , _highFreqAbsorption(0.8f)
        , _lowCoeff(0.0f)
        , _midCoeff(0.0f)
        , _highCoeff(0.0f)
        , _lowPass(0.0f)
        , _bandPass(0.0f)
        , _highPass(0.0f)
    {
        UpdateCoefficients();
    }

    /// <summary>
    /// Gets the overall sound absorption coefficient.
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetAbsorption() const
    {
        return _absorption;
    }

    /// <summary>
    /// Sets the overall sound absorption coefficient.
    /// </summary>
    API_PROPERTY() void SetAbsorption(float value)
    {
        _absorption = Math::Saturate(value);
        UpdateCoefficients();
    }

    /// <summary>
    /// Gets the low frequency absorption coefficient.
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetLowFreqAbsorption() const
    {
        return _lowFreqAbsorption;
    }

    /// <summary>
    /// Sets the low frequency absorption coefficient.
    /// </summary>
    API_PROPERTY() void SetLowFreqAbsorption(float value)
    {
        _lowFreqAbsorption = Math::Saturate(value);
        UpdateCoefficients();
    }

    /// <summary>
    /// Gets the mid frequency absorption coefficient.
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetMidFreqAbsorption() const
    {
        return _midFreqAbsorption;
    }

    /// <summary>
    /// Sets the mid frequency absorption coefficient.
    /// </summary>
    API_PROPERTY() void SetMidFreqAbsorption(float value)
    {
        _midFreqAbsorption = Math::Saturate(value);
        UpdateCoefficients();
    }

    /// <summary>
    /// Gets the high frequency absorption coefficient.
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetHighFreqAbsorption() const
    {
        return _highFreqAbsorption;
    }

    /// <summary>
    /// Sets the high frequency absorption coefficient.
    /// </summary>
    API_PROPERTY() void SetHighFreqAbsorption(float value)
    {
        _highFreqAbsorption = Math::Saturate(value);
        UpdateCoefficients();
    }

    /// <summary>
    /// Gets the sound scattering coefficient (diffusion).
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetScattering() const
    {
        return _scattering;
    }

    /// <summary>
    /// Sets the sound scattering coefficient (diffusion).
    /// </summary>
    API_PROPERTY() void SetScattering(float value)
    {
        _scattering = Math::Saturate(value);
    }

    // [AudioEffect]
    void Process(const float* inputBuffer, float* outputBuffer, uint32 numSamples, const AudioDataInfo& format) override;

    static AcousticMaterialEffect* Spawn()
    {
        SpawnParams params(Guid::New(), TypeInitializer);
        return New<AcousticMaterialEffect>(params);
    }

private:
    void UpdateCoefficients();
};
