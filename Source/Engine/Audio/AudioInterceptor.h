// Copyright (c) 2025 Robert Valentine. All rights reserved.

#pragma once

#include "Engine/Engine/EngineService.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Math.h"
#include "Audio.h"

// This file creates a hook into the audio backend to intercept buffer data before playback

class AudioBuffer
{
public:
    float* Data = nullptr;
    int32 SampleCount = 0;
    int32 Channels = 0;
    int32 SampleRate = 0;
    uint32 SourceID = 0;
    bool Processed = false;

    AudioBuffer() = default;
    ~AudioBuffer() = default;

    // Add copy/move constructors if needed
    AudioBuffer(const AudioBuffer& other) = default;
    AudioBuffer& operator=(const AudioBuffer& other) = default;
};

/// <summary>
/// Audio buffer interception service that processes audio buffers before playback.
/// </summary>
class AudioInterceptor : public EngineService
//class FLAXENGINE_API AudioInterceptor : public EngineService
{
private:
    
    
    static Array<AudioBuffer> _pendingBuffers;
    static CriticalSection _bufferLocker;
    
public:
    /// <summary>
    /// Creates a new audio interceptor service.
    /// </summary>
    AudioInterceptor();
    
    // [EngineService]
    void Update() override;
    
    /// <summary>
    /// Intercepts an audio buffer before playback.
    /// </summary>
    /// <param name="buffer">Audio buffer</param>
    /// <param name="sampleCount">Sample count per channel</param>
    /// <param name="channels">Channels count</param>
    /// <param name="sampleRate">Sample rate</param>
    /// <param name="sourceID">Source ID</param>
    static void InterceptBuffer(float* buffer, int32 sampleCount, int32 channels, int32 sampleRate, uint32 sourceID);
};
