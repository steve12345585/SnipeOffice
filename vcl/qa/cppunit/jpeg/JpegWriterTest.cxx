/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#include <sal/config.h>

#include <string_view>

#include <unotest/bootstrapfixturebase.hxx>
#include <vcl/graphicfilter.hxx>
#include <vcl/BitmapReadAccess.hxx>
#include <tools/stream.hxx>
#include <graphic/GraphicFormatDetector.hxx>

constexpr OUStringLiteral gaDataUrl(u"/vcl/qa/cppunit/jpeg/data/");

class JpegWriterTest : public test::BootstrapFixtureBase
{
    OUString getFullUrl(std::u16string_view sFileName)
    {
        return m_directories.getURLFromSrc(gaDataUrl) + sFileName;
    }

    BitmapEx load(const OUString& aURL);
    BitmapEx roundtripJPG(const BitmapEx& bitmap);
    BitmapEx roundtripJPG(const OUString& aURL);

public:
    void testWrite8BitGrayscale();
    void testWrite8BitNonGrayscale();

    CPPUNIT_TEST_SUITE(JpegWriterTest);
    CPPUNIT_TEST(testWrite8BitGrayscale);
    CPPUNIT_TEST(testWrite8BitNonGrayscale);
    CPPUNIT_TEST_SUITE_END();
};

BitmapEx JpegWriterTest::load(const OUString& aURL)
{
    GraphicFilter& rFilter = GraphicFilter::GetGraphicFilter();
    Graphic aGraphic;
    SvFileStream aFileStream(aURL, StreamMode::READ);
    ErrCode bResult = rFilter.ImportGraphic(aGraphic, aURL, aFileStream);
    CPPUNIT_ASSERT_EQUAL(ERRCODE_NONE, bResult);
    return aGraphic.GetBitmapEx();
}

BitmapEx JpegWriterTest::roundtripJPG(const OUString& aURL) { return roundtripJPG(load(aURL)); }

BitmapEx JpegWriterTest::roundtripJPG(const BitmapEx& bitmap)
{
    // EXPORT JPEG
    SvMemoryStream aStream;
    GraphicFilter& rFilter = GraphicFilter::GetGraphicFilter();
    sal_uInt16 exportFormatJPG = rFilter.GetExportFormatNumberForShortName(JPG_SHORTNAME);
    Graphic aExportGraphic(bitmap);
    ErrCode bResult = rFilter.ExportGraphic(aExportGraphic, u"memory", aStream, exportFormatJPG);
    CPPUNIT_ASSERT_EQUAL(ERRCODE_NONE, bResult);
    //Detect the magic bytes - we need to be sure the file is actually a JPEG
    aStream.Seek(0);
    vcl::GraphicFormatDetector aDetector(aStream, u""_ustr);
    CPPUNIT_ASSERT(aDetector.detect());
    CPPUNIT_ASSERT(aDetector.checkJPG());
    // IMPORT JPEG
    aStream.Seek(0);
    Graphic aImportGraphic;
    sal_uInt16 importFormatJPG = rFilter.GetImportFormatNumberForShortName(JPG_SHORTNAME);
    bResult = rFilter.ImportGraphic(aImportGraphic, u"memory", aStream, importFormatJPG);
    CPPUNIT_ASSERT_EQUAL(ERRCODE_NONE, bResult);
    return aImportGraphic.GetBitmapEx();
}

void JpegWriterTest::testWrite8BitGrayscale()
{
    Bitmap bitmap = roundtripJPG(getFullUrl(u"8BitGrayscale.jpg")).GetBitmap();
    BitmapScopedReadAccess access(bitmap);
    const ScanlineFormat format = access->GetScanlineFormat();
    // Check that it's still 8bit grayscale.
    CPPUNIT_ASSERT_EQUAL(ScanlineFormat::N8BitPal, format);
    CPPUNIT_ASSERT(bitmap.HasGreyPalette8Bit());
    // Check that the content is valid.
    CPPUNIT_ASSERT_EQUAL(BitmapColor(COL_WHITE), access->GetColor(0, 0));
    CPPUNIT_ASSERT_EQUAL(BitmapColor(COL_WHITE), access->GetColor(0, access->Width() - 1));
    CPPUNIT_ASSERT_EQUAL(BitmapColor(COL_WHITE), access->GetColor(access->Height() - 1, 0));
    CPPUNIT_ASSERT_EQUAL(BitmapColor(COL_BLACK),
                         access->GetColor(access->Height() - 1, access->Width() - 1));
}

void JpegWriterTest::testWrite8BitNonGrayscale()
{
    Bitmap bitmap = roundtripJPG(getFullUrl(u"8BitNonGrayscale.gif")).GetBitmap();
    BitmapScopedReadAccess access(bitmap);
    const ScanlineFormat format = access->GetScanlineFormat();
    // Check that it's still 8bit grayscale.
    CPPUNIT_ASSERT_EQUAL(ScanlineFormat::N8BitPal, format);
    // The original image has grayscale palette, just with entries in a different order.
    // Do not check for grayscale 8bit, the roundtrip apparently fixes that. What's important
    // is the content.
    CPPUNIT_ASSERT(bitmap.HasGreyPaletteAny());
    // CPPUNIT_ASSERT(bitmap.HasGreyPalette8Bit());
    // Check that the content is valid.
    CPPUNIT_ASSERT_EQUAL(BitmapColor(COL_WHITE), access->GetColor(0, 0));
    CPPUNIT_ASSERT_EQUAL(BitmapColor(COL_WHITE), access->GetColor(0, access->Width() - 1));
    CPPUNIT_ASSERT_EQUAL(BitmapColor(COL_WHITE), access->GetColor(access->Height() - 1, 0));
    CPPUNIT_ASSERT_EQUAL(BitmapColor(COL_BLACK),
                         access->GetColor(access->Height() - 1, access->Width() - 1));
}

CPPUNIT_TEST_SUITE_REGISTRATION(JpegWriterTest);

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
