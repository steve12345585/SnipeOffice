/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <string_view>

#include <test/bootstrapfixture.hxx>

#include <com/sun/star/graphic/XGraphic.hpp>
#include <com/sun/star/graphic/GraphicType.hpp>
#include <com/sun/star/graphic/GraphicProvider.hpp>
#include <com/sun/star/graphic/XGraphicProvider.hpp>
#include <com/sun/star/awt/Size.hpp>

#include <comphelper/processfactory.hxx>
#include <comphelper/propertysequence.hxx>

namespace
{
using namespace css;

constexpr OUStringLiteral gaDataUrl = u"/vcl/qa/api/data/";

class XGraphicTest : public test::BootstrapFixture
{
public:
    XGraphicTest()
        : BootstrapFixture(true, false)
    {
    }

    OUString getFullUrl(std::u16string_view sFileName)
    {
        return m_directories.getURLFromSrc(gaDataUrl) + sFileName;
    }

    void testGraphic();
    void testGraphicDescriptor();
    void testGraphicProvider();

    CPPUNIT_TEST_SUITE(XGraphicTest);
    CPPUNIT_TEST(testGraphic);
    CPPUNIT_TEST(testGraphicDescriptor);
    CPPUNIT_TEST(testGraphicProvider);
    CPPUNIT_TEST_SUITE_END();
};

BitmapEx createBitmap()
{
    Bitmap aBitmap(Size(100, 50), vcl::PixelFormat::N24_BPP);
    aBitmap.Erase(COL_LIGHTRED);

    return BitmapEx(aBitmap);
}

void XGraphicTest::testGraphic()
{
    Graphic aGraphic;
    uno::Reference<graphic::XGraphic> xGraphic = aGraphic.GetXGraphic();
}

void XGraphicTest::testGraphicDescriptor()
{
    Graphic aGraphic(createBitmap());
    uno::Reference<graphic::XGraphic> xGraphic = aGraphic.GetXGraphic();
    uno::Reference<beans::XPropertySet> xGraphicDescriptor(xGraphic, uno::UNO_QUERY_THROW);

    //[property] byte GraphicType;
    sal_Int8 nType;
    CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"GraphicType"_ustr) >>= nType);
    CPPUNIT_ASSERT_EQUAL(graphic::GraphicType::PIXEL, nType);

    //[property] string MimeType;
    OUString sMimeType;
    CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"MimeType"_ustr) >>= sMimeType);
    CPPUNIT_ASSERT_EQUAL(u"image/x-vclgraphic"_ustr, sMimeType);

    //[optional, property] ::com::sun::star::awt::Size SizePixel;
    awt::Size aSizePixel;
    CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"SizePixel"_ustr) >>= aSizePixel);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(100), aSizePixel.Width);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(50), aSizePixel.Height);

    //[optional, property] ::com::sun::star::awt::Size Size100thMM;
    awt::Size aSize100thMM;
    CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"Size100thMM"_ustr) >>= aSize100thMM);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), aSize100thMM.Width);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), aSize100thMM.Height);

    //[optional, property] byte BitsPerPixel;
    sal_Int8 nBitsPerPixel;
    CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"BitsPerPixel"_ustr) >>= nBitsPerPixel);
    CPPUNIT_ASSERT_EQUAL(sal_Int8(24), nBitsPerPixel);

    //[optional, property] boolean Transparent;
    bool bTransparent;
    CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"Transparent"_ustr) >>= bTransparent);
    CPPUNIT_ASSERT_EQUAL(false, bTransparent);

    //[optional, property] boolean Alpha;
    bool bAlpha;
    CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"Alpha"_ustr) >>= bAlpha);
    CPPUNIT_ASSERT_EQUAL(false, bAlpha);

    //[optional, property] boolean Animated;
    bool bAnimated;
    CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"Animated"_ustr) >>= bAnimated);
    CPPUNIT_ASSERT_EQUAL(false, bAnimated);
}

void XGraphicTest::testGraphicProvider()
{
    OUString aGraphicURL = getFullUrl(u"TestGraphic.png");

    { // Load lazy
        uno::Reference<uno::XComponentContext> xContext(comphelper::getProcessComponentContext());
        uno::Reference<graphic::XGraphicProvider> xGraphicProvider;
        xGraphicProvider.set(graphic::GraphicProvider::create(xContext), uno::UNO_SET_THROW);

        auto aMediaProperties(comphelper::InitPropertySequence({
            { "URL", uno::Any(aGraphicURL) },
            { "LazyRead", uno::Any(true) },
            { "LoadAsLink", uno::Any(false) },
        }));

        uno::Reference<graphic::XGraphic> xGraphic(
            xGraphicProvider->queryGraphic(aMediaProperties));
        CPPUNIT_ASSERT(xGraphic.is());
        Graphic aGraphic(xGraphic);
        CPPUNIT_ASSERT_EQUAL(false, aGraphic.isAvailable());

        uno::Reference<beans::XPropertySet> xGraphicDescriptor(xGraphic, uno::UNO_QUERY_THROW);

        sal_Int8 nType;
        CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"GraphicType"_ustr) >>= nType);
        CPPUNIT_ASSERT_EQUAL(graphic::GraphicType::PIXEL, nType);

        awt::Size aSizePixel;
        CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"SizePixel"_ustr) >>= aSizePixel);
        CPPUNIT_ASSERT_EQUAL(sal_Int32(8), aSizePixel.Width);
        CPPUNIT_ASSERT_EQUAL(sal_Int32(8), aSizePixel.Height);

        bool bLinked;
        CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"Linked"_ustr) >>= bLinked);
        CPPUNIT_ASSERT_EQUAL(false, bLinked);

        OUString sOriginURL;
        CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"OriginURL"_ustr) >>= sOriginURL);
        CPPUNIT_ASSERT_EQUAL(OUString(), sOriginURL);

        CPPUNIT_ASSERT_EQUAL(false, aGraphic.isAvailable());
    }

    { // Load as link
        uno::Reference<uno::XComponentContext> xContext(comphelper::getProcessComponentContext());
        uno::Reference<graphic::XGraphicProvider> xGraphicProvider;
        xGraphicProvider.set(graphic::GraphicProvider::create(xContext), uno::UNO_SET_THROW);

        auto aMediaProperties(comphelper::InitPropertySequence({
            { "URL", uno::Any(aGraphicURL) },
            { "LazyRead", uno::Any(false) },
            { "LoadAsLink", uno::Any(true) },
        }));

        uno::Reference<graphic::XGraphic> xGraphic(
            xGraphicProvider->queryGraphic(aMediaProperties));
        CPPUNIT_ASSERT(xGraphic.is());
        Graphic aGraphic(xGraphic);
        CPPUNIT_ASSERT_EQUAL(true, aGraphic.isAvailable());

        uno::Reference<beans::XPropertySet> xGraphicDescriptor(xGraphic, uno::UNO_QUERY_THROW);

        sal_Int8 nType;
        CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"GraphicType"_ustr) >>= nType);
        CPPUNIT_ASSERT_EQUAL(graphic::GraphicType::PIXEL, nType);

        awt::Size aSizePixel;
        CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"SizePixel"_ustr) >>= aSizePixel);
        CPPUNIT_ASSERT_EQUAL(sal_Int32(8), aSizePixel.Width);
        CPPUNIT_ASSERT_EQUAL(sal_Int32(8), aSizePixel.Height);

        bool bLinked;
        CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"Linked"_ustr) >>= bLinked);
        CPPUNIT_ASSERT_EQUAL(true, bLinked);

        OUString sOriginURL;
        CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"OriginURL"_ustr) >>= sOriginURL);
        CPPUNIT_ASSERT_EQUAL(aGraphicURL, sOriginURL);
    }

    { // Load lazy and as link
        uno::Reference<uno::XComponentContext> xContext(comphelper::getProcessComponentContext());
        uno::Reference<graphic::XGraphicProvider> xGraphicProvider;
        xGraphicProvider.set(graphic::GraphicProvider::create(xContext), uno::UNO_SET_THROW);

        auto aMediaProperties(comphelper::InitPropertySequence({
            { "URL", uno::Any(aGraphicURL) },
            { "LazyRead", uno::Any(true) },
            { "LoadAsLink", uno::Any(true) },
        }));

        uno::Reference<graphic::XGraphic> xGraphic(
            xGraphicProvider->queryGraphic(aMediaProperties));
        CPPUNIT_ASSERT(xGraphic.is());
        Graphic aGraphic(xGraphic);

        CPPUNIT_ASSERT_EQUAL(false, aGraphic.isAvailable());

        uno::Reference<beans::XPropertySet> xGraphicDescriptor(xGraphic, uno::UNO_QUERY_THROW);

        sal_Int8 nType;
        CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"GraphicType"_ustr) >>= nType);
        CPPUNIT_ASSERT_EQUAL(graphic::GraphicType::PIXEL, nType);

        awt::Size aSizePixel;
        CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"SizePixel"_ustr) >>= aSizePixel);
        CPPUNIT_ASSERT_EQUAL(sal_Int32(8), aSizePixel.Width);
        CPPUNIT_ASSERT_EQUAL(sal_Int32(8), aSizePixel.Height);

        bool bLinked;
        CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"Linked"_ustr) >>= bLinked);
        CPPUNIT_ASSERT_EQUAL(true, bLinked);

        OUString sOriginURL;
        CPPUNIT_ASSERT(xGraphicDescriptor->getPropertyValue(u"OriginURL"_ustr) >>= sOriginURL);
        CPPUNIT_ASSERT_EQUAL(aGraphicURL, sOriginURL);

        CPPUNIT_ASSERT_EQUAL(false, aGraphic.isAvailable());
    }
}

} // namespace

CPPUNIT_TEST_SUITE_REGISTRATION(XGraphicTest);

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
