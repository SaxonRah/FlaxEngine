// RayTracedAudioManager.h
#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Ray.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Physics/Colliders/Collider.h"
#include "AudioEffectChain.h"

class AudioListener;
class AudioSource;
class OcclusionEffect;
class ReverbEffect;
class AcousticMaterialEffect;

/// <summary>
/// Manager for ray-traced audio simulation
/// </summary>
API_CLASS() class FLAXENGINE_API RayTracedAudioManager : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(RayTracedAudioManager);

private:
    static bool _enabled;
    static int32 _maxRays;
    static int32 _maxReflections;
    static float _maxDistance;
    static Dictionary<Guid, AudioEffectChain*> _sourceEffectChains;
    
    struct RayPath
    {
        Array<Vector3> Points;
        Array<Guid> MaterialIds;
        float TotalDistance;
        float Attenuation;
    };
    
    static Array<RayPath> _rayPaths;

public:

    RayTracedAudioManager(const SpawnParams& params)
            : ScriptingObject(params)
    {
    }

    /// <summary>
    /// Gets a value indicating whether ray tracing is enabled.
    /// </summary>
    API_PROPERTY() static bool GetEnabled()
    {
        return _enabled;
    }

    /// <summary>
    /// Sets a value indicating whether ray tracing is enabled.
    /// </summary>
    API_PROPERTY() static void SetEnabled(bool value)
    {
        _enabled = value;
    }

    /// <summary>
    /// Gets the maximum number of rays cast per sound source.
    /// </summary>
    API_PROPERTY() static int32 GetMaxRays()
    {
        return _maxRays;
    }

    /// <summary>
    /// Sets the maximum number of rays cast per sound source.
    /// </summary>
    API_PROPERTY() static void SetMaxRays(int32 value)
    {
        _maxRays = Math::Clamp(value, 1, 256);
    }

    /// <summary>
    /// Gets the maximum reflection count for each ray.
    /// </summary>
    API_PROPERTY() static int32 GetMaxReflections()
    {
        return _maxReflections;
    }

    /// <summary>
    /// Sets the maximum reflection count for each ray.
    /// </summary>
    API_PROPERTY() static void SetMaxReflections(int32 value)
    {
        _maxReflections = Math::Clamp(value, 0, 10);
    }

    /// <summary>
    /// Gets the maximum distance for ray tracing.
    /// </summary>
    API_PROPERTY() static float GetMaxDistance()
    {
        return _maxDistance;
    }

    /// <summary>
    /// Sets the maximum distance for ray tracing.
    /// </summary>
    API_PROPERTY() static void SetMaxDistance(float value)
    {
        _maxDistance = Math::Max(value, 100.0f);
    }

    /// <summary>
    /// Initializes the ray traced audio manager.
    /// </summary>
    API_FUNCTION() static void Init();

    /// <summary>
    /// Updates the ray traced audio simulation.
    /// </summary>
    API_FUNCTION() static void Update();

    /// <summary>
    /// Traces sound rays between source and listener.
    /// </summary>
    /// <param name="scene">The scene.</param>
    /// <param name="source">The audio source.</param>
    /// <param name="listener">The audio listener.</param>
    static void TraceSound(Scene* scene, AudioSource* source, AudioListener* listener);

    /// <summary>
    /// Disposes the ray traced audio manager.
    /// </summary>
    API_FUNCTION() static void Dispose();

    static RayTracedAudioManager* Spawn()
    {
        SpawnParams params(Guid::New(), TypeInitializer);
        return New<RayTracedAudioManager>(params);
    }
};
