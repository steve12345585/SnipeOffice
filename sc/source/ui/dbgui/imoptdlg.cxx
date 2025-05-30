/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project. MAJOR BOM
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

#include <imoptdlg.hxx>
#include <asciiopt.hxx>
#include <comphelper/string.hxx>
#include <unotools/charclass.hxx>
#include <osl/thread.h>
#include <o3tl/string_view.hxx>
#include <global.hxx>

const char pStrFix[] = "FIX";

//  The option string can no longer contain a semicolon (because of pick list),
//  therefore, starting with version 336 comma instead

ScImportOptions::ScImportOptions( std::u16string_view rStr )
{
    // Use the same string format as ScAsciiOptions,
    // because the import options string is passed here when a CSV file is loaded and saved again.
    // The old format is still supported because it might be used in macros.

    bFixedWidth = false;
    nFieldSepCode = 0;
    nTextSepCode = 0;
    eCharSet = RTL_TEXTENCODING_DONTKNOW;
    bSaveAsShown = true;    // "true" if not in string (after CSV import)
    bQuoteAllText = false;
    bSaveNumberAsSuch = true;
    bSaveFormulas = false;
    bRemoveSpace = false;
    nSheetToExport = 0;
    bEvaluateFormulas = true;   // true if not present at all, for compatibility
    bIncludeBOM = true;  // Always include BOM for UTF-8 by default
    sal_Int32 nTokenCount = comphelper::string::getTokenCount(rStr, ',');
    if ( nTokenCount < 3 )
        return;

    sal_Int32 nIdx{ 0 };
    // first 3 tokens: common
    OUString aToken( o3tl::getToken(rStr, 0, ',', nIdx ) );
    if( aToken.equalsIgnoreAsciiCase( pStrFix ) )
        bFixedWidth = true;
    else
        nFieldSepCode = ScAsciiOptions::GetWeightedFieldSep( aToken, true);
    nTextSepCode  = static_cast<sal_Unicode>(o3tl::toInt32(o3tl::getToken(rStr, 0, ',', nIdx)));
    aStrFont      = o3tl::getToken(rStr, 0, ',', nIdx);
    eCharSet      = ScGlobal::GetCharsetValue(aStrFont);

    if ( nTokenCount == 4 )
    {
        // compatibility with old options string: "Save as shown" as 4th token, numeric
        bSaveAsShown = o3tl::toInt32(o3tl::getToken(rStr, 0, ',', nIdx)) != 0;
        bQuoteAllText = true;   // use old default then
    }
    else
    {
        // look at the same positions as in ScAsciiOptions
        if ( nTokenCount >= 7 )
            bQuoteAllText = o3tl::getToken(rStr, 3, ',', nIdx) == u"true";  // 7th token
        if ( nTokenCount >= 8 )
            bSaveNumberAsSuch = o3tl::getToken(rStr, 0, ',', nIdx) == u"true";
        if ( nTokenCount >= 9 )
            bSaveAsShown = o3tl::getToken(rStr, 0, ',', nIdx) == u"true";
        if ( nTokenCount >= 10 )
            bSaveFormulas = o3tl::getToken(rStr, 0, ',', nIdx) == u"true";
        if ( nTokenCount >= 11 )
            bRemoveSpace = o3tl::getToken(rStr, 0, ',', nIdx) == u"true";
        if ( nTokenCount >= 12 )
        {
            const OUString aTok(o3tl::getToken(rStr,0, ',', nIdx));
            if (aTok == "-1")
                nSheetToExport = -1;    // all
            else if (aTok.isEmpty() || CharClass::isAsciiNumeric(aTok))
                nSheetToExport = aTok.toInt32();
            else
                nSheetToExport = -23;   // invalid, force error
        }
        if ( nTokenCount >= 13 )
            // If present, defaults to "false".
            bEvaluateFormulas = o3tl::getToken(rStr, 0, ',', nIdx) == u"true";
        if (nTokenCount >= 14)
            bIncludeBOM = o3tl::getToken(rStr, 0, ',', nIdx) == u"true";
    }
}

OUString ScImportOptions::BuildString() const
{
    OUString aResult;

    if( bFixedWidth )
        aResult += pStrFix;
    else
        aResult += OUString::number(nFieldSepCode);
    aResult += "," + OUString::number(nTextSepCode) + "," + aStrFont +
                                                 // use the same string format as ScAsciiOptions:
            ",1,,0," +                           // first row, no column info, default language
            OUString::boolean( bQuoteAllText ) + // same as "quoted field as text" in ScAsciiOptions
            "," +
            OUString::boolean( bSaveNumberAsSuch ) + // "save number as such": not in ScAsciiOptions
            "," +
            OUString::boolean( bSaveAsShown ) +  // "save as shown": not in ScAsciiOptions
            "," +
            OUString::boolean( bSaveFormulas ) +  // "save formulas": not in ScAsciiOptions
            "," +
            OUString::boolean( bRemoveSpace ) +  // same as "Remove space" in ScAsciiOptions
            "," +
            OUString::number(nSheetToExport) +  // Only available for command line --convert-to
            "," +
            OUString::boolean( bEvaluateFormulas ) +  // same as "Evaluate formulas" in ScAsciiOptions
            "," +
            OUString::boolean(bIncludeBOM) ;  // same as "Include BOM" in ScAsciiOptions

    return aResult;
}

void ScImportOptions::SetTextEncoding( rtl_TextEncoding nEnc )
{
    eCharSet = (nEnc == RTL_TEXTENCODING_DONTKNOW ?
        osl_getThreadTextEncoding() : nEnc);
    if (eCharSet == RTL_TEXTENCODING_UTF8)
        bIncludeBOM = true;  // Always include BOM for UTF-8
    aStrFont = ScGlobal::GetCharsetString( nEnc );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
