// TestRTAudio.cpp
#include "TestRTAudio.h"

#include "Audio.h"
#include "AudioSource.h"
#include "AudioListener.h"

#include "Engine/Core/Log.h"
#include "Engine/Debug/DebugDraw.h"

#include "Engine/Level/Level.h"
#include "Engine/Input/Input.h"

TestRTAudio::TestRTAudio(const SpawnParams& params)
    : Script(params)
{
    _tickUpdate = true;
}

void TestRTAudio::OnUpdate()
{
    // Check if F8 key is pressed to run tests
    if (Input::GetKeyDown(KeyboardKeys::F8))
    {
        LOG(Info, "Starting DSP Chain tests...");
        TestDSPChains();

        LOG(Info, "Starting Real Audio tests...");
        TestWithRealAudio();

        LOG(Info, "Starting Acoustic Ray Tracing tests...");
        TestAcousticRayTracing();

        LOG(Info, "Starting Full Acoustic System test...");
        TestFullAcousticSystem();

        LOG(Info, "All tests completed!");
    }
}

void TestRTAudio::TestDSPChains()
{
    // Create an effect chain
    SpawnParams params(Guid::New(), AudioEffectChain::TypeInitializer);
    AudioEffectChain* chain = New<AudioEffectChain>(params);
    
    // Add some effects
    ReverbEffect* reverb = New<ReverbEffect>(params);
    reverb->SetDecay(0.6f);
    reverb->SetRoomSize(0.7f);
    
    EQEffect* eq = New<EQEffect>(params);
    eq->SetLowGain(1.2f);
    eq->SetMidGain(1.0f);
    eq->SetHighGain(0.8f);
    
    // Add effects to chain
    chain->AddEffect(reverb);
    chain->AddEffect(eq);
    
    // Create some test audio data
    const uint32 numSamples = 1024;
    Array<float> inputBuffer, outputBuffer;
    inputBuffer.Resize(numSamples);
    outputBuffer.Resize(numSamples);
    
    // Fill with a test tone (simple sine wave)
    for (uint32 i = 0; i < numSamples; i++)
    {
        inputBuffer[i] = Math::Sin(i * 0.1f) * 0.5f;
    }
    
    // Create audio format info
    AudioDataInfo format;
    format.BitDepth = 32;
    format.NumChannels = 1;
    format.NumSamples = numSamples;
    format.SampleRate = 44100;
    
    // Process through the chain
    chain->Process(inputBuffer.Get(), outputBuffer.Get(), numSamples, format);
    
    // Output or visualize results
    LOG(Info, "DSP Chain processing test completed");
    
    // Clean up
    Delete(chain);
}

void TestRTAudio::TestWithRealAudio()
{
    // Find an active AudioSource
    if (Audio::Sources.IsEmpty())
    {
        LOG(Warning, "No audio sources found for testing");
        return;
    }
    
    AudioSource* source = Audio::Sources[0];
    
    // Create an effect chain
    SpawnParams params(Guid::New(), AudioEffectChain::TypeInitializer);
    AudioEffectChain* chain = New<AudioEffectChain>(params);
    
    // Add effects
    ReverbEffect* reverb = New<ReverbEffect>(params);
    reverb->SetDecay(0.6f);
    reverb->SetRoomSize(0.7f);
    chain->AddEffect(reverb);
    
    // Assign to source
    source->SetEffectChain(chain);
    
    // Play audio
    source->Play();
    
    LOG(Info, "Applied effect chain to audio source: {0}", source->GetNamePath());
}

void TestRTAudio::TestAcousticRayTracing()
{
    // Find a scene with audio sources and listeners
    Scene* scene;
    Array<Scene*> scenes;
    Level::GetScenes(scenes);
    if (scenes.Count() == 0)
    {
        LOG(Warning, "No scenes for ray tracing test");
        return;
    } else {
        //scene = Level::GetScene(0);
        scene = scenes[0];
        if (!scene)
        {
            LOG(Warning, "No active scene for ray tracing test");
            return;
        }
    }

    // Find sources and listeners
    if (Audio::Sources.IsEmpty() || Audio::Listeners.IsEmpty())
    {
        LOG(Warning, "Need both audio sources and listeners for ray tracing test");
        return;
    }
    
    AudioSource* source = Audio::Sources[0];
    AudioListener* listener = Audio::Listeners[0];
    
    // Create ray tracer
    AcousticRayTracer rayTracer;
    rayTracer.SetScene(scene);
    rayTracer.SetMaxReflections(3);
    rayTracer.SetMaxDistance(5000.0f);
    
    // Test direct path
    RayCastHit hit;
    bool blocked = rayTracer.TraceDirectPath(source->GetPosition(), listener->GetPosition(), hit);
    LOG(Info, "Direct path between source and listener is {0}", blocked ? String("blocked") : String("clear"));
    
    // Test reflection paths
    Array<AcousticRayPath> paths;
    rayTracer.TraceReflectionPaths(
        source->GetPosition(),
        source->GetOrientation(),
        listener->GetPosition(),
        16, // Number of rays
        paths
    );
    
    // Log results
    for (int32 i = 0; i < paths.Count(); i++)
    {
        if (paths[i].Points.Count() > 1)
        {
            LOG(Info, "Path {0}: {1} points, total distance: {2}, attenuation: {3}, delay: {4}s",
                i, paths[i].Points.Count(), paths[i].TotalDistance, 
                paths[i].Attenuation, paths[i].Delay);
        }
    }
}

void TestRTAudio::VisualizeAcousticRays(Scene* scene, AudioSource* source, AudioListener* listener)
{
    if (!scene || !source || !listener)
        return;
    
    AcousticRayTracer rayTracer;
    rayTracer.SetScene(scene);
    
    // Trace rays
    Array<AcousticRayPath> paths;
    rayTracer.TraceReflectionPaths(
        source->GetPosition(),
        source->GetOrientation(),
        listener->GetPosition(),
        32, // More rays for visualization
        paths
    );
    
    // Draw paths using debug visualization
    for (const AcousticRayPath& path : paths)
    {
        if (path.Points.Count() < 2)
            continue;
            
        // Color based on attenuation (red = high attenuation, green = low)
        Color pathColor = Color::Lerp(Color::Green, Color::Red, 1.0f - path.Attenuation);
        
        // Draw lines between points
        for (int i = 0; i < path.Points.Count() - 1; i++)
        {
            DEBUG_DRAW_LINE(path.Points[i], path.Points[i + 1], pathColor, 0, false);
        }
    }
}

void TestRTAudio::TestFullAcousticSystem()
{
    // Initialize the ray-traced audio manager
    RayTracedAudioManager::Init();
    RayTracedAudioManager::SetEnabled(true);
    RayTracedAudioManager::SetMaxRays(32);
    RayTracedAudioManager::SetMaxReflections(3);
    
    // Run a manual update
    RayTracedAudioManager::Update();
    
    // Visualize ray paths
    Scene* scene = Level::GetScene(0);
    if (scene && !Audio::Sources.IsEmpty() && !Audio::Listeners.IsEmpty())
    {
        VisualizeAcousticRays(scene, Audio::Sources[0], Audio::Listeners[0]);
        LOG(Info, "Visualizing acoustic ray paths");
    }
    
    LOG(Info, "Full acoustic system test completed");
}
