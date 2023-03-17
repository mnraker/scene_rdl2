// Copyright 2023 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

//
//
#pragma once

#include <cstdint>

namespace scene_rdl2 {
namespace fb_util {

class ReGammaC2F
{
public:

    //
    // Conversion from 8bit quantized value to 32bit single float by LUT.
    // Input uc value should be generated by GammaF2C::g22() or similar logic.
    //
    static float rg22(const uint8_t uc);
}; // ReGammaC2F

} // namespace fb_util
} // namespace scene_rdl2
