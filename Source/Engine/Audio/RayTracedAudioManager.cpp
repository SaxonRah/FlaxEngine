// RayTracedAudioManager.cpp
#include "RayTracedAudioManager.h"
#include "Engine/Audio/Audio.h"
#include "Engine/Audio/AudioListener.h"
#include "Engine/Audio/AudioSource.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Engine/EngineService.h"
#include "OcclusionEffect.h"
#include "ReverbEffect.h"
#include "AcousticMaterialEffect.h"

bool RayTracedAudioManager::_enabled = false;
int32 RayTracedAudioManager::_maxRays = 32;
int32 RayTracedAudioManager::_maxReflections = 4;
float RayTracedAudioManager::_maxDistance = 10000.0f;
Dictionary<Guid, AudioEffectChain*> RayTracedAudioManager::_sourceEffectChains;
Array<RayTracedAudioManager::RayPath> RayTracedAudioManager::_rayPaths;

class RayTracedAudioService : public EngineService
{
public:
    RayTracedAudioService()
        : EngineService(TEXT("RayTracedAudio"), -40) // After Audio service (-50)
    {
    }

    bool Init() override
    {
        RayTracedAudioManager::Init();
        return false;
    }

    void Update() override
    {
        RayTracedAudioManager::Update();
    }

    void Dispose() override
    {
        RayTracedAudioManager::Dispose();
    }
};

RayTracedAudioService RayTracedAudioServiceInstance;

void RayTracedAudioManager::Init()
{
    _rayPaths.Resize(_maxRays);
}

void RayTracedAudioManager::Update()
{
    if (!_enabled)
        return;
    
    // Process each listener with each source
    for (AudioListener* listener : Audio::Listeners)
    {
        for (AudioSource* source : Audio::Sources)
        {
            // Skip sources that aren't 3D or aren't playing
            if (!source->Is3D() || source->GetState() != AudioSource::States::Playing)
                continue;
            
            // Trace sound rays for this source-listener pair
            TraceSound(listener->GetScene(), source, listener);
        }
    }
}

void RayTracedAudioManager::TraceSound(Scene* scene, AudioSource* source, AudioListener* listener)
{
    if (!scene || !source || !listener)
        return;
    
    // Get source and listener positions
    const Vector3 sourcePos = source->GetPosition();
    const Vector3 listenerPos = listener->GetPosition();
    
    // Calculate direct path distance
    const float directDistance = Vector3::Distance(sourcePos, listenerPos);
    
    // Skip if too far away
    if (directDistance > _maxDistance)
        return;
    
    // Initialize or get effect chain for this source
    AudioEffectChain* effectChain = nullptr;
    if (!_sourceEffectChains.TryGet(source->GetID(), effectChain))
    {
        // Create new effect chain with default effects
        SpawnParams params(Guid::New(), RayTracedAudioManager::TypeInitializer);
        effectChain = New<AudioEffectChain>(params);
        
        // Add occlusion effect
        OcclusionEffect* occlusionEffect = New<OcclusionEffect>(params);
        effectChain->AddEffect(occlusionEffect);
        
        // Add reverb effect
        ReverbEffect* reverbEffect = New<ReverbEffect>(params);
        effectChain->AddEffect(reverbEffect);
        
        // Store effect chain
        _sourceEffectChains.Add(source->GetID(), effectChain);
        
        // Assign to source
        source->SetEffectChain(effectChain);
    }
    
    // Direct ray test for occlusion
    RayCastHit hit;
    Vector3 direction = listenerPos - sourcePos;
    float distance = direction.Length();
    if (distance > ZeroTolerance)
        direction /= distance;
    bool directPathBlocked = Physics::RayCast(sourcePos, direction, hit, distance, Physics::LayerMasks[0], true);
    
    // Update occlusion effect
    OcclusionEffect* occlusionEffect = dynamic_cast<OcclusionEffect*>(effectChain->GetEffects()[0]);
    if (occlusionEffect)
    {
        const float occlusionLevel = directPathBlocked ? 0.8f : 0.0f;
        occlusionEffect->SetOcclusionLevel(occlusionLevel);
    }
    
    // Update reverb based on environment
    ReverbEffect* reverbEffect = dynamic_cast<ReverbEffect*>(effectChain->GetEffects()[1]);
    if (reverbEffect)
    {
        // For now, use a simple heuristic - larger rooms have more reverb
        const float roomSize = directPathBlocked ? 0.3f : 0.7f;
        reverbEffect->SetRoomSize(roomSize);
        
        // More reflections = longer decay
        float reflectionCount = 0.0f;
        for (int32 i = 0; i < _maxRays; i++)
        {
            reflectionCount += _rayPaths[i].Points.Count() - 1;
        }
        
        // Normalize and set decay
        reflectionCount /= (_maxRays * _maxReflections);
        const float decay = 0.3f + reflectionCount * 0.6f;
        reverbEffect->SetDecay(decay);
    }
    
    // Cast rays in different directions for sound reflections
    const int32 sqrtRays = (int32)Math::Sqrt((float)_maxRays);
    for (int32 i = 0; i < _maxRays; i++)
    {
        RayPath& path = _rayPaths[i];
        path.Points.Clear();
        path.MaterialIds.Clear();
        path.TotalDistance = 0.0f;
        path.Attenuation = 1.0f;
        
        // Use golden ratio spherical distribution for ray directions
        const float phi = PI * (3.0f - Math::Sqrt(5.0f)); // Golden angle
        const float y = 1.0f - (i / (float)(_maxRays - 1)) * 2.0f; // y goes from 1 to -1
        const float radius = Math::Sqrt(1.0f - y * y); // Radius at y
        const float theta = phi * i; // Golden angle increment
        
        Vector3 rayDir(
            Math::Cos(theta) * radius,
            y,
            Math::Sin(theta) * radius
        );
        
        // Transform direction to world space
        rayDir = source->GetOrientation() * rayDir;

        
        // Start ray path
        Vector3 rayOrigin = sourcePos;
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
            
            // Check if ray reached listener (approximate)
            const float distToListener = Vector3::Distance(hit.Point, listenerPos);
            if (distToListener < 100.0f) // Some threshold
            {
                // Ray reached listener after reflection
                break;
            }
            
            // Store material ID for acoustic properties
            //path.MaterialIds.Add(hit.ActorId);
            path.MaterialIds.Add(hit.Collider ? hit.Collider->GetID() : Guid::Empty);
            
            // Calculate reflection vector
            //rayDir = Vector3::Reflect(rayDir, hit.Normal);
            Vector3::Reflect(rayDir, hit.Normal, rayDir);

            rayOrigin = hit.Point + rayDir * 1.0f; // Offset to avoid self-intersection
        }
    }
}

void RayTracedAudioManager::Dispose()
{
    // Clean up effect chains
    for (auto& pair : _sourceEffectChains)
    {
        Delete(pair.Value);
    }
    _sourceEffectChains.Clear();
    _rayPaths.Clear();
}
