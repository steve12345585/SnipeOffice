/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/unoapi_test.hxx>
#include <test/container/xelementaccess.hxx>
#include <test/container/xenumerationaccess.hxx>
#include <test/util/xrefreshable.hxx>

#include <com/sun/star/frame/Desktop.hpp>

#include <com/sun/star/lang/XMultiServiceFactory.hpp>

#include <com/sun/star/text/XTextDocument.hpp>
#include <com/sun/star/text/XTextFieldsSupplier.hpp>
#include <com/sun/star/text/XDependentTextField.hpp>

#include <comphelper/processfactory.hxx>

using namespace css;
using namespace css::uno;

namespace
{
/**
 * Initial tests for SwXTextFieldTypes.
 */
class SwXTextFieldTypes final : public UnoApiTest,
                                public apitest::XElementAccess,
                                public apitest::XEnumerationAccess,
                                public apitest::XRefreshable
{
public:
    SwXTextFieldTypes()
        : UnoApiTest(u""_ustr)
        , XElementAccess(cppu::UnoType<text::XDependentTextField>::get())
    {
    }

    Reference<XInterface> init() override
    {
        loadFromURL(u"private:factory/swriter"_ustr);
        Reference<text::XTextDocument> xTextDocument(mxComponent, UNO_QUERY_THROW);

        Reference<text::XTextFieldsSupplier> xTFS;

        try
        {
            xTFS = Reference<text::XTextFieldsSupplier>(xTextDocument, UNO_QUERY_THROW);
        }
        catch (Exception&)
        {
        }

        return Reference<XInterface>(xTFS->getTextFields(), UNO_QUERY_THROW);
    }

    CPPUNIT_TEST_SUITE(SwXTextFieldTypes);
    CPPUNIT_TEST(testGetElementType);
    CPPUNIT_TEST(testHasElements);
    CPPUNIT_TEST(testCreateEnumeration);
    CPPUNIT_TEST(testRefreshListener);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SwXTextFieldTypes);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
