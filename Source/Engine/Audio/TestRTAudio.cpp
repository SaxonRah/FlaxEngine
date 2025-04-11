// TestRTAudio.cpp
#include "TestRTAudio.h"

#include "Audio.h"
#include "AudioSource.h"
#include "AudioListener.h"

#include "Engine/Core/Log.h"
#include "Engine/Content/Content.h"
#include "Engine/Debug/DebugDraw.h"

#include "Engine/Level/Level.h"
#include "Engine/Input/Input.h"

#include "AudioBackend.h"

TestRTAudio::TestRTAudio(const SpawnParams& params)
    : Script(params)
{
    _tickUpdate = true;
}

void TestRTAudio::OnDisable()
{
    CleanupSineWave();

    // Clean up effects and chains
    if (_effectChain)
    {
        LOG(Info, "Cleaning up effect chain and effects");
        Delete(_effectChain);
        _effectChain = nullptr;
    }

    // Note: The effects should be deleted as part of the chain
    _reverb = nullptr;
    _eq = nullptr;
    _occlusion = nullptr;
    _spatializer = nullptr;
}

void TestRTAudio::OnUpdate()
{
    // Check if F8 key is pressed to run tests
    if (Input::GetKeyDown(KeyboardKeys::Alpha1))
    {
        LOG(Info, "Starting DSP Chain tests...");
        TestDSPChains();
    }
    if (Input::GetKeyDown(KeyboardKeys::Alpha2))
    {
        LOG(Info, "Starting Real Audio tests...");
        TestWithRealAudio();
    }
    if (Input::GetKeyDown(KeyboardKeys::Alpha3))
    {
        LOG(Info, "Starting Acoustic Ray Tracing tests...");
        TestAcousticRayTracing();
    }
    if (Input::GetKeyDown(KeyboardKeys::Alpha4))
    {
        LOG(Info, "Starting Full Acoustic System test...");
        TestFullAcousticSystem();
    }
    if (Input::GetKeyDown(KeyboardKeys::Alpha5))
    {
        LOG(Info, "Starting SineWave Effect test...");
        TestWithSineWave();   
    }
    if (Input::GetKeyDown(KeyboardKeys::Alpha6))
    {
        LOG(Info, "Starting SineWave test...");
        PlaySineWave();
        Platform::Sleep(2000);
        CleanupSineWave();
    }
}

// Create a sine wave audio clip asset
void TestRTAudio::PlaySineWave()
{
    // Define audio parameters
    const uint32 sampleRate = 44100;
    const uint32 bitDepth = 16;
    const uint32 channels = 1;
    const float duration = 2.0f;
    const float frequency = 440.0f;

    // Calculate total samples
    uint32 totalSamples = (uint32)(sampleRate * duration * channels);

    // Create buffer to hold audio data
    Array<byte> audioData;
    audioData.Resize(totalSamples * (bitDepth / 8));

    // Fill buffer with sine wave data
    int16* samples = (int16*)audioData.Get();
    for (uint32 i = 0; i < totalSamples; i++)
    {
        float time = (float)i / sampleRate;
        float value = Math::Sin(2.0f * PI * frequency * time);
        samples[i] = (int16)(value * 32767.0f);
    }

    // Create audio data info
    AudioDataInfo info;
    info.BitDepth = bitDepth;
    info.NumChannels = channels;
    info.NumSamples = totalSamples;
    info.SampleRate = sampleRate;

    // Create an audio buffer
    uint32 bufferID = AudioBackend::Buffer::Create();

    // Write the sine wave data to the buffer
    AudioBackend::Buffer::Write(bufferID, audioData.Get(), info);

    // Create an audio source directly through the backend
    uint32 sourceID = AudioBackend::Source::Add(
        info, Vector3::Zero, Quaternion::Identity,
        0.8f, // volume
        1.0f, // pitch
        0.0f, // pan
        true, // loop
        false, // spatial
        1.0f, // attenuation
        100.0f, // min distance
        1.0f // doppler factor
    );

    // Assign buffer to source
    AudioBackend::Source::SetNonStreamingBuffer(sourceID, bufferID);

    // Apply effect chain if available
    if (_effectChain)
    {
        AudioBackend::EffectChain::Set(sourceID, _effectChain);
        LOG(Info, "Applied effect chain to source ID: {0}", sourceID);
    }

    // Play it
    AudioBackend::Source::Play(sourceID);

    LOG(Info, "Created and playing sine wave with sourceID: {0}, bufferID: {1}", sourceID, bufferID);

    // Store the IDs for later cleanup
    _testSourceID = sourceID;
    _testBufferID = bufferID;
}

void TestRTAudio::CleanupSineWave()
{
    if (_testSourceID != 0)
    {
        AudioBackend::Source::Stop(_testSourceID);
        AudioBackend::Source::Remove(_testSourceID);
        _testSourceID = 0;
    }

    if (_testBufferID != 0)
    {
        AudioBackend::Buffer::Delete(_testBufferID);
        _testBufferID = 0;
    }

    LOG(Info, "Cleaned up sine wave test resources");

    // Note: We're NOT cleaning up the effect chain here
    // The effect chain should remain valid for other tests
}

void TestRTAudio::TestWithSineWave()
{
    LOG(Info, "Starting sine wave audio test with effects");

    // Create effect chain if needed
    if (!_effectChain)
    {
        SpawnParams params(Guid::New(), AudioEffectChain::TypeInitializer);
        _effectChain = New<AudioEffectChain>(params);

        // Create effects
        _reverb = New<ReverbEffect>(params);
        _eq = New<EQEffect>(params);

        // Add to chain - store references to prevent deletion
        _effectChain->AddEffect(_reverb);
        _effectChain->AddEffect(_eq);
    }

    // Configure effects with exaggerated values
    if (_reverb)
    {
        _reverb->SetRoomSize(0.9f);
        _reverb->SetDecay(0.8f);
        _reverb->SetWetDryMix(0.9f);
    }

    if (_eq)
    {
        _eq->SetLowGain(2.0f);
        _eq->SetMidGain(0.5f);
        _eq->SetHighGain(0.2f);
    }

    // Play the sine wave
    PlaySineWave();

    // Wait for 2 seconds to hear the effect
    Platform::Sleep(2000);

    // Clean up only the sine wave resources, not the effect chain
    CleanupSineWave();
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
    if (Audio::Sources.IsEmpty() || Audio::Listeners.IsEmpty())
    {
        LOG(Warning, "Need both audio sources and listeners for this test");
        return;
    }

    AudioSource* source = Audio::Sources[0];
    AudioListener* listener = Audio::Listeners[0];

    // Stop the source first
    source->Stop();

    // Set up ray tracer to get environmental data
    Scene* scene = nullptr;
    Array<Scene*> scenes;
    Level::GetScenes(scenes);
    if (scenes.Count() > 0)
    {
        scene = scenes[0];
    }

    if (!scene)
    {
        LOG(Warning, "No active scene for ray tracing, using default parameters");
    }

    // Acoustic analysis variables
    float occlusionFactor = 0.0f;
    float reverbFactor = 0.7f;
    float roomSize = 0.8f;
    bool directPathBlocked = false;
    float directPathDistance = 0.0f;
    Vector3 reflectionPoint = Vector3::Zero;
    int reflectionCount = 0;
    float maxReflectionDelay = 0.0f;
    Array<AcousticRayPath> rayPaths;

    // Detailed acoustic analysis
    if (scene)
    {
        // Create ray tracer with detailed settings
        AcousticRayTracer rayTracer;
        rayTracer.SetScene(scene);
        rayTracer.SetMaxReflections(5);    // More reflections for better analysis
        rayTracer.SetMaxDistance(10000.0f); // Larger distance to catch more paths

        // Test direct path for occlusion
        RayCastHit hit;
        directPathBlocked = rayTracer.TraceDirectPath(source->GetPosition(), listener->GetPosition(), hit);

        if (directPathBlocked)
        {
            occlusionFactor = 0.8f; // High occlusion if direct path is blocked
            LOG(Info, "Direct path is blocked by object at position {0}", hit.Point.ToString());
            reflectionPoint = hit.Point;
        }

        // Calculate direct path distance
        directPathDistance = Vector3::Distance(source->GetPosition(), listener->GetPosition());
        LOG(Info, "Direct path distance: {0} units", directPathDistance);

        // Trace reflection paths for detailed analysis
        rayTracer.TraceReflectionPaths(
            source->GetPosition(),
            source->GetOrientation(),
            listener->GetPosition(),
            32, // More rays for better analysis
            rayPaths
        );

        // Analysis of ray paths
        if (rayPaths.Count() > 0)
        {
            float totalAttenuation = 0.0f;
            float maxDistance = 0.0f;
            float totalDelay = 0.0f;

            for (const AcousticRayPath& path : rayPaths)
            {
                if (path.Points.Count() > 1)
                {
                    reflectionCount += path.Points.Count() - 2; // Count reflection points
                    totalAttenuation += path.Attenuation;
                    maxDistance = Math::Max(maxDistance, path.TotalDistance);
                    totalDelay += path.Delay;
                    maxReflectionDelay = Math::Max(maxReflectionDelay, path.Delay);

                    // Find a good reflection point to use
                    if (path.Points.Count() > 2 && reflectionPoint == Vector3::Zero)
                    {
                        reflectionPoint = path.Points[1]; // First reflection point
                    }
                }
            }

            // Calculate acoustic properties from ray data
            reverbFactor = Math::Saturate(totalAttenuation / Math::Max(1, rayPaths.Count()));
            roomSize = Math::Saturate(maxDistance / 10000.0f);

            LOG(Info, "Ray analysis: {0} paths, {1} reflections, max delay {2}s",
                rayPaths.Count(), reflectionCount, maxReflectionDelay);
        }
    }

    // Create effect chain only if not created yet
    if (!_effectChain)
    {
        SpawnParams params(Guid::New(), AudioEffectChain::TypeInitializer);
        _effectChain = New<AudioEffectChain>(params);

        // Create all effects
        _reverb = New<ReverbEffect>(params);
        _eq = New<EQEffect>(params);
        _occlusion = New<OcclusionEffect>(params);
        _spatializer = New<SpatializerEffect>(params);

        // Add them to the chain
        _effectChain->AddEffect(_reverb);
        _effectChain->AddEffect(_eq);
        _effectChain->AddEffect(_occlusion);
        _effectChain->AddEffect(_spatializer);
    }

    // -------------------------------------------------------------------------
    // 1. Set up REVERB effect based on acoustic analysis
    // -------------------------------------------------------------------------
    // Adjust room size based on ray analysis
    _reverb->SetRoomSize(roomSize);

    // Adjust decay based on reflection count and attenuation
    float decayValue = Math::Clamp(reverbFactor * (1.0f + reflectionCount * 0.1f), 0.1f, 0.98f);
    _reverb->SetDecay(decayValue);

    // Higher wet mix for larger rooms
    float reverbWetMix = Math::Lerp(0.5f, 0.9f, roomSize);
    _reverb->SetWetDryMix(reverbWetMix);

    LOG(Info, "Reverb: room size={0}, decay={1}, wet/dry={2}",
        roomSize, decayValue, reverbWetMix);

    // -------------------------------------------------------------------------
    // 2. Set up EQ effect based on occlusion and environment
    // -------------------------------------------------------------------------
    // Occluded sound loses high frequencies
    float highGain = directPathBlocked ? 0.3f : 1.8f;

    // Distant sounds have reduced mid-range clarity
    float midGain = Math::Lerp(1.0f, 0.3f, directPathDistance / 5000.0f);

    // Bass tends to propagate through obstacles
    float lowGain = Math::Lerp(1.0f, 2.0f, occlusionFactor);

    _eq->SetHighGain(highGain);
    _eq->SetMidGain(midGain);
    _eq->SetLowGain(lowGain);
    _eq->SetWetDryMix(0.9f);

    LOG(Info, "EQ: low={0}, mid={1}, high={2} (based on occlusion={3}, distance={4})",
        lowGain, midGain, highGain, occlusionFactor, directPathDistance);

    // -------------------------------------------------------------------------
    // 3. Set up OCCLUSION effect based on ray casting
    // -------------------------------------------------------------------------
    // Set occlusion level directly from ray analysis
    _occlusion->SetOcclusionLevel(occlusionFactor);

    // Call update to recalculate filter parameters
    _occlusion->UpdateParameters();

    // Set wet mix based on whether path is blocked
    float occlusionWetMix = directPathBlocked ? 0.9f : 0.5f;
    _occlusion->SetWetDryMix(occlusionWetMix);

    LOG(Info, "Occlusion: level={0}, wet/dry={1}",
        occlusionFactor, occlusionWetMix);

    // -------------------------------------------------------------------------
    // 4. Set up SPATIALIZER effect with detailed positional data
    // -------------------------------------------------------------------------
    // Set actual listener position
    _spatializer->SetListenerPosition(listener->GetPosition());
    _spatializer->SetListenerOrientation(listener->GetOrientation());

    // Use reflection point if available, otherwise use source position
    Vector3 spatialPosition = (reflectionPoint != Vector3::Zero) ?
        reflectionPoint : source->GetPosition();
    _spatializer->SetSourcePosition(spatialPosition);

    // Set source radius based on room size (larger radius in larger rooms)
    float sourceRadius = Math::Lerp(50.0f, 500.0f, roomSize);
    _spatializer->SetSourceRadius(sourceRadius);

    // Set reverb mix based on analysis
    _spatializer->SetReverbMix(reverbFactor);

    // Higher wet mix for more immersive effect
    _spatializer->SetWetDryMix(0.9f);

    LOG(Info, "Spatializer: position={0}, radius={1}, reverb={2}",
        spatialPosition.ToString(), sourceRadius, reverbFactor);

    // Assign the chain to the source
    source->SetEffectChain(_effectChain);

    // Set appropriate volume to avoid clipping with effects
    source->SetVolume(0.7f);

    // Force rebuffering based on audio type
    if (!source->UseStreaming())
    {
        LOG(Info, "Non-streaming audio: Forcing complete rebuffer");
        // Force recreation of source
        if (source->SourceID != 0)
        {
            uint32 oldSourceID = source->SourceID;
            source->Stop();
            source->SourceID = 0;  // Force recreation
            LOG(Info, "Forced source rebuffer: old ID={0}", oldSourceID);
        }
    }
    else
    {
        LOG(Info, "Streaming audio: Requesting buffer update");
        source->RequestStreamingBuffersUpdate();

        // For streaming audio, we may need to make sure pending buffers are processed
        if (source->SourceID != 0)
        {
            int32 queuedBuffers = 0;
            AudioBackend::Source::GetQueuedBuffersCount(source->SourceID, queuedBuffers);
            LOG(Info, "Current source has {0} buffers queued", queuedBuffers);

            if (queuedBuffers > 0)
            {
                LOG(Info, "Flushing existing buffers to ensure effects are applied");
                AudioBackend::Source::DequeueProcessedBuffers(source->SourceID);
            }
        }
    }

    // Play the audio
    source->Play();

    // Visualize the acoustic rays
    if (scene)
    {
        VisualizeAcousticRays(scene, source, listener);
        LOG(Info, "Visualizing {0} acoustic ray paths", rayPaths.Count());
    }

    LOG(Info, "Applied full DSP chain with environment-based settings");
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
            DEBUG_DRAW_LINE(path.Points[i], path.Points[i + 1], pathColor, 5, false);
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
