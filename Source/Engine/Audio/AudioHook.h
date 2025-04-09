// Copyright (c) 2025 Robert Valentine. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "AudioInterceptor.h"
#include "AudioSource.h"
#include "AudioDSPSystem.h"

// Hook functions to be inserted into the audio backend
namespace AudioHook
{
    extern int32 BufferSubmitCallCount;

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
        BufferSubmitCallCount++;
        LOG(Warning, "AudioHook: OnBufferSubmit called for source {0}, samples {1}, channels {2}",
            sourceID, sampleCount, channels);

        // Skip processing for very large buffers
        if (sampleCount > 16384) {
            LOG(Warning, "AudioHook: Buffer too large for direct processing, skipping DSP");
            return;  // Just pass through the buffer without modification
        }

        // Find audio source
        AudioSource* source = nullptr;
        for (auto s : Audio::Sources)
        {
            if (s && s->SourceID == sourceID)
            {
                source = s;
                break;
            }
        }

        if (source)
        {
            try {
                AudioDSPSystem::ProcessSource(source, buffer, sampleCount, channels, sampleRate);
            }
            catch (const std::exception& e) {
                LOG(Error, "Exception during DSP processing: {0}", String(e.what()));
            }
            catch (...) {
                LOG(Error, "Unknown exception during DSP processing");
            }
        }
        //AudioInterceptor::InterceptBuffer(buffer, sampleCount, channels, sampleRate, sourceID);
    }
}
