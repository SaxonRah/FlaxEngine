// Copyright (c) 2025 Robert Valentine. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "AudioInterceptor.h"

// Hook functions to be inserted into the audio backend
namespace AudioHook
{
    /// <summary>
    /// Called when audio buffer is about to be submitted to the audio backend.
    /// </summary>
    /// <param name="buffer">Audio buffer</param>
    /// <param name="sampleCount">Sample count per channel</param>
    /// <param name="channels">Channels count</param>
    /// <param name="sampleRate">Sample rate</param>
    /// <param name="sourceID">Source ID</param>
    inline void OnBufferSubmit(float* buffer, int32 sampleCount, int32 channels, int32 sampleRate, uint32 sourceID)
    {
        // Intercept buffer
        AudioInterceptor::InterceptBuffer(buffer, sampleCount, channels, sampleRate, sourceID);
    }
}
