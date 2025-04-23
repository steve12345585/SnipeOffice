/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/bootstrapfixture.hxx>

#include <tools/stream.hxx>

#include <vcl/BitmapWriteAccess.hxx>
#include <vcl/bitmap/BitmapAlphaClampFilter.hxx>
#include <vcl/bitmap/BitmapArithmeticBlendFilter.hxx>
#include <vcl/bitmap/BitmapDarkenBlendFilter.hxx>
#include <vcl/bitmap/BitmapLightenBlendFilter.hxx>
#include <vcl/bitmap/BitmapMultiplyBlendFilter.hxx>
#include <vcl/bitmap/BitmapNormalBlendFilter.hxx>
#include <vcl/bitmap/BitmapScreenBlendFilter.hxx>
#include <vcl/bitmap/BitmapBasicMorphologyFilter.hxx>
#include <vcl/bitmap/BitmapFilterStackBlur.hxx>
#include <vcl/graphicfilter.hxx>

#include <BitmapSymmetryCheck.hxx>

#include <chrono>
#include <string_view>

namespace
{
constexpr bool constWriteResultBitmap(false);
constexpr bool constEnablePerformanceTest(false);

class BitmapFilterTest : public test::BootstrapFixture
{
public:
    BitmapFilterTest()
        : test::BootstrapFixture(true, false)
    {
    }

    void testClampAlpha();
    void testBlurCorrectness();
    void testBasicMorphology();
    void testPerformance();
    void testGenerateStripRanges();
    void testMultiplyBlendFilter();
    void testNormalBlendFilter();
    void testDarkenBlendFilter();
    void testLightenBlendFilter();
    void testScreenBlendFilter();
    void testArithmeticBlendFilter();

    CPPUNIT_TEST_SUITE(BitmapFilterTest);
    CPPUNIT_TEST(testClampAlpha);
    CPPUNIT_TEST(testBlurCorrectness);
    CPPUNIT_TEST(testBasicMorphology);
    CPPUNIT_TEST(testPerformance);
    CPPUNIT_TEST(testGenerateStripRanges);
    CPPUNIT_TEST(testMultiplyBlendFilter);
    CPPUNIT_TEST(testNormalBlendFilter);
    CPPUNIT_TEST(testDarkenBlendFilter);
    CPPUNIT_TEST(testLightenBlendFilter);
    CPPUNIT_TEST(testScreenBlendFilter);
    CPPUNIT_TEST(testArithmeticBlendFilter);
    CPPUNIT_TEST_SUITE_END();

private:
    OUString getFullUrl(std::u16string_view sFileName)
    {
        return m_directories.getURLFromSrc(u"vcl/qa/cppunit/data/") + sFileName;
    }

    BitmapEx loadBitmap(std::u16string_view sFileName)
    {
        Graphic aGraphic;
        const OUString aURL(getFullUrl(sFileName));
        SvFileStream aFileStream(aURL, StreamMode::READ);
        GraphicFilter& rFilter = GraphicFilter::GetGraphicFilter();
        ErrCode aResult = rFilter.ImportGraphic(aGraphic, aURL, aFileStream);
        CPPUNIT_ASSERT_EQUAL(ERRCODE_NONE, aResult);
        return aGraphic.GetBitmapEx();
    }

    template <class BitmapT> // handle both Bitmap and BitmapEx
    void savePNG(const OUString& sWhere, const BitmapT& rBmp)
    {
        SvFileStream aStream(sWhere, StreamMode::WRITE | StreamMode::TRUNC);
        GraphicFilter& rFilter = GraphicFilter::GetGraphicFilter();
        rFilter.compressAsPNG(BitmapEx(rBmp), aStream);
    }
};

void BitmapFilterTest::testClampAlpha()
{
    // Setup test bitmap
    Size aSize(1, 1);
    Bitmap aBitmap24Bit(aSize, vcl::PixelFormat::N24_BPP);

    {
        BitmapScopedWriteAccess aWriteAccess(aBitmap24Bit);
        aWriteAccess->Erase(COL_RED);
    }

    BitmapEx aBitmapEx24Bit(aBitmap24Bit);
    BitmapFilter::Filter(aBitmapEx24Bit, BitmapAlphaClampFilter(0x7F));
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_uInt8>(0xFF), aBitmapEx24Bit.GetAlpha(0, 0));
}

void BitmapFilterTest::testBlurCorrectness()
{
    // Setup test bitmap
    Size aSize(41, 31);
    Bitmap aBitmap24Bit(aSize, vcl::PixelFormat::N24_BPP);

    ScanlineFormat scanlineFormat = ScanlineFormat::NONE;
    auto ePixelFormat = aBitmap24Bit.getPixelFormat();

    {
        tools::Long aMargin1 = 1;
        tools::Long aMargin2 = 3;
        BitmapScopedWriteAccess aWriteAccess(aBitmap24Bit);
        scanlineFormat = aWriteAccess->GetScanlineFormat();
        aWriteAccess->Erase(COL_WHITE);
        aWriteAccess->SetLineColor(COL_BLACK);

        tools::Rectangle aRectangle1(aMargin1, aMargin1, aSize.Width() - 1 - aMargin1,
                                     aSize.Height() - 1 - aMargin1);

        tools::Rectangle aRectangle2(aMargin2, aMargin2, aSize.Width() - 1 - aMargin2,
                                     aSize.Height() - 1 - aMargin2);

        tools::Rectangle aRectangle3(aSize.Width() / 2, aSize.Height() / 2, aSize.Width() / 2,
                                     aSize.Height() / 2);

        aWriteAccess->DrawRect(aRectangle1);
        aWriteAccess->DrawRect(aRectangle2);
        aWriteAccess->DrawRect(aRectangle3);
    }

    if (constWriteResultBitmap)
    {
        savePNG(u"~/blurBefore.png"_ustr, aBitmap24Bit);
    }

    // Perform blur
    BitmapFilterStackBlur aBlurFilter(2);
    aBitmap24Bit = aBlurFilter.filter(aBitmap24Bit);

    // Check the result

    if (constWriteResultBitmap)
    {
        savePNG(u"~/blurAfter.png"_ustr, aBitmap24Bit);
    }

    // Check blurred bitmap parameters
    CPPUNIT_ASSERT_EQUAL(static_cast<tools::Long>(41), aBitmap24Bit.GetSizePixel().Width());
    CPPUNIT_ASSERT_EQUAL(static_cast<tools::Long>(31), aBitmap24Bit.GetSizePixel().Height());

    CPPUNIT_ASSERT_EQUAL(ePixelFormat, aBitmap24Bit.getPixelFormat());

    // Check that the bitmap is horizontally and vertically symmetrical
    CPPUNIT_ASSERT(BitmapSymmetryCheck::check(aBitmap24Bit));

    {
        BitmapScopedReadAccess aReadAccess(aBitmap24Bit);
        CPPUNIT_ASSERT_EQUAL(scanlineFormat, aReadAccess->GetScanlineFormat());
    }
}

void BitmapFilterTest::testBasicMorphology()
{
    const BitmapEx aOrigBitmap = loadBitmap(u"testBasicMorphology.png");
    const BitmapEx aRefBitmapDilated1 = loadBitmap(u"testBasicMorphologyDilated1.png");
    const BitmapEx aRefBitmapDilated1Eroded1
        = loadBitmap(u"testBasicMorphologyDilated1Eroded1.png");
    const BitmapEx aRefBitmapDilated2 = loadBitmap(u"testBasicMorphologyDilated2.png");
    const BitmapEx aRefBitmapDilated2Eroded1
        = loadBitmap(u"testBasicMorphologyDilated2Eroded1.png");

    BitmapEx aTransformBitmap = aOrigBitmap;
    BitmapFilter::Filter(aTransformBitmap, BitmapDilateFilter(1));
    if (constWriteResultBitmap)
        savePNG(u"~/Dilated1.png"_ustr, aTransformBitmap);
    CPPUNIT_ASSERT_EQUAL(aRefBitmapDilated1.GetChecksum(), aTransformBitmap.GetChecksum());
    BitmapFilter::Filter(aTransformBitmap, BitmapErodeFilter(1));
    if (constWriteResultBitmap)
        savePNG(u"~/Dilated1Eroded1.png"_ustr, aTransformBitmap);
    CPPUNIT_ASSERT_EQUAL(aRefBitmapDilated1Eroded1.GetChecksum(), aTransformBitmap.GetChecksum());

    aTransformBitmap = aOrigBitmap;
    BitmapFilter::Filter(aTransformBitmap, BitmapDilateFilter(2));
    if (constWriteResultBitmap)
        savePNG(u"~/Dilated2.png"_ustr, aTransformBitmap);
    CPPUNIT_ASSERT_EQUAL(aRefBitmapDilated2.GetChecksum(), aTransformBitmap.GetChecksum());
    BitmapFilter::Filter(aTransformBitmap, BitmapErodeFilter(1));
    if (constWriteResultBitmap)
        savePNG(u"~/Dilated2Eroded1.png"_ustr, aTransformBitmap);
    CPPUNIT_ASSERT_EQUAL(aRefBitmapDilated2Eroded1.GetChecksum(), aTransformBitmap.GetChecksum());
}

void BitmapFilterTest::testPerformance()
{
    if (!constEnablePerformanceTest)
        return;

    Size aSize(4000, 3000); // A rather common picture size

    // Prepare bitmap
    Bitmap aBigBitmap(aSize, vcl::PixelFormat::N24_BPP);
    {
        tools::Long aMargin = 500;
        BitmapScopedWriteAccess aWriteAccess(aBigBitmap);
        aWriteAccess->Erase(COL_WHITE);
        aWriteAccess->SetLineColor(COL_BLACK);
        aWriteAccess->SetFillColor(COL_BLACK);
        tools::Rectangle aRectangle(aMargin, aMargin, aSize.Width() - 1 - aMargin,
                                    aSize.Height() - 1 - aMargin);

        aWriteAccess->DrawRect(aRectangle);
    }

    int nIterations = 10;
    auto start = std::chrono::high_resolution_clock::now();
    Bitmap aResult;
    for (int i = 0; i < nIterations; i++)
    {
        BitmapFilterStackBlur aBlurFilter(250);
        aResult = aBlurFilter.filter(aBigBitmap);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = (end - start) / nIterations;

    if (constWriteResultBitmap)
    {
        std::unique_ptr<SvFileStream> pStream(new SvFileStream(
            u"~/BlurBigPerformance.png"_ustr, StreamMode::WRITE | StreamMode::TRUNC));
        GraphicFilter& rFilter = GraphicFilter::GetGraphicFilter();
        rFilter.compressAsPNG(BitmapEx(aResult), *pStream);

        pStream.reset(new SvFileStream(u"~/BlurBigPerformance.txt"_ustr, StreamMode::WRITE));
        pStream->WriteOString("Blur average time: ");
        pStream->WriteOString(OString::number(
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()));
        pStream->WriteOString("\n");
    }
}

void BitmapFilterTest::testGenerateStripRanges()
{
    {
        constexpr tools::Long nFirstIndex = 0;
        constexpr tools::Long nLastIndex = 100;
        constexpr tools::Long nStripSize = 32;

        std::vector<std::tuple<tools::Long, tools::Long, bool>> aRanges;

        vcl::bitmap::generateStripRanges<nStripSize>(
            nFirstIndex, nLastIndex,
            [&](tools::Long const nStart, tools::Long const nEnd, bool const bLast) {
                aRanges.emplace_back(nStart, nEnd, bLast);
            });

        CPPUNIT_ASSERT_EQUAL(size_t(4), aRanges.size());

        CPPUNIT_ASSERT_EQUAL(tools::Long(0), std::get<0>(aRanges[0]));
        CPPUNIT_ASSERT_EQUAL(tools::Long(31), std::get<1>(aRanges[0]));
        CPPUNIT_ASSERT_EQUAL(false, std::get<2>(aRanges[0]));

        CPPUNIT_ASSERT_EQUAL(tools::Long(32), std::get<0>(aRanges[1]));
        CPPUNIT_ASSERT_EQUAL(tools::Long(63), std::get<1>(aRanges[1]));
        CPPUNIT_ASSERT_EQUAL(false, std::get<2>(aRanges[1]));

        CPPUNIT_ASSERT_EQUAL(tools::Long(64), std::get<0>(aRanges[2]));
        CPPUNIT_ASSERT_EQUAL(tools::Long(95), std::get<1>(aRanges[2]));
        CPPUNIT_ASSERT_EQUAL(false, std::get<2>(aRanges[2]));

        CPPUNIT_ASSERT_EQUAL(tools::Long(96), std::get<0>(aRanges[3]));
        CPPUNIT_ASSERT_EQUAL(tools::Long(100), std::get<1>(aRanges[3]));
        CPPUNIT_ASSERT_EQUAL(true, std::get<2>(aRanges[3]));
    }

    {
        constexpr tools::Long nFirstIndex = 0;
        constexpr tools::Long nLastIndex = 95;
        constexpr tools::Long nStripSize = 32;

        std::vector<std::tuple<tools::Long, tools::Long, bool>> aRanges;

        vcl::bitmap::generateStripRanges<nStripSize>(
            nFirstIndex, nLastIndex,
            [&](tools::Long const nStart, tools::Long const nEnd, bool const bLast) {
                aRanges.emplace_back(nStart, nEnd, bLast);
            });

        CPPUNIT_ASSERT_EQUAL(size_t(3), aRanges.size());

        CPPUNIT_ASSERT_EQUAL(tools::Long(0), std::get<0>(aRanges[0]));
        CPPUNIT_ASSERT_EQUAL(tools::Long(31), std::get<1>(aRanges[0]));
        CPPUNIT_ASSERT_EQUAL(false, std::get<2>(aRanges[0]));

        CPPUNIT_ASSERT_EQUAL(tools::Long(32), std::get<0>(aRanges[1]));
        CPPUNIT_ASSERT_EQUAL(tools::Long(63), std::get<1>(aRanges[1]));
        CPPUNIT_ASSERT_EQUAL(false, std::get<2>(aRanges[1]));

        CPPUNIT_ASSERT_EQUAL(tools::Long(64), std::get<0>(aRanges[2]));
        CPPUNIT_ASSERT_EQUAL(tools::Long(95), std::get<1>(aRanges[2]));
        CPPUNIT_ASSERT_EQUAL(true, std::get<2>(aRanges[2]));
    }
}

void BitmapFilterTest::testMultiplyBlendFilter()
{
    Bitmap aRedBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aRedBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aRedBitmap);
        aWriteAccess->Erase(COL_LIGHTRED);
    }

    Bitmap aGreenBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aGreenBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aGreenBitmap);
        aWriteAccess->Erase(COL_GREEN);
    }

    Bitmap aTransparentBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aTransparentBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aTransparentBitmap);
        aWriteAccess->Erase(COL_AUTO);
    }

    BitmapEx aRedBitmapEx(aRedBitmap);
    BitmapEx aGreenBitmapEx(aGreenBitmap);
    BitmapEx aTransparentBitmapEx(aTransparentBitmap);

    // same color
    {
        BitmapMultiplyBlendFilter* pArithmeticFilter
            = new BitmapMultiplyBlendFilter(aRedBitmapEx, aRedBitmapEx);
        BitmapEx aResBitmapEx = pArithmeticFilter->execute();
        CPPUNIT_ASSERT_EQUAL(COL_LIGHTRED, aResBitmapEx.GetPixelColor(2, 2));
    }

    // different color
    {
        BitmapMultiplyBlendFilter* pArithmeticFilter
            = new BitmapMultiplyBlendFilter(aRedBitmapEx, aGreenBitmapEx);
        BitmapEx aResBitmapEx = pArithmeticFilter->execute();
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0xFF, 0x00, 0x00, 0x00),
                             aResBitmapEx.GetPixelColor(2, 2));
    }

    // transparent
    {
        BitmapMultiplyBlendFilter* pArithmeticFilter
            = new BitmapMultiplyBlendFilter(aRedBitmapEx, aTransparentBitmapEx);
        BitmapEx aResBitmapEx = pArithmeticFilter->execute();
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0xFF, 0xFF, 0x00, 0x00),
                             aResBitmapEx.GetPixelColor(2, 2));
    }
}

void BitmapFilterTest::testNormalBlendFilter()
{
    Bitmap aRedBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aRedBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aRedBitmap);
        aWriteAccess->Erase(COL_LIGHTRED);
    }

    Bitmap aGreenBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aGreenBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aGreenBitmap);
        aWriteAccess->Erase(COL_GREEN);
    }

    Bitmap aTransparentBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aTransparentBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aTransparentBitmap);
        aWriteAccess->Erase(COL_AUTO);
    }

    BitmapEx aRedBitmapEx(aRedBitmap);
    BitmapEx aGreenBitmapEx(aGreenBitmap);
    BitmapEx aTransparentBitmapEx(aTransparentBitmap);

    // same color
    {
        BitmapNormalBlendFilter* pArithmeticFilter
            = new BitmapNormalBlendFilter(aRedBitmapEx, aRedBitmapEx);
        BitmapEx aResBitmapEx = pArithmeticFilter->execute();
        CPPUNIT_ASSERT_EQUAL(COL_LIGHTRED, aResBitmapEx.GetPixelColor(2, 2));
    }

    // different color
    {
        BitmapNormalBlendFilter* pArithmeticFilter
            = new BitmapNormalBlendFilter(aRedBitmapEx, aGreenBitmapEx);
        BitmapEx aResBitmapEx = pArithmeticFilter->execute();
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0xFF, 0xFF, 0x00, 0x00),
                             aResBitmapEx.GetPixelColor(2, 2));
    }

    // transparent
    {
        BitmapNormalBlendFilter* pArithmeticFilter
            = new BitmapNormalBlendFilter(aRedBitmapEx, aTransparentBitmapEx);
        BitmapEx aResBitmapEx = pArithmeticFilter->execute();
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0xFF, 0xFF, 0x00, 0x00),
                             aResBitmapEx.GetPixelColor(2, 2));
    }
}

void BitmapFilterTest::testDarkenBlendFilter()
{
    Bitmap aRedBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aRedBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aRedBitmap);
        aWriteAccess->Erase(COL_LIGHTRED);
    }

    Bitmap aGreenBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aGreenBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aGreenBitmap);
        aWriteAccess->Erase(COL_GREEN);
    }

    Bitmap aTransparentBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aTransparentBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aTransparentBitmap);
        aWriteAccess->Erase(COL_AUTO);
    }

    BitmapEx aRedBitmapEx(aRedBitmap);
    BitmapEx aGreenBitmapEx(aGreenBitmap);
    BitmapEx aTransparentBitmapEx(aTransparentBitmap);

    // same color
    {
        BitmapDarkenBlendFilter aArithmeticFilter(aRedBitmapEx);
        BitmapEx aResBitmapEx = aArithmeticFilter.execute(aRedBitmapEx);
        CPPUNIT_ASSERT_EQUAL(COL_LIGHTRED, aResBitmapEx.GetPixelColor(2, 2));
    }

    // different color
    {
        BitmapDarkenBlendFilter aArithmeticFilter(aGreenBitmapEx);
        BitmapEx aResBitmapEx = aArithmeticFilter.execute(aRedBitmapEx);
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0xFF, 0x00, 0x00, 0x00),
                             aResBitmapEx.GetPixelColor(2, 2));
    }

    // transparent
    {
        BitmapDarkenBlendFilter aArithmeticFilter(aTransparentBitmapEx);
        BitmapEx aResBitmapEx = aArithmeticFilter.execute(aRedBitmapEx);
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0xFF, 0xFF, 0x00, 0x00),
                             aResBitmapEx.GetPixelColor(2, 2));
    }
}

void BitmapFilterTest::testLightenBlendFilter()
{
    Bitmap aRedBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aRedBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aRedBitmap);
        aWriteAccess->Erase(COL_LIGHTRED);
    }

    Bitmap aGreenBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aGreenBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aGreenBitmap);
        aWriteAccess->Erase(COL_GREEN);
    }

    Bitmap aTransparentBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aTransparentBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aTransparentBitmap);
        aWriteAccess->Erase(COL_AUTO);
    }

    BitmapEx aRedBitmapEx(aRedBitmap);
    BitmapEx aGreenBitmapEx(aGreenBitmap);
    BitmapEx aTransparentBitmapEx(aTransparentBitmap);

    // same color
    {
        BitmapLightenBlendFilter* pArithmeticFilter
            = new BitmapLightenBlendFilter(aRedBitmapEx, aRedBitmapEx);
        BitmapEx aResBitmapEx = pArithmeticFilter->execute();
        CPPUNIT_ASSERT_EQUAL(COL_LIGHTRED, aResBitmapEx.GetPixelColor(2, 2));
    }

    // different color
    {
        BitmapLightenBlendFilter* pArithmeticFilter
            = new BitmapLightenBlendFilter(aRedBitmapEx, aGreenBitmapEx);
        BitmapEx aResBitmapEx = pArithmeticFilter->execute();
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0xFF, 0xFF, 0x80, 0x00),
                             aResBitmapEx.GetPixelColor(2, 2));
    }

    // transparent
    {
        BitmapLightenBlendFilter* pArithmeticFilter
            = new BitmapLightenBlendFilter(aRedBitmapEx, aTransparentBitmapEx);
        BitmapEx aResBitmapEx = pArithmeticFilter->execute();
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0xFF, 0xFF, 0xFF, 0xFF),
                             aResBitmapEx.GetPixelColor(2, 2));
    }
}

void BitmapFilterTest::testScreenBlendFilter()
{
    Bitmap aRedBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aRedBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aRedBitmap);
        aWriteAccess->Erase(COL_LIGHTRED);
    }

    Bitmap aGreenBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aGreenBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aGreenBitmap);
        aWriteAccess->Erase(COL_GREEN);
    }

    Bitmap aTransparentBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aTransparentBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aTransparentBitmap);
        aWriteAccess->Erase(COL_AUTO);
    }

    BitmapEx aRedBitmapEx(aRedBitmap);
    BitmapEx aGreenBitmapEx(aGreenBitmap);
    BitmapEx aTransparentBitmapEx(aTransparentBitmap);

    // same color
    {
        BitmapScreenBlendFilter* pArithmeticFilter
            = new BitmapScreenBlendFilter(aRedBitmapEx, aRedBitmapEx);
        BitmapEx aResBitmapEx = pArithmeticFilter->execute();
        CPPUNIT_ASSERT_EQUAL(COL_LIGHTRED, aResBitmapEx.GetPixelColor(2, 2));
    }

    // different color
    {
        BitmapScreenBlendFilter* pArithmeticFilter
            = new BitmapScreenBlendFilter(aRedBitmapEx, aGreenBitmapEx);
        BitmapEx aResBitmapEx = pArithmeticFilter->execute();
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0xFF, 0xFF, 0x80, 0x00),
                             aResBitmapEx.GetPixelColor(2, 2));
    }

    // transparent
    {
        BitmapScreenBlendFilter* pArithmeticFilter
            = new BitmapScreenBlendFilter(aRedBitmapEx, aTransparentBitmapEx);
        BitmapEx aResBitmapEx = pArithmeticFilter->execute();
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0xFF, 0xFF, 0xFF, 0xFF),
                             aResBitmapEx.GetPixelColor(2, 2));
    }
}

void BitmapFilterTest::testArithmeticBlendFilter()
{
    Bitmap aRedBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aRedBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aRedBitmap);
        aWriteAccess->Erase(COL_LIGHTRED);
    }

    Bitmap aGreenBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aGreenBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aGreenBitmap);
        aWriteAccess->Erase(COL_GREEN);
    }

    Bitmap aTransparentBitmap(Size(4, 4), vcl::PixelFormat::N24_BPP);
    CPPUNIT_ASSERT_EQUAL(vcl::PixelFormat::N24_BPP, aTransparentBitmap.getPixelFormat());
    {
        BitmapScopedWriteAccess aWriteAccess(aTransparentBitmap);
        aWriteAccess->Erase(COL_AUTO);
    }

    BitmapEx aRedBitmapEx(aRedBitmap);
    BitmapEx aGreenBitmapEx(aGreenBitmap);
    BitmapEx aTransparentBitmapEx(aTransparentBitmap);

    // same color
    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx, BitmapArithmeticBlendFilter(aRedBitmapEx, 0, 0, 0, 0));

        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0x00, 0x00, 0x00, 0x00),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx, BitmapArithmeticBlendFilter(aRedBitmapEx, 1, 0, 0, 0));
        CPPUNIT_ASSERT_EQUAL(COL_LIGHTRED, aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx, BitmapArithmeticBlendFilter(aRedBitmapEx, 0, 1, 0, 0));
        CPPUNIT_ASSERT_EQUAL(COL_LIGHTRED, aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx, BitmapArithmeticBlendFilter(aRedBitmapEx, 0, 0, 1, 0));
        CPPUNIT_ASSERT_EQUAL(COL_LIGHTRED, aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx, BitmapArithmeticBlendFilter(aRedBitmapEx, 0, 0, 0, 1));
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0xFF, 0xFF, 0xFF, 0xFF),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx, BitmapArithmeticBlendFilter(aRedBitmapEx, 0.5, 0, 0, 0));
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0x7F, 0xFF, 0x00, 0x00),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx, BitmapArithmeticBlendFilter(aRedBitmapEx, 0, 0.5, 0, 0));
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0x7F, 0xFF, 0x00, 0x00),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx, BitmapArithmeticBlendFilter(aRedBitmapEx, 0, 0, 0.5, 0));
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0x7F, 0xFF, 0x00, 0x00),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx, BitmapArithmeticBlendFilter(aRedBitmapEx, 0, 0, 0, 0.5));
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0x7F, 0xFF, 0xFF, 0xFF),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    // Different colors
    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx, BitmapArithmeticBlendFilter(aGreenBitmapEx, 0, 0, 0, 0));
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0x00, 0x00, 0x00, 0x00),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx, BitmapArithmeticBlendFilter(aGreenBitmapEx, 1, 0, 0, 0));
        CPPUNIT_ASSERT_EQUAL(COL_BLACK, aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx, BitmapArithmeticBlendFilter(aGreenBitmapEx, 0, 1, 0, 0));
        CPPUNIT_ASSERT_EQUAL(COL_LIGHTRED, aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx, BitmapArithmeticBlendFilter(aGreenBitmapEx, 0, 0, 1, 0));
        CPPUNIT_ASSERT_EQUAL(COL_GREEN, aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx, BitmapArithmeticBlendFilter(aGreenBitmapEx, 0, 0, 0, 1));
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0xFF, 0xFF, 0xFF, 0xFF),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx,
                             BitmapArithmeticBlendFilter(aGreenBitmapEx, 0.5, 0, 0, 0));
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0x7F, 0x00, 0x00, 0x00),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx,
                             BitmapArithmeticBlendFilter(aGreenBitmapEx, 0, 0.5, 0, 0));
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0x7F, 0xFF, 0x00, 0x00),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx,
                             BitmapArithmeticBlendFilter(aGreenBitmapEx, 0, 0, 0.5, 0));
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0x7F, 0x00, 0x81, 0x00),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx,
                             BitmapArithmeticBlendFilter(aGreenBitmapEx, 0, 0, 0, 0.5));
        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0x7F, 0xFF, 0xFF, 0xFF),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    // transparent
    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx,
                             BitmapArithmeticBlendFilter(aTransparentBitmapEx, 0, 0, 0, 0));

        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0x00, 0x00, 0x00, 0x00),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx,
                             BitmapArithmeticBlendFilter(aTransparentBitmapEx, 1, 0, 0, 0));

        CPPUNIT_ASSERT_EQUAL(COL_LIGHTRED, aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx,
                             BitmapArithmeticBlendFilter(aTransparentBitmapEx, 0, 1, 0, 0));

        CPPUNIT_ASSERT_EQUAL(COL_LIGHTRED, aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx,
                             BitmapArithmeticBlendFilter(aTransparentBitmapEx, 0, 0, 1, 0));

        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0xFF, 0xFF, 0xFF, 0xFF),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx,
                             BitmapArithmeticBlendFilter(aTransparentBitmapEx, 0, 0, 0, 1));

        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0xFF, 0xFF, 0xFF, 0xFF),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx,
                             BitmapArithmeticBlendFilter(aTransparentBitmapEx, 0.5, 0, 0, 0));

        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0x7F, 0xFF, 0x00, 0x00),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx,
                             BitmapArithmeticBlendFilter(aTransparentBitmapEx, 0, 0.5, 0, 0));

        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0x7F, 0xFF, 0x00, 0x00),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx,
                             BitmapArithmeticBlendFilter(aTransparentBitmapEx, 0, 0, 0.5, 0));

        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0x7F, 0xFF, 0xFF, 0xFF),
                             aResBitmapEx.GetPixelColor(0, 0));
    }

    {
        BitmapEx aResBitmapEx(aRedBitmapEx);
        BitmapFilter::Filter(aResBitmapEx,
                             BitmapArithmeticBlendFilter(aTransparentBitmapEx, 0, 0, 0, 0.5));

        CPPUNIT_ASSERT_EQUAL(Color(ColorAlpha, 0x7F, 0xFF, 0xFF, 0xFF),
                             aResBitmapEx.GetPixelColor(0, 0));
    }
}

} // namespace

CPPUNIT_TEST_SUITE_REGISTRATION(BitmapFilterTest);

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
