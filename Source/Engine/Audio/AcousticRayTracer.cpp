// AcousticRayTracer.cpp
#include "AcousticRayTracer.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/Actors/PhysicsColliderActor.h"

/*
Scene* AcousticRayTracer::_scene = nullptr;
int32 AcousticRayTracer::_maxReflections = 4;
float AcousticRayTracer::_maxDistance = 10000.0f;
float AcousticRayTracer::_speedOfSound = 343.0f * 100.0f; // 343 m/s in Flax units
*/

bool AcousticRayTracer::TraceDirectPath(const Vector3& sourcePosition, const Vector3& listenerPosition, RayCastHit& hit)
{
    if (!_scene)
        return false;
    
    const Vector3 direction = listenerPosition - sourcePosition;
    const float distance = direction.Length();
    
    if (distance < ZeroTolerance)
        return false;
    
    return Physics::RayCast(sourcePosition, direction.GetNormalized(), hit, distance, Physics::LayerMasks[0], true);
}

void AcousticRayTracer::TraceReflectionPaths(
    const Vector3& sourcePosition,
    const Quaternion& sourceOrientation,
    const Vector3& listenerPosition,
    int32 rayCount,
    Array<AcousticRayPath>& paths)
{
    if (!_scene)
        return;
    
    // Clear and resize output array
    paths.Clear();
    paths.Resize(rayCount);
    
    for (int32 i = 0; i < rayCount; i++)
    {
        AcousticRayPath& path = paths[i];
        path.Points.Clear();
        path.MaterialIds.Clear();
        path.TotalDistance = 0.0f;
        path.Attenuation = 1.0f;
        path.Delay = 0.0f;
        
        // Use golden ratio spherical distribution for ray directions
        const float phi = PI * (3.0f - Math::Sqrt(5.0f)); // Golden angle
        const float y = 1.0f - (i / (float)(rayCount - 1)) * 2.0f; // y goes from 1 to -1
        const float radius = Math::Sqrt(1.0f - y * y); // Radius at y
        const float theta = phi * i; // Golden angle increment
        
        Vector3 rayDir(
            Math::Cos(theta) * radius,
            y,
            Math::Sin(theta) * radius
        );
        
        // Transform direction to world space
        rayDir = sourceOrientation * rayDir;
        
        // Start ray path
        Vector3 rayOrigin = sourcePosition;
        path.Points.Add(rayOrigin);
        
        // Trace ray with reflections
        for (int32 r = 0; r < _maxReflections; r++)
        {
            RayCastHit hit;
            if (!Physics::RayCast(rayOrigin, rayDir, hit, _maxDistance, Physics::LayerMasks[0], true))
                break;
            
            // Add hit point to path
            path.Points.Add(hit.Point);
            
            // Update distance and attenuation
            const float segmentDistance = Vector3::Distance(rayOrigin, hit.Point);
            path.TotalDistance += segmentDistance;
            
            // Apply distance attenuation (inverse square law)
            path.Attenuation *= 1.0f / (1.0f + segmentDistance * 0.01f);
            
            // Check if ray can reach listener from this point
            RayCastHit listenerHit;
            const Vector3 toListener = listenerPosition - hit.Point;
            const float distToListener = toListener.Length();
            
            if (distToListener < _maxDistance)
            {
                if (!Physics::RayCast(hit.Point, toListener.GetNormalized(), listenerHit, distToListener, Physics::LayerMasks[0], true))
                {
                    // Ray can reach listener from this reflection point
                    path.Points.Add(listenerPosition);
                    path.TotalDistance += distToListener;
                    path.Attenuation *= 1.0f / (1.0f + distToListener * 0.01f);
                    break;
                }
            }
            
            // Store material ID for acoustic properties
            path.MaterialIds.Add(hit.Collider ? hit.Collider->GetID() : Guid::Empty);
            
            // Calculate reflection vector
            Vector3::Reflect(rayDir, hit.Normal, rayDir);

            rayOrigin = hit.Point + rayDir * 1.0f; // Offset to avoid self-intersection
        }
        
        // Calculate delay based on total distance and speed of sound
        path.Delay = path.TotalDistance / _speedOfSound;

        //path.Delay = Math::Abs(path.TotalDistance) / _speedOfSound;
    }
}
