// Copyright (c) 2025 Robert Valentine. All rights reserved.

#include "AudioInterceptor.h"
#include "AudioDSPSystem.h"
#include "AudioSource.h"
#include "Engine/Core/Log.h"

// Initialize static members
Array<AudioBuffer> AudioInterceptor::_pendingBuffers;
CriticalSection AudioInterceptor::_bufferLocker;

AudioInterceptor::AudioInterceptor()
    : EngineService(TEXT("Audio Interceptor"), 60)
{
}

void AudioInterceptor::Update()
{
    static int updateCounter = 0;
    updateCounter++;

    if (updateCounter % 100 == 0)
    {
        LOG(Warning, "AudioInterceptor: Update called, pending buffers = {0}", _pendingBuffers.Count());
    }

    ScopeLock lock(_bufferLocker);

    // Process all pending buffers with extreme caution
    for (int i = 0; i < _pendingBuffers.Count(); i++)
    {
        AudioBuffer& buffer = _pendingBuffers[i];

        if (!buffer.Processed)
        {
            // Find audio source for this source ID
            AudioSource* source = nullptr;
            for (auto s : Audio::Sources)
            {
                if (s && s->SourceID == buffer.SourceID)
                {
                    source = s;
                    break;
                }
            }

            if (source)
            {
                if (updateCounter % 100 == 0)
                {
                    LOG(Warning, "AudioInterceptor: Processing source {0}, ID {1}",
                        source->GetNamePath(), buffer.SourceID);
                }

                try
                {
                    // For large buffers, skip DSP to avoid crashes
                    if (buffer.SampleCount <= 16384)
                    {
                        // Process buffer through DSP system
                        AudioDSPSystem::ProcessSource(source, buffer.Data, buffer.SampleCount, buffer.Channels, buffer.SampleRate);
                    }
                    else
                    {
                        LOG(Warning, "AudioInterceptor: Skipping DSP for large buffer");
                    }
                }
                catch (...)
                {
                    LOG(Error, "AudioInterceptor: Exception during audio processing");
                }
            }
            else if (updateCounter % 100 == 0)
            {
                LOG(Warning, "AudioInterceptor: Could not find source for ID {0}", buffer.SourceID);
            }

            buffer.Processed = true;
        }
    }

    // Clean up processed buffers
    for (int i = _pendingBuffers.Count() - 1; i >= 0; i--)
    {
        if (_pendingBuffers[i].Processed)
        {
            _pendingBuffers.RemoveAt(i);
        }
    }
}

void AudioInterceptor::InterceptBuffer(float* buffer, int32 sampleCount, int32 channels, int32 sampleRate, uint32 sourceID)
{
    if (!buffer || sampleCount <= 0 || channels <= 0 || sampleRate <= 0 || sourceID == 0)
        return;

    // Skip processing for extremely large buffers to avoid crashes
    if (sampleCount > 16384)
    {
        LOG(Warning, "AudioInterceptor: Skipping DSP for large buffer (source {0}, samples {1})",
            sourceID, sampleCount);
        return;
    }

    LOG(Warning, "AudioInterceptor: Intercepting buffer for source ID {0}, samples: {1}, channels: {2}",
        sourceID, sampleCount, channels);

    ScopeLock lock(_bufferLocker);

    // Add buffer to pending list
    AudioBuffer audioBuffer;
    audioBuffer.Data = buffer;
    audioBuffer.SampleCount = sampleCount;
    audioBuffer.Channels = channels;
    audioBuffer.SampleRate = sampleRate;
    audioBuffer.SourceID = sourceID;
    audioBuffer.Processed = false;

    _pendingBuffers.Add(audioBuffer);
}

// Register engine service
AudioInterceptor AudioInterceptorInstance;
