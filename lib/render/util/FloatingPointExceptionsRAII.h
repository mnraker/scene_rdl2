// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#pragma once

#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <scene_rdl2/common/platform/Platform.h>
#ifndef _MSC_VER
#include <fenv.h>
#else
#pragma fenv_access (on)
// Simple mapping of exception flags
#define FE_DIVBYZERO _EM_ZERODIVIDE
#define FE_INEXACT _EM_INEXACT
#define FE_INVALID _EM_INVALID
#define FE_OVERFLOW _EM_OVERFLOW
#define FE_UNDERFLOW _EM_UNDERFLOW
#endif

namespace scene_rdl2 {
namespace util {

// See: https://coherent-labs.com/Documentation/cpp-gt/d8/deb/_f_p_exceptions.html

#ifndef _MSC_VER
class FloatingPointExceptionsRAII
{
public:
    explicit FloatingPointExceptionsRAII(int excepts) :
        mFlags(fegetexcept())
    {
        fedisableexcept(FE_ALL_EXCEPT);
        feenableexcept(excepts);
    }

    ~FloatingPointExceptionsRAII()
    {
        feclearexcept(FE_ALL_EXCEPT);
        fedisableexcept(FE_ALL_EXCEPT);
        feenableexcept(mFlags);
    }

private:
    fexcept_t mFlags;
};
#else

class FloatingPointExceptionsRAII
{
public:
    explicit FloatingPointExceptionsRAII(unsigned int excepts)
    {
        unsigned int cw = _controlfp(0, 0) & _MCW_EM;
        mFlags = cw;
        cw |= excepts;

        _clearfp();
        _controlfp(cw, _MCW_EM);
    }

    ~FloatingPointExceptionsRAII()
    {
        _clearfp();
        _controlfp(mFlags, _MCW_EM);
    }

private:
    unsigned int mFlags;
};
#endif

} // namespace util
} // namespace scene_rdl2

