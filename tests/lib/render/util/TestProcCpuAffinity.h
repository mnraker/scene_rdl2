// Copyright 2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <scene_rdl2/render/util/ProcCpuAffinity.h>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

namespace scene_rdl2 {
namespace affinity {
namespace unittest {

class TestProcCpuAffinity : public CppUnit::TestFixture
{
public:
    void setUp() override {}
    void tearDown() override {}

    void testPartialAffinity();
    void testFullAffinity();

    CPPUNIT_TEST_SUITE(TestProcCpuAffinity);
    // No affinity support on windows (yet)
#ifndef _MSC_VER
    CPPUNIT_TEST(testPartialAffinity);
    CPPUNIT_TEST(testFullAffinity);
#endif
    CPPUNIT_TEST_SUITE_END();

private:
    using SetCpuIdFunc = std::function<void(const unsigned numCpu, ProcCpuAffinity& proc)>;

    void testMain(const SetCpuIdFunc& setCpuIdFunc);
};

} // namespace unittest
} // namespace affinity
} // namespace scene_rdl2
