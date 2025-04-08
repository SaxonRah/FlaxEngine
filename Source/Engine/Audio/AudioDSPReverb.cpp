// Copyright (c) 2025 Robert Valentine. All rights reserved.

#include "AudioDSPReverb.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Memory/Memory.h"

AudioDSPReverb::AudioDSPReverb()
    : AudioDSPEffect(EffectType::Reverb)
    , _roomSize(0.5f)
    , _damping(0.5f)
    , _wetLevel(0.33f)
    , _dryLevel(0.6f)
    , _width(1.0f)
{
}

void AudioDSPReverb::SetRoomSize(float value)
{
    ScopeLock lock(_locker);
    _roomSize = Math::Clamp(value, 0.0f, 1.0f);
    UpdateParameters();
}

void AudioDSPReverb::SetDamping(float value)
{
    ScopeLock lock(_locker);
    _damping = Math::Clamp(value, 0.0f, 1.0f);
    UpdateParameters();
}

void AudioDSPReverb::SetWetLevel(float value)
{
    ScopeLock lock(_locker);
    _wetLevel = Math::Clamp(value, 0.0f, 1.0f);
}

void AudioDSPReverb::SetDryLevel(float value)
{
    ScopeLock lock(_locker);
    _dryLevel = Math::Clamp(value, 0.0f, 1.0f);
}

void AudioDSPReverb::SetWidth(float value)
{
    ScopeLock lock(_locker);
    _width = Math::Clamp(value, 0.0f, 1.0f);
}

void AudioDSPReverb::Process(const float* input, float* output, int32 sampleCount, int32 channels, int32 sampleRate)
{
    ScopeLock lock(_locker);

    if (!_isEnabled)
    {
        if (input != output)
            Memory::CopyItems(output, input, sampleCount * channels * sizeof(float));
        return;
    }

    // Initialize or update if sample rate changed
    Initialize(sampleRate);

    // Process samples using Schroeder reverb algorithm
    for (int32 i = 0; i < sampleCount; i++)
    {
        float left = 0.0f;
        float right = 0.0f;

        // Get mono input (average if stereo)
        float monoInput = 0.0f;
        for (int32 c = 0; c < channels; c++)
        {
            monoInput += input[i * channels + c];
        }
        monoInput /= channels;

        // Process through comb filters in parallel
        for (int j = 0; j < NUM_COMBS; j++)
        {
            left += ProcessComb(_combFilters[j], monoInput);

            // For stereo reverb, use slightly different feedback values for right channel
            if (channels > 1)
            {
                right += ProcessComb(_combFilters[j], monoInput) * (j % 2 == 0 ? 0.98f : 1.02f);
            }
        }

        // Process through allpass filters in series
        for (int j = 0; j < NUM_ALLPASSES; j++)
        {
            left = ProcessAllpass(_allpassFilters[j], left);

            if (channels > 1)
            {
                right = ProcessAllpass(_allpassFilters[j], right);
            }
        }

        // Mix wet and dry signals
        for (int32 c = 0; c < channels; c++)
        {
            const int32 index = i * channels + c;
            const float dry = input[index];

            // Apply stereo width
            float wet = (channels == 1 || c == 0) ? left : right * _width + left * (1.0f - _width);

            output[index] = dry * _dryLevel + wet * _wetLevel;
        }
    }
}

float AudioDSPReverb::ProcessComb(CombFilter& filter, float input)
{
    // Read from buffer
    float output = filter.buffer[filter.bufferIndex];

    // Apply damping
    filter.filterStore = output * (1.0f - filter.damp1) + filter.filterStore * filter.damp1;

    // Write to buffer
    filter.buffer[filter.bufferIndex] = input + filter.filterStore * filter.feedback;

    // Increment buffer index
    filter.bufferIndex++;
    if (filter.bufferIndex >= filter.bufferSize)
        filter.bufferIndex = 0;

    return output;
}

float AudioDSPReverb::ProcessAllpass(AllpassFilter& filter, float input)
{
    // Read from buffer
    float output = filter.buffer[filter.bufferIndex];

    // Write to buffer
    filter.buffer[filter.bufferIndex] = input + output * filter.feedback;

    // Increment buffer index
    filter.bufferIndex++;
    if (filter.bufferIndex >= filter.bufferSize)
        filter.bufferIndex = 0;

    // Output is inverted difference between input and processed value
    return output - input;
}

void AudioDSPReverb::Initialize(int32 sampleRate)
{
    // Comb filter buffer sizes for 44.1kHz (will be scaled for other sample rates)
    static const int combTunings[NUM_COMBS] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };

    // Allpass filter buffer sizes for 44.1kHz (will be scaled for other sample rates)
    static const int allpassTunings[NUM_ALLPASSES] = { 556, 441, 341, 225 };

    // Scale tunings for current sample rate
    const float sampleRateScale = (float)sampleRate / 44100.0f;

    // Initialize comb filters
    for (int i = 0; i < NUM_COMBS; i++)
    {
        CombFilter& filter = _combFilters[i];

        filter.bufferSize = (int)(combTunings[i] * sampleRateScale);
        if (filter.buffer.Count() != filter.bufferSize)
        {
            filter.buffer.Resize(filter.bufferSize);
            for (int j = 0; j < filter.bufferSize; j++)
                filter.buffer[j] = 0.0f;
            filter.bufferIndex = 0;
        }
    }

    // Initialize allpass filters
    for (int i = 0; i < NUM_ALLPASSES; i++)
    {
        AllpassFilter& filter = _allpassFilters[i];

        filter.bufferSize = (int)(allpassTunings[i] * sampleRateScale);
        if (filter.buffer.Count() != filter.bufferSize)
        {
            filter.buffer.Resize(filter.bufferSize);
            for (int j = 0; j < filter.bufferSize; j++)
                filter.buffer[j] = 0.0f;
            filter.bufferIndex = 0;
        }

        filter.feedback = 0.5f;
    }

    // Update parameters
    UpdateParameters();
}

void AudioDSPReverb::UpdateParameters()
{
    // Calculate room size and damping parameters
    const float roomSize = _roomSize * 0.28f + 0.7f;
    const float damp = _damping * 0.4f;

    // Update comb filters
    for (int i = 0; i < NUM_COMBS; i++)
    {
        _combFilters[i].feedback = roomSize;
        _combFilters[i].damp1 = damp;
        _combFilters[i].damp2 = 1.0f - damp;
    }
}