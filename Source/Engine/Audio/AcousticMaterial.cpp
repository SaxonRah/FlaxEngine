// AcousticMaterial.cpp
#include "AcousticMaterial.h"

AcousticMaterial::AcousticMaterial(const SpawnParams& params)
    : Actor(params)
    , _absorption(0.5f)
    , _reflection(0.5f)
    , _transmission(0.2f)
    , _scattering(0.3f)
    , _lowFreqAbsorption(0.2f)
    , _midFreqAbsorption(0.5f)
    , _highFreqAbsorption(0.8f)
{
    SetMaterialType(GetMaterialType());
}

void AcousticMaterial::SetMaterialType(AcousticMaterialType value)
{
    _materialType = value;
    
    switch (_materialType)
    {
    case AcousticMaterialType::Brick:
        _absorption = 0.4f;
        _lowFreqAbsorption = 0.2f;
        _midFreqAbsorption = 0.3f;
        _highFreqAbsorption = 0.5f;
        _transmission = 0.05f;
        _scattering = 0.3f;
        break;
    
    case AcousticMaterialType::Concrete:
        _absorption = 0.2f;
        _lowFreqAbsorption = 0.1f;
        _midFreqAbsorption = 0.2f;
        _highFreqAbsorption = 0.3f;
        _transmission = 0.02f;
        _scattering = 0.2f;
        break;
    
    case AcousticMaterialType::Wood:
        _absorption = 0.3f;
        _lowFreqAbsorption = 0.2f;
        _midFreqAbsorption = 0.3f;
        _highFreqAbsorption = 0.4f;
        _transmission = 0.1f;
        _scattering = 0.2f;
        break;
    
    case AcousticMaterialType::Glass:
        _absorption = 0.1f;
        _lowFreqAbsorption = 0.05f;
        _midFreqAbsorption = 0.1f;
        _highFreqAbsorption = 0.15f;
        _transmission = 0.3f;
        _scattering = 0.1f;
        break;
    
    case AcousticMaterialType::Carpet:
        _absorption = 0.8f;
        _lowFreqAbsorption = 0.4f;
        _midFreqAbsorption = 0.8f;
        _highFreqAbsorption = 0.9f;
        _transmission = 0.2f;
        _scattering = 0.5f;
        break;
    
    case AcousticMaterialType::Metal:
        _absorption = 0.1f;
        _lowFreqAbsorption = 0.05f;
        _midFreqAbsorption = 0.1f;
        _highFreqAbsorption = 0.15f;
        _transmission = 0.0f;
        _scattering = 0.1f;
        break;
    
    case AcousticMaterialType::Water:
        _absorption = 0.03f;
        _lowFreqAbsorption = 0.01f;
        _midFreqAbsorption = 0.03f;
        _highFreqAbsorption = 0.05f;
        _transmission = 0.95f;
        _scattering = 0.4f;
        break;
    
    default:
        // Custom - do nothing
        break;
    }
    
    _reflection = 1.0f - _absorption;
}
