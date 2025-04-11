// TestRTAudio.h
#pragma once

//#include "Engine/Level/Actor.h"
#include "Engine/Scripting/Script.h"

#include "AudioSource.h"
#include "AudioListener.h"
#include "AudioEffectChain.h"
#include "AcousticRayTracer.h"
#include "RayTracedAudioManager.h"
#include "EQEffect.h"
#include "OcclusionEffect.h"
#include "ReverbEffect.h"
#include "SpatializerEffect.h"

/// <summary>
/// Actor that defines acoustic properties of a surface for ray-traced sound
/// </summary>
API_CLASS() class FLAXENGINE_API TestRTAudio : public Script
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE(TestRTAudio);
private:

    AudioEffectChain* _effectChain = nullptr;
    EQEffect* _eq = nullptr;
    OcclusionEffect* _occlusion = nullptr;
    ReverbEffect* _reverb = nullptr;
    SpatializerEffect* _spatializer = nullptr;

    uint32 _testSourceID = 0;
    uint32 _testBufferID = 0;

public:

    // void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
    // void OnStart() override;

    /// PlaySineWave
    API_FUNCTION()
    void PlaySineWave();

    /// CleanupSineWave
    API_FUNCTION()
    void CleanupSineWave();
    
    /// TestWithSineWave
    API_FUNCTION()
    void TestWithSineWave();

    /// TestDSPChains
    API_FUNCTION()
    void TestDSPChains();

    /// TestWithRealAudio
    API_FUNCTION()
    void TestWithRealAudio();

    /// TestAcousticRayTracing
    API_FUNCTION()
    void TestAcousticRayTracing();

    /// VisualizeAcousticRays
    API_FUNCTION()
    void VisualizeAcousticRays(Scene* scene, AudioSource* source, AudioListener* listener);

    /// TestFullAcousticSystem
    API_FUNCTION()
    void TestFullAcousticSystem();
	
};
