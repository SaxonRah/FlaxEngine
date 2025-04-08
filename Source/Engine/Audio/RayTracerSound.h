// Copyright (c) 2025 Robert Valentine. All rights reserved.

#pragma once

#include "Engine/Scripting/Script.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Array.h"
// Updated includes for standalone classes
#include "Engine/Audio/AudioDSPSystem.h"
#include "Engine/Audio/AudioDSPChain.h"
#include "Engine/Audio/AudioDSPEffect.h"
#include "Engine/Audio/AudioDSPReverb.h"
#include "Engine/Audio/AudioDSPLowPass.h"
#include "Engine/Audio/AudioDSPConvolution.h"
#include "Engine/Audio/AudioSource.h"
#include <unordered_map>
#include <vector>

API_CLASS() class RayTracerSound : public Script
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE(RayTracerSound);

public:
    // SDF parameters
    API_FIELD() int32 SDFSize = 64;
    API_FIELD() float VoxelScale = 10.0f;

    // Ray tracer parameters
    API_FIELD() int32 RaysPerEmission = 256;
    API_FIELD() int32 MaxBounces = 8;
    API_FIELD() float MaxAudioDistance = 500.0f;
    API_FIELD() float SoundSpeed = 343.0f; // m/s in air at room temperature

    // Sound source properties
    API_FIELD() Actor* SoundSource = nullptr;
    API_FIELD() Actor* Listener = nullptr;

    // Audio influence parameters
    API_FIELD() int32 AudioInfluenceExtent = 32;

    // Visualization options
    API_FIELD() bool DrawDebugVoxels = false;
    API_FIELD() bool DrawDebugRays = true;
    API_FIELD() bool DrawImpulseResponse = false;

    // Audio properties
    API_FIELD() float ReflectionFactor = 0.7f;
    API_FIELD() float AbsorptionFactor = 0.2f;
    API_FIELD() float DiffusionFactor = 0.3f;
    API_FIELD() float TransmissionFactor = 0.1f;

    // Audio DSP properties
    API_FIELD(Attributes = "EditorOrder(500), EditorDisplay(\"Audio\")")
    AudioSource* SourceAudio = nullptr;

    API_FIELD(Attributes = "EditorOrder(510), DefaultValue(true), EditorDisplay(\"Audio\")")
    bool ModifyAudio = true;

    API_FIELD(Attributes = "EditorOrder(520), DefaultValue(0.5f), Limit(0, 1, 0.01f), EditorDisplay(\"Audio\")")
    float ReverbAmount = 0.5f;

    API_FIELD(Attributes = "EditorOrder(530), DefaultValue(0.5f), Limit(0, 1, 0.01f), EditorDisplay(\"Audio\")")
    float ReverbRoomSize = 0.5f;

    API_FIELD(Attributes = "EditorOrder(540), DefaultValue(0.5f), Limit(0, 1, 0.01f), EditorDisplay(\"Audio\")")
    float ReverbDamping = 0.5f;

    API_FIELD(Attributes = "EditorOrder(550), DefaultValue(0.0f), Limit(0, 1, 0.01f), EditorDisplay(\"Audio\")")
    float LowPassAmount = 0.0f;

    API_FIELD(Attributes = "EditorOrder(560), DefaultValue(1000.0f), Limit(20, 20000, 100.0f), EditorDisplay(\"Audio\")")
    float LowPassFrequency = 1000.0f;

    // References to DSP effects
    AudioDSPChain* _dspChain = nullptr;
    AudioDSPReverb* _reverbEffect = nullptr;
    AudioDSPLowPass* _lowPassEffect = nullptr;
    AudioDSPConvolution* _convolutionEffect = nullptr;

    // SDF functions
    GPUTexture* SDFTexture = nullptr;
    bool InitializeSDF();
    float SampleSDF(const Vector3& worldPos);
    bool RayMarchSound(const Vector3& origin, const Vector3& direction, float maxDistance, float& hitDistance, Vector3& hitNormal);
    void GenerateSphereSDF();

    // Audio functions
    void UpdateAudioVoxels(const Vector3& playerPosition);
    void CastAudioRays(const Vector3& audioSource, const Vector3& listener);
    float GetAudioAttenuation(const Vector3& listenerPosition);

    // New advanced audio functions
    void TraceSoundRays(const Vector3& source, const Vector3& listener);
    void AnalyzeImpulseResponse();
    void VisualizeImpulseResponse();

    // Audio DSP integration
    void SetupAudioDSP();
    void UpdateAudioDSP();
    void CreateConvolutionFromImpulse();
    Array<float> ConvertImpulseResponseToSamples(int32 sampleRate = 48000);

    // Test Audio Effects
    void TestAudioEffects();
    void DiagnoseDSPSystem();

private:
    // SDF data
    Array<float> StoredSDFData;
    Array<float> SceneSDFData;
    Vector3 SDFOrigin;
    bool IsSceneSDFInitialized = false;

    // Audio rays data
    struct AudioRayHit {
        Vector3 HitPoint;
        Vector3 Normal;
        float Distance;
        float Intensity;
        int MaterialID;
        int Bounces;
    };
    Array<AudioRayHit> AudioRayHits;

    // Acoustic material properties
    struct AcousticMaterial
    {
        float Absorption;     // 0.0 to 1.0, how much sound energy is absorbed
        float Reflection;     // 0.0 to 1.0, how much is reflected
        float Diffusion;      // 0.0 to 1.0, how much the reflection is scattered
        float Transmission;   // 0.0 to 1.0, how much passes through
        float ResonanceFreq;  // Hz, resonant frequency of the material
        float ResonanceQ;     // Q factor for resonance
    };
    std::unordered_map<int, AcousticMaterial> MaterialLibrary;

    // Sound ray structure
    struct SoundRay
    {
        Vector3 Origin;
        Vector3 Direction;
        float Energy;
        float PathLength;
        int Bounces;
        float Frequency;
        Array<Vector3> PathPoints;
    };

    // Impulse response data
    struct SoundImpulseResponse
    {
        Array<float> Amplitudes;
        Array<float> Delays;
        Array<Vector3> Directions;
        Array<float> Frequencies;
    } ImpulseResponse;

    // Voxel-based acceleration
    struct SoundVoxelGrid
    {
        Vector3 Origin;            // World-space origin of the grid
        Vector3 Size;              // Total size of the grid
        Int3 Dimensions;          // Number of voxels in each dimension
        Vector3 VoxelSize;         // Size of each voxel

        // Each voxel stores:
        struct Voxel
        {
            int MaterialID;           // Material properties
            float AverageReflectivity; // Pre-computed reflectivity
            float AverageAbsorption;   // Pre-computed absorption
            float Occlusion;           // Occlusion factor (0-1)
            bool IsSolid;              // Is this voxel solid or air
        };

        Array<Voxel> Voxels;
        Array<float> DistanceField;   // Signed distance at each voxel

        void Initialize(const Vector3& origin, const Vector3& size, const Int3& dimensions);
        Int3 WorldToVoxel(const Vector3& worldPos) const;
        int VoxelCoordToIndex(const Int3& coord) const;
        Voxel& GetVoxelAtWorld(const Vector3& worldPos);
        float GetDistanceAtWorld(const Vector3& worldPos) const;
        bool RaycastVoxels(const Vector3& origin, const Vector3& direction, float maxDistance, AudioRayHit& hit);
    };

    SoundVoxelGrid VoxelGrid;
    bool UseVoxelAcceleration = true;

    // Helper functions
    void InitializeMaterials();
    int GetMaterialIDAtPoint(const Vector3& point);
    bool TraceRay(SoundRay& ray, const Vector3& listener);
    float CalculateFrequencyAbsorption(const AcousticMaterial& material, float frequency);
    float CalculateFrequencyTransmission(const AcousticMaterial& material, float frequency);
    float MaterialGetMufflingFactor(const AcousticMaterial& material);
    void HandleMaterialResonance(const AcousticMaterial& material, SoundRay& ray);
    float CalculateAirAttenuation(float distance, float frequency);
    void RecordImpulseResponse(float amplitude, float delay, const Vector3& direction, float frequency, int bounces);
    Vector3 AddRandomDeviation(const Vector3& direction, float amount);
    Vector3 RandomHemisphereDirection(const Vector3& normal);

protected:
    void OnStart() override;
    void OnUpdate() override;
};
