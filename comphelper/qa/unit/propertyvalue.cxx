/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <com/sun/star/awt/Size.hpp>

#include <comphelper/propertyvalue.hxx>
#include <comphelper/propertysequence.hxx>
#include <cppu/unotype.hxx>
#include <o3tl/any.hxx>

using namespace com::sun::star;

namespace
{
class MakePropertyValueTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(MakePropertyValueTest);
    CPPUNIT_TEST(testLvalue);
    CPPUNIT_TEST(testRvalue);
    CPPUNIT_TEST(testBitField);
    CPPUNIT_TEST(testJson);
    CPPUNIT_TEST(testJsonAwtSize);
    CPPUNIT_TEST_SUITE_END();

    void testLvalue()
    {
        sal_Int32 const i = 123;
        auto const v = comphelper::makePropertyValue(u"test"_ustr, i);
        CPPUNIT_ASSERT_EQUAL(cppu::UnoType<sal_Int32>::get(), v.Value.getValueType());
        CPPUNIT_ASSERT_EQUAL(sal_Int32(123), *o3tl::doAccess<sal_Int32>(v.Value));
    }

    void testRvalue()
    {
        auto const v = comphelper::makePropertyValue(u"test"_ustr, sal_Int32(456));
        CPPUNIT_ASSERT_EQUAL(cppu::UnoType<sal_Int32>::get(), v.Value.getValueType());
        CPPUNIT_ASSERT_EQUAL(sal_Int32(456), *o3tl::doAccess<sal_Int32>(v.Value));
    }

    void testBitField()
    {
        struct
        {
            bool b : 1;
        } s = { false };
        auto const v = comphelper::makePropertyValue(u"test"_ustr, s.b);
        CPPUNIT_ASSERT_EQUAL(cppu::UnoType<bool>::get(), v.Value.getValueType());
        CPPUNIT_ASSERT_EQUAL(false, *o3tl::doAccess<bool>(v.Value));
    }

    void testJson()
    {
        std::vector<beans::PropertyValue> aRet = comphelper::JsonToPropertyValues(R"json(
{
    "FieldType": {
        "type": "string",
        "value": "vnd.oasis.opendocument.field.UNHANDLED"
    },
    "FieldCommandPrefix": {
        "type": "string",
        "value": "ADDIN ZOTERO_ITEM"
    },
    "Fields": {
        "type": "[][]com.sun.star.beans.PropertyValue",
        "value": [
            {
                "FieldType": {
                    "type": "string",
                    "value": "vnd.oasis.opendocument.field.UNHANDLED"
                },
                "FieldCommand": {
                    "type": "string",
                    "value": "ADDIN ZOTERO_ITEM new command 1"
                },
                "Fields": {
                    "type": "string",
                    "value": "new result 1"
                }
            },
            {
                "FieldType": {
                    "type": "string",
                    "value": "vnd.oasis.opendocument.field.UNHANDLED"
                },
                "FieldCommandPrefix": {
                    "type": "string",
                    "value": "ADDIN ZOTERO_ITEM new command 2"
                },
                "Fields": {
                    "type": "string",
                    "value": "new result 2"
                }
            }
        ]
    }
}
)json");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3), aRet.size());
        beans::PropertyValue aFirst = aRet[0];
        CPPUNIT_ASSERT_EQUAL(u"FieldType"_ustr, aFirst.Name);
        CPPUNIT_ASSERT_EQUAL(u"vnd.oasis.opendocument.field.UNHANDLED"_ustr,
                             aFirst.Value.get<OUString>());
        beans::PropertyValue aSecond = aRet[1];
        CPPUNIT_ASSERT_EQUAL(u"FieldCommandPrefix"_ustr, aSecond.Name);
        CPPUNIT_ASSERT_EQUAL(u"ADDIN ZOTERO_ITEM"_ustr, aSecond.Value.get<OUString>());
        beans::PropertyValue aThird = aRet[2];
        CPPUNIT_ASSERT_EQUAL(u"Fields"_ustr, aThird.Name);
        uno::Sequence<uno::Sequence<beans::PropertyValue>> aSeqs;
        aThird.Value >>= aSeqs;
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(2), aSeqs.getLength());
        uno::Sequence<beans::PropertyValue> aFirstSeq = aSeqs[0];
        CPPUNIT_ASSERT_EQUAL(u"FieldType"_ustr, aFirstSeq[0].Name);
        CPPUNIT_ASSERT_EQUAL(u"FieldCommand"_ustr, aFirstSeq[1].Name);
        CPPUNIT_ASSERT_EQUAL(u"ADDIN ZOTERO_ITEM new command 1"_ustr,
                             aFirstSeq[1].Value.get<OUString>());
    }

    void testJsonAwtSize()
    {
        // Given a list of beans::PropertyValues in JSON:
        OString aJson = R"json(
{
    "mykey": {
        "type": "any",
        "value": {
            "type": "com.sun.star.awt.Size",
            "value": {
                "Width": {
                    "type": "long",
                    "value": 42
                },
                "Height": {
                    "type": "long",
                    "value": 43
                }
            }
        }
    }
}
)json"_ostr;

        // When parsing that:
        std::vector<beans::PropertyValue> aRet = comphelper::JsonToPropertyValues(aJson);

        // Then make sure we can construct an awt::Size:
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), aRet.size());
        beans::PropertyValue aFirst = aRet[0];
        CPPUNIT_ASSERT_EQUAL(OUString("mykey"), aFirst.Name);
        // Without the accompanying fix in place, this test would have failed with:
        // - Cannot extract an Any(void) to com.sun.star.awt.Size
        auto aSize = aFirst.Value.get<awt::Size>();
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(42), aSize.Width);
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(43), aSize.Height);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(MakePropertyValueTest);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
