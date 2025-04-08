// Copyright (c) 2025 Robert Valentine. All rights reserved.

#include "AudioDSPConvolution.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Memory/Memory.h"

AudioDSPConvolution::AudioDSPConvolution()
    : AudioDSPEffect(EffectType::Convolution)
    , _irLength(0)
    , _bufferPosition(0)
    , _wetLevel(1.0f)
    , _dryLevel(0.0f)
{
}

Array<float> AudioDSPConvolution::GetImpulseResponse() const
{
    ScopeLock lock(_locker);
    // Create a new array that's a copy of the impulse response
    return Array<float>(_impulseResponse);
}

void AudioDSPConvolution::SetImpulseResponse(const Array<float>& impulseResponse)
{
    ScopeLock lock(_locker);
    // Create a new array as a copy of the input array
    _impulseResponse = Array<float>(impulseResponse);
    _irLength = _impulseResponse.Count();

    // Resize circular buffer to store input signal
    if (_irLength > 0)
    {
        _buffer.Resize(_irLength);
        for (int32 i = 0; i < _irLength; i++)
            _buffer[i] = 0.0f;
        _bufferPosition = 0;
    }

    LOG(Info, "AudioDSPConvolution: Set impulse response with {0} samples", _irLength);
}

void AudioDSPConvolution::SetWetLevel(float value)
{
    ScopeLock lock(_locker);
    _wetLevel = Math::Clamp(value, 0.0f, 1.0f);
}

void AudioDSPConvolution::SetDryLevel(float value)
{
    ScopeLock lock(_locker);
    _dryLevel = Math::Clamp(value, 0.0f, 1.0f);
}

void AudioDSPConvolution::Process(const float* input, float* output, int32 sampleCount, int32 channels, int32 sampleRate)
{
    ScopeLock lock(_locker);

    if (!_isEnabled || _irLength == 0)
    {
        if (input != output)
            Memory::CopyItems(output, input, sampleCount * channels * sizeof(float));
        return;
    }

    // Simple brute-force convolution (inefficient but straightforward)
    // NOTE: For a real implementation, FFT-based convolution would be much more efficient
    for (int32 i = 0; i < sampleCount; i++)
    {
        for (int32 c = 0; c < channels; c++)
        {
            const int32 index = i * channels + c;
            const float x = input[index];

            // Store input in circular buffer
            _buffer[_bufferPosition] = x;

            // Compute convolution
            float y = 0.0f;
            for (int32 j = 0; j < _irLength; j++)
            {
                int32 bufIndex = _bufferPosition - j;
                if (bufIndex < 0)
                    bufIndex += _irLength;

                y += _buffer[bufIndex] * _impulseResponse[j];
            }

            // Mix dry and wet signals
            output[index] = _dryLevel * x + _wetLevel * y;
        }

        // Advance buffer position
        _bufferPosition++;
        if (_bufferPosition >= _irLength)
            _bufferPosition = 0;
    }
}