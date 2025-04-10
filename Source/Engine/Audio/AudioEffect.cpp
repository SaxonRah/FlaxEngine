#include "AudioEffect.h"

// Constructor implementation
AudioEffect::AudioEffect(const SpawnParams& params)
    : ScriptingObject(params)
    , _isEnabled(true)
    , _wetDryMix(0.5f)
{
    // Any additional initialization code
}
