/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/text/textdocumentsettings.hxx>
#include <test/unoapi_property_testers.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/uno/Reference.hxx>

using namespace css::uno;

namespace apitest
{
TextDocumentSettings::~TextDocumentSettings() {}

void TextDocumentSettings::testDocumentSettingsProperties()
{
    css::uno::Reference<css::beans::XPropertySet> xDocumentSettings(init(),
                                                                    css::uno::UNO_QUERY_THROW);

    testBooleanOptionalProperty(xDocumentSettings, u"ChartAutoUpdate"_ustr);
    testBooleanOptionalProperty(xDocumentSettings, u"AddParaTableSpacing"_ustr);
    testBooleanOptionalProperty(xDocumentSettings, u"AddParaTableSpacingAtStart"_ustr);
    testBooleanOptionalProperty(xDocumentSettings, u"AlignTabStopPosition"_ustr);
    testBooleanOptionalProperty(xDocumentSettings, u"SaveGlobalDocumentLinks"_ustr);
    testBooleanOptionalProperty(xDocumentSettings, u"IsLabelDocument"_ustr);
    testBooleanOptionalProperty(xDocumentSettings, u"UseFormerLineSpacing"_ustr);
    testBooleanOptionalProperty(xDocumentSettings, u"AddParaSpacingToTableCells"_ustr);
    testBooleanOptionalProperty(xDocumentSettings, u"UseFormerObjectPositioning"_ustr);
    testBooleanOptionalProperty(xDocumentSettings, u"ConsiderTextWrapOnObjPos"_ustr);
    testBooleanOptionalProperty(xDocumentSettings, u"MathBaselineAlignment"_ustr);
}

} // end namespace apitest

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
