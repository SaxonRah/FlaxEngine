// TestRTAudio.h
#pragma once

//#include "Engine/Level/Actor.h"
#include "Engine/Scripting/Script.h"

#include "AudioSource.h"
#include "AudioListener.h"
#include "AudioEffectChain.h"
#include "ReverbEffect.h"
#include "EQEffect.h"
#include "AcousticRayTracer.h"
#include "RayTracedAudioManager.h"

/// <summary>
/// Actor that defines acoustic properties of a surface for ray-traced sound
/// </summary>
API_CLASS() class FLAXENGINE_API TestRTAudio : public Script
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE(TestRTAudio);

public:

    // void OnEnable() override;
    // void OnDisable() override;
    void OnUpdate() override;
    // void OnStart() override;

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
