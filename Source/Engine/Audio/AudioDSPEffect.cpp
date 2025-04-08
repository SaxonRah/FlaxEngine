// Copyright (c) 2025 Robert Valentine. All rights reserved.

#include "AudioDSPEffect.h"

AudioDSPEffect::AudioDSPEffect(EffectType type)
{
    _type = type;
}

void AudioDSPEffect::SetEnabled(bool value)
{
    _isEnabled = value;
}
