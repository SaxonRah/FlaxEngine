// AudioEffectChain.h
#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "AudioEffect.h"

/// <summary>
/// Represents a chain of audio effects to be applied in sequence
/// </summary>
API_CLASS() class FLAXENGINE_API AudioEffectChain : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(AudioEffectChain);

private:
    Array<AudioEffect*> _effects;
    Array<float> _processingBuffer;
    bool _isDirty;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="AudioEffectChain"/> class.
    /// </summary>
    /// <param name="params">The object initialization parameters.</param>
    AudioEffectChain(const SpawnParams& params)
        : ScriptingObject(params)
        , _isDirty(false)
    {
    }

    /// <summary>
    /// Adds an effect to the chain.
    /// </summary>
    /// <param name="effect">The effect to add.</param>
    API_FUNCTION() void AddEffect(AudioEffect* effect);

    /// <summary>
    /// Removes an effect from the chain.
    /// </summary>
    /// <param name="effect">The effect to remove.</param>
    API_FUNCTION() void RemoveEffect(AudioEffect* effect);

    /// <summary>
    /// Moves an effect within the chain.
    /// </summary>
    /// <param name="effect">The effect to move.</param>
    /// <param name="newIndex">The new index.</param>
    API_FUNCTION() void MoveEffect(AudioEffect* effect, int32 newIndex);

    /// <summary>
    /// Gets all effects in the chain.
    /// </summary>
    /// <returns>Array of effects.</returns>
    API_PROPERTY() Array<AudioEffect*> GetEffects() const
    {
        return _effects;
    }

    /// <summary>
    /// Clears all effects from the chain.
    /// </summary>
    API_FUNCTION() void Clear();

    /// <summary>
    /// Processes the audio data through the effect chain.
    /// </summary>
    /// <param name="inputBuffer">The input buffer.</param>
    /// <param name="outputBuffer">The output buffer.</param>
    /// <param name="numSamples">The number of samples to process.</param>
    /// <param name="format">The audio data format.</param>
    void Process(const float* inputBuffer, float* outputBuffer, uint32 numSamples, const AudioDataInfo& format);

    /// <summary>
    /// Marks the chain as dirty, requiring recreation of backend resources.
    /// </summary>
    void MarkDirty()
    {
        _isDirty = true;
    }

    /// <summary>
    /// Gets a value indicating whether this chain is dirty.
    /// </summary>
    bool IsDirty() const
    {
        return _isDirty;
    }

    /// <summary>
    /// Clears the dirty flag.
    /// </summary>
    void ClearDirty()
    {
        _isDirty = false;
    }

    static AudioEffectChain* Spawn()
    {
        SpawnParams params(Guid::New(), TypeInitializer);
        return New<AudioEffectChain>(params);
    }
};
