// AcousticRayTracer.h
#pragma once

#include "Engine/Core/Math/Ray.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Physics/Types.h"

/// <summary>
/// Acoustic ray path with materials and attenuation
/// </summary>
API_STRUCT() struct AcousticRayPath
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(AcousticRayPath);
    
    /// <summary>
    /// Points along the ray path including source and final destination
    /// </summary>
    API_FIELD() Array<Vector3> Points;
    
    /// <summary>
    /// Material IDs for each reflection surface
    /// </summary>
    API_FIELD() Array<Guid> MaterialIds;
    
    /// <summary>
    /// Distance traveled along the path
    /// </summary>
    API_FIELD() float TotalDistance;
    
    /// <summary>
    /// Attenuation due to distance and reflections
    /// </summary>
    API_FIELD() float Attenuation;
    
    /// <summary>
    /// Time delay for the path
    /// </summary>
    API_FIELD() float Delay;
};


/// <summary>
/// Class for performing acoustic ray tracing
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API AcousticRayTracer
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(AcousticRayTracer);

public:

    AcousticRayTracer()
    {
        _scene = nullptr;
        _maxReflections = 4;
        _maxDistance = 10000.0f;
        _speedOfSound = 343.0f * 100.0f; // 343 m/s in Flax units
    }

private:
    Scene* _scene;
    int32 _maxReflections;
    float _maxDistance;
    float _speedOfSound;

public:

    Scene* GetScene() const
    {
        return _scene;
    }

    void SetScene(Scene* scene)
    {
        _scene = scene;
    }

    int32 GetMaxReflections() const
    {
        return _maxReflections;
    }

    void SetMaxReflections(int32 value)
    {
        _maxReflections = Math::Max(0, value);
    }

    float GetMaxDistance() const
    {
        return _maxDistance;
    }

    void SetMaxDistance(float value)
    {
        _maxDistance = Math::Max(100.0f, value);
    }

    float GetSpeedOfSound() const
    {
        return _speedOfSound;
    }
    
    void SetSpeedOfSound(float value)
    {
        _speedOfSound = Math::Max(100.0f, value);
    }

    bool TraceDirectPath(const Vector3& sourcePosition, const Vector3& listenerPosition, RayCastHit& hit);

    void TraceReflectionPaths(
        const Vector3& sourcePosition,
        const Quaternion& sourceOrientation,
        const Vector3& listenerPosition,
        int32 rayCount,
        Array<AcousticRayPath>& paths);
};
