// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0


#pragma once

#include "AttributeKey.h"
#include "Node.h"
#include "SceneClass.h"
#include "SceneObject.h"
#include "Types.h"

#include <scene_rdl2/common/except/exceptions.h>

#include <string>

namespace scene_rdl2 {
namespace rdl2 {

enum class TextureFilterType
{
    // Keep this in sync with moonray/lib/rendering/pbr/core/Distribution.h and .hh
    TEXTURE_FILTER_NEAREST = 0,
    TEXTURE_FILTER_BILINEAR,
    TEXTURE_FILTER_NEAREST_MIP_NEAREST,
    TEXTURE_FILTER_BILINEAR_MIP_NEAREST,

    TEXTURE_FILTER_NUM_TYPES
};
        
class Light : public Node
{
public:
    typedef Node Parent;

    Light(const SceneClass& sceneClass, const std::string& name);
    virtual ~Light();
    static SceneObjectInterface declare(SceneClass& sceneClass);

    /// Returns the visibility mask.
    int getVisibilityMask() const;

    // Attributes common to all Lights.
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool>  sOnKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool>  sMbKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sVisibleInCameraKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Rgb> sColorKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sIntensityKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sExposureKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sMaxShadowDistanceKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sMinShadowDistanceKey;
    /// enum PresenceShadows {
    ///    PRESENCE_SHADOWS_OFF,         // Presence shadows off for this light.
    ///    PRESENCE_SHADOWS_ON,          // Presence shadows on for this light.
    ///    PRESENCE_SHADOWS_USE_GLOBAL,  // Use "enable presence shadows" from scene vars.
    /// };
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sPresenceShadowsKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sRayTerminationKey;
    /// see enum class TextureFilterType above
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sTextureFilterKey;

    RDL2_DSO_BUILTIN_API static AttributeKey<String> sTextureKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Rgb> sSaturationKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Rgb> sContrastKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Rgb> sGammaKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Rgb> sGainKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Rgb> sOffsetKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Vec3f> sTemperatureKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sTextureRotationKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Vec2f> sTextureTranslationKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Vec2f> sTextureCoverageKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sTextureRepsUKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sTextureRepsVKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sTextureMirrorUKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sTextureMirrorVKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Rgb> sTextureBorderColorKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<SceneObjectVector> sLightFiltersKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<String> sLabel;

    // visibility flags
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sVisibleDiffuseReflection;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sVisibleDiffuseTransmission;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sVisibleGlossyReflection;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sVisibleGlossyTransmission;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sVisibleMirrorReflection;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sVisibleMirrorTransmission;
};

template <>
inline const Light*
SceneObject::asA() const
{
    return isA<Light>() ? static_cast<const Light*>(this) : nullptr;
}

template <>
inline Light*
SceneObject::asA()
{
    return isA<Light>() ? static_cast<Light*>(this) : nullptr;
}

} // namespace rdl2
} // namespace scene_rdl2

