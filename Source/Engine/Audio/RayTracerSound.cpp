// Copyright (c) 2025 Robert Valentine. All rights reserved.

#include "RayTracerSound.h"
#include "Engine/Level/Actors/Camera.h"
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Level/Level.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Debug/DebugDraw.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Engine/Time.h"
#include "Engine/Core/Random.h"
#include "Engine/Audio/AudioDSPSystem.h"
#include "Engine/Audio/AudioDSPChain.h"
#include "Engine/Audio/AudioDSPEffect.h"
#include "Engine/Audio/AudioDSPReverb.h"
#include "Engine/Audio/AudioDSPLowPass.h"
#include "Engine/Audio/AudioDSPConvolution.h"
#include "Engine/Audio/Audio.h"

RayTracerSound::RayTracerSound(const SpawnParams& params)
    : Script(params)
{
    _tickUpdate = true;
}

void RayTracerSound::SoundVoxelGrid::Initialize(const Vector3& origin, const Vector3& size, const Int3& dimensions)
{
    Origin = origin;
    Size = size;
    Dimensions = dimensions;
    VoxelSize = Vector3(size.X / dimensions.X, size.Y / dimensions.Y, size.Z / dimensions.Z);
    
    // Allocate voxels
    int totalVoxels = dimensions.X * dimensions.Y * dimensions.Z;
    Voxels.Resize(totalVoxels);
    DistanceField.Resize(totalVoxels);
    
    // Initialize with default values
    for (int i = 0; i < totalVoxels; i++)
    {
        Voxels[i].IsSolid = false;
        Voxels[i].MaterialID = 0;
        Voxels[i].AverageReflectivity = 0.0f;
        Voxels[i].AverageAbsorption = 0.0f;
        Voxels[i].Occlusion = 0.0f;
        DistanceField[i] = 9999.0f;
    }
    
    LOG(Warning, "RayTracerSound: Voxel grid initialized with {0} voxels", totalVoxels);
}

Int3 RayTracerSound::SoundVoxelGrid::WorldToVoxel(const Vector3& worldPos) const
{
    Vector3 localPos = worldPos - Origin;
    return Int3(
        Math::Clamp(static_cast<int>(localPos.X / VoxelSize.X), 0, Dimensions.X - 1),
        Math::Clamp(static_cast<int>(localPos.Y / VoxelSize.Y), 0, Dimensions.Y - 1),
        Math::Clamp(static_cast<int>(localPos.Z / VoxelSize.Z), 0, Dimensions.Z - 1)
    );
}

int RayTracerSound::SoundVoxelGrid::VoxelCoordToIndex(const Int3& coord) const
{
    return coord.X + coord.Y * Dimensions.X + coord.Z * Dimensions.X * Dimensions.Y;
}

RayTracerSound::SoundVoxelGrid::Voxel& RayTracerSound::SoundVoxelGrid::GetVoxelAtWorld(const Vector3& worldPos)
{
    Int3 voxelCoord = WorldToVoxel(worldPos);
    int index = VoxelCoordToIndex(voxelCoord);
    return Voxels[index];
}

float RayTracerSound::SoundVoxelGrid::GetDistanceAtWorld(const Vector3& worldPos) const
{
    Int3 voxelCoord = WorldToVoxel(worldPos);
    int index = VoxelCoordToIndex(voxelCoord);
    return DistanceField[index];
}

bool RayTracerSound::SoundVoxelGrid::RaycastVoxels(const Vector3& origin, const Vector3& direction, float maxDistance, AudioRayHit& hit)
{
    Vector3 dirInv = Vector3(
        Math::Abs(direction.X) < 1e-6f ? 1e6f : 1.0f / direction.X,
        Math::Abs(direction.Y) < 1e-6f ? 1e6f : 1.0f / direction.Y,
        Math::Abs(direction.Z) < 1e-6f ? 1e6f : 1.0f / direction.Z
    );
    
    Int3 voxelPos = WorldToVoxel(origin);
    Int3 step(
        direction.X > 0 ? 1 : -1,
        direction.Y > 0 ? 1 : -1,
        direction.Z > 0 ? 1 : -1
    );
    
    Vector3 worldVoxelPos(
        Origin.X + voxelPos.X * VoxelSize.X,
        Origin.Y + voxelPos.Y * VoxelSize.Y,
        Origin.Z + voxelPos.Z * VoxelSize.Z
    );
    
    Vector3 nextBoundary(
        worldVoxelPos.X + (step.X > 0 ? VoxelSize.X : 0),
        worldVoxelPos.Y + (step.Y > 0 ? VoxelSize.Y : 0),
        worldVoxelPos.Z + (step.Z > 0 ? VoxelSize.Z : 0)
    );
    
    Vector3 tMax(
        (nextBoundary.X - origin.X) * dirInv.X,
        (nextBoundary.Y - origin.Y) * dirInv.Y,
        (nextBoundary.Z - origin.Z) * dirInv.Z
    );
    
    Vector3 tDelta(
        VoxelSize.X * Math::Abs(dirInv.X),
        VoxelSize.Y * Math::Abs(dirInv.Y),
        VoxelSize.Z * Math::Abs(dirInv.Z)
    );
    
    float distanceTraveled = 0.0f;
    
    // Fast voxel traversal loop
    while (distanceTraveled < maxDistance)
    {
        // Check current voxel
        int voxelIndex = VoxelCoordToIndex(voxelPos);
        
        if (voxelIndex >= 0 && voxelIndex < Voxels.Count())
        {
            const Voxel& voxel = Voxels[voxelIndex];
            
            if (voxel.IsSolid)
            {
                // We hit a solid voxel - calculate precise hit point
                Vector3 hitPoint = origin + direction * distanceTraveled;
                
                // Calculate normal using distance field gradients
                const float delta = VoxelSize.X * 0.1f;
                Vector3 normal(
                    GetDistanceAtWorld(hitPoint + Vector3(delta, 0, 0)) - 
                    GetDistanceAtWorld(hitPoint - Vector3(delta, 0, 0)),
                    
                    GetDistanceAtWorld(hitPoint + Vector3(0, delta, 0)) - 
                    GetDistanceAtWorld(hitPoint - Vector3(0, delta, 0)),
                    
                    GetDistanceAtWorld(hitPoint + Vector3(0, 0, delta)) - 
                    GetDistanceAtWorld(hitPoint - Vector3(0, 0, delta))
                );
                normal.Normalize();
                
                hit.HitPoint = hitPoint;
                hit.Normal = normal;
                hit.Distance = distanceTraveled;
                hit.MaterialID = voxel.MaterialID;
                hit.Intensity = 1.0f - (distanceTraveled / maxDistance);
                
                return true;
            }
        }
        
        // Move to next voxel
        if (tMax.X < tMax.Y && tMax.X < tMax.Z)
        {
            distanceTraveled = tMax.X;
            voxelPos.X += step.X;
            tMax.X += tDelta.X;
        }
        else if (tMax.Y < tMax.Z)
        {
            distanceTraveled = tMax.Y;
            voxelPos.Y += step.Y;
            tMax.Y += tDelta.Y;
        }
        else
        {
            distanceTraveled = tMax.Z;
            voxelPos.Z += step.Z;
            tMax.Z += tDelta.Z;
        }
        
        // Out of bounds check
        if (voxelPos.X < 0 || voxelPos.X >= Dimensions.X ||
            voxelPos.Y < 0 || voxelPos.Y >= Dimensions.Y ||
            voxelPos.Z < 0 || voxelPos.Z >= Dimensions.Z)
        {
            break;
        }
    }
    
    return false;
}

bool RayTracerSound::InitializeSDF()
{
    LOG(Warning, "RayTracerSound: InitializeSDF - Entering");
    
    if (SDFTexture)
    {
        LOG(Warning, "RayTracerSound: SDF Texture already exists");
        return true;
    }

    // Create texture description
    GPUTextureDescription desc = GPUTextureDescription::New3D(
        SDFSize, SDFSize, SDFSize,
        PixelFormat::R32_Float,
        GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess
    );
    
    LOG(Warning, "RayTracerSound: Created texture description: {0}x{1}x{2}", desc.Width, desc.Height, desc.Depth);

    // Create the texture
    SDFTexture = GPUTexture::New();
    if (!SDFTexture)
    {
        LOG(Error, "RayTracerSound: Failed to create GPU texture");
        return false;
    }
    
    // Initialize the texture
    bool result = SDFTexture->Init(desc);    
    LOG(Warning, "RayTracerSound: InitializeSDF result: {0}", String(result ? "Success" : "Failed"));
    
    // Init() returns true for failure, false for success
    return !result;
}

void RayTracerSound::GenerateSphereSDF()
{
    Array<float> sdfData;
    sdfData.Resize(SDFSize * SDFSize * SDFSize);

    Vector3 center = Vector3((float)SDFSize) * 0.5f;
    float radius = SDFSize * 0.3f;

    // Create a more complex shape - a sphere with a dent
    for (int z = 0; z < SDFSize; z++)
    for (int y = 0; y < SDFSize; y++)
    for (int x = 0; x < SDFSize; x++)
    {
        int idx = x + y * SDFSize + z * SDFSize * SDFSize;
        Vector3 p((float)x, (float)y, (float)z);
        
        // Basic sphere SDF
        float sphereDist = Vector3::Distance(p, center) - radius;
        
        // Create a dent or cavity in one side
        Vector3 dent(SDFSize * 0.8f, SDFSize * 0.5f, SDFSize * 0.5f);
        float denting = 10.0f; 
        float dent_radius = SDFSize * 0.2f;
        float dentDist = Vector3::Distance(p, dent) - dent_radius;
        
        // Combine the shapes using smooth min
        sdfData[idx] = Math::Min(sphereDist, dentDist);
    }

    // Calculate the row and slice pitch for the 3D texture
    uint32 rowPitch = SDFSize * sizeof(float);
    uint32 slicePitch = rowPitch * SDFSize;
    
    // Update the texture data using the GPUDevice singleton
    GPUDevice::Instance->GetMainContext()->UpdateTexture(
        SDFTexture,            // Target texture
        0,                     // Array index
        0,                     // Mip level
        sdfData.Get(),         // Source data
        rowPitch,              // Row pitch
        slicePitch             // Slice pitch
    );
    
    // Store the SDF data in memory for CPU access
    StoredSDFData = sdfData;
}

float RayTracerSound::SampleSDF(const Vector3& worldPos)
{
    if (!IsSceneSDFInitialized)
        return 9999.0f;
    
    // Convert world position to voxel space
    Vector3 localPos = (worldPos - SDFOrigin) / VoxelScale;
    
    Int3 grid = Int3(
        static_cast<int32>(Math::Floor(localPos.X)), 
        static_cast<int32>(Math::Floor(localPos.Y)), 
        static_cast<int32>(Math::Floor(localPos.Z))
    );
    
    // Check bounds
    if (grid.X < 0 || grid.Y < 0 || grid.Z < 0 ||
        grid.X >= AudioInfluenceExtent || 
        grid.Y >= AudioInfluenceExtent || 
        grid.Z >= AudioInfluenceExtent)
    {
        return 9999.0f;
    }
    
    // Calculate index in the SDF data
    int idx = grid.X + grid.Y * AudioInfluenceExtent + 
              grid.Z * AudioInfluenceExtent * AudioInfluenceExtent;
    
    return SceneSDFData[idx];
}

bool RayTracerSound::RayMarchSound(const Vector3& origin, const Vector3& direction, float maxDistance, float& hitDistance, Vector3& hitNormal)
{
    Vector3 pos = origin;
    float t = 0.0f;

    // Reduce iteration count for better performance
    for (int i = 0; i < 50 && t < maxDistance; ++i)
    {
        float dist = SampleSDF(pos);

        // Make it easier to get a hit - use a bigger threshold
        if (dist < 0.5f) // Reduced from 1.0f for more precise hits
        {
            // Calculate normal using central differences
            const float delta = 0.1f * VoxelScale;
            hitNormal = Vector3(
                SampleSDF(pos + Vector3(delta, 0, 0)) - SampleSDF(pos - Vector3(delta, 0, 0)),
                SampleSDF(pos + Vector3(0, delta, 0)) - SampleSDF(pos - Vector3(0, delta, 0)),
                SampleSDF(pos + Vector3(0, 0, delta)) - SampleSDF(pos - Vector3(0, 0, delta))
            );
            hitNormal.Normalize();

            hitDistance = t;
            return true;
        }

        // More efficient step size calculation
        float stepSize = Math::Max(dist * 0.8f, 0.1f); // Slightly more aggressive stepping
        t += stepSize;
        pos += direction * stepSize;
    }

    hitDistance = maxDistance;
    return false;
}

void RayTracerSound::UpdateAudioVoxels(const Vector3& playerPosition)
{
    // Center the SDF around the player
    SDFOrigin = playerPosition - Vector3((float)AudioInfluenceExtent * VoxelScale * 0.5f);
    int totalVoxels = AudioInfluenceExtent * AudioInfluenceExtent * AudioInfluenceExtent;

    // Resize the SDF data array if needed
    if (SceneSDFData.Count() != totalVoxels)
        SceneSDFData.Resize(totalVoxels);

    LOG(Warning, "RayTracerSound: Updating audio voxels around player at {0}", playerPosition.ToString());

    // For each voxel, calculate distance field
    for (int z = 0; z < AudioInfluenceExtent; z++)
        for (int y = 0; y < AudioInfluenceExtent; y++)
            for (int x = 0; x < AudioInfluenceExtent; x++)
            {
                int idx = x + y * AudioInfluenceExtent + z * AudioInfluenceExtent * AudioInfluenceExtent;

                // Convert voxel position to world space
                Vector3 voxelPos = SDFOrigin + Vector3((float)x, (float)y, (float)z) * VoxelScale;

                // Calculate SDF value (distance to nearest surface)
                // For demonstration, create a spherical room around the player
                float roomRadius = (float)AudioInfluenceExtent * VoxelScale * 0.4f;
                float distToCenter = Vector3::Distance(voxelPos, playerPosition);

                // Negative inside the room, positive outside
                SceneSDFData[idx] = distToCenter - roomRadius;

                // Also update the voxel grid for advanced simulation if enabled
                if (UseVoxelAcceleration && VoxelGrid.Voxels.Count() > 0)
                {
                    Int3 voxelCoord = VoxelGrid.WorldToVoxel(voxelPos);
                    if (voxelCoord.X >= 0 && voxelCoord.X < VoxelGrid.Dimensions.X &&
                        voxelCoord.Y >= 0 && voxelCoord.Y < VoxelGrid.Dimensions.Y &&
                        voxelCoord.Z >= 0 && voxelCoord.Z < VoxelGrid.Dimensions.Z)
                    {
                        int voxelIdx = VoxelGrid.VoxelCoordToIndex(voxelCoord);

                        // Make sure index is valid
                        if (voxelIdx >= 0 && voxelIdx < VoxelGrid.Voxels.Count())
                        {
                            VoxelGrid.DistanceField[voxelIdx] = SceneSDFData[idx];
                            VoxelGrid.Voxels[voxelIdx].IsSolid = SceneSDFData[idx] < 0.0f;

                            // Assign materials based on position
                            if (VoxelGrid.Voxels[voxelIdx].IsSolid)
                            {
                                float y_rel = (voxelPos.Y - SDFOrigin.Y) / (AudioInfluenceExtent * VoxelScale);

                                if (y_rel < 0.2f)
                                    VoxelGrid.Voxels[voxelIdx].MaterialID = 1; // Floor
                                else if (y_rel > 0.8f)
                                    VoxelGrid.Voxels[voxelIdx].MaterialID = 3; // Ceiling
                                else
                                    VoxelGrid.Voxels[voxelIdx].MaterialID = 2; // Walls

                                // Set material properties
                                auto it = MaterialLibrary.find(VoxelGrid.Voxels[voxelIdx].MaterialID);
                                if (it != MaterialLibrary.end())
                                {
                                    const AcousticMaterial& mat = it->second;
                                    VoxelGrid.Voxels[voxelIdx].AverageReflectivity = mat.Reflection;
                                    VoxelGrid.Voxels[voxelIdx].AverageAbsorption = mat.Absorption;
                                }
                            }
                        }
                    }
                }
            }

    IsSceneSDFInitialized = true;
    LOG(Warning, "RayTracerSound: Audio voxels updated");
}

void RayTracerSound::CastAudioRays(const Vector3& audioSource, const Vector3& listener)
{
    if (!IsSceneSDFInitialized)
    {
        LOG(Warning, "RayTracerSound: Cannot cast audio rays, SDF not initialized");
        return;
    }
    
    // Clear previous ray hits
    AudioRayHits.Clear();
    
    // Cast rays in various directions
    const int raysPerAxis = 8; // Number of rays per axis
    const float angleStep = 2.0f * PI / (float)raysPerAxis;
    
    LOG(Warning, "RayTracerSound: Casting audio rays from {0}", audioSource.ToString());
    
    // Cast rays in a sphere around the audio source
    for (int pitch = 0; pitch < raysPerAxis / 2; pitch++)
    {
        float pitchAngle = (pitch * angleStep) - (PI / 2.0f);
        
        for (int yaw = 0; yaw < raysPerAxis; yaw++)
        {
            float yawAngle = yaw * angleStep;
            
            // Calculate ray direction
            Vector3 direction;
            direction.X = Math::Cos(pitchAngle) * Math::Cos(yawAngle);
            direction.Y = Math::Sin(pitchAngle);
            direction.Z = Math::Cos(pitchAngle) * Math::Sin(yawAngle);
            
            // Raycast into the SDF
            float hitDistance = MaxAudioDistance;
            Vector3 hitNormal;
            
            if (RayMarchSound(audioSource, direction, MaxAudioDistance, hitDistance, hitNormal))
            {
                // We hit something, record the hit
                Vector3 hitPoint = audioSource + direction * hitDistance;
                
                // Calculate intensity based on distance and surface properties
                float intensity = 1.0f - (hitDistance / MaxAudioDistance);
                intensity *= ReflectionFactor; // Reduce based on reflection properties
                
                AudioRayHit hit;
                hit.HitPoint = hitPoint;
                hit.Normal = hitNormal;
                hit.Distance = hitDistance;
                hit.Intensity = intensity;
                hit.MaterialID = GetMaterialIDAtPoint(hitPoint);
                hit.Bounces = 0;
                
                AudioRayHits.Add(hit);
                
                // Debug visualization
                if (DrawDebugRays)
                {
                    // Ray color based on intensity (green to red)
                    Color rayColor = Color::Lerp(Color::Red, Color::Green, intensity);
                    DebugDraw::DrawLine(audioSource, hitPoint, rayColor, 0.0f, true);
                    
                    // Draw a small sphere at the hit point
                    BoundingSphere hitSphere = BoundingSphere(hitPoint, 1.0f);
                    DebugDraw::DrawSphere(hitSphere, rayColor, 0.0f, true);
                    
                    // Draw the normal at the hit point
                    DebugDraw::DrawLine(hitPoint, hitPoint + hitNormal * 5.0f, Color::Yellow, 0.0f, true);
                }
            }
        }
    }
    
    LOG(Warning, "RayTracerSound: Cast {0} audio rays", AudioRayHits.Count());
}

float RayTracerSound::GetAudioAttenuation(const Vector3& listenerPosition)
{
    if (AudioRayHits.Count() == 0)
        return 1.0f; // No attenuation if no rays
    
    // Find the closest ray hit to the listener
    float closestDistance = 9999.0f;
    float totalInfluence = 0.0f;
    
    for (const auto& hit : AudioRayHits)
    {
        float distToListener = Vector3::Distance(hit.HitPoint, listenerPosition);
        
        // Accumulate influence inversely proportional to distance
        if (distToListener > 0.1f) // Avoid division by zero
        {
            float influence = hit.Intensity / distToListener;
            totalInfluence += influence;
            
            if (distToListener < closestDistance)
                closestDistance = distToListener;
        }
    }
    
    // Calculate attenuation factor (0 = full attenuation, 1 = no attenuation)
    float attenuation = Math::Clamp(totalInfluence, 0.0f, 1.0f);
    
    LOG(Warning, "RayTracerSound: Audio attenuation at {0} = {1}", 
        listenerPosition.ToString(), attenuation);
    
    return attenuation;
}

void RayTracerSound::InitializeMaterials()
{
    // Set up default material library
    // 1: Concrete (floor)
    MaterialLibrary[1] = { AbsorptionFactor, ReflectionFactor, 0.3f, 0.0f, 120.0f, 2.0f };
    
    // 2: Wood/drywall (walls)
    MaterialLibrary[2] = { AbsorptionFactor * 0.7f, ReflectionFactor * 1.1f, DiffusionFactor * 0.8f, TransmissionFactor * 2.0f, 250.0f, 3.0f };
    
    // 3: Acoustic ceiling tiles
    MaterialLibrary[3] = { AbsorptionFactor * 1.5f, ReflectionFactor * 0.5f, DiffusionFactor * 1.2f, 0.0f, 80.0f, 1.5f };
    
    // 4: Glass
    MaterialLibrary[4] = { AbsorptionFactor * 0.2f, ReflectionFactor * 1.5f, DiffusionFactor * 0.1f, TransmissionFactor * 5.0f, 800.0f, 5.0f };
    
    // 5: Metal
    MaterialLibrary[5] = { AbsorptionFactor * 0.1f, ReflectionFactor * 1.8f, DiffusionFactor * 0.05f, 0.0f, 1200.0f, 8.0f };
    
    LOG(Warning, "RayTracerSound: Materials initialized");
}

int RayTracerSound::GetMaterialIDAtPoint(const Vector3& point)
{
    // In a real implementation, you'd query scene objects or a material map
    // For this example, we'll derive material based on position and SDF
    
    if (!IsSceneSDFInitialized)
        return 1; // Default to concrete
    
    // Get normalized position within the SDF
    Vector3 normalizedPos = (point - SDFOrigin) / ((float)AudioInfluenceExtent * VoxelScale);
    
    // Floor
    if (normalizedPos.Y < 0.2f)
        return 1; // Concrete
    
    // Ceiling
    if (normalizedPos.Y > 0.8f)
        return 3; // Acoustic ceiling
    
    // One wall is glass
    if (normalizedPos.X > 0.8f)
        return 4; // Glass
    
    // Another wall is metal
    if (normalizedPos.Z > 0.8f)
        return 5; // Metal
    
    // All other walls are wood/drywall
    return 2;
}

Vector3 RayTracerSound::AddRandomDeviation(const Vector3& direction, float amount)
{
    // Create random deviation
    float angle = PI * amount * Random::Rand();
    float theta = 2.0f * PI * Random::Rand();
    
    // Create local coordinate system
    Vector3 up = direction;
    Vector3 right = Vector3::Cross(up, Vector3(0, 1, 0));
    if (right.Length() < 0.001f)
        right = Vector3::Cross(up, Vector3(1, 0, 0));
    right.Normalize();
    Vector3 forward = Vector3::Cross(right, up);
    
    // Create deviation vector
    Vector3 deviation = 
        right * (Math::Sin(angle) * Math::Cos(theta)) +
        forward * (Math::Sin(angle) * Math::Sin(theta)) +
        up * Math::Cos(angle);
    
    return Vector3::Normalize(deviation);
}

Vector3 RayTracerSound::RandomHemisphereDirection(const Vector3& normal)
{
    float theta = 2.0f * PI * Random::Rand();
    float phi = Math::Acos(Math::Sqrt(Random::Rand())); // Cosine-weighted distribution
    
    // Create local coordinate system
    Vector3 up = normal;
    Vector3 right = Vector3::Cross(up, Vector3(0, 1, 0));
    if (right.Length() < 0.001f)
        right = Vector3::Cross(up, Vector3(1, 0, 0));
    right.Normalize();
    Vector3 forward = Vector3::Cross(right, up);
    
    // Create direction vector
    return Vector3::Normalize(
        right * (Math::Sin(phi) * Math::Cos(theta)) +
        forward * (Math::Sin(phi) * Math::Sin(theta)) +
        up * Math::Cos(phi)
    );
}

void RayTracerSound::TraceSoundRays(const Vector3& source, const Vector3& listener)
{
    // Clear previous impulse response
    ImpulseResponse.Amplitudes.Clear();
    ImpulseResponse.Delays.Clear();
    ImpulseResponse.Directions.Clear();
    ImpulseResponse.Frequencies.Clear();
    
    LOG(Warning, "RayTracerSound: Tracing sound rays from {0} to {1}", 
        source.ToString(), listener.ToString());
    
    // Check direct path first
    Vector3 directDir = listener - source;
    float directDist = directDir.Length();
    directDir.Normalize();
    
    float hitDistance;
    Vector3 hitNormal;
    bool directPathBlocked = RayMarchSound(source, directDir, directDist, hitDistance, hitNormal);
    
    if (!directPathBlocked)
    {
        // Direct path exists - add to impulse response
        float delay = directDist / SoundSpeed;
        float amplitude = 1.0f / (directDist * directDist);
        
        LOG(Warning, "RayTracerSound: Direct path exists, dist={0}, delay={1}ms, amp={2}",
            directDist, delay * 1000.0f, amplitude);
        
        RecordImpulseResponse(amplitude, delay, -directDir, 1000.0f, 0);
        
        // Draw direct path
        if (DrawDebugRays)
        {
            DebugDraw::DrawLine(source, listener, Color::Green, 0.0f, true);
        }
    }
    else
    {
        LOG(Warning, "RayTracerSound: Direct path blocked");
    }
    
    // Cast rays in all directions to find reflections
    const int numRays = Math::Min(RaysPerEmission, 512); // Limit for performance
    
    for (int i = 0; i < numRays; i++)
    {
        // Create a ray with uniform distribution on a sphere
        float theta = 2.0f * PI * Random::Rand();
        float phi = Math::Acos(2.0f * Random::Rand() - 1.0f);
        
        Vector3 direction(
            Math::Sin(phi) * Math::Cos(theta),
            Math::Sin(phi) * Math::Sin(theta),
            Math::Cos(phi)
        );
        
        SoundRay ray;
        ray.Origin = source;
        ray.Direction = direction;
        ray.Energy = 1.0f;
        ray.PathLength = 0.0f;
        ray.Bounces = 0;
        ray.Frequency = 1000.0f; // Default frequency
        ray.PathPoints.Add(source);
        
        // Trace this ray
        TraceRay(ray, listener);
    }
    
    LOG(Warning, "RayTracerSound: Generated impulse response with {0} paths", 
        ImpulseResponse.Amplitudes.Count());
    
    // Analyze the impulse response to extract acoustic parameters
    AnalyzeImpulseResponse();
}

bool RayTracerSound::TraceRay(SoundRay& ray, const Vector3& listener)
{
    const float listenerRadius = 2.0f; // Size of listener's "ear" for ray detection
    
    while (ray.Bounces < MaxBounces && ray.Energy > 0.01f)
    {
        // Try to use voxel raycast if available, otherwise use ray marching
        AudioRayHit hit;
        bool hitSomething = false;
        
        if (UseVoxelAcceleration && VoxelGrid.Voxels.Count() > 0)
        {
            hitSomething = VoxelGrid.RaycastVoxels(ray.Origin, ray.Direction, MaxAudioDistance, hit);
        }
        else
        {
            float hitDistance;
            Vector3 hitNormal;
            hitSomething = RayMarchSound(ray.Origin, ray.Direction, MaxAudioDistance, hitDistance, hitNormal);
            
            if (hitSomething)
            {
                hit.HitPoint = ray.Origin + ray.Direction * hitDistance;
                hit.Normal = hitNormal;
                hit.Distance = hitDistance;
                hit.MaterialID = GetMaterialIDAtPoint(hit.HitPoint);
                hit.Intensity = 1.0f - (hitDistance / MaxAudioDistance);
                hit.Bounces = ray.Bounces;
            }
        }
        
        if (hitSomething)
        {
            // Calculate distance to hit point
            float distanceToHit = hit.Distance;
            ray.PathLength += distanceToHit;
            ray.PathPoints.Add(hit.HitPoint);
            
            // Get material properties
            const AcousticMaterial& material = MaterialLibrary[hit.MaterialID];
            
            // Apply frequency-dependent material properties
            float freqAbsorption = CalculateFrequencyAbsorption(material, ray.Frequency);
            float freqTransmission = CalculateFrequencyTransmission(material, ray.Frequency);
            
            // Update ray energy based on material absorption
            ray.Energy *= (1.0f - freqAbsorption);
            
            // Draw ray path
            if (DrawDebugRays && ray.Bounces < 2) // Draw only a few bounces to avoid clutter
            {
                Color rayColor = Color::Lerp(Color::White, Color::Blue, (float)ray.Bounces / (float)MaxBounces);
                
                if (ray.PathPoints.Count() >= 2)
                {
                    int lastIdx = ray.PathPoints.Count() - 1;
                    DebugDraw::DrawLine(ray.PathPoints[lastIdx-1], ray.PathPoints[lastIdx], rayColor, 0.0f, true);
                }
            }
            
            // Check if we should transmit through the material
            if (freqTransmission > 0.0f && Random::Rand() < freqTransmission)
            {
                // Simple transmission - just continue through with reduced energy
                ray.Energy *= freqTransmission;
                ray.Origin = hit.HitPoint + ray.Direction * 0.1f; // Slight offset
                
                // Apply frequency-dependent muffling
                ray.Frequency *= MaterialGetMufflingFactor(material);
                
                LOG(Warning, "RayTracerSound: Ray transmitted through {0}, energy={1}, freq={2}",
                    hit.MaterialID, ray.Energy, ray.Frequency);
            }
            else
            {
                // Reflect the ray
                Vector3 reflectedDir;
                
                // Apply diffusion - mix between perfect reflection and random direction
                if (Random::Rand() < material.Diffusion)
                {
                    // Random direction in hemisphere oriented around the normal
                    reflectedDir = RandomHemisphereDirection(hit.Normal);
                    
                    LOG(Warning, "RayTracerSound: Diffuse reflection from {0}", hit.MaterialID);
                }
                else
                {
                    // Perfect reflection
                    Vector3::Reflect(ray.Direction, hit.Normal, reflectedDir);
                    
                    LOG(Warning, "RayTracerSound: Specular reflection from {0}", hit.MaterialID);
                }
                
                // Apply resonance effect (frequency-dependent)
                HandleMaterialResonance(material, ray);
                
                ray.Direction = reflectedDir;
                ray.Origin = hit.HitPoint + reflectedDir * 0.1f; // Slight offset
                ray.Bounces++;
            }
            
            // Check if this ray passes near the listener after this bounce
            Vector3 toListener = listener - ray.Origin;
            float distanceToListener = toListener.Length();
            
            if (distanceToListener < listenerRadius)
            {
                // Ray reached the listener, record its contribution
                float delay = ray.PathLength / SoundSpeed;
                // Inverse square law with path length
                float amplitude = ray.Energy / (ray.PathLength * ray.PathLength);
                
                // Apply frequency-dependent air attenuation
                amplitude *= CalculateAirAttenuation(ray.PathLength, ray.Frequency);
                
                RecordImpulseResponse(amplitude, delay, Vector3::Normalize(-ray.Direction), 
                                    ray.Frequency, ray.Bounces);
                
                LOG(Warning, "RayTracerSound: Ray reached listener, delay={0}ms, amp={1}, bounces={2}",
                    delay * 1000.0f, amplitude, ray.Bounces);
                
                return true; // Ray completed
            }
        }
        else
        {
            // Ray didn't hit anything, stop tracing
            LOG(Warning, "RayTracerSound: Ray escaped scene bounds");
            break;
        }
    }
    
    return false;
}

float RayTracerSound::CalculateFrequencyAbsorption(const AcousticMaterial& material, float frequency)
{
    // Higher frequencies are absorbed more
    float freqFactor = Math::Clamp(frequency / 1000.0f, 0.5f, 2.0f);
    return Math::Clamp(material.Absorption * freqFactor, 0.0f, 1.0f);
}

float RayTracerSound::CalculateFrequencyTransmission(const AcousticMaterial& material, float frequency)
{
    // Lower frequencies pass through materials better
    float freqFactor = Math::Clamp(1000.0f / frequency, 0.5f, 2.0f);
    return Math::Clamp(material.Transmission * freqFactor, 0.0f, 1.0f);
}

float RayTracerSound::MaterialGetMufflingFactor(const AcousticMaterial& material)
{
    // Muffling reduces high frequencies (lowers the frequency)
    return Math::Lerp(0.9f, 0.5f, material.Absorption);
}

void RayTracerSound::HandleMaterialResonance(const AcousticMaterial& material, SoundRay& ray)
{
    // Calculate how close the ray's frequency is to resonant frequency
    float freqRatio = ray.Frequency / material.ResonanceFreq;
    
    // Resonance occurs near the resonant frequency
    if (freqRatio > 0.9f && freqRatio < 1.1f)
    {
        // Q factor determines how sharp the resonance is
        float resonanceFactor = 1.0f + (material.ResonanceQ / 10.0f) * (1.0f - Math::Abs(freqRatio - 1.0f));
        
        // Boost energy at resonant frequency
        ray.Energy *= resonanceFactor;
        
        // Shift frequency toward resonant frequency
        ray.Frequency = Math::Lerp(ray.Frequency, material.ResonanceFreq, 0.2f);
        
        LOG(Warning, "RayTracerSound: Resonance effect at freq={0}, boost={1}",
            ray.Frequency, resonanceFactor);
    }
}

float RayTracerSound::CalculateAirAttenuation(float distance, float frequency)
{
    // Simplified model: high frequencies attenuate more with distance
    float attenuation = 1.0f;
    
    // Only apply significant attenuation for longer distances
    if (distance > 10.0f)
    {
        float freqFactor = Math::Clamp(frequency / 1000.0f, 1.0f, 10.0f);
        float distanceFactor = (distance - 10.0f) / 100.0f;
        attenuation = Math::Exp(-distanceFactor * freqFactor * 0.1f);
    }
    
    return attenuation;
}

void RayTracerSound::RecordImpulseResponse(float amplitude, float delay, const Vector3& direction, 
                                          float frequency, int bounces)
{
    // Add this sample to our impulse response
    ImpulseResponse.Amplitudes.Add(amplitude);
    ImpulseResponse.Delays.Add(delay);
    ImpulseResponse.Directions.Add(direction);
    ImpulseResponse.Frequencies.Add(frequency);
}

void RayTracerSound::AnalyzeImpulseResponse()
{
    if (ImpulseResponse.Delays.IsEmpty())
    {
        LOG(Warning, "RayTracerSound: No impulse response to analyze");
        return;
    }
    
    // Sort impulse response by delay time
    Array<int> sortedIndices;
    sortedIndices.Resize(ImpulseResponse.Delays.Count());
    
    for (int i = 0; i < sortedIndices.Count(); i++)
        sortedIndices[i] = i;
    
    // Simple bubble sort (not efficient but ok for small arrays)
    for (int i = 0; i < sortedIndices.Count(); i++)
    {
        for (int j = i + 1; j < sortedIndices.Count(); j++)
        {
            if (ImpulseResponse.Delays[sortedIndices[i]] > ImpulseResponse.Delays[sortedIndices[j]])
            {
                int temp = sortedIndices[i];
                sortedIndices[i] = sortedIndices[j];
                sortedIndices[j] = temp;
            }
        }
    }
    
    // Separate early reflections from late reflections
    float earlyReflectionTime = 0.1f; // 100ms
    
    float directEnergy = 0.0f;
    float earlyEnergy = 0.0f;
    float lateEnergy = 0.0f;
    
    for (int i = 0; i < sortedIndices.Count(); i++)
    {
        int idx = sortedIndices[i];
        float delay = ImpulseResponse.Delays[idx];
        float amplitude = ImpulseResponse.Amplitudes[idx];
        
        if (i == 0)
            directEnergy = amplitude;
        else if (delay < earlyReflectionTime)
            earlyEnergy += amplitude;
        else
            lateEnergy += amplitude;
    }
    
    // Calculate reverb time (RT60) - simplified
    float totalEnergy = directEnergy + earlyEnergy + lateEnergy;
    float reverbTime = 0.0f;
    float energySoFar = 0.0f;
    
    // Find the time when energy reaches 99.9% of total (RT60 approximation)
    for (int i = 0; i < sortedIndices.Count(); i++)
    {
        int idx = sortedIndices[i];
        energySoFar += ImpulseResponse.Amplitudes[idx];
        
        if (energySoFar >= totalEnergy * 0.999f)
        {
            reverbTime = ImpulseResponse.Delays[idx];
            break;
        }
    }
    
    // Calculate clarity (C50) - ratio of energy in first 50ms to energy after 50ms
    float energy50ms = 0.0f;
    float energyAfter50ms = 0.0f;
    
    for (int i = 0; i < sortedIndices.Count(); i++)
    {
        int idx = sortedIndices[i];
        if (ImpulseResponse.Delays[idx] < 0.05f)
            energy50ms += ImpulseResponse.Amplitudes[idx];
        else
            energyAfter50ms += ImpulseResponse.Amplitudes[idx];
    }
    
    float clarity = energyAfter50ms > 0.0f ? 
                    10.0f * Math::Log10(energy50ms / energyAfter50ms) : 30.0f;
    
    // Calculate echo density
    float maxDelay = ImpulseResponse.Delays[sortedIndices.Last()];
    float echoDensity = maxDelay > 0.0f ? (float)ImpulseResponse.Delays.Count() / maxDelay : 0.0f;
    
    // Output analysis results
    LOG(Warning, "RayTracerSound: Impulse Response Analysis:");
    LOG(Warning, "  Direct Energy: {0}", directEnergy);
    LOG(Warning, "  Early Reflection Energy: {0}", earlyEnergy);
    LOG(Warning, "  Late Reflection Energy: {0}", lateEnergy);
    LOG(Warning, "  Reverb Time (RT60): {0} seconds", reverbTime);
    LOG(Warning, "  Clarity (C50): {0} dB", clarity);
    LOG(Warning, "  Echo Density: {0} reflections/second", echoDensity);
    
    // Update DSP parameters based on acoustic simulation
    if (ModifyAudio && SourceAudio)
    {
        // Calculate reverb parameters
        ReverbAmount = Math::Clamp(lateEnergy / totalEnergy * 2.0f, 0.0f, 1.0f);
        ReverbRoomSize = Math::Clamp((earlyEnergy + lateEnergy) / totalEnergy, 0.0f, 1.0f);
        ReverbDamping = 0.5f;
        
        // Calculate direct path occlusion
        float occlusion = 0.0f;

        auto cam = Camera::GetMainCamera();
        if (!cam)
            return;
        Vector3 playerPosition = cam->GetPosition();

        Vector3 listenerPos = Listener ? Listener->GetPosition() : playerPosition;
        Vector3 sourcePos = SoundSource ? SoundSource->GetPosition() :
            playerPosition + Vector3(50.0f, 0.0f, 50.0f);
        Vector3 dirToListener = listenerPos - sourcePos;

        float distToListener = dirToListener.Length();
        
        if (distToListener > 0.1f)
        {
            dirToListener.Normalize();
            float hitDistance;
            Vector3 hitNormal;
            if (RayMarchSound(sourcePos, dirToListener, distToListener, hitDistance, hitNormal))
            {
                // Direct path is occluded
                occlusion = Math::Clamp(1.0f - (hitDistance / distToListener), 0.0f, 1.0f);
            }
        }
        
        // Apply occlusion as low-pass filter
        LowPassAmount = Math::Clamp(occlusion * 0.8f, 0.0f, 1.0f);
        LowPassFrequency = Math::Lerp(5000.0f, 500.0f, occlusion);
        
        // Update DSP effects
        UpdateAudioDSP();
    }
    
    // Visualize the impulse response
    if (DrawImpulseResponse)
    {
        VisualizeImpulseResponse();
    }
}

void RayTracerSound::VisualizeImpulseResponse()
{
    if (ImpulseResponse.Delays.IsEmpty())
        return;
        
    // Find an appropriate visualization position (in front of camera)
    auto cam = Camera::GetMainCamera();
    if (!cam)
        return;
        
    Vector3 visPos = cam->GetPosition() + cam->GetDirection() * 50.0f;
    
    // Draw impulse response as a series of colored spheres
    // Size represents amplitude, color represents frequency
    const float maxSphereSize = 5.0f;
    const float spacing = 10.0f;
    
    for (int i = 0; i < ImpulseResponse.Delays.Count(); i++)
    {
        float delay = ImpulseResponse.Delays[i];
        float amplitude = ImpulseResponse.Amplitudes[i];
        float frequency = ImpulseResponse.Frequencies[i];
        
        // Position based on delay
        Vector3 spherePos = visPos + Vector3(delay * spacing, 0, 0);
        
        // Size based on amplitude
        float size = Math::Sqrt(amplitude) * maxSphereSize;
        
        // Color based on frequency (blue to red)
        float normalizedFreq = Math::Clamp(frequency / 2000.0f, 0.0f, 1.0f);
        Color sphereColor = Color::Lerp(Color::Blue, Color::Red, normalizedFreq);
        
        // Draw sphere
        BoundingSphere sphere = BoundingSphere(spherePos, size);
        DebugDraw::DrawSphere(sphere, sphereColor, 0.0f, true);
        
        // Draw direction vector
        Vector3 direction = ImpulseResponse.Directions[i];
        DebugDraw::DrawLine(spherePos, spherePos + direction * 5.0f, sphereColor, 0.0f, true);
    }
}

// Audio DSP Integration Methods
void RayTracerSound::SetupAudioDSP()
{
    if (!SourceAudio)
    {
        LOG(Error, "RayTracerSound: SourceAudio is null, cannot set up DSP effects");
        return;
    }

    if (!ModifyAudio)
    {
        LOG(Error, "RayTracerSound: ModifyAudio is disabled, cannot set up DSP effects");
        return;
    }

    LOG(Warning, "RayTracerSound: Setting up DSP for {0}", SourceAudio->GetNamePath());

    // Get the DSP chain from the source
    _dspChain = SourceAudio->GetDSPChain();
    if (!_dspChain)
    {
        LOG(Error, "RayTracerSound: Failed to get DSP chain for source {0}", SourceAudio->GetNamePath());
        return;
    }

    // Initialize effects
    LOG(Warning, "RayTracerSound: Clearing audio effects");
    SourceAudio->ClearAudioEffects();

    // Set initial parameters
    LOG(Warning, "RayTracerSound: Setting initial DSP parameters");
    SourceAudio->SetReverbEffect(ReverbAmount, ReverbRoomSize, ReverbDamping);
    SourceAudio->SetLowPassFilter(LowPassFrequency, LowPassAmount > 0.01f);

    // Create test impulse response if none exists
    if (ImpulseResponse.Amplitudes.IsEmpty())
    {
        LOG(Warning, "RayTracerSound: Creating test impulse response");
        // Create a simple test impulse
        ImpulseResponse.Amplitudes.Add(1.0f);
        ImpulseResponse.Delays.Add(0.0f);
        ImpulseResponse.Directions.Add(Vector3::Forward);
        ImpulseResponse.Frequencies.Add(1000.0f);

        // Add some reflections
        for (int i = 1; i < 5; i++)
        {
            ImpulseResponse.Amplitudes.Add(0.5f / i);
            ImpulseResponse.Delays.Add(0.1f * i);
            ImpulseResponse.Directions.Add(Vector3(0, i % 2 == 0 ? 1 : -1, 0));
            ImpulseResponse.Frequencies.Add(1000.0f / i);
        }

        CreateConvolutionFromImpulse();
    }

    LOG(Warning, "RayTracerSound: DSP setup complete");
}

void RayTracerSound::UpdateAudioDSP()
{
    if (!SourceAudio || !ModifyAudio)
        return;
        
    // Update reverb effect
    SourceAudio->SetReverbEffect(ReverbAmount, ReverbRoomSize, ReverbDamping);
    
    // Update low-pass filter
    SourceAudio->SetLowPassFilter(LowPassFrequency, LowPassAmount > 0.01f);
    
    // Update convolution effect
    if (ImpulseResponse.Amplitudes.Count() > 0 && ReverbAmount > 0.1f)
    {
        // Create convolution impulse response
        Array<float> ir = ConvertImpulseResponseToSamples();
        SourceAudio->SetConvolutionImpulseResponse(ir, ReverbAmount * 0.5f);
    }
}

Array<float> RayTracerSound::ConvertImpulseResponseToSamples(int32 sampleRate)
{
    // Convert the impulse response to audio samples
    const int maxSamples = 8192; // Limit for performance
    Array<float> ir;
    ir.Resize(maxSamples);

    // Clear the buffer
    for (int i = 0; i < maxSamples; i++)
        ir[i] = 0.0f;

    // Early exit if no impulse data
    if (ImpulseResponse.Delays.IsEmpty())
    {
        LOG(Warning, "RayTracerSound: No impulse response data to convert");
        return ir;
    }

    // Process impulse points - directly apply to samples
    for (int i = 0; i < ImpulseResponse.Delays.Count(); i++)
    {
        float delay = ImpulseResponse.Delays[i];
        float amplitude = ImpulseResponse.Amplitudes[i];

        int sampleIndex = (int)(delay * sampleRate);
        if (sampleIndex < maxSamples)
        {
            ir[sampleIndex] += amplitude;

            // Add some spreading for smoother response (convolution kernel)
            const int spread = 4;
            for (int j = 1; j <= spread; j++)
            {
                int idx1 = sampleIndex + j;
                int idx2 = sampleIndex - j;

                if (idx1 < maxSamples)
                    ir[idx1] += amplitude * (1.0f - (float)j / (float)(spread + 1));

                if (idx2 >= 0)
                    ir[idx2] += amplitude * (1.0f - (float)j / (float)(spread + 1));
            }
        }
    }

    // Apply exponential decay envelope
    float maxDelay = 0.0f;
    for (int i = 0; i < ImpulseResponse.Delays.Count(); i++)
        maxDelay = Math::Max(maxDelay, ImpulseResponse.Delays[i]);

    float decayFactor = 6.0f / (maxDelay * sampleRate);
    for (int i = 0; i < maxSamples; i++)
        ir[i] *= Math::Exp(-i * decayFactor);

    // Normalize the impulse response
    float maxAmplitude = 0.0f;
    for (int i = 0; i < maxSamples; i++)
        maxAmplitude = Math::Max(maxAmplitude, Math::Abs(ir[i]));

    if (maxAmplitude > 0.001f)
    {
        for (int i = 0; i < maxSamples; i++)
            ir[i] /= maxAmplitude;
    }

    return ir;
}

void RayTracerSound::CreateConvolutionFromImpulse()
{
    if (!SourceAudio || !ModifyAudio)
        return;
        
    // Convert impulse response to audio samples
    Array<float> ir = ConvertImpulseResponseToSamples();
    
    // Set impulse response through the AudioSource interface
    SourceAudio->SetConvolutionImpulseResponse(ir, ReverbAmount * 0.5f);
    
    LOG(Warning, "RayTracerSound: Created convolution impulse response with {0} samples", ir.Count());
}

void RayTracerSound::OnStart()
{
    LOG(Warning, "RayTracerSound: OnStart called");

    // Initialize acoustic materials first
    InitializeMaterials();

    // Initialize the SDF
    bool initResult = InitializeSDF();
    if (initResult)
    {
        LOG(Warning, "RayTracerSound: SDF initialized successfully");
        GenerateSphereSDF();
    }
    else
    {
        LOG(Error, "RayTracerSound: Failed to initialize SDF texture, falling back to CPU-based SDF");
        // We can still continue with CPU-based SDF
    }

    // Initialize voxel grid for advanced simulation
    if (UseVoxelAcceleration)
    {
        Vector3 sceneMin = Vector3::Zero - Vector3(AudioInfluenceExtent * VoxelScale * 0.5f);
        Vector3 sceneSize = Vector3(AudioInfluenceExtent * VoxelScale);
        Int3 gridDimensions(AudioInfluenceExtent, AudioInfluenceExtent, AudioInfluenceExtent);

        VoxelGrid.Initialize(sceneMin, sceneSize, gridDimensions);
        LOG(Warning, "RayTracerSound: Voxel grid initialized");
    }

    // Set up audio DSP if available
    if (SourceAudio && ModifyAudio)
    {
        LOG(Warning, "RayTracerSound: Setting up audio DSP");
        SetupAudioDSP();
    }
    else if (ModifyAudio)
    {
        LOG(Warning, "RayTracerSound: No audio source assigned, DSP effects will not be applied");
    }

    bool isSystemReady = AudioDSPSystem::GetSourceDSP(SourceAudio) != nullptr;
    LOG(Warning, "RayTracerSound: AudioDSPSystem ready status: {0}", isSystemReady);

    TestAudioEffects();

    // Create an initial spherical environment
    auto cam = Camera::GetMainCamera();
    if (cam)
    {
        Vector3 playerPosition = cam->GetPosition();
        UpdateAudioVoxels(playerPosition);
    }

}

void RayTracerSound::OnUpdate()
{
    // Get main camera as listener position
    auto cam = Camera::GetMainCamera();
    if (!cam)
        return;
    
    Vector3 playerPosition = cam->GetPosition();
    
    // Update audio voxels periodically
    static float updateTimer = 0.0f;
    updateTimer += Time::GetDeltaTime();
    
    if (updateTimer > 1.0f || !IsSceneSDFInitialized)
    {
        UpdateAudioVoxels(playerPosition);
        updateTimer = 0.0f;
    }
    
    // Place the audio source at a fixed position if not set
    Vector3 audioSource = SoundSource ? SoundSource->GetPosition() : 
                          playerPosition + Vector3(50.0f, 0.0f, 50.0f);
    
    // Use the player position as listener if not set
    Vector3 listenerPos = Listener ? Listener->GetPosition() : playerPosition;
    
    // Draw audio source
    BoundingSphere sourceSphere = BoundingSphere(audioSource, 5.0f);
    DebugDraw::DrawSphere(sourceSphere, Color::Yellow, 0.0f, true);
    
    // Draw listener position
    BoundingSphere listenerSphere = BoundingSphere(listenerPos, 2.0f);
    DebugDraw::DrawSphere(listenerSphere, Color::Green, 0.0f, true);
    
    // Basic ray casting for visualization
    CastAudioRays(audioSource, listenerPos);
    
    // Calculate audio attenuation for the player
    float attenuation = GetAudioAttenuation(listenerPos);
    
    // Advanced sound ray tracing
    static float rayTraceTimer = 0.0f;
    rayTraceTimer += Time::GetDeltaTime();
    
    if (rayTraceTimer > 1.0f)
    {
        TraceSoundRays(audioSource, listenerPos);
        
        // Update audio DSP effects based on impulse response
        if (ModifyAudio && SourceAudio)
        {
            UpdateAudioDSP();
        }
        
        rayTraceTimer = 0.0f;
    }
    
    // Visualize the SDF volume bounds
    if (DrawDebugVoxels)
    {
        Vector3 volumeMax = SDFOrigin + Vector3((float)AudioInfluenceExtent * VoxelScale);
        Vector3 volumeCenter = (SDFOrigin + volumeMax) * 0.5f;
        Vector3 volumeSize = volumeMax - SDFOrigin;
        
        BoundingBox debugBox = BoundingBox(volumeCenter, volumeSize);
        DebugDraw::DrawWireBox(debugBox, Color::Cyan, 0.0f, true);
    }
}

void RayTracerSound::TestAudioEffects()
{
    if (!SourceAudio)
        return;

    // Try setting extreme values that would be obviously audible
    ReverbAmount = 0.9f;
    ReverbRoomSize = 0.9f;
    ReverbDamping = 0.2f;
    LowPassAmount = 0.8f;
    LowPassFrequency = 500.0f;

    LOG(Warning, "RayTracerSound: Setting test audio effects with extreme values");
    SourceAudio->SetReverbEffect(ReverbAmount, ReverbRoomSize, ReverbDamping);
    SourceAudio->SetLowPassFilter(LowPassFrequency, true);

    // Create a simple test impulse response
    Array<float> testIR;
    testIR.Resize(2048);
    for (int i = 0; i < 2048; i++)
    {
        if (i % 200 < 20) // Create some spikes
            testIR[i] = 0.8f * Math::Exp(-0.001f * i);
        else
            testIR[i] = 0.0f;
    }

    SourceAudio->SetConvolutionImpulseResponse(testIR, 0.7f);

    LOG(Warning, "RayTracerSound: Test audio effects applied");
}

void RayTracerSound::DiagnoseDSPSystem()
{
    if (!SourceAudio)
    {
        LOG(Error, "RayTracerSound: No audio source for diagnosis");
        return;
    }

    LOG(Warning, "RayTracerSound: DSP System Diagnosis");
    LOG(Warning, "  AudioSource: {0}", SourceAudio->GetNamePath());
    LOG(Warning, "  Source ID: {0}", SourceAudio->SourceID);
    LOG(Warning, "  Is Playing: {0}", SourceAudio->GetState() == AudioSource::States::Playing);
    LOG(Warning, "  Is Actually Playing: {0}", SourceAudio->IsActuallyPlaying());
    LOG(Warning, "  Has Clip: {0}", SourceAudio->Clip != nullptr);

    // Check DSP chain
    AudioDSPChain* chain = SourceAudio->GetDSPChain();
    LOG(Warning, "  Has DSP Chain: {0}", chain != nullptr);

    if (chain)
    {
        LOG(Warning, "  Chain Enabled: {0}", chain->IsEnabled());

        // Get all effects in the chain
        Array<AudioDSPEffect*> effects = chain->GetEffects();
        LOG(Warning, "  Effects Count: {0}", effects.Count());

        for (int i = 0; i < effects.Count(); i++)
        {
            AudioDSPEffect* effect = effects[i];
            LOG(Warning, "    Effect {0}: Type {1}, Enabled {2}",
                i, (int)effect->GetType(), effect->IsEnabled());
        }
    }
}
