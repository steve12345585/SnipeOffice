/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <tools/simdsupport.hxx>

#include <stdlib.h>

#include "test_cpu_runtime_detection_x86_checks.hxx"

/* WARNING: This file is compiled with SSSE3 support, don't call
 * any function without checking cpuid to check the CPU can actually
 * handle it, and don't include any headers that contain templates
 * or inline functions, which includes cppunit.
 */
#ifdef LO_SSSE3_AVAILABLE
#define CPPUNIT_ASSERT_EQUAL(a, b) ((a) == (b) ? (void)0 : abort())
#endif
void CpuRuntimeDetectionX86Checks::checkSSSE3()
{
#ifdef LO_SSSE3_AVAILABLE
    // Try some SSSE3 intrinsics calculation
    __m128i a = _mm_set1_epi32(3);
    __m128i b = _mm_set1_epi32(3);
    __m128i c = _mm_maddubs_epi16(a, b);

    // Check result is 9
    CPPUNIT_ASSERT_EQUAL(0xFFFF, _mm_movemask_epi8(_mm_cmpeq_epi32(c, _mm_set1_epi32(9))));
#endif
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
