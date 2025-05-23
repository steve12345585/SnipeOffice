/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_TEST_SHEET_XSPREADSHEETS2_HXX
#define INCLUDED_TEST_SHEET_XSPREADSHEETS2_HXX

#include <test/testdllapi.hxx>

#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <com/sun/star/sheet/XSpreadsheet.hpp>
#include <com/sun/star/sheet/XNamedRanges.hpp>
#include <com/sun/star/lang/XComponent.hpp>

namespace apitest {

class OOO_DLLPUBLIC_TEST XSpreadsheets2
{
public:
    XSpreadsheets2();
    virtual ~XSpreadsheets2();

    // XSpreadsheets2
    void testImportedSheetNameAndIndex();
    void testImportString();
    void testImportValue();
    void testImportFormulaBasicMath();
    void testImportFormulaWithNamedRange();
    void testImportOverExistingNamedRange();
    void testImportNamedRangeDefinedInSource();
    void testImportNamedRangeRedefinedInSource();
    void testImportNewNamedRange();
    void testImportCellStyle();
    void testLastAfterInsertCopy();

    virtual css::uno::Reference< css::uno::XInterface > init() = 0;
    virtual css::uno::Reference< css::lang::XComponent > loadFromDesktop(const OUString&) = 0;

protected:
    css::uno::Reference< css::sheet::XSpreadsheetDocument> xDocument;

private:
    css::uno::Reference< css::sheet::XSpreadsheetDocument> getDoc(const OUString&);
    static css::uno::Reference< css::sheet::XNamedRanges> getNamedRanges(css::uno::Reference< css::sheet::XSpreadsheetDocument > const &);
    void importSheetToCopy();
    static bool isExternalReference(std::u16string_view aDestContent, std::u16string_view aSrcContent );

    css::uno::Reference< css::sheet::XSpreadsheetDocument> xDestDoc;
    css::uno::Reference< css::sheet::XSpreadsheet > xDestSheet;
    css::uno::Reference< css::sheet::XSpreadsheet > xSrcSheet;
};

}

#endif // INCLUDED_TEST_SHEET_XSPREADSHEETS2_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
