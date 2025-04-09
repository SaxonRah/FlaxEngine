// AudioEffect.h
#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Core/Math/Math.h"
#include "Types.h"

/// <summary>
/// Base class for all audio effects in the DSP chain
/// </summary>
API_CLASS() class FLAXENGINE_API AudioEffect : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE(AudioEffect);

public:

    /// <summary>
    /// Gets a value indicating whether this effect is enabled.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool GetIsEnabled() const
    {
        return _isEnabled;
    }

    /// <summary>
    /// Sets a value indicating whether this effect is enabled.
    /// </summary>
    API_PROPERTY() virtual void SetIsEnabled(bool value)
    {
        _isEnabled = value;
        OnParamsChanged();
    }

    /// <summary>
    /// Gets the wet/dry mix ratio (0 = all dry, 1 = all wet).
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetWetDryMix() const
    {
        return _wetDryMix;
    }

    /// <summary>
    /// Sets the wet/dry mix ratio (0 = all dry, 1 = all wet).
    /// </summary>
    API_PROPERTY() virtual void SetWetDryMix(float value)
    {
        _wetDryMix = Math::Saturate(value);
        OnParamsChanged();
    }

    /// <summary>
    /// Processes the audio data.
    /// </summary>
    /// <param name="inputBuffer">The input buffer.</param>
    /// <param name="outputBuffer">The output buffer.</param>
    /// <param name="numSamples">The number of samples to process.</param>
    /// <param name="format">The audio data format.</param>
    virtual void Process(const float* inputBuffer, float* outputBuffer, uint32 numSamples, const AudioDataInfo& format)
    {
        // Default implementation: pass-through (no effect)
        if (inputBuffer != outputBuffer)
            Platform::MemoryCopy(outputBuffer, inputBuffer, numSamples * sizeof(float));
    }

protected:
    /// <summary>
    /// Called when effect parameters have been changed.
    /// </summary>
    virtual void OnParamsChanged()
    {
    }

private:
    bool _isEnabled;
    float _wetDryMix;
};
