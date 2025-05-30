/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <sal/config.h>

#include <cmath>
#include <limits>
#include <span>

#include <svx/EnhancedCustomShapeGeometry.hxx>
#include <com/sun/star/drawing/EnhancedCustomShapeGluePointType.hpp>

const sal_Int32 MIN_INT32 = std::numeric_limits<sal_Int32>::min();

const sal_Int32 mso_sptDefault1400[] =
{
    1, 1400
};
const sal_Int32 mso_sptDefault1800[] =
{
    1, 1800
};
const sal_Int32 mso_sptDefault2500[] =
{
    1, 2500
};
const sal_Int32 mso_sptDefault2700[] =
{
    1, 2700
};
const sal_Int32 mso_sptDefault3600[] =
{
    1, 3600
};
const sal_Int32 mso_sptDefault3700[] =
{
    1, 3700
};
const sal_Int32 mso_sptDefault5400[] =
{
    1, 5400
};
const sal_Int32 mso_sptDefault7200[] =
{
    1, 7200
};
const sal_Int32 mso_sptDefault8100[] =
{
    1, 8100
};
const sal_Int32 mso_sptDefault9600[] =
{
    1, 9600
};
const sal_Int32 mso_sptDefault10800[] =
{
    1, 10800
};
const sal_Int32 mso_sptDefault12000[] =
{
    1, 12000
};
const sal_Int32 mso_sptDefault13500[] =
{
    1, 13500
};
const sal_Int32 mso_sptDefault16200[] =
{
    1, 16200
};
const sal_Int32 mso_sptDefault16200and5400[] =
{
    2, 16200, 5400
};

const SvxMSDffVertPair mso_sptArcVert[] =
{
    { 0, 0 }, { 21600, 21600 }, { 3 MSO_I, 1 MSO_I }, { 7 MSO_I, 5 MSO_I }, { 10800, 10800 },
    { 0, 0 }, { 21600, 21600 }, { 3 MSO_I, 1 MSO_I }, { 7 MSO_I, 5 MSO_I }
};
const sal_uInt16 mso_sptArcSegm[] =
{
    0xa604, 0xab00, 0x0001, 0x6001, 0x8000,
    0xa604, 0xaa00, 0x8000
};
const SvxMSDffCalculationData mso_sptArcCalc[] =
{
    { 0x4009, { 10800, DFF_Prop_adjustValue, 0 } },
    { 0x2000, { 0x400, 10800, 0 } },
    { 0x400a, { 10800, DFF_Prop_adjustValue, 0 } },
    { 0x2000, { 0x402, 10800, 0 } },
    { 0x4009, { 10800, DFF_Prop_adjust2Value, 0 } },
    { 0x2000, { 0x404, 10800, 0 } },
    { 0x400a, { 10800, DFF_Prop_adjust2Value, 0 } },
    { 0x2000, { 0x406, 10800, 0 } }
};
const sal_Int32 mso_sptArcDefault[] =
{
    2, 270, 0
};
const SvxMSDffVertPair mso_sptStandardGluePoints[] =
{
    { 10800, 0 }, { 0, 10800 }, { 10800, 21600 }, { 21600, 10800 }
};
const SvxMSDffHandle mso_sptArcHandle[] =
{
    {   SvxMSDffHandleFlags::POLAR | SvxMSDffHandleFlags::RADIUS_RANGE,
        10800, 0x100, 10800, 10800, 10800, 10800, MIN_INT32, 0x7fffffff },
    {   SvxMSDffHandleFlags::POLAR | SvxMSDffHandleFlags::RADIUS_RANGE,
        10800, 0x101, 10800, 10800, 10800, 10800, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoArc =
{
    std::span<const SvxMSDffVertPair>(mso_sptArcVert),
    const_cast<sal_uInt16*>(mso_sptArcSegm), sizeof( mso_sptArcSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptArcCalc),
    const_cast<sal_Int32*>(mso_sptArcDefault),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptArcHandle),
};

const SvxMSDffVertPair mso_sptTextSimpleVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 }, { 0, 0 }
};
const mso_CustomShape msoTextSimple =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextSimpleVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptRectangleVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 }, { 0, 0 }
};
const mso_CustomShape msoRectangle =
{
    std::span<const SvxMSDffVertPair>(mso_sptRectangleVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptRoundRectangleVert[] = // adj value 0 -> 10800
{
    { 7 MSO_I, 0 }, { 0, 8 MSO_I }, { 0, 9 MSO_I }, { 7 MSO_I, 21600 },
    { 10 MSO_I, 21600 }, { 21600, 9 MSO_I }, { 21600, 8 MSO_I }, { 10 MSO_I, 0 }
};
const sal_uInt16 mso_sptRoundRectangleSegm[] =
{
    0x4000, 0xa701, 0x0001, 0xa801, 0x0001, 0xa701, 0x0001, 0xa801, 0x6000, 0x8000
};
const SvxMSDffCalculationData mso_sptRoundRectangleCalc[] =
{
    { 0x000e, { 0, 45, 0 } },
    { 0x6009, { DFF_Prop_adjustValue, 0x400, 0 } },
    { 0x2001, { 0x401, 3163, 7636 } },
    { 0x6000, { DFF_Prop_geoLeft, 0x402, 0 } },
    { 0x6000, { DFF_Prop_geoTop, 0x402, 0 } },
    { 0xa000, { DFF_Prop_geoRight, 0, 0x402 } },
    { 0xa000, { DFF_Prop_geoBottom, 0, 0x402 } },
    { 0x6000, { DFF_Prop_geoLeft, DFF_Prop_adjustValue, 0 } },
    { 0x6000, { DFF_Prop_geoTop, DFF_Prop_adjustValue, 0 } },
    { 0xa000, { DFF_Prop_geoBottom, 0, DFF_Prop_adjustValue } },
    { 0xa000, { DFF_Prop_geoRight, 0, DFF_Prop_adjustValue } }
};
const SvxMSDffTextRectangles mso_sptRoundRectangleTextRect[] =
{
    { { 3 MSO_I, 4 MSO_I }, { 5 MSO_I, 6 MSO_I } }
};
const SvxMSDffHandle mso_sptRoundRectangleHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::SWITCHED,
        0x100, 0, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoRoundRectangle =
{
    std::span<const SvxMSDffVertPair>(mso_sptRoundRectangleVert),
    const_cast<sal_uInt16*>(mso_sptRoundRectangleSegm), sizeof( mso_sptRoundRectangleSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptRoundRectangleCalc),
    const_cast<sal_Int32*>(mso_sptDefault3600),
    std::span<const SvxMSDffTextRectangles>(mso_sptRoundRectangleTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptRoundRectangleHandle),
};

const SvxMSDffVertPair mso_sptRightTriangleVert[] =
{
    { 0, 0 }, { 21600, 21600 }, { 0, 21600 }, { 0, 0 }
};
const SvxMSDffTextRectangles mso_sptRightTriangleTextRect[] =
{
    { { 1900, 12700 }, { 12700, 19700 } }
};
const SvxMSDffVertPair mso_sptRightTriangleGluePoints[] =
{
    { 0, 0 }, { 0, 10800 }, { 0, 21600 }, { 10800, 21600 }, { 21600, 21600 }, { 10800, 10800 }
};
const mso_CustomShape msoRightTriangle =
{
    std::span<const SvxMSDffVertPair>(mso_sptRightTriangleVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptRightTriangleTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptRightTriangleGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptEllipseVert[] =
{
    { 10800, 10800 }, { 10800, 10800 },  { 0, 360 }
};
const sal_uInt16 mso_sptEllipseSegm[] =
{
    0xa203, 0x6000, 0x8000
};
const SvxMSDffTextRectangles mso_sptEllipseTextRect[] =
{
    { { 3163, 3163 }, { 18437, 18437 } }
};
const SvxMSDffVertPair mso_sptEllipseGluePoints[] =
{
    { 10800, 0 }, { 3163, 3163 }, { 0, 10800 }, { 3163, 18437 }, { 10800, 21600 }, { 18437, 18437 }, { 21600, 10800 }, { 18437, 3163 }
};
const mso_CustomShape msoEllipse =
{
    std::span<const SvxMSDffVertPair>(mso_sptEllipseVert),
    const_cast<sal_uInt16*>(mso_sptEllipseSegm), sizeof( mso_sptEllipseSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptEllipseTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptEllipseGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptParallelogramVert[] =  // adjustment1 : 0 - 21600
{
    { 0 MSO_I, 0 }, { 21600, 0 }, { 1 MSO_I, 21600 }, { 0, 21600 }
};
const sal_uInt16 mso_sptParallelogramSegm[] =
{
    0x4000, 0x0003, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptParallelogramCalc[] =
{
    { 0x4000, { 0, DFF_Prop_adjustValue, 0 } },
    { 0x8000, { 0, 21600, DFF_Prop_adjustValue } },
    { 0x2001, { DFF_Prop_adjustValue, 10, 24 } },
    { 0x2000, { 0x0402, 1750, 0 } },
    { 0x8000, { 21600, 0, 0x0403 } },
    { 0x2001, { 0x400, 1, 2 } },
    { 0x4000, { 10800, 0x405, 0 } },
    { 0x2000, { 0x400, 0, 10800 } },
    { 0x6006, { 0x407, 0x40d, 0 } },
    { 0x8000, { 10800, 0, 0x405 } },
    { 0x6006, { 0x407, 0x40c, 21600 } },
    { 0x8000, { 21600, 0, 0x405 } },
    { 0x8001, { 21600, 10800, 0x400 } },
    { 0x8000, { 21600, 0, 0x40c } }
};
const SvxMSDffTextRectangles mso_sptParallelogramTextRect[] =
{
    { { 3 MSO_I, 3 MSO_I }, { 4 MSO_I, 4 MSO_I } }
};
const SvxMSDffVertPair mso_sptParallelogramGluePoints[] =
{
    { 6 MSO_I, 0 }, { 10800, 8 MSO_I }, { 11 MSO_I, 10800 }, { 9 MSO_I, 21600 }, { 10800, 10 MSO_I }, { 5 MSO_I, 10800 }
};
const SvxMSDffHandle mso_sptParallelogramHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 0, 10800, 10800, 0, 21600, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoParallelogram =
{
    std::span<const SvxMSDffVertPair>(mso_sptParallelogramVert),
    const_cast<sal_uInt16*>(mso_sptParallelogramSegm), sizeof( mso_sptParallelogramSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptParallelogramCalc),
    const_cast<sal_Int32*>(mso_sptDefault5400),
    std::span<const SvxMSDffTextRectangles>(mso_sptParallelogramTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptParallelogramGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptParallelogramHandle),
};

const SvxMSDffVertPair mso_sptDiamondVert[] =
{
    { 10800, 0 }, { 21600, 10800 }, { 10800, 21600 }, { 0, 10800 }, { 10800, 0 }
};
const SvxMSDffTextRectangles mso_sptDiamondTextRect[] =
{
    { { 5400, 5400 }, { 16200, 16200 } }
};
const mso_CustomShape msoDiamond =
{
    std::span<const SvxMSDffVertPair>(mso_sptDiamondVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptDiamondTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptTrapezoidVert[] =      // adjustment1 : 0 - 10800
{
    { 0, 0 }, { 21600, 0 }, {0 MSO_I, 21600 }, { 1 MSO_I, 21600 }
};
const sal_uInt16 mso_sptTrapezoidSegm[] =
{
    0x4000, 0x0003, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptTrapezoidCalc[] =
{
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } },
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2001, { DFF_Prop_adjustValue, 10, 18 } },
    { 0x2000, { 0x0402, 1750, 0 } },
    { 0x8000, { 21600, 0, 0x403 } },
    { 0x2001, { DFF_Prop_adjustValue, 1, 2 } },
    { 0x8000, { 21600, 0, 0x405 } }
};
const SvxMSDffTextRectangles mso_sptTrapezoidTextRect[] =
{
    { { 3 MSO_I, 3 MSO_I }, { 4 MSO_I, 4 MSO_I } }
};
const SvxMSDffVertPair mso_sptTrapezoidGluePoints[] =
{
    { 6 MSO_I, 10800 }, { 10800, 21600 }, { 5 MSO_I, 10800 }, { 10800, 0 }
};
const SvxMSDffHandle mso_sptTrapezoidHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 1, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoTrapezoid =
{
    std::span<const SvxMSDffVertPair>(mso_sptTrapezoidVert),
    const_cast<sal_uInt16*>(mso_sptTrapezoidSegm), sizeof( mso_sptTrapezoidSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTrapezoidCalc),
    const_cast<sal_Int32*>(mso_sptDefault5400),
    std::span<const SvxMSDffTextRectangles>(mso_sptTrapezoidTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptTrapezoidGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptTrapezoidHandle),
};

/*

 The side of the enclosing square for the regular (all sides the
 same, all angles the same) octagon described below is 21600. Let's
 call that 'a'.

 The "adjustment1" is the horizontal (or vertical) distance from a
 side of the square to the nearest vertex. Let's call that 'd'.

 Let's call the side of the regular octagon 'b'.

 We know a. We want d. d=(a-b)/2

 Pythagoras says that b^2 = 2d^2

 Solving for b, we get b = (sqrt(2)-1)a


     !------------a=21600-------!

     !--d--!
           x--------------x
          /                \
         /                  \
        /                    \
       /                      \
      /                        \
     x                          x
     !                          !
     !                          !
     !                          !
     !                          !
     !                          !
     !                          !
     x                          x
      \                        /
       \                      /
        \                    /
         \                  /
          \                /
           x--------------x

*/

const SvxMSDffVertPair mso_sptOctagonVert[] =        // adjustment1 : 0 - 10800
{
    { 0 MSO_I, 0 }, { 2 MSO_I, 0 }, { 21600, 1 MSO_I }, { 21600, 3 MSO_I },
    { 2 MSO_I, 21600 }, { 0 MSO_I, 21600 }, { 0, 3 MSO_I }, { 0, 1 MSO_I }
};
const sal_uInt16 mso_sptOctagonSegm[] =
{
    0x4000, 0x0007, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptOctagonCalc[] =
{
    { 0x6000, { DFF_Prop_geoLeft, DFF_Prop_adjustValue, 0 } },
    { 0x6000, { DFF_Prop_geoTop, DFF_Prop_adjustValue, 0 } },
    { 0xa000, { DFF_Prop_geoRight, 0, DFF_Prop_adjustValue } },
    { 0xa000, { DFF_Prop_geoBottom, 0, DFF_Prop_adjustValue } },
    { 0x2001, { DFF_Prop_adjustValue, 1, 2 } },
    { 0x6000, { DFF_Prop_geoLeft, 0x404, 0 } },
    { 0x6000, { DFF_Prop_geoTop, 0x404, 0 } },
    { 0xa000, { DFF_Prop_geoRight, 0, 0x404 } },
    { 0xa000, { DFF_Prop_geoBottom, 0, 0x404 } }
};
const sal_Int32 mso_sptOctagonDefault[] =
{
    1, static_cast<sal_Int32>((21600 - ((M_SQRT2-1)*21600)) / 2)
};
const SvxMSDffTextRectangles mso_sptOctagonTextRect[] =
{
    { { 5 MSO_I, 6 MSO_I }, { 7 MSO_I, 8 MSO_I } }
};
const SvxMSDffHandle mso_sptOctagonHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 0, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoOctagon =
{
    std::span<const SvxMSDffVertPair>(mso_sptOctagonVert),
    const_cast<sal_uInt16*>(mso_sptOctagonSegm), sizeof( mso_sptOctagonSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptOctagonCalc),
    const_cast<sal_Int32*>(mso_sptOctagonDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptOctagonTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptOctagonHandle),
};

const SvxMSDffVertPair mso_sptIsocelesTriangleVert[] =   // adjustment1 : 0 - 21600
{
    { 0 MSO_I, 0 }, { 21600, 21600 }, { 0, 21600 }
};
const sal_uInt16 mso_sptIsocelesTriangleSegm[] =
{
    0x4000, 0x0002, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptIsocelesTriangleCalc[] =
{
    { 0x4000, { 0, DFF_Prop_adjustValue, 0 } },
    { 0x2001, { DFF_Prop_adjustValue, 1, 2 } },
    { 0x2000, { 0x401, 10800, 0 } },
    { 0x2001, { DFF_Prop_adjustValue, 2, 3 } },
    { 0x2000, { 0x403, 7200, 0 } },
    { 0x8000, { 21600, 0, 0x400 } },
    { 0x2001, { 0x405, 1, 2 } },
    { 0x8000, { 21600, 0, 0x406 } }
};
const SvxMSDffTextRectangles mso_sptIsocelesTriangleTextRect[] =
{
    { { 1 MSO_I, 10800 }, { 2 MSO_I, 18000 } },
    { { 3 MSO_I, 7200 }, { 4 MSO_I, 21600   } }
};
const SvxMSDffVertPair mso_sptIsocelesTriangleGluePoints[] =
{
    { 0 MSO_I, 0 }, { 1 MSO_I, 10800 }, { 0, 21600 }, { 10800, 21600 }, { 21600, 21600 }, { 7 MSO_I, 10800 }
};
const SvxMSDffHandle mso_sptIsocelesTriangleHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 0, 10800, 10800, 0, 21600, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoIsocelesTriangle =
{
    std::span<const SvxMSDffVertPair>(mso_sptIsocelesTriangleVert),
    const_cast<sal_uInt16*>(mso_sptIsocelesTriangleSegm), sizeof( mso_sptIsocelesTriangleSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptIsocelesTriangleCalc),
    const_cast<sal_Int32*>(mso_sptDefault10800),
    std::span<const SvxMSDffTextRectangles>(mso_sptIsocelesTriangleTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptIsocelesTriangleGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptIsocelesTriangleHandle),
};

const SvxMSDffVertPair mso_sptHexagonVert[] =                // adjustment1 : 0 - 10800
{
    { 0 MSO_I, 0 }, { 1 MSO_I, 0 }, { 21600, 10800 }, { 1 MSO_I, 21600 },
    { 0 MSO_I, 21600 }, { 0, 10800 }
};
const sal_uInt16 mso_sptHexagonSegm[] =
{
    0x4000, 0x0005, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptHexagonCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } },
    { 0x2001, { DFF_Prop_adjustValue, 100, 234 } },
    { 0x2000, { 0x402, 1700, 0 } },
    { 0x8000, { 21600, 0, 0x403 } }
};
const SvxMSDffTextRectangles mso_sptHexagonTextRect[] =
{
    { { 3 MSO_I, 3 MSO_I }, { 4 MSO_I, 4 MSO_I } }
};
const SvxMSDffHandle mso_sptHexagonHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 0, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoHexagon =
{
    std::span<const SvxMSDffVertPair>(mso_sptHexagonVert),
    const_cast<sal_uInt16*>(mso_sptHexagonSegm), sizeof( mso_sptHexagonSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptHexagonCalc),
    const_cast<sal_Int32*>(mso_sptDefault5400),
    std::span<const SvxMSDffTextRectangles>(mso_sptHexagonTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptHexagonHandle),
};

const SvxMSDffVertPair mso_sptPentagonVert[] =
{
    { 10800, 0 }, { 0, 8260 }, { 4230, 21600 }, { 17370, 21600 },
    { 21600, 8260 }, { 10800, 0 }
};
const SvxMSDffTextRectangles mso_sptPentagonTextRect[] =
{
    { { 4230, 5080 }, { 17370, 21600 } }
};
const SvxMSDffVertPair mso_sptPentagonGluePoints[] =
{
    { 10800, 0 }, { 0, 8260 }, { 4230, 21600 }, { 10800, 21600 },
    { 17370, 21600 }, { 21600, 8260 }
};
const mso_CustomShape msoPentagon =
{
    std::span<const SvxMSDffVertPair>(mso_sptPentagonVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptPentagonTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptPentagonGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptPlusVert[] =               // adjustment1 : 0 - 10800
{
    { 1 MSO_I, 0 }, { 2 MSO_I, 0 }, { 2 MSO_I, 1 MSO_I }, { 21600, 1 MSO_I },
    { 21600, 3 MSO_I }, { 2 MSO_I, 3 MSO_I }, { 2 MSO_I, 21600 }, { 1 MSO_I, 21600 },
    { 1 MSO_I, 3 MSO_I }, { 0, 3 MSO_I }, { 0, 1 MSO_I }, { 1 MSO_I, 1 MSO_I }, { 1 MSO_I, 0 }
};
const SvxMSDffCalculationData mso_sptPlusCalc[] =
{
    { 0x2001, { DFF_Prop_adjustValue, 10799, 10800 } },
    { 0x2000, { 0x400, 0, 0 } },
    { 0xa000, { DFF_Prop_geoRight, 0, 0x400 } },
    { 0xa000, { DFF_Prop_geoBottom, 0, 0x400 } }
};
const SvxMSDffTextRectangles mso_sptPlusTextRect[] =
{
    { { 1 MSO_I, 1 MSO_I }, { 2 MSO_I, 3 MSO_I } }
};
const SvxMSDffHandle mso_sptPlusHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::SWITCHED,
        0x100, 0, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoPlus =
{
    std::span<const SvxMSDffVertPair>(mso_sptPlusVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(mso_sptPlusCalc),
    const_cast<sal_Int32*>(mso_sptDefault5400),
    std::span<const SvxMSDffTextRectangles>(mso_sptPlusTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptPlusHandle),
};

const SvxMSDffVertPair mso_sptCanVert[] =        // adjustment1 : 0 - 10800
{
    { 44, 0 },
    { 20, 0 }, { 0, 2 MSO_I }, { 0, 0 MSO_I },          // ccp
    { 0, 3 MSO_I },                                     // p
    { 0, 4 MSO_I }, { 20, 21600 }, { 44, 21600 },       // ccp
    { 68, 21600 }, { 88, 4 MSO_I }, { 88, 3 MSO_I },    // ccp
    { 88, 0 MSO_I },                                    // p
    { 88, 2 MSO_I }, { 68, 0 }, { 44, 0 },              // ccp
    { 44, 0 },                                          // p
    { 20, 0 }, { 0, 2 MSO_I }, { 0, 0 MSO_I },          // ccp
    { 0, 5 MSO_I }, { 20, 6 MSO_I }, { 44, 6 MSO_I },   // ccp
    { 68, 6 MSO_I },{ 88, 5 MSO_I }, { 88, 0 MSO_I },   // ccp
    { 88, 2 MSO_I },{ 68, 0 }, { 44, 0 }                // ccp
};
const sal_uInt16 mso_sptCanSegm[] =
{
    0x4000, 0x2001, 0x0001, 0x2002, 0x0001, 0x2001, 0x6001, 0x8000,
    0x4000, 0x2004, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptCanCalc[] =
{
    { 0x2001, { DFF_Prop_adjustValue, 2, 4 } },     // 1/4
    { 0x2001, { 0x0400, 6, 11 } },
    { 0xa000, { 0x0400, 0, 0x0401 } },
    { 0x8000, { 21600, 0, 0x0400 } },
    { 0x6000, { 0x0403, 0x0401, 0 } },
    { 0x6000, { 0x0400, 0x0401, 0 } },
    { 0x2001, { DFF_Prop_adjustValue, 2, 2 } },
    { 0x0000, { 44, 0, 0 } }
};
const SvxMSDffTextRectangles mso_sptCanTextRect[] =
{
    { { 0, 6 MSO_I }, { 88, 3 MSO_I } }
};
const SvxMSDffVertPair mso_sptCanGluePoints[] =
{
    { 44, 6 MSO_I }, { 44, 0 }, { 0, 10800 }, { 44, 21600 }, { 88, 10800 }
};
const SvxMSDffHandle mso_sptCanHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        7 + 3, 0x100, 44, 10800, MIN_INT32, 0x7fffffff, 0, 10800 }
};
const mso_CustomShape msoCan =
{
    std::span<const SvxMSDffVertPair>(mso_sptCanVert),
    const_cast<sal_uInt16*>(mso_sptCanSegm), sizeof( mso_sptCanSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCanCalc),
    const_cast<sal_Int32*>(mso_sptDefault5400),
    std::span<const SvxMSDffTextRectangles>(mso_sptCanTextRect),
    88, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptCanGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptCanHandle),
};

const SvxMSDffVertPair mso_sptArrowVert[] =  // adjustment1: x 0 - 21600
{                                                   // adjustment2: y 0 - 10800
    { 0, 0 MSO_I }, { 1 MSO_I, 0 MSO_I }, { 1 MSO_I, 0 }, { 21600, 10800 },
    { 1 MSO_I, 21600 }, { 1 MSO_I, 2 MSO_I }, { 0, 2 MSO_I }
};
const sal_uInt16 mso_sptArrowSegm[] =
{
    0x4000, 0x0006, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptArrowCalc[] =
{
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x8000, { 21600, 0, DFF_Prop_adjust2Value } },
    { 0x8000, { 21600, 0, 0x401 } },
    { 0x6001, { 0x403, 0x400, 10800 } },
    { 0x6000, { 0x401, 0x404, 0 } },
    { 0x6001, { 0x401, 0x400, 10800 } },
    { 0xa000, { 0x401, 0, 0x406 } }
};
const SvxMSDffTextRectangles mso_sptArrowTextRect[] =
{
    { { 0, 0 MSO_I }, { 5 MSO_I, 2 MSO_I } }
};
const SvxMSDffHandle mso_sptArrowHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 0x101, 10800, 10800, 0, 21600, 0, 10800 }
};
const mso_CustomShape msoArrow =
{
    std::span<const SvxMSDffVertPair>(mso_sptArrowVert),
    const_cast<sal_uInt16*>(mso_sptArrowSegm), sizeof( mso_sptArrowSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptArrowCalc),
    const_cast<sal_Int32*>(mso_sptDefault16200and5400),
    std::span<const SvxMSDffTextRectangles>(mso_sptArrowTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptArrowHandle),
};

const SvxMSDffVertPair mso_sptLeftArrowVert[] =  // adjustment1: x 0 - 21600
{                                                       // adjustment2: y 0 - 10800
    { 21600, 0 MSO_I }, { 1 MSO_I, 0 MSO_I }, { 1 MSO_I, 0 }, { 0, 10800 },
    { 1 MSO_I, 21600 }, { 1 MSO_I, 2 MSO_I }, { 21600, 2 MSO_I }
};
const sal_uInt16 mso_sptLeftArrowSegm[] =
{
    0x4000, 0x0006, 0x6001, 0x8000
};
const sal_Int32 mso_sptLeftArrowDefault[] =
{
    2, 5400, 5400
};
const SvxMSDffTextRectangles mso_sptLeftArrowTextRect[] =
{
    { { 7 MSO_I, 0 MSO_I }, { 21600, 2 MSO_I } }
};
const SvxMSDffHandle mso_sptLeftArrowHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 0x101, 10800, 10800, 0, 21600, 0, 10800 }
};
const mso_CustomShape msoLeftArrow =
{
    std::span<const SvxMSDffVertPair>(mso_sptLeftArrowVert),
    const_cast<sal_uInt16*>(mso_sptLeftArrowSegm), sizeof( mso_sptLeftArrowSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptArrowCalc),
    const_cast<sal_Int32*>(mso_sptLeftArrowDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptLeftArrowTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptLeftArrowHandle),
};

const SvxMSDffVertPair mso_sptDownArrowVert[] =  // adjustment1: x 0 - 21600
{                                                       // adjustment2: y 0 - 10800
    { 0 MSO_I, 0 }, { 0 MSO_I, 1 MSO_I }, { 0, 1 MSO_I }, { 10800, 21600 },
    { 21600, 1 MSO_I }, { 2 MSO_I, 1 MSO_I }, { 2 MSO_I, 0 }
};
const sal_uInt16 mso_sptDownArrowSegm[] =
{
    0x4000, 0x0006, 0x6001, 0x8000
};
const SvxMSDffTextRectangles mso_sptDownArrowTextRect[] =
{
    { { 0 MSO_I, 0 }, { 2 MSO_I, 5 MSO_I } }
};
const SvxMSDffHandle mso_sptDownArrowHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x101, 0x100, 10800, 10800, 0, 10800, 0, 21600 }
};
const mso_CustomShape msoDownArrow =
{
    std::span<const SvxMSDffVertPair>(mso_sptDownArrowVert),
    const_cast<sal_uInt16*>(mso_sptDownArrowSegm), sizeof( mso_sptDownArrowSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptArrowCalc),
    const_cast<sal_Int32*>(mso_sptDefault16200and5400),
    std::span<const SvxMSDffTextRectangles>(mso_sptDownArrowTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptDownArrowHandle),
};

const SvxMSDffVertPair mso_sptUpArrowVert[] =    // adjustment1: x 0 - 21600
{                                                       // adjustment2: y 0 - 10800
    { 0 MSO_I, 21600 }, { 0 MSO_I, 1 MSO_I }, { 0, 1 MSO_I }, { 10800, 0 },
    { 21600, 1 MSO_I }, { 2 MSO_I, 1 MSO_I }, { 2 MSO_I, 21600 }
};
const sal_uInt16 mso_sptUpArrowSegm[] =
{
    0x4000, 0x0006, 0x6001, 0x8000
};
const sal_Int32 mso_sptUpArrowDefault[] =
{
    2, 5400, 5400
};
const SvxMSDffTextRectangles mso_sptUpArrowTextRect[] =
{
    { { 0 MSO_I, 7 MSO_I }, { 2 MSO_I, 21600 } }
};
const SvxMSDffHandle mso_sptUpArrowHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x101, 0x100, 10800, 10800, 0, 10800, 0, 21600 }
};
const mso_CustomShape msoUpArrow =
{
    std::span<const SvxMSDffVertPair>(mso_sptUpArrowVert),
    const_cast<sal_uInt16*>(mso_sptUpArrowSegm), sizeof( mso_sptUpArrowSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptArrowCalc),
    const_cast<sal_Int32*>(mso_sptUpArrowDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptUpArrowTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptUpArrowHandle),
};

const SvxMSDffVertPair mso_sptLeftRightArrowVert[] = // adjustment1: x 0 - 10800
{                                                           // adjustment2: y 0 - 10800
    { 0, 10800 }, { 0 MSO_I, 0 }, { 0 MSO_I, 1 MSO_I }, { 2 MSO_I, 1 MSO_I },
    { 2 MSO_I, 0 }, { 21600, 10800 }, { 2 MSO_I, 21600 }, { 2 MSO_I, 3 MSO_I },
    { 0 MSO_I, 3 MSO_I }, { 0 MSO_I, 21600 }
};
const sal_uInt16 mso_sptLeftRightArrowSegm[] =
{
    0x4000, 0x0009, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptDoubleArrowCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } },
    { 0x8000, { 21600, 0, DFF_Prop_adjust2Value } },
    { 0x8000, { 10800, 0, DFF_Prop_adjust2Value } },
    { 0x6001, { DFF_Prop_adjustValue, 0x404, 10800 } },
    { 0x8000, { 21600, 0, 0x405 } },
    { 0x8000, { 10800, 0, DFF_Prop_adjustValue } },
    { 0x6001, { DFF_Prop_adjust2Value, 0x407, 10800 } },
    { 0x8000, { 21600, 0, 0x408 } }
};
const sal_Int32 mso_sptLeftRightArrowDefault[] =
{
    2, 4300, 5400
};
const SvxMSDffTextRectangles mso_sptLeftRightArrowTextRect[] =
{
    { { 5 MSO_I, 1 MSO_I }, { 6 MSO_I, 3 MSO_I } }
};
const SvxMSDffHandle mso_sptLeftRightArrowHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 0x101, 10800, 10800, 0, 10800, 0, 10800 }
};
const mso_CustomShape msoLeftRightArrow =
{
    std::span<const SvxMSDffVertPair>(mso_sptLeftRightArrowVert),
    const_cast<sal_uInt16*>(mso_sptLeftRightArrowSegm), sizeof( mso_sptLeftRightArrowSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptDoubleArrowCalc),
    const_cast<sal_Int32*>(mso_sptLeftRightArrowDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptLeftRightArrowTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptLeftRightArrowHandle),
};

const SvxMSDffVertPair mso_sptUpDownArrowVert[] =    // adjustment1: x 0 - 10800
{                                                           // adjustment2: y 0 - 10800
    { 0, 1 MSO_I }, { 10800, 0 }, { 21600, 1 MSO_I }, { 2 MSO_I, 1 MSO_I },
    { 2 MSO_I, 3 MSO_I }, { 21600, 3 MSO_I }, { 10800, 21600 }, { 0, 3 MSO_I },
    { 0 MSO_I, 3 MSO_I }, { 0 MSO_I, 1 MSO_I }
};
const sal_uInt16 mso_sptUpDownArrowSegm[] =
{
    0x4000, 0x0009, 0x6001, 0x8000
};
const sal_Int32 mso_sptUpDownArrowDefault[] =
{
    2, 5400, 4300
};
const SvxMSDffTextRectangles mso_sptUpDownArrowTextRect[] =
{
    { { 0 MSO_I, 8 MSO_I }, { 2 MSO_I, 9 MSO_I } }
};
const SvxMSDffHandle mso_sptUpDownArrowHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 0x101, 10800, 10800, 0, 10800, 0, 10800 }
};
const mso_CustomShape msoUpDownArrow =
{
    std::span<const SvxMSDffVertPair>(mso_sptUpDownArrowVert),
    const_cast<sal_uInt16*>(mso_sptUpDownArrowSegm), sizeof( mso_sptUpDownArrowSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptDoubleArrowCalc),
    const_cast<sal_Int32*>(mso_sptUpDownArrowDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptUpDownArrowTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptUpDownArrowHandle),
};

const SvxMSDffVertPair mso_sptQuadArrowVert[] =  // adjustment1: x 0 - 10800, adjustment2: x 0 - 10800
{                                                       // adjustment3: y 0 - 10800
    { 0, 10800 }, { 0 MSO_I, 1 MSO_I }, { 0 MSO_I, 2 MSO_I }, { 2 MSO_I, 2 MSO_I },
    { 2 MSO_I, 0 MSO_I }, { 1 MSO_I, 0 MSO_I }, { 10800, 0 }, { 3 MSO_I, 0 MSO_I },
    { 4 MSO_I, 0 MSO_I }, { 4 MSO_I, 2 MSO_I }, { 5 MSO_I, 2 MSO_I }, { 5 MSO_I, 1 MSO_I },
    { 21600, 10800 }, { 5 MSO_I, 3 MSO_I }, { 5 MSO_I, 4 MSO_I }, { 4 MSO_I, 4 MSO_I },
    { 4 MSO_I, 5 MSO_I }, { 3 MSO_I, 5 MSO_I }, { 10800, 21600 }, { 1 MSO_I, 5 MSO_I },
    { 2 MSO_I, 5 MSO_I }, { 2 MSO_I, 4 MSO_I }, { 0 MSO_I, 4 MSO_I }, { 0 MSO_I, 3 MSO_I }
};
const sal_uInt16 mso_sptQuadArrowSegm[] =
{
    0x4000, 0x0017, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptQuadArrowCalc[] =
{
    { 0x2000, { DFF_Prop_adjust3Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } },
    { 0x8000, { 21600, 0, DFF_Prop_adjust2Value } },
    { 0x8000, { 21600, 0, DFF_Prop_adjust3Value } }
};
const sal_Int32 mso_sptQuadArrowDefault[] =
{
    3, 6500, 8600, 4300
};
const SvxMSDffTextRectangles mso_sptQuadArrowTextRect[] =    // todo
{
    { { 0, 0 }, { 21600, 21600 } }
};
const SvxMSDffHandle mso_sptQuadArrowHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL,
        0x101, 0x102, 10800, 10800, 0x100, 10800, 0, 0x100 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
        0x100, 0, 10800, 10800, 0x102, 0x101, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoQuadArrow =
{
    std::span<const SvxMSDffVertPair>(mso_sptQuadArrowVert),
    const_cast<sal_uInt16*>(mso_sptQuadArrowSegm), sizeof( mso_sptQuadArrowSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptQuadArrowCalc),
    const_cast<sal_Int32*>(mso_sptQuadArrowDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptQuadArrowTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptQuadArrowHandle),
};

const SvxMSDffVertPair mso_sptLeftRightUpArrowVert[] =   // adjustment1: x 0 - 10800, adjustment2: x 0 - 10800
{                                                               // adjustment3: y 0 - 21600
    { 10800, 0 }, { 3 MSO_I, 2 MSO_I }, { 4 MSO_I, 2 MSO_I }, { 4 MSO_I, 1 MSO_I },
    { 5 MSO_I, 1 MSO_I }, { 5 MSO_I, 0 MSO_I }, { 21600, 10800 }, { 5 MSO_I, 3 MSO_I },
    { 5 MSO_I, 4 MSO_I }, { 2 MSO_I, 4 MSO_I }, { 2 MSO_I, 3 MSO_I }, { 0, 10800 },
    { 2 MSO_I, 0 MSO_I }, { 2 MSO_I, 1 MSO_I }, { 1 MSO_I, 1 MSO_I }, { 1 MSO_I, 2 MSO_I },
    { 0 MSO_I, 2 MSO_I }
};
const sal_uInt16 mso_sptLeftRightUpArrowSegm[] =
{
    0x4000, 0x0010, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptLeftRightUpArrowCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },             // 0
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },            // 1
    { 0x6001, { 0x0403, DFF_Prop_adjust3Value, 21600 } },   // 2
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } },         // 3
    { 0x8000, { 21600, 0, DFF_Prop_adjust2Value } },        // 4
    { 0x8000, { 21600, 0, 0x0402 } }                        // 5
};
const sal_Int32 mso_sptLeftRightUpArrowDefault[] =
{
    3, 6500, 8600, 6200
};
const SvxMSDffTextRectangles mso_sptLeftRightUpArrowTextRect[] =     // todo
{
    { { 0, 0 }, { 21600, 21600 } }
};
const SvxMSDffHandle mso_sptLeftRightUpArrowHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL,
        0x101, 0x102, 10800, 10800, 0x100, 10800, 0, 0x100 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
        0x100, 0, 10800, 10800, 0x102, 0x101, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoLeftRightUpArrow =
{
    std::span<const SvxMSDffVertPair>(mso_sptLeftRightUpArrowVert),
    const_cast<sal_uInt16*>(mso_sptLeftRightUpArrowSegm), sizeof( mso_sptLeftRightUpArrowSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptLeftRightUpArrowCalc),
    const_cast<sal_Int32*>(mso_sptLeftRightUpArrowDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptLeftRightUpArrowTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptLeftRightUpArrowHandle),
};

const SvxMSDffVertPair mso_sptBentArrowVert[] =  // adjustment1 : x 12427 - 21600
{                                                       // adjustment2 : y 0 - 6079
    { 0, 21600 }, { 0, 12160 }, { 12427, 1 MSO_I }, { 0 MSO_I, 1 MSO_I },
    { 0 MSO_I, 0 }, { 21600, 6079 }, { 0 MSO_I, 12158 }, { 0 MSO_I, 2 MSO_I },
    { 12427, 2 MSO_I }, { 4 MSO_I, 12160 }, { 4 MSO_I, 21600 }
};
const sal_uInt16 mso_sptBentArrowSegm[] =
{
    0x4000, 0x0001, 0xa801, 0x0006, 0xa701, 0x0001, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptBentArrowCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x8000, { 12158, 0, DFF_Prop_adjust2Value } },
    { 0x8000, { 6079, 0, DFF_Prop_adjust2Value } },
    { 0x2001, { 0x0403, 2, 1 } }
};
const sal_Int32 mso_sptBentArrowDefault[] =
{
    2, 15100, 2900
};
const SvxMSDffTextRectangles mso_sptBentArrowTextRect[] =    // todo
{
    { { 0, 0 }, { 21600, 21600 } }
};
const SvxMSDffHandle mso_sptBentArrowHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 0x101, 10800, 10800, 12427, 21600, 0, 6079 }
};
const mso_CustomShape msoBentArrow =
{
    std::span<const SvxMSDffVertPair>(mso_sptBentArrowVert),
    const_cast<sal_uInt16*>(mso_sptBentArrowSegm), sizeof( mso_sptBentArrowSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptBentArrowCalc),
    const_cast<sal_Int32*>(mso_sptBentArrowDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptBentArrowTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptBentArrowHandle),
};

const SvxMSDffVertPair mso_sptUturnArrowVert[] =
{
    { 0, 21600 }, { 0, 8550 },                                          // pp
    { 0, 3540 }, { 4370, 0 }, { 9270, 0 },                              // ccp
    { 13890, 0 }, { 18570, 3230 }, { 18600, 8300 },                     // ccp
    { 21600, 8300 }, { 15680, 14260 }, { 9700, 8300 }, { 12500, 8300 }, // pppp
    { 12320, 6380 }, { 10870, 5850 }, { 9320, 5850 },                   // ccp
    { 7770, 5850 }, { 6040, 6410 }, { 6110, 8520 },                     // ccp
    { 6110, 21600 }
};
const sal_uInt16 mso_sptUturnArrowSegm[] =
{
    0x4000, 0x0001, 0x2002, 0x0004, 0x2002, 0x0001, 0x6000, 0x8000
};
const SvxMSDffTextRectangles mso_sptUturnArrowTextRect[] =
{
    { { 0, 8280 }, { 6110, 21600 } }
};
const mso_CustomShape msoUturnArrow =
{
    std::span<const SvxMSDffVertPair>(mso_sptUturnArrowVert),
    const_cast<sal_uInt16*>(mso_sptUturnArrowSegm), sizeof( mso_sptUturnArrowSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptUturnArrowTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptLeftUpArrowVert[] =    // adjustment1 : x 0 - 21600, adjustment2 : x 0 - 21600
{                                                           // adjustment3 : y 0 - 21600
    { 0, 5 MSO_I }, { 2 MSO_I, 0 MSO_I }, { 2 MSO_I, 7 MSO_I }, { 7 MSO_I, 7 MSO_I },
    { 7 MSO_I, 2 MSO_I }, { 0 MSO_I, 2 MSO_I }, { 5 MSO_I, 0 }, { 21600, 2 MSO_I },
    { 1 MSO_I, 2 MSO_I }, { 1 MSO_I, 1 MSO_I }, { 2 MSO_I, 1 MSO_I }, { 2 MSO_I, 21600 }
};
const sal_uInt16 mso_sptLeftUpArrowSegm[] =
{
    0x4000, 0x000b, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptLeftUpArrowCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },     // 0
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },        // 1
    { 0x2000, { DFF_Prop_adjust3Value, 0, 0 } },        // 2
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } }, // 3
    { 0x2001, { 0x0403, 1, 2 } },                       // 4
    { 0x6000, { DFF_Prop_adjustValue, 0x0404, 0 } },    // 5
    { 0x8000, { 21600, 0, DFF_Prop_adjust2Value } },    // 6
    { 0x6000, { DFF_Prop_adjustValue, 0x0406, 0 } },    // 7
    { 0x8000, { 21600, 0, 0x406 } },                    // 8
    { 0xa000, { 0x408, 0, 0x406 } }                 // 9
};
const sal_Int32 mso_sptLeftUpArrowDefault[] =
{
    3, 9340, 18500, 6200
};
const SvxMSDffTextRectangles mso_sptLeftUpArrowTextRect[] =
{
    { { 2 MSO_I, 7 MSO_I }, { 1 MSO_I, 1 MSO_I } },
    { { 7 MSO_I, 2 MSO_I }, { 1 MSO_I, 1 MSO_I } }
};
const SvxMSDffHandle mso_sptLeftUpArrowHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL,
        0x101, 0x102, 10800, 10800, 3 + 5, 21600, 0, 0x100 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
        0x100, 0, 10800, 10800, 0x102, 3 + 9, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoLeftUpArrow =
{
    std::span<const SvxMSDffVertPair>(mso_sptLeftUpArrowVert),
    const_cast<sal_uInt16*>(mso_sptLeftUpArrowSegm), sizeof( mso_sptLeftUpArrowSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptLeftUpArrowCalc),
    const_cast<sal_Int32*>(mso_sptLeftUpArrowDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptLeftUpArrowTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptLeftUpArrowHandle),
};

const SvxMSDffVertPair mso_sptBentUpArrowVert[] =    // adjustment1 : x 0 - 21600, adjustment2 : x 0 - 21600
{                                                           // adjustment3 : y 0 - 21600
    { 0, 8 MSO_I }, { 7 MSO_I, 8 MSO_I }, { 7 MSO_I, 2 MSO_I }, { 0 MSO_I, 2 MSO_I },
    { 5 MSO_I, 0 }, { 21600, 2 MSO_I }, { 1 MSO_I, 2 MSO_I }, { 1 MSO_I, 21600 },
    { 0, 21600 }
};
const sal_uInt16 mso_sptBentUpArrowSegm[] =
{
    0x4000, 0x0008, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptBentUpArrowCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },     // 0
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },        // 1
    { 0x2000, { DFF_Prop_adjust3Value, 0, 0 } },        // 2
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } }, // 3
    { 0x2001, { 0x0403, 1, 2 } },                       // 4
    { 0x6000, { DFF_Prop_adjustValue, 0x0404, 0 } },    // 5
    { 0x8000, { 21600, 0, DFF_Prop_adjust2Value } },    // 6
    { 0x6000, { DFF_Prop_adjustValue, 0x0406, 0 } },    // 7
    { 0x6000, { 0x0407, 0x0406, 0 } },                  // 8
    { 0x8000, { 21600, 0, 0x406 } },                    // 9
    { 0xa000, { 0x409, 0, 0x406 } }                 // a
};
const sal_Int32 mso_sptBentUpArrowDefault[] =
{
    3, 9340, 18500, 7200
};
const SvxMSDffTextRectangles mso_sptBentUpArrowTextRect[] =
{
    { { 2 MSO_I, 7 MSO_I }, { 1 MSO_I, 1 MSO_I } },
    { { 7 MSO_I, 2 MSO_I }, { 1 MSO_I, 1 MSO_I } }
};
const SvxMSDffHandle mso_sptBentUpArrowHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL,
        0x101, 0x102, 10800, 10800, 3 + 5, 21600, 0, 0x100 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
        0x100, 0, 10800, 10800, 0x102, 3 + 10, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoBentUpArrow =
{
    std::span<const SvxMSDffVertPair>(mso_sptBentUpArrowVert),
    const_cast<sal_uInt16*>(mso_sptBentUpArrowSegm), sizeof( mso_sptBentUpArrowSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptBentUpArrowCalc),
    const_cast<sal_Int32*>(mso_sptBentUpArrowDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptBentUpArrowTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptBentUpArrowHandle),
};

const SvxMSDffVertPair mso_sptCurvedRightVert[] =
{
    { 0, 0 }, { 23 MSO_I, 3 MSO_I }, { 22 MSO_I, 0 }, { 0, 4 MSO_I }, { 0, 15 MSO_I }, { 23 MSO_I, 1 MSO_I }, { 0, 7 MSO_I }, { 2 MSO_I, 13 MSO_I },
    { 2 MSO_I, 14 MSO_I }, { 22 MSO_I, 8 MSO_I }, { 2 MSO_I, 12 MSO_I },
    { 0, 0 }, { 23 MSO_I, 3 MSO_I }, { 2 MSO_I, 11 MSO_I }, { 26 MSO_I, 17 MSO_I }, { 0, 15 MSO_I }, { 23 MSO_I, 1 MSO_I }, { 26 MSO_I, 17 MSO_I }, { 22 MSO_I, 15 MSO_I },
    { 0, 0 }, { 23 MSO_I, 3 MSO_I }, { 22 MSO_I, 0 }, { 0, 4 MSO_I },
    { 0, 0 }, { 23 MSO_I, 3 MSO_I }, { 0, 4 MSO_I }, { 26 MSO_I, 17 MSO_I },
    { 0, 15 MSO_I }, { 23 MSO_I, 1 MSO_I }, { 26 MSO_I, 17 MSO_I }, { 22 MSO_I, 15 MSO_I }
};
const sal_uInt16 mso_sptCurvedRightSegm[] =
{
    0xa404,
    0xa304,
    0x0003,
    0xa508,
    0x6000,
    0x8000,
    0xa404,
    0xa304,
    0xa504,
    0x6000,
    0x8000
};
const SvxMSDffCalculationData mso_sptCurvedRightCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust3Value, 0, 0 } },
    { 0xa000, { DFF_Prop_adjustValue, 21600, DFF_Prop_adjust2Value } },
    { 0x2001, { 0x403, 1, 2 } },
    { 0x6000, { DFF_Prop_adjust2Value, DFF_Prop_adjust2Value, 21600 } },
    { 0xe000, { 0x405, DFF_Prop_adjust2Value, DFF_Prop_adjustValue } },
    { 0x2001, { 0x406, 1, 2 } },
    { 0x4002, { 21600, DFF_Prop_adjustValue, 0 } },
    { 0x8000, { 21600, 0, DFF_Prop_adjust3Value } },
    { 0xa00f, { 0x409, 21600, 0x404 } },
    { 0x6000, { 0x404, 0x40a, 0 } },
    { 0x6000, { 0x40b, DFF_Prop_adjust2Value, 21600 } },
    { 0x6000, { 0x407, 0x40a, 0 } },
    { 0xa000, { 0x40c, 21600, DFF_Prop_adjustValue } },
    { 0xa000, { 0x405, 0, DFF_Prop_adjustValue } },
    { 0x2001, { 0x40f, 1, 2 } },
    { 0x6002, { 0x404, 0x407, 0 } },
    { 0x6000, { DFF_Prop_adjustValue, DFF_Prop_adjust2Value, 21600 } },
    { 0x2001, { 0x412, 1, 2 } },
    { 0xa000, { 0x411, 0, 0x413 } },
    { 0x0000, { 21600, 0, 0 } },
    { 0x0000, { 21600, 0, 0 } },
    { 0x0001, { 21600, 2, 1 } },
    { 0xa000, { 0x411, 0, 0x404 } },
    { 0x600f, { 0x418, 0x404, 21600 } },
    { 0x8000, { 21600, 0, 0x419 } },
    { 0x2000, { 0x408, 128, 0 } },
    { 0x2001, { 0x405, 1, 2 } },
    { 0x2000, { 0x405, 0, 128 } },
    { 0xe000, { DFF_Prop_adjustValue, 0x411, 0x40c } },
    { 0x600f, { 0x414, 0x404, 21600 } },
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } },
    { 0x2001, { 0x420, 1, 2 } },
    { 0x0001, { 21600, 21600, 1 } },
    { 0x6001, { 0x409, 0x409, 1 } },
    { 0xa000, { 0x422, 0, 0x423 } },
    { 0x200d, { 0x424, 0, 0 } },
    { 0x2000, { 0x425, 21600, 0 } },
    { 0x8001, { 21600, 21600, 0x426 } },
    { 0x2000, { 0x427, 64, 0 } },
    { 0x2001, { DFF_Prop_adjustValue, 1, 2 } },
    { 0x600f, { 0x421, 0x429, 21600 } },
    { 0x8000, { 21600, 0, 0x42a } },
    { 0x2000, { 0x42b, 64, 0 } },
    { 0x2001, { 0x404, 1, 2 } },
    { 0xa000, { DFF_Prop_adjust2Value, 0, 0x42d } },
    { 0x0001, { 21600, 2195, 16384 } },
    { 0x0001, { 21600, 14189, 16384 } }
};
const sal_Int32 mso_sptCurvedRightDefault[] =
{
    3, 12960, 19440, 14400
};
const SvxMSDffTextRectangles mso_sptCurvedRightTextRect[] =
{
    { { 47 MSO_I, 45 MSO_I }, { 48 MSO_I, 46 MSO_I } }
};
const SvxMSDffVertPair mso_sptCurvedRightGluePoints[] =
{
    { 0, 17 MSO_I }, { 2 MSO_I, 14 MSO_I }, { 22 MSO_I, 8 MSO_I }, { 2 MSO_I, 12 MSO_I }, { 22 MSO_I, 16 MSO_I }
};
const SvxMSDffHandle mso_sptCurvedRightHandles[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        21600, 0x100, 10800, 10800, 0, 10800, 3 + 40, 3 + 29 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        21600, 0x101, 10800, 10800, 0, 10800, 3 + 27, 3 + 21 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
        0x102, 21600, 10800, 10800, 3 + 44, 3 + 22, 3375, 21600 }
};
const mso_CustomShape msoCurvedRightArrow =
{
    std::span<const SvxMSDffVertPair>(mso_sptCurvedRightVert),
    const_cast<sal_uInt16*>(mso_sptCurvedRightSegm), sizeof( mso_sptCurvedRightSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCurvedRightCalc),
    const_cast<sal_Int32*>(mso_sptCurvedRightDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptCurvedRightTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptCurvedRightGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptCurvedRightHandles),
};

const SvxMSDffVertPair mso_sptCurvedDownVert[] =
{
   { 0, 0 }, { 3 MSO_I, 23 MSO_I }, { 0, 22 MSO_I }, { 4 MSO_I, 0 },
   { 15 MSO_I, 0 }, { 1 MSO_I, 23 MSO_I }, { 7 MSO_I, 0 }, { 13 MSO_I, 2 MSO_I },
   { 14 MSO_I, 2 MSO_I }, { 8 MSO_I, 22 MSO_I }, { 12 MSO_I, 2 MSO_I },
   { 0, 0 }, { 3 MSO_I, 23 MSO_I }, { 11 MSO_I, 2 MSO_I }, { 17 MSO_I, 26 MSO_I }, { 15 MSO_I, 0 },
   { 1 MSO_I, 23 MSO_I }, { 17 MSO_I, 26 MSO_I }, { 15 MSO_I, 22 MSO_I },
   { 0, 0 }, { 3 MSO_I, 23 MSO_I }, { 0, 22 MSO_I }, { 4 MSO_I, 0 },
   { 0, 0 }, { 3 MSO_I, 23 MSO_I }, { 4 MSO_I, 0 }, { 17 MSO_I, 26 MSO_I },
   { 15 MSO_I, 0 }, { 1 MSO_I, 23 MSO_I }, { 17 MSO_I, 26 MSO_I }, { 15 MSO_I, 22 MSO_I }
};
const sal_uInt16 mso_sptCurvedDownSegm[] =
{
    0xa604,
    0xa504,
    0x0003,
    0xa308,
    0x6000,
    0x8000,
   0xa604,
   0xa504,
   0xa304,
   0x6000,
   0x8000
};
const SvxMSDffTextRectangles mso_sptCurvedDownTextRect[] =
{
   { { 45 MSO_I, 47 MSO_I }, { 46 MSO_I, 48 MSO_I } }
};
const SvxMSDffVertPair mso_sptCurvedDownGluePoints[] =
{
   { 17 MSO_I, 0 }, { 16 MSO_I, 22 MSO_I }, { 12 MSO_I, 2 MSO_I }, { 8 MSO_I, 22 MSO_I }, { 14 MSO_I, 2 MSO_I }
};
const SvxMSDffHandle mso_sptCurvedDownHandles[] =
{
   {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
       0x100, 21600, 10800, 10800, 3 + 40, 3 + 29, 0, 10800 },
   {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
       0x101, 21600, 10800, 10800, 3 + 27, 3 + 21, 0, 10800 },
   {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
       21600, 0x102, 10800, 10800, 3375, 21600, 3 + 44, 3 + 22 }
};

const mso_CustomShape msoCurvedDownArrow =
{
   std::span<const SvxMSDffVertPair>(mso_sptCurvedDownVert),
   const_cast<sal_uInt16*>(mso_sptCurvedDownSegm), sizeof( mso_sptCurvedDownSegm ) >> 1,
   std::span<const SvxMSDffCalculationData>(mso_sptCurvedRightCalc),
   const_cast<sal_Int32*>(mso_sptCurvedRightDefault),
   std::span<const SvxMSDffTextRectangles>(mso_sptCurvedDownTextRect),
   21600, 21600,
   MIN_INT32, MIN_INT32,
   std::span<const SvxMSDffVertPair>(mso_sptCurvedDownGluePoints),
   std::span<const SvxMSDffHandle>(mso_sptCurvedDownHandles),
};

const SvxMSDffVertPair mso_sptCurvedUpVert[] =
{
   { 0, 22 MSO_I }, { 3 MSO_I, 21 MSO_I }, { 0, 0 }, { 4 MSO_I, 21 MSO_I },
   { 14 MSO_I, 22 MSO_I }, { 1 MSO_I, 21 MSO_I }, { 7 MSO_I, 21 MSO_I },
   { 12 MSO_I, 2 MSO_I }, { 13 MSO_I, 2 MSO_I }, { 8 MSO_I, 0 }, { 11 MSO_I, 2 MSO_I },
   { 0, 22 MSO_I }, { 3 MSO_I, 21 MSO_I }, { 10 MSO_I, 2 MSO_I }, { 16 MSO_I, 24 MSO_I },
   { 14 MSO_I, 22 MSO_I }, { 1 MSO_I, 21 MSO_I }, { 16 MSO_I, 24 MSO_I }, { 14 MSO_I, 0 },
   { 0, 22 MSO_I }, { 3 MSO_I, 21 MSO_I }, { 0, 0 }, { 4 MSO_I, 21 MSO_I },
   { 14 MSO_I, 22 MSO_I }, { 1 MSO_I, 21 MSO_I }, { 7 MSO_I, 21 MSO_I }, { 16 MSO_I, 24 MSO_I },
   { 14 MSO_I, 22 MSO_I }, { 1 MSO_I, 21 MSO_I }, { 16 MSO_I, 24 MSO_I }, { 14 MSO_I, 0 }
};
const sal_uInt16 mso_sptCurvedUpSegm[] =
{
   0xa404,
   0xa304,
   0x0003,
   0xa508,
   0x6000,
   0x8000,
   0xa404,
   0xa508,
   0xa504,
   0x6000,
   0x8000
};
const SvxMSDffCalculationData mso_sptCurvedUpCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust3Value, 0, 0 } },
    { 0xa000, { DFF_Prop_adjustValue, 21600, DFF_Prop_adjust2Value } },
    { 0x2001, { 0x403, 1, 2 } },
    { 0x6000, { DFF_Prop_adjust2Value, DFF_Prop_adjust2Value, 21600 } },
    { 0xe000, { 0x405, DFF_Prop_adjust2Value, DFF_Prop_adjustValue } },
    { 0x2001, { 0x406, 1, 2 } },
    { 0x4002, { 21600, DFF_Prop_adjustValue, 0 } },
    { 0xa00f, { DFF_Prop_adjust3Value, 21600, 0x404 } },
    { 0x6000, { 0x404, 0x409, 0 } },
    { 0x6000, { 0x40a, DFF_Prop_adjust2Value, 21600 } },
    { 0x6000, { 0x407, 0x409, 0 } },
    { 0xa000, { 0x40b, 21600, DFF_Prop_adjustValue } },
    { 0xa000, { 0x405, 0, DFF_Prop_adjustValue } },
    { 0x2001, { 0x40e, 1, 2 } },
    { 0x6002, { 0x404, 0x407, 0 } },
    { 0x6000, { DFF_Prop_adjustValue, DFF_Prop_adjust2Value, 21600 } },
    { 0x2001, { 0x411, 1, 2 } },
    { 0xa000, { 0x410, 0, 0x412 } },
    { 0x0000, { 21600, 0, 0 } },
    { 0x0000, { 21600, 0, 0 } },
    { 0x0000, { 0, 0, 21600 } },
    { 0xa000, { 0x410, 0, 0x404 } },
    { 0x600f, { 0x417, 0x404, 21600 } },
    { 0x2000, { 0x408, 128, 0 } },
    { 0x2001, { 0x405, 1, 2 } },
    { 0x2000, { 0x405, 0, 128 } },
    { 0xe000, { DFF_Prop_adjustValue, 0x410, 0x40b } },
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } },
    { 0x2001, { 0x41d, 1, 2 } },
    { 0x0001, { 21600, 21600, 1 } },
    { 0x6001, { DFF_Prop_adjust3Value, DFF_Prop_adjust3Value, 1 } },
    { 0xa000, { 0x41f, 0, 0x420 } },
    { 0x200d, { 0x421, 0, 0 } },
    { 0x2000, { 0x422, 21600, 0 } },
    { 0x8001, { 21600, 21600, 0x423 } },
    { 0x2000, { 0x424, 64, 0 } },
    { 0x2001, { DFF_Prop_adjustValue, 1, 2 } },
    { 0x600f, { 0x41e, 0x426, 21600 } },
    { 0x2000, { 0x427, 0, 64 } },
    { 0x2001, { 0x404, 1, 2 } },
    { 0xa000, { DFF_Prop_adjust2Value, 0, 0x429 } },
    { 0x0001, { 21600, 2195, 16384 } },
    { 0x0001, { 21600, 14189, 16384 } }
};
const sal_Int32 mso_sptCurvedUpDefault[] =
{
    3, 12960, 19440, 7200
};
const SvxMSDffTextRectangles mso_sptCurvedUpTextRect[] =
{
   { { 41 MSO_I, 43 MSO_I }, { 42 MSO_I, 44 MSO_I } }
};
const SvxMSDffVertPair mso_sptCurvedUpGluePoints[] =
{
   { 8 MSO_I, 0 }, { 11 MSO_I, 2 MSO_I }, { 15 MSO_I, 0 }, { 16 MSO_I, 21 MSO_I }, { 13 MSO_I, 2 MSO_I }
};
const SvxMSDffHandle mso_sptCurvedUpHandles[] =
{
   {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
       0x100, 0, 10800, 10800, 3 + 37, 3 + 27, 0, 10800 },
   {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
       0x101, 0, 10800, 10800, 3 + 25, 3 + 20, 0, 10800 },
   {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
       21600, 0x102, 10800, 10800, 3375, 21600, 0, 3 + 40 }
};
const mso_CustomShape msoCurvedUpArrow =
{
   std::span<const SvxMSDffVertPair>(mso_sptCurvedUpVert),
   const_cast<sal_uInt16*>(mso_sptCurvedUpSegm), sizeof( mso_sptCurvedUpSegm ) >> 1,
   std::span<const SvxMSDffCalculationData>(mso_sptCurvedUpCalc),
   const_cast<sal_Int32*>(mso_sptCurvedUpDefault),
   std::span<const SvxMSDffTextRectangles>(mso_sptCurvedUpTextRect),
   21600, 21600,
   MIN_INT32, MIN_INT32,
   std::span<const SvxMSDffVertPair>(mso_sptCurvedUpGluePoints),
   std::span<const SvxMSDffHandle>(mso_sptCurvedUpHandles),
};

const SvxMSDffVertPair mso_sptCurvedLeftVert[] =
{
   { 22 MSO_I, 0 }, { 21 MSO_I, 3 MSO_I }, { 0, 0 }, { 21 MSO_I, 4 MSO_I },
   { 22 MSO_I, 14 MSO_I }, { 21 MSO_I, 1 MSO_I }, { 21 MSO_I, 7 MSO_I }, { 2 MSO_I, 12 MSO_I },
   { 2 MSO_I, 13 MSO_I }, { 0, 8 MSO_I }, { 2 MSO_I, 11 MSO_I },
   { 22 MSO_I, 0 }, { 21 MSO_I, 3 MSO_I }, { 2 MSO_I, 10 MSO_I }, { 24 MSO_I, 16 MSO_I },
   { 22 MSO_I, 14 MSO_I }, { 21 MSO_I, 1 MSO_I }, { 24 MSO_I, 16 MSO_I }, { 0, 14 MSO_I },
   { 22 MSO_I, 0 }, { 21 MSO_I, 3 MSO_I }, { 0, 0 }, { 21 MSO_I, 4 MSO_I },
   { 22 MSO_I, 14 MSO_I }, { 21 MSO_I, 1 MSO_I }, { 21 MSO_I, 7 MSO_I }, { 24 MSO_I, 16 MSO_I },
   { 22 MSO_I, 14 MSO_I }, { 21 MSO_I, 1 MSO_I }, { 24 MSO_I, 16 MSO_I }, { 0, 14 MSO_I }
};
const sal_uInt16 mso_sptCurvedLeftSegm[] =
{
   0xa604,
   0xa504,
   0x0003,
   0xa308,
   0x6000,
   0x8000,
   0xa604,
   0xa308,
   0x6000,
   0x8000
};
const SvxMSDffTextRectangles mso_sptCurvedLeftTextRect[] =
{
    { { 43 MSO_I, 41 MSO_I }, { 44 MSO_I, 42 MSO_I } }
};
const SvxMSDffVertPair mso_sptCurvedLeftGluePoints[] =
{
    { 0, 15 MSO_I }, { 2 MSO_I, 11 MSO_I }, { 0, 8 MSO_I }, { 2 MSO_I, 13 MSO_I }, { 21 MSO_I, 16 MSO_I }
};
const SvxMSDffHandle mso_sptCurvedLeftHandles[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        0, 0x100, 10800, 10800, 0, 10800, 3 + 37, 3 + 27 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        0, 0x101, 10800, 10800, 0, 10800, 3 + 25, 3 + 20 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
        0x102, 21600, 10800, 10800, 0, 3 + 40, 3375, 21600 }
};
const mso_CustomShape msoCurvedLeftArrow =
{
    std::span<const SvxMSDffVertPair>(mso_sptCurvedLeftVert),
    const_cast<sal_uInt16*>(mso_sptCurvedLeftSegm), sizeof( mso_sptCurvedLeftSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCurvedUpCalc),
    const_cast<sal_Int32*>(mso_sptCurvedUpDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptCurvedLeftTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptCurvedLeftGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptCurvedLeftHandles),
};

const SvxMSDffVertPair mso_sptStripedRightArrowVert[] =  // adjustment1 : x 3375 - 21600
{                                                               // adjustment2 : y 0 - 10800
    { 3375, 0 MSO_I }, { 1 MSO_I, 0 MSO_I }, { 1 MSO_I, 0 }, { 21600, 10800 },
    { 1 MSO_I, 21600 }, { 1 MSO_I, 2 MSO_I }, { 3375, 2 MSO_I }, { 0, 0 MSO_I },
    { 675, 0 MSO_I }, { 675, 2 MSO_I }, { 0, 2 MSO_I }, { 1350, 0 MSO_I },
    { 2700, 0 MSO_I }, { 2700, 2 MSO_I }, { 1350, 2 MSO_I }
};
const sal_uInt16 mso_sptStripedRightArrowSegm[] =
{
    0x4000, 0x0006, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptStripedRightArrowCalc[] =
{
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x8000, { 21600, 0, DFF_Prop_adjust2Value } },
    { 0x8000, { 21600, 0, 0x401 } },
    { 0x6001, { 0x403, 0x400, 10800 } },
    { 0x6000, { 0x401, 0x404, 0 } }
};
const SvxMSDffTextRectangles mso_sptStripedRightArrowTextRect[] =
{
    { { 3375, 0 MSO_I }, { 5 MSO_I, 2 MSO_I } }
};
const SvxMSDffHandle mso_sptStripedRightArrowHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 0x101, 10800, 10800, 3375, 21600, 0, 10800 }
};
const mso_CustomShape msoStripedRightArrow =
{
    std::span<const SvxMSDffVertPair>(mso_sptStripedRightArrowVert),
    const_cast<sal_uInt16*>(mso_sptStripedRightArrowSegm), sizeof( mso_sptStripedRightArrowSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptStripedRightArrowCalc),
    const_cast<sal_Int32*>(mso_sptDefault16200and5400),
    std::span<const SvxMSDffTextRectangles>(mso_sptStripedRightArrowTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptStripedRightArrowHandle),
};

const SvxMSDffVertPair mso_sptNotchedRightArrowVert[] =  // adjustment1 : x 0 - 21600    (default 16200)
{                                                               // adjustment2 : y 0 - 10800    (default 5400)
    { 0, 1 MSO_I }, { 0 MSO_I, 1 MSO_I }, { 0 MSO_I, 0 }, { 21600, 10800 },
    { 0 MSO_I, 21600 }, { 0 MSO_I, 2 MSO_I }, { 0, 2 MSO_I }, { 5 MSO_I, 10800 }, { 0, 1 MSO_I }
};
const SvxMSDffCalculationData mso_sptNotchedRightArrowCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x8000, { 21600, 0, DFF_Prop_adjust2Value } },
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } },
    { 0x8000, { 10800, 0, DFF_Prop_adjust2Value } },
    { 0x6001, { 0x403, 0x404, 10800 } },
    { 0x8000, { 21600, 0, 0x405 }}
};
const SvxMSDffTextRectangles mso_sptNotchedRightArrowTextRect[] =    // todo
{
    { { 5 MSO_I, 1 MSO_I }, { 6 MSO_I, 2 MSO_I } }
};
const SvxMSDffHandle mso_sptNotchedRightArrowHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 0x101, 10800, 10800, 0, 21600, 0, 10800 }
};
const mso_CustomShape msoNotchedRightArrow =
{
    std::span<const SvxMSDffVertPair>(mso_sptNotchedRightArrowVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(mso_sptNotchedRightArrowCalc),
    const_cast<sal_Int32*>(mso_sptDefault16200and5400),
    std::span<const SvxMSDffTextRectangles>(mso_sptNotchedRightArrowTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptNotchedRightArrowHandle),
};

const SvxMSDffVertPair mso_sptHomePlateVert[] =  // adjustment1 : x 0 - 21600
{
    { 0, 0 }, { 0 MSO_I, 0 }, { 21600, 10800 }, { 0 MSO_I, 21600 },
    { 0, 21600 }
};
const sal_uInt16 mso_sptHomePlateSegm[] =
{
    0x4000, 0x0004, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptHomePlateCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } }
};
const sal_Int32 mso_sptHomePlateDefault[] =
{
    1, 16200
};
const SvxMSDffTextRectangles mso_sptHomePlateTextRect[] =    // todo
{
    { { 0, 0 }, { 21600, 21600 } }
};
const SvxMSDffHandle mso_sptHomePlateHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 0, 10800, 10800, 0, 21600, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoHomePlate =
{
    std::span<const SvxMSDffVertPair>(mso_sptHomePlateVert),
    const_cast<sal_uInt16*>(mso_sptHomePlateSegm), sizeof( mso_sptHomePlateSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptHomePlateCalc),
    const_cast<sal_Int32*>(mso_sptHomePlateDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptHomePlateTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptHomePlateHandle),
};

const SvxMSDffVertPair mso_sptChevronVert[] =    // adjustment1 : x 0 - 21600
{
    { 0, 0 }, { 0 MSO_I, 0 }, { 21600, 10800 }, { 0 MSO_I, 21600 },
    { 0, 21600 }, { 1 MSO_I, 10800 }
};
const sal_uInt16 mso_sptChevronSegm[] =
{
    0x4000, 0x0005, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptChevronCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x8000, { 21600, 0, 0x0400 } }
};
const sal_Int32 mso_sptChevronDefault[] =
{
    1, 16200
};
const SvxMSDffTextRectangles mso_sptChevronTextRect[] =  // todo
{
    { { 0, 0 }, { 21600, 21600 } }
};
const SvxMSDffHandle mso_sptChevronHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 0, 10800, 10800, 0, 21600, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoChevron =
{
    std::span<const SvxMSDffVertPair>(mso_sptChevronVert),
    const_cast<sal_uInt16*>(mso_sptChevronSegm), sizeof( mso_sptChevronSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptChevronCalc),
    const_cast<sal_Int32*>(mso_sptChevronDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptChevronTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptChevronHandle),
};

const SvxMSDffVertPair mso_sptRightArrowCalloutVert[] =  // adjustment1 : x 0 - 21000
{                                                               // adjustment2 : y 0 - 10800
    { 0, 0 }, { 0 MSO_I, 0 }, { 0 MSO_I, 3 MSO_I }, { 2 MSO_I, 3 MSO_I },
    { 2 MSO_I, 1 MSO_I }, { 21600, 10800 }, { 2 MSO_I, 4 MSO_I }, { 2 MSO_I, 5 MSO_I },
    { 0 MSO_I, 5 MSO_I }, { 0 MSO_I, 21600 }, { 0, 21600 }
};
const sal_uInt16 mso_sptRightArrowCalloutSegm[] =
{
    0x4000, 0x000a, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptRightArrowCalloutCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust3Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust4Value, 0, 0 } },
    { 0x8000, { 21600, 0, 0x0401 } },
    { 0x8000, { 21600, 0, 0x0403 } }
};
const sal_Int32 mso_sptRightArrowCalloutDefault[] =
{
    4, 14400, 5400, 18000, 8100
};
const SvxMSDffTextRectangles mso_sptRightArrowCalloutTextRect[] =
{
    { { 0, 0 }, { 0 MSO_I, 21600 } }
};
const SvxMSDffHandle mso_sptRightArrowCalloutHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
        0x100, 0, 10800, 10800, 0, 0x102, MIN_INT32, 0x7fffffff },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL,
        0x102, 0x103, 10800, 10800, 0x100, 21600, 0x101, 10800 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        1, 0x101, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 0x103 }
};
const mso_CustomShape msoRightArrowCallout =
{
    std::span<const SvxMSDffVertPair>(mso_sptRightArrowCalloutVert),
    const_cast<sal_uInt16*>(mso_sptRightArrowCalloutSegm), sizeof( mso_sptRightArrowCalloutSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptRightArrowCalloutCalc),
    const_cast<sal_Int32*>(mso_sptRightArrowCalloutDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptRightArrowCalloutTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptRightArrowCalloutHandle),
};

const SvxMSDffVertPair mso_sptLeftArrowCalloutVert[] =   // adjustment1 : x 0 - 21600, adjustment2 : y 0 - 10800
{                                                               // adjustment3 : x 0 - 21600, adjustment4 : y 0 - 10800
    { 0 MSO_I, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0 MSO_I, 21600 },
    { 0 MSO_I, 5 MSO_I }, { 2 MSO_I, 5 MSO_I }, { 2 MSO_I, 4 MSO_I }, { 0, 10800 },
    { 2 MSO_I, 1 MSO_I }, { 2 MSO_I, 3 MSO_I }, { 0 MSO_I, 3 MSO_I }
};
const sal_uInt16 mso_sptLeftArrowCalloutSegm[] =
{
    0x4000, 0x000a, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptLeftArrowCalloutCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust3Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust4Value, 0, 0 } },
    { 0x8000, { 21600, 0, 0x0401 } },
    { 0x8000, { 21600, 0, 0x0403 } }
};
const sal_Int32 mso_sptLeftArrowCalloutDefault[] =
{
    4, 7200, 5400, 3600, 8100
};
const SvxMSDffTextRectangles mso_sptLeftArrowCalloutTextRect[] =
{
    { { 0 MSO_I, 0 }, { 21600, 21600 } }
};
const SvxMSDffHandle mso_sptLeftArrowCalloutHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL,
        0x100, 0, 10800, 10800, 0x102, 21600, MIN_INT32, 0x7fffffff },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL,
        0x102, 0x103, 10800, 10800, 0, 0x100, 0x101, 10800 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        0, 0x101, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 0x103 }
};
const mso_CustomShape msoLeftArrowCallout =
{
    std::span<const SvxMSDffVertPair>(mso_sptLeftArrowCalloutVert),
    const_cast<sal_uInt16*>(mso_sptLeftArrowCalloutSegm), sizeof( mso_sptLeftArrowCalloutSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptLeftArrowCalloutCalc),
    const_cast<sal_Int32*>(mso_sptLeftArrowCalloutDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptLeftArrowCalloutTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptLeftArrowCalloutHandle),
};

const SvxMSDffVertPair mso_sptUpArrowCalloutVert[] =
{
    { 21600, 0 MSO_I }, { 21600, 21600 }, { 0, 21600 }, { 0, 0 MSO_I },
    { 3 MSO_I, 0 MSO_I }, { 3 MSO_I, 2 MSO_I }, { 1 MSO_I, 2 MSO_I }, { 10800, 0 },
    { 4 MSO_I, 2 MSO_I }, { 5 MSO_I, 2 MSO_I }, { 5 MSO_I, 0 MSO_I }
};
const sal_uInt16 mso_sptUpArrowCalloutSegm[] =
{
    0x4000, 0x000a, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptUpArrowCalloutCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust3Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust4Value, 0, 0 } },
    { 0x8000, { 21600, 0, 0x0401 } },
    { 0x8000, { 21600, 0, 0x0403 } }
};
const sal_Int32 mso_sptUpArrowCalloutDefault[] =
{
    4, 7200, 5400, 3600, 8100
};
const SvxMSDffTextRectangles mso_sptUpArrowCalloutTextRect[] =
{
    { { 0, 0 MSO_I }, { 21600, 21600 } }
};
const SvxMSDffHandle mso_sptUpArrowCalloutHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0x102, 21600 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        0x103, 0x102, 10800, 10800, 0x101, 10800, 0, 0x100 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
        0x101, 0, 10800, 10800, 0, 0x103, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoUpArrowCallout =
{
    std::span<const SvxMSDffVertPair>(mso_sptUpArrowCalloutVert),
    const_cast<sal_uInt16*>(mso_sptUpArrowCalloutSegm), sizeof( mso_sptUpArrowCalloutSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptUpArrowCalloutCalc),
    const_cast<sal_Int32*>(mso_sptUpArrowCalloutDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptUpArrowCalloutTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptUpArrowCalloutHandle),
};

const SvxMSDffVertPair mso_sptDownArrowCalloutVert[] =
{
    { 0, 0 MSO_I }, { 0, 0 }, { 21600, 0 }, { 21600, 0 MSO_I },
    { 5 MSO_I, 0 MSO_I }, { 5 MSO_I, 2 MSO_I }, { 4 MSO_I, 2 MSO_I }, { 10800, 21600 },
    { 1 MSO_I, 2 MSO_I }, { 3 MSO_I, 2 MSO_I }, { 3 MSO_I, 0 MSO_I }
};
const sal_uInt16 mso_sptDownArrowCalloutSegm[] =
{
    0x4000, 0x000a, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptDownArrowCalloutCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust3Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust4Value, 0, 0 } },
    { 0x8000, { 21600, 0, 0x0401 } },
    { 0x8000, { 21600, 0, 0x0403 } }
};
const sal_Int32 mso_sptDownArrowCalloutDefault[] =
{
    4, 14400, 5400, 18000, 8100
};
const SvxMSDffTextRectangles mso_sptDownArrowCalloutTextRect[] =
{
    { { 0, 0 }, { 21600, 0 MSO_I } }
};
const SvxMSDffHandle mso_sptDownArrowCalloutHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 0x102 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL,
        0x103, 0x102, 10800, 10800, 0x101, 10800, 0x100, 21600 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
        0x101, 1, 10800, 10800, 0, 0x103, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoDownArrowCallout =
{
    std::span<const SvxMSDffVertPair>(mso_sptDownArrowCalloutVert),
    const_cast<sal_uInt16*>(mso_sptDownArrowCalloutSegm), sizeof( mso_sptDownArrowCalloutSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptDownArrowCalloutCalc),
    const_cast<sal_Int32*>(mso_sptDownArrowCalloutDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptDownArrowCalloutTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptDownArrowCalloutHandle),
};

const SvxMSDffVertPair mso_sptLeftRightArrowCalloutVert[] =
{
    { 0 MSO_I, 0 }, { 4 MSO_I, 0 }, { 4 MSO_I, 3 MSO_I }, { 6 MSO_I, 3 MSO_I },
    { 6 MSO_I, 1 MSO_I }, { 21600, 10800 }, { 6 MSO_I, 5 MSO_I }, { 6 MSO_I, 7 MSO_I },
    { 4 MSO_I, 7 MSO_I }, { 4 MSO_I, 21600 }, { 0 MSO_I, 21600 }, { 0 MSO_I, 7 MSO_I },
    { 2 MSO_I, 7 MSO_I }, { 2 MSO_I, 5 MSO_I }, { 0, 10800 }, { 2 MSO_I, 1 MSO_I },
    { 2 MSO_I, 3 MSO_I }, { 0 MSO_I, 3 MSO_I }
};
const sal_uInt16 mso_sptLeftRightArrowCalloutSegm[] =
{
    0x4000, 0x0011, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptLeftRightArrowCalloutCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust3Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust4Value, 0, 0 } },
    { 0x8000, { 21600, 0, 0x0400 } },
    { 0x8000, { 21600, 0, 0x0401 } },
    { 0x8000, { 21600, 0, 0x0402 } },
    { 0x8000, { 21600, 0, 0x0403 } }
};
const sal_Int32 mso_sptLeftRightArrowCalloutDefault[] =
{
    4, 5400, 5500, 2700, 8100
};
const SvxMSDffTextRectangles mso_sptLeftRightArrowCalloutTextRect[] =
{
    { { 0 MSO_I, 0 }, { 4 MSO_I, 21600 } }
};
const SvxMSDffHandle mso_sptLeftRightArrowCalloutHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL,
        0x100, 0, 10800, 10800, 0x102, 10800, MIN_INT32, 0x7fffffff },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL,
        0x102, 0x103, 10800, 10800, 0, 0x100, 0x101, 10800 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        0, 0x101, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 0x103 }
};
const mso_CustomShape msoLeftRightArrowCallout =
{
    std::span<const SvxMSDffVertPair>(mso_sptLeftRightArrowCalloutVert),
    const_cast<sal_uInt16*>(mso_sptLeftRightArrowCalloutSegm), sizeof( mso_sptLeftRightArrowCalloutSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptLeftRightArrowCalloutCalc),
    const_cast<sal_Int32*>(mso_sptLeftRightArrowCalloutDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptLeftRightArrowCalloutTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptLeftRightArrowCalloutHandle),
};

const SvxMSDffVertPair mso_sptUpDownArrowCalloutVert[] =
{
    { 0, 0 MSO_I }, { 0, 4 MSO_I }, { 3 MSO_I, 4 MSO_I }, { 3 MSO_I, 6 MSO_I },
    { 1 MSO_I, 6 MSO_I }, { 10800, 21600 }, { 5 MSO_I, 6 MSO_I }, { 7 MSO_I, 6 MSO_I },
    { 7 MSO_I, 4 MSO_I }, { 21600, 4 MSO_I }, { 21600, 0 MSO_I }, { 7 MSO_I, 0 MSO_I },
    { 7 MSO_I, 2 MSO_I }, { 5 MSO_I, 2 MSO_I }, { 10800, 0 }, { 1 MSO_I, 2 MSO_I },
    { 3 MSO_I, 2 MSO_I }, { 3 MSO_I, 0 MSO_I }
};
const sal_uInt16 mso_sptUpDownArrowCalloutSegm[] =
{
    0x4000, 0x0011, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptUpDownArrowCalloutCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust3Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust4Value, 0, 0 } },
    { 0x8000, { 21600, 0, 0x0400 } },
    { 0x8000, { 21600, 0, 0x0401 } },
    { 0x8000, { 21600, 0, 0x0402 } },
    { 0x8000, { 21600, 0, 0x0403 } }
};
const sal_Int32 mso_sptUpDownArrowCalloutDefault[] =
{
    4, 5400, 5500, 2700, 8100
};
const SvxMSDffTextRectangles mso_sptUpDownArrowCalloutTextRect[] =
{
    { { 0, 0 MSO_I }, { 21600, 4 MSO_I } }
};
const SvxMSDffHandle mso_sptUpDownArrowCalloutHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0x102, 10800 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        0x103, 0x102, 10800, 10800, 0x101, 10800, 0, 0x100 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
        0x101, 0, 10800, 10800, 0, 0x103, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoUpDownArrowCallout =
{
    std::span<const SvxMSDffVertPair>(mso_sptUpDownArrowCalloutVert),
    const_cast<sal_uInt16*>(mso_sptUpDownArrowCalloutSegm), sizeof( mso_sptUpDownArrowCalloutSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptUpDownArrowCalloutCalc),
    const_cast<sal_Int32*>(mso_sptUpDownArrowCalloutDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptUpDownArrowCalloutTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptUpDownArrowCalloutHandle),
};

const SvxMSDffVertPair mso_sptQuadArrowCalloutVert[] =
{
    { 0 MSO_I, 0 MSO_I }, { 3 MSO_I, 0 MSO_I }, { 3 MSO_I, 2 MSO_I }, { 1 MSO_I, 2 MSO_I },
    { 10800, 0 }, { 5 MSO_I, 2 MSO_I }, { 7 MSO_I, 2 MSO_I }, { 7 MSO_I, 0 MSO_I },
    { 4 MSO_I, 0 MSO_I }, { 4 MSO_I, 3 MSO_I }, { 6 MSO_I, 3 MSO_I }, { 6 MSO_I, 1 MSO_I },
    { 21600, 10800 }, { 6 MSO_I, 5 MSO_I }, { 6 MSO_I, 7 MSO_I }, { 4 MSO_I, 7 MSO_I },
    { 4 MSO_I, 4 MSO_I }, { 7 MSO_I, 4 MSO_I }, { 7 MSO_I, 6 MSO_I }, { 5 MSO_I, 6 MSO_I },
    { 10800, 21600 }, { 1 MSO_I, 6 MSO_I }, { 3 MSO_I, 6 MSO_I }, { 3 MSO_I, 4 MSO_I },
    { 0 MSO_I, 4 MSO_I }, { 0 MSO_I, 7 MSO_I }, { 2 MSO_I, 7 MSO_I }, { 2 MSO_I, 5 MSO_I },
    { 0, 10800 }, { 2 MSO_I, 1 MSO_I }, { 2 MSO_I, 3 MSO_I }, { 0 MSO_I, 3 MSO_I }
};
const sal_uInt16 mso_sptQuadArrowCalloutSegm[] =
{
    0x4000, 0x001f, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptQuadArrowCalloutCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust3Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust4Value, 0, 0 } },
    { 0x8000, { 21600, 0, 0x0400 } },
    { 0x8000, { 21600, 0, 0x0401 } },
    { 0x8000, { 21600, 0, 0x0402 } },
    { 0x8000, { 21600, 0, 0x0403 } }
};
const sal_Int32 mso_sptQuadArrowCalloutDefault[] =
{
    4, 5400, 8100, 2700, 9400
};
const SvxMSDffTextRectangles mso_sptQuadArrowCalloutTextRect[] =
{
    { { 0 MSO_I, 0 MSO_I }, { 4 MSO_I, 4 MSO_I } }
};
const SvxMSDffHandle mso_sptQuadArrowCalloutHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0x102, 0x101 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL,
        0x101, 0, 10800, 10800, 0x100, 10800, MIN_INT32, 0x7fffffff },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        0x103, 0x102, 10800, 10800, 0x101, 10800, 0, 0x100 }
};
const mso_CustomShape msoQuadArrowCallout =
{
    std::span<const SvxMSDffVertPair>(mso_sptQuadArrowCalloutVert),
    const_cast<sal_uInt16*>(mso_sptQuadArrowCalloutSegm), sizeof( mso_sptQuadArrowCalloutSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptQuadArrowCalloutCalc),
    const_cast<sal_Int32*>(mso_sptQuadArrowCalloutDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptQuadArrowCalloutTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptQuadArrowCalloutHandle),
};

const SvxMSDffVertPair mso_sptCircularArrowVert[] =
{
    { 0x03 MSO_I, 0x03 MSO_I },
    { 0x14 MSO_I, 0x14 MSO_I },
    { 0x13 MSO_I, 0x12 MSO_I },
    { 0x11 MSO_I, 0x10 MSO_I },
    { 0, 0 }, { 21600, 21600 },
    { 0x09 MSO_I, 0x08 MSO_I },
    { 0x0B MSO_I, 0x0A MSO_I },
    { 0x18 MSO_I, 0x17 MSO_I },
    { 0x2F MSO_I, 0x2E MSO_I },
    { 0x1D MSO_I, 0x1C MSO_I }
};
const sal_uInt16 mso_sptCircularArrowSegm[] =
{
    0xa404, 0xa504, 0x0003, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptCircularArrowCalc[] =
{
    { 0x2000, { 0x0147, 0x0000, 0x0000 } },
    { 0x2000, { 0x0148, 0x0000, 0x0000 } },
    { 0x2000, { 0x0149, 0x0000, 0x0000 } },
    { 0x4000, { 0x2A30, 0x0149, 0x0000 } },
    { 0x4009, { 0x2A30, 0x0147, 0x0000 } },
    { 0x400A, { 0x2A30, 0x0147, 0x0000 } },
    { 0x4009, { 0x2A30, 0x0148, 0x0000 } },
    { 0x400A, { 0x2A30, 0x0148, 0x0000 } },
    { 0x2000, { 0x0404, 0x2A30, 0x0000 } },
    { 0x2000, { 0x0405, 0x2A30, 0x0000 } },
    { 0x2000, { 0x0406, 0x2A30, 0x0000 } },
    { 0x2000, { 0x0407, 0x2A30, 0x0000 } },
    { 0x6009, { 0x0403, 0x0147, 0x0000 } },
    { 0x600A, { 0x0403, 0x0147, 0x0000 } },
    { 0x6009, { 0x0403, 0x0148, 0x0000 } },
    { 0x600A, { 0x0403, 0x0148, 0x0000 } },
    { 0x2000, { 0x040C, 0x2A30, 0x0000 } },
    { 0x2000, { 0x040D, 0x2A30, 0x0000 } },
    { 0x2000, { 0x040E, 0x2A30, 0x0000 } },
    { 0x2000, { 0x040F, 0x2A30, 0x0000 } },
    { 0x8000, { 0x5460, 0x0000, 0x0403 } },
    { 0x4009, { 0x34BC, 0x0148, 0x0000 } },
    { 0x400A, { 0x34BC, 0x0148, 0x0000 } },
    { 0x2000, { 0x0415, 0x2A30, 0x0000 } },
    { 0x2000, { 0x0416, 0x2A30, 0x0000 } },
    { 0x2000, { 0x0149, 0x0000, 0x0A8C } },
    { 0x6009, { 0x0419, 0x0148, 0x0000 } },
    { 0x600A, { 0x0419, 0x0148, 0x0000 } },
    { 0x2000, { 0x041A, 0x2A30, 0x0000 } },
    { 0x2000, { 0x041B, 0x2A30, 0x0000 } },
    { 0xA000, { 0x041D, 0x0000, 0x0418 } },
    { 0xA000, { 0x041D, 0x0000, 0x0418 } },
    { 0x6001, { 0x041E, 0x041F, 0x0001 } },
    { 0xA000, { 0x041C, 0x0000, 0x0417 } },
    { 0xA000, { 0x041C, 0x0000, 0x0417 } },
    { 0x6001, { 0x0421, 0x0422, 0x0001 } },
    { 0x6000, { 0x0420, 0x0423, 0x0000 } },
    { 0x200D, { 0x0424, 0x0000, 0x0000 } },
    { 0x200E, { 0x0148, 0x002D, 0x0000 } },
    { 0x6009, { 0x0425, 0x0426, 0x0000 } },
    { 0x200E, { 0x0148, 0x002D, 0x0000 } },
    { 0x600A, { 0x0425, 0x0428, 0x0000 } },
    { 0x000E, { 0x0000, 0x002D, 0x0000 } },
    { 0x6009, { 0x0427, 0x042A, 0x0000 } },
    { 0x000E, { 0x0000, 0x002D, 0x0000 } },
    { 0x6009, { 0x0429, 0x042C, 0x0000 } },
    { 0x6000, { 0x041C, 0x042B, 0x0000 } },
    { 0x6000, { 0x041D, 0x042D, 0x0000 } }
};
const sal_Int32 mso_sptCircularArrowDefault[] =
{
    3, 180, 0, 5500
};
const SvxMSDffTextRectangles mso_sptCircularArrowTextRect[] =    // todo
{
    { { 0, 0 }, { 21600, 21600 } }
};
const SvxMSDffHandle mso_sptCircularArrowHandle[] =
{
    {   SvxMSDffHandleFlags::POLAR | SvxMSDffHandleFlags::RADIUS_RANGE,
        10800, 0x100, 10800, 10800, 10800, 10800, -0x7f4c0000, 0x00b40000 },
    {   SvxMSDffHandleFlags::POLAR | SvxMSDffHandleFlags::RADIUS_RANGE,
        0x102, 0x101, 10800, 10800, 0, 10800, -0x7f4c0000, 0x00b40000 }
};
const mso_CustomShape msoCircularArrow =
{
    std::span<const SvxMSDffVertPair>(mso_sptCircularArrowVert),
    const_cast<sal_uInt16*>(mso_sptCircularArrowSegm), sizeof( mso_sptCircularArrowSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCircularArrowCalc),
    const_cast<sal_Int32*>(mso_sptCircularArrowDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptCircularArrowTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCircularArrowHandle),
};

const SvxMSDffVertPair mso_sptCubeVert[] =
{
    { 0, 12 MSO_I }, { 0, 1 MSO_I }, { 2 MSO_I, 0 }, { 11 MSO_I, 0 }, { 11 MSO_I, 3 MSO_I }, { 4 MSO_I, 12 MSO_I },
    { 0, 1 MSO_I }, { 2 MSO_I, 0 }, { 11 MSO_I, 0 }, { 4 MSO_I, 1 MSO_I },
    { 4 MSO_I, 12 MSO_I  }, { 4 MSO_I, 1 MSO_I },   { 11 MSO_I, 0 }, { 11 MSO_I, 3 MSO_I }
};
const sal_uInt16 mso_sptCubeSegm[] =
{
    0x4000, 0x0005, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptCubeCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x6000, { DFF_Prop_geoTop, 0x400, 0 } },
    { 0x6000, { DFF_Prop_geoLeft, 0x400, 0 } },
    { 0xa000, { DFF_Prop_geoBottom, 0, 0x400 } },
    { 0xa000, { DFF_Prop_geoRight, 0, 0x400 } },
    { 0xa000, { DFF_Prop_geoRight, 0, 0x402 } },    // 5
    { 0x2001, { 0x405, 1, 2 } },                    // 6
    { 0x6000, { 0x402, 0x406, 0 } },                // 7
    { 0xa000, { DFF_Prop_geoBottom, 0, 0x401 } },   // 8
    { 0x2001, { 0x408, 1, 2 } },                    // 9
    { 0x6000, { 0x401, 0x409, 0 } },                // 10
    { 0x2000, { DFF_Prop_geoRight, 0, 0 } },        // 11
    { 0x2000, { DFF_Prop_geoBottom, 0, 0 } }        // 12
};
const SvxMSDffTextRectangles mso_sptCubeTextRect[] =
{
    { { 0, 1 MSO_I }, { 4 MSO_I, 12 MSO_I } }
};
const SvxMSDffHandle mso_sptCubeHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::SWITCHED,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 21600 }
};
const SvxMSDffVertPair mso_sptCubeGluePoints[] =
{
    { 7 MSO_I, 0 }, { 6 MSO_I, 1 MSO_I }, { 0, 10 MSO_I }, { 6 MSO_I, 21600 }, { 4 MSO_I, 10 MSO_I }, { 21600, 9 MSO_I }
};
const mso_CustomShape msoCube =
{
    std::span<const SvxMSDffVertPair>(mso_sptCubeVert),
    const_cast<sal_uInt16*>(mso_sptCubeSegm), sizeof( mso_sptCubeSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCubeCalc),
    const_cast<sal_Int32*>(mso_sptDefault5400),
    std::span<const SvxMSDffTextRectangles>(mso_sptCubeTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(mso_sptCubeGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptCubeHandle),
};

const SvxMSDffVertPair mso_sptBevelVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 },
    { 0, 0 }, { 21600, 0 }, { 1 MSO_I, 0 MSO_I }, { 0 MSO_I, 0 MSO_I },
    { 21600, 0 }, { 21600, 21600 }, { 1 MSO_I, 2 MSO_I }, { 1 MSO_I, 0 MSO_I },
    { 21600, 21600 }, { 0, 21600 }, { 0 MSO_I, 2 MSO_I }, { 1 MSO_I, 2 MSO_I },
    { 0, 21600 }, { 0, 0 }, { 0 MSO_I, 0 MSO_I }, { 0 MSO_I, 2 MSO_I }
};
const sal_uInt16 mso_sptBevelSegm[] =
{
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptBevelCalc[] =
{
    { 0x2001, { DFF_Prop_adjustValue, 21599, 21600 } },
    { 0xa000, { DFF_Prop_geoRight, 0, 0x400 } },
    { 0xa000, { DFF_Prop_geoBottom, 0, 0x400 } }
};

const SvxMSDffTextRectangles mso_sptBevelTextRect[] =
{
    { { 0 MSO_I, 0 MSO_I }, { 1 MSO_I, 2 MSO_I } }
};
const SvxMSDffHandle mso_sptBevelHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::SWITCHED,
        0x100, 0, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoBevel =
{
    std::span<const SvxMSDffVertPair>(mso_sptBevelVert),
    const_cast<sal_uInt16*>(mso_sptBevelSegm), sizeof( mso_sptBevelSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptBevelCalc),
    const_cast<sal_Int32*>(mso_sptDefault2700),
    std::span<const SvxMSDffTextRectangles>(mso_sptBevelTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptBevelHandle),
};

const SvxMSDffVertPair mso_sptFoldedCornerVert[] =   // adjustment1 : x 10800 - 21600
{
    { 0, 0 }, { 21600, 0 }, { 21600, 0 MSO_I }, { 0 MSO_I, 21600 },
    { 0, 21600 }, { 0 MSO_I, 21600 }, { 3 MSO_I, 0 MSO_I }, { 8 MSO_I, 9 MSO_I },
    { 10 MSO_I, 11 MSO_I }, { 21600, 0 MSO_I }
};
const sal_uInt16 mso_sptFoldedCornerSegm[] =
{
    0x4000, 0x0004, 0x6001, 0x8000,
    0x4000, 0x0001, 0x2001, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptFoldedCornerCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x8000, { 21600, 0, 0x400 } },
    { 0x2001, { 0x0401, 8000, 10800 } },
    { 0x8000, { 21600, 0, 0x0402 } },
    { 0x2001, { 0x0401, 1, 2 } },
    { 0x2001, { 0x0401, 1, 4 } },
    { 0x2001, { 0x0401, 1, 7 } },
    { 0x2001, { 0x0401, 1, 16 } },
    { 0x6000, { 0x0403, 0x405, 0 } },
    { 0x6000, { 0x0400, 0x406, 0 } },
    { 0x8000, { 21600, 0, 0x404 } },
    { 0x6000, { 0x400, 0x407, 0 } }
};
const sal_Int32 mso_sptFoldedCornerDefault[] =
{
    1, 18900
};
const SvxMSDffTextRectangles mso_sptFoldedCornerTextRect[] =
{
    { { 0, 0 }, { 21600, 11 MSO_I } }
};
const SvxMSDffHandle mso_sptFoldedCornerHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 1, 10800, 10800, 10800, 21600, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoFoldedCorner =
{
    std::span<const SvxMSDffVertPair>(mso_sptFoldedCornerVert),
    const_cast<sal_uInt16*>(mso_sptFoldedCornerSegm), sizeof( mso_sptFoldedCornerSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptFoldedCornerCalc),
    const_cast<sal_Int32*>(mso_sptFoldedCornerDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFoldedCornerTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptFoldedCornerHandle),
};

const SvxMSDffVertPair mso_sptActionButtonBlankVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 },
    { 0, 0 }, { 21600, 0 }, { 1 MSO_I, 0 MSO_I }, { 0 MSO_I, 0 MSO_I },
    { 21600, 0 }, { 21600, 21600 }, { 1 MSO_I, 2 MSO_I }, { 1 MSO_I, 0 MSO_I },
    { 21600, 21600 }, { 0, 21600 }, { 0 MSO_I, 2 MSO_I }, { 1 MSO_I, 2 MSO_I },
    { 0, 21600 }, { 0, 0 }, { 0 MSO_I, 0 MSO_I }, { 0 MSO_I, 2 MSO_I }
};
const sal_uInt16 mso_sptActionButtonBlankSegm[] =
{
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptActionButtonBlankCalc[] =
{
    { 0x2001, { DFF_Prop_adjustValue, 21599, 21600 } },
    { 0xa000, { DFF_Prop_geoRight, 0, 0x400 } },
    { 0xa000, { DFF_Prop_geoBottom, 0, 0x400 } }
};
const SvxMSDffTextRectangles mso_sptActionButtonBlankTextRect[] =
{
    { { 0 MSO_I, 0 MSO_I }, { 1 MSO_I, 2 MSO_I } }
};
const SvxMSDffHandle mso_sptButtonHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::SWITCHED,
        0x100, 0, 10800, 10800, 0, 5400, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoActionButtonBlank =
{
    std::span<const SvxMSDffVertPair>(mso_sptActionButtonBlankVert),
    const_cast<sal_uInt16*>(mso_sptActionButtonBlankSegm), sizeof( mso_sptActionButtonBlankSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptActionButtonBlankCalc),
    const_cast<sal_Int32*>(mso_sptDefault1400),
    std::span<const SvxMSDffTextRectangles>(mso_sptActionButtonBlankTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptButtonHandle),
};

const SvxMSDffTextRectangles mso_sptActionButtonTextRect[] =
{
    { { 1 MSO_I, 2 MSO_I }, { 3 MSO_I, 4 MSO_I } }
};
const SvxMSDffVertPair mso_sptActionButtonHomeVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 },
    { 0, 0 }, { 21600, 0 }, { 3 MSO_I, 2 MSO_I }, { 1 MSO_I, 2 MSO_I },
    { 21600, 0 }, { 21600, 21600 }, { 3 MSO_I, 4 MSO_I }, { 3 MSO_I, 2 MSO_I },
    { 21600, 21600 }, { 0, 21600 }, { 1 MSO_I, 4 MSO_I }, { 3 MSO_I, 4 MSO_I },
    { 0, 21600 }, { 0, 0 }, { 1 MSO_I, 2 MSO_I }, { 1 MSO_I, 4 MSO_I },

    { 7 MSO_I, 0xa MSO_I }, { 0xc MSO_I, 0xe MSO_I }, { 0xc MSO_I, 0x10 MSO_I }, { 0x12 MSO_I, 0x10 MSO_I },
    { 0x12 MSO_I, 0x14 MSO_I }, { 0x16 MSO_I, 8 MSO_I }, { 0x18 MSO_I, 8 MSO_I }, { 0x18 MSO_I, 0x1a MSO_I },
    { 0x1c MSO_I, 0x1a MSO_I }, { 0x1c MSO_I, 8 MSO_I }, { 0x1e MSO_I, 8 MSO_I },

    { 0xc MSO_I, 0xe MSO_I }, { 0xc MSO_I, 0x10 MSO_I }, { 0x12 MSO_I, 0x10 MSO_I },{ 0x12 MSO_I, 0x14 MSO_I },

    { 0x20 MSO_I, 0x24 MSO_I }, { 0x22 MSO_I, 0x24 MSO_I }, { 0x22 MSO_I, 0x1a MSO_I }, { 0x18 MSO_I, 0x1a MSO_I },
    { 0x18 MSO_I, 8 MSO_I }, { 0x1c MSO_I, 8 MSO_I }, { 0x1c MSO_I, 0x1a MSO_I }, { 0x20 MSO_I, 0x1a MSO_I }

};
const sal_uInt16 mso_sptActionButtonHomeSegm[] =
{
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x000a, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0007, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptActionButtonHomeCalc[] =    // adj value 0 - 5400
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x6000, { DFF_Prop_geoLeft, DFF_Prop_adjustValue, 0 } },
    { 0x6000, { DFF_Prop_geoTop, DFF_Prop_adjustValue, 0 } },
    { 0xa000, { DFF_Prop_geoRight, 0, DFF_Prop_adjustValue } },
    { 0xa000, { DFF_Prop_geoBottom, 0, DFF_Prop_adjustValue } },
    { 0x8000, { 10800, 0, DFF_Prop_adjustValue } },
    { 0x2001, { 0x0405, 1, 10800 } },                           // scaling   6
    { 0x2001, { DFF_Prop_geoRight, 1, 2 } },    // lr center 7
    { 0x2001, { DFF_Prop_geoBottom, 1, 2 } },   // ul center 8

    { 0x4001, { -8000, 0x0406, 1 } },   // 9
    { 0x6000, { 0x0409, 0x0408, 0 } },  // a
    { 0x4001, { 2960, 0x0406, 1 } },    // b
    { 0x6000, { 0x040b, 0x0407, 0 } },  // c
    { 0x4001, { -5000, 0x0406, 1 } },   // d
    { 0x6000, { 0x040d, 0x0408, 0 } },  // e
    { 0x4001, { -7000, 0x0406, 1 } },   // f
    { 0x6000, { 0x040f, 0x0408, 0 } },  // 10
    { 0x4001, { 5000, 0x0406, 1 } },    // 11
    { 0x6000, { 0x0411, 0x0407, 0 } },  // 12
    { 0x4001, { -2960, 0x0406, 1 } },   // 13
    { 0x6000, { 0x0413, 0x0408, 0 } },  // 14
    { 0x4001, { 8000, 0x0406, 1 } },    // 15
    { 0x6000, { 0x0415,0x0407, 0 } },   // 16
    { 0x4001, { 6100, 0x0406, 1 } },    // 17
    { 0x6000, { 0x0417,0x0407, 0 } },   // 18
    { 0x4001, { 8260, 0x0406, 1 } },    // 19
    { 0x6000, { 0x0419, 0x0408, 0 } },  // 1a
    { 0x4001, { -6100, 0x0406, 1 } },   // 1b
    { 0x6000, { 0x041b, 0x0407, 0 } },  // 1c
    { 0x4001, { -8000, 0x0406, 1 } },   // 1d
    { 0x6000, { 0x041d, 0x0407, 0 } },  // 1e
    { 0x4001, { -1060, 0x0406, 1 } },   // 1f
    { 0x6000, { 0x041f, 0x0407, 0 } },  // 20
    { 0x4001, { 1060, 0x0406, 1 } },    // 21
    { 0x6000, { 0x0421, 0x0407, 0 } },  // 22
    { 0x4001, { 4020, 0x0406, 1 } },    // 23
    { 0x6000, { 0x0423, 0x0408, 0 } }   // 24

};
const mso_CustomShape msoActionButtonHome =
{
    std::span<const SvxMSDffVertPair>(mso_sptActionButtonHomeVert),
    const_cast<sal_uInt16*>(mso_sptActionButtonHomeSegm), sizeof( mso_sptActionButtonHomeSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptActionButtonHomeCalc),
    const_cast<sal_Int32*>(mso_sptDefault1400),
    std::span<const SvxMSDffTextRectangles>(mso_sptActionButtonTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptButtonHandle),
};

const SvxMSDffVertPair mso_sptActionButtonHelpVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 },
    { 0, 0 }, { 21600, 0 }, { 3 MSO_I, 2 MSO_I }, { 1 MSO_I, 2 MSO_I },
    { 21600, 0 }, { 21600, 21600 }, { 3 MSO_I, 4 MSO_I }, { 3 MSO_I, 2 MSO_I },
    { 21600, 21600 }, { 0, 21600 }, { 1 MSO_I, 4 MSO_I }, { 3 MSO_I, 4 MSO_I },
    { 0, 21600 }, { 0, 0 }, { 1 MSO_I, 2 MSO_I }, { 1 MSO_I,4 MSO_I },
    { 7 MSO_I, 0xc MSO_I }, { 0xa MSO_I, 0x3e MSO_I }, { 7 MSO_I, 0x10 MSO_I }, { 0xe MSO_I, 0x3e MSO_I }, { 7 MSO_I, 0xc MSO_I },
    { 0x12 MSO_I, 0x14 MSO_I }, { 0x12 MSO_I, 0x16 MSO_I },                             // pp
    { 0x12 MSO_I, 0x18 MSO_I }, { 0x1a MSO_I, 8 MSO_I }, { 0x1c MSO_I, 8 MSO_I },       // ccp
    { 0x1e MSO_I, 8 MSO_I }, { 0x20 MSO_I, 0x22 MSO_I }, { 0x20 MSO_I, 0x24 MSO_I },    // ccp
    { 0x20 MSO_I, 0x26 MSO_I }, { 0x28 MSO_I, 0x2a MSO_I }, { 7 MSO_I, 0x2a MSO_I },    // ccp
    { 0x2c MSO_I, 0x2a MSO_I }, { 0x2e MSO_I, 0x26 MSO_I }, { 0x2e MSO_I, 0x24 MSO_I }, // ccp
    { 0x30 MSO_I, 0x24 MSO_I }, { 0x30 MSO_I, 0x32 MSO_I }, { 0x34 MSO_I, 0x36 MSO_I }, // ccp
    { 7 MSO_I, 0x36 MSO_I },                                                            // p
    { 0x12 MSO_I, 0x36 MSO_I }, { 0x1c MSO_I, 0x32 MSO_I }, { 0x1c MSO_I, 0x24 MSO_I }, // ccp
    { 0x1c MSO_I, 0x38 MSO_I }, { 0x3a MSO_I, 0x3c MSO_I }, { 0x12 MSO_I, 0x3c MSO_I }, // ccp
    { 7 MSO_I, 0x3c MSO_I }, { 0x34 MSO_I, 8 MSO_I }, { 0x34 MSO_I, 0x16 MSO_I },       // ccp
    { 0x34 MSO_I, 0x14 MSO_I }
};
const sal_uInt16 mso_sptActionButtonHelpSegm[] =
{
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0xa704, 0x6000, 0x8000,
    0x4000, 0x0001, 0x2004, 0x0001, 0x2004, 0x0001, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptActionButtonHelpCalc[] =    // adj value 0 - 5400
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x6000, { DFF_Prop_geoLeft, DFF_Prop_adjustValue, 0 } },
    { 0x6000, { DFF_Prop_geoTop, DFF_Prop_adjustValue, 0 } },
    { 0xa000, { DFF_Prop_geoRight, 0, DFF_Prop_adjustValue } },
    { 0xa000, { DFF_Prop_geoBottom, 0, DFF_Prop_adjustValue } },
    { 0x8000, { 10800, 0, DFF_Prop_adjustValue } },
    { 0x2001, { 0x0405, 1, 10800 } },           // scaling   6
    { 0x2001, { DFF_Prop_geoRight, 1, 2 } },    // lr center 7
    { 0x2001, { DFF_Prop_geoBottom, 1, 2 } },   // ul center 8

    { 0x4001, { -1690, 0x0406, 1 } },   // 9
    { 0x6000, { 0x0409, 0x0407, 0 } },  // a
    { 0x4001, { 4600, 0x0406, 1 } },    // b
    { 0x6000, { 0x040b, 0x0408, 0 } },  // c
    { 0x4001, { 1690, 0x0406, 1 } },    // d
    { 0x6000, { 0x040d, 0x0407, 0 } },  // e
    { 0x4001, { 7980, 0x0406, 1 } },    // f
    { 0x6000, { 0x040f, 0x0408, 0 } },  // 10
    { 0x4001, { 1270, 0x0406, 1 } },    // 11
    { 0x6000, { 0x0411, 0x0407, 0 } },  // 12
    { 0x4001, { 4000, 0x0406, 1 } },    // 13
    { 0x6000, { 0x0413, 0x0408, 0 } },  // 14
    { 0x4001, { 1750, 0x0406, 1 } },    // 15
    { 0x6000, { 0x0415, 0x0408, 0 } },  // 16
    { 0x4001, { 800, 0x0406, 1 } },     // 17
    { 0x6000, { 0x0417, 0x0408, 0 } },  // 18
    { 0x4001, { 1650, 0x0406, 1 } },    // 19
    { 0x6000, { 0x0419, 0x0407, 0 } },  // 1a
    { 0x4001, { 2340, 0x0406, 1 } },    // 1b
    { 0x6000, { 0x041b, 0x0407, 0 } },  // 1c
    { 0x4001, { 3640, 0x0406, 1 } },    // 1d
    { 0x6000, { 0x041d, 0x0407, 0 } },  // 1e
    { 0x4001, { 4670, 0x0406, 1 } },    // 1f
    { 0x6000, { 0x041f, 0x0407, 0 } },  // 20
    { 0x4001, { -1570, 0x0406, 1 } },   // 21
    { 0x6000, { 0x0421, 0x0408, 0 } },  // 22
    { 0x4001, { -3390, 0x0406, 1 } },   // 23
    { 0x6000, { 0x0423, 0x0408, 0 } },  // 24
    { 0x4001, { -6050, 0x0406, 1 } },   // 25
    { 0x6000, { 0x0425, 0x0408, 0 } },  // 26
    { 0x4001, { 2540, 0x0406, 1 } },    // 27
    { 0x6000, { 0x0427, 0x0407, 0 } },  // 28
    { 0x4001, { -8050, 0x0406, 1 } },   // 29
    { 0x6000, { 0x0429, 0x0408, 0 } },  // 2a
    { 0x4001, { -2540, 0x0406, 1 } },   // 2b
    { 0x6000, { 0x042b, 0x0407, 0 } },  // 2c
    { 0x4001, { -4460, 0x0406, 1 } },   // 2d
    { 0x6000, { 0x042d, 0x0407, 0 } },  // 2e
    { 0x4001, { -2330, 0x0406, 1 } },   // 2f
    { 0x6000, { 0x042f, 0x0407, 0 } },  // 30
    { 0x4001, { -4700, 0x0406, 1 } },   // 31
    { 0x6000, { 0x0431, 0x0408, 0 } },  // 32
    { 0x4001, { -1270, 0x0406, 1 } },   // 33
    { 0x6000, { 0x0433, 0x0407, 0 } },  // 34
    { 0x4001, { -5720, 0x0406, 1 } },   // 35
    { 0x6000, { 0x0435, 0x0408, 0 } },  // 36
    { 0x4001, { -2540, 0x0406, 1 } },   // 37
    { 0x6000, { 0x0437, 0x0408, 0 } },  // 38
    { 0x4001, { 1800, 0x0406, 1 } },    // 39
    { 0x6000, { 0x0439, 0x0407, 0 } },  // 3a
    { 0x4001, { -1700, 0x0406, 1 } },   // 3b
    { 0x6000, { 0x043b, 0x0408, 0 } },  // 3c
    { 0x4001, { 6290, 0x0406, 1 } },    // 3d
    { 0x6000, { 0x043d, 0x0408, 0 } }   // 3e
};
const mso_CustomShape msoActionButtonHelp =
{
    std::span<const SvxMSDffVertPair>(mso_sptActionButtonHelpVert),
    const_cast<sal_uInt16*>(mso_sptActionButtonHelpSegm), sizeof( mso_sptActionButtonHelpSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptActionButtonHelpCalc),
    const_cast<sal_Int32*>(mso_sptDefault1400),
    std::span<const SvxMSDffTextRectangles>(mso_sptActionButtonTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptButtonHandle),
};

const SvxMSDffVertPair mso_sptActionButtonInformationVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 },
    { 0, 0 }, { 21600, 0 }, { 3 MSO_I, 2 MSO_I }, { 1 MSO_I, 2 MSO_I },
    { 21600, 0 }, { 21600, 21600 }, { 3 MSO_I, 4 MSO_I }, { 3 MSO_I, 2 MSO_I },
    { 21600, 21600 }, { 0, 21600 }, { 1 MSO_I, 4 MSO_I }, { 3 MSO_I, 4 MSO_I },
    { 0, 21600 }, { 0, 0 }, { 1 MSO_I, 2 MSO_I }, { 1 MSO_I, 4 MSO_I },
    { 0x7 MSO_I, 0xc MSO_I }, { 0xa MSO_I, 0x8 MSO_I }, { 0x7 MSO_I, 0x10 MSO_I }, { 0xe MSO_I, 8 MSO_I }, { 0x7 MSO_I, 0xc MSO_I },
    { 0x7 MSO_I, 0x14 MSO_I }, { 0x12 MSO_I, 0x2a MSO_I }, { 0x7 MSO_I, 0x18 MSO_I }, { 0x16 MSO_I, 0x2a MSO_I }, { 0x7 MSO_I, 0x14 MSO_I },
    { 0x1a MSO_I, 0x1c MSO_I }, { 0x1e MSO_I, 0x1c MSO_I }, { 0x1e MSO_I, 0x20 MSO_I }, { 0x22 MSO_I, 0x20 MSO_I },
    { 0x22 MSO_I, 0x24 MSO_I }, { 0x1a MSO_I, 0x24 MSO_I }, { 0x1a MSO_I, 0x20 MSO_I }, { 0x26 MSO_I, 0x20 MSO_I },
    { 0x26 MSO_I, 0x28 MSO_I }, { 0x1a MSO_I, 0x28 MSO_I }
};
const sal_uInt16 mso_sptActionButtonInformationSegm[] =
{
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0xa704, 0x6000, 0x8000,
    0x4000, 0xa704, 0x6000, 0x8000,
    0x4000, 0x0009, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptActionButtonInformationCalc[] = // adj value 0 - 5400
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x6000, { DFF_Prop_geoLeft, DFF_Prop_adjustValue, 0 } },
    { 0x6000, { DFF_Prop_geoTop, DFF_Prop_adjustValue, 0 } },
    { 0xa000, { DFF_Prop_geoRight, 0, DFF_Prop_adjustValue } },
    { 0xa000, { DFF_Prop_geoBottom, 0, DFF_Prop_adjustValue } },
    { 0x8000, { 10800, 0, DFF_Prop_adjustValue } },
    { 0x2001, { 0x0405, 1, 10800 } },           // scaling   6
    { 0x2001, { DFF_Prop_geoRight, 1, 2 } },    // lr center 7
    { 0x2001, { DFF_Prop_geoBottom, 1, 2 } },   // ul center 8

    { 0x4001, { -8050, 0x0406, 1 } },   // 9
    { 0x6000, { 0x0409, 0x0407, 0 } },  // a
    { 0x4001, { -8050, 0x0406, 1 } },   // b
    { 0x6000, { 0x040b, 0x0408, 0 } },  // c
    { 0x4001, { 8050, 0x0406, 1 } },    // d
    { 0x6000, { 0x040d, 0x0407, 0 } },  // e
    { 0x4001, { 8050, 0x0406, 1 } },    // f
    { 0x6000, { 0x040f, 0x0408, 0 } },  // 10

    { 0x4001, { -2060, 0x0406, 1 } },   // 11
    { 0x6000, { 0x0411, 0x0407, 0 } },  // 12
    { 0x4001, { -7620, 0x0406, 1 } },   // 13
    { 0x6000, { 0x0413, 0x0408, 0 } },  // 14
    { 0x4001, { 2060, 0x0406, 1 } },    // 15
    { 0x6000, { 0x0415, 0x0407, 0 } },  // 16
    { 0x4001, { -3500, 0x0406, 1 } },   // 17
    { 0x6000, { 0x0417, 0x0408, 0 } },  // 18

    { 0x4001, { -2960, 0x0406, 1 } },   // 19
    { 0x6000, { 0x0419, 0x0407, 0 } },  // 1a
    { 0x4001, { -2960, 0x0406, 1 } },   // 1b
    { 0x6000, { 0x041b, 0x0408, 0 } },  // 1c
    { 0x4001, { 1480, 0x0406, 1 } },    // 1d
    { 0x6000, { 0x041d, 0x0407, 0 } },  // 1e
    { 0x4001, { 5080, 0x0406, 1 } },    // 1f
    { 0x6000, { 0x041f, 0x0408, 0 } },  // 20
    { 0x4001, { 2960, 0x0406, 1 } },    // 21
    { 0x6000, { 0x0421, 0x0407, 0 } },  // 22
    { 0x4001, { 6140, 0x0406, 1 } },    // 23
    { 0x6000, { 0x0423, 0x0408, 0 } },  // 24
    { 0x4001, { -1480, 0x0406, 1 } },   // 25
    { 0x6000, { 0x0425, 0x0407, 0 } },  // 26
    { 0x4001, { -1920, 0x0406, 1 } },   // 27
    { 0x6000, { 0x0427, 0x0408, 0 } },  // 28

    { 0x4001, { -5560, 0x0406, 1 } },   // 29
    { 0x6000, { 0x0429, 0x0408, 0 } },  // 2a

};
const mso_CustomShape msoActionButtonInformation =
{
    std::span<const SvxMSDffVertPair>(mso_sptActionButtonInformationVert),
    const_cast<sal_uInt16*>(mso_sptActionButtonInformationSegm), sizeof( mso_sptActionButtonInformationSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptActionButtonInformationCalc),
    const_cast<sal_Int32*>(mso_sptDefault1400),
    std::span<const SvxMSDffTextRectangles>(mso_sptActionButtonTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptButtonHandle),
};

const SvxMSDffVertPair mso_sptActionButtonBackPreviousVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 },
    { 0, 0 }, { 21600, 0 }, { 3 MSO_I, 2 MSO_I }, { 1 MSO_I, 2 MSO_I },
    { 21600, 0 }, { 21600, 21600 }, { 3 MSO_I, 4 MSO_I }, { 3 MSO_I, 2 MSO_I },
    { 21600, 21600 }, { 0, 21600 }, { 1 MSO_I, 4 MSO_I }, { 3 MSO_I,4 MSO_I },
    { 0, 21600 }, { 0, 0 }, { 1 MSO_I, 2 MSO_I }, { 1 MSO_I, 4 MSO_I },

    { 0xa MSO_I, 8 MSO_I }, { 0xe MSO_I, 0xc MSO_I }, { 0xe MSO_I, 0x10 MSO_I }
};
const sal_uInt16 mso_sptActionButtonForwardBackSegm[] =
{
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0002, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptActionButtonForwardBackCalc[] = // adj value 0 - 5400
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x6000, { DFF_Prop_geoLeft, DFF_Prop_adjustValue, 0 } },
    { 0x6000, { DFF_Prop_geoTop, DFF_Prop_adjustValue, 0 } },
    { 0xa000, { DFF_Prop_geoRight, 0, DFF_Prop_adjustValue } },
    { 0xa000, { DFF_Prop_geoBottom, 0, DFF_Prop_adjustValue } },
    { 0x8000, { 10800, 0, DFF_Prop_adjustValue } },
    { 0x2001, { 0x0405, 1, 10800 } },           // scaling   6
    { 0x2001, { DFF_Prop_geoRight, 1, 2 } },    // lr center 7
    { 0x2001, { DFF_Prop_geoBottom, 1, 2 } },   // ul center 8

    { 0x4001, { -8050, 0x0406, 1 } },   // 9
    { 0x6000, { 0x0409, 0x0407, 0 } },  // a
    { 0x4001, { -8050, 0x0406, 1 } },   // b
    { 0x6000, { 0x040b, 0x0408, 0 } },  // c
    { 0x4001, { 8050, 0x0406, 1 } },    // d
    { 0x6000, { 0x040d, 0x0407, 0 } },  // e
    { 0x4001, { 8050, 0x0406, 1 } },    // f
    { 0x6000, { 0x040f, 0x0408, 0 } }   // 10
};
const mso_CustomShape msoActionButtonBackPrevious =
{
    std::span<const SvxMSDffVertPair>(mso_sptActionButtonBackPreviousVert),
    const_cast<sal_uInt16*>(mso_sptActionButtonForwardBackSegm), sizeof( mso_sptActionButtonForwardBackSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptActionButtonForwardBackCalc),
    const_cast<sal_Int32*>(mso_sptDefault1400),
    std::span<const SvxMSDffTextRectangles>(mso_sptActionButtonTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptButtonHandle),
};

const SvxMSDffVertPair mso_sptActionButtonForwardNextVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 },
    { 0, 0 }, { 21600, 0 }, { 3 MSO_I, 2 MSO_I }, { 1 MSO_I, 2 MSO_I },
    { 21600, 0 }, { 21600, 21600 }, { 3 MSO_I, 4 MSO_I }, { 3 MSO_I, 2 MSO_I },
    { 21600, 21600 }, { 0, 21600 }, { 1 MSO_I, 4 MSO_I }, { 3 MSO_I, 4 MSO_I },
    { 0, 21600 }, { 0, 0 }, { 1 MSO_I, 2 MSO_I }, { 1 MSO_I, 4 MSO_I },

    { 0xa MSO_I, 0xc MSO_I }, { 0xe MSO_I, 8 MSO_I }, { 0xa MSO_I, 0x10 MSO_I }
};
const mso_CustomShape msoActionButtonForwardNext =
{
    std::span<const SvxMSDffVertPair>(mso_sptActionButtonForwardNextVert),
    const_cast<sal_uInt16*>(mso_sptActionButtonForwardBackSegm), sizeof( mso_sptActionButtonForwardBackSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptActionButtonForwardBackCalc),
    const_cast<sal_Int32*>(mso_sptDefault1400),
    std::span<const SvxMSDffTextRectangles>(mso_sptActionButtonTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptButtonHandle),
};

const SvxMSDffVertPair mso_sptActionButtonBeginningVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 },
    { 0, 0 }, { 21600, 0 }, { 3 MSO_I, 2 MSO_I }, { 1 MSO_I, 2 MSO_I },
    { 21600, 0 }, { 21600, 21600 }, { 3 MSO_I, 4 MSO_I }, { 3 MSO_I, 2 MSO_I },
    { 21600, 21600 }, { 0, 21600 }, { 1 MSO_I, 4 MSO_I }, { 3 MSO_I, 4 MSO_I },
    { 0, 21600 }, { 0, 0 }, { 1 MSO_I, 2 MSO_I }, { 1 MSO_I, 4 MSO_I },

    { 0xa MSO_I, 8 MSO_I }, { 0xe MSO_I, 0xc MSO_I }, { 0xe MSO_I, 0x10 MSO_I }, { 0x12 MSO_I, 0xc MSO_I },
    { 0x14 MSO_I, 0xc MSO_I }, { 0x14 MSO_I, 0x10 MSO_I }, { 0x12 MSO_I, 0x10 MSO_I }
};
const sal_uInt16 mso_sptActionButtonBeginningEndSegm[] =
{
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,

    0x4000, 0x0002, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptActionButtonBeginningEndCalc[] =    // adj value 0 - 5400
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x6000, { DFF_Prop_geoLeft, DFF_Prop_adjustValue, 0 } },
    { 0x6000, { DFF_Prop_geoTop, DFF_Prop_adjustValue, 0 } },
    { 0xa000, { DFF_Prop_geoRight, 0, DFF_Prop_adjustValue } },
    { 0xa000, { DFF_Prop_geoBottom, 0, DFF_Prop_adjustValue } },
    { 0x8000, { 10800, 0, DFF_Prop_adjustValue } },
    { 0x2001, { 0x0405, 1, 10800 } },           // scaling   6
    { 0x2001, { DFF_Prop_geoRight, 1, 2 } },    // lr center 7
    { 0x2001, { DFF_Prop_geoBottom, 1, 2 } },   // ul center 8

    { 0x4001, { -4020, 0x0406, 1 } },   // 9
    { 0x6000, { 0x0409, 0x0407, 0 } },  // a
    { 0x4001, { -8050, 0x0406, 1 } },   // b
    { 0x6000, { 0x040b, 0x0408, 0 } },  // c
    { 0x4001, { 8050, 0x0406, 1 } },    // d
    { 0x6000, { 0x040d, 0x0407, 0 } },  // e
    { 0x4001, { 8050, 0x0406, 1 } },    // f
    { 0x6000, { 0x040f, 0x0408, 0 } },  // 10

    { 0x4001, { -8050, 0x0406, 1 } },   // 11
    { 0x6000, { 0x0411, 0x0407, 0 } },  // 12
    { 0x4001, { -6140, 0x0406, 1 } },   // 13
    { 0x6000, { 0x0413, 0x0407, 0 } },  // 14


    { 0x4001, { 4020, 0x0406, 1 } },    // 15
    { 0x6000, { 0x0415, 0x0407, 0 } },  // 16
    { 0x4001, { 6140, 0x0406, 1 } },    // 17
    { 0x6000, { 0x0417, 0x0407, 0 } }   // 18
};
const mso_CustomShape msoActionButtonBeginning =
{
    std::span<const SvxMSDffVertPair>(mso_sptActionButtonBeginningVert),
    const_cast<sal_uInt16*>(mso_sptActionButtonBeginningEndSegm), sizeof( mso_sptActionButtonBeginningEndSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptActionButtonBeginningEndCalc),
    const_cast<sal_Int32*>(mso_sptDefault1400),
    std::span<const SvxMSDffTextRectangles>(mso_sptActionButtonTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptButtonHandle),
};

const SvxMSDffVertPair mso_sptActionButtonEndVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 },
    { 0, 0 }, { 21600, 0 }, { 3 MSO_I, 2 MSO_I }, { 1 MSO_I, 2 MSO_I },
    { 21600, 0 }, { 21600, 21600 }, { 3 MSO_I, 4 MSO_I }, { 3 MSO_I, 2 MSO_I },
    { 21600, 21600 }, { 0, 21600 }, { 1 MSO_I, 4 MSO_I }, { 3 MSO_I, 4 MSO_I },
    { 0, 21600 }, { 0, 0 }, { 1 MSO_I, 2 MSO_I }, { 1 MSO_I, 4 MSO_I },

    { 0x16 MSO_I, 8 MSO_I }, { 0x12 MSO_I, 0x10 MSO_I }, { 0x12 MSO_I, 0xc MSO_I },

    { 0x18 MSO_I, 0xc MSO_I }, { 0x18 MSO_I, 0x10 MSO_I }, { 0xe MSO_I, 0x10 MSO_I }, { 0xe MSO_I, 0xc MSO_I }
};
const mso_CustomShape msoActionButtonEnd =
{
    std::span<const SvxMSDffVertPair>(mso_sptActionButtonEndVert),
    const_cast<sal_uInt16*>(mso_sptActionButtonBeginningEndSegm), sizeof( mso_sptActionButtonBeginningEndSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptActionButtonBeginningEndCalc),
    const_cast<sal_Int32*>(mso_sptDefault1400),
    std::span<const SvxMSDffTextRectangles>(mso_sptActionButtonTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptButtonHandle),
};

const SvxMSDffVertPair mso_sptActionButtonReturnVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 },
    { 0, 0 }, { 21600, 0 }, { 3 MSO_I, 2 MSO_I }, { 1 MSO_I, 2 MSO_I },
    { 21600, 0 }, { 21600,  21600 }, { 3 MSO_I, 4 MSO_I }, { 3 MSO_I, 2 MSO_I },
    { 21600, 21600 }, { 0, 21600 }, { 1 MSO_I, 4 MSO_I }, { 3 MSO_I, 4 MSO_I },
    { 0, 21600 }, { 0, 0 }, { 1 MSO_I, 2 MSO_I }, { 1 MSO_I, 4 MSO_I },

    { 0xa MSO_I, 0xc MSO_I }, { 0xe MSO_I, 0xc MSO_I }, { 0xe MSO_I, 0x10 MSO_I },                          // ppp
    { 0xe MSO_I, 0x12 MSO_I }, { 0x14 MSO_I, 0x16 MSO_I }, { 0x18 MSO_I, 0x16 MSO_I },                      // ccp
    { 7 MSO_I, 0x16 MSO_I },                                                                                // p
    { 0x1a MSO_I, 0x16 MSO_I }, { 0x1c MSO_I, 0x12 MSO_I }, { 0x1c MSO_I, 0x10 MSO_I },                     // ccp
    { 0x1c MSO_I, 0xc MSO_I }, { 7 MSO_I, 0xc MSO_I }, { 0x1e MSO_I, 0x20 MSO_I }, { 0x22 MSO_I, 0xc MSO_I },// pppp
    { 0x24 MSO_I, 0xc MSO_I }, { 0x24 MSO_I, 0x10 MSO_I },                                                  // pp
    { 0x24 MSO_I, 0x26 MSO_I }, { 0x28 MSO_I, 0x2a MSO_I }, { 7 MSO_I, 0x2a MSO_I },                        // ccp
    { 0x18 MSO_I, 0x2a MSO_I },                                                                             // p
    { 0x2c MSO_I, 0x2a MSO_I }, { 0xa MSO_I, 0x26 MSO_I }, { 0xa MSO_I, 0x10 MSO_I }                        // ccp
};
const sal_uInt16 mso_sptActionButtonReturnSegm[] =
{
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0002, 0x2001, 0x0001, 0x2001, 0x0006,0x2001, 0x0001, 0x2001, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptActionButtonReturnCalc[] =  // adj value 0 - 5400
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x6000, { DFF_Prop_geoLeft, DFF_Prop_adjustValue, 0 } },
    { 0x6000, { DFF_Prop_geoTop, DFF_Prop_adjustValue, 0 } },
    { 0xa000, { DFF_Prop_geoRight, 0, DFF_Prop_adjustValue } },
    { 0xa000, { DFF_Prop_geoBottom, 0, DFF_Prop_adjustValue } },
    { 0x8000, { 10800, 0, DFF_Prop_adjustValue } },
    { 0x2001, { 0x0405, 1, 10800 } },           // scaling   6
    { 0x2001, { DFF_Prop_geoRight, 1, 2 } },    // lr center 7
    { 0x2001, { DFF_Prop_geoBottom, 1, 2 } },   // ul center 8

    { 0x4001, { -8050, 0x0406, 1 } },   // 9
    { 0x6000, { 0x0409, 0x0407, 0 } },  // a
    { 0x4001, { -3800, 0x0406, 1 } },   // b
    { 0x6000, { 0x040b, 0x0408, 0 } },  // c
    { 0x4001, { -4020, 0x0406, 1 } },   // d
    { 0x6000, { 0x040d, 0x0407, 0 } },  // e
    { 0x4001, { 2330, 0x0406, 1 } },    // f
    { 0x6000, { 0x040f, 0x0408, 0 } },  // 10
    { 0x4001, { 3390, 0x0406, 1 } },    // 11
    { 0x6000, { 0x0411, 0x0408, 0 } },  // 12
    { 0x4001, { -3100, 0x0406, 1 } },   // 13
    { 0x6000, { 0x0413, 0x0407, 0 } },  // 14
    { 0x4001, { 4230, 0x0406, 1 } },    // 15
    { 0x6000, { 0x0415, 0x0408, 0 } },  // 16
    { 0x4001, { -1910, 0x0406, 1 } },   // 17
    { 0x6000, { 0x0417, 0x0407, 0 } },  // 18
    { 0x4001, { 1190, 0x0406, 1 } },    // 19
    { 0x6000, { 0x0419, 0x0407, 0 } },  // 1a
    { 0x4001, { 2110, 0x0406, 1 } },    // 1b
    { 0x6000, { 0x041b, 0x0407, 0 } },  // 1c
    { 0x4001, { 4030, 0x0406, 1 } },    // 1d
    { 0x6000, { 0x041d, 0x0407, 0 } },  // 1e
    { 0x4001, { -7830, 0x0406, 1 } },   // 1f
    { 0x6000, { 0x041f, 0x0408, 0 } },  // 20
    { 0x4001, { 8250, 0x0406, 1 } },    // 21
    { 0x6000, { 0x0421, 0x0407, 0 } },  // 22
    { 0x4001, { 6140, 0x0406, 1 } },    // 23
    { 0x6000, { 0x0423, 0x0407, 0 } },  // 24
    { 0x4001, { 5510, 0x0406, 1 } },    // 25
    { 0x6000, { 0x0425, 0x0408, 0 } },  // 26
    { 0x4001, { 3180, 0x0406, 1 } },    // 27
    { 0x6000, { 0x0427, 0x0407, 0 } },  // 28
    { 0x4001, { 8450, 0x0406, 1 } },    // 29
    { 0x6000, { 0x0429, 0x0408, 0 } },  // 2a
    { 0x4001, { -5090, 0x0406, 1 } },   // 2b
    { 0x6000, { 0x042b, 0x0407, 0 } }   // 2c
};
const mso_CustomShape msoActionButtonReturn =
{
    std::span<const SvxMSDffVertPair>(mso_sptActionButtonReturnVert),
    const_cast<sal_uInt16*>(mso_sptActionButtonReturnSegm), sizeof( mso_sptActionButtonReturnSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptActionButtonReturnCalc),
    const_cast<sal_Int32*>(mso_sptDefault1400),
    std::span<const SvxMSDffTextRectangles>(mso_sptActionButtonTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptButtonHandle),
};

const SvxMSDffVertPair mso_sptActionButtonDocumentVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 },
    { 0, 0 }, { 21600, 0 }, { 3 MSO_I, 2 MSO_I }, { 1 MSO_I, 2 MSO_I },
    { 21600, 0 }, { 21600, 21600 }, { 3 MSO_I, 4 MSO_I }, { 3 MSO_I, 2 MSO_I },
    { 21600, 21600 }, { 0, 21600 }, { 1 MSO_I, 4 MSO_I }, { 3 MSO_I, 4 MSO_I },
    { 0, 21600 }, { 0, 0 }, { 1 MSO_I, 2 MSO_I }, { 1 MSO_I, 4 MSO_I },

    { 0xa MSO_I, 0xc MSO_I }, { 0xe MSO_I, 0xc MSO_I }, { 0x10 MSO_I, 0x12 MSO_I }, { 0x10 MSO_I, 0x14 MSO_I },
    { 0xa MSO_I, 0x14 MSO_I }, { 0xe MSO_I, 0xc MSO_I }, { 0x10 MSO_I, 0x12 MSO_I }, { 0xe MSO_I, 0x12 MSO_I }
};
const sal_uInt16 mso_sptActionButtonDocumentSegm[] =
{
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,

    0x4000, 0x0004, 0x6001, 0x8000,
    0x4000, 0x0002, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptActionButtonDocumentCalc[] =    // adj value 0 - 5400
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x6000, { DFF_Prop_geoLeft, DFF_Prop_adjustValue, 0 } },
    { 0x6000, { DFF_Prop_geoTop, DFF_Prop_adjustValue, 0 } },
    { 0xa000, { DFF_Prop_geoRight, 0, DFF_Prop_adjustValue } },
    { 0xa000, { DFF_Prop_geoBottom, 0, DFF_Prop_adjustValue } },
    { 0x8000, { 10800, 0, DFF_Prop_adjustValue } },
    { 0x2001, { 0x0405, 1, 10800 } },           // scaling   6
    { 0x2001, { DFF_Prop_geoRight, 1, 2 } },    // lr center 7
    { 0x2001, { DFF_Prop_geoBottom, 1, 2 } },   // ul center 8

    { 0x4001, { -6350, 0x0406, 1 } },   // 9
    { 0x6000, { 0x0409, 0x0407, 0 } },  // a
    { 0x4001, { -7830, 0x0406, 1 } },   // b
    { 0x6000, { 0x040b, 0x0408, 0 } },  // c
    { 0x4001, { 1690, 0x0406, 1 } },    // d
    { 0x6000, { 0x040d, 0x0407, 0 } },  // e
    { 0x4001, { 6350, 0x0406, 1 } },    // f
    { 0x6000, { 0x040f, 0x0407, 0 } },  // 10
    { 0x4001, { -3810, 0x0406, 1 } },   // 11
    { 0x6000, { 0x0411, 0x0408, 0 } },  // 12
    { 0x4001, { 7830, 0x0406, 1 } },    // 13
    { 0x6000, { 0x0413, 0x0408, 0 } }   // 14
};
const mso_CustomShape msoActionButtonDocument =
{
    std::span<const SvxMSDffVertPair>(mso_sptActionButtonDocumentVert),
    const_cast<sal_uInt16*>(mso_sptActionButtonDocumentSegm), sizeof( mso_sptActionButtonDocumentSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptActionButtonDocumentCalc),
    const_cast<sal_Int32*>(mso_sptDefault1400),
    std::span<const SvxMSDffTextRectangles>(mso_sptActionButtonTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptButtonHandle),
};

const SvxMSDffVertPair mso_sptActionButtonSoundVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 },
    { 0, 0 }, { 21600, 0 }, { 3 MSO_I, 2 MSO_I }, { 1 MSO_I, 2 MSO_I },
    { 21600, 0 }, { 21600, 21600 }, { 3 MSO_I, 4 MSO_I }, { 3 MSO_I, 2 MSO_I },
    { 21600, 21600 }, { 0, 21600 }, { 1 MSO_I, 4 MSO_I }, { 3 MSO_I, 4 MSO_I },
    { 0, 21600 }, { 0, 0 }, { 1 MSO_I, 2 MSO_I }, { 1 MSO_I, 4 MSO_I },

    { 0xa MSO_I, 0xc MSO_I }, { 0xe MSO_I, 0xc MSO_I }, { 0x10 MSO_I, 0x12 MSO_I }, { 0x10 MSO_I, 0x14 MSO_I },
    { 0xe MSO_I, 0x16 MSO_I }, { 0xa MSO_I, 0x16 MSO_I }, { 0x18 MSO_I, 8 MSO_I }, { 0x1a MSO_I, 8 MSO_I },

    { 0x18 MSO_I, 0xc MSO_I }, { 0x1a MSO_I, 0x1c MSO_I },

    { 0x18 MSO_I, 0x16 MSO_I }, { 0x1a MSO_I, 0x1e MSO_I }
};
const sal_uInt16 mso_sptActionButtonSoundSegm[] =
{
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,

    0x4000, 0x0005, 0x6001, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffCalculationData mso_sptActionButtonSoundCalc[] =   // adj value 0 - 5400
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x6000, { DFF_Prop_geoLeft, DFF_Prop_adjustValue, 0 } },
    { 0x6000, { DFF_Prop_geoTop, DFF_Prop_adjustValue, 0 } },
    { 0xa000, { DFF_Prop_geoRight, 0, DFF_Prop_adjustValue } },
    { 0xa000, { DFF_Prop_geoBottom, 0, DFF_Prop_adjustValue } },
    { 0x8000, { 10800, 0, DFF_Prop_adjustValue } },
    { 0x2001, { 0x0405, 1, 10800 } },           // scaling   6
    { 0x2001, { DFF_Prop_geoRight, 1, 2 } },    // lr center 7
    { 0x2001, { DFF_Prop_geoBottom, 1, 2 } },   // ul center 8

    { 0x4001, { -8050, 0x0406, 1 } },   // 9
    { 0x6000, { 0x0409, 0x0407, 0 } },  // a
    { 0x4001, { -2750, 0x0406, 1 } },   // b
    { 0x6000, { 0x040b, 0x0408, 0 } },  // c
    { 0x4001, { -2960, 0x0406, 1 } },   // d
    { 0x6000, { 0x040d, 0x0407, 0 } },  // e
    { 0x4001, { 2120, 0x0406, 1 } },    // f
    { 0x6000, { 0x040f, 0x0407, 0 } },  // 10
    { 0x4001, { -8050, 0x0406, 1 } },   // 11
    { 0x6000, { 0x0411, 0x0408, 0 } },  // 12
    { 0x4001, { 8050, 0x0406, 1 } },    // 13
    { 0x6000, { 0x0413, 0x0408, 0 } },  // 14
    { 0x4001, { 2750, 0x0406, 1 } },    // 15
    { 0x6000, { 0x0415, 0x0408, 0 } },  // 16
    { 0x4001, { 4020, 0x0406, 1 } },    // 17
    { 0x6000, { 0x0417, 0x0407, 0 } },  // 18
    { 0x4001, { 8050, 0x0406, 1 } },    // 19
    { 0x6000, { 0x0419, 0x0407, 0 } },  // 1a
    { 0x4001, { -5930, 0x0406, 1 } },   // 1b
    { 0x6000, { 0x041b, 0x0408, 0 } },  // 1c
    { 0x4001, { 5930, 0x0406, 1 } },    // 1d
    { 0x6000, { 0x041d, 0x0408, 0 } }   // 1e
};
const mso_CustomShape msoActionButtonSound =
{
    std::span<const SvxMSDffVertPair>(mso_sptActionButtonSoundVert),
    const_cast<sal_uInt16*>(mso_sptActionButtonSoundSegm), sizeof( mso_sptActionButtonSoundSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptActionButtonSoundCalc),
    const_cast<sal_Int32*>(mso_sptDefault1400),
    std::span<const SvxMSDffTextRectangles>(mso_sptActionButtonTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptButtonHandle),
};

const SvxMSDffVertPair mso_sptActionButtonMovieVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 },
    { 0, 0 }, { 21600, 0 }, { 3 MSO_I, 2 MSO_I }, { 1 MSO_I, 2 MSO_I },
    { 21600, 0 }, { 21600, 21600 }, { 3 MSO_I, 4 MSO_I }, { 3 MSO_I, 2 MSO_I },
    { 21600, 21600 }, { 0, 21600 }, { 1 MSO_I, 4 MSO_I }, { 3 MSO_I, 4 MSO_I },
    { 0, 21600 }, { 0, 0 }, { 1 MSO_I, 2 MSO_I }, { 1 MSO_I, 4 MSO_I },

    { 0xa MSO_I, 0xc MSO_I }, { 0xe MSO_I, 0xc MSO_I }, { 0x10 MSO_I, 0x12 MSO_I }, { 0x14 MSO_I, 0x12 MSO_I },
    { 0x16 MSO_I, 0x18 MSO_I }, { 0x16 MSO_I, 0x1a MSO_I }, { 0x1c MSO_I, 0x1a MSO_I }, { 0x1e MSO_I, 0x18 MSO_I },
    { 0x20 MSO_I, 0x18 MSO_I }, { 0x20 MSO_I, 0x22 MSO_I }, { 0x1e MSO_I, 0x22 MSO_I }, { 0x1c MSO_I, 0x24 MSO_I },
    { 0x16 MSO_I, 0x24 MSO_I }, { 0x16 MSO_I, 0x26 MSO_I }, { 0x2a MSO_I, 0x26 MSO_I }, { 0x2a MSO_I, 0x28 MSO_I },
    { 0x10 MSO_I, 0x28 MSO_I }, { 0xe MSO_I, 0x2c MSO_I }, { 0xa MSO_I, 0x2c MSO_I }
};
const sal_uInt16 mso_sptActionButtonMovieSegm[] =
{
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0003, 0x6001, 0x8000,
    0x4000, 0x0012, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptActionButtonMovieCalc[] =   // adj value 0 - 5400
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x6000, { DFF_Prop_geoLeft, DFF_Prop_adjustValue, 0 } },
    { 0x6000, { DFF_Prop_geoTop, DFF_Prop_adjustValue, 0 } },
    { 0xa000, { DFF_Prop_geoRight, 0, DFF_Prop_adjustValue } },
    { 0xa000, { DFF_Prop_geoBottom, 0, DFF_Prop_adjustValue } },
    { 0x8000, { 10800, 0, DFF_Prop_adjustValue } },
    { 0x2001, { 0x0405, 1, 10800 } },           // scaling   6
    { 0x2001, { DFF_Prop_geoRight, 1, 2 } },    // lr center 7
    { 0x2001, { DFF_Prop_geoBottom, 1, 2 } },   // ul center 8

    { 0x4001, { -8050, 0x0406, 1 } },   // 9
    { 0x6000, { 0x0409, 0x0407, 0 } },  // a
    { 0x4001, { -4020, 0x0406, 1 } },   // b
    { 0x6000, { 0x040b, 0x0408, 0 } },  // c
    { 0x4001, { -7000, 0x0406, 1 } },   // d
    { 0x6000, { 0x040d, 0x0407, 0 } },  // e
    { 0x4001, { -6560, 0x0406, 1 } },   // f
    { 0x6000, { 0x040f, 0x0407, 0 } },  // 10
    { 0x4001, { -3600, 0x0406, 1 } },   // 11
    { 0x6000, { 0x0411, 0x0408, 0 } },  // 12
    { 0x4001, { 4020, 0x0406, 1 } },    // 13
    { 0x6000, { 0x0413, 0x0407, 0 } },  // 14
    { 0x4001, { 4660, 0x0406, 1 } },    // 15
    { 0x6000, { 0x0415, 0x0407, 0 } },  // 16
    { 0x4001, { -2960, 0x0406, 1 } },   // 17
    { 0x6000, { 0x0417, 0x0408, 0 } },  // 18
    { 0x4001, { -2330, 0x0406, 1 } },   // 19
    { 0x6000, { 0x0419, 0x0408, 0 } },  // 1a
    { 0x4001, { 6780, 0x0406, 1 } },    // 1b
    { 0x6000, { 0x041b, 0x0407, 0 } },  // 1c
    { 0x4001, { 7200, 0x0406, 1 } },    // 1d
    { 0x6000, { 0x041d, 0x0407, 0 } },  // 1e
    { 0x4001, { 8050, 0x0406, 1 } },    // 1f
    { 0x6000, { 0x041f, 0x0407, 0 } },  // 20
    { 0x4001, { 2960, 0x0406, 1 } },    // 21
    { 0x6000, { 0x0421, 0x0408, 0 } },  // 22
    { 0x4001, { 2330, 0x0406, 1 } },    // 23
    { 0x6000, { 0x0423, 0x0408, 0 } },  // 24
    { 0x4001, { 3800, 0x0406, 1 } },    // 25
    { 0x6000, { 0x0425, 0x0408, 0 } },  // 26
    { 0x4001, { -1060, 0x0406, 1 } },   // 27
    { 0x6000, { 0x0427, 0x0408, 0 } },  // 28
    { 0x4001, { -6350, 0x0406, 1 } },   // 29
    { 0x6000, { 0x0429, 0x0407, 0 } },  // 2a
    { 0x4001, { -640, 0x0406, 1 } },    // 2b
    { 0x6000, { 0x042b, 0x0408, 0 } }   // 2c
};
const mso_CustomShape msoActionButtonMovie =
{
    std::span<const SvxMSDffVertPair>(mso_sptActionButtonMovieVert),
    const_cast<sal_uInt16*>(mso_sptActionButtonMovieSegm), sizeof( mso_sptActionButtonMovieSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptActionButtonMovieCalc),
    const_cast<sal_Int32*>(mso_sptDefault1400),
    std::span<const SvxMSDffTextRectangles>(mso_sptActionButtonTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptButtonHandle),
};

const SvxMSDffVertPair mso_sptSmileyFaceVert[] = // adj value 15510 - 17520
{
    { 10800, 10800 }, { 10800, 10800 }, { 0, 360 },
    { 7305, 7515 }, { 1000, 1865 }, { 0, 360 },
    { 14295, 7515 }, { 1000, 1865 }, { 0, 360 },
    { 4870, 1 MSO_I }, { 8680, 2 MSO_I }, { 12920, 2 MSO_I }, { 16730, 1 MSO_I }
};
const sal_uInt16 mso_sptSmileyFaceSegm[] =
{
    0xa203, 0x6000, 0x8000,
    0xa203, 0x6000, 0x8000,
    0xa203, 0x6000, 0x8000,
    0x4000, 0x2001, 0xaa00, 0x8000
};
const SvxMSDffCalculationData mso_sptSmileyFaceCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 14510 } },
    { 0x8000, { 18520, 0, 0x400 } },
    { 0x4000, { 14510, 0x400, 0 } }
};

const sal_Int32 mso_sptSmileyFaceDefault[] =
{
    1, 18520
};
const SvxMSDffHandle mso_sptSmileyHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        10800, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 14510, 18520 }
};
const mso_CustomShape msoSmileyFace =
{
    std::span<const SvxMSDffVertPair>(mso_sptSmileyFaceVert),
    const_cast<sal_uInt16*>(mso_sptSmileyFaceSegm), sizeof( mso_sptSmileyFaceSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptSmileyFaceCalc),
    const_cast<sal_Int32*>(mso_sptSmileyFaceDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptEllipseTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptEllipseGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptSmileyHandle),
};

const SvxMSDffVertPair mso_sptDonutVert[] =  // adj value 0 - 10800
{
    { 10800, 10800 }, { 10800, 10800 }, { 0, 360 },
    { 10800, 10800 }, { 1 MSO_I, 1 MSO_I }, { 0, 360 }
};
const sal_uInt16 mso_sptDonutSegm[] =
{
    0xa203, 0x6000, 0xa203, 0x8000
};
const SvxMSDffCalculationData mso_sptDonutCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x8000, { 10800, 0, DFF_Prop_adjustValue } }
};
const SvxMSDffHandle mso_sptDonutHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 10800, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoDonut =
{
    std::span<const SvxMSDffVertPair>(mso_sptDonutVert),
    const_cast<sal_uInt16*>(mso_sptDonutSegm), sizeof( mso_sptDonutSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptDonutCalc),
    const_cast<sal_Int32*>(mso_sptDefault5400),
    std::span<const SvxMSDffTextRectangles>(mso_sptEllipseTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptEllipseGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptDonutHandle),
};

const SvxMSDffVertPair mso_sptNoSmokingVert[] =  // adj value 0 - 7200
{
    { 10800, 10800 },  { 10800, 10800 }, { 0, 360 },
    { 0 MSO_I, 0 MSO_I }, { 1 MSO_I, 1 MSO_I },
    { 9 MSO_I, 0xa MSO_I }, { 0xb MSO_I, 0xc MSO_I }, { 0 MSO_I, 0 MSO_I }, { 1 MSO_I, 1 MSO_I },
    { 0xd MSO_I, 0xe MSO_I }, { 0xf MSO_I, 0x10 MSO_I }
};
const sal_uInt16 mso_sptNoSmokingSegm[] =
{
    0xa203, 0x6000, 0xa404, 0x6000, 0xa404, 0x6000, 0x8000
};
const SvxMSDffCalculationData mso_sptNoSmokingCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },         // 0
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } },     // 1
    { 0x8000, { 10800, 0, DFF_Prop_adjustValue } },     // 2
    { 0x2001, { DFF_Prop_adjustValue, 1, 2 } },         // 3
    { 0xa080, { 0x403, 0, 0x402 } },                    // 4
    { 0x8000, { 10800, 0, 0x403 } },                    // 5 x1
    { 0x4000, { 10800, 0x403, 0 } },                    // 6 x2
    { 0x8000, { 10800, 0, 0x404 } },                    // 7 y1
    { 0x4000, { 10800, 0x404, 0 } },                    // 8 y2
    { 0x6081, { 0x405, 0x407, 45 } },                   // 9
    { 0x6082, { 0x405, 0x407, 45 } },                   // a
    { 0x6081, { 0x405, 0x408, 45 } },                   // b
    { 0x6082, { 0x405, 0x408, 45 } },                   // c
    { 0x6081, { 0x406, 0x408, 45 } },                   // d
    { 0x6082, { 0x406, 0x408, 45 } },                   // e
    { 0x6081, { 0x406, 0x407, 45 } },                   // f
    { 0x6082, { 0x406, 0x407, 45 } }                    // 10
};
const SvxMSDffHandle mso_sptNoSmokingHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 10800, 10800, 10800, 0, 7200, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoNoSmoking =
{
    std::span<const SvxMSDffVertPair>(mso_sptNoSmokingVert),
    const_cast<sal_uInt16*>(mso_sptNoSmokingSegm), sizeof( mso_sptNoSmokingSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptNoSmokingCalc),
    const_cast<sal_Int32*>(mso_sptDefault2700),
    std::span<const SvxMSDffTextRectangles>(mso_sptEllipseTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptEllipseGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptNoSmokingHandle),
};

const SvxMSDffVertPair mso_sptBlockArcVert[] =   // adj value 0 (degrees)
{                                                       // adj value 1: 0 -> 10800;
    { 0, 0 }, { 21600, 21600 }, { 4 MSO_I, 3 MSO_I }, { 2 MSO_I, 3 MSO_I },
    { 5 MSO_I, 5 MSO_I }, { 6 MSO_I, 6 MSO_I }, { 2 MSO_I, 3 MSO_I }, { 4 MSO_I, 3 MSO_I }
};
const sal_uInt16 mso_sptBlockArcSegm[] =
{
    0xA404, 0xa504, 0x6001, 0x8000
};
const sal_Int32 mso_sptBlockArcDefault[] =
{
    2, 180, 5400
};
const SvxMSDffCalculationData mso_sptBlockArcCalc[] =
{
    { 0x400a, { 10800, DFF_Prop_adjustValue, 0 } },
    { 0x4009, { 10800, DFF_Prop_adjustValue, 0 } },
    { 0x2000, { 0x400, 10800, 0 } },
    { 0x2000, { 0x401, 10800, 0 } },
    { 0x8000, { 21600, 0, 0x402 } },
    { 0x8000, { 10800, 0, DFF_Prop_adjust2Value } },
    { 0x4000, { 10800, DFF_Prop_adjust2Value, 0 } },
    { 0x600a, { 0x405, DFF_Prop_adjustValue, 0 } },
    { 0x6009, { 0x405, DFF_Prop_adjustValue, 0 } }
};
const SvxMSDffHandle mso_sptBlockArcHandle[] =
{
    {   SvxMSDffHandleFlags::POLAR | SvxMSDffHandleFlags::RADIUS_RANGE,
        0x101, 0x100, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoBlockArc =
{
    std::span<const SvxMSDffVertPair>(mso_sptBlockArcVert),
    const_cast<sal_uInt16*>(mso_sptBlockArcSegm), sizeof( mso_sptBlockArcSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptBlockArcCalc),
    const_cast<sal_Int32*>(mso_sptBlockArcDefault),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptBlockArcHandle),
};

// aware : control points are always part of the bounding box
const SvxMSDffVertPair mso_sptHeartVert[] =
{
    { 10800, 21599 }, { 321, 6886 }, { 70, 6036 },      // ppp
    { -9, 5766 }, { -1, 5474 }, { 2, 5192 },            // ccp
    { 6, 4918 }, { 43, 4641 }, { 101, 4370 },           // ccp
    { 159, 4103 }, { 245, 3837 }, { 353, 3582 },        // ccp
    { 460, 3326 }, { 591, 3077 }, { 741, 2839 },        // ccp
    { 892, 2598 }, { 1066, 2369 }, { 1253, 2155 },      // ccp
    { 1443, 1938 }, { 1651, 1732 }, { 1874, 1543 },     // ccp
    { 2097, 1351 }, { 2337, 1174 }, { 2587, 1014 },     // ccp
    { 2839, 854 }, { 3106, 708 }, { 3380, 584 },        // ccp
    { 3656, 459 }, { 3945, 350 }, { 4237, 264 },        // ccp
    { 4533, 176 }, { 4838, 108 }, { 5144, 66 },         // ccp
    { 5454, 22 }, { 5771, 1 }, { 6086, 3 },             // ccp
    { 6407, 7 }, { 6731, 35 }, { 7048, 89 },            // ccp
    { 7374, 144 }, { 7700, 226 }, { 8015, 335 },        // ccp
    { 8344, 447 }, { 8667, 590 }, { 8972, 756 },        // ccp
    { 9297, 932 }, { 9613, 1135 }, { 9907, 1363 },      // ccp
    { 10224, 1609 }, { 10504, 1900 }, { 10802, 2169 },  // ccp
    { 11697, 1363 },                                    // p
    { 11971, 1116 }, { 12304, 934 }, { 12630, 756 },    // ccp
    { 12935, 590 }, { 13528, 450 }, { 13589, 335 },     // ccp
    { 13901, 226 }, { 14227, 144 }, { 14556, 89 },      // ccp
    { 14872, 35 }, { 15195, 7 }, { 15517, 3 },          // ccp
    { 15830, 0 }, { 16147, 22 }, { 16458, 66 },         // ccp
    { 16764, 109 }, { 17068, 177 }, { 17365, 264 },     // ccp
    { 17658, 349 }, { 17946, 458 }, { 18222, 584 },     // ccp
    { 18496, 708 }, { 18762, 854 }, { 19015, 1014 },    // ccp
    { 19264, 1172 }, { 19504, 1349 }, { 19730, 1543 },  // ccp
    { 19950, 1731 }, { 20158, 1937 }, { 20350, 2155 },  // ccp
    { 20536, 2369 }, { 20710, 2598 }, { 20861, 2839 },  // ccp
    { 21010, 3074 }, { 21143, 3323 }, { 21251, 3582 },  // ccp
    { 21357, 3835 }, { 21443, 4099 }, { 21502, 4370 },  // ccp
    { 21561, 4639 }, { 21595, 4916 }, { 21600, 5192 },  // ccp
    { 21606, 5474 }, { 21584, 5760 }, { 21532, 6036 },  // ccp
    { 21478, 6326 }, { 21366, 6603 }, { 21282, 6887 },  // ccp
    { 10802, 21602 }                                    // p
};
const sal_uInt16 mso_sptHeartSegm[] =
{
    0x4000, 0x0002, 0x2010, 0x0001, 0x2010, 0x0001, 0x6001, 0x8000
};
const SvxMSDffTextRectangles mso_sptHeartTextRect[] =
{
    { { 5080, 2540 }, { 16520, 13550 } }
};
const SvxMSDffVertPair mso_sptHeartGluePoints[] =
{
    { 10800, 2180 }, { 3090, 10800 }, { 10800, 21600 }, { 18490, 10800 }
};
const mso_CustomShape msoHeart =
{
    std::span<const SvxMSDffVertPair>(mso_sptHeartVert),
    const_cast<sal_uInt16*>(mso_sptHeartSegm), sizeof( mso_sptHeartSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptHeartTextRect),
    21615, 21602,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptHeartGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptLightningBoldVert[] =
{
    { 8458, 0 }, { 0, 3923 }, { 7564, 8416 }, { 4993, 9720 },
    { 12197, 13904 }, { 9987, 14934 }, { 21600, 21600 }, { 14768, 12911 },
    { 16558, 12016 }, { 11030, 6840 }, { 12831, 6120 }, { 8458, 0 }
};
const SvxMSDffTextRectangles mso_sptLightningBoldTextRect[] =
{
    { { 8680, 7410 }, { 13970, 14190 } }
};
const SvxMSDffVertPair mso_sptLightningBoldGluePoints[] =
{
    { 8458, 0 }, { 0, 3923 }, { 4993, 9720 }, { 9987, 14934 }, { 21600, 21600 },
    { 16558, 12016 }, { 12831, 6120 }
};
const mso_CustomShape msoLightningBold =
{
    std::span<const SvxMSDffVertPair>(mso_sptLightningBoldVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptLightningBoldTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptLightningBoldGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptSunVert[] =    // adj value 2700 -> 10125
{
    { 0, 10800 },               { 4 MSO_I, 8 MSO_I },       { 4 MSO_I, 9 MSO_I },
    { 0x0a MSO_I, 0x0b MSO_I }, { 0x0c MSO_I, 0x0d MSO_I }, { 0x0e MSO_I, 0x0f MSO_I },
    { 0x10 MSO_I, 0x11 MSO_I }, { 0x12 MSO_I, 0x13 MSO_I }, { 0x14 MSO_I, 0x15 MSO_I },
    { 0x16 MSO_I, 0x17 MSO_I }, { 0x18 MSO_I, 0x19 MSO_I }, { 0x1a MSO_I, 0x1b MSO_I },
    { 0x1c MSO_I, 0x1d MSO_I }, { 0x1e MSO_I, 0x1f MSO_I }, { 0x20 MSO_I, 0x21 MSO_I },
    { 0x22 MSO_I, 0x23 MSO_I }, { 0x24 MSO_I, 0x25 MSO_I }, { 0x26 MSO_I, 0x27 MSO_I },
    { 0x28 MSO_I, 0x29 MSO_I }, { 0x2a MSO_I, 0x2b MSO_I }, { 0x2c MSO_I, 0x2d MSO_I },
    { 0x2e MSO_I, 0x2f MSO_I }, { 0x30 MSO_I, 0x31 MSO_I }, { 0x32 MSO_I, 0x33 MSO_I },
    { 10800, 10800 }, { 0x36 MSO_I, 0x36 MSO_I },   { 0, 360 }
};
const sal_uInt16 mso_sptSunSegm[] =
{
    0x4000, 0x0002, 0x6001, 0x8000,
    0x4000, 0x0002, 0x6001, 0x8000,
    0x4000, 0x0002, 0x6001, 0x8000,
    0x4000, 0x0002, 0x6001, 0x8000,
    0x4000, 0x0002, 0x6001, 0x8000,
    0x4000, 0x0002, 0x6001, 0x8000,
    0x4000, 0x0002, 0x6001, 0x8000,
    0x4000, 0x0002, 0x6001, 0x8000,
    0xa203, 0x6000, 0x8000
};
const SvxMSDffCalculationData mso_sptSunCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } },
    { 0x2000, { DFF_Prop_adjustValue, 0, 2700 } },
    { 0x2001, { 0x402, 5080, 7425 } },
    { 0x2000, { 0x403, 2540, 0 } },
    { 0x8000, { 10125, 0, DFF_Prop_adjustValue } },
    { 0x2001, { 0x405, 2120, 7425 } },
    { 0x2000, { 0x406, 210, 0 } },
    { 0x4000, { 10800, 0x407, 0 } },    // y1 (0x8)
    { 0x8000, { 10800, 0, 0x407 } },    // y2 (0x9)
    { 0x0081, { 0, 10800, 45 } },       // 0xa
    { 0x0082, { 0, 10800, 45 } },       // 0xb
    { 0x6081, { 0x404, 0x408, 45 } },   // 0xc
    { 0x6082, { 0x404, 0x408, 45 } },   // 0xd
    { 0x6081, { 0x404, 0x409, 45 } },   // 0xe
    { 0x6082, { 0x404, 0x409, 45 } },   // 0xf
    { 0x0081, { 0, 10800, 90 } },       // 0x10
    { 0x0082, { 0, 10800, 90 } },       // 0x11
    { 0x6081, { 0x404, 0x408, 90 } },   // 0x12
    { 0x6082, { 0x404, 0x408, 90 } },   // 0x13
    { 0x6081, { 0x404, 0x409, 90 } },   // 0x14
    { 0x6082, { 0x404, 0x409, 90 } },   // 0x15
    { 0x0081, { 0, 10800, 135 } },      // 0x16
    { 0x0082, { 0, 10800, 135 } },      // 0x17
    { 0x6081, { 0x404, 0x408, 135 } },  // 0x18
    { 0x6082, { 0x404, 0x408, 135 } },  // 0x19
    { 0x6081, { 0x404, 0x409, 135 } },  // 0x1a
    { 0x6082, { 0x404, 0x409, 135 } },  // 0x1b
    { 0x0081, { 0, 10800, 180 } },      // 0x1c
    { 0x0082, { 0, 10800, 180 } },      // 0x1d
    { 0x6081, { 0x404, 0x408, 180 } },  // 0x1e
    { 0x6082, { 0x404, 0x408, 180 } },  // 0x1f
    { 0x6081, { 0x404, 0x409, 180 } },  // 0x20
    { 0x6082, { 0x404, 0x409, 180 } },  // 0x21
    { 0x0081, { 0, 10800, 225 } },      // 0x22
    { 0x0082, { 0, 10800, 225 } },      // 0x23
    { 0x6081, { 0x404, 0x408, 225 } },  // 0x24
    { 0x6082, { 0x404, 0x408, 225 } },  // 0x25
    { 0x6081, { 0x404, 0x409, 225 } },  // 0x26
    { 0x6082, { 0x404, 0x409, 225 } },  // 0x27
    { 0x0081, { 0, 10800, 270 } },      // 0x28
    { 0x0082, { 0, 10800, 270 } },      // 0x29
    { 0x6081, { 0x404, 0x408, 270 } },  // 0x2a
    { 0x6082, { 0x404, 0x408, 270 } },  // 0x2b
    { 0x6081, { 0x404, 0x409, 270 } },  // 0x2c
    { 0x6082, { 0x404, 0x409, 270 } },  // 0x2d
    { 0x0081, { 0, 10800, 315 } },      // 0x2e
    { 0x0082, { 0, 10800, 315 } },      // 0x2f
    { 0x6081, { 0x404, 0x408, 315 } },  // 0x30
    { 0x6082, { 0x404, 0x408, 315 } },  // 0x31
    { 0x6081, { 0x404, 0x409, 315 } },  // 0x32
    { 0x6082, { 0x404, 0x409, 315 } },  // 0x33
    { 0x2081, { DFF_Prop_adjustValue, 10800, 45 } },    // 0x34 ( textbox )
    { 0x2081, { DFF_Prop_adjustValue, 10800, 225 } },   // 0x35
    { 0x8000, { 10800, 0, DFF_Prop_adjustValue } }
};
const SvxMSDffTextRectangles mso_sptSunTextRect[] =
{
    { { 0x34 MSO_I, 0x34 MSO_I }, { 0x35 MSO_I, 0x35 MSO_I } }
};
const SvxMSDffHandle mso_sptSunHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 10800, 10800, 10800, 2700, 10125, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoSun =
{
    std::span<const SvxMSDffVertPair>(mso_sptSunVert),
    const_cast<sal_uInt16*>(mso_sptSunSegm), sizeof( mso_sptSunSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptSunCalc),
    const_cast<sal_Int32*>(mso_sptDefault5400),
    std::span<const SvxMSDffTextRectangles>(mso_sptSunTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptSunHandle),
};

const SvxMSDffVertPair mso_sptMoonVert[] =   // adj value 0 -> 18900
{
    { 21600, 0 },
    { 3 MSO_I, 4 MSO_I },   { 0 MSO_I, 5080 },      { 0 MSO_I, 10800 }, // ccp
    { 0 MSO_I, 16520 },     { 3 MSO_I, 5 MSO_I },   { 21600, 21600 },   // ccp
    { 9740, 21600 },        { 0, 16730 },           { 0, 10800 },       // ccp
    { 0, 4870 },            { 9740, 0 },            { 21600, 0  }       // ccp
};
const sal_uInt16 mso_sptMoonSegm[] =
{
    0x4000, 0x2004, 0x6000, 0x8000
};
const SvxMSDffCalculationData mso_sptMoonCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } },
    { 0x2001, { 0x401, 1, 2 } },
    { 0x6000, { 0x402, DFF_Prop_adjustValue, 0 } },
    { 0x2001, { DFF_Prop_adjustValue, 1794, 10000 } },
    { 0x8000, { 21600, 0, 0x0404 } },
    { 0x2001, { DFF_Prop_adjustValue, 400, 18900 } },
    { 0x8081, { 0, 10800, 0x406 } },
    { 0x8082, { 0, 10800, 0x406 } },
    { 0x6000, { 0x407, 0x407, 0 } },
    { 0x8000, { 21600, 0, 0x408 } }
};
const SvxMSDffTextRectangles mso_sptMoonTextRect[] =
{
    { { 9 MSO_I, 8 MSO_I }, { 0 MSO_I, 0xa MSO_I } }
};
const SvxMSDffVertPair mso_sptMoonGluePoints[] =
{
    { 21600, 0 }, { 0, 10800 }, { 21600, 21600 }, { 0 MSO_I, 10800 }
};
const SvxMSDffHandle mso_sptMoonHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 10800, 10800, 10800, 0, 18900, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoMoon =
{
    std::span<const SvxMSDffVertPair>(mso_sptMoonVert),
    const_cast<sal_uInt16*>(mso_sptMoonSegm), sizeof( mso_sptMoonSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptMoonCalc),
    const_cast<sal_Int32*>(mso_sptDefault10800),
    std::span<const SvxMSDffTextRectangles>(mso_sptMoonTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptMoonGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptMoonHandle),
};

const SvxMSDffVertPair mso_sptBracketPairVert[] =    // adj value 0 -> 10800
{
    { 0 MSO_I, 0 },     { 0, 1 MSO_I },     // left top alignment
    { 0, 2 MSO_I },     { 0 MSO_I, 21600 }, // left  bottom "
    { 3 MSO_I, 21600 }, { 21600, 2 MSO_I }, // right bottom "
    { 21600, 1 MSO_I }, { 3 MSO_I, 0 },     // right top    "
    { 0 MSO_I, 0 },     { 0, 1 MSO_I },     // filling area
    { 0, 2 MSO_I },     { 0 MSO_I, 21600 },
    { 3 MSO_I, 21600 }, { 21600, 2 MSO_I },
    { 21600, 1 MSO_I }, { 3 MSO_I, 0 }
};
const sal_uInt16 mso_sptBracketPairSegm[] =
{
    0x4000, 0xa701, 0x0001, 0xa801, 0xaa00, 0x8000,
    0x4000, 0xa701, 0x0001, 0xa801, 0xaa00, 0x8000,
    0x4000, 0xa701, 0x0001, 0xa801, 0x0001,             // filling area
    0xa701, 0x0001, 0xa801, 0x6000, 0xab00, 0x8000
};
const SvxMSDffCalculationData mso_sptBracketPairCalc[] =
{
    { 0x6000, { DFF_Prop_geoLeft, DFF_Prop_adjustValue, 0 } },
    { 0x6000, { DFF_Prop_geoTop, DFF_Prop_adjustValue, 0 } },
    { 0xa000, { DFF_Prop_geoBottom, 0, DFF_Prop_adjustValue } },
    { 0xa000, { DFF_Prop_geoRight, 0, DFF_Prop_adjustValue } },
    { 0x2082, { DFF_Prop_adjustValue, 0, 45 } },
    { 0x2000, { 0x404, 0, 10800 } },
    { 0x8000, { 0, 0, DFF_Prop_adjustValue } },
    { 0xa000, { 0x406, 0, 0x405 } },
    { 0xa000, { DFF_Prop_geoLeft, 0, 0x407 } },
    { 0xa000, { DFF_Prop_geoTop, 0, 0x407 } },
    { 0x6000, { DFF_Prop_geoRight, 0x407, 0 } },
    { 0x6000, { DFF_Prop_geoBottom, 0x407, 0 } },
    { 0xa000, { DFF_Prop_geoLeft, 0, 0x405 } },
    { 0xa000, { DFF_Prop_geoTop, 0, 0x405 } },
    { 0x6000, { DFF_Prop_geoRight, 0x405, 0 } },
    { 0x6000, { DFF_Prop_geoBottom, 0x405, 0 } }
};
const SvxMSDffTextRectangles mso_sptBracketPairTextRect[] =
{
    { { 8 MSO_I, 9 MSO_I }, { 0xa MSO_I, 0xb MSO_I } }
};
const SvxMSDffHandle mso_sptBracketPairHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::SWITCHED,
        0x100, 0, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoBracketPair =
{
    std::span<const SvxMSDffVertPair>(mso_sptBracketPairVert),
    const_cast<sal_uInt16*>(mso_sptBracketPairSegm), sizeof( mso_sptBracketPairSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptBracketPairCalc),
    const_cast<sal_Int32*>(mso_sptDefault3700),
    std::span<const SvxMSDffTextRectangles>(mso_sptBracketPairTextRect),
    21600, 21600,
    10800, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptBracketPairHandle),
};

const sal_uInt16 mso_sptPlaqueSegm[] =
{
    0x4000, 0xa801, 0x0001, 0xa701, 0x0001, 0xa801, 0x0001, 0xa701, 0x6000, 0x8000
};
const SvxMSDffTextRectangles mso_sptPlaqueTextRect[] =
{
    { { 0xc MSO_I, 0xd MSO_I }, { 0xe MSO_I, 0xf MSO_I } }
};
const SvxMSDffHandle mso_sptPlaqueHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::SWITCHED,
        0x100, 0, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoPlaque =
{
    std::span<const SvxMSDffVertPair>(mso_sptBracketPairVert),
    const_cast<sal_uInt16*>(mso_sptPlaqueSegm), sizeof( mso_sptPlaqueSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptBracketPairCalc),
    const_cast<sal_Int32*>(mso_sptDefault3600),
    std::span<const SvxMSDffTextRectangles>(mso_sptPlaqueTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptPlaqueHandle),
};

const SvxMSDffVertPair mso_sptBracePairVert[] =  // adj value 0 -> 5400
{
    { 4 MSO_I, 0 }, { 0 MSO_I, 1 MSO_I }, { 0 MSO_I, 6 MSO_I }, { 0 ,10800 },           // left bracket
    { 0 MSO_I, 7 MSO_I }, { 0 MSO_I, 2 MSO_I }, { 4 MSO_I, 21600 },
    { 8 MSO_I, 21600 }, { 3 MSO_I, 2 MSO_I }, { 3 MSO_I, 7 MSO_I }, { 21600, 10800 },   // right bracket
    { 3 MSO_I, 6 MSO_I }, { 3 MSO_I, 1 MSO_I }, { 8 MSO_I, 0 },
    { 4 MSO_I, 0 }, { 0 MSO_I, 1 MSO_I }, { 0 MSO_I, 6 MSO_I }, { 0 ,10800 },           // filling area
    { 0 MSO_I, 7 MSO_I }, { 0 MSO_I, 2 MSO_I }, { 4 MSO_I, 21600 },
    { 8 MSO_I, 21600 }, { 3 MSO_I, 2 MSO_I }, { 3 MSO_I, 7 MSO_I }, { 21600, 10800 },
    { 3 MSO_I, 6 MSO_I }, { 3 MSO_I, 1 MSO_I }, { 8 MSO_I, 0 }
};
const sal_uInt16 mso_sptBracePairSegm[] =
{
    0x4000, 0xa701, 0x0001, 0xa801, 0xa701, 0x0001, 0xa801, 0xaa00, 0x8000,
    0x4000, 0xa701, 0x0001, 0xa801, 0xa701, 0x0001, 0xa801, 0xaa00, 0x8000,
    0x4000, 0xa701, 0x0001, 0xa801, 0xa701, 0x0001, 0xa801, 0x0001,                     // filling area
    0xa701, 0x0001, 0xa801, 0xa701, 0x0001, 0xa801, 0x6000, 0xab00, 0x8000
};
const SvxMSDffCalculationData mso_sptBracePairCalc[] =
{
    { 0x6000, { DFF_Prop_geoLeft, DFF_Prop_adjustValue, 0 } },
    { 0x6000, { DFF_Prop_geoTop, DFF_Prop_adjustValue, 0 } },
    { 0xa000, { DFF_Prop_geoBottom, 0, DFF_Prop_adjustValue } },
    { 0xa000, { DFF_Prop_geoRight, 0, DFF_Prop_adjustValue } },
    { 0x2001, { 0x400, 2, 1 } },                                    //  4
    { 0x2001, { DFF_Prop_adjustValue, 2, 1 } },                     //  5
    { 0x8000, { 10800, 0, DFF_Prop_adjustValue } },                 //  6
    { 0x8000, { 21600, 0, 0x406 } },                                //  7
    { 0xa000, { DFF_Prop_geoRight, 0, 0x405 } },                    //  8
    { 0x2001, { DFF_Prop_adjustValue, 1, 3 } },                     //  9
    { 0x6000, { 0x409, DFF_Prop_adjustValue, 0 } },                 // xa
    { 0x6000, { DFF_Prop_geoLeft, 0x40a, 0 } },                     // xb
    { 0x6000, { DFF_Prop_geoTop, 0x409, 0 } },                      // xc
    { 0xa000, { DFF_Prop_geoRight, 0, 0x40a } },                    // xd
    { 0xa000, { DFF_Prop_geoBottom, 0, 0x409 } }                    // xe
};
const SvxMSDffTextRectangles mso_sptBracePairTextRect[] =
{
    { { 0xb MSO_I, 0xc MSO_I }, { 0xd MSO_I, 0xe MSO_I } }
};
const SvxMSDffHandle mso_sptBracePairHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::SWITCHED,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 5400 }
};
const mso_CustomShape msoBracePair =
{
    std::span<const SvxMSDffVertPair>(mso_sptBracePairVert),
    const_cast<sal_uInt16*>(mso_sptBracePairSegm), sizeof( mso_sptBracePairSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptBracePairCalc),
    const_cast<sal_Int32*>(mso_sptDefault1800),
    std::span<const SvxMSDffTextRectangles>(mso_sptBracePairTextRect),
    21600, 21600,
    10800, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptBracePairHandle),
};

const SvxMSDffCalculationData mso_sptBracketCalc[] =
{
    { 0x2001, { DFF_Prop_adjustValue, 1, 2 } },
    { 0x6000, { DFF_Prop_geoTop, DFF_Prop_adjustValue, 0 } },
    { 0xa000, { DFF_Prop_geoBottom, 0, DFF_Prop_adjustValue } },
    { 0x6000, { DFF_Prop_geoTop, 0x400, 0 } },
    { 0xa000, { DFF_Prop_geoBottom, 0, 0x400 } }
};
const sal_uInt16 mso_sptBracketSegm[] =
{
    0x4000, 0x2001, 0x0001, 0x2001, 0x8000
};
const SvxMSDffVertPair mso_sptLeftBracketVert[] =    // adj value 0 -> 10800
{
    { 21600, 0 }, { 10800,  0 }, { 0, 3 MSO_I }, { 0, 1 MSO_I },
    { 0, 2 MSO_I }, { 0, 4 MSO_I }, { 10800, 21600 }, { 21600, 21600 }
};
const SvxMSDffTextRectangles mso_sptLeftBracketTextRect[] =
{
    { { 6350, 3 MSO_I }, { 21600, 4 MSO_I } }
};
const SvxMSDffVertPair mso_sptLeftBracketGluePoints[] =
{
    { 21600, 0 }, { 0, 10800 }, { 21600, 21600 }
};
const SvxMSDffHandle mso_sptLeftBracketHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 10800 }
};
const mso_CustomShape msoLeftBracket =
{
    std::span<const SvxMSDffVertPair>(mso_sptLeftBracketVert),
    const_cast<sal_uInt16*>(mso_sptBracketSegm), sizeof( mso_sptBracketSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptBracketCalc),
    const_cast<sal_Int32*>(mso_sptDefault1800),
    std::span<const SvxMSDffTextRectangles>(mso_sptLeftBracketTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptLeftBracketGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptLeftBracketHandle),
};
const SvxMSDffVertPair mso_sptRightBracketVert[] =   // adj value 0 -> 10800
{
    { 0, 0 }, { 10800, 0 }, { 21600, 3 MSO_I }, { 21600, 1 MSO_I },
    { 21600, 2 MSO_I }, { 21600, 4 MSO_I }, { 10800, 21600 }, { 0, 21600 }
};
const SvxMSDffTextRectangles mso_sptRightBracketTextRect[] =
{
    { { 0, 3 MSO_I }, { 15150, 4 MSO_I } }
};
const SvxMSDffVertPair mso_sptRightBracketGluePoints[] =
{
    { 0, 0 }, { 0, 21600 }, { 21600, 10800 }
};
const SvxMSDffHandle mso_sptRightBracketHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        1, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 10800 }
};
const mso_CustomShape msoRightBracket =
{
    std::span<const SvxMSDffVertPair>(mso_sptRightBracketVert),
    const_cast<sal_uInt16*>(mso_sptBracketSegm), sizeof( mso_sptBracketSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptBracketCalc),
    const_cast<sal_Int32*>(mso_sptDefault1800),
    std::span<const SvxMSDffTextRectangles>(mso_sptRightBracketTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptRightBracketGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptRightBracketHandle),
};

const SvxMSDffCalculationData mso_sptBraceCalc[] =
{
    { 0x2001, { DFF_Prop_adjustValue, 1, 2 } },
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0xa000, { 0x404, 0, DFF_Prop_adjustValue } },
    { 0xa000, { 0x404, 0, 0x400 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x6000, { 0x404, 0x400, 0 } },
    { 0x6000, { 0x404, DFF_Prop_adjustValue, 0 } },
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } },
    { 0x8000, { 21600, 0, 0x400 } },
    { 0x2001, { DFF_Prop_adjustValue, 10000, 31953 } },
    { 0x8000, { 21600, 0, 0x409 } }
};
const sal_uInt16 mso_sptBraceSegm[] =
{
    0x4000, 0x2001, 0x0001, 0x2002, 0x0001, 0x2001, 0x8000
};
const sal_Int32 mso_sptBraceDefault[] =
{
    2, 1800, 10800
};
const SvxMSDffVertPair mso_sptLeftBraceVert[] =
{
    { 21600, 0 },                                               // p
    { 16200, 0 }, { 10800, 0 MSO_I }, { 10800, 1 MSO_I },       // ccp
    { 10800, 2 MSO_I },                                         // p
    { 10800, 3 MSO_I }, { 5400, 4 MSO_I }, { 0, 4 MSO_I },      // ccp
    { 5400, 4 MSO_I },  { 10800, 5 MSO_I }, { 10800, 6 MSO_I }, // ccp
    { 10800, 7 MSO_I },                                         // p
    { 10800, 8 MSO_I }, { 16200, 21600 }, { 21600, 21600 }      // ccp
};
const SvxMSDffTextRectangles mso_sptLeftBraceTextRect[] =
{
    { { 13800, 9 MSO_I }, { 21600, 10 MSO_I } }
};
const SvxMSDffHandle mso_sptLeftBraceHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        10800, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 5400 },
    {   SvxMSDffHandleFlags::RANGE,
        0, 0x101, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 21600 }
};
const mso_CustomShape msoLeftBrace =     // adj value0 0 -> 5400
{                                               // adj value1 0 -> 21600
    std::span<const SvxMSDffVertPair>(mso_sptLeftBraceVert),
    const_cast<sal_uInt16*>(mso_sptBraceSegm), sizeof( mso_sptBraceSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptBraceCalc),
    const_cast<sal_Int32*>(mso_sptBraceDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptLeftBraceTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptLeftBracketGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptLeftBraceHandle),
};
const SvxMSDffVertPair mso_sptRightBraceVert[] =
{
    { 0, 0 },                                                   // p
    { 5400, 0 }, { 10800, 0 MSO_I }, { 10800, 1 MSO_I },        // ccp
    { 10800, 2 MSO_I },                                         // p
    { 10800, 3 MSO_I }, { 16200, 4 MSO_I }, { 21600, 4 MSO_I }, // ccp
    { 16200, 4 MSO_I }, { 10800, 5 MSO_I }, { 10800, 6 MSO_I }, // ccp
    { 10800, 7 MSO_I },                                         // p
    { 10800, 8 MSO_I }, { 5400, 21600 }, { 0, 21600 }           // ccp
};
const SvxMSDffTextRectangles mso_sptRightBraceTextRect[] =
{
    { { 0, 9 MSO_I }, { 7800, 10 MSO_I } }
};
const SvxMSDffHandle mso_sptRightBraceHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        10800, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 5400 },
    {   SvxMSDffHandleFlags::RANGE,
        1, 0x101, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 21600 }
};
const mso_CustomShape msoRightBrace =        // adj value0 0 -> 5400
{                                               // adj value1 0 -> 21600
    std::span<const SvxMSDffVertPair>(mso_sptRightBraceVert),
    const_cast<sal_uInt16*>(mso_sptBraceSegm), sizeof( mso_sptBraceSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptBraceCalc),
    const_cast<sal_Int32*>(mso_sptBraceDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptRightBraceTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptRightBracketGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptRightBraceHandle),
};

const SvxMSDffVertPair mso_sptIrregularSeal1Vert[] =
{
    { 10901, 5905 }, { 8458, 2399 }, { 7417, 6425 }, { 476, 2399 },
    { 4732, 7722 }, { 106, 8718 }, { 3828, 11880 }, { 243, 14689 },
    { 5772, 14041 }, { 4868, 17719 }, { 7819, 15730 }, { 8590, 21600 },
    { 10637, 15038 }, { 13349, 19840 }, { 14125, 14561 }, { 18248, 18195 },
    { 16938, 13044 }, { 21600, 13393 }, { 17710, 10579 }, { 21198, 8242 },
    { 16806, 7417 }, { 18482, 4560 }, { 14257, 5429 }, { 14623, 106 }, { 10901, 5905 }
};
const SvxMSDffTextRectangles mso_sptIrregularSeal1TextRect[] =
{
    { { 4680, 6570 }, { 16140, 13280 } }
};
const SvxMSDffVertPair mso_sptIrregularSeal1GluePoints[] =
{
    { 14623, 106 }, { 106, 8718 }, { 8590, 21600 }, { 21600, 13393 }
};
const mso_CustomShape msoIrregularSeal1 =
{
    std::span<const SvxMSDffVertPair>(mso_sptIrregularSeal1Vert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptIrregularSeal1TextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptIrregularSeal1GluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptIrregularSeal2Vert[] =
{
    { 11464, 4340 }, { 9722, 1887 }, { 8548, 6383 }, { 4503, 3626 },
    { 5373, 7816 }, { 1174, 8270 }, { 3934, 11592 }, { 0, 12875 },
    { 3329, 15372 }, { 1283, 17824 }, { 4804, 18239 }, { 4918, 21600 },
    { 7525, 18125 }, { 8698, 19712 }, { 9871, 17371 }, { 11614, 18844 },
    { 12178, 15937 }, { 14943, 17371 }, { 14640, 14348 }, { 18878, 15632 },
    { 16382, 12311 }, { 18270, 11292 }, { 16986, 9404 }, { 21600, 6646 },
    { 16382, 6533 }, { 18005, 3172 }, { 14524, 5778 }, { 14789, 0 },
    { 11464, 4340 }
};
const SvxMSDffTextRectangles mso_sptIrregularSeal2TextRect[] =
{
    { { 5400, 6570 }, { 14160, 15290 } }
};
const SvxMSDffVertPair mso_sptIrregularSeal2GluePoints[] =
{
    { 9722, 1887 }, { 0, 12875 }, { 11614, 18844 }, { 21600, 6646 }
};
const mso_CustomShape msoIrregularSeal2 =
{
    std::span<const SvxMSDffVertPair>(mso_sptIrregularSeal2Vert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptIrregularSeal2TextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptIrregularSeal2GluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptSeal4Vert[] =      // adjustment1 : 0 - 10800
{
    { 0, 10800 }, { 4 MSO_I, 4 MSO_I }, { 10800, 0 }, { 3 MSO_I, 4 MSO_I },
    { 21600, 10800 }, { 3 MSO_I, 3 MSO_I }, { 10800, 21600 }, { 4 MSO_I, 3 MSO_I },
    { 0, 10800 }
};
const SvxMSDffCalculationData mso_sptSeal4Calc[] =
{
    { 0x0000, { 7600, 0, 0 } },
    { 0x6001, { 0x400, DFF_Prop_adjustValue, 10800 } },
    { 0xa000, { 0x400, 0, 0x401 } },
    { 0x4000, { 10800, 0x402, 0 } },
    { 0x8000, { 10800, 0, 0x402 } }
};
const SvxMSDffTextRectangles mso_sptSeal4TextRect[] =
{
    { { 4 MSO_I, 4 MSO_I }, { 3 MSO_I, 3 MSO_I } }
};
const SvxMSDffHandle mso_sptSealHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 10800, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoSeal4 =
{
    std::span<const SvxMSDffVertPair>(mso_sptSeal4Vert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(mso_sptSeal4Calc),
    const_cast<sal_Int32*>(mso_sptDefault8100),
    std::span<const SvxMSDffTextRectangles>(mso_sptSeal4TextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptSealHandle),
};

const SvxMSDffVertPair mso_sptStarVert[] =
{
    { 10797, 0 }, { 8278, 8256 }, { 0, 8256 }, { 6722, 13405 },
    { 4198, 21600 }, { 10797, 16580 }, { 17401, 21600 }, { 14878, 13405 },
    { 21600, 8256 }, { 13321, 8256 }, { 10797, 0 }
};
const SvxMSDffTextRectangles mso_sptStarTextRect[] =
{
    { { 6722, 8256 }, { 14878, 15460 } }
};

const SvxMSDffVertPair mso_sptStarGluePoints[] =
{
    { 10800, 0 }, { 0, 8259 }, { 4200, 21600 }, { 17400, 21600 }, { 21600, 8259 }
};
const mso_CustomShape msoStar =
{
    std::span<const SvxMSDffVertPair>(mso_sptStarVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptStarTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>( mso_sptStarGluePoints ),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffCalculationData mso_sptSeal24Calc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },     // 0x00
    { 0x2081, { 0x400, 10800, 315 } },              // 0x01 ( textframe )
    { 0x2082, { 0x400, 10800, 315 } },              // 0x02
    { 0x2081, { 0x400, 10800, 135 } },              // 0x03
    { 0x2082, { 0x400, 10800, 135 } },              // 0x04
    { 0x0081, { 0,   10800, 0 } },
    { 0x0082, { 0,   10800, 0 } },
    { 0x2081, { 0x400, 10800, 7 } },
    { 0x2082, { 0x400, 10800, 7 } },
    { 0x0081, { 0,   10800, 15 } },
    { 0x0082, { 0,   10800, 15 } },
    { 0x2081, { 0x400, 10800, 22 } },
    { 0x2082, { 0x400, 10800, 22 } },
    { 0x0081, { 0,   10800, 30 } },
    { 0x0082, { 0,   10800, 30 } },
    { 0x2081, { 0x400, 10800, 37 } },
    { 0x2082, { 0x400, 10800, 37 } },
    { 0x0081, { 0,   10800, 45 } },
    { 0x0082, { 0,   10800, 45 } },
    { 0x2081, { 0x400, 10800, 52 } },
    { 0x2082, { 0x400, 10800, 52 } },
    { 0x0081, { 0,   10800, 60 } },
    { 0x0082, { 0,   10800, 60 } },
    { 0x2081, { 0x400, 10800, 67 } },
    { 0x2082, { 0x400, 10800, 67 } },
    { 0x0081, { 0,   10800, 75 } },
    { 0x0082, { 0,   10800, 75 } },
    { 0x2081, { 0x400, 10800, 82 } },
    { 0x2082, { 0x400, 10800, 82 } },
    { 0x0081, { 0,   10800, 90 } },
    { 0x0082, { 0,   10800, 90 } },
    { 0x2081, { 0x400, 10800, 97 } },
    { 0x2082, { 0x400, 10800, 97 } },
    { 0x0081, { 0,   10800, 105 } },
    { 0x0082, { 0,   10800, 105 } },
    { 0x2081, { 0x400, 10800, 112 } },
    { 0x2082, { 0x400, 10800, 112 } },
    { 0x0081, { 0,   10800, 120 } },
    { 0x0082, { 0,   10800, 120 } },
    { 0x2081, { 0x400, 10800, 127 } },
    { 0x2082, { 0x400, 10800, 127 } },
    { 0x0081, { 0,   10800, 135 } },
    { 0x0082, { 0,   10800, 135 } },
    { 0x2081, { 0x400, 10800, 142 } },
    { 0x2082, { 0x400, 10800, 142 } },
    { 0x0081, { 0,   10800, 150 } },
    { 0x0082, { 0,   10800, 150 } },
    { 0x2081, { 0x400, 10800, 157 } },
    { 0x2082, { 0x400, 10800, 157 } },
    { 0x0081, { 0,   10800, 165 } },
    { 0x0082, { 0,   10800, 165 } },
    { 0x2081, { 0x400, 10800, 172 } },
    { 0x2082, { 0x400, 10800, 172 } },
    { 0x0081, { 0,   10800, 180 } },
    { 0x0082, { 0,   10800, 180 } },
    { 0x2081, { 0x400, 10800, 187 } },
    { 0x2082, { 0x400, 10800, 187 } },
    { 0x0081, { 0,   10800, 195 } },
    { 0x0082, { 0,   10800, 195 } },
    { 0x2081, { 0x400, 10800, 202 } },
    { 0x2082, { 0x400, 10800, 202 } },
    { 0x0081, { 0,   10800, 210 } },
    { 0x0082, { 0,   10800, 210 } },
    { 0x2081, { 0x400, 10800, 217 } },
    { 0x2082, { 0x400, 10800, 217 } },
    { 0x0081, { 0,   10800, 225 } },
    { 0x0082, { 0,   10800, 225 } },
    { 0x2081, { 0x400, 10800, 232 } },
    { 0x2082, { 0x400, 10800, 232 } },
    { 0x0081, { 0,   10800, 240 } },
    { 0x0082, { 0,   10800, 240 } },
    { 0x2081, { 0x400, 10800, 247 } },
    { 0x2082, { 0x400, 10800, 247 } },
    { 0x0081, { 0,   10800, 255 } },
    { 0x0082, { 0,   10800, 255 } },
    { 0x2081, { 0x400, 10800, 262 } },
    { 0x2082, { 0x400, 10800, 262 } },
    { 0x0081, { 0,   10800, 270 } },
    { 0x0082, { 0,   10800, 270 } },
    { 0x2081, { 0x400, 10800, 277 } },
    { 0x2082, { 0x400, 10800, 277 } },
    { 0x0081, { 0,   10800, 285 } },
    { 0x0082, { 0,   10800, 285 } },
    { 0x2081, { 0x400, 10800, 292 } },
    { 0x2082, { 0x400, 10800, 292 } },
    { 0x0081, { 0,   10800, 300 } },
    { 0x0082, { 0,   10800, 300 } },
    { 0x2081, { 0x400, 10800, 307 } },
    { 0x2082, { 0x400, 10800, 307 } },
    { 0x0081, { 0,   10800, 315 } },
    { 0x0082, { 0,   10800, 315 } },
    { 0x2081, { 0x400, 10800, 322 } },
    { 0x2082, { 0x400, 10800, 322 } },
    { 0x0081, { 0,   10800, 330 } },
    { 0x0082, { 0,   10800, 330 } },
    { 0x2081, { 0x400, 10800, 337 } },
    { 0x2082, { 0x400, 10800, 337 } },
    { 0x0081, { 0,   10800, 345 } },
    { 0x0082, { 0,   10800, 345 } },
    { 0x2081, { 0x400, 10800, 352 } },
    { 0x2082, { 0x400, 10800, 352 } }
};
const SvxMSDffVertPair mso_sptSeal8Vert[] =  // adj value 0 -> 10800
{
    { 5 MSO_I, 6 MSO_I }, { 11 MSO_I, 12 MSO_I }, { 17 MSO_I, 18 MSO_I }, { 23 MSO_I, 24 MSO_I },
    { 29 MSO_I, 30 MSO_I }, { 35 MSO_I, 36 MSO_I }, { 41 MSO_I, 42 MSO_I }, { 47 MSO_I, 48 MSO_I },
    { 53 MSO_I, 54 MSO_I }, { 59 MSO_I, 60 MSO_I }, { 65 MSO_I, 66 MSO_I }, { 71 MSO_I, 72 MSO_I },
    { 77 MSO_I, 78 MSO_I }, { 83 MSO_I, 84 MSO_I }, { 89 MSO_I, 90 MSO_I }, { 95 MSO_I, 96 MSO_I },
    { 5 MSO_I, 6 MSO_I }
};
const SvxMSDffTextRectangles mso_sptSealTextRect[] =
{
    { { 1 MSO_I, 2 MSO_I }, { 3 MSO_I, 4 MSO_I } }
};
const mso_CustomShape msoSeal8 =
{
    std::span<const SvxMSDffVertPair>(mso_sptSeal8Vert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(mso_sptSeal24Calc),
    const_cast<sal_Int32*>(mso_sptDefault2500),
    std::span<const SvxMSDffTextRectangles>(mso_sptSealTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptSealHandle),
};
const SvxMSDffVertPair mso_sptSeal16Vert[] = // adj value 0 -> 10800
{
    { 0x05 MSO_I, 0x06 MSO_I }, { 0x07 MSO_I, 0x08 MSO_I }, { 0x09 MSO_I, 0x0a MSO_I }, { 0x0b MSO_I, 0x0c MSO_I },
    { 0x0d MSO_I, 0x0e MSO_I }, { 0x0f MSO_I, 0x10 MSO_I }, { 0x11 MSO_I, 0x12 MSO_I }, { 0x13 MSO_I, 0x14 MSO_I },
    { 0x15 MSO_I, 0x16 MSO_I }, { 0x17 MSO_I, 0x18 MSO_I }, { 0x19 MSO_I, 0x1a MSO_I }, { 0x1b MSO_I, 0x1c MSO_I },
    { 0x1d MSO_I, 0x1e MSO_I }, { 0x1f MSO_I, 0x20 MSO_I }, { 0x21 MSO_I, 0x22 MSO_I }, { 0x23 MSO_I, 0x24 MSO_I },
    { 0x25 MSO_I, 0x26 MSO_I }, { 0x27 MSO_I, 0x28 MSO_I }, { 0x29 MSO_I, 0x2a MSO_I }, { 0x2b MSO_I, 0x2c MSO_I },
    { 0x2d MSO_I, 0x2e MSO_I }, { 0x2f MSO_I, 0x30 MSO_I }, { 0x31 MSO_I, 0x32 MSO_I }, { 0x33 MSO_I, 0x34 MSO_I },
    { 0x35 MSO_I, 0x36 MSO_I }, { 0x37 MSO_I, 0x38 MSO_I }, { 0x39 MSO_I, 0x3a MSO_I }, { 0x3b MSO_I, 0x3c MSO_I },
    { 0x3d MSO_I, 0x3e MSO_I }, { 0x3f MSO_I, 0x40 MSO_I }, { 0x41 MSO_I, 0x42 MSO_I }, { 0x43 MSO_I, 0x44 MSO_I },
    { 0x05 MSO_I, 0x06 MSO_I }
};
const SvxMSDffCalculationData mso_sptSeal16Calc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },     // 0x00
    { 0x2081, { 0x400, 10800, 315 } },              // 0x01 ( textframe )
    { 0x2082, { 0x400, 10800, 315 } },              // 0x02
    { 0x2081, { 0x400, 10800, 135 } },              // 0x03
    { 0x2082, { 0x400, 10800, 135 } },              // 0x04
    { 0x0081, { 0,   10800, 0 } },
    { 0x0082, { 0,   10800, 0 } },
    { 0x2081, { 0x400, 10800, 11 } },
    { 0x2082, { 0x400, 10800, 11 } },
    { 0x0081, { 0,   10800, 22 } },
    { 0x0082, { 0,   10800, 22 } },
    { 0x2081, { 0x400, 10800, 33 } },
    { 0x2082, { 0x400, 10800, 33 } },
    { 0x0081, { 0,   10800, 45 } },
    { 0x0082, { 0,   10800, 45 } },
    { 0x2081, { 0x400, 10800, 56 } },
    { 0x2082, { 0x400, 10800, 56 } },
    { 0x0081, { 0,   10800, 67 } },
    { 0x0082, { 0,   10800, 67 } },
    { 0x2081, { 0x400, 10800, 78 } },
    { 0x2082, { 0x400, 10800, 78 } },
    { 0x0081, { 0,   10800, 90 } },
    { 0x0082, { 0,   10800, 90 } },
    { 0x2081, { 0x400, 10800, 101 } },
    { 0x2082, { 0x400, 10800, 101 } },
    { 0x0081, { 0,   10800, 112 } },
    { 0x0082, { 0,   10800, 112 } },
    { 0x2081, { 0x400, 10800, 123 } },
    { 0x2082, { 0x400, 10800, 123 } },
    { 0x0081, { 0,   10800, 135 } },
    { 0x0082, { 0,   10800, 135 } },
    { 0x2081, { 0x400, 10800, 146 } },
    { 0x2082, { 0x400, 10800, 146 } },
    { 0x0081, { 0,   10800, 157 } },
    { 0x0082, { 0,   10800, 157 } },
    { 0x2081, { 0x400, 10800, 168 } },
    { 0x2082, { 0x400, 10800, 168 } },
    { 0x0081, { 0,   10800, 180 } },
    { 0x0082, { 0,   10800, 180 } },
    { 0x2081, { 0x400, 10800, 191 } },
    { 0x2082, { 0x400, 10800, 191 } },
    { 0x0081, { 0,   10800, 202 } },
    { 0x0082, { 0,   10800, 202 } },
    { 0x2081, { 0x400, 10800, 213 } },
    { 0x2082, { 0x400, 10800, 213 } },
    { 0x0081, { 0,   10800, 225 } },
    { 0x0082, { 0,   10800, 225 } },
    { 0x2081, { 0x400, 10800, 236 } },
    { 0x2082, { 0x400, 10800, 236 } },
    { 0x0081, { 0,   10800, 247 } },
    { 0x0082, { 0,   10800, 247 } },
    { 0x2081, { 0x400, 10800, 258 } },
    { 0x2082, { 0x400, 10800, 258 } },
    { 0x0081, { 0,   10800, 270 } },
    { 0x0082, { 0,   10800, 270 } },
    { 0x2081, { 0x400, 10800, 281 } },
    { 0x2082, { 0x400, 10800, 281 } },
    { 0x0081, { 0,   10800, 292 } },
    { 0x0082, { 0,   10800, 292 } },
    { 0x2081, { 0x400, 10800, 303 } },
    { 0x2082, { 0x400, 10800, 303 } },
    { 0x0081, { 0,   10800, 315 } },
    { 0x0082, { 0,   10800, 315 } },
    { 0x2081, { 0x400, 10800, 326 } },
    { 0x2082, { 0x400, 10800, 326 } },
    { 0x0081, { 0,   10800, 337 } },
    { 0x0082, { 0,   10800, 337 } },
    { 0x2081, { 0x400, 10800, 348 } },
    { 0x2082, { 0x400, 10800, 348 } }
};
const mso_CustomShape msoSeal16 =
{
    std::span<const SvxMSDffVertPair>(mso_sptSeal16Vert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(mso_sptSeal16Calc),
    const_cast<sal_Int32*>(mso_sptDefault2500),
    std::span<const SvxMSDffTextRectangles>(mso_sptSealTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptSealHandle),
};
const SvxMSDffVertPair mso_sptSeal24Vert[] =
{
    { 0x05 MSO_I, 0x06 MSO_I }, { 0x07 MSO_I, 0x08 MSO_I }, { 0x09 MSO_I, 0x0a MSO_I }, { 0x0b MSO_I, 0x0c MSO_I },
    { 0x0d MSO_I, 0x0e MSO_I }, { 0x0f MSO_I, 0x10 MSO_I }, { 0x11 MSO_I, 0x12 MSO_I }, { 0x13 MSO_I, 0x14 MSO_I },
    { 0x15 MSO_I, 0x16 MSO_I }, { 0x17 MSO_I, 0x18 MSO_I }, { 0x19 MSO_I, 0x1a MSO_I }, { 0x1b MSO_I, 0x1c MSO_I },
    { 0x1d MSO_I, 0x1e MSO_I }, { 0x1f MSO_I, 0x20 MSO_I }, { 0x21 MSO_I, 0x22 MSO_I }, { 0x23 MSO_I, 0x24 MSO_I },
    { 0x25 MSO_I, 0x26 MSO_I }, { 0x27 MSO_I, 0x28 MSO_I }, { 0x29 MSO_I, 0x2a MSO_I }, { 0x2b MSO_I, 0x2c MSO_I },
    { 0x2d MSO_I, 0x2e MSO_I }, { 0x2f MSO_I, 0x30 MSO_I }, { 0x31 MSO_I, 0x32 MSO_I }, { 0x33 MSO_I, 0x34 MSO_I },
    { 0x35 MSO_I, 0x36 MSO_I }, { 0x37 MSO_I, 0x38 MSO_I }, { 0x39 MSO_I, 0x3a MSO_I }, { 0x3b MSO_I, 0x3c MSO_I },
    { 0x3d MSO_I, 0x3e MSO_I }, { 0x3f MSO_I, 0x40 MSO_I }, { 0x41 MSO_I, 0x42 MSO_I }, { 0x43 MSO_I, 0x44 MSO_I },
    { 0x45 MSO_I, 0x46 MSO_I }, { 0x47 MSO_I, 0x48 MSO_I }, { 0x49 MSO_I, 0x4a MSO_I }, { 0x4b MSO_I, 0x4c MSO_I },
    { 0x4d MSO_I, 0x4e MSO_I }, { 0x4f MSO_I, 0x50 MSO_I }, { 0x51 MSO_I, 0x52 MSO_I }, { 0x53 MSO_I, 0x54 MSO_I },
    { 0x55 MSO_I, 0x56 MSO_I }, { 0x57 MSO_I, 0x58 MSO_I }, { 0x59 MSO_I, 0x5a MSO_I }, { 0x5b MSO_I, 0x5c MSO_I },
    { 0x5d MSO_I, 0x5e MSO_I }, { 0x5f MSO_I, 0x60 MSO_I }, { 0x61 MSO_I, 0x62 MSO_I }, { 0x63 MSO_I, 0x64 MSO_I },
    { 0x05 MSO_I, 0x06 MSO_I }
};
const mso_CustomShape msoSeal24 =
{
    std::span<const SvxMSDffVertPair>(mso_sptSeal24Vert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(mso_sptSeal24Calc),
    const_cast<sal_Int32*>(mso_sptDefault2500),
    std::span<const SvxMSDffTextRectangles>(mso_sptSealTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptSealHandle),
};
const SvxMSDffCalculationData mso_sptSeal32Calc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },     // 0x00
    { 0x2081, { 0x400, 10800, 315 } },              // 0x01 ( textframe )
    { 0x2082, { 0x400, 10800, 315 } },              // 0x02
    { 0x2081, { 0x400, 10800, 135 } },              // 0x03
    { 0x2082, { 0x400, 10800, 135 } },              // 0x04
    { 0x0081, { 0,   10800, 0 } },
    { 0x0082, { 0,   10800, 0 } },
    { 0x2081, { 0x400, 10800, 5 } },
    { 0x2082, { 0x400, 10800, 5 } },
    { 0x0081, { 0,   10800, 11 } },
    { 0x0082, { 0,   10800, 11 } },
    { 0x2081, { 0x400, 10800, 16 } },
    { 0x2082, { 0x400, 10800, 16 } },
    { 0x0081, { 0,   10800, 22 } },
    { 0x0082, { 0,   10800, 22 } },
    { 0x2081, { 0x400, 10800, 28 } },
    { 0x2082, { 0x400, 10800, 28 } },
    { 0x0081, { 0,   10800, 33 } },
    { 0x0082, { 0,   10800, 33 } },
    { 0x2081, { 0x400, 10800, 39 } },
    { 0x2082, { 0x400, 10800, 39 } },
    { 0x0081, { 0,   10800, 45 } },
    { 0x0082, { 0,   10800, 45 } },
    { 0x2081, { 0x400, 10800, 50 } },
    { 0x2082, { 0x400, 10800, 50 } },
    { 0x0081, { 0,   10800, 56 } },
    { 0x0082, { 0,   10800, 56 } },
    { 0x2081, { 0x400, 10800, 61 } },
    { 0x2082, { 0x400, 10800, 61 } },
    { 0x0081, { 0,   10800, 67 } },
    { 0x0082, { 0,   10800, 67 } },
    { 0x2081, { 0x400, 10800, 73 } },
    { 0x2082, { 0x400, 10800, 73 } },
    { 0x0081, { 0,   10800, 78 } },
    { 0x0082, { 0,   10800, 78 } },
    { 0x2081, { 0x400, 10800, 84 } },
    { 0x2082, { 0x400, 10800, 84 } },
    { 0x0081, { 0,   10800, 90 } },
    { 0x0082, { 0,   10800, 90 } },
    { 0x2081, { 0x400, 10800, 95 } },
    { 0x2082, { 0x400, 10800, 95 } },
    { 0x0081, { 0,   10800, 101 } },
    { 0x0082, { 0,   10800, 101 } },
    { 0x2081, { 0x400, 10800, 106 } },
    { 0x2082, { 0x400, 10800, 106 } },
    { 0x0081, { 0,   10800, 112 } },
    { 0x0082, { 0,   10800, 112 } },
    { 0x2081, { 0x400, 10800, 118 } },
    { 0x2082, { 0x400, 10800, 118 } },
    { 0x0081, { 0,   10800, 123 } },
    { 0x0082, { 0,   10800, 123 } },
    { 0x2081, { 0x400, 10800, 129 } },
    { 0x2082, { 0x400, 10800, 129 } },
    { 0x0081, { 0,   10800, 135 } },
    { 0x0082, { 0,   10800, 135 } },
    { 0x2081, { 0x400, 10800, 140 } },
    { 0x2082, { 0x400, 10800, 140 } },
    { 0x0081, { 0,   10800, 146 } },
    { 0x0082, { 0,   10800, 146 } },
    { 0x2081, { 0x400, 10800, 151 } },
    { 0x2082, { 0x400, 10800, 151 } },
    { 0x0081, { 0,   10800, 157 } },
    { 0x0082, { 0,   10800, 157 } },
    { 0x2081, { 0x400, 10800, 163 } },
    { 0x2082, { 0x400, 10800, 163 } },
    { 0x0081, { 0,   10800, 168 } },
    { 0x0082, { 0,   10800, 168 } },
    { 0x2081, { 0x400, 10800, 174 } },
    { 0x2082, { 0x400, 10800, 174 } },
    { 0x0081, { 0,   10800, 180 } },
    { 0x0082, { 0,   10800, 180 } },
    { 0x2081, { 0x400, 10800, 185 } },
    { 0x2082, { 0x400, 10800, 185 } },
    { 0x0081, { 0,   10800, 191 } },
    { 0x0082, { 0,   10800, 191 } },
    { 0x2081, { 0x400, 10800, 196 } },
    { 0x2082, { 0x400, 10800, 196 } },
    { 0x0081, { 0,   10800, 202 } },
    { 0x0082, { 0,   10800, 202 } },
    { 0x2081, { 0x400, 10800, 208 } },
    { 0x2082, { 0x400, 10800, 208 } },
    { 0x0081, { 0,   10800, 213 } },
    { 0x0082, { 0,   10800, 213 } },
    { 0x2081, { 0x400, 10800, 219 } },
    { 0x2082, { 0x400, 10800, 219 } },
    { 0x0081, { 0,   10800, 225 } },
    { 0x0082, { 0,   10800, 225 } },
    { 0x2081, { 0x400, 10800, 230 } },
    { 0x2082, { 0x400, 10800, 230 } },
    { 0x0081, { 0,   10800, 236 } },
    { 0x0082, { 0,   10800, 236 } },
    { 0x2081, { 0x400, 10800, 241 } },
    { 0x2082, { 0x400, 10800, 241 } },
    { 0x0081, { 0,   10800, 247 } },
    { 0x0082, { 0,   10800, 247 } },
    { 0x2081, { 0x400, 10800, 253 } },
    { 0x2082, { 0x400, 10800, 253 } },
    { 0x0081, { 0,   10800, 258 } },
    { 0x0082, { 0,   10800, 258 } },
    { 0x2081, { 0x400, 10800, 264 } },
    { 0x2082, { 0x400, 10800, 264 } },
    { 0x0081, { 0,   10800, 270 } },
    { 0x0082, { 0,   10800, 270 } },
    { 0x2081, { 0x400, 10800, 275 } },
    { 0x2082, { 0x400, 10800, 275 } },
    { 0x0081, { 0,   10800, 281 } },
    { 0x0082, { 0,   10800, 281 } },
    { 0x2081, { 0x400, 10800, 286 } },
    { 0x2082, { 0x400, 10800, 286 } },
    { 0x0081, { 0,   10800, 292 } },
    { 0x0082, { 0,   10800, 292 } },
    { 0x2081, { 0x400, 10800, 298 } },
    { 0x2082, { 0x400, 10800, 298 } },
    { 0x0081, { 0,   10800, 303 } },
    { 0x0082, { 0,   10800, 303 } },
    { 0x2081, { 0x400, 10800, 309 } },
    { 0x2082, { 0x400, 10800, 309 } },
    { 0x0081, { 0,   10800, 315 } },
    { 0x0082, { 0,   10800, 315 } },
    { 0x2081, { 0x400, 10800, 320 } },
    { 0x2082, { 0x400, 10800, 320 } },
    { 0x0081, { 0,   10800, 326 } },
    { 0x0082, { 0,   10800, 326 } },
    { 0x2081, { 0x400, 10800, 331 } },
    { 0x2082, { 0x400, 10800, 331 } },
    { 0x0081, { 0,   10800, 337 } },
    { 0x0082, { 0,   10800, 337 } },
    { 0x2081, { 0x400, 10800, 343 } },
    { 0x2082, { 0x400, 10800, 343 } },
    { 0x0081, { 0,   10800, 348 } },
    { 0x0082, { 0,   10800, 348 } },
    { 0x2081, { 0x400, 10800, 354 } },
    { 0x2082, { 0x400, 10800, 354 } }
};
const SvxMSDffVertPair mso_sptSeal32Vert[] =
{
    { 0x05 MSO_I, 0x06 MSO_I }, { 0x07 MSO_I, 0x08 MSO_I }, { 0x09 MSO_I, 0x0a MSO_I }, { 0x0b MSO_I, 0x0c MSO_I },
    { 0x0d MSO_I, 0x0e MSO_I }, { 0x0f MSO_I, 0x10 MSO_I }, { 0x11 MSO_I, 0x12 MSO_I }, { 0x13 MSO_I, 0x14 MSO_I },
    { 0x15 MSO_I, 0x16 MSO_I }, { 0x17 MSO_I, 0x18 MSO_I }, { 0x19 MSO_I, 0x1a MSO_I }, { 0x1b MSO_I, 0x1c MSO_I },
    { 0x1d MSO_I, 0x1e MSO_I }, { 0x1f MSO_I, 0x20 MSO_I }, { 0x21 MSO_I, 0x22 MSO_I }, { 0x23 MSO_I, 0x24 MSO_I },
    { 0x25 MSO_I, 0x26 MSO_I }, { 0x27 MSO_I, 0x28 MSO_I }, { 0x29 MSO_I, 0x2a MSO_I }, { 0x2b MSO_I, 0x2c MSO_I },
    { 0x2d MSO_I, 0x2e MSO_I }, { 0x2f MSO_I, 0x30 MSO_I }, { 0x31 MSO_I, 0x32 MSO_I }, { 0x33 MSO_I, 0x34 MSO_I },
    { 0x35 MSO_I, 0x36 MSO_I }, { 0x37 MSO_I, 0x38 MSO_I }, { 0x39 MSO_I, 0x3a MSO_I }, { 0x3b MSO_I, 0x3c MSO_I },
    { 0x3d MSO_I, 0x3e MSO_I }, { 0x3f MSO_I, 0x40 MSO_I }, { 0x41 MSO_I, 0x42 MSO_I }, { 0x43 MSO_I, 0x44 MSO_I },
    { 0x45 MSO_I, 0x46 MSO_I }, { 0x47 MSO_I, 0x48 MSO_I }, { 0x49 MSO_I, 0x4a MSO_I }, { 0x4b MSO_I, 0x4c MSO_I },
    { 0x4d MSO_I, 0x4e MSO_I }, { 0x4f MSO_I, 0x50 MSO_I }, { 0x51 MSO_I, 0x52 MSO_I }, { 0x53 MSO_I, 0x54 MSO_I },
    { 0x55 MSO_I, 0x56 MSO_I }, { 0x57 MSO_I, 0x58 MSO_I }, { 0x59 MSO_I, 0x5a MSO_I }, { 0x5b MSO_I, 0x5c MSO_I },
    { 0x5d MSO_I, 0x5e MSO_I }, { 0x5f MSO_I, 0x60 MSO_I }, { 0x61 MSO_I, 0x62 MSO_I }, { 0x63 MSO_I, 0x64 MSO_I },
    { 0x65 MSO_I, 0x66 MSO_I }, { 0x67 MSO_I, 0x68 MSO_I }, { 0x69 MSO_I, 0x6a MSO_I }, { 0x6b MSO_I, 0x6c MSO_I },
    { 0x6d MSO_I, 0x6e MSO_I }, { 0x6f MSO_I, 0x70 MSO_I }, { 0x71 MSO_I, 0x72 MSO_I }, { 0x73 MSO_I, 0x74 MSO_I },
    { 0x75 MSO_I, 0x76 MSO_I }, { 0x77 MSO_I, 0x78 MSO_I }, { 0x79 MSO_I, 0x7a MSO_I }, { 0x7b MSO_I, 0x7c MSO_I },
    { 0x7d MSO_I, 0x7e MSO_I }, { 0x7f MSO_I, 0x80 MSO_I }, { 0x81 MSO_I, 0x82 MSO_I }, { 0x83 MSO_I, 0x84 MSO_I },
    { 0x05 MSO_I, 0x06 MSO_I }
};
const mso_CustomShape msoSeal32 =
{
    std::span<const SvxMSDffVertPair>(mso_sptSeal32Vert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(mso_sptSeal32Calc),
    const_cast<sal_Int32*>(mso_sptDefault2500),
    std::span<const SvxMSDffTextRectangles>(mso_sptSealTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptSealHandle),
};

const SvxMSDffVertPair mso_sptRibbon2Vert[] =    // adjustment1 : x 2700  - 8100     def 5400
{                                                       // adjustment2 : y 14400 - 21600    def 18900
    { 12 MSO_I, 1 MSO_I }, { 12 MSO_I, 13 MSO_I },                                      // pp
    { 12 MSO_I, 14 MSO_I }, { 15 MSO_I, 21600 }, { 16 MSO_I, 21600 },                   // ccp
    { 0, 21600 }, { 2750, 7 MSO_I }, { 0, 2 MSO_I }, { 0 MSO_I, 2 MSO_I },              // pppp
    { 0 MSO_I, 4 MSO_I },                                                               // p
    { 0 MSO_I, 5 MSO_I }, { 10 MSO_I, 0 }, { 11 MSO_I, 0 },                             // ccp
    { 17 MSO_I, 0 },                                                                    // p
    { 18 MSO_I, 0 }, { 19 MSO_I, 5 MSO_I }, { 19 MSO_I, 4 MSO_I },                      // ccp
    { 19 MSO_I, 2 MSO_I }, { 21600, 2 MSO_I }, { 18850, 7 MSO_I }, { 21600, 21600 },    // pppp
    { 20 MSO_I, 21600 },                                                                // p
    { 21 MSO_I, 21600 }, { 22 MSO_I, 14 MSO_I }, { 22 MSO_I, 13 MSO_I },                // ccp
    { 22 MSO_I, 1 MSO_I }, { 12 MSO_I, 1 MSO_I }, { 12 MSO_I, 13 MSO_I },               // ppp
    { 12 MSO_I, 23 MSO_I }, { 15 MSO_I, 24 MSO_I }, { 16 MSO_I, 24 MSO_I },             // ccp
    { 11 MSO_I, 24 MSO_I },                                                             // p
    { 10 MSO_I, 24 MSO_I }, { 0 MSO_I, 26 MSO_I }, { 0 MSO_I, 25 MSO_I },               // ccp
    { 0 MSO_I, 27 MSO_I }, { 10 MSO_I, 1 MSO_I }, { 11 MSO_I, 1 MSO_I },                // ccp

    { 22 MSO_I, 1 MSO_I }, { 22 MSO_I, 13 MSO_I },                                      // pp
    { 22 MSO_I, 23 MSO_I }, { 21 MSO_I, 24 MSO_I }, { 20 MSO_I, 24 MSO_I },             // ccp
    { 17 MSO_I, 24 MSO_I },                                                             // p
    { 18 MSO_I, 24 MSO_I }, { 19 MSO_I, 26 MSO_I }, { 19 MSO_I, 25 MSO_I },             // ccp
    { 19 MSO_I, 27 MSO_I }, { 18 MSO_I, 1 MSO_I }, { 17 MSO_I, 1 MSO_I },               // ccp

    { 0 MSO_I, 25 MSO_I }, { 0 MSO_I, 2 MSO_I },                                        // pp

    { 19 MSO_I, 25 MSO_I }, { 19 MSO_I, 2 MSO_I }                                       // pp
};
const sal_uInt16 mso_sptRibbon2Segm[] =
{
    0x4000, 0x0001, 0x2001, 0x0005, 0x2001, 0x0001, 0x2001, 0x0005, 0x2001, 0x0001, 0x6001, 0x8000,
    0x4000, 0x0001, 0x2001, 0x0001, 0x2002, 0x6001, 0x8000,
    0x4000, 0x0001, 0x2001, 0x0001, 0x2002, 0x6001, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffCalculationData mso_sptRibbon2Calc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },             // 00
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },            // 01
    { 0x8000, { 21600, 0, 0x401 } },                        // 02
    { 0x2001, { 0x402, 1, 2 } },                            // 03
    { 0x2001, { 0x403, 1, 2 } },                            // 04
    { 0x2001, { 0x404, 1, 2 } },                            // 05
    { 0x2001, { 0x401, 1, 2 } },                            // 06
    { 0x8000, { 21600, 0, 0x406 } },                        // 07
    { 0x0000, { 420, 0, 0 } },                              // 08
    { 0x2001, { 0x408, 2, 1 } },                            // 09
    { 0x6000, { 0x400, 0x408, 0 } },                        // 10
    { 0x6000, { 0x400, 0x409, 0 } },                        // 11
    { 0x2000, { 0x400, 2700, 0 } },                         // 12
    { 0x8000, { 21600, 0, 0x404 } },                        // 13
    { 0x8000, { 21600, 0, 0x405 } },                        // 14
    { 0xa000, { 0x40c, 0, 0x408 } },                        // 15
    { 0xa000, { 0x40c, 0, 0x409 } },                        // 16

    { 0x8000, { 21600, 0, 0x40b } },                        // 17
    { 0x8000, { 21600, 0, 0x40a } },                        // 18
    { 0x8000, { 21600, 0, 0x400 } },                        // 19
    { 0x8000, { 21600, 0, 0x410 } },                        // 20
    { 0x8000, { 21600, 0, 0x40f } },                        // 21
    { 0x8000, { 21600, 0, 0x40c } },                        // 22

    { 0xa000, { 0x40d, 0, 0x405 } },                        // 23
    { 0x6000, { 0x401, 0x403, 0 } },                        // 24
    { 0x6000, { 0x401, 0x404, 0 } },                        // 25
    { 0x6000, { 0x419, 0x405, 0 } },                        // 26
    { 0xa000, { 0x419, 0, 0x405 } }                         // 27
};
const sal_Int32 mso_sptRibbon2Default[] =
{
    2, 5400, 18900
};
const SvxMSDffTextRectangles mso_sptRibbon2TextRect[] =
{
    { { 0 MSO_I, 0 }, { 19 MSO_I, 1 MSO_I } }
};
const SvxMSDffHandle mso_sptRibbon2Handle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 0, 10800, 10800, 2700, 8100, MIN_INT32, 0x7fffffff },
    {   SvxMSDffHandleFlags::RANGE,
        10800, 0x101, 10800, 10800, MIN_INT32, 0x7fffffff, 14400, 21600 }
};
const mso_CustomShape msoRibbon2 =
{
    std::span<const SvxMSDffVertPair>(mso_sptRibbon2Vert),
    const_cast<sal_uInt16*>(mso_sptRibbon2Segm), sizeof( mso_sptRibbon2Segm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptRibbon2Calc),
    const_cast<sal_Int32*>(mso_sptRibbon2Default),
    std::span<const SvxMSDffTextRectangles>(mso_sptRibbon2TextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptRibbon2Handle),
};

const SvxMSDffVertPair mso_sptRibbonVert[] =
{
    { 0, 0 }, { 3 MSO_I, 0 },
    { 4 MSO_I, 11 MSO_I }, { 4 MSO_I, 10 MSO_I }, { 5 MSO_I, 10 MSO_I }, { 5 MSO_I, 11 MSO_I },
    { 6 MSO_I, 0 }, { 21600, 0 }, { 18 MSO_I, 14 MSO_I }, { 21600, 15 MSO_I }, { 9 MSO_I, 15 MSO_I }, { 9 MSO_I, 16 MSO_I }, { 8 MSO_I, 21600 }, { 1 MSO_I, 21600 },
    { 0 MSO_I, 16 MSO_I }, { 0 MSO_I, 15 MSO_I }, { 0, 15 MSO_I }, { 2700, 14 MSO_I },

    { 4 MSO_I, 11 MSO_I },
    { 3 MSO_I, 12 MSO_I }, { 1 MSO_I, 12 MSO_I },
    { 0 MSO_I, 13 MSO_I }, { 1 MSO_I, 10 MSO_I }, { 4 MSO_I, 10 MSO_I },
    { 5 MSO_I, 11 MSO_I },
    { 6 MSO_I, 12 MSO_I }, { 8 MSO_I, 12 MSO_I },
    { 9 MSO_I, 13 MSO_I }, { 8 MSO_I, 10 MSO_I }, { 5 MSO_I, 10 MSO_I },
    { 0 MSO_I, 13 MSO_I },
    { 0 MSO_I, 15 MSO_I },
    { 9 MSO_I, 13 MSO_I },
    { 9 MSO_I, 15 MSO_I }
};
const sal_uInt16 mso_sptRibbonSegm[] =
{
    0x4000, 0x0001, 0xa701, 0x0003, 0xa801, 0x0005, 0xa801, 0x0001, 0xa701, 0x0003, 0x6000, 0x8000,
    0x4000, 0xa801, 0x0001, 0xa702, 0x0001, 0x6000, 0x8000,
    0x4000, 0xa801, 0x0001, 0xa702, 0x0001, 0x6000, 0x8000,
    0x4000, 0x0001, 0x6000, 0x8000,
    0x4000, 0x0001, 0x6000, 0x8000
};
const SvxMSDffCalculationData mso_sptRibbonCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },             // 00
    { 0x2000, { 0x400, 675, 0 } },                          // 01
    { 0x2000, { 0x401, 675, 0 } },                          // 02
    { 0x2000, { 0x402, 675, 0 } },                          // 03
    { 0x2000, { 0x403, 675, 0 } },                          // 04
    { 0x8000, { 21600, 0, 0x404 } },                        // 05
    { 0x8000, { 21600, 0, 0x403 } },                        // 06
    { 0x8000, { 21600, 0, 0x402 } },                        // 07
    { 0x8000, { 21600, 0, 0x401 } },                        // 08
    { 0x8000, { 21600, 0, 0x400 } },                        // 09
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },            // 10
    { 0x2001, { 0x40a, 1, 4 } },                            // 11
    { 0x2001, { 0x40b, 2, 1 } },                            // 12
    { 0x2001, { 0x40b, 3, 1 } },                            // 13
    { 0x8000, { 10800, 0, 0x40c } },                        // 14
    { 0x8000, { 21600, 0, 0x40a } },                        // 15
    { 0x8000, { 21600, 0, 0x40b } },                        // 16
    { 0x0001, { 21600, 1, 2 } },                            // 17
    { 0x0000, { 21600, 0, 2700 } },                         // 18
    { 0x2000, { 0x411, 0, 2700 } }                          // 19
};
const sal_Int32 mso_sptRibbonDefault[] =
{
    2, 5400, 2700
};
const SvxMSDffTextRectangles mso_sptRibbonTextRect[] =
{
    { { 0 MSO_I, 10 MSO_I }, { 9 MSO_I, 21600 } }
};
const SvxMSDffVertPair mso_sptRibbonGluePoints[] =
{
    { 17 MSO_I, 10 MSO_I }, { 2700, 14 MSO_I }, { 17 MSO_I, 21600 }, { 18 MSO_I, 14 MSO_I }
};
const SvxMSDffHandle mso_sptRibbonHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 0, 10800, 10800, 2700, 8100, MIN_INT32, 0x7fffffff },
    {   SvxMSDffHandleFlags::RANGE,
        10800, 0x101, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 7200 }
};
const mso_CustomShape msoRibbon =
{
    std::span<const SvxMSDffVertPair>(mso_sptRibbonVert),
    const_cast<sal_uInt16*>(mso_sptRibbonSegm), sizeof( mso_sptRibbonSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptRibbonCalc),
    const_cast<sal_Int32*>(mso_sptRibbonDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptRibbonTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptRibbonGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptRibbonHandle),
};
//msosptEllipseRibbon
//svg path = ar@9@38@8@37,0@27@0@26@9@13@8@4@0@25@22@25@9@38@8@37@22@26@3@27l@7@40@3,wa@9@35@8@10@3,0@21@33@9@36@8@1@21@31@20@31@9@35@8@10@20@33,,l@5@40xewr@9@36@8@1@20@31@0@32nfl@20@33ear@9@36@8@1@21@31@22@32nfl@21@33em@0@26nfl@0@32em@22@26nfl@22@32e
//odp path = A ?f9 ?f38 ?f8 ?f37 0 ?f27 ?f0 ?f26 ?f9 ?f13 ?f8 ?f4 ?f0 ?f25 ?f22 ?f25 ?f9 ?f38 ?f8 ?f37 ?f22 ?f26 ?f3 ?f27 L ?f7 ?f40 ?f3 0 W ?f9 ?f35 ?f8 ?f10 ?f3 0 ?f21 ?f33 ?f9 ?f36 ?f8 ?f1 ?f21 ?f31 ?f20 ?f31 ?f9 ?f35 ?f8 ?f10 ?f20 ?f33 0 0 L ?f5 ?f40 Z N W ?f9 ?f36 ?f8 ?f1 ?f20 ?f31 ?f0 ?f32 F L ?f20 ?f33 N A ?f9 ?f36 ?f8 ?f1 ?f21 ?f31 ?f22 ?f32 F L ?f21 ?f33 N M ?f0 ?f26 F L ?f0 ?f32 N M ?f22 ?f26 F L ?f22 ?f32 N
const SvxMSDffVertPair mso_sptEllipseRibbonVert[] =
{
    { 9 MSO_I , 38 MSO_I }, { 8 MSO_I , 37 MSO_I }, { 0 , 27 MSO_I }, { 0 MSO_I , 26 MSO_I },        // ar 1
    { 9 MSO_I , 13 MSO_I }, { 8 MSO_I , 4 MSO_I }, { 0 MSO_I , 25 MSO_I }, { 22 MSO_I , 25 MSO_I },  // ar 2
    { 9 MSO_I , 38 MSO_I }, { 8 MSO_I , 37 MSO_I }, { 22 MSO_I , 26 MSO_I }, { 3 MSO_I , 27 MSO_I }, // ar 3
    { 7 MSO_I , 40 MSO_I }, { 3 MSO_I , 0 }, // l
    { 9 MSO_I , 35 MSO_I }, { 8 MSO_I , 10 MSO_I }, { 3 MSO_I , 0 }, { 21 MSO_I , 33 MSO_I },        // wa 1
    { 9 MSO_I , 36 MSO_I }, { 8 MSO_I , 1 MSO_I }, { 21 MSO_I , 31 MSO_I }, { 20 MSO_I , 31 MSO_I }, // wa 2
    { 9 MSO_I , 35 MSO_I }, { 8 MSO_I , 10 MSO_I }, { 20 MSO_I , 33 MSO_I }, { 0 , 0 },              // wa 3
    { 5 MSO_I , 40 MSO_I },  // l

    { 9 MSO_I , 36 MSO_I }, { 8 MSO_I , 1 MSO_I }, { 20 MSO_I , 31 MSO_I }, { 0 MSO_I , 32 MSO_I },  // wr
    { 20 MSO_I , 33 MSO_I }, // l

    { 9 MSO_I , 36 MSO_I }, { 8 MSO_I , 1 MSO_I }, { 21 MSO_I , 31 MSO_I }, { 22 MSO_I , 32 MSO_I }, // ar
    { 21 MSO_I , 33 MSO_I }, // l

    { 0 MSO_I , 26 MSO_I },  // m
    { 0 MSO_I , 32 MSO_I },  // l

    { 22 MSO_I , 26 MSO_I }, // m
    { 22 MSO_I , 32 MSO_I }  // l
};

const sal_uInt16 mso_sptEllipseRibbonSegm[] =
{
    0xa30c/*ar*/, 0x0002/*l*/,  0xa50c/*wa*/, 0x0001/*l*/, 0x6001/*x*/, 0x8000/*e*/,
    0xa504/*wr*/, 0x0001/*l*/,  0x6001/*x*/,  0x8000/*e*/,
    0xa304/*ar*/, 0x0001/*l*/,  0x6001/*x*/,  0x8000/*e*/,
    0x4000/*m*/,  0xaa00/*nf*/, 0x0001/*l*/,  0x8000/*e*/,
    0x4000/*m*/,  0xaa00/*nf*/, 0x0001/*l*/,  0x8000/*e*/
};

const SvxMSDffCalculationData mso_sptEllipseRibbonCalc[] =
{
    { 0x2000 , { DFF_Prop_adjustValue , 0 , 0 } }, //val #0
    { 0x2000 , { DFF_Prop_adjust2Value , 0 , 0 } }, //val #1
    { 0x2000 , { DFF_Prop_adjust3Value , 0 , 0 } }, //val #2
    { 0x2000 , { DFF_Prop_geoRight , 0 , 0 } }, //val width
    { 0x2000 , { DFF_Prop_geoBottom , 0 , 0 } }, //val height
    { 0x2001 , { DFF_Prop_geoRight , 1 , 8 } }, //prod width 1 8
    { 0x2001 , { DFF_Prop_geoRight , 1 , 2 } }, //prod width 1 2
    { 0x2001 , { DFF_Prop_geoRight , 7 , 8 } }, //prod width 7 8
    { 0x2001 , { DFF_Prop_geoRight , 3 , 2 } }, //prod width 3 2
    { 0x8000 , { 0 , 0 , 0x406 } }, //sum 0 0 @6
    { 0xa000 , { DFF_Prop_geoBottom , 0 , DFF_Prop_adjust3Value } }, //sum height 0 #2
    { 0x2001 , { 0x40a , 30573 , 4096 } }, //prod @10 30573 4096
    { 0x2001 , { 0x40b , 2 , 1 } }, //prod @11 2 1
    { 0xa000 , { DFF_Prop_geoBottom , 0 , 0x40c } }, //sum height 0 @12
    { 0x6000 , { 0x40b , DFF_Prop_adjust3Value , 0 } }, //sum @11 #2 0
    { 0xe000 , { 0x40b , DFF_Prop_geoBottom , DFF_Prop_adjust2Value } }, //sum @11 height #1
    { 0xa000 , { DFF_Prop_geoBottom , 0 , DFF_Prop_adjust2Value } }, //sum height 0 #1
    { 0x2001 , { 0x410 , 1 , 2 } }, //prod @16 1 2
    { 0x6000 , { 0x40b , 0x411 , 0 } }, //sum @11 @17 0
    { 0xe000 , { 0x40e , DFF_Prop_adjust2Value , DFF_Prop_geoBottom } }, //sum @14 #1 height
    { 0x6000 , { DFF_Prop_adjustValue , 0x405 , 0 } }, //sum #0 @5 0
    { 0xa000 , { DFF_Prop_geoRight , 0 , 0x414 } }, //sum width 0 @20
    { 0xa000 , { DFF_Prop_geoRight , 0 , DFF_Prop_adjustValue } }, //sum width 0 #0
    { 0xa000 , { 0x406 , 0 , DFF_Prop_adjustValue } }, //sum @6 0 #0
    { 0xe00f , { 0x417 , DFF_Prop_geoRight , 0x40b } }, //ellipse @23 width @11
    { 0xe000 , { 0x418 , DFF_Prop_geoBottom , 0x40b } }, //sum @24 height @11
    { 0xe000 , { 0x419 , 0x40b , 0x413 } }, //sum @25 @11 @19
    { 0xe000 , { DFF_Prop_adjust3Value , 0x40b , 0x413 } }, //sum #2 @11 @19
    { 0x2001 , { 0x40b , 2391 , 32768 } }, //prod @11 2391 32768
    { 0xa000 , { 0x406 , 0 , 0x414 } }, //sum @6 0 @20
    { 0xe00f , { 0x41d , DFF_Prop_geoRight , 0x40b } }, //ellipse @29 width @11
    { 0xe000 , { DFF_Prop_adjust2Value , 0x41e , 0x40b } }, //sum #1 @30 @11
    { 0xe000 , { 0x419 , DFF_Prop_adjust2Value , DFF_Prop_geoBottom } }, //sum @25 #1 height
    { 0xe000 , { DFF_Prop_geoBottom , 0x41e , 0x40e } }, //sum height @30 @14
    { 0x6000 , { 0x40b , 0x40e , 0 } }, //sum @11 @14 0
    { 0xa000 , { DFF_Prop_geoBottom , 0 , 0x422 } }, //sum height 0 @34
    { 0xe000 , { 0x423 , 0x413 , 0x40b } }, //sum @35 @19 @11
    { 0xe000 , { 0x40a , 0x40f , 0x40b } }, //sum @10 @15 @11
    { 0xe000 , { 0x423 , 0x40f , 0x40b } }, //sum @35 @15 @11
    { 0xe000 , { 0x41c , 0x40e , 0x412 } }, //sum @28 @14 @18
    { 0xa000 , { DFF_Prop_geoBottom , 0 , 0x427 } }, //sum height 0 @39
    { 0xa000 , { 0x413 , 0 , 0x412 } }, //sum @19 0 @18
    { 0x2001 , { 0x429 , 2 , 3 } }, //prod @41 2 3
    { 0xa000 , { DFF_Prop_adjust2Value , 0 , 0x42a } }, //sum #1 0 @42
    { 0xa000 , { DFF_Prop_adjust3Value , 0 , 0x42a } }, //sum #2 0 @42
    { 0x2004 , { 0x42c , 20925 , 0 } }, //min @44 20925
    { 0x2001 , { DFF_Prop_geoRight , 3 , 8 } }, //prod width 3 8
    { 0x2000 , { 0x42e , 0 , 4 } }, //sum @46 0 4
};

const SvxMSDffTextRectangles mso_sptEllipseRibbonTextRect[] =
{//@0,@1,@22,@25
    { { 0 MSO_I, 1 MSO_I }, { 22 MSO_I, 25 MSO_I } }
};

const sal_Int32 mso_sptEllipseRibbonDefault[] =
{
    3,5400,5400,18900
};

const SvxMSDffHandle mso_sptEllipseRibbonHandle[] =
{
//position="#0,bottomRight" xrange="@5,@47
//position="center,#1" yrange="@10,@43
//position="topLeft,#2" yrange="@27,@45
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL| SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL,
        0x100, 21600, 0, 0, 0x8/*5+3*/, 0x32/*47+3*/, MIN_INT32, 0x7fffffff },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL,
        10800, 0x101, 0, 0, MIN_INT32, 0x7fffffff,0xd/*10+3*/, 0x2e /*43+3*/ },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL,
        0, 0x102, 0, 0, MIN_INT32, 0x7fffffff,0x1e/*27+3*/, 0x30 /*45+3*/ }
};

const mso_CustomShape msosptEllipseRibbon =
{
    std::span<const SvxMSDffVertPair>(mso_sptEllipseRibbonVert),
    const_cast<sal_uInt16*>(mso_sptEllipseRibbonSegm), sizeof( mso_sptEllipseRibbonSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptEllipseRibbonCalc),
    const_cast<sal_Int32*>(mso_sptEllipseRibbonDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptEllipseRibbonTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptEllipseRibbonHandle),
};

//msosptEllipseRibbon2
//svg path = wr@9@34@8@35,0@24@0@23@9,0@8@11@0@22@19@22@9@34@8@35@19@23@3@24l@7@36@3@4at@9@31@8@32@3@4@18@30@9@1@8@33@18@28@17@28@9@31@8@32@17@30,0@4l@5@36xear@9@1@8@33@17@28@0@29nfl@17@30ewr@9@1@8@33@18@28@19@29nfl@18@30em@0@23nfl@0@29em@19@23nfl@19@29e
const SvxMSDffVertPair mso_sptEllipseRibbon2Vert[] =
{
    { 9 MSO_I , 34 MSO_I }, { 8 MSO_I , 35 MSO_I }, { 0 , 24 MSO_I }, { 0 MSO_I , 23 MSO_I },        // wr 1
    { 9 MSO_I , 0 }, { 8 MSO_I , 11 MSO_I }, { 0 MSO_I , 22 MSO_I }, { 19 MSO_I , 22 MSO_I },        // wr 2
    { 9 MSO_I , 34 MSO_I }, { 8 MSO_I , 35 MSO_I }, { 19 MSO_I , 23 MSO_I }, { 3 MSO_I , 24 MSO_I }, // wr 3
    { 7 MSO_I , 36 MSO_I }, { 3 MSO_I , 4 MSO_I },   // l
    { 9 MSO_I , 31 MSO_I }, { 8 MSO_I , 32 MSO_I }, { 3 MSO_I , 4 MSO_I }, { 18 MSO_I , 30 MSO_I },  // at 1
    { 9 MSO_I , 1 MSO_I }, { 8 MSO_I , 33 MSO_I }, { 18 MSO_I , 28 MSO_I }, { 17 MSO_I , 28 MSO_I }, // at 2
    { 9 MSO_I , 31 MSO_I }, { 8 MSO_I , 32 MSO_I }, { 17 MSO_I , 30 MSO_I }, { 0 , 4l MSO_I },       // at 3
    { 5 MSO_I , 36 MSO_I },  // l

    { 9 MSO_I , 1 MSO_I }, { 8 MSO_I , 33 MSO_I }, { 17 MSO_I , 28 MSO_I }, { 0 MSO_I , 29 MSO_I },  // ar
    { 17 MSO_I , 30 MSO_I }, // l

    { 9 MSO_I , 1 MSO_I }, { 8 MSO_I , 33 MSO_I }, { 18 MSO_I , 28 MSO_I }, { 19 MSO_I , 29 MSO_I }, // wr
    { 18 MSO_I , 30 MSO_I }, // l

    { 0 MSO_I , 23 MSO_I }, { 0 MSO_I , 29 MSO_I },  // m l

    { 19 MSO_I , 23 MSO_I }, { 19 MSO_I , 29 MSO_I } // m l
};
const sal_uInt16 mso_sptEllipseRibbon2Segm[] =
{
    0xa50c/*wr*/, 0x0002/*l*/,  0xa30c/*at*/, 0x0001/*l*/, 0x6001/*x*/, 0x8000/*e*/,
    0xa304/*ar*/, 0x0001/*l*/,  0x6001/*x*/,  0x8000/*e*/,
    0xa504/*wr*/, 0x0001/*l*/,  0x6001/*x*/,  0x8000/*e*/,
    0x4000/*m*/,  0xaa00/*nf*/, 0x0001/*l*/,  0x8000/*e*/,
    0x4000/*m*/,  0xaa00/*nf*/, 0x0001/*l*/,  0x8000/*e*/
};

const SvxMSDffCalculationData mso_sptEllipseRibbon2Calc[] =
{
    { 0x2000 , { DFF_Prop_adjustValue , 0 , 0 } }, //val #0
    { 0x2000 , { DFF_Prop_adjust2Value , 0 , 0 } }, //val #1
    { 0x2000 , { DFF_Prop_adjust3Value , 0 , 0 } }, //val #2
    { 0x2000 , { DFF_Prop_geoRight , 0 , 0 } }, //val width
    { 0x2000 , { DFF_Prop_geoBottom , 0 , 0 } }, //val height
    { 0x2001 , { DFF_Prop_geoRight , 1 , 8 } }, //prod width 1 8
    { 0x2001 , { DFF_Prop_geoRight , 1 , 2 } }, //prod width 1 2
    { 0x2001 , { DFF_Prop_geoRight , 7 , 8 } }, //prod width 7 8
    { 0x2001 , { DFF_Prop_geoRight , 3 , 2 } }, //prod width 3 2
    { 0x8000 , { 0 , 0 , 0x406 } }, //sum 0 0 @6
    { 0x2001 , { DFF_Prop_adjust3Value , 30573 , 4096 } }, //prod #2 30573 4096
    { 0x2001 , { 0x40a , 2 , 1 } }, //prod @10 2 1
    { 0xe000 , { 0x40a , DFF_Prop_geoBottom , DFF_Prop_adjust3Value } }, //sum @10 height #2
    { 0x6000 , { 0x40a , DFF_Prop_adjust2Value , 0 } }, //sum @10 #1 0
    { 0x2001 , { DFF_Prop_adjust2Value , 1 , 2 } }, //prod #1 1 2
    { 0x6000 , { 0x40a , 0x40e , 0 } }, //sum @10 @14 0
    { 0xa000 , { 0x40c , 0 , DFF_Prop_adjust2Value } }, //sum @12 0 #1
    { 0x6000 , { DFF_Prop_adjustValue , 0x405 , 0 } }, //sum #0 @5 0
    { 0xa000 , { DFF_Prop_geoRight , 0 , 0x411 } }, //sum width 0 @17
    { 0xa000 , { DFF_Prop_geoRight , 0 , DFF_Prop_adjustValue } }, //sum width 0 #0
    { 0xa000 , { 0x406 , 0 , DFF_Prop_adjustValue } }, //sum @6 0 #0
    { 0xe00f , { 0x414 , DFF_Prop_geoRight , 0x40a } }, //ellipse @20 width @10
    { 0xa000 , { 0x40a , 0 , 0x415 } }, //sum @10 0 @21
    { 0xe000 , { 0x416 , 0x410 , 0x40a } }, //sum @22 @16 @10
    { 0xe000 , { DFF_Prop_adjust3Value , 0x410 , 0x40a } }, //sum #2 @16 @10
    { 0x2001 , { 0x40a , 2391 , 32768 } }, //prod @10 2391 32768
    { 0xa000 , { 0x406 , 0 , 0x411 } }, //sum @6 0 @17
    { 0xe00f , { 0x41a , DFF_Prop_geoRight , 0x40a } }, //ellipse @26 width @10
    { 0xe000 , { 0x40a , DFF_Prop_adjust2Value , 0x41b } }, //sum @10 #1 @27
    { 0x6000 , { 0x416 , DFF_Prop_adjust2Value , 0 } }, //sum @22 #1 0
    { 0xa000 , { 0x40c , 0 , 0x41b } }, //sum @12 0 @27
    { 0xa000 , { DFF_Prop_geoBottom , 0 , DFF_Prop_adjust3Value } }, //sum height 0 #2
    { 0x6000 , { 0x40a , 0x40c , 0 } }, //sum @10 @12 0
    { 0xe000 , { 0x420 , 0x40a , 0x410 } }, //sum @32 @10 @16
    { 0xe000 , { 0x41f , 0x40a , 0x40d } }, //sum @31 @10 @13
    { 0xe000 , { 0x420 , 0x40a , 0x40d } }, //sum @32 @10 @13
    { 0xe000 , { 0x419 , 0x40c , 0x40f } }, //sum @25 @12 @15
    { 0xa000 , { 0x410 , 0 , 0x40f } }, //sum @16 0 @15
    { 0x2001 , { 0x425 , 2 , 3 } }, //prod @37 2 3
    { 0x6000 , { 0x401 , 0x426 , 0 } }, //sum @1 @38 0
    { 0x6000 , { DFF_Prop_adjust3Value , 0x426 , 0 } }, //sum #2 @38 0
    { 0x2005 , { 0x428 , 675 , 0 } }, //max @40 675
    { 0x2001 , { DFF_Prop_geoRight , 3 , 8 } }, //prod width 3 8
    { 0x2000 , { 0x42a , 0 , 4 } } //sum @42 0 4
};

const SvxMSDffTextRectangles mso_sptEllipseRibbon2TextRect[] =
{//@0,@22,@19,@1
    { { 0 MSO_I, 22 MSO_I }, { 19 MSO_I, 1 MSO_I } }
};

const sal_Int32 mso_sptEllipseRibbon2Default[] =
{
    3,5400,16200,2700
};

const SvxMSDffHandle mso_sptEllipseRibbon2Handle[] =
{
//position="#0,topLeft" xrange="@5,@43
//position="center,#1" yrange="@39,@31
//position="topLeft,#2" yrange="@41,@24
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL| SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL,
        0x100, 0, 0, 0, 0x8/*5+3*/, 0x2e/*43+3*/, MIN_INT32, 0x7fffffff },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL,
        10800, 0x101, 0, 0, MIN_INT32, 0x7fffffff,0x2a/*39+3*/, 0x22 /*31+3*/ },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL,
        0, 0x102, 0, 0, MIN_INT32, 0x7fffffff,0x2c/*41+3*/, 0x1b /*24+3*/ }
};

const mso_CustomShape msosptEllipseRibbon2 =
{
    std::span<const SvxMSDffVertPair>(mso_sptEllipseRibbon2Vert),
    const_cast<sal_uInt16*>(mso_sptEllipseRibbon2Segm), sizeof( mso_sptEllipseRibbon2Segm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptEllipseRibbon2Calc),
    const_cast<sal_Int32*>(mso_sptEllipseRibbon2Default),
    std::span<const SvxMSDffTextRectangles>(mso_sptEllipseRibbon2TextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptEllipseRibbon2Handle),
};
// End
const SvxMSDffVertPair mso_sptVerticalScrollVert[] = // adjustment1 : 0 - 5400
{
    { 1 MSO_I, 21600 }, { 0, 11 MSO_I }, { 1 MSO_I, 12 MSO_I }, { 0 MSO_I, 12 MSO_I },
    { 0 MSO_I, 1 MSO_I }, { 4 MSO_I, 0 }, { 2 MSO_I, 0 }, { 21600, 1 MSO_I },
    { 2 MSO_I, 0 MSO_I }, { 3 MSO_I, 0 MSO_I }, { 3 MSO_I, 11 MSO_I }, { 5 MSO_I, 21600 },

    { 6 MSO_I, 1 MSO_I }, { 4 MSO_I, 0 MSO_I }, { 8 MSO_I, 9 MSO_I }, { 4 MSO_I, 1 MSO_I },

    { 0 MSO_I, 11 MSO_I }, { 1 MSO_I, 21600 }, { 0, 11 MSO_I }, { 1 MSO_I, 12 MSO_I },
    { 9 MSO_I, 10 MSO_I }, { 1 MSO_I, 11 MSO_I },

    { 4 MSO_I, 0 }, { 6 MSO_I, 1 MSO_I },

    { 0 MSO_I, 12 MSO_I }, { 0 MSO_I, 11 MSO_I },

    { 4 MSO_I, 0 MSO_I },
    { 2 MSO_I, 0 MSO_I }
};
const sal_uInt16 mso_sptVerticalScrollSegm[] =
{
    0x4000, 0xa702, 0x0002, 0xa801, 0x0001, 0xa702, 0x0002, 0xa801, 0x6001, 0x8000,
    0x4000, 0xa801, 0xa702, 0x6000, 0x8000,
    0x4000, 0xa803, 0xa702, 0x6001, 0x8000,
    0x4000, 0xa701, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffCalculationData mso_sptScrollCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2001, { 0x400, 1, 2 } },
    { 0xa000, { DFF_Prop_geoRight, 0, 0x401 } },
    { 0xa000, { DFF_Prop_geoRight, 0, 0x400 } },
    { 0x6000, { 0x400, 0x401, 0 } },
    { 0xa000, { DFF_Prop_geoRight, 0, 0x404 } },
    { 0x2001, { 0x400, 2, 1 } },
    { 0x2001, { 0x401, 1, 2 } },
    { 0x6000, { 0x400, 0x407, 0 } },
    { 0x6000, { 0x401, 0x407, 0 } },
    { 0xa000, { DFF_Prop_geoBottom, 0, 0x409 } },
    { 0xa000, { DFF_Prop_geoBottom, 0, 0x401 } },
    { 0xa000, { DFF_Prop_geoBottom, 0, 0x400 } },
    { 0xa000, { DFF_Prop_geoBottom, 0, 0x404 } }
};
const SvxMSDffTextRectangles mso_sptScrollTextRect[] =
{
    { { 0 MSO_I, 0 MSO_I }, { 3 MSO_I, 12 MSO_I } }
};
const SvxMSDffHandle mso_sptVerticalScrollHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 5400 }
};
const mso_CustomShape msoVerticalScroll =
{
    std::span<const SvxMSDffVertPair>(mso_sptVerticalScrollVert),
    const_cast<sal_uInt16*>(mso_sptVerticalScrollSegm), sizeof( mso_sptVerticalScrollSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptScrollCalc),
    const_cast<sal_Int32*>(mso_sptDefault2700),
    std::span<const SvxMSDffTextRectangles>(mso_sptScrollTextRect),
    21600, 21600,
    11000, 10800,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptVerticalScrollHandle),
};
const SvxMSDffVertPair mso_sptHorizontalScrollVert[] =   // adjustment1 : 0 - 5400
{
    { 0, 4 MSO_I }, { 1 MSO_I, 0 MSO_I }, { 3 MSO_I, 0 MSO_I }, { 3 MSO_I, 1 MSO_I },
    { 2 MSO_I, 0 }, { 21600, 1 MSO_I }, { 21600, 13 MSO_I }, { 2 MSO_I, 12 MSO_I },
    { 0 MSO_I, 12 MSO_I }, { 0 MSO_I, 11 MSO_I }, { 1 MSO_I, 21600 }, { 0, 11 MSO_I },

    { 1 MSO_I, 4 MSO_I }, { 9 MSO_I, 8 MSO_I }, { 0 MSO_I, 4 MSO_I }, { 1 MSO_I, 6 MSO_I },

    { 2 MSO_I, 1 MSO_I }, { 3 MSO_I, 9 MSO_I }, { 3 MSO_I, 1 MSO_I }, { 2 MSO_I, 0 },
    { 21600, 1 MSO_I }, { 2 MSO_I, 0 MSO_I },

    { 1 MSO_I, 6 MSO_I },
    { 0, 4 MSO_I },

    { 2 MSO_I, 0 MSO_I },
    { 3 MSO_I, 0 MSO_I },

    { 0 MSO_I, 4 MSO_I },
    { 0 MSO_I, 11 MSO_I }
};
const sal_uInt16 mso_sptHorizontalScrollSegm[] =
{
    0x4000, 0xa801, 0x0002, 0xa802, 0x0001, 0xa801, 0x0002, 0xa802, 0x6001, 0x8000,
    0x4000, 0xa803, 0x6000, 0x8000,
    0x4000, 0xa803, 0xa702, 0x6000, 0x8000,
    0x4000, 0xa701, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffHandle mso_sptHorizontalScrollHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 0, 10800, 10800, 0, 5400, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoHorizontalScroll =
{
    std::span<const SvxMSDffVertPair>(mso_sptHorizontalScrollVert),
    const_cast<sal_uInt16*>(mso_sptHorizontalScrollSegm), sizeof( mso_sptHorizontalScrollSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptScrollCalc),
    const_cast<sal_Int32*>(mso_sptDefault2700),
    std::span<const SvxMSDffTextRectangles>(mso_sptScrollTextRect),
    21600, 21600,
    10800, 11000,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptHorizontalScrollHandle),
};

const SvxMSDffVertPair mso_sptFlowChartProcessVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 }, { 0, 0 }
};
const mso_CustomShape msoFlowChartProcess =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartProcessVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartAlternateProcessVert[] =
{
    { 0, 2 MSO_I }, { 0 MSO_I, 0 }, { 1 MSO_I, 0 }, { 21600, 2 MSO_I },
    { 21600, 3 MSO_I }, { 1 MSO_I, 21600 }, { 0 MSO_I, 21600 }, { 0, 3 MSO_I }
};
const sal_uInt16 mso_sptFlowChartAlternateProcessSegm[] =
{
    0x4000, 0xa801, 0x0001, 0xa701, 0x0001, 0xa801, 0x0001, 0xa701, 0x6000, 0x8000
};
const SvxMSDffCalculationData mso_sptFlowChartAlternateProcessCalc[] =
{
    { 0x2000, { DFF_Prop_geoLeft, 3600, 0 } },
    { 0x2000, { DFF_Prop_geoRight, 0, 3600 } },
    { 0x2000, { DFF_Prop_geoTop, 3600, 0 } },
    { 0x2000, { DFF_Prop_geoBottom, 0, 3600 } },
    { 0x2000, { DFF_Prop_geoLeft, 1054, 0 } },
    { 0x2000, { DFF_Prop_geoRight, 0, 1054 } },
    { 0x2000, { DFF_Prop_geoTop, 1054, 0 } },
    { 0x2000, { DFF_Prop_geoBottom,0, 1054 } }
};
const SvxMSDffTextRectangles mso_sptFlowChartAlternateProcessTextRect[] =
{
    { { 4 MSO_I, 6 MSO_I }, { 5 MSO_I, 7 MSO_I } }
};
const mso_CustomShape msoFlowChartAlternateProcess =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartAlternateProcessVert),
    const_cast<sal_uInt16*>(mso_sptFlowChartAlternateProcessSegm), sizeof( mso_sptFlowChartAlternateProcessSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptFlowChartAlternateProcessCalc),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartAlternateProcessTextRect),
    21600, 21600,
    10800, 10800,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartDecisionVert[] =
{
    { 0, 10800 }, { 10800, 0 }, { 21600, 10800 }, { 10800, 21600 }, { 0, 10800 }
};
const SvxMSDffTextRectangles mso_sptFlowChartDecisionTextRect[] =
{
    { { 5400, 5400 }, { 16200, 16200 } }
};
const mso_CustomShape msoFlowChartDecision =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartDecisionVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartDecisionTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartInputOutputVert[] =
{
    { 4230, 0 }, { 21600, 0 }, { 17370, 21600 }, { 0, 21600 }, { 4230, 0 }
};
const SvxMSDffTextRectangles mso_sptFlowChartInputOutputTextRect[] =
{
    { { 4230, 0 }, { 17370, 21600 } }
};
const SvxMSDffVertPair mso_sptFlowChartInputOutputGluePoints[] =
{
    { 12960, 0 }, { 10800, 0 }, { 2160, 10800 }, { 8600, 21600 }, { 10800, 21600 }, { 19400, 10800 }
};
const mso_CustomShape msoFlowChartInputOutput =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartInputOutputVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartInputOutputTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartInputOutputGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartPredefinedProcessVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 },

    { 2540, 0 }, { 2540, 21600 },

    { 21600 - 2540, 0 }, { 21600 - 2540, 21600 }
};
const sal_uInt16 mso_sptFlowChartPredefinedProcessSegm[] =
{
    0x4000, 0x0003, 0x6000, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffTextRectangles mso_sptFlowChartPredefinedProcessTextRect[] =
{
    { { 2540, 0 }, { 21600 - 2540, 21600 } }
};
const mso_CustomShape msoFlowChartPredefinedProcess =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartPredefinedProcessVert),
    const_cast<sal_uInt16*>(mso_sptFlowChartPredefinedProcessSegm), sizeof( mso_sptFlowChartPredefinedProcessSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartPredefinedProcessTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartInternalStorageVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 },

    { 4230, 0 }, { 4230, 21600 },

    { 0, 4230 }, { 21600, 4230 }
};
const sal_uInt16 mso_sptFlowChartInternalStorageSegm[] =
{
    0x4000, 0x0003, 0x6000, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffTextRectangles mso_sptFlowChartInternalStorageTextRect[] =
{
    { { 4230, 4230 }, { 21600, 21600 } }
};
const mso_CustomShape msoFlowChartInternalStorage =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartInternalStorageVert),
    const_cast<sal_uInt16*>(mso_sptFlowChartInternalStorageSegm), sizeof( mso_sptFlowChartInternalStorageSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartInternalStorageTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartDocumentVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 17360 },
    { 13050, 17220 }, { 13340, 20770 }, { 5620, 21600 },    // ccp
    { 2860, 21100 }, { 1850, 20700 }, { 0,  20120 }         // ccp
};
const sal_uInt16 mso_sptFlowChartDocumentSegm[] =
{
    0x4000, 0x0002, 0x2002, 0x6000, 0x8000
};
const SvxMSDffTextRectangles mso_sptFlowChartDocumentTextRect[] =
{
    { { 0, 0 }, { 21600, 17360 } }
};
const SvxMSDffVertPair mso_sptFlowChartDocumentGluePoints[] =
{
    { 10800, 0 }, { 0, 10800 }, { 10800, 20320 }, { 21600, 10800 }
};
const mso_CustomShape msoFlowChartDocument =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartDocumentVert),
    const_cast<sal_uInt16*>(mso_sptFlowChartDocumentSegm), sizeof( mso_sptFlowChartDocumentSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartDocumentTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartDocumentGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartMultidocumentVert[] =
{
    { 0, 3600 }, { 1500, 3600 }, { 1500, 1800 }, { 3000, 1800 },
    { 3000, 0 }, { 21600, 0 }, { 21600, 14409 }, { 21600 - 1500, 14409 },
    { 21600 - 1500, 14409 + 1800 }, { 21600 - 3000, 14409 + 1800 }, { 21600 - 3000, 14409 + 3600 },
    { 11610, 14293 + 3600 }, { 11472, 17239 + 3600 }, { 4833, 17928 + 3600 },                       // ccp
    { 2450, 17513 + 3600 }, { 1591, 17181 + 3600 }, { 0, 16700 + 3600 },                            // ccp

    { 1500, 3600 }, { 21600 - 3000, 3600 }, { 21600 - 3000, 14409 + 1800 },

    { 3000, 1800 }, { 21600 - 1500, 1800 }, { 21600 - 1500, 14409 }
};
const sal_uInt16 mso_sptFlowChartMultidocumentSegm[] =
{
    0x4000, 0x000a, 0x2002, 0x6000, 0x8000,
    0x4000, 0xaa00, 0x0002, 0x8000,         // NO FILL
    0x4000, 0xaa00, 0x0002, 0x8000          // NO FILL
};
const SvxMSDffTextRectangles mso_sptFlowChartMultidocumentTextRect[] =
{
    { { 0, 3600 }, { 21600 - 3000, 14409 + 3600 } }
};
const SvxMSDffVertPair mso_sptFlowChartMultidocumentGluePoints[] =
{
    { 10800, 0 }, { 0, 10800 }, { 10800, 19890 }, { 21600, 10800 }
};
const mso_CustomShape msoFlowChartMultidocument =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartMultidocumentVert),
    const_cast<sal_uInt16*>(mso_sptFlowChartMultidocumentSegm), sizeof( mso_sptFlowChartMultidocumentSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartMultidocumentTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartMultidocumentGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartTerminatorVert[] =
{
    { 3470, 21600 }, { 0, 10800 }, { 3470, 0 }, { 18130, 0 },
    { 21600, 10800 }, { 18130, 21600 }
};
const sal_uInt16 mso_sptFlowChartTerminatorSegm[] =
{
    0x4000, 0xa702, 0x0001, 0xa702, 0x6000, 0x8000
};
const SvxMSDffTextRectangles mso_sptFlowChartTerminatorTextRect[] =
{
    { { 1060, 3180 }, { 20540, 18420 } }
};
const mso_CustomShape msoFlowChartTerminator =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartTerminatorVert),
    const_cast<sal_uInt16*>(mso_sptFlowChartTerminatorSegm), sizeof( mso_sptFlowChartTerminatorSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartTerminatorTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartPreparationVert[] =
{
    { 4350, 0 }, { 17250, 0 }, { 21600, 10800 }, { 17250, 21600 },
    { 4350, 21600 }, { 0, 10800 }, { 4350, 0 }
};
const SvxMSDffTextRectangles mso_sptFlowChartPreparationTextRect[] =
{
    { { 4350, 0 }, { 17250, 21600 } }
};
const mso_CustomShape msoFlowChartPreparation =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartPreparationVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartPreparationTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartManualInputVert[] =
{
    { 0, 4300 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 }, { 0, 4300 }
};
const SvxMSDffTextRectangles mso_sptFlowChartManualInputTextRect[] =
{
    { { 0, 4300 }, { 21600, 21600 } }
};
const SvxMSDffVertPair mso_sptFlowChartManualInputGluePoints[] =
{
    { 10800, 2150 }, { 0, 10800 }, { 10800, 19890 }, { 21600, 10800 }
};
const mso_CustomShape msoFlowChartManualInput =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartManualInputVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartManualInputTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartManualInputGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartManualOperationVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 17250, 21600 }, { 4350, 21600 }, { 0, 0 }
};
const SvxMSDffTextRectangles mso_sptFlowChartManualOperationTextRect[] =
{
    { { 4350, 0 }, { 17250, 21600 } }
};
const SvxMSDffVertPair mso_sptFlowChartManualOperationGluePoints[] =
{
    { 10800, 0 }, { 2160, 10800 }, { 10800, 21600 }, { 19440, 10800 }
};
const mso_CustomShape msoFlowChartManualOperation =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartManualOperationVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartManualOperationTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartManualOperationGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartConnectorVert[] =
{
    { 10800, 10800 }, { 10800, 10800 },  { 0, 360 }
};
const sal_uInt16 mso_sptFlowChartConnectorSegm[] =
{
    0xa203, 0x6000, 0x8000
};
const SvxMSDffTextRectangles mso_sptFlowChartConnectorTextRect[] =
{
    { { 3180, 3180 }, { 18420, 18420 } }
};
const mso_CustomShape msoFlowChartConnector =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartConnectorVert),
    const_cast<sal_uInt16*>(mso_sptFlowChartConnectorSegm), sizeof( mso_sptFlowChartConnectorSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartConnectorTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptEllipseGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartOffpageConnectorVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 17150 }, { 10800, 21600 },
    { 0, 17150 }, { 0, 0 }
};
const SvxMSDffTextRectangles mso_sptFlowChartOffpageConnectorTextRect[] =
{
    { { 0, 0 }, { 21600, 17150 } }
};
const mso_CustomShape msoFlowChartOffpageConnector =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartOffpageConnectorVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartOffpageConnectorTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartPunchedCardVert[] =
{
    { 4300, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 },
    { 0, 4300 }, { 4300, 0 }
};
const SvxMSDffTextRectangles mso_sptFlowChartPunchedCardTextRect[] =
{
    { { 0, 4300 }, { 21600, 21600 } }
};
const mso_CustomShape msoFlowChartPunchedCard =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartPunchedCardVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartPunchedCardTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartPunchedTapeVert[] =
{
    { 0, 2230 },                                            // p
    { 820, 3990 }, { 3410, 3980 }, { 5370, 4360 },          // ccp
    { 7430, 4030 }, { 10110, 3890 }, { 10690, 2270 },       // ccp
    { 11440, 300 }, { 14200, 160 }, { 16150, 0 },           // ccp
    { 18670, 170 }, {  20690, 390 }, { 21600, 2230 },       // ccp
    { 21600, 19420 },                                       // p
    { 20640, 17510 }, { 18320, 17490 }, { 16140, 17240 },   // ccp
    { 14710, 17370 }, { 11310, 17510 }, { 10770, 19430 },   // ccp
    { 10150, 21150 }, { 7380, 21290 }, { 5290, 21600 },     // ccp
    { 3220, 21250 }, { 610, 21130 }, { 0, 19420 }           // ccp
};
const sal_uInt16 mso_sptFlowChartPunchedTapeSegm[] =
{
    0x4000, 0x2004, 0x0001, 0x2004, 0x6000, 0x8000
};
const SvxMSDffTextRectangles mso_sptFlowChartPunchedTapeTextRect[] =
{
    { { 0, 4360 }, { 21600, 17240 } }
};
const SvxMSDffVertPair mso_sptFlowChartPunchedTapeGluePoints[] =
{
    { 10800, 2020 }, { 0, 10800 }, { 10800, 19320 }, { 21600, 10800 }
};
const mso_CustomShape msoFlowChartPunchedTape =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartPunchedTapeVert),
    const_cast<sal_uInt16*>(mso_sptFlowChartPunchedTapeSegm), sizeof( mso_sptFlowChartPunchedTapeSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartPunchedTapeTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartPunchedTapeGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartSummingJunctionVert[] =
{
    { 10800, 10800 }, { 10800, 10800 }, { 0, 360 },

    { 3100, 3100 },
    { 18500, 18500 },

    { 3100, 18500 },
    { 18500, 3100 }
};
const sal_uInt16 mso_sptFlowChartSummingJunctionSegm[] =
{
    0xa203, 0x6000, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffTextRectangles mso_sptFlowChartSummingJunctionTextRect[] =
{
    { { 3100, 3100 }, { 18500, 18500 } }
};
const mso_CustomShape msoFlowChartSummingJunction =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartSummingJunctionVert),
    const_cast<sal_uInt16*>(mso_sptFlowChartSummingJunctionSegm), sizeof( mso_sptFlowChartSummingJunctionSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartSummingJunctionTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptEllipseGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartOrVert[] =
{
    { 10800, 10800 }, { 10800, 10800 }, { 0, 360 },

    { 0, 10800 }, { 21600, 10800 },

    { 10800, 0 }, { 10800, 21600 }
};
const sal_uInt16 mso_sptFlowChartOrSegm[] =
{
    0xa203, 0x6000, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffTextRectangles mso_sptFlowChartOrTextRect[] =
{
    { { 3100, 3100 }, { 18500, 18500 } }
};
const mso_CustomShape msoFlowChartOr =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartOrVert),
    const_cast<sal_uInt16*>(mso_sptFlowChartOrSegm), sizeof( mso_sptFlowChartOrSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartOrTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptEllipseGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartCollateVert[] =
{
    { 0, 0 }, { 21600, 21600 }, { 0, 21600 }, { 21600, 0 }, { 0, 0 }
};
const SvxMSDffTextRectangles mso_sptFlowChartCollateTextRect[] =
{
    { { 5400, 5400 }, { 16200, 16200 } }
};
const SvxMSDffVertPair mso_sptFlowChartCollateGluePoints[] =
{
    { 10800, 0 }, { 10800, 10800 }, { 10800, 21600 }
};
const mso_CustomShape msoFlowChartCollate =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartCollateVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartCollateTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartCollateGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartSortVert[] =
{
    { 0, 10800 }, { 10800, 0 }, { 21600, 10800 }, { 10800, 21600 },

    { 0, 10800 }, { 21600, 10800 }
};
const sal_uInt16 mso_sptFlowChartSortSegm[] =
{
    0x4000, 0x0003, 0x6000, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffTextRectangles mso_sptFlowChartSortTextRect[] =
{
    { { 5400, 5400 }, { 16200, 16200 } }
};
const mso_CustomShape msoFlowChartSort =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartSortVert),
    const_cast<sal_uInt16*>(mso_sptFlowChartSortSegm), sizeof( mso_sptFlowChartSortSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartSortTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartExtractVert[] =
{
    { 10800, 0 }, { 21600, 21600 }, { 0, 21600 }, { 10800, 0 }
};
const SvxMSDffTextRectangles mso_sptFlowChartExtractTextRect[] =
{
    { { 5400, 10800 }, { 16200, 21600 } }
};
const SvxMSDffVertPair mso_sptFlowChartExtractGluePoints[] =
{
    { 10800, 0 }, { 5400, 10800 }, { 10800, 21600 }, { 16200, 10800 }
};
const mso_CustomShape msoFlowChartExtract =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartExtractVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartExtractTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartExtractGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartMergeVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 10800, 21600 }, { 0, 0 }
};
const SvxMSDffTextRectangles mso_sptFlowChartMergeTextRect[] =
{
    { { 5400, 0 }, { 16200, 10800 } }
};
const mso_CustomShape msoFlowChartMerge =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartMergeVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartMergeTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartExtractGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartOnlineStorageVert[] =
{
    { 3600, 21600 }, { 0, 10800 }, { 3600, 0 }, { 21600, 0 },
    { 18000, 10800 }, { 21600, 21600 }
};
const sal_uInt16 mso_sptFlowChartOnlineStorageSegm[] =
{
    0x4000, 0xa702, 0x0001, 0xa702, 0x6000, 0x8000
};
const SvxMSDffTextRectangles mso_sptFlowChartOnlineStorageTextRect[] =
{
    { { 3600, 0 }, { 18000, 21600 } }
};
const SvxMSDffVertPair mso_sptFlowChartOnlineStorageGluePoints[] =
{
    { 10800, 0 }, { 0, 10800 }, { 10800, 21600 }, { 18000, 10800 }
};
const mso_CustomShape msoFlowChartOnlineStorage =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartOnlineStorageVert),
    const_cast<sal_uInt16*>(mso_sptFlowChartOnlineStorageSegm), sizeof( mso_sptFlowChartOnlineStorageSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartOnlineStorageTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartOnlineStorageGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartDelayVert[] =
{
    { 10800, 0 }, { 21600, 10800 }, { 10800, 21600 }, { 0, 21600 },
    { 0, 0 }
};
const sal_uInt16 mso_sptFlowChartDelaySegm[] =
{
    0x4000, 0xa702, 0x0002, 0x6000, 0x8000
};
const SvxMSDffTextRectangles mso_sptFlowChartDelayTextRect[] =
{
    { { 0, 3100 }, { 18500, 18500 } }
};
const mso_CustomShape msoFlowChartDelay =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartDelayVert),
    const_cast<sal_uInt16*>(mso_sptFlowChartDelaySegm), sizeof( mso_sptFlowChartDelaySegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartDelayTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartMagneticTapeVert[] =
{
    { 20980, 18150 }, { 20980, 21600 }, { 10670, 21600 },
    { 4770, 21540 }, { 0, 16720 }, { 0, 10800 },            // ccp
    { 0, 4840 }, { 4840, 0 }, { 10800, 0 },                 // ccp
    { 16740, 0 }, { 21600, 4840 }, { 21600, 10800 },        // ccp
    { 21600, 13520 }, { 20550, 16160 }, { 18670, 18170 }    // ccp
};
const sal_uInt16 mso_sptFlowChartMagneticTapeSegm[] =
{
    0x4000, 0x0002, 0x2004, 0x6000, 0x8000
};
const SvxMSDffTextRectangles mso_sptFlowChartMagneticTapeTextRect[] =
{
    { { 3100, 3100 }, { 18500, 18500 } }
};
const mso_CustomShape msoFlowChartMagneticTape =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartMagneticTapeVert),
    const_cast<sal_uInt16*>(mso_sptFlowChartMagneticTapeSegm), sizeof( mso_sptFlowChartMagneticTapeSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartMagneticTapeTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartMagneticDiskVert[] =
{
    { 0, 3400 }, { 10800, 0 }, { 21600, 3400 }, { 21600, 18200 },
    { 10800, 21600 }, { 0, 18200 },

    { 0, 3400 }, { 10800, 6800 }, { 21600, 3400 }
};
const sal_uInt16 mso_sptFlowChartMagneticDiskSegm[] =
{
    0x4000, 0xa802, 0x0001, 0xa802, 0x6000, 0x8000,
    0x4000, 0xa802, 0x8000
};
const SvxMSDffTextRectangles mso_sptFlowChartMagneticDiskTextRect[] =
{
    { { 0, 6800 }, { 21600, 18200 } }
};
const SvxMSDffVertPair mso_sptFlowChartMagneticDiskGluePoints[] =
{
    { 10800, 6800 }, { 10800, 0 }, { 0, 10800 }, { 10800, 21600 }, { 21600, 10800 }
};
const mso_CustomShape msoFlowChartMagneticDisk =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartMagneticDiskVert),
    const_cast<sal_uInt16*>(mso_sptFlowChartMagneticDiskSegm), sizeof( mso_sptFlowChartMagneticDiskSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartMagneticDiskTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartMagneticDiskGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartMagneticDrumVert[] =
{
    { 18200, 0 }, { 21600, 10800 }, { 18200, 21600 }, { 3400, 21600 },
    { 0, 10800 }, { 3400, 0 },

    { 18200, 0 }, { 14800, 10800 }, { 18200, 21600 }
};
const sal_uInt16 mso_sptFlowChartMagneticDrumSegm[] =
{
    0x4000, 0xa702, 0x0001, 0xa702, 0x6000, 0x8000,
    0x4000, 0xa702, 0x8000
};
const SvxMSDffTextRectangles mso_sptFlowChartMagneticDrumTextRect[] =
{
    { { 3400, 0 }, { 14800, 21600 } }
};
const SvxMSDffVertPair mso_sptFlowChartMagneticDrumGluePoints[] =
{
    { 10800, 0 }, { 0, 10800 }, { 10800, 21600 }, { 14800, 10800 }, { 21600, 10800 }
};
const mso_CustomShape msoFlowChartMagneticDrum =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartMagneticDrumVert),
    const_cast<sal_uInt16*>(mso_sptFlowChartMagneticDrumSegm), sizeof( mso_sptFlowChartMagneticDrumSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartMagneticDrumTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartMagneticDrumGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptFlowChartDisplayVert[] =
{
    { 3600, 0 }, { 17800, 0 }, { 21600, 10800 }, { 17800, 21600 },
    { 3600, 21600 }, { 0, 10800 }
};
const sal_uInt16 mso_sptFlowChartDisplaySegm[] =
{
    0x4000, 0x0001, 0xa702, 0x0002, 0x6000, 0x8000
};
const SvxMSDffTextRectangles mso_sptFlowChartDisplayTextRect[] =
{
    { { 3600, 0 }, { 17800, 21600 } }
};
const mso_CustomShape msoFlowChartDisplay =
{
    std::span<const SvxMSDffVertPair>(mso_sptFlowChartDisplayVert),
    const_cast<sal_uInt16*>(mso_sptFlowChartDisplaySegm), sizeof( mso_sptFlowChartDisplaySegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(mso_sptFlowChartDisplayTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptStandardGluePoints),
    std::span<const SvxMSDffHandle>()     // handles
};

const SvxMSDffVertPair mso_sptWedgeRectCalloutVert[] =
{
    { 0, 0 },
    { 0, 3590 }, { 2 MSO_I, 3 MSO_I }, { 0, 8970 },
    { 0, 12630 },{ 4 MSO_I, 5 MSO_I }, { 0, 18010 },
    { 0, 21600 },
    { 3590, 21600 }, { 6 MSO_I, 7 MSO_I }, { 8970, 21600 },
    { 12630, 21600 }, { 8 MSO_I, 9 MSO_I }, { 18010, 21600 },
    { 21600, 21600 },
    { 21600, 18010 }, { 10 MSO_I, 11 MSO_I }, { 21600, 12630 },
    { 21600, 8970 }, { 12 MSO_I, 13 MSO_I }, { 21600, 3590 },
    { 21600, 0 },
    { 18010, 0 }, { 14 MSO_I, 15 MSO_I }, { 12630, 0 },
    { 8970, 0 }, { 16 MSO_I, 17 MSO_I }, { 3590, 0 },
    { 0, 0 }
};
const SvxMSDffCalculationData mso_sptWedgeRectCalloutCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 10800 } },     //0x400
    { 0x2000, { DFF_Prop_adjust2Value, 0,10800 } },
    { 0x6006, { 0x412, DFF_Prop_adjustValue, 0 } },     //0x402
    { 0x6006, { 0x412, DFF_Prop_adjust2Value, 6280 } },
    { 0x6006, { 0x417, DFF_Prop_adjustValue, 0 } },     //0x404
    { 0x6006, { 0x417, DFF_Prop_adjust2Value, 15320 } },
    { 0x6006, { 0x41a, DFF_Prop_adjustValue, 6280 } },  //0x406
    { 0x6006, { 0x41a, DFF_Prop_adjust2Value, 21600 } },
    { 0x6006, { 0x41d, DFF_Prop_adjustValue, 15320 } }, //0x408
    { 0x6006, { 0x41d, DFF_Prop_adjust2Value, 21600 } },
    { 0x6006, { 0x420, DFF_Prop_adjustValue, 21600 } }, //0x40a
    { 0x6006, { 0x420, DFF_Prop_adjust2Value, 15320 } },
    { 0x6006, { 0x422, DFF_Prop_adjustValue, 21600 } }, //0x40c
    { 0x6006, { 0x422, DFF_Prop_adjust2Value, 6280 } },
    { 0x6006, { 0x424, DFF_Prop_adjustValue, 15320 } }, //0x40e
    { 0x6006, { 0x424, DFF_Prop_adjust2Value, 0 } },
    { 0x6006, { 0x426, DFF_Prop_adjustValue, 6280 } },  //0x410
    { 0x6006, { 0x426, DFF_Prop_adjust2Value, 0 } },
    { 0xa006, { DFF_Prop_adjustValue, -1, 0x413 } },    //0x412
    { 0xa006, { 0x401, -1, 0x416 } },
    { 0x2003, { 0x400, 0, 0 } },                        //0x414
    { 0x2003, { 0x401, 0, 0 } },
    { 0xa000, { 0x414, 0, 0x415 } },                    //0x416
    { 0xa006, { DFF_Prop_adjustValue, -1, 0x418 } },
    { 0x6006, { 0x401, 0x416, -1 } },                   //0x418
    { 0x2000, { DFF_Prop_adjust2Value, 0, 21600 } },
    { 0x6006, { 0x419, 0x41b, -1 } },                   //0x41a
    { 0xa006, { 0x400, -1, 0x41c } },
    { 0xa000, { 0x415, 0, 0x414 } },                    //0x41c
    { 0x6006, { 0x419, 0x41e, -1 } },
    { 0x6006, { 0x400, 0x41c, -1 } },                   //0x41e
    { 0x2000, { DFF_Prop_adjustValue, 0, 21600 } },
    { 0x6006, { 0x41f, 0x421, -1 } },                   //0x420
    { 0x6006, { 0x401, 0x416, -1 } },
    { 0x6006, { 0x41f, 0x423, -1 } },                   //0x422
    { 0xa006, { 0x401, -1, 0x416 } },
    { 0xa006, { DFF_Prop_adjust2Value, -1, 0x425 } },   //0x424
    { 0x6006, { 0x400, 0x41c, -1 } },
    { 0xa006, { DFF_Prop_adjust2Value, -1, 0x427 } },   //0x426
    { 0xa006, { 0x400, -1, 0x41c } },
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },         //0x428
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } }
};
const sal_Int32 mso_sptWedgeRectCalloutDefault[] =
{
    2, 1400, 25920
};
const SvxMSDffTextRectangles mso_sptWedgeRectCalloutTextRect[] =
{
    { { 0, 0 }, { 21600, 21600 } }
};
const SvxMSDffVertPair mso_sptWedgeRectCalloutGluePoints[] =
{
    { 10800, 0 }, { 0, 10800 },  { 10800, 21600 }, { 21600, 10800 }, { 40 MSO_I, 41 MSO_I }
};
const SvxMSDffHandle mso_sptCalloutHandle[] =
{
    {
        SvxMSDffHandleFlags::NONE,
        0x100, 0x101, 10800, 10800, MIN_INT32, 0x7fffffff, MIN_INT32, 0x7fffffff
    }
};
const mso_CustomShape msoWedgeRectCallout =
{
    std::span<const SvxMSDffVertPair>(mso_sptWedgeRectCalloutVert),
    nullptr, 0,
    std::span<const SvxMSDffCalculationData>(mso_sptWedgeRectCalloutCalc),
    const_cast<sal_Int32*>(mso_sptWedgeRectCalloutDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptWedgeRectCalloutTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptWedgeRectCalloutGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle),
};
const SvxMSDffVertPair mso_sptWedgeRRectCalloutVert[] =
{
    { 3590, 0 },
    { 0, 3590 },
    { 2 MSO_I, 3 MSO_I }, { 0, 8970 },
    { 0, 12630 },{ 4 MSO_I, 5 MSO_I }, { 0, 18010 },
    { 3590, 21600 },
    { 6 MSO_I, 7 MSO_I }, { 8970, 21600 },
    { 12630, 21600 }, { 8 MSO_I, 9 MSO_I }, { 18010, 21600 },
    { 21600, 18010 },
    { 10 MSO_I, 11 MSO_I }, { 21600, 12630 },
    { 21600, 8970 }, { 12 MSO_I, 13 MSO_I }, { 21600, 3590 },
    { 18010, 0 },
    { 14 MSO_I, 15 MSO_I }, { 12630, 0 },
    { 8970, 0 }, { 16 MSO_I, 17 MSO_I }
};
const sal_uInt16 mso_sptWedgeRRectCalloutSegm[] =
{
    0x4000, 0xa701, 0x0005, 0xa801, 0x0005, 0xa701, 0x0005, 0xa801, 0x0004, 0x6001, 0x8000
};
const SvxMSDffTextRectangles mso_sptWedgeRRectCalloutTextRect[] =
{
    { { 800, 800 }, { 20800, 20800 } }
};
const mso_CustomShape msoWedgeRRectCallout =
{
    std::span<const SvxMSDffVertPair>(mso_sptWedgeRRectCalloutVert),
    const_cast<sal_uInt16*>(mso_sptWedgeRRectCalloutSegm), sizeof( mso_sptWedgeRRectCalloutSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptWedgeRectCalloutCalc),
    const_cast<sal_Int32*>(mso_sptWedgeRectCalloutDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptWedgeRRectCalloutTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle),
};
const SvxMSDffVertPair mso_sptBalloonVert[] =
{
    { 3590, 0 },
    { 0, 3590 },
    { 0, 14460 },
    { 3590, 18050 },
    { 40 MSO_I, 21600 }, { 5420, 18050 },
    { 18010, 18050 },
    { 21600, 14460 },
    { 21600, 3590 },
    { 18010, 0 }
};
const sal_uInt16 mso_sptBalloonSegm[] =
{
    0x4000, 0xa701, 0x0001, 0xa801, 0x0003, 0xa701, 0x0001, 0xa801, 0x6001, 0x8000
};
const SvxMSDffHandle mso_sptBalloonHandle[] =
{
    {
        SvxMSDffHandleFlags::RANGE,
        0x100, 1, 10800, 10800, 0, 8990, MIN_INT32, 0x7fffffff
    }
};
const SvxMSDffTextRectangles mso_sptBalloonTextRect[] =
{
    { { 800, 800 }, { 20800, 17250 } }
};
const mso_CustomShape msoBalloon =
{
    std::span<const SvxMSDffVertPair>(mso_sptBalloonVert),
    const_cast<sal_uInt16*>(mso_sptBalloonSegm), sizeof( mso_sptBalloonSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptWedgeRectCalloutCalc),
    const_cast<sal_Int32*>(mso_sptWedgeRectCalloutDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptBalloonTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptBalloonHandle),
};
const SvxMSDffVertPair mso_sptWedgeEllipseCalloutVert[] =
{
    { 0, 0 }, { 21600, 21600 }, { 0x16 MSO_I, 0x17 MSO_I }, { 0x12 MSO_I, 0x13 MSO_I }, { 0xe MSO_I, 0xf MSO_I }
};
const sal_uInt16 mso_sptWedgeEllipseCalloutSegm[] =
{
    0xa504, 0x0001, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptWedgeEllipseCalloutCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 10800 } },     // 00 rad x
    { 0x2000, { DFF_Prop_adjust2Value, 0, 10800 } },    // 01 rad y
    { 0x6001, { 0x400, 0x400, 1 } },                    // 02 rad x^2
    { 0x6001, { 0x401, 0x401, 1 } },                    // 03 rad y^2
    { 0x6000, { 0x402, 0x403, 0 } },                    // 04
    { 0x200d, { 0x404, 0, 0 } },                        // 05
    { 0x2000, { 0x405, 0, 10800 } },                    // 06 > 0 ? spur needs to be drawn : 10800
    { 0x6008, { 0x400, 0x401, 0 } },                    // 07 atan2 -> angle
    { 0x2000, { 0x407, 0, 10 } },                       // 08
    { 0x2000, { 0x407, 10, 0 } },                       // 09
    { 0x400a, { 10800, 0x407, 0 } },                    // 0a
    { 0x4009, { 10800, 0x407, 0 } },                    // 0b
    { 0x2000, { 0x40a, 10800, 0 } },                    // 0c
    { 0x2000, { 0x40b, 10800, 0 } },                    // 0d
    { 0xe006, { 0x406, DFF_Prop_adjustValue, 0x40c } }, // 0e
    { 0xe006, { 0x406, DFF_Prop_adjust2Value, 0x40d } },// 0f
    { 0x400a, { 10800, 0x408, 0 } },                    // 10
    { 0x4009, { 10800, 0x408, 0 } },                    // 11
    { 0x2000, { 0x410, 10800, 0 } },                    // 12
    { 0x2000, { 0x411, 10800, 0 } },                    // 13
    { 0x400a, { 10800, 0x409, 0 } },                    // 14
    { 0x4009, { 10800, 0x409, 0 } },                    // 15
    { 0x2000, { 0x414, 10800, 0 } },                    // 16
    { 0x2000, { 0x415, 10800, 0 } }                     // 17
};
const sal_Int32 mso_sptWedgeEllipseCalloutDefault[] =
{
    2, 1350, 25920
};
const SvxMSDffVertPair mso_sptWedgeEllipseCalloutGluePoints[] =
{
    { 10800, 0 }, { 3160, 3160 }, { 0, 10800 }, { 3160, 18440 }, { 10800, 21600 }, { 18440, 18440 }, { 21600, 10800 }, { 18440, 3160 }, { 0xe MSO_I, 0xf MSO_I }
};
const SvxMSDffTextRectangles mso_sptWedgeEllipseCalloutTextRect[] =
{
    { { 3200, 3200 }, { 18400, 18400 } }
};
const mso_CustomShape msoWedgeEllipseCallout =
{
    std::span<const SvxMSDffVertPair>(mso_sptWedgeEllipseCalloutVert),
    const_cast<sal_uInt16*>(mso_sptWedgeEllipseCalloutSegm), sizeof( mso_sptWedgeEllipseCalloutSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptWedgeEllipseCalloutCalc),
    const_cast<sal_Int32*>(mso_sptWedgeEllipseCalloutDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptWedgeEllipseCalloutTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptWedgeEllipseCalloutGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle),
};

const SvxMSDffVertPair mso_sptCloudCalloutVert[] =
{
    { 1930,7160 },                                      // p
    { 1530,4490 }, { 3400,1970 }, { 5270,1970 },        // ccp
    { 5860,1950 }, { 6470,2210 }, { 6970,2600 },        // ccp
    { 7450,1390 }, { 8340,650 }, { 9340,650 },          // ccp
    { 10004,690 }, { 10710,1050 }, { 11210,1700 },      // ccp
    { 11570,630 }, { 12330,0 }, { 13150,0 },            // ccp
    { 13840,0 }, { 14470,460 }, { 14870,1160 },         // ccp
    { 15330,440 }, { 16020,0 }, { 16740,0 },            // ccp
    { 17910,0 }, { 18900,1130 }, { 19110,2710 },        // ccp
    { 20240,3150 }, { 21060,4580 }, { 21060,6220 },     // ccp
    { 21060,6720 }, { 21000,7200 }, { 20830,7660 },     // ccp
    { 21310,8460 }, { 21600,9450 }, { 21600,10460 },    // ccp
    { 21600,12750 }, { 20310,14680 }, { 18650,15010 },  // ccp
    { 18650,17200 }, { 17370,18920 }, { 15770,18920 },  // ccp
    { 15220,18920 }, { 14700,18710 }, { 14240,18310 },  // ccp
    { 13820,20240 }, { 12490,21600 }, { 11000,21600 },  // ccp
    { 9890,21600 }, { 8840,20790 }, { 8210,19510 },     // ccp
    { 7620,20000 }, { 7930,20290 }, { 6240,20290 },     // ccp
    { 4850,20290 }, { 3570,19280 }, { 2900,17640 },     // ccp
    { 1300,17600 }, { 480,16300 }, { 480,14660 },       // ccp
    { 480,13900 }, { 690,13210 }, { 1070,12640 },       // ccp
    { 380,12160 }, { 0,11210 }, { 0,10120 },            // ccp
    { 0,8590 }, { 840,7330 }, { 1930,7160 },            // ccp

    { 1930, 7160 }, { 1950, 7410 }, { 2040, 7690 }, { 2090, 7920 },         // pccp
    { 6970, 2600 }, { 7200, 2790 }, { 7480, 3050 }, { 7670, 3310 },         // pccp
    { 11210, 1700 }, { 11130, 1910 }, { 11080, 2160 }, { 11030, 2400 },     // pccp
    { 14870, 1160 }, { 14720, 1400 }, { 14640, 1720 }, { 14540, 2010 },     // pccp
    { 19110, 2710 }, { 19130, 2890 }, { 19230, 3290 }, { 19190, 3380 },     // pccp
    { 20830, 7660 }, { 20660, 8170 }, { 20430, 8620 }, { 20110, 8990 },     // pccp
    { 18660, 15010 }, { 18740, 14200 }, { 18280, 12200 }, { 17000, 11450 }, // pccp
    { 14240, 18310 }, { 14320, 17980 }, { 14350, 17680 }, { 14370, 17360 }, // pccp
    { 8220, 19510 }, { 8060, 19250 }, { 7960, 18950 }, { 7860, 18640 },     // pccp
    { 2900, 17640 }, { 3090, 17600 }, { 3280, 17540 }, { 3460, 17450 },     // pccp
    { 1070, 12640 }, { 1400, 12900 }, { 1780, 13130 }, { 2330, 13040 },     // pccp

    { 0x11 MSO_I, 0x12 MSO_I }, { 1800, 1800 }, { 0, 360 },                 // circ1
    { 0x13 MSO_I, 0x14 MSO_I }, { 1200, 1200 }, { 0, 360 },                 // circ2
    { 0xd MSO_I, 0xe MSO_I }, { 700, 700 }, { 0, 360 }                      // circ3
};
const sal_uInt16 mso_sptCloudCalloutSegm[] =
{
    0x4000, 0x2016, 0x6001, 0x8000,
    0x4000, 0x2001, 0xaa00, 0x8000,
    0x4000, 0x2001, 0xaa00, 0x8000,
    0x4000, 0x2001, 0xaa00, 0x8000,
    0x4000, 0x2001, 0xaa00, 0x8000,
    0x4000, 0x2001, 0xaa00, 0x8000,
    0x4000, 0x2001, 0xaa00, 0x8000,
    0x4000, 0x2001, 0xaa00, 0x8000,
    0x4000, 0x2001, 0xaa00, 0x8000,
    0x4000, 0x2001, 0xaa00, 0x8000,
    0x4000, 0x2001, 0xaa00, 0x8000,
    0x4000, 0x2001, 0xaa00, 0x8000,
    0xa203, 0x6001, 0x8000,
    0xa203, 0x6001, 0x8000,
    0xa203, 0x6001, 0x8000
};
const SvxMSDffCalculationData mso_sptCloudCalloutCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 10800 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 10800 } },
    { 0x6008, { 0x400, 0x401, 0 } },
    { 0x400a, { 10800, 0x402, 0 } },                    // 3
    { 0x4009, { 10800, 0x402, 0 } },                    // 4
    { 0x2000, { 0x403, 10800, 0 } },                    // 5
    { 0x2000, { 0x404, 10800, 0 } },                    // 6
    { 0xa000, { DFF_Prop_adjustValue, 0, 0x405 } },     // 7
    { 0xa000, { DFF_Prop_adjust2Value,0, 0x406 } },     // 8
    { 0x2001, { 0x407, 1, 3 } },                        // 9
    { 0x2001, { 0x408, 1, 3 } },                        // 0xa
    { 0x2001, { 0x407, 2, 3 } },                        // 0xb
    { 0x2001, { 0x408, 2, 3 } },                        // 0xc
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },         // 0xd
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },        // 0xe
    { 0x2001, { 0x403, 1, 10800 / 900 } },              // 0xf  taking half x distance of the radius from the first bobble
    { 0x2001, { 0x404, 1, 10800 / 900 } },              // 0x10
    { 0xe000, { 0x409, 0x405, 0x40f } },                // 0x11
    { 0xe000, { 0x40a, 0x406, 0x410 } },                // 0x12
    { 0x6000, { 0x40b, 0x405, 0 } },                    // 0x13
    { 0x6000, { 0x40c, 0x406, 0 } }                     // 0x14
};
const sal_Int32 mso_sptCloudCalloutDefault[] =
{
    2, 1350, 25920
};
const SvxMSDffTextRectangles mso_sptCloudCalloutTextRect[] =
{
    { { 3000, 3320 }, { 17110, 17330 } }
};
const mso_CustomShape msoCloudCallout =
{
    std::span<const SvxMSDffVertPair>(mso_sptCloudCalloutVert),
    const_cast<sal_uInt16*>(mso_sptCloudCalloutSegm), sizeof( mso_sptCloudCalloutSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCloudCalloutCalc),
    const_cast<sal_Int32*>(mso_sptCloudCalloutDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptCloudCalloutTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle),
};

const SvxMSDffVertPair mso_sptWaveVert[] =   // adjustment1 : 0 - 4460
{                                                   // adjustment2 : 8640 - 12960
    { 7 MSO_I, 0 MSO_I }, { 15 MSO_I, 9 MSO_I }, { 16 MSO_I, 10 MSO_I }, { 12 MSO_I, 0 MSO_I },
    { 24 MSO_I, 1 MSO_I }, { 25 MSO_I, 26 MSO_I }, { 27 MSO_I, 28 MSO_I }, { 29 MSO_I, 1 MSO_I }
};
const SvxMSDffCalculationData mso_sptWaveCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } }, //400 (vert.adj)
    { 0x8000, { 21600, 0, 0x400 } },            //401
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },//402 (horz.adj)
    { 0x2000, { 0x402, 0, 10800 } },            //403 -2160 -> 2160 (horz.adj)
    { 0x2001, { 0x403, 2, 1 } },                //404 -4320 -> 4320 (horz.adj)
    { 0x2003, { 0x404, 0, 0 } },                //405 abs( 0x404 )  (horz.adj)
    { 0x8000, { 4320, 0, 0x405 } },             //406
    { 0xa006, { 0x403, 0, 0x405 } },            //407
    { 0x4001, { 15800, 0x400, 4460 } },         //408 0 -> 15800    (vert.adj)
    { 0xa000, { 0x400, 0, 0x408 } },            //409
    { 0x6000, { 0x400, 0x408, 0 } },            //40a
    { 0x8000, { 21600, 0, 0x404 } },            //40b
    { 0x6006, { 0x403, 0x40b, 21600 } },        //40c
    { 0xa000, { 0x40c, 0, 0x407 } },            //40d width between p0 and p1
    { 0x2001, { 0x405, 1, 2 } },                //40e
    { 0xa000, { 0x407, 7200, 0x40e } },         //40f
    { 0x6000, { 0x40c, 0x40e, 7200 } },         //410
    { 0x2001, { 0x40d, 1, 2 } },                //411 1/2 width
    { 0x6000, { 0x407, 0x411, 0 } },            //412 top center glue xpos
    { 0x8000, { 21600, 0, 0x412 } },            //413 bottom center glue xpos
    { 0x2001, { 0x405, 1, 2 } },                //414 left glue x pos
    { 0x8000, { 21600, 0, 0x414 } },            //415 right glue x pos
    { 0x2001, { 0x400, 2, 1 } },                //416 y1 (textbox)
    { 0x8000, { 21600, 0, 0x416 } },            //417 y2 (textbox)

    { 0x8000, { 21600, 0, 0x407 } },            //418 p2

    { 0x8000, { 21600, 0, 0x40f } },            //419 c
    { 0x6000, { 0x401, 0x408, 0 } },            //41a

    { 0x8000, { 21600, 0, 0x410 } },            //41b c
    { 0xa000, { 0x401, 0, 0x408 } },            //41c

    { 0x8000, { 21600, 0, 0x40c } }             //41d p3
};
const SvxMSDffVertPair mso_sptWaveGluePoints[] =
{
    { 0x12 MSO_I, 0 MSO_I }, { 0x14 MSO_I, 10800 }, { 0x13 MSO_I, 1 MSO_I }, { 0x15 MSO_I, 10800 }
};
const sal_uInt16 mso_sptWaveSegm[] =
{
    0x4000, 0x2001, 0x0001, 0x2001, 0x6000, 0x8000
};
const SvxMSDffHandle mso_sptWaveHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 4460 },
    {   SvxMSDffHandleFlags::RANGE,
        0x101, 21600, 10800, 10800, 8640, 12960, MIN_INT32, 0x7fffffff }
};
const sal_Int32 mso_sptWaveDefault[] =
{
    2, 1400, 10800
};
const SvxMSDffTextRectangles mso_sptWaveTextRect[] =
{
    { { 5 MSO_I, 22 MSO_I }, { 11 MSO_I, 23 MSO_I } }
};
const mso_CustomShape msoWave =
{
    std::span<const SvxMSDffVertPair>(mso_sptWaveVert),
    const_cast<sal_uInt16*>(mso_sptWaveSegm), sizeof( mso_sptWaveSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptWaveCalc),
    const_cast<sal_Int32*>(mso_sptWaveDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptWaveTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptWaveGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptWaveHandle),
};

const SvxMSDffVertPair mso_sptDoubleWaveVert[] = // adjustment1 : 0 - 2230
{                                                       // adjustment2 : 8640 - 12960
    { 7 MSO_I, 0 MSO_I }, { 15 MSO_I, 9 MSO_I }, { 0x1f MSO_I, 10 MSO_I }, { 0x12 MSO_I, 0 MSO_I }, { 0x1e MSO_I, 9 MSO_I }, { 16 MSO_I, 10 MSO_I }, { 12 MSO_I, 0 MSO_I },
    { 24 MSO_I, 1 MSO_I }, { 25 MSO_I, 26 MSO_I }, { 0x20 MSO_I, 28 MSO_I }, { 0x13 MSO_I, 1 MSO_I }, { 0x21 MSO_I, 26 MSO_I }, { 27 MSO_I, 28 MSO_I }, { 29 MSO_I, 1 MSO_I }
};
const SvxMSDffCalculationData mso_sptDoubleWaveCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } }, //400 (vert.adj)
    { 0x8000, { 21600, 0, 0x400 } },            //401
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },//402 (horz.adj)
    { 0x2000, { 0x402, 0, 10800 } },            //403 -2160 -> 2160 (horz.adj)
    { 0x2001, { 0x403, 2, 1 } },                //404 -4320 -> 4320 (horz.adj)
    { 0x2003, { 0x404, 0, 0 } },                //405 abs( 0x404 )  (horz.adj)
    { 0x8000, { 4320, 0, 0x405 } },             //406 -> not used
    { 0xa006, { 0x403, 0, 0x405 } },            //407
    { 0x4001, { 7900, 0x400, 2230 } },          //408 0 -> 7900 (vert.adj)
    { 0xa000, { 0x400, 0, 0x408 } },            //409
    { 0x6000, { 0x400, 0x408, 0 } },            //40a
    { 0x8000, { 21600, 0, 0x404 } },            //40b
    { 0x6006, { 0x403, 0x40b, 21600 } },        //40c
    { 0xa000, { 0x40c, 0, 0x407 } },            //40d width between p0 and p1
    { 0x2001, { 0x405, 1, 2 } },                //40e
    { 0xa000, { 0x407, 3600, 0x40e } },         //40f
    { 0x6000, { 0x40c, 0x40e, 3600 } },         //410
    { 0x2001, { 0x40d, 1, 2 } },                //411 1/2 width
    { 0x6000, { 0x407, 0x411, 0 } },            //412 top center glue xpos
    { 0x8000, { 21600, 0, 0x412 } },            //413 bottom center glue xpos
    { 0x2001, { 0x405, 1, 2 } },                //414 left glue x pos
    { 0x8000, { 21600, 0, 0x414 } },            //415 right glue x pos
    { 0x2001, { 0x400, 2, 1 } },                //416 y1 (textbox)
    { 0x8000, { 21600, 0, 0x416 } },            //417 y2 (textbox)

    { 0x8000, { 21600, 0, 0x407 } },            //418 p2

    { 0x8000, { 21600, 0, 0x40f } },            //419 c
    { 0x6000, { 0x401, 0x408, 0 } },            //41a

    { 0x8000, { 21600, 0, 0x410 } },            //41b c
    { 0xa000, { 0x401, 0, 0x408 } },            //41c

    { 0x8000, { 21600, 0, 0x40c } },            //41d p3
    { 0xa000, { 0x412, 3600, 0x40e } },         //41e
    { 0x6000, { 0x412, 0x40e, 3600 } },         //41f
    { 0xa000, { 0x413, 3600, 0x40e } },         //420
    { 0x6000, { 0x413, 0x40e, 3600 } }          //421
};
const SvxMSDffVertPair mso_sptDoubleWaveGluePoints[] =
{
    { 0x12 MSO_I, 0 MSO_I }, { 0x14 MSO_I, 10800 }, { 0x13 MSO_I, 1 MSO_I }, { 0x15 MSO_I, 10800 }
};
const sal_uInt16 mso_sptDoubleWaveSegm[] =
{
    0x4000, 0x2002, 0x0001, 0x2002, 0x6000, 0x8000
};
const SvxMSDffHandle mso_sptDoubleWaveHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 2230 },
    {   SvxMSDffHandleFlags::RANGE,
        0x101, 21600, 10800, 10800, 8640, 12960, MIN_INT32, 0x7fffffff }
};
const sal_Int32 mso_sptDoubleWaveDefault[] =
{
    2, 1400, 10800
};
const SvxMSDffTextRectangles mso_sptDoubleWaveTextRect[] =
{
    { { 5 MSO_I, 22 MSO_I }, { 11 MSO_I, 23 MSO_I } }
};
const mso_CustomShape msoDoubleWave =
{
    std::span<const SvxMSDffVertPair>(mso_sptDoubleWaveVert),
    const_cast<sal_uInt16*>(mso_sptDoubleWaveSegm), sizeof( mso_sptDoubleWaveSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptDoubleWaveCalc),
    const_cast<sal_Int32*>(mso_sptDoubleWaveDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptDoubleWaveTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptDoubleWaveGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptDoubleWaveHandle),
};

// for each shapetype a bit of 1 is indicating that the shape is NOT filled by default
const sal_uInt16 mso_DefaultFillingTable[] =
{
    0x0000, 0x0018, 0x01ff, 0x0000, 0x0c00, 0x01e0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0600, 0x0000, 0x0000, 0x0000, 0x0000
};
bool IsCustomShapeFilledByDefault( MSO_SPT eSpType )
{
    bool bIsFilledByDefault = true;
    sal_uInt32 i = static_cast<sal_uInt32>(eSpType);
    if ( i < 0x100 )
        bIsFilledByDefault = ( mso_DefaultFillingTable[ i >> 4 ] & ( 1 << ( i & 0xf ) ) ) == 0;
    return bIsFilledByDefault;
}
sal_Int16 GetCustomShapeConnectionTypeDefault( MSO_SPT eSpType )
{
    sal_Int16 nGluePointType = css::drawing::EnhancedCustomShapeGluePointType::SEGMENTS;
    const mso_CustomShape* pDefCustomShape = GetCustomShapeContent( eSpType );
    if (pDefCustomShape && !pDefCustomShape->pGluePoints.empty())
        nGluePointType = css::drawing::EnhancedCustomShapeGluePointType::CUSTOM;
    else
    {
        switch( eSpType )
        {
            case mso_sptRectangle :
            case mso_sptRoundRectangle :
            case mso_sptPictureFrame :
            case mso_sptFlowChartProcess :
            case mso_sptFlowChartPredefinedProcess :
            case mso_sptFlowChartInternalStorage :
            case mso_sptTextPlainText :
            case mso_sptTextBox :
            case mso_sptVerticalScroll :
            case mso_sptHorizontalScroll :
                nGluePointType = css::drawing::EnhancedCustomShapeGluePointType::RECT;
                break;
            default: break;
        }
    }
    return nGluePointType;
}

// for each shapetype a bit of 1 is indicating that the shape is NOT stroked by default
// #i28269#
const sal_uInt16 mso_DefaultStrokingTable[] =
{
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0800, 0x0000, 0x0000, 0x0000, // #i28269# Added shape 75 (mso_sptPictureFrame)
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000
};
// #i28269#
bool IsCustomShapeStrokedByDefault( MSO_SPT eSpType )
{
    bool bIsStrokedByDefault = true;
    sal_uInt32 i = static_cast<sal_uInt32>(eSpType);
    if ( i < 0x100 )
        bIsStrokedByDefault = ( mso_DefaultStrokingTable[ i >> 4 ] & ( 1 << ( i & 0xf ) ) ) == 0;
    return bIsStrokedByDefault;
}

const sal_uInt16 msoSortFilledObjectsToBackTable[] =
{
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0010, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};
bool SortFilledObjectsToBackByDefault( MSO_SPT eSpType )
{
    bool bSortFilledObjectsToBackByDefault = true;
    sal_uInt32 i = static_cast<sal_uInt32>(eSpType);
    if ( i < 0x100 )
        bSortFilledObjectsToBackByDefault = ( msoSortFilledObjectsToBackTable[ i >> 4 ] & ( 1 << ( i & 0xf ) ) ) != 0;
    return bSortFilledObjectsToBackByDefault;
}

const SvxMSDffTextRectangles mso_sptFontWorkTextRect[] =
{
    { { 0, 0 }, { 21600, 21600 } }
};

const SvxMSDffVertPair mso_sptTextPlainTextVert[] =
{
    { 3 MSO_I, 0 }, { 5 MSO_I, 0 }, { 6 MSO_I, 21600 }, { 7 MSO_I, 21600 }
};
const SvxMSDffCalculationData mso_sptTextPlainTextCalc[] =   // adjustment1 : 6629 - 14971
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 10800 } },
    { 0x2001, { 0x400, 2, 1 } },
    { 0x2003, { 0x401, 0, 0 } },
    { 0xa006, { 0x401, 0, 0x402 } },        // x1(3)
    { 0x8000, { 21600, 0, 0x402 } },
    { 0x6006, { 0x401, 0x404, 21600 } },    // x2(5)
    { 0x6006, { 0x401, 0x402, 0 } },        // x2
    { 0xa006, { 0x401, 21600, 0x404 } }     // x3(7)
};
const sal_uInt16 mso_sptTextPlainTextSegm[] =
{
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffHandle mso_sptTextPlainTextHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 21600, 10800, 10800, 6629, 14971, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoTextPlainText =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextPlainTextVert),
    const_cast<sal_uInt16*>(mso_sptTextPlainTextSegm), sizeof( mso_sptTextPlainTextSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextPlainTextCalc),
    const_cast<sal_Int32*>(mso_sptDefault10800),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextPlainTextHandle),
};

const SvxMSDffVertPair mso_sptTextStopVert[] =
{
    { 0, 0 MSO_I }, { 7200, 0 }, { 14400, 0 }, { 21600, 0 MSO_I },
    { 0, 1 MSO_I }, { 7200, 21600 }, { 14400, 21600 }, { 21600, 1 MSO_I }
};
const SvxMSDffCalculationData mso_sptTextStopCalc[] =    // adjustment1 : 3080 - 10800
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } }
};
const sal_uInt16 mso_sptTextStopSegm[] =
{
    0x4000, 0x0003, 0x8000,
    0x4000, 0x0003, 0x8000
};
const sal_Int32 mso_sptTextStopDefault[] =
{
    1, 2700
};
const SvxMSDffHandle mso_sptTextStopHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 3080, 10800 }
};
const mso_CustomShape msoTextStop =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextStopVert),
    const_cast<sal_uInt16*>(mso_sptTextStopSegm), sizeof( mso_sptTextStopSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextStopCalc),
    const_cast<sal_Int32*>(mso_sptTextStopDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextStopHandle),
};

const SvxMSDffVertPair mso_sptTextTriangleVert[] =
{
    { 0, 0 MSO_I }, { 10800, 0 }, { 21600, 0 MSO_I }, { 0, 21600 }, { 21600, 21600 }
};
const SvxMSDffCalculationData mso_sptTextTriangleCalc[] =    // adjustment1 : 6629 - 14971
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } }
};
const sal_uInt16 mso_sptTextTriangleSegm[] =
{
    0x4000, 0x0002, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffHandle mso_sptTextTriangleHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 21600 }
};
const mso_CustomShape msoTextTriangle =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextTriangleVert),
    const_cast<sal_uInt16*>(mso_sptTextTriangleSegm), sizeof( mso_sptTextTriangleSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextTriangleCalc),
    const_cast<sal_Int32*>(mso_sptDefault10800),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextTriangleHandle),
};
const SvxMSDffVertPair mso_sptTextTriangleInvertedVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 0, 0 MSO_I }, { 10800, 21600 }, { 21600, 0 MSO_I }
};
const sal_uInt16 mso_sptTextTriangleInvertedSegm[] =
{
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0002, 0x8000
};
const mso_CustomShape msoTextTriangleInverted =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextTriangleInvertedVert),
    const_cast<sal_uInt16*>(mso_sptTextTriangleInvertedSegm), sizeof( mso_sptTextTriangleInvertedSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextTriangleCalc),
    const_cast<sal_Int32*>(mso_sptDefault10800),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextTriangleHandle),
};

const SvxMSDffVertPair mso_sptTextChevronVert[] =
{
    { 0, 0 MSO_I }, { 10800, 0 }, { 21600, 0 MSO_I }, { 0, 21600 }, { 10800, 1 MSO_I }, { 21600, 21600 }
};
const SvxMSDffCalculationData mso_sptTextChevronCalc[] = // adjustment1 : 6629 - 14971
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } }
};
const sal_uInt16 mso_sptTextChevronSegm[] =
{
    0x4000, 0x0002, 0x8000,
    0x4000, 0x0002, 0x8000
};
const SvxMSDffHandle mso_sptTextChevronHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 10800 }
};
const mso_CustomShape msoTextChevron =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextChevronVert),
    const_cast<sal_uInt16*>(mso_sptTextChevronSegm), sizeof( mso_sptTextChevronSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextChevronCalc),
    const_cast<sal_Int32*>(mso_sptDefault5400),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextChevronHandle),
};

const SvxMSDffVertPair mso_sptTextChevronInvertedVert[] =
{
    { 0, 0 }, { 10800, 1 MSO_I }, { 21600, 0 }, { 0, 0 MSO_I }, { 10800, 21600 }, { 21600, 0 MSO_I }
};
const SvxMSDffCalculationData mso_sptTextChevronInvertedCalc[] = // adjustment1 : 6629 - 14971
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } }
};
const sal_uInt16 mso_sptTextChevronInvertedSegm[] =
{
    0x4000, 0x0002, 0x8000,
    0x4000, 0x0002, 0x8000
};
const SvxMSDffHandle mso_sptTextChevronInvertedHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 10800, 21600 }
};
const mso_CustomShape msoTextChevronInverted =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextChevronInvertedVert),
    const_cast<sal_uInt16*>(mso_sptTextChevronInvertedSegm), sizeof( mso_sptTextChevronInvertedSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextChevronInvertedCalc),
    const_cast<sal_Int32*>(mso_sptDefault16200),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextChevronInvertedHandle),
};
//V 0 0 21600 ?f2 0 ?f0 21600 ?f0
//W 0 0 21600 ?f2 21600 ?f0 0 ?f0 N
//V 0 ?f3 21600 21600 0 ?f1 21600 ?f1
//W 0 ?f3 21600 21600 21600 ?f1 0 ?f1 N
//mso_sptTextRingInside
const SvxMSDffVertPair mso_sptTextRingInsideVert[] =
{
    { 0, 0 }, { 21600, 2 MSO_I }, { 0, 0 MSO_I },{ 21600, 0 MSO_I },//V
    { 0, 0 }, { 21600, 2 MSO_I }, { 21600, 0 MSO_I },{ 0, 0 MSO_I },//W
    { 0, 3 MSO_I }, { 21600, 21600 }, { 0, 1 MSO_I },{ 21600, 1 MSO_I },//V
    { 0, 3 MSO_I }, { 21600, 21600 }, { 21600, 1 MSO_I },{ 0, 1 MSO_I }//W
};
const SvxMSDffCalculationData mso_sptTextRingInsideCalc[] =  // adjustment1 : 6629 - 14971
{
    { 0x2001, { DFF_Prop_adjustValue, 1, 2 } },
    { 0x8000, { 21600, 0, 0x400 } },
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } }, //$0
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } }//21600-$0
};
const sal_uInt16 mso_sptTextRingInsideSegm[] =
{
    0xa604, 0xa504,0x8000,
    0xa604, 0xa504,0x8000
};
const SvxMSDffHandle mso_sptTextRingInsideHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE| SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL,
        10800, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 10800, 21600 }
};
const mso_CustomShape msoTextRingInside =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextRingInsideVert),
    const_cast<sal_uInt16*>(mso_sptTextRingInsideSegm), sizeof( mso_sptTextRingInsideSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextRingInsideCalc),
    const_cast<sal_Int32*>(mso_sptDefault13500),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextRingInsideHandle),
};
//mso_sptTextRingOutside
//path = U 10800 ?f0 10800 ?f2 180 539 N U 10800 ?f1 10800 ?f2 180 539 N
// MSO binary format has swing angle, not end angle, therefore 359 instead of 539.
const SvxMSDffVertPair mso_sptTextRingOutsideVert[] =
{
    { 10800, 0 MSO_I }, { 10800, 0 MSO_I }, { 180, 359 },
    { 10800, 1 MSO_I }, { 10800, 0 MSO_I }, { 180, 359 }
};
const SvxMSDffCalculationData mso_sptTextRingOutsideCalc[] = // adjustment1 : 6629 - 14971
{
    { 0x2001, { DFF_Prop_adjustValue, 1, 2 } },
    { 0x8000, { 21600, 0, 0x400 } }
};
const sal_uInt16 mso_sptTextRingOutsideSegm[] =
{
    0xA203, 0x8000,
    0xA203, 0x8000
};
const SvxMSDffHandle mso_sptTextRingOutsideHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        10800, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 10800, 21600 }
};
const mso_CustomShape msoTextRingOutside =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextRingOutsideVert),
    const_cast<sal_uInt16*>(mso_sptTextRingOutsideSegm), sizeof( mso_sptTextRingOutsideSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextRingOutsideCalc),
    const_cast<sal_Int32*>(mso_sptDefault13500),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextRingOutsideHandle),
};

const SvxMSDffVertPair mso_sptTextFadeRightVert[] =
{
    { 0, 0 }, { 21600, 0 MSO_I }, { 0, 21600 }, { 21600, 1 MSO_I }
};
const SvxMSDffCalculationData mso_sptTextFadeCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } }
};
const sal_uInt16 mso_sptTextFadeSegm[] =
{
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffHandle mso_sptTextFadeRightHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        21600, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 10800 }
};
const mso_CustomShape msoTextFadeRight =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextFadeRightVert),
    const_cast<sal_uInt16*>(mso_sptTextFadeSegm), sizeof( mso_sptTextFadeSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextFadeCalc),
    const_cast<sal_Int32*>(mso_sptDefault7200),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextFadeRightHandle),
};

const SvxMSDffVertPair mso_sptTextFadeLeftVert[] =
{
    { 0, 0 MSO_I }, { 21600, 0 }, { 0, 1 MSO_I }, { 21600, 21600 }
};
const SvxMSDffHandle mso_sptTextFadeLeftHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 10800 }
};
const mso_CustomShape msoTextFadeLeft =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextFadeLeftVert),
    const_cast<sal_uInt16*>(mso_sptTextFadeSegm), sizeof( mso_sptTextFadeSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextFadeCalc),
    const_cast<sal_Int32*>(mso_sptDefault7200),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextFadeLeftHandle),
};

const SvxMSDffVertPair mso_sptTextFadeUpVert[] =
{
    { 0 MSO_I, 0 }, { 1 MSO_I, 0 }, { 0, 21600 }, { 21600, 21600 }
};
const SvxMSDffHandle mso_sptTextFadeUpHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 0, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoTextFadeUp =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextFadeUpVert),
    const_cast<sal_uInt16*>(mso_sptTextFadeSegm), sizeof( mso_sptTextFadeSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextFadeCalc),
    const_cast<sal_Int32*>(mso_sptDefault7200),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextFadeUpHandle),
};

const SvxMSDffVertPair mso_sptTextFadeDownVert[] =
{
    { 0, 0 }, { 21600, 0 }, { 0 MSO_I, 21600 }, { 1 MSO_I, 21600 }
};
const SvxMSDffHandle mso_sptTextFadeDownHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 21600, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoTextFadeDown =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextFadeDownVert),
    const_cast<sal_uInt16*>(mso_sptTextFadeSegm), sizeof( mso_sptTextFadeSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextFadeCalc),
    const_cast<sal_Int32*>(mso_sptDefault7200),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextFadeDownHandle),
};

const SvxMSDffVertPair mso_sptTextSlantUpVert[] =
{
    { 0, 0 MSO_I }, { 21600, 0 }, { 0, 21600 }, { 21600, 1 MSO_I }
};
const SvxMSDffHandle mso_sptTextSlantUpHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 15400 }
};
const mso_CustomShape msoTextSlantUp =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextSlantUpVert),
    const_cast<sal_uInt16*>(mso_sptTextFadeSegm), sizeof( mso_sptTextFadeSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextFadeCalc),
    const_cast<sal_Int32*>(mso_sptDefault12000),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextSlantUpHandle),
};

const SvxMSDffVertPair mso_sptTextSlantDownVert[] =
{
    { 0, 0 }, { 21600, 1 MSO_I }, { 0, 0 MSO_I }, { 21600, 21600 }
};
const SvxMSDffHandle mso_sptTextSlantDownHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 6200, 21600 }
};
const mso_CustomShape msoTextSlantDown =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextSlantDownVert),
    const_cast<sal_uInt16*>(mso_sptTextFadeSegm), sizeof( mso_sptTextFadeSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextFadeCalc),
    const_cast<sal_Int32*>(mso_sptDefault12000),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextSlantDownHandle),
};

const SvxMSDffVertPair mso_sptTextCascadeUpVert[] =
{
    { 0, 2 MSO_I }, { 21600, 0 }, { 0, 21600 }, { 21600, 0 MSO_I }
};
const SvxMSDffCalculationData mso_sptTextCascadeCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } },
    { 0x2001, { 0x401, 1, 4 } }
};
const SvxMSDffHandle mso_sptTextCascadeUpHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        21600, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 6200, 21600 }
};
const mso_CustomShape msoTextCascadeUp =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextCascadeUpVert),
    const_cast<sal_uInt16*>(mso_sptTextFadeSegm), sizeof( mso_sptTextFadeSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextCascadeCalc),
    const_cast<sal_Int32*>(mso_sptDefault9600),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextCascadeUpHandle),
};

const SvxMSDffVertPair mso_sptTextCascadeDownVert[] =
{
    { 0, 0 }, { 21600, 2 MSO_I }, { 0, 0 MSO_I }, { 21600, 21600 }
};
const SvxMSDffHandle mso_sptTextCascadeDownHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 6200, 21600 }
};
const mso_CustomShape msoTextCascadeDown =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextCascadeDownVert),
    const_cast<sal_uInt16*>(mso_sptTextFadeSegm), sizeof( mso_sptTextFadeSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextCascadeCalc),
    const_cast<sal_Int32*>(mso_sptDefault9600),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextCascadeDownHandle),
};

const SvxMSDffVertPair mso_sptTextArchUpCurveVert[] =
{
    { 0, 0 }, { 21600, 21600 }, { 2 MSO_I, 3 MSO_I }, { 4 MSO_I, 3 MSO_I }
};
const SvxMSDffCalculationData mso_sptTextArchCurveCalc[] =
{
    { 0x400a, { 10800, DFF_Prop_adjustValue, 0 } },
    { 0x4009, { 10800, DFF_Prop_adjustValue, 0 } },
    { 0x2000, { 0x400, 10800, 0 } },
    { 0x2000, { 0x401, 10800, 0 } },
    { 0x8000, { 21600, 0, 0x402 } }
};
const sal_uInt16 mso_sptTextArchUpCurveSegm[] =
{
    0xA504, 0x8000      // clockwise arc
};
const SvxMSDffHandle mso_sptTextArchUpCurveHandle[] =
{
    {   SvxMSDffHandleFlags::POLAR,
        10800, 0x100, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const sal_Int32 mso_sptTextArchUpCurveDefault[] =
{
    1, 180
};
const mso_CustomShape msoTextArchUpCurve =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextArchUpCurveVert),
    const_cast<sal_uInt16*>(mso_sptTextArchUpCurveSegm), sizeof( mso_sptTextArchUpCurveSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextArchCurveCalc),
    const_cast<sal_Int32*>(mso_sptTextArchUpCurveDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextArchUpCurveHandle),
};

const SvxMSDffVertPair mso_sptTextArchDownCurveVert[] =
{
    { 0, 0 }, { 21600, 21600 }, { 4 MSO_I, 3 MSO_I }, { 2 MSO_I, 3 MSO_I }
};
const sal_uInt16 mso_sptTextArchDownCurveSegm[] =
{
    0xA304, 0x8000      // counter clockwise arc to
};
const SvxMSDffHandle mso_sptTextArchDownCurveHandle[] =
{
    {   SvxMSDffHandleFlags::POLAR,
        10800, 0x100, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const sal_Int32 mso_sptTextArchDownCurveDefault[] =
{
    1, 0
};
const mso_CustomShape msoTextArchDownCurve =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextArchDownCurveVert),
    const_cast<sal_uInt16*>(mso_sptTextArchDownCurveSegm), sizeof( mso_sptTextArchDownCurveSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextArchCurveCalc),
    const_cast<sal_Int32*>(mso_sptTextArchDownCurveDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextArchDownCurveHandle),
};

const SvxMSDffVertPair mso_sptTextCircleCurveVert[] =
{
    { 0, 0 }, { 21600, 21600 }, { 2 MSO_I, 3 MSO_I }, { 2 MSO_I, 4 MSO_I }
};
const SvxMSDffCalculationData mso_sptTextCircleCurveCalc[] =
{
    { 0x400a, { 10800, DFF_Prop_adjustValue, 0 } },
    { 0x4009, { 10800, DFF_Prop_adjustValue, 0 } },
    { 0x2000, { 0x400, 10800, 0 } },
    { 0x2000, { 0x401, 10800, 0 } },
    { 0x8000, { 21600, 0, 0x403 } }
};
const sal_uInt16 mso_sptTextCircleCurveSegm[] =
{
    0xA504, 0x8000      // clockwise arc to
};
const SvxMSDffHandle mso_sptTextCircleCurveHandle[] =
{
    {   SvxMSDffHandleFlags::POLAR,
        10800, 0x100, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const sal_Int32 mso_sptTextCircleCurveDefault[] =
{
    1, -179
};
const mso_CustomShape msoTextCircleCurve =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextCircleCurveVert),
    const_cast<sal_uInt16*>(mso_sptTextCircleCurveSegm), sizeof( mso_sptTextCircleCurveSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextCircleCurveCalc),
    const_cast<sal_Int32*>(mso_sptTextCircleCurveDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextCircleCurveHandle),
};

const SvxMSDffVertPair mso_sptTextButtonCurveVert[] =
{
    { 0, 0 }, { 21600, 21600 }, { 2 MSO_I, 3 MSO_I }, { 4 MSO_I, 3 MSO_I },
    { 0, 10800 }, { 21600, 10800 },
    { 0, 0 }, { 21600, 21600 }, { 2 MSO_I, 5 MSO_I }, { 4 MSO_I, 5 MSO_I }
};
const SvxMSDffCalculationData mso_sptTextButtonCurveCalc[] =
{
    { 0x400a, { 10800, DFF_Prop_adjustValue, 0 } },
    { 0x4009, { 10800, DFF_Prop_adjustValue, 0 } },
    { 0x2000, { 0x400, 10800, 0 } },
    { 0x2000, { 0x401, 10800, 0 } },
    { 0x8000, { 21600, 0, 0x402 } },
    { 0x8000, { 21600, 0, 0x403 } }
};
const sal_uInt16 mso_sptTextButtonCurveSegm[] =
{
    0xA504, 0x8000,     // clockwise arc
    0x4000, 0x0001, 0x8000,
    0xA304, 0x8000      // counter clockwise
};
const SvxMSDffHandle mso_sptTextButtonCurveHandle[] =
{
    {   SvxMSDffHandleFlags::POLAR,
        10800, 0x100, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const sal_Int32 mso_sptTextButtonCurveDefault[] =
{
    1, 180
};
const mso_CustomShape msoTextButtonCurve =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextButtonCurveVert),
    const_cast<sal_uInt16*>(mso_sptTextButtonCurveSegm), sizeof( mso_sptTextButtonCurveSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextButtonCurveCalc),
    const_cast<sal_Int32*>(mso_sptTextButtonCurveDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextButtonCurveHandle),
};

const SvxMSDffVertPair mso_sptTextArchUpPourVert[] =
{
    { 0, 0 }, { 21600, 21600 }, { 2 MSO_I, 3 MSO_I }, { 4 MSO_I, 3 MSO_I },
    { 5 MSO_I, 5 MSO_I }, { 11 MSO_I, 11 MSO_I }, { 8 MSO_I, 9 MSO_I }, { 0xa MSO_I, 9 MSO_I }
};
const SvxMSDffCalculationData mso_sptTextArchPourCalc[] =
{
    { 0x400a, { 10800, DFF_Prop_adjustValue, 0 } },
    { 0x4009, { 10800, DFF_Prop_adjustValue, 0 } },
    { 0x2000, { 0x400, 10800, 0 } },
    { 0x2000, { 0x401, 10800, 0 } },
    { 0x8000, { 21600, 0, 0x402 } },
    { 0x8000, { 10800, 0, DFF_Prop_adjust2Value } },
    { 0x600a, { 0x405, DFF_Prop_adjustValue, 0 } }, // 6
    { 0x6009, { 0x405, DFF_Prop_adjustValue, 0 } },
    { 0x2000, { 0x406, 10800, 0 } },                // 8
    { 0x2000, { 0x407, 10800, 0 } },
    { 0x8000, { 21600, 0, 0x408 } },                // 10
    { 0x8000, { 21600, 0, 0x405 } }
};
const sal_uInt16 mso_sptTextArchUpPourSegm[] =
{
    0xA504, 0x8000, 0xA504, 0x8000
};
const SvxMSDffHandle mso_sptTextArchPourHandle[] =
{
    {   SvxMSDffHandleFlags::POLAR | SvxMSDffHandleFlags::RADIUS_RANGE,
        0x101, 0x100, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const sal_Int32 mso_sptTextArchUpPourDefault[] =
{
    2, 180, 5400
};
const mso_CustomShape msoTextArchUpPour =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextArchUpPourVert),
    const_cast<sal_uInt16*>(mso_sptTextArchUpPourSegm), sizeof( mso_sptTextArchUpPourSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextArchPourCalc),
    const_cast<sal_Int32*>(mso_sptTextArchUpPourDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextArchPourHandle),
};

const SvxMSDffVertPair mso_sptTextArchDownPourVert[] =
{
    { 5 MSO_I, 5 MSO_I }, { 11 MSO_I, 11 MSO_I }, { 0xa MSO_I, 9 MSO_I }, { 8 MSO_I, 9 MSO_I },
    { 0, 0 }, { 21600, 21600 }, { 4 MSO_I, 3 MSO_I }, { 2 MSO_I, 3 MSO_I }
};
const sal_uInt16 mso_sptTextArchDownPourSegm[] =
{
    0xA304, 0x8000, 0xA304, 0x8000
};
const sal_Int32 mso_sptTextArchDownPourDefault[] =
{
    2, 0, 5400
};
const mso_CustomShape msoTextArchDownPour =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextArchDownPourVert),
    const_cast<sal_uInt16*>(mso_sptTextArchDownPourSegm), sizeof( mso_sptTextArchDownPourSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextArchPourCalc),
    const_cast<sal_Int32*>(mso_sptTextArchDownPourDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextArchPourHandle),
};

const SvxMSDffVertPair mso_sptTextCirclePourVert[] =
{
    { 0, 0 }, { 21600, 21600 }, { 2 MSO_I, 3 MSO_I }, { 2 MSO_I, 4 MSO_I },
    { 5 MSO_I, 5 MSO_I }, { 11 MSO_I, 11 MSO_I }, { 8 MSO_I, 9 MSO_I }, { 8 MSO_I, 0xa MSO_I }
};
const SvxMSDffCalculationData mso_sptTextCirclePourCalc[] =
{
    { 0x400a, { 10800, DFF_Prop_adjustValue, 0 } },
    { 0x4009, { 10800, DFF_Prop_adjustValue, 0 } },
    { 0x2000, { 0x400, 10800, 0 } },
    { 0x2000, { 0x401, 10800, 0 } },
    { 0x8000, { 21600, 0, 0x403 } },
    { 0x8000, { 10800, 0, DFF_Prop_adjust2Value } },
    { 0x600a, { 0x405, DFF_Prop_adjustValue, 0 } }, // 6
    { 0x6009, { 0x405, DFF_Prop_adjustValue, 0 } },
    { 0x2000, { 0x406, 10800, 0 } },                // 8
    { 0x2000, { 0x407, 10800, 0 } },
    { 0x8000, { 21600, 0, 0x409 } },                // 10
    { 0x8000, { 21600, 0, 0x405 } },
    { 0x000, { 21600, 0, 0 } }
};
const sal_uInt16 mso_sptTextCirclePourSegm[] =
{
    0xA504, 0x8000, 0xA504, 0x8000
};
const SvxMSDffHandle mso_sptTextCirclePourHandle[] =
{
    {   SvxMSDffHandleFlags::POLAR | SvxMSDffHandleFlags::RADIUS_RANGE,
        0x101, 0x100, 10800, 10800, 0, 10800, MIN_INT32, 0x7fffffff }
};
const sal_Int32 mso_sptTextCirclePourDefault[] =
{
    2,  -179, 5400
};
const mso_CustomShape msoTextCirclePour =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextCirclePourVert),
    const_cast<sal_uInt16*>(mso_sptTextCirclePourSegm), sizeof( mso_sptTextCirclePourSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextCirclePourCalc),
    const_cast<sal_Int32*>(mso_sptTextCirclePourDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextCirclePourHandle),
};

const SvxMSDffVertPair mso_sptTextButtonPourVert[] =
{
    { 0, 0 }, { 21600, 21600 }, { 2 MSO_I, 3 MSO_I }, { 4 MSO_I, 3 MSO_I },
    { 6 MSO_I, 6 MSO_I }, { 7 MSO_I, 7 MSO_I }, { 10 MSO_I, 11 MSO_I }, { 12 MSO_I, 11 MSO_I },
    { 0x16 MSO_I, 16 MSO_I }, { 0x15 MSO_I, 16 MSO_I },
    { 0x16 MSO_I, 15 MSO_I }, { 0x15 MSO_I, 15 MSO_I },
    { 6 MSO_I, 6 MSO_I }, { 7 MSO_I, 7 MSO_I }, { 10 MSO_I, 13 MSO_I }, { 12 MSO_I, 13 MSO_I },
    { 0, 0 }, { 21600, 21600 }, { 2 MSO_I, 5 MSO_I }, { 4 MSO_I, 5 MSO_I }
};
const SvxMSDffCalculationData mso_sptTextButtonPourCalc[] =
{
    { 0x400a, { 10800, DFF_Prop_adjustValue, 0 } },                 // 0x00
    { 0x4009, { 10800, DFF_Prop_adjustValue, 0 } },                 // 0x01
    { 0x2000, { 0x400, 10800, 0 } },                                // 0x02
    { 0x2000, { 0x401, 10800, 0 } },                                // 0x03
    { 0x8000, { 21600, 0, 0x402 } },                                // 0x04
    { 0x8000, { 21600, 0, 0x403 } },                                // 0x05

    { 0x8000, { 10800, 0, DFF_Prop_adjust2Value } },                // 0x06
    { 0x8000, { 21600, 0, 0x406 } },                                // 0x07

    { 0x600a, { DFF_Prop_adjust2Value, DFF_Prop_adjustValue, 0 } }, // 0x08
    { 0x6009, { DFF_Prop_adjust2Value, DFF_Prop_adjustValue, 0 } }, // 0x09
    { 0x2000, { 0x408, 10800, 0 } },    // 0x0a
    { 0x2000, { 0x409, 10800, 0 } },    // 0x0b
    { 0x8000, { 21600, 0, 0x40a } },    // 0x0c
    { 0x8000, { 21600, 0, 0x40b } },    // 0x0d
    { 0x2001, { 0x406, 1, 2 } },        // 0x0e
    { 0x4000, { 10800, 0x40e, 0 } },    // 0x0f
    { 0x8000, { 10800, 0, 0x40e } },    // 0x10
    { 0x6001, { 0x40e, 0x40e, 1 } },    // 0x11
    { 0x6001, { DFF_Prop_adjust2Value, DFF_Prop_adjust2Value, 1 } },    // 0x12
    { 0xA000, { 0x412, 0, 0x411 } },    // 0x13
    { 0x200d, { 0x413, 0, 0 } },        // 0x14
    { 0x4000, { 10800, 0x414, 0 } },    // 0x15
    { 0x8000, { 10800, 0, 0x414 } }     // 0x16
};
const sal_uInt16 mso_sptTextButtonPourSegm[] =
{
    0xA504, 0x8000,     // clockwise arc
    0xA504, 0x8000,     // clockwise arc
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000,
    0xA304, 0x8000,     // counter clockwise
    0xA304, 0x8000      // counter clockwise
};
const SvxMSDffHandle mso_sptTextButtonPourHandle[] =
{
    {   SvxMSDffHandleFlags::POLAR | SvxMSDffHandleFlags::RADIUS_RANGE,
        0x101, 0x100, 10800, 10800, 4320, 10800, MIN_INT32, 0x7fffffff }
};
const sal_Int32 mso_sptTextButtonPourDefault[] =
{
    2, 180, 5400
};
const mso_CustomShape msoTextButtonPour =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextButtonPourVert),
    const_cast<sal_uInt16*>(mso_sptTextButtonPourSegm), sizeof( mso_sptTextButtonPourSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextButtonPourCalc),
    const_cast<sal_Int32*>(mso_sptTextButtonPourDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextButtonPourHandle),
};

const SvxMSDffVertPair mso_sptTextCurveUpVert[] =
{
    { 0, 0 MSO_I }, { 4900, 1 MSO_I /*12170->0 14250 ->0*/ }, { 11640, 2 MSO_I /*12170->0 12800 ->0*/ }, { 21600, 0 },
    { 0, 4 MSO_I /*12170->0 17220 ->21600*/ }, { 3700, 21600 }, { 8500, 21600 }, { 10100, 21600 }, { 14110, 21600 }, { 15910, 21600 }, { 21600, 4 MSO_I /*12170->0 17220 ->21600*/ }
};
const SvxMSDffCalculationData mso_sptTextCurveUpCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } }, // 400
    { 0x4001, { 14250, 0x400, 12170 } },        // 401
    { 0x4001, { 12800, 0x400, 12170 } },        // 402
    { 0x4001, { 6380, 0x400, 12170 } },         // 403
    { 0x8000, { 21600, 0, 0x403 } }             // 404
};
const sal_uInt16 mso_sptTextCurveUpSegm[] =
{
    0x4000, 0x2001, 0x8000,
    0x4000, 0x2002, 0x8000
};
const SvxMSDffHandle mso_sptTextCurveUpHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 12170 }
};
const sal_Int32 mso_sptTextCurveUpDefault[] =
{
    1, 9900
};
const mso_CustomShape msoTextCurveUp =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextCurveUpVert),
    const_cast<sal_uInt16*>(mso_sptTextCurveUpSegm), sizeof( mso_sptTextCurveUpSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextCurveUpCalc),
    const_cast<sal_Int32*>(mso_sptTextCurveUpDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextCurveUpHandle),
};

const SvxMSDffVertPair mso_sptTextCurveDownVert[] =
{
//  { 0, 0 MSO_I }, { 4900, 1 MSO_I /*12170->0 14250 ->0*/ }, { 11640, 2 MSO_I /*12170->0 12800 ->0*/ }, { 21600, 0 },
    { 0, 0 }, { 9960, 2 MSO_I }, { 16700, 1 MSO_I }, { 21600, 0 MSO_I },

//  { 0, 4 MSO_I /*12170->0 17220 ->21600*/ }, { 3700, 21600 }, { 8500, 21600 }, { 10100, 21600 }, { 14110, 21600 }, { 15910, 21600 }, { 21600, 4 MSO_I /*12170->0 17220 ->21600*/ }
    { 0, 4 MSO_I }, { 5690, 21600 }, { 7490, 21600 }, { 11500, 21600 }, { 13100, 21600 }, { 17900, 21600 }, { 21600, 4 MSO_I }
};
const SvxMSDffHandle mso_sptTextCurveDownHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        21600, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 12170 }
};
const mso_CustomShape msoTextCurveDown =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextCurveDownVert),
    const_cast<sal_uInt16*>(mso_sptTextCurveUpSegm), sizeof( mso_sptTextCurveUpSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextCurveUpCalc),
    const_cast<sal_Int32*>(mso_sptTextCurveUpDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextCurveDownHandle),
};

const SvxMSDffVertPair mso_sptTextCanUpVert[] =
{
    { 0, 1 MSO_I }, { 900, 0 }, { 7100, 0 }, { 10800, 0 }, { 14500, 0 }, { 20700, 0 }, { 21600, 1 MSO_I },
    { 0, 21600 }, { 900, 4 MSO_I }, { 7100, 0 MSO_I }, { 10800, 0 MSO_I }, { 14500, 0 MSO_I }, { 20700, 4 MSO_I }, { 21600, 21600 }
};
const SvxMSDffCalculationData mso_sptTextCanUpCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },     // 400
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } }, // 401
    { 0x2000, { DFF_Prop_adjustValue, 0, 14400 } }, // 402
    { 0x4001, { 5470, 0x402, 7200 } },              // 403
    { 0x4000, { 16130, 0x403, 0 } }                 // 404
};
const sal_uInt16 mso_sptTextCanUpSegm[] =
{
    0x4000, 0x2002, 0x8000,
    0x4000, 0x2002, 0x8000
};
const SvxMSDffHandle mso_sptTextCanUpHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        10800, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 14400, 21600 }
};
const sal_Int32 mso_sptTextCanUpDefault[] =
{
    1, 18500
};
const mso_CustomShape msoTextCanUp =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextCanUpVert),
    const_cast<sal_uInt16*>(mso_sptTextCanUpSegm), sizeof( mso_sptTextCanUpSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextCanUpCalc),
    const_cast<sal_Int32*>(mso_sptTextCanUpDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextCanUpHandle),
};

const SvxMSDffVertPair mso_sptTextCanDownVert[] =
{
    { 0, 0 }, { 900, 2 MSO_I }, { 7100, 0 MSO_I }, { 10800, 0 MSO_I }, { 14500, 0 MSO_I }, { 20700, 2 MSO_I }, { 21600, 0 },
    { 0, 1 MSO_I }, { 900, 21600 }, { 7100, 21600 }, { 10800, 21600 }, { 14500, 21600 }, { 20700, 21600 }, { 21600, 1 MSO_I }
};
const SvxMSDffCalculationData mso_sptTextCanDownCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },     // 400
    { 0x8000, { 21600, 0, DFF_Prop_adjustValue } }, // 401
    { 0x4001, { 5470, 0x400, 7200 } }               // 402
};
const SvxMSDffHandle mso_sptTextCanDownHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        10800, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 7200 }
};
const sal_Int32 mso_sptTextCanDownDefault[] =
{
    1, 3100
};
const mso_CustomShape msoTextCanDown =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextCanDownVert),
    const_cast<sal_uInt16*>(mso_sptTextCanUpSegm), sizeof( mso_sptTextCanUpSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextCanDownCalc),
    const_cast<sal_Int32*>(mso_sptTextCanDownDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextCanDownHandle),
};

const SvxMSDffVertPair mso_sptTextInflateVert[] =
{
    { 0, 0 MSO_I }, { 4100, 1 MSO_I }, { 7300, 0 }, { 10800, 0 }, { 14300, 0 }, { 17500, 1 MSO_I }, { 21600, 0 MSO_I },
    { 0, 2 MSO_I }, { 4100, 3 MSO_I }, { 7300, 21600 }, { 10800, 21600 }, { 14300, 21600 }, { 17500, 3 MSO_I }, { 21600, 2 MSO_I }
};
const SvxMSDffCalculationData mso_sptTextInflateCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } }, // 400
    { 0x4001, { 1530, 0x400, 4650 } },          // 401
    { 0x8000, { 21600, 0, 0x400 } },            // 402
    { 0x8000, { 21600, 0, 0x401 } }             // 403
};
const SvxMSDffHandle mso_sptTextInflateHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 4650 }
};
const sal_Int32 mso_sptTextInflateDefault[] =
{
    1, 2950
};
const mso_CustomShape msoTextInflate =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextInflateVert),
    const_cast<sal_uInt16*>(mso_sptTextCanUpSegm), sizeof( mso_sptTextCanUpSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextInflateCalc),
    const_cast<sal_Int32*>(mso_sptTextInflateDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextInflateHandle),
};

const SvxMSDffVertPair mso_sptTextDeflateVert[] =
{
    { 0, 0 }, { 3500, 1 MSO_I }, { 7100, 0 MSO_I }, { 10800, 0 MSO_I }, { 14500, 0 MSO_I }, { 18100, 1 MSO_I }, { 21600, 0 },
    { 0, 21600 }, { 3500, 3 MSO_I }, { 7100, 2 MSO_I }, { 10800, 2 MSO_I }, { 14500, 2 MSO_I }, { 18100, 3 MSO_I }, { 21600, 21600 }
};
const SvxMSDffCalculationData mso_sptTextDeflateCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } }, // 400
    { 0x2001, { 0x400, 5320, 7100 } },          // 401
    { 0x8000, { 21600, 0, 0x400 } },            // 402
    { 0x8000, { 21600, 0, 0x401 } }             // 403
};
const SvxMSDffHandle mso_sptTextDeflateHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        10800, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 8100 }
};
const mso_CustomShape msoTextDeflate =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextDeflateVert),
    const_cast<sal_uInt16*>(mso_sptTextCanUpSegm), sizeof( mso_sptTextCanUpSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextDeflateCalc),
    const_cast<sal_Int32*>(mso_sptDefault8100),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextDeflateHandle),
};

const SvxMSDffVertPair mso_sptTextInflateBottomVert[] =
{
    { 0, 0 }, { 21600, 0 },
    { 0, 0 MSO_I }, { 3500, 3 MSO_I }, { 7300, 21600 }, { 10800, 21600 }, { 14300, 21600 }, { 18100, 3 MSO_I }, { 21600, 0 MSO_I }
};
const SvxMSDffCalculationData mso_sptTextInflateBottomCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } }, // 400
    { 0x2000, { 0x400, 0, 11150 } },            // 401 0->10450
    { 0x2001, { 0x401, 3900, 10450 } },         // 402
    { 0x2000, { 0x402, 17700, 0 } }             // 403
};
const sal_uInt16 mso_sptTextInflateBottomSegm[] =
{
    0x4000, 0x0001, 0x8000,
    0x4000, 0x2002, 0x8000
};
const SvxMSDffHandle mso_sptTextInflateBottomHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 11150, 21600 }
};
const sal_Int32 mso_sptTextInflateBottomDefault[] =
{
    1, 14700
};
const mso_CustomShape msoTextInflateBottom =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextInflateBottomVert),
    const_cast<sal_uInt16*>(mso_sptTextInflateBottomSegm), sizeof( mso_sptTextInflateBottomSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextInflateBottomCalc),
    const_cast<sal_Int32*>(mso_sptTextInflateBottomDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextInflateBottomHandle),
};

const SvxMSDffVertPair mso_sptTextDeflateBottomVert[] =
{
    { 0, 0 }, { 21600, 0 },
    { 0, 21600 }, { 2900, 3 MSO_I }, { 7200, 0 MSO_I }, { 10800, 0 MSO_I }, { 14400, 0 MSO_I }, { 18700, 3 MSO_I }, { 21600, 21600 }
};
const SvxMSDffCalculationData mso_sptTextDeflateBottomCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } }, // 400
    { 0x2000, { 0x400, 0, 1350 } },             // 401 0->20250
    { 0x2001, { 0x401, 12070, 20250 } },        // 402
    { 0x2000, { 0x402, 9530, 0 } }              // 403
};
const sal_uInt16 mso_sptTextDeflateBottomSegm[] =
{
    0x4000, 0x0001, 0x8000,
    0x4000, 0x2002, 0x8000
};
const SvxMSDffHandle mso_sptTextDeflateBottomHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        10800, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 1350, 21600 }
};
const sal_Int32 mso_sptTextDeflateBottomDefault[] =
{
    1, 11500
};
const mso_CustomShape msoTextDeflateBottom =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextDeflateBottomVert),
    const_cast<sal_uInt16*>(mso_sptTextDeflateBottomSegm), sizeof( mso_sptTextDeflateBottomSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextDeflateBottomCalc),
    const_cast<sal_Int32*>(mso_sptTextDeflateBottomDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextDeflateBottomHandle),
};

const SvxMSDffVertPair mso_sptTextInflateTopVert[] =
{
    { 0, 0 MSO_I }, { 3500, 1 MSO_I }, { 7300, 0 }, { 10800, 0 }, { 14300, 0 }, { 18100, 1 MSO_I }, { 21600, 0 MSO_I },
    { 0, 21600 }, { 21600, 21600 }
};
const SvxMSDffCalculationData mso_sptTextInflateTopCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } }, // 400
    { 0x2001, { 0x400, 3900, 10450 } }          // 401
};
const sal_uInt16 mso_sptTextInflateTopSegm[] =
{
    0x4000, 0x2002, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffHandle mso_sptTextInflateTopHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 10450 }
};
const sal_Int32 mso_sptTextInflateTopDefault[] =
{
    1, 6900
};
const mso_CustomShape msoTextInflateTop =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextInflateTopVert),
    const_cast<sal_uInt16*>(mso_sptTextInflateTopSegm), sizeof( mso_sptTextInflateTopSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextInflateTopCalc),
    const_cast<sal_Int32*>(mso_sptTextInflateTopDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextInflateTopHandle),
};

const SvxMSDffVertPair mso_sptTextDeflateTopVert[] =
{
    { 0, 0 }, { 2900, 1 MSO_I }, { 7200, 0 MSO_I }, { 10800, 0 MSO_I }, { 14400, 0 MSO_I }, { 18700, 1 MSO_I }, { 21600, 0 },
    { 0, 21600 }, { 21600, 21600 }
};
const SvxMSDffCalculationData mso_sptTextDeflateTopCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } }, // 400
    { 0x2001, { 0x400, 12070, 20250 } }         // 402
};
const sal_uInt16 mso_sptTextDeflateTopSegm[] =
{
    0x4000, 0x2002, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffHandle mso_sptTextDeflateTopHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        10800, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 0, 20250 }
};
const sal_Int32 mso_sptTextDeflateTopDefault[] =
{
    1, 10100
};
const mso_CustomShape msoTextDeflateTop =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextDeflateTopVert),
    const_cast<sal_uInt16*>(mso_sptTextDeflateTopSegm), sizeof( mso_sptTextDeflateTopSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextDeflateTopCalc),
    const_cast<sal_Int32*>(mso_sptTextDeflateTopDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextDeflateTopHandle),
};

const SvxMSDffVertPair mso_sptTextDeflateInflateVert[] =
{
    { 0, 0 }, { 21600, 0 },
    { 0, 10100 }, { 3300, 3 MSO_I }, { 7100, 5 MSO_I }, { 10800, 5 MSO_I }, { 14500, 5 MSO_I }, { 18300, 3 MSO_I }, { 21600, 10100 },
    { 0, 11500 }, { 3300, 4 MSO_I }, { 7100, 6 MSO_I }, { 10800, 6 MSO_I }, { 14500, 6 MSO_I }, { 18300, 4 MSO_I }, { 21600, 11500 },
    { 0, 21600 }, { 21600, 21600 }
};
const SvxMSDffCalculationData mso_sptTextDeflateInflateCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } }, // 400
    { 0x8000, { 10800, 0, 0x400 } },            // 401
    { 0x2001, { 0x401, 5770, 9500 } },          // 402
    { 0x8000, { 10100, 0, 0x402 } },            // 403
    { 0x8000, { 11500, 0, 0x402 } },            // 404
    { 0x2000, { 0x400, 0, 700 } },              // 405
    { 0x2000, { 0x400, 700, 0 } }               // 406
};
const sal_uInt16 mso_sptTextDeflateInflateSegm[] =
{
    0x4000, 0x0001, 0x8000,
    0x4000, 0x2002, 0x8000,
    0x4000, 0x2002, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffHandle mso_sptTextDeflateInflateHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        10800, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 1300, 20300 }
};
const sal_Int32 mso_sptTextDeflateInflateDefault[] =
{
    1, 6500
};
const mso_CustomShape msoTextDeflateInflate =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextDeflateInflateVert),
    const_cast<sal_uInt16*>(mso_sptTextDeflateInflateSegm), sizeof( mso_sptTextDeflateInflateSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextDeflateInflateCalc),
    const_cast<sal_Int32*>(mso_sptTextDeflateInflateDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextDeflateInflateHandle),
};

const SvxMSDffVertPair mso_sptTextDeflateInflateDeflateVert[] =
{
    { 0, 0 }, { 21600, 0 },
    { 0, 6600 }, { 3600, 3 MSO_I }, { 7250, 4 MSO_I }, { 10800, 4 MSO_I }, { 14350, 4 MSO_I }, { 18000, 3 MSO_I }, { 21600, 6600 },
    { 0, 7500 }, { 3600, 5 MSO_I }, { 7250, 6 MSO_I }, { 10800, 6 MSO_I }, { 14350, 6 MSO_I }, { 18000, 5 MSO_I }, { 21600, 7500 },
    { 0, 14100 }, { 3600, 9 MSO_I }, { 7250, 10 MSO_I }, { 10800, 10 MSO_I }, { 14350, 10 MSO_I }, { 18000, 9 MSO_I }, { 21600, 14100 },
    { 0, 15000 }, { 3600, 7 MSO_I }, { 7250, 8 MSO_I }, { 10800, 8 MSO_I }, { 14350, 8 MSO_I }, { 18000, 7 MSO_I }, { 21600, 15000 },
    { 0, 21600 }, { 21600, 21600 }
};
const SvxMSDffCalculationData mso_sptTextDeflateInflateDeflateCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 850 } },       // 400
    { 0x2001, { 0x400, 6120, 8700 } },
    { 0x2000, { 0x401, 0, 4280 } },
    { 0x4000, { 6600, 0x402, 0 } },
    { 0x2000, { DFF_Prop_adjustValue, 0, 450 } },       // 404
    { 0x2000, { 0x403, 900, 0 } },                      // 405
    { 0x2000, { 0x404, 900, 0 } },                      // 406
    { 0x8000, { 21600, 0, 0x403 } },                    // 407
    { 0x8000, { 21600, 0, 0x404 } },                    // 408
    { 0x8000, { 21600, 0, 0x405 } },                    // 409
    { 0x8000, { 21600, 0, 0x406 } }                     // 410
};
const sal_uInt16 mso_sptTextDeflateInflateDeflateSegm[] =
{
    0x4000, 0x0001, 0x8000,
    0x4000, 0x2002, 0x8000,
    0x4000, 0x2002, 0x8000,
    0x4000, 0x2002, 0x8000,
    0x4000, 0x2002, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffHandle mso_sptTextDeflateInflateDeflateHandle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        10800, 0x100, 10800, 10800, MIN_INT32, 0x7fffffff, 850, 9550 }
};
const sal_Int32 mso_sptTextDeflateInflateDeflateDefault[] =
{
    1, 6050
};
const mso_CustomShape msoTextDeflateInflateDeflate =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextDeflateInflateDeflateVert),
    const_cast<sal_uInt16*>(mso_sptTextDeflateInflateDeflateSegm), sizeof( mso_sptTextDeflateInflateDeflateSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTextDeflateInflateDeflateCalc),
    const_cast<sal_Int32*>(mso_sptTextDeflateInflateDeflateDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTextDeflateInflateDeflateHandle),
};

const SvxMSDffVertPair mso_sptTextWave1Vert[] =  // adjustment1 : 0 - 4459
{                                                   // adjustment2 : 8640 - 12960
    { 7 MSO_I, 0 MSO_I }, { 15 MSO_I, 9 MSO_I }, { 16 MSO_I, 10 MSO_I }, { 12 MSO_I, 0 MSO_I },
    { 29 MSO_I, 1 MSO_I }, { 27 MSO_I, 28 MSO_I }, { 25 MSO_I, 26 MSO_I }, { 24 MSO_I, 1 MSO_I }
};
const sal_uInt16 mso_sptTextWave1Segm[] =
{
    0x4000, 0x2001, 0x8000,
    0x4000, 0x2001, 0x8000
};
const mso_CustomShape msoTextWave1 =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextWave1Vert),
    const_cast<sal_uInt16*>(mso_sptTextWave1Segm), sizeof( mso_sptTextWave1Segm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptWaveCalc),
    const_cast<sal_Int32*>(mso_sptWaveDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptWaveGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptWaveHandle),
};

const SvxMSDffVertPair mso_sptTextWave2Vert[] =  // adjustment1 : 0 - 4459
{                                                   // adjustment2 : 8640 - 12960
    { 7 MSO_I, 0 MSO_I }, { 15 MSO_I, 10 MSO_I }, { 16 MSO_I, 9 MSO_I }, { 12 MSO_I, 0 MSO_I },
    { 29 MSO_I, 1 MSO_I }, { 27 MSO_I, 26 MSO_I }, { 25 MSO_I, 28 MSO_I }, { 24 MSO_I, 1 MSO_I }
};
const mso_CustomShape msoTextWave2 =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextWave2Vert),
    const_cast<sal_uInt16*>(mso_sptTextWave1Segm), sizeof( mso_sptTextWave1Segm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptWaveCalc),
    const_cast<sal_Int32*>(mso_sptWaveDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptFontWorkTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptWaveGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptWaveHandle),
};

const SvxMSDffVertPair mso_sptTextWave3Vert[] =  // adjustment1 : 0 - 2230
{                                                       // adjustment2 : 8640 - 12960
    { 7 MSO_I, 0 MSO_I }, { 15 MSO_I, 9 MSO_I }, { 0x1f MSO_I, 10 MSO_I }, { 0x12 MSO_I, 0 MSO_I }, { 0x1e MSO_I, 9 MSO_I }, { 16 MSO_I, 10 MSO_I }, { 12 MSO_I, 0 MSO_I },
    { 29 MSO_I, 1 MSO_I }, { 27 MSO_I, 28 MSO_I }, { 0x21 MSO_I, 26 MSO_I }, { 0x13 MSO_I, 1 MSO_I }, { 0x20 MSO_I, 28 MSO_I }, { 25 MSO_I, 26 MSO_I }, { 24 MSO_I, 1 MSO_I }
};
const sal_uInt16 mso_sptTextWave3Segm[] =
{
    0x4000, 0x2002, 0x8000,
    0x4000, 0x2002, 0x8000
};
const mso_CustomShape msoTextWave3 =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextWave3Vert),
    const_cast<sal_uInt16*>(mso_sptTextWave3Segm), sizeof( mso_sptTextWave3Segm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptDoubleWaveCalc),
    const_cast<sal_Int32*>(mso_sptDoubleWaveDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptDoubleWaveTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptDoubleWaveGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptDoubleWaveHandle),
};

const SvxMSDffVertPair mso_sptTextWave4Vert[] =  // adjustment1 : 0 - 2230
{                                                       // adjustment2 : 8640 - 12960
    { 7 MSO_I, 0 MSO_I }, { 15 MSO_I, 10 MSO_I }, { 0x1f MSO_I, 9 MSO_I }, { 0x12 MSO_I, 0 MSO_I }, { 0x1e MSO_I, 10 MSO_I }, { 16 MSO_I, 9 MSO_I }, { 12 MSO_I, 0 MSO_I },
    { 29 MSO_I, 1 MSO_I }, { 27 MSO_I, 26 MSO_I }, { 0x21 MSO_I, 28 MSO_I }, { 0x13 MSO_I, 1 MSO_I }, { 0x20 MSO_I, 26 MSO_I }, { 25 MSO_I, 28 MSO_I }, { 24 MSO_I, 1 MSO_I }
};
const mso_CustomShape msoTextWave4 =
{
    std::span<const SvxMSDffVertPair>(mso_sptTextWave4Vert),
    const_cast<sal_uInt16*>(mso_sptTextWave3Segm), sizeof( mso_sptTextWave3Segm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptDoubleWaveCalc),
    const_cast<sal_Int32*>(mso_sptDoubleWaveDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptDoubleWaveTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(mso_sptDoubleWaveGluePoints),
    std::span<const SvxMSDffHandle>(mso_sptDoubleWaveHandle),
};

const sal_Int32 mso_sptCalloutDefault1[] =
{
    4, -1800, 24500, -1800, 4000
};
const sal_Int32 mso_sptCalloutDefault2[] =
{
    4, -8300, 24500, -1800, 4000
};
const sal_Int32 mso_sptCalloutDefault3[] =
{
    6, -10000, 24500, -3600, 4000, -1800, 4000
};
const sal_Int32 mso_sptCalloutDefault4[] =
{
    8, 23400, 24500, 25200, 21600, 25200, 4000, 23400, 4000
};
const SvxMSDffVertPair mso_sptCalloutVert1[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 }, { 0 MSO_I, 1 MSO_I }, { 2 MSO_I, 3 MSO_I }
};
const SvxMSDffHandle mso_sptCalloutHandle1[] =
{
    {   SvxMSDffHandleFlags::NONE,
        0x100, 0x101, 10800, 10800, MIN_INT32, 0x7fffffff, MIN_INT32, 0x7fffffff },
    {   SvxMSDffHandleFlags::NONE,
        0x102, 0x103, 10800, 10800, MIN_INT32, 0x7fffffff, MIN_INT32, 0x7fffffff }
};
const sal_uInt16 mso_sptCalloutSegm1a[] =
{
    0x4000, 0x0003, 0x6000, 0x8000,
    0x4000, 0x0001, 0x8000
};
const sal_uInt16 mso_sptCalloutSegm1b[] =
{
    0x4000, 0xab00, 0x0003, 0x6000, 0x8000, // NO STROKE
    0x4000, 0x0001, 0x8000
};
const SvxMSDffVertPair mso_sptCallout1Vert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 }, { 0 MSO_I, 1 MSO_I }, { 2 MSO_I, 3 MSO_I }, { 2 MSO_I, 0 }, { 2 MSO_I, 21600 }
};
const sal_uInt16 mso_sptCallout1Segm1a[] =
{
    0x4000, 0x0003, 0x6000, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000
};
const sal_uInt16 mso_sptCallout1Segm1b[] =
{
    0x4000, 0xab00, 0x0003, 0x6000, 0x8000, // NO STROKE
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffVertPair mso_sptCallout2Verta[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 }, { 0 MSO_I, 1 MSO_I }, { 2 MSO_I, 3 MSO_I }, { 2 MSO_I, 3 MSO_I }, { 4 MSO_I, 5 MSO_I }
};
const SvxMSDffVertPair mso_sptCallout2Vertb[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 }, { 0 MSO_I, 1 MSO_I }, { 2 MSO_I, 3 MSO_I }, { 2 MSO_I, 3 MSO_I }, { 4 MSO_I, 5 MSO_I }, { 4 MSO_I, 0 }, { 4 MSO_I, 21600 }
};
const SvxMSDffHandle mso_sptCalloutHandle2[] =
{
    {   SvxMSDffHandleFlags::NONE,
        0x100, 0x101, 10800, 10800, MIN_INT32, 0x7fffffff, MIN_INT32, 0x7fffffff },
    {   SvxMSDffHandleFlags::NONE,
        0x102, 0x103, 10800, 10800, MIN_INT32, 0x7fffffff, MIN_INT32, 0x7fffffff },
    {   SvxMSDffHandleFlags::NONE,
        0x104, 0x105, 10800, 10800, MIN_INT32, 0x7fffffff, MIN_INT32, 0x7fffffff }
};
const sal_uInt16 mso_sptCallout2Segm1a[] =
{
    0x4000, 0x0003, 0x6000, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000
};
const sal_uInt16 mso_sptCallout2Segm1b[] =
{
    0x4000, 0xab00, 0x0003, 0x6000, 0x8000, // NO STROKE
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000
};
const sal_uInt16 mso_sptCallout2Segm1c[] =
{
    0x4000, 0x0003, 0x6000, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000
};
const sal_uInt16 mso_sptCallout2Segm1d[] =
{
    0x4000, 0xab00, 0x0003, 0x6000, 0x8000, // NO STROKE
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000,
    0x4000, 0x0001, 0x8000
};
const SvxMSDffVertPair mso_sptCallout3Verta[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 }, { 6 MSO_I, 7 MSO_I }, { 4 MSO_I, 5 MSO_I }, { 2 MSO_I, 3 MSO_I }, { 0 MSO_I, 1 MSO_I }
};
const SvxMSDffVertPair mso_sptCallout3Vertb[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }, { 0, 21600 }, { 6 MSO_I, 7 MSO_I }, { 4 MSO_I, 5 MSO_I }, { 2 MSO_I, 3 MSO_I }, { 0 MSO_I, 1 MSO_I }, { 6 MSO_I, 0 }, { 6 MSO_I, 21600 }
};
const SvxMSDffHandle mso_sptCalloutHandle3[] =
{
    {   SvxMSDffHandleFlags::NONE,
        0x100, 0x101, 10800, 10800, MIN_INT32, 0x7fffffff, MIN_INT32, 0x7fffffff },
    {   SvxMSDffHandleFlags::NONE,
        0x102, 0x103, 10800, 10800, MIN_INT32, 0x7fffffff, MIN_INT32, 0x7fffffff },
    {   SvxMSDffHandleFlags::NONE,
        0x104, 0x105, 10800, 10800, MIN_INT32, 0x7fffffff, MIN_INT32, 0x7fffffff },
    {   SvxMSDffHandleFlags::NONE,
        0x106, 0x107, 10800, 10800, MIN_INT32, 0x7fffffff, MIN_INT32, 0x7fffffff }
};
const sal_uInt16 mso_sptCallout3Segm1a[] =
{
    0x4000, 0x0003, 0x6000, 0x8000,
    0x4000, 0xaa00, 0x0003, 0x8000          // NO_FILL
};
const sal_uInt16 mso_sptCallout3Segm1b[] =
{
    0x4000, 0xab00, 0x0003, 0x6000, 0x8000, // NO STROKE
    0x4000, 0xaa00, 0x0003, 0x8000          // NO FILL
};
const sal_uInt16 mso_sptCallout3Segm1c[] =
{
    0x4000, 0x0003, 0x6000, 0x8000,
    0x4000, 0xaa00, 0x0003, 0x8000,         // NO FILL
    0x4000, 0x0001, 0x8000
};
const sal_uInt16 mso_sptCallout3Segm1d[] =
{
    0x4000, 0xab00, 0x0003, 0x6000, 0x8000, // NO STROKE
    0x4000, 0xaa00, 0x0003, 0x8000,         // NO FILL
    0x4000, 0x0001, 0x8000
};

const SvxMSDffCalculationData mso_sptCalloutCalc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust3Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust4Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust5Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust6Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust7Value, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust8Value, 0, 0 } }
};

const mso_CustomShape msoCallout90 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCalloutVert1),
    const_cast<sal_uInt16*>(mso_sptCalloutSegm1b), sizeof( mso_sptCalloutSegm1b ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCalloutCalc),
    const_cast<sal_Int32*>(mso_sptCalloutDefault1),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle1),
};
const mso_CustomShape msoCallout1 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCalloutVert1),
    const_cast<sal_uInt16*>(mso_sptCalloutSegm1b), sizeof( mso_sptCalloutSegm1b ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCalloutCalc),
    const_cast<sal_Int32*>(mso_sptCalloutDefault2),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle1),
};
const mso_CustomShape msoCallout2 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCallout2Verta),
    const_cast<sal_uInt16*>(mso_sptCallout2Segm1b), sizeof( mso_sptCallout2Segm1b ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCalloutCalc),
    const_cast<sal_Int32*>(mso_sptCalloutDefault3),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle2),
};
const mso_CustomShape msoCallout3 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCallout3Verta),
    const_cast<sal_uInt16*>(mso_sptCallout3Segm1b), sizeof( mso_sptCallout3Segm1b ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCalloutCalc),
    const_cast<sal_Int32*>(mso_sptCalloutDefault4),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle3),
};
const mso_CustomShape msoAccentCallout90 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCalloutVert1),
    const_cast<sal_uInt16*>(mso_sptCalloutSegm1b), sizeof( mso_sptCalloutSegm1b ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCalloutCalc),
    const_cast<sal_Int32*>(mso_sptCalloutDefault1),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle1),
};
const mso_CustomShape msoAccentCallout1 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCallout1Vert),
    const_cast<sal_uInt16*>(mso_sptCallout1Segm1b), sizeof( mso_sptCallout1Segm1b ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCalloutCalc),
    const_cast<sal_Int32*>(mso_sptCalloutDefault2),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle1),
};
const mso_CustomShape msoAccentCallout2 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCallout2Vertb),
    const_cast<sal_uInt16*>(mso_sptCallout2Segm1d), sizeof( mso_sptCallout2Segm1d ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCalloutCalc),
    const_cast<sal_Int32*>(mso_sptCalloutDefault3),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle2),
};
const mso_CustomShape msoAccentCallout3 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCallout3Vertb),
    const_cast<sal_uInt16*>(mso_sptCallout3Segm1d), sizeof( mso_sptCallout3Segm1d ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCalloutCalc),
    const_cast<sal_Int32*>(mso_sptCalloutDefault4),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle3),
};
const mso_CustomShape msoBorderCallout90 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCalloutVert1),
    const_cast<sal_uInt16*>(mso_sptCalloutSegm1a), sizeof( mso_sptCalloutSegm1a ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCalloutCalc),
    const_cast<sal_Int32*>(mso_sptCalloutDefault1),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle1),
};
const mso_CustomShape msoBorderCallout1 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCalloutVert1),
    const_cast<sal_uInt16*>(mso_sptCalloutSegm1a), sizeof( mso_sptCalloutSegm1a ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCalloutCalc),
    const_cast<sal_Int32*>(mso_sptCalloutDefault2),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle1),
};
const mso_CustomShape msoBorderCallout2 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCallout2Verta),
    const_cast<sal_uInt16*>(mso_sptCallout2Segm1a), sizeof( mso_sptCallout2Segm1a ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCalloutCalc),
    const_cast<sal_Int32*>(mso_sptCalloutDefault3),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle2),
};
const mso_CustomShape msoBorderCallout3 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCallout3Verta),
    const_cast<sal_uInt16*>(mso_sptCallout3Segm1a), sizeof( mso_sptCallout3Segm1a ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCalloutCalc),
    const_cast<sal_Int32*>(mso_sptCalloutDefault4),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle3),
};
const mso_CustomShape msoAccentBorderCallout90 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCalloutVert1),
    const_cast<sal_uInt16*>(mso_sptCalloutSegm1a), sizeof( mso_sptCalloutSegm1a ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCalloutCalc),
    const_cast<sal_Int32*>(mso_sptCalloutDefault1),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle1),
};
const mso_CustomShape msoAccentBorderCallout1 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCallout1Vert),
    const_cast<sal_uInt16*>(mso_sptCallout1Segm1a), sizeof( mso_sptCallout1Segm1a ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCalloutCalc),
    const_cast<sal_Int32*>(mso_sptCalloutDefault2),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle1),
};
const mso_CustomShape msoAccentBorderCallout2 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCallout2Vertb),
    const_cast<sal_uInt16*>(mso_sptCallout2Segm1c), sizeof( mso_sptCallout2Segm1c ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCalloutCalc),
    const_cast<sal_Int32*>(mso_sptCalloutDefault3),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle2),
};
const mso_CustomShape msoAccentBorderCallout3 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCallout3Vertb),
    const_cast<sal_uInt16*>(mso_sptCallout3Segm1c), sizeof( mso_sptCallout3Segm1c ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCalloutCalc),
    const_cast<sal_Int32*>(mso_sptCalloutDefault4),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCalloutHandle3),
};

const SvxMSDffVertPair mso_sptStraightConnector1Vert[] =
{
    { 0, 0 }, { 21600, 21600 }
};
const sal_uInt16 mso_sptStraightConnector1Segm[] =
{
    0x4000, 0x0001, 0x8000
};
const mso_CustomShape msoStraightConnector1 =
{
    std::span<const SvxMSDffVertPair>(mso_sptStraightConnector1Vert),
    const_cast<sal_uInt16*>(mso_sptStraightConnector1Segm), sizeof( mso_sptStraightConnector1Segm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>()
};

const SvxMSDffVertPair mso_sptBentConnector2Vert[] =
{
    { 0, 0 }, { 21600, 0 }, { 21600, 21600 }
};
const sal_uInt16 mso_sptBentConnector2Segm[] =
{
    0x4000, 0x0002, 0x8000
};
const mso_CustomShape msoBentConnector2 =
{
    std::span<const SvxMSDffVertPair>(mso_sptBentConnector2Vert),
    const_cast<sal_uInt16*>(mso_sptBentConnector2Segm), sizeof( mso_sptBentConnector2Segm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>()
};

const SvxMSDffVertPair mso_sptBentConnector3Vert[] =
{
    { 0, 0 }, { 0 MSO_I, 0 }, { 0 MSO_I, 21600 }, { 21600, 21600 }
};
const sal_uInt16 mso_sptBentConnector3Segm[] =
{
    0x4000, 0x0003, 0x8000
};
const SvxMSDffCalculationData mso_sptBentConnector3Calc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } }
};
const sal_Int32 mso_sptBentConnector3Default[] =
{
    1, 10800
};
const SvxMSDffHandle mso_sptBentConnector3Handle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 10800, 10800, 10800, MIN_INT32, 0x7fffffff, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoBentConnector3 =
{
    std::span<const SvxMSDffVertPair>(mso_sptBentConnector3Vert),
    const_cast<sal_uInt16*>(mso_sptBentConnector3Segm), sizeof( mso_sptBentConnector3Segm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptBentConnector3Calc),
    const_cast<sal_Int32*>(mso_sptBentConnector3Default),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptBentConnector3Handle),
};

const SvxMSDffVertPair mso_sptBentConnector4Vert[] =
{
    { 0, 0 }, { 0 MSO_I, 0 }, { 0 MSO_I, 1 MSO_I }, { 21600, 1 MSO_I }, { 21600, 21600 }
};
const sal_uInt16 mso_sptBentConnector4Segm[] =
{
    0x4000, 0x0004, 0x8000
};
const SvxMSDffCalculationData mso_sptBentConnector4Calc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x2000, { 0x400, 21600, 0 } },
    { 0x2001, { 0x402, 1, 2 } },
    { 0x2001, { 0x401, 1, 2 } }
};
const sal_Int32 mso_sptBentConnector4Default[] =
{
    2, 10800, 10800
};
const SvxMSDffHandle mso_sptBentConnector4Handle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        0x100, 4 + 3, 10800, 10800, MIN_INT32, 0x7fffffff, 4 + 3, 4 + 3 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
        3 + 3, 0x101, 10800, 10800, 3 + 3, 3 + 3, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoBentConnector4 =
{
    std::span<const SvxMSDffVertPair>(mso_sptBentConnector4Vert),
    const_cast<sal_uInt16*>(mso_sptBentConnector4Segm), sizeof( mso_sptBentConnector4Segm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptBentConnector4Calc),
    const_cast<sal_Int32*>(mso_sptBentConnector4Default),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptBentConnector4Handle),
};

const SvxMSDffVertPair mso_sptBentConnector5Vert[] =
{
    { 0, 0 }, { 0 MSO_I, 0 }, { 0 MSO_I, 4 MSO_I }, { 1 MSO_I, 4 MSO_I }, { 1 MSO_I, 21600 }, { 21600, 21600 }
};
const sal_uInt16 mso_sptBentConnector5Segm[] =
{
    0x4000, 0x0005, 0x8000
};
const SvxMSDffCalculationData mso_sptBentConnector5Calc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust3Value, 0, 0 } },
    { 0x6000, { 0x400, 0x401, 0 } },
    { 0x2001, { 0x402, 1, 2 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x2001, { 0x404, 1, 2 } },
    { 0x4000, { 21600, 0x404, 0 } },
    { 0x2001, { 0x406, 1, 2 } }
};
const sal_Int32 mso_sptBentConnector5Default[] =
{
    3, 10800, 10800, 10800
};
const SvxMSDffHandle mso_sptBentConnector5Handle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        0x100, 5 + 3, 10800, 10800, MIN_INT32, 0x7fffffff, 5 + 3, 5 + 3 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
        3 + 3, 0x101, 10800, 10800, 3 + 3, 3 + 3, MIN_INT32, 0x7fffffff },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        0x102, 7 + 3, 10800, 10800, MIN_INT32, 0x7fffffff, 7 + 3, 7 + 3 }
};
const mso_CustomShape msoBentConnector5 =
{
    std::span<const SvxMSDffVertPair>(mso_sptBentConnector5Vert),
    const_cast<sal_uInt16*>(mso_sptBentConnector5Segm), sizeof( mso_sptBentConnector5Segm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptBentConnector5Calc),
    const_cast<sal_Int32*>(mso_sptBentConnector5Default),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptBentConnector5Handle),
};

const SvxMSDffVertPair mso_sptCurvedConnector2Vert[] =
{
    { 0, 0 }, { 10800, 0 }, { 21600, 10800 }, { 21600, 21600 }
};
const sal_uInt16 mso_sptCurvedConnector2Segm[] =
{
    0x4000, 0x2001, 0x8000
};
const mso_CustomShape msoCurvedConnector2 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCurvedConnector2Vert),
    const_cast<sal_uInt16*>(mso_sptCurvedConnector2Segm), sizeof( mso_sptCurvedConnector2Segm ) >> 1,
    std::span<const SvxMSDffCalculationData>(),
    nullptr,
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>()
};

const SvxMSDffVertPair mso_sptCurvedConnector3Vert[] =
{
    { 0, 0 }, { 1 MSO_I, 0 }, { 0 MSO_I, 5400 }, { 0 MSO_I, 10800 }, { 0 MSO_I, 16200 }, { 3 MSO_I, 21600 }, { 21600, 21600 }
};
const sal_uInt16 mso_sptCurvedConnector3Segm[] =
{
    0x4000, 0x2002, 0x8000
};
const SvxMSDffCalculationData mso_sptCurvedConnector3Calc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2001, { 0x400, 1, 2 } },
    { 0x2000, { 0x400, 21600, 0 } },
    { 0x2001, { 0x402, 1, 2 } }
};
const sal_Int32 mso_sptCurvedConnector3Default[] =
{
    1, 10800
};
const SvxMSDffHandle mso_sptCurvedConnector3Handle[] =
{
    {   SvxMSDffHandleFlags::RANGE,
        0x100, 10800, 10800, 10800, MIN_INT32, 0x7fffffff, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoCurvedConnector3 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCurvedConnector3Vert),
    const_cast<sal_uInt16*>(mso_sptCurvedConnector3Segm), sizeof( mso_sptCurvedConnector3Segm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCurvedConnector3Calc),
    const_cast<sal_Int32*>(mso_sptCurvedConnector3Default),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCurvedConnector3Handle),
};

const SvxMSDffVertPair mso_sptCurvedConnector4Vert[] =
{
    { 0, 0 }, { 1 MSO_I, 0 }, { 0 MSO_I, 10 MSO_I }, { 0 MSO_I, 9 MSO_I },
    { 0 MSO_I, 12 MSO_I }, { 5 MSO_I, 8 MSO_I }, { 3 MSO_I, 8 MSO_I },
    { 7 MSO_I, 8 MSO_I }, { 21600, 14 MSO_I }, { 21600, 21600 }

};
const sal_uInt16 mso_sptCurvedConnector4Segm[] =
{
    0x4000, 0x2003, 0x8000
};
const SvxMSDffCalculationData mso_sptCurvedConnector4Calc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2001, { 0x400, 1, 2 } },
    { 0x4000, { 21600, 0x400, 0 } },
    { 0x2001, { 0x402, 1, 2 } },
    { 0x6000, { 0x400, 0x403, 0 } },
    { 0x2001, { 0x404, 1, 2 } },
    { 0x2000, { 0x403, 21600, 0 } },
    { 0x2001, { 0x406, 1, 2 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x2001, { DFF_Prop_adjust2Value, 1, 2 } },
    { 0x2001, { DFF_Prop_adjust2Value, 1, 4 } },
    { 0x6000, { 0x408, 0x409, 0 } },
    { 0x2001, { 0x40b, 1, 2 } },
    { 0x2000, { 0x408, 21600, 0 } },
    { 0x2001, { 0x40d, 1, 2 } }
};
const sal_Int32 mso_sptCurvedConnector4Default[] =
{
    2, 10800, 10800
};
const SvxMSDffHandle mso_sptCurvedConnector4Handle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        0x100, 9 + 3, 10800, 10800, MIN_INT32, 0x7fffffff, 9 + 3, 9 + 3 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
        3 + 3, 0x101, 10800, 10800, 3 + 3, 3 + 3, MIN_INT32, 0x7fffffff }
};
const mso_CustomShape msoCurvedConnector4 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCurvedConnector4Vert),
    const_cast<sal_uInt16*>(mso_sptCurvedConnector4Segm), sizeof( mso_sptCurvedConnector4Segm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCurvedConnector4Calc),
    const_cast<sal_Int32*>(mso_sptCurvedConnector4Default),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCurvedConnector4Handle),
};

const SvxMSDffVertPair mso_sptCurvedConnector5Vert[] =
{
    { 0, 0 },
    { 21 MSO_I, 0 }, { 0 MSO_I, 12 MSO_I }, { 0 MSO_I, 11 MSO_I },
    { 0 MSO_I, 14 MSO_I }, { 6 MSO_I, 4 MSO_I }, { 3 MSO_I, 4 MSO_I },
    { 8 MSO_I, 4 MSO_I }, { 1 MSO_I, 18 MSO_I }, { 1 MSO_I, 16 MSO_I },
    { 1 MSO_I, 20 MSO_I }, { 10 MSO_I, 21600 }, { 21600, 21600 }
};
const sal_uInt16 mso_sptCurvedConnector5Segm[] =
{
    0x4000, 0x2004, 0x8000
};
const SvxMSDffCalculationData mso_sptCurvedConnector5Calc[] =
{
    { 0x2000, { DFF_Prop_adjustValue, 0, 0 } },
    { 0x2000, { DFF_Prop_adjust3Value, 0, 0 } },
    { 0x6000, { 0x400, 0x401, 0 } },
    { 0x2001, { 0x402, 1, 2 } },
    { 0x2000, { DFF_Prop_adjust2Value, 0, 0 } },
    { 0x6000, { 0x400, 0x403, 0 } },
    { 0x2001, { 0x405, 1, 2 } },
    { 0x6000, { 0x401, 0x403, 0 } },
    { 0x2001, { 0x407, 1, 2 } },
    { 0x2000, { 0x401, 21600, 0 } },
    { 0x2001, { 0x409, 1, 2 } },
    { 0x2001, { 0x404, 1, 2 } },
    { 0x2001, { 0x40b, 1, 2 } },
    { 0x6000, { 0x404, 0x40b, 0 } },
    { 0x2001, { 0x40d, 1, 2 } },
    { 0x2000, { 0x404, 21600, 0 } },
    { 0x2001, { 0x40f, 1, 2 } },
    { 0x6000, { 0x404, 0x410, 0 } },
    { 0x2001, { 0x411, 1, 2 } },
    { 0x2000, { 0x410, 21600, 0 } },
    { 0x2001, { 0x413, 1, 2 } },
    { 0x2001, { 0x400, 1, 2 } }
};
const sal_Int32 mso_sptCurvedConnector5Default[] =
{
    3, 10800, 10800, 10800
};
const SvxMSDffHandle mso_sptCurvedConnector5Handle[] =
{
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        0x100, 11 + 3, 10800, 10800, MIN_INT32, 0x7fffffff, 11 + 3, 11 + 3 },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL,
        3 + 3, 0x101, 10800, 10800, 3 + 3, 3 + 3, MIN_INT32, 0x7fffffff },
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_Y_MIN_IS_SPECIAL | SvxMSDffHandleFlags::RANGE_Y_MAX_IS_SPECIAL,
        0x102, 16 + 3, 10800, 10800, MIN_INT32, 0x7fffffff, 16 + 3, 16 + 3 }
};
const mso_CustomShape msoCurvedConnector5 =
{
    std::span<const SvxMSDffVertPair>(mso_sptCurvedConnector5Vert),
    const_cast<sal_uInt16*>(mso_sptCurvedConnector5Segm), sizeof( mso_sptCurvedConnector5Segm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptCurvedConnector5Calc),
    const_cast<sal_Int32*>(mso_sptCurvedConnector5Default),
    std::span<const SvxMSDffTextRectangles>(),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptCurvedConnector5Handle),
};

/////////////////////////////teardrop
const SvxMSDffVertPair mso_sptTearDropVert[] =
{
    { 10800, 0 },
    { 0, 10800 },                                                   // X
    { 10800, 21600 },                                               // Y
    { 21600, 10800 },                                               // X
    { 21600, 10800 }, { 21600, 3 MSO_I }, { 0 MSO_I, 1 MSO_I },             // C
    { 0 MSO_I, 1 MSO_I }, { 4 MSO_I, 0 }, { 10800, 0 }
};

//  the last number (0x***n)  :  0 = sum, 1 = prod, 2 = mid, 3 = abs, 4 = min, 5 = max, 6 = if, 13 = sqrt, 15 = eclipse ...
//    the first number(0xn***)  :  2/4/8 the first/second/third value is not directly value
const SvxMSDffCalculationData mso_sptTearDropCalc[] =
{
    { 0x2000 , { DFF_Prop_adjustValue , 0 , 0 } },  // 0  adjust value #0
    { 0x8000 , { 21600 , 0 , 0x0400 } },            // 1  21600 - @0                     y0
    { 0x8000 , { 32400 , 0 , 0x0400 } },            // 2   (32400 - @0)
    { 0x2001 , { 0x0402 , 1 , 2 } },                // 3   (32400 - @0)/2               y1
    { 0x2002 , { 0x0400 , 10800 , 0 } },            // 4  (@0+10800)/2                  x2
};

//m, qx, qy, qx,C,C
//the last number(0x***n) : repeat number of this current Segm
const sal_uInt16 mso_sptTearDropSegm[] =
{
    0x4000, 0xa701, 0xa801, 0xa701, 0x2002, 0x6000, 0x8000
};

const SvxMSDffTextRectangles mso_sptTearDropTextRect[] =
{
    { { 2863, 2863 }, { 18737, 18737 } }
};

//the range of adjust values
const SvxMSDffHandle mso_sptTearDropHandle[] =
{
    //position="$0,0" xrange="10800,32400"
    {   SvxMSDffHandleFlags::RANGE | SvxMSDffHandleFlags::RANGE_X_MAX_IS_SPECIAL| SvxMSDffHandleFlags::RANGE_X_MIN_IS_SPECIAL,
        0x100, 0, 10800, 10800, 10800, 32400, MIN_INT32, 0x7fffffff }
};

//the number of adjust values, the default values
const sal_Int32 mso_sptTearDropDefault[] =
{
    1, 21600
};

const mso_CustomShape msoTearDrop =
{
    std::span<const SvxMSDffVertPair>(mso_sptTearDropVert),
    const_cast<sal_uInt16*>(mso_sptTearDropSegm), sizeof( mso_sptTearDropSegm ) >> 1,
    std::span<const SvxMSDffCalculationData>(mso_sptTearDropCalc),
    const_cast<sal_Int32*>(mso_sptTearDropDefault),
    std::span<const SvxMSDffTextRectangles>(mso_sptTearDropTextRect),
    21600, 21600,
    MIN_INT32, MIN_INT32,
    std::span<const SvxMSDffVertPair>(),
    std::span<const SvxMSDffHandle>(mso_sptTearDropHandle),
};

///////////////////////////// sinusoid
const SvxMSDffVertPair mso_sptSinusoidVert[] =
{
    // calculations courtesy of section 6.3.2 of
    // https://documentation.libreoffice.org/assets/Uploads/Documentation/en/Tutorials/CustomShapes7/Custom-Shape-Tutorial.pdf#page=52

    // taking K = (3pi/2 - 3pi^2/8 - 1/2)
    // transforming points to viewbox of size 21600 x 21600

    // Start point
    {     0, 10800 }, // (0, 0)
    // First Bezier
    {  1757,  5278 }, // (K, K)
    {  3437,     0 }, // (1, 1)
    {  5400,     0 }, // (pi/2, 1)
    // Second Bezier
    {  7362,     0 }, // (pi - 1, 1)
    {  9042,  5278 }, // (pi - K, K)
    { 10800, 10800 }, // (pi, 0)
    // Third Bezier
    { 12557, 16321 }, // (pi + K, -K)
    { 14237, 21600 }, // (pi + 1, -1)
    { 16200, 21600 }, // (3pi/2, -1)
    // Fourth Bezier
    { 18162, 21600 }, // (2pi - 1, -1)
    { 19842, 16321 }, // (2pi - K, -K)
    { 21600, 10800 }  // (2pi, 0)
};

const sal_uInt16 mso_sptSinusoidSegm[] =
{
    0x4000, 0x2004, 0x8000
};

const SvxMSDffVertPair mso_sptSinusoidGluePoints[] =
{
    {     0, 10800 },  // x = 0.00
    {  5400,     0 },  // x = 0.25
    { 10800, 10800 },  // x = 0.50
    { 16200, 21600 },  // x = 0.75
    { 21600, 10800 }   // x = 1.00
};

const mso_CustomShape msoSinusoid =
{
    std::span<const SvxMSDffVertPair>(mso_sptSinusoidVert),                         // vertices
    const_cast<sal_uInt16*>(mso_sptSinusoidSegm), sizeof(mso_sptSinusoidSegm) >> 1, // segments, count
    std::span<const SvxMSDffCalculationData>(),                                     // calculations (none)
    nullptr,                                                                        // default values (none)
    std::span<const SvxMSDffTextRectangles>(),                                      // text area (default)
    21600, 21600,                                                                   // viewbox w, h (default)
    MIN_INT32, MIN_INT32,                                                           // stretch x, y (unset)
    std::span<const SvxMSDffVertPair>(mso_sptSinusoidGluePoints),                   // glue points
    std::span<const SvxMSDffHandle>(),                                              // handles (none)
};


const mso_CustomShape* GetCustomShapeContent( MSO_SPT eSpType )
{
    const mso_CustomShape* pCustomShape = nullptr;
    switch( eSpType )
    {
        case mso_sptArc :                       pCustomShape = &msoArc; break;
        case mso_sptLine:                       pCustomShape = &msoStraightConnector1; break;
        case mso_sptRectangle :                 pCustomShape = &msoRectangle; break;
        case mso_sptParallelogram :             pCustomShape = &msoParallelogram; break;
        case mso_sptTrapezoid :                 pCustomShape = &msoTrapezoid; break;
        case mso_sptDiamond :                   pCustomShape = &msoDiamond; break;
        case mso_sptRoundRectangle :            pCustomShape = &msoRoundRectangle; break;
        case mso_sptOctagon :                   pCustomShape = &msoOctagon; break;
        case mso_sptIsocelesTriangle :          pCustomShape = &msoIsocelesTriangle; break;
        case mso_sptRightTriangle :             pCustomShape = &msoRightTriangle; break;
        case mso_sptEllipse :                   pCustomShape = &msoEllipse; break;
        case mso_sptHexagon :                   pCustomShape = &msoHexagon; break;
        case mso_sptPlus :                      pCustomShape = &msoPlus; break;
        case mso_sptPentagon :                  pCustomShape = &msoPentagon; break;
        case mso_sptCan :                       pCustomShape = &msoCan; break;
        case mso_sptCube :                      pCustomShape = &msoCube; break;
        case mso_sptBalloon :                   pCustomShape = &msoBalloon; break;
        case mso_sptActionButtonBlank :         pCustomShape = &msoActionButtonBlank; break;
        case mso_sptActionButtonHome :          pCustomShape = &msoActionButtonHome; break;
        case mso_sptActionButtonHelp :          pCustomShape = &msoActionButtonHelp; break;
        case mso_sptActionButtonInformation :   pCustomShape = &msoActionButtonInformation; break;
        case mso_sptActionButtonBackPrevious :  pCustomShape = &msoActionButtonBackPrevious; break;
        case mso_sptActionButtonForwardNext :   pCustomShape = &msoActionButtonForwardNext; break;
        case mso_sptActionButtonBeginning :     pCustomShape = &msoActionButtonBeginning; break;
        case mso_sptActionButtonEnd :           pCustomShape = &msoActionButtonEnd; break;
        case mso_sptActionButtonReturn :        pCustomShape = &msoActionButtonReturn;  break;
        case mso_sptActionButtonDocument :      pCustomShape = &msoActionButtonDocument; break;
        case mso_sptActionButtonSound :         pCustomShape = &msoActionButtonSound; break;
        case mso_sptActionButtonMovie :         pCustomShape = &msoActionButtonMovie; break;
        case mso_sptBevel :                     pCustomShape = &msoBevel; break;
        case mso_sptFoldedCorner :              pCustomShape = &msoFoldedCorner; break;
        case mso_sptSmileyFace :                pCustomShape = &msoSmileyFace;  break;
        case mso_sptDonut :                     pCustomShape = &msoDonut; break;
        case mso_sptNoSmoking :                 pCustomShape = &msoNoSmoking; break;
        case mso_sptBlockArc :                  pCustomShape = &msoBlockArc; break;
        case mso_sptHeart :                     pCustomShape = &msoHeart; break;
        case mso_sptLightningBolt :             pCustomShape = &msoLightningBold; break;
        case mso_sptSun :                       pCustomShape = &msoSun; break;
        case mso_sptMoon :                      pCustomShape = &msoMoon; break;
        case mso_sptBracketPair :               pCustomShape = &msoBracketPair; break;
        case mso_sptBracePair :                 pCustomShape = &msoBracePair; break;
        case mso_sptPlaque :                    pCustomShape = &msoPlaque; break;
        case mso_sptLeftBracket :               pCustomShape = &msoLeftBracket; break;
        case mso_sptRightBracket :              pCustomShape = &msoRightBracket; break;
        case mso_sptLeftBrace :                 pCustomShape = &msoLeftBrace; break;
        case mso_sptRightBrace :                pCustomShape = &msoRightBrace; break;
        case mso_sptArrow :                     pCustomShape = &msoArrow; break;
        case mso_sptUpArrow :                   pCustomShape = &msoUpArrow; break;
        case mso_sptDownArrow :                 pCustomShape = &msoDownArrow; break;
        case mso_sptLeftArrow :                 pCustomShape = &msoLeftArrow; break;
        case mso_sptLeftRightArrow :            pCustomShape = &msoLeftRightArrow; break;
        case mso_sptUpDownArrow :               pCustomShape = &msoUpDownArrow; break;
        case mso_sptQuadArrow :                 pCustomShape = &msoQuadArrow; break;
        case mso_sptLeftRightUpArrow :          pCustomShape = &msoLeftRightUpArrow; break;
        case mso_sptBentArrow :                 pCustomShape = &msoBentArrow; break;
        case mso_sptUturnArrow :                pCustomShape = &msoUturnArrow; break;
        case mso_sptLeftUpArrow :               pCustomShape = &msoLeftUpArrow; break;
        case mso_sptBentUpArrow :               pCustomShape = &msoBentUpArrow; break;
        case mso_sptCurvedRightArrow :          pCustomShape = &msoCurvedRightArrow; break;
        case mso_sptCurvedLeftArrow :           pCustomShape = &msoCurvedLeftArrow; break;
        case mso_sptCurvedUpArrow :             pCustomShape = &msoCurvedUpArrow; break;
        case mso_sptCurvedDownArrow :           pCustomShape = &msoCurvedDownArrow; break;
        case mso_sptStripedRightArrow :         pCustomShape = &msoStripedRightArrow; break;
        case mso_sptNotchedRightArrow :         pCustomShape = &msoNotchedRightArrow; break;
        case mso_sptHomePlate :                 pCustomShape = &msoHomePlate; break;
        case mso_sptChevron :                   pCustomShape = &msoChevron; break;
        case mso_sptRightArrowCallout :         pCustomShape = &msoRightArrowCallout; break;
        case mso_sptLeftArrowCallout :          pCustomShape = &msoLeftArrowCallout; break;
        case mso_sptUpArrowCallout :            pCustomShape = &msoUpArrowCallout; break;
        case mso_sptDownArrowCallout :          pCustomShape = &msoDownArrowCallout; break;
        case mso_sptLeftRightArrowCallout :     pCustomShape = &msoLeftRightArrowCallout; break;
        case mso_sptUpDownArrowCallout :        pCustomShape = &msoUpDownArrowCallout; break;
        case mso_sptQuadArrowCallout :          pCustomShape = &msoQuadArrowCallout; break;
        case mso_sptCircularArrow :             pCustomShape = &msoCircularArrow; break;
        case mso_sptIrregularSeal1 :            pCustomShape = &msoIrregularSeal1; break;
        case mso_sptIrregularSeal2 :            pCustomShape = &msoIrregularSeal2; break;
        case mso_sptSeal4 :                     pCustomShape = &msoSeal4; break;
        case mso_sptStar :                      pCustomShape = &msoStar; break;
        case mso_sptSeal8 :                     pCustomShape = &msoSeal8; break;
        case mso_sptSeal :
        case mso_sptSeal16 :                    pCustomShape = &msoSeal16; break;
        case mso_sptSeal24 :                    pCustomShape = &msoSeal24; break;
        case mso_sptSeal32 :                    pCustomShape = &msoSeal32; break;
        case mso_sptRibbon2 :                   pCustomShape = &msoRibbon2; break;
        case mso_sptRibbon :                    pCustomShape = &msoRibbon; break;
        case mso_sptEllipseRibbon2 :            pCustomShape = &msosptEllipseRibbon2; break;    // SJ: TODO
        case mso_sptEllipseRibbon :             pCustomShape = &msosptEllipseRibbon; break; // SJ: TODO
        case mso_sptVerticalScroll :            pCustomShape = &msoVerticalScroll;  break;
        case mso_sptHorizontalScroll :          pCustomShape = &msoHorizontalScroll; break;
        case mso_sptFlowChartProcess :          pCustomShape = &msoFlowChartProcess; break;
        case mso_sptFlowChartAlternateProcess : pCustomShape = &msoFlowChartAlternateProcess; break;
        case mso_sptFlowChartDecision :         pCustomShape = &msoFlowChartDecision; break;
        case mso_sptFlowChartInputOutput :      pCustomShape = &msoFlowChartInputOutput; break;
        case mso_sptFlowChartPredefinedProcess :pCustomShape = &msoFlowChartPredefinedProcess; break;
        case mso_sptFlowChartInternalStorage :  pCustomShape = &msoFlowChartInternalStorage; break;
        case mso_sptFlowChartDocument :         pCustomShape = &msoFlowChartDocument; break;
        case mso_sptFlowChartMultidocument :    pCustomShape = &msoFlowChartMultidocument; break;
        case mso_sptFlowChartTerminator :       pCustomShape = &msoFlowChartTerminator; break;
        case mso_sptFlowChartPreparation :      pCustomShape = &msoFlowChartPreparation; break;
        case mso_sptFlowChartManualInput :      pCustomShape = &msoFlowChartManualInput; break;
        case mso_sptFlowChartManualOperation :  pCustomShape = &msoFlowChartManualOperation; break;
        case mso_sptFlowChartConnector :        pCustomShape = &msoFlowChartConnector; break;
        case mso_sptFlowChartOffpageConnector : pCustomShape = &msoFlowChartOffpageConnector; break;
        case mso_sptFlowChartPunchedCard :      pCustomShape = &msoFlowChartPunchedCard; break;
        case mso_sptFlowChartPunchedTape :      pCustomShape = &msoFlowChartPunchedTape; break;
        case mso_sptFlowChartSummingJunction :  pCustomShape = &msoFlowChartSummingJunction; break;
        case mso_sptFlowChartOr :               pCustomShape = &msoFlowChartOr; break;
        case mso_sptFlowChartCollate :          pCustomShape = &msoFlowChartCollate; break;
        case mso_sptFlowChartSort :             pCustomShape = &msoFlowChartSort; break;
        case mso_sptFlowChartExtract :          pCustomShape = &msoFlowChartExtract; break;
        case mso_sptFlowChartMerge :            pCustomShape = &msoFlowChartMerge; break;
        case mso_sptFlowChartOnlineStorage :    pCustomShape = &msoFlowChartOnlineStorage; break;
        case mso_sptFlowChartDelay :            pCustomShape = &msoFlowChartDelay; break;
        case mso_sptFlowChartMagneticTape :     pCustomShape = &msoFlowChartMagneticTape; break;
        case mso_sptFlowChartMagneticDisk :     pCustomShape = &msoFlowChartMagneticDisk; break;
        case mso_sptFlowChartMagneticDrum :     pCustomShape = &msoFlowChartMagneticDrum; break;
        case mso_sptFlowChartDisplay :          pCustomShape = &msoFlowChartDisplay; break;
        case mso_sptWedgeRectCallout :          pCustomShape = &msoWedgeRectCallout; break;
        case mso_sptWedgeRRectCallout :         pCustomShape = &msoWedgeRRectCallout; break;
        case mso_sptWedgeEllipseCallout :       pCustomShape = &msoWedgeEllipseCallout; break;
        case mso_sptCloudCallout :              pCustomShape = &msoCloudCallout; break;
        case mso_sptWave :                      pCustomShape = &msoWave; break;
        case mso_sptDoubleWave :                pCustomShape = &msoDoubleWave; break;
        case mso_sptSinusoid :                  pCustomShape = &msoSinusoid; break;

        // callout
        case mso_sptCallout1 :                  pCustomShape = &msoCallout1; break;
        case mso_sptCallout2 :                  pCustomShape = &msoCallout2; break;
        case mso_sptCallout3 :                  pCustomShape = &msoCallout3; break;
        case mso_sptAccentCallout1 :            pCustomShape = &msoAccentCallout1; break;
        case mso_sptAccentCallout2 :            pCustomShape = &msoAccentCallout2; break;
        case mso_sptAccentCallout3 :            pCustomShape = &msoAccentCallout3; break;
        case mso_sptBorderCallout1 :            pCustomShape = &msoBorderCallout1; break;
        case mso_sptBorderCallout2 :            pCustomShape = &msoBorderCallout2; break;
        case mso_sptBorderCallout3 :            pCustomShape = &msoBorderCallout3; break;
        case mso_sptAccentBorderCallout1 :      pCustomShape = &msoAccentBorderCallout1; break;
        case mso_sptAccentBorderCallout2 :      pCustomShape = &msoAccentBorderCallout2; break;
        case mso_sptAccentBorderCallout3 :      pCustomShape = &msoAccentBorderCallout3; break;
        case mso_sptCallout90 :                 pCustomShape = &msoCallout90; break;
        case mso_sptAccentCallout90 :           pCustomShape = &msoAccentCallout90; break;
        case mso_sptBorderCallout90 :           pCustomShape = &msoBorderCallout90; break;
        case mso_sptAccentBorderCallout90 :     pCustomShape = &msoAccentBorderCallout90; break;

        // connectors
        case mso_sptStraightConnector1 :        pCustomShape = &msoStraightConnector1; break;
        case mso_sptBentConnector2 :            pCustomShape = &msoBentConnector2; break;
        case mso_sptBentConnector3 :            pCustomShape = &msoBentConnector3; break;
        case mso_sptBentConnector4 :            pCustomShape = &msoBentConnector4; break;
        case mso_sptBentConnector5 :            pCustomShape = &msoBentConnector5; break;
        case mso_sptCurvedConnector2 :          pCustomShape = &msoCurvedConnector2; break;
        case mso_sptCurvedConnector3 :          pCustomShape = &msoCurvedConnector3; break;
        case mso_sptCurvedConnector4 :          pCustomShape = &msoCurvedConnector4; break;
        case mso_sptCurvedConnector5 :          pCustomShape = &msoCurvedConnector5; break;

        // Don't know, simply mapping to TextSimple
        case mso_sptTextBox :                   pCustomShape = &msoTextSimple; break;

        // FontWork
        case mso_sptTextSimple :
        case mso_sptTextPlainText :             pCustomShape = &msoTextPlainText; break;
        case mso_sptTextOctagon :
        case mso_sptTextStop :                  pCustomShape = &msoTextStop; break;
        case mso_sptTextHexagon :
        case mso_sptTextTriangle :              pCustomShape = &msoTextTriangle; break;
        case mso_sptTextTriangleInverted :      pCustomShape = &msoTextTriangleInverted; break;
        case mso_sptTextChevron :               pCustomShape = &msoTextChevron; break;
        case mso_sptTextChevronInverted :       pCustomShape = &msoTextChevronInverted; break;
        case mso_sptTextRingInside :            pCustomShape = &msoTextRingInside; break;   // SJ: TODO->the orientation of the ellipse needs to be changed
        case mso_sptTextRingOutside :           pCustomShape = &msoTextRingOutside; break;
        case mso_sptTextFadeRight :             pCustomShape = &msoTextFadeRight; break;
        case mso_sptTextFadeLeft :              pCustomShape = &msoTextFadeLeft; break;
        case mso_sptTextFadeUp :                pCustomShape = &msoTextFadeUp; break;
        case mso_sptTextFadeDown :              pCustomShape = &msoTextFadeDown; break;
        case mso_sptTextSlantUp :               pCustomShape = &msoTextSlantUp; break;
        case mso_sptTextSlantDown :             pCustomShape = &msoTextSlantDown; break;
        case mso_sptTextCascadeUp :             pCustomShape = &msoTextCascadeUp; break;
        case mso_sptTextCascadeDown :           pCustomShape = &msoTextCascadeDown; break;
        case mso_sptTextOnRing :
        case mso_sptTextArchUpCurve :           pCustomShape = &msoTextArchUpCurve; break;
        case mso_sptTextArchDownCurve :         pCustomShape = &msoTextArchDownCurve; break;
        case mso_sptTextCircleCurve :           pCustomShape = &msoTextCircleCurve; break;
        case mso_sptTextButtonCurve :           pCustomShape = &msoTextButtonCurve; break;
        case mso_sptTextRing :
        case mso_sptTextArchUpPour :            pCustomShape = &msoTextArchUpPour; break;
        case mso_sptTextArchDownPour :          pCustomShape = &msoTextArchDownPour; break;
        case mso_sptTextCirclePour :            pCustomShape = &msoTextCirclePour; break;
        case mso_sptTextButtonPour :            pCustomShape = &msoTextButtonPour; break;
        case mso_sptTextCurveUp :               pCustomShape = &msoTextCurveUp; break;
        case mso_sptTextCurveDown :             pCustomShape = &msoTextCurveDown; break;
        case mso_sptTextCanUp :                 pCustomShape = &msoTextCanUp; break;
        case mso_sptTextCurve :
        case mso_sptTextOnCurve :
        case mso_sptTextCanDown :               pCustomShape = &msoTextCanDown; break;
        case mso_sptTextInflate :               pCustomShape = &msoTextInflate; break;
        case mso_sptTextDeflate :               pCustomShape = &msoTextDeflate; break;
        case mso_sptTextInflateBottom :         pCustomShape = &msoTextInflateBottom; break;
        case mso_sptTextDeflateBottom :         pCustomShape = &msoTextDeflateBottom; break;
        case mso_sptTextInflateTop :            pCustomShape = &msoTextInflateTop; break;
        case mso_sptTextDeflateTop :            pCustomShape = &msoTextDeflateTop; break;
        case mso_sptTextDeflateInflate :        pCustomShape = &msoTextDeflateInflate; break;
        case mso_sptTextDeflateInflateDeflate : pCustomShape = &msoTextDeflateInflateDeflate; break;
        case mso_sptTextWave :
        case mso_sptTextWave1 :                 pCustomShape = &msoTextWave1; break;
        case mso_sptTextWave2 :                 pCustomShape = &msoTextWave2; break;
        case mso_sptTextWave3 :                 pCustomShape = &msoTextWave3; break;
        case mso_sptTextWave4 :                 pCustomShape = &msoTextWave4; break;
        case mso_sptTearDrop :                  pCustomShape = &msoTearDrop; break;
        default :
        break;
    }
    return pCustomShape;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
