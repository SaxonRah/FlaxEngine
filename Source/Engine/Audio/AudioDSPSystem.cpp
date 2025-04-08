// Copyright (c) 2025 Robert Valentine. All rights reserved.

#include "AudioDSPSystem.h"
#include "AudioDSPChain.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Core/Collections/ChunkedArray.h"
#include "AudioSource.h"
#include "Audio.h"

Array<AudioDSPChain*> AudioDSPSystem::_chains;
CriticalSection AudioDSPSystem::_locker;
/*
// Create an engine service to handle audio processing
class AudioDSPProcessingService : public EngineService
{
public:
    AudioDSPProcessingService()
        : EngineService(TEXT("Audio DSP"), 100)
    {
        LOG(Info, "AudioDSPProcessingService: Constructor called");
    }

    bool Init() override
    {
        LOG(Info, "AudioDSPProcessingService: Init called");
        try {
            AudioDSPSystem::Initialize();
            LOG(Info, "AudioDSPProcessingService: Initialize succeeded");
            return true;
        }
        catch (const std::exception& e) {
            LOG(Error, "AudioDSPProcessingService: Exception during initialization: {0}", String(e.what()));
            return false;
        }
        catch (...) {
            LOG(Error, "AudioDSPProcessingService: Unknown exception during initialization");
            return false;
        }
    }

    void Dispose() override
    {
        LOG(Info, "AudioDSPProcessingService: Dispose called");
        AudioDSPSystem::Shutdown();
    }
};
// Register engine service
AudioDSPProcessingService AudioDSPProcessingServiceInstance;
*/

void AudioDSPSystem::Initialize()
{
    LOG(Info, "AudioDSPSystem: Initializing");

    // Initialize system here
    _chains.Clear();

    LOG(Info, "AudioDSPSystem: Initialization complete");
}

void AudioDSPSystem::Shutdown()
{
    LOG(Info, "AudioDSPSystem: Shutting down");
    
    // Clean up all chains
    ScopeLock lock(_locker);

    for (AudioDSPChain* chain : _chains)
    {
        Delete(chain);
    }

    _chains.Clear();
    
}

AudioDSPChain* AudioDSPSystem::GetSourceDSP(AudioSource* source)
{
    LOG(Warning, "AudioDSPSystem: GetSourceDSP called but not fully implemented");

    if (!source)
        return nullptr;

    ScopeLock lock(_locker);

    // Find existing chain
    for (AudioDSPChain* chain : _chains)
    {
        if (chain->GetSource() == source)
            return chain;
    }

    // Create new chain
    AudioDSPChain* chain = new AudioDSPChain(source);
    _chains.Add(chain);

    LOG(Info, "AudioDSPSystem: Created new DSP chain for source {0}", source->GetNamePath());

    return chain;
}

void AudioDSPSystem::RemoveSourceDSP(AudioSource* source)
{
    LOG(Info, "AudioDSPSystem: Request to remove DSP chain for source {0}",
        source ? source->GetNamePath() : TEXT("null"));

    if (!source)
        return;

    ScopeLock lock(_locker);

    // Find and remove chain
    for (int i = 0; i < _chains.Count(); i++)
    {
        if (_chains[i] && _chains[i]->GetSource() == source)
        {
            AudioDSPChain* chain = _chains[i];
            _chains.RemoveAt(i);

            // Properly delete the chain
            if (chain)
                delete chain;

            LOG(Info, "AudioDSPSystem: Removed DSP chain for source {0}", source->GetNamePath());
            return;
        }
    }
}

bool AudioDSPSystem::ProcessSource(AudioSource* source, float* buffer, int32 sampleCount, int32 channels, int32 sampleRate)
{
    if (!source)
        return false;

    ScopeLock lock(_locker);

    // Find chain
    for (AudioDSPChain* chain : _chains)
    {
        if (chain->GetSource() == source)
        {
            // Process audio
            chain->Process(buffer, sampleCount, channels, sampleRate);
            return true;
        }
    }

    return false;
}

