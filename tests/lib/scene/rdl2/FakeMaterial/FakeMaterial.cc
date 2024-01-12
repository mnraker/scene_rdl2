// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0


#include <scene_rdl2/scene/rdl2/rdl2.h>

#include "attributes.cc"

using namespace scene_rdl2;

namespace moonray { namespace shading { class BsdfBuilder; }}

namespace {

void
fakeShade(const rdl2::Material*,
         moonray::shading::TLState *tls,
         const moonray::shading::State& state,
         moonray::shading::BsdfBuilder& bsdfBuilder)
{
}

} // namespace

RDL2_DSO_CLASS_BEGIN(FakeMaterial, rdl2::Material)

public:
    FakeMaterial(const rdl2::SceneClass& sceneClass, const std::string& name);

RDL2_DSO_CLASS_END(FakeMaterial)

FakeMaterial::FakeMaterial(const rdl2::SceneClass& sceneClass,
                           const std::string& name) :
    Parent(sceneClass, name)
{
    mShadeFunc = fakeShade;
}

