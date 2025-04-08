// AudioDSPSystem.h
#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/CriticalSection.h"

class AudioSource;
class AudioDSPEffect;
class AudioDSPChain;

// Audio DSP processing system - not exposed to scripting
class FLAXENGINE_API AudioDSPSystem
{
private:
    static Array<AudioDSPChain*> _chains;
    static CriticalSection _locker;

public:
    // Gets the DSP chain for an audio source, creates one if it doesn't exist
    static AudioDSPChain* GetSourceDSP(AudioSource* source);

    // Removes the DSP chain for an audio source
    static void RemoveSourceDSP(AudioSource* source);

    // Processes audio samples for a specific audio source
    static bool ProcessSource(AudioSource* source, float* buffer, int32 sampleCount, int32 channels, int32 sampleRate);

    // Initializes the Audio DSP system
    static void Initialize();

    // Shuts down the Audio DSP system
    static void Shutdown();
};
