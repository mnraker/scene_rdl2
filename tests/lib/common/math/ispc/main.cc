// Copyright 2023 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

/// @file main.cc
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <scene_rdl2/pdevunit/pdevunit.h>

#include "Test.h"
#include "TestAsA.h"
#include "TestAutodiff.h"
#include "TestCol3.h"
#include "TestCol4.h"
#include "TestColorSpace.h"
#include "TestConstants.h"
#include "TestMat3.h"
#include "TestMat4.h"
#include "TestQuaternion.h"
#include "TestReferenceFrame.h"
#include "TestVec2.h"
#include "TestVec3.h"
#include "TestVec4.h"
#include "TestXform.h"
#include "TestXformv.h"

int
main(int argc, char *argv[])
{
    using namespace scene_rdl2::common::math::ispc::unittest;

    CPPUNIT_TEST_SUITE_REGISTRATION(Test);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestAsA);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestAutodiff);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestCol3);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestCol4);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestColorSpace);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestConstants);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestMat3);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestMat4);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestQuaternion);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestReferenceFrame);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestVec2);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestVec3);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestVec4);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestXform);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestXformv);

    return pdevunit::run(argc, argv);
}
