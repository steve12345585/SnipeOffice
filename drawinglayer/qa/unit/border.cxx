/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <memory>
#include <cppunit/TestAssert.h>
#include <cppunit/plugin/TestPlugIn.h>

#include <drawinglayer/geometry/viewinformation2d.hxx>
#include <drawinglayer/primitive2d/borderlineprimitive2d.hxx>
#include <drawinglayer/primitive2d/PolygonStrokePrimitive2D.hxx>
#include <drawinglayer/primitive2d/groupprimitive2d.hxx>
#include <drawinglayer/processor2d/baseprocessor2d.hxx>
#include <drawinglayer/processor2d/processor2dtools.hxx>
#include <rtl/ref.hxx>
#include <test/bootstrapfixture.hxx>
#include <vcl/metaact.hxx>
#include <vcl/vclptr.hxx>
#include <vcl/virdev.hxx>
#include <editeng/borderline.hxx>
#include <svtools/borderhelper.hxx>

using namespace com::sun::star;

namespace
{
class DrawinglayerBorderTest : public test::BootstrapFixture
{
};

CPPUNIT_TEST_FIXTURE(DrawinglayerBorderTest, testDoubleDecompositionSolid)
{
    // Create a border line primitive that's similar to the one from the bugdoc:
    // 1.47 pixels is 0.03cm at 130% zoom and 96 DPI.
    basegfx::B2DPoint aStart(0, 20);
    basegfx::B2DPoint aEnd(100, 20);
    double const fLeftWidth = 1.47;
    double const fDistance = 1.47;
    double const fRightWidth = 1.47;
    double const fExtendLeftStart = 0;
    double const fExtendLeftEnd = 0;
    double const fExtendRightStart = 0;
    double const fExtendRightEnd = 0;
    basegfx::BColor aColorRight;
    basegfx::BColor aColorLeft;
    std::vector<double> aDashing(svtools::GetLineDashing(SvxBorderLineStyle::DOUBLE, 10.0));
    const drawinglayer::attribute::StrokeAttribute aStrokeAttribute(std::move(aDashing));
    std::vector<drawinglayer::primitive2d::BorderLine> aBorderlines{

        drawinglayer::primitive2d::BorderLine(
            drawinglayer::attribute::LineAttribute(aColorLeft, fLeftWidth), fExtendLeftStart,
            fExtendLeftStart, fExtendLeftEnd, fExtendLeftEnd),

        drawinglayer::primitive2d::BorderLine(fDistance),

        drawinglayer::primitive2d::BorderLine(
            drawinglayer::attribute::LineAttribute(aColorRight, fRightWidth), fExtendRightStart,
            fExtendRightStart, fExtendRightEnd, fExtendRightEnd)
    };

    rtl::Reference<drawinglayer::primitive2d::BorderLinePrimitive2D> aBorder(
        new drawinglayer::primitive2d::BorderLinePrimitive2D(aStart, aEnd, std::move(aBorderlines),
                                                             aStrokeAttribute));

    // Decompose it into polygons.
    drawinglayer::geometry::ViewInformation2D aView;
    drawinglayer::primitive2d::Primitive2DContainer aContainer;
    aBorder->get2DDecomposition(aContainer, aView);

    // Make sure it results in two borders as it's a double one.
    CPPUNIT_ASSERT_EQUAL(static_cast<std::size_t>(1), aContainer.size());
    auto* pGroupPrimitive
        = dynamic_cast<const drawinglayer::primitive2d::GroupPrimitive2D*>(aContainer[0].get());
    CPPUNIT_ASSERT_EQUAL(static_cast<std::size_t>(2), pGroupPrimitive->getChildren().size());

    // Get the inside line, now a PolygonStrokePrimitive2D
    auto pInside = dynamic_cast<const drawinglayer::primitive2d::PolygonStrokePrimitive2D*>(
        pGroupPrimitive->getChildren()[0].get());
    CPPUNIT_ASSERT(pInside);

    // Make sure the inside line's height is fLeftWidth.
    const double fLineWidthFromDecompose = pInside->getLineAttribute().getWidth();

    // This was 2.47, i.e. the width of the inner line was 1 unit (in the bugdoc's case: 1 pixel) wider than expected.
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fLeftWidth, fLineWidthFromDecompose,
                                 basegfx::fTools::getSmallValue());
}

CPPUNIT_TEST_FIXTURE(DrawinglayerBorderTest, testDoublePixelProcessing)
{
    // Creating a pixel-processor and after that attaching a metafile
    // recording is not possible anymore, the pixel-processor may be
    // a SDPR, e.g. a CairoSDPR, and *not* a VclPixelProcessor2D anymore.
    // Since the intention had changed already (see comments below
    // where it is explained why two lines are expected nowadays)
    // it is also okay to just use a VclMetafileProcessor2D - to record
    // a metafile.
    ScopedVclPtrInstance<VirtualDevice> pDev;
    GDIMetaFile aMetaFile;
    aMetaFile.Record(pDev);
    drawinglayer::geometry::ViewInformation2D aView;

    // This creates a VclMetafileProcessor2D - the only processor that
    // (as the name states) can record metafiles
    std::unique_ptr<drawinglayer::processor2d::BaseProcessor2D> pProcessor(
        drawinglayer::processor2d::createProcessor2DFromOutputDevice(*pDev, aView));
    CPPUNIT_ASSERT(pProcessor);

    // Create a border line primitive that's similar to the one from the bugdoc:
    // 1.47 pixels is 0.03cm at 130% zoom and 96 DPI.
    basegfx::B2DPoint aStart(0, 20);
    basegfx::B2DPoint aEnd(100, 20);
    double const fLeftWidth = 1.47;
    double const fDistance = 1.47;
    double const fRightWidth = 1.47;
    double const fExtendLeftStart = 0;
    double const fExtendLeftEnd = 0;
    double const fExtendRightStart = 0;
    double const fExtendRightEnd = 0;
    basegfx::BColor aColorRight;
    basegfx::BColor aColorLeft;
    std::vector<double> aDashing(svtools::GetLineDashing(SvxBorderLineStyle::DOUBLE, 10.0));
    const drawinglayer::attribute::StrokeAttribute aStrokeAttribute(std::move(aDashing));
    std::vector<drawinglayer::primitive2d::BorderLine> aBorderlines{
        drawinglayer::primitive2d::BorderLine(
            drawinglayer::attribute::LineAttribute(aColorLeft, fLeftWidth), fExtendLeftStart,
            fExtendLeftStart, fExtendLeftEnd, fExtendLeftEnd),

        drawinglayer::primitive2d::BorderLine(fDistance),

        drawinglayer::primitive2d::BorderLine(
            drawinglayer::attribute::LineAttribute(aColorRight, fRightWidth), fExtendRightStart,
            fExtendRightStart, fExtendRightEnd, fExtendRightEnd)
    };

    rtl::Reference<drawinglayer::primitive2d::BorderLinePrimitive2D> aBorder(
        new drawinglayer::primitive2d::BorderLinePrimitive2D(aStart, aEnd, std::move(aBorderlines),
                                                             aStrokeAttribute));

    drawinglayer::primitive2d::Primitive2DContainer aPrimitives;
    aPrimitives.push_back(drawinglayer::primitive2d::Primitive2DReference(aBorder));

    // Process the primitives.
    pProcessor->process(aPrimitives);

    // Double line now gets decomposed in Metafile to painting four lines
    // with width == 0 in a cross pattern due to real line width being between
    // 1.0 and 2.0. Count created lines
    aMetaFile.Stop();
    aMetaFile.WindStart();
    sal_uInt32 nPolyLineActionCount = 0;

    for (std::size_t nAction = 0; nAction < aMetaFile.GetActionSize(); ++nAction)
    {
        MetaAction* pAction = aMetaFile.GetAction(nAction);

        if (MetaActionType::POLYLINE == pAction->GetType())
        {
            auto pMPLAction = static_cast<MetaPolyLineAction*>(pAction);

            if (0 != pMPLAction->GetLineInfo().GetWidth()
                && LineStyle::Solid == pMPLAction->GetLineInfo().GetStyle())
            {
                nPolyLineActionCount++;
            }
        }
    }

    // Check if all eight (2x four) simple lines with width == 0 and
    // solid were created
    //
    // This has changed: Now, just the needed 'real' lines get created
    // which have a width of 1. This are two lines. The former multiple
    // lines were a combination of view-dependent force to a single-pixel
    // line width (0 == lineWidth -> hairline) and vcl rendering this
    // using a (insane) combination of single non-AAed lines. All the
    // system-dependent part of the BorderLine stuff is now done in
    // SdrFrameBorderPrimitive2D and svx.
    // Adapted this test - still useful, breaking it may be a hint :-)
    const sal_uInt32 nExpectedNumPolyLineActions = 2;

    CPPUNIT_ASSERT_EQUAL(nExpectedNumPolyLineActions, nPolyLineActionCount);
}
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
