// Copyright (c) 2025 Robert Valentine. All rights reserved.

#include "AudioDSPHighPass.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Memory/Memory.h"

AudioDSPHighPass::AudioDSPHighPass()
    : AudioDSPEffect(EffectType::HighPass)
    , _cutoffFrequency(200.0f)
    , _resonance(0.707f)
    , _a0(0)
    , _a1(0)
    , _a2(0)
    , _b1(0)
    , _b2(0)
{
}

void AudioDSPHighPass::SetCutoffFrequency(float value)
{
    ScopeLock lock(_locker);
    _cutoffFrequency = Math::Clamp(value, 20.0f, 20000.0f);
}

void AudioDSPHighPass::SetResonance(float value)
{
    ScopeLock lock(_locker);
    _resonance = Math::Clamp(value, 0.1f, 10.0f);
}

void AudioDSPHighPass::Process(const float* input, float* output, int32 sampleCount, int32 channels, int32 sampleRate)
{
    if (!input || !output)
    {
        LOG(Error, "AudioDSPHighPass: Null input or output pointer");
        return;
    }

    // Copy input to output with no processing when the sample count is too high
    if (sampleCount > 16384)
    {
        LOG(Warning, "AudioDSPHighPass: Sample count too large ({0}), skipping processing", sampleCount);

        // Safer copy implementation - do it in chunks
        const int32 chunkSize = 1024;
        const int32 totalSamples = sampleCount * channels;

        for (int32 i = 0; i < totalSamples; i += chunkSize)
        {
            int32 samplesThisChunk = Math::Min(chunkSize, totalSamples - i);
            Memory::CopyItems(output + i, input + i, samplesThisChunk);
        }
        return;
    }

    ScopeLock lock(_locker);

    if (!_isEnabled)
    {
        if (input != output)
            Memory::CopyItems(output, input, sampleCount * channels * sizeof(float));
        return;
    }

    // Update coefficients if needed
    UpdateCoefficients(sampleRate);

    // Ensure state buffers have correct size
    if (_x1.Count() != channels || _x2.Count() != channels || _y1.Count() != channels || _y2.Count() != channels)
    {
        _x1.Resize(channels);
        _x2.Resize(channels);
        _y1.Resize(channels);
        _y2.Resize(channels);

        for (int32 i = 0; i < channels; i++)
        {
            _x1[i] = _x2[i] = _y1[i] = _y2[i] = 0.0f;
        }
    }

    // Process samples
    for (int32 i = 0; i < sampleCount; i++)
    {
        for (int32 c = 0; c < channels; c++)
        {
            const int32 index = i * channels + c;
            const float x0 = input[index];

            // Biquad filter
            const float y0 = _a0 * x0 + _a1 * _x1[c] + _a2 * _x2[c] - _b1 * _y1[c] - _b2 * _y2[c];

            // Update state
            _x2[c] = _x1[c];
            _x1[c] = x0;
            _y2[c] = _y1[c];
            _y1[c] = y0;

            output[index] = y0;
        }
    }
}

void AudioDSPHighPass::UpdateCoefficients(int32 sampleRate)
{
    const float omega = 2.0f * PI * _cutoffFrequency / (float)sampleRate;
    const float alpha = Math::Sin(omega) / (2.0f * _resonance);

    const float cosOmega = Math::Cos(omega);

    // Biquad filter coefficients for high-pass
    _a0 = (1.0f + cosOmega) / 2.0f;
    _a1 = -(1.0f + cosOmega);
    _a2 = (1.0f + cosOmega) / 2.0f;

    const float norm = 1.0f / (1.0f + alpha);
    _a0 *= norm;
    _a1 *= norm;
    _a2 *= norm;
    _b1 = -2.0f * cosOmega * norm;
    _b2 = (1.0f - alpha) * norm;
}
