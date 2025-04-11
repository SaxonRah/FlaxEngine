// AcousticMaterial.h
#pragma once

#include "Engine/Level/Actor.h"

/// <summary>
/// Common acoustic material types.
/// </summary>
API_ENUM() enum class AcousticMaterialType
{
    /// <summary>
    /// Custom user-defined material.
    /// </summary>
    Custom = 0,

    /// <summary>
    /// Brick material.
    /// </summary>
    Brick = 1,

    /// <summary>
    /// Concrete material.
    /// </summary>
    Concrete = 2,

    /// <summary>
    /// Wood material.
    /// </summary>
    Wood = 3,

    /// <summary>
    /// Glass material.
    /// </summary>
    Glass = 4,

    /// <summary>
    /// Carpet material.
    /// </summary>
    Carpet = 5,

    /// <summary>
    /// Metal material.
    /// </summary>
    Metal = 6,

    /// <summary>
    /// Water material.
    /// </summary>
    Water = 7
};

/// <summary>
/// Actor that defines acoustic properties of a surface for ray-traced sound
/// </summary>
API_CLASS() class FLAXENGINE_API AcousticMaterial : public Actor
{
    DECLARE_SCENE_OBJECT(AcousticMaterial);

private:
    float _absorption;
    float _reflection;
    float _transmission;
    float _scattering;
    float _lowFreqAbsorption;
    float _midFreqAbsorption;
    float _highFreqAbsorption;

public:
    
    /// <summary>
    /// Gets the overall sound absorption coefficient.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(10), DefaultValue(0.5f), Limit(0, 1, 0.01f), EditorDisplay(\"Acoustic Properties\")")
    FORCE_INLINE float GetAbsorption() const
    {
        return _absorption;
    }

    /// <summary>
    /// Sets the overall sound absorption coefficient.
    /// </summary>
    API_PROPERTY() void SetAbsorption(float value)
    {
        _absorption = Math::Saturate(value);
        _reflection = 1.0f - _absorption;
    }

    /// <summary>
    /// Gets the low frequency absorption coefficient.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(20), DefaultValue(0.2f), Limit(0, 1, 0.01f), EditorDisplay(\"Acoustic Properties\")")
    FORCE_INLINE float GetLowFreqAbsorption() const
    {
        return _lowFreqAbsorption;
    }

    /// <summary>
    /// Sets the low frequency absorption coefficient.
    /// </summary>
    API_PROPERTY() void SetLowFreqAbsorption(float value)
    {
        _lowFreqAbsorption = Math::Saturate(value);
    }

    /// <summary>
    /// Gets the mid frequency absorption coefficient.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(30), DefaultValue(0.5f), Limit(0, 1, 0.01f), EditorDisplay(\"Acoustic Properties\")")
    FORCE_INLINE float GetMidFreqAbsorption() const
    {
        return _midFreqAbsorption;
    }

    /// <summary>
    /// Sets the mid frequency absorption coefficient.
    /// </summary>
    API_PROPERTY() void SetMidFreqAbsorption(float value)
    {
        _midFreqAbsorption = Math::Saturate(value);
    }

    /// <summary>
    /// Gets the high frequency absorption coefficient.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(40), DefaultValue(0.8f), Limit(0, 1, 0.01f), EditorDisplay(\"Acoustic Properties\")")
    FORCE_INLINE float GetHighFreqAbsorption() const
    {
        return _highFreqAbsorption;
    }

    /// <summary>
    /// Sets the high frequency absorption coefficient.
    /// </summary>
    API_PROPERTY() void SetHighFreqAbsorption(float value)
    {
        _highFreqAbsorption = Math::Saturate(value);
    }

    /// <summary>
    /// Gets the sound transmission coefficient.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(50), DefaultValue(0.2f), Limit(0, 1, 0.01f), EditorDisplay(\"Acoustic Properties\")")
    FORCE_INLINE float GetTransmission() const
    {
        return _transmission;
    }

    /// <summary>
    /// Sets the sound transmission coefficient.
    /// </summary>
    API_PROPERTY() void SetTransmission(float value)
    {
        _transmission = Math::Saturate(value);
    }

    /// <summary>
    /// Gets the sound scattering coefficient (diffusion).
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(60), DefaultValue(0.3f), Limit(0, 1, 0.01f), EditorDisplay(\"Acoustic Properties\")")
    FORCE_INLINE float GetScattering() const
    {
        return _scattering;
    }

    /// <summary>
    /// Sets the sound scattering coefficient (diffusion).
    /// </summary>
    API_PROPERTY() void SetScattering(float value)
    {
        _scattering = Math::Saturate(value);
    }

    /// <summary>
    /// Gets the current material type.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(5), DefaultValue(AcousticMaterialType.Custom), EditorDisplay(\"Acoustic Properties\")")
    FORCE_INLINE AcousticMaterialType GetMaterialType() const
    {
        return _materialType;
    }

    /// <summary>
    /// Sets the material type and applies its acoustic properties.
    /// </summary>
    API_PROPERTY() void SetMaterialType(AcousticMaterialType value);

private:
    AcousticMaterialType _materialType;
};
