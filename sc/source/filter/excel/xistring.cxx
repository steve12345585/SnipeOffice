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

#include <utility>
#include <xistring.hxx>
#include <xlstyle.hxx>
#include <xistream.hxx>
#include <xiroot.hxx>
#include <xltools.hxx>
#include <sal/log.hxx>

// Byte/Unicode strings =======================================================

/** All allowed flags for import. */
const XclStrFlags nAllowedFlags = XclStrFlags::EightBitLength | XclStrFlags::SmartFlags | XclStrFlags::SeparateFormats;

XclImpString::XclImpString()
{
}

XclImpString::XclImpString( OUString aString ) :
    maString(std::move( aString ))
{
}

void XclImpString::Read( XclImpStream& rStrm, XclStrFlags nFlags )
{
    if( !( nFlags & XclStrFlags::SeparateFormats ) )
        maFormats.clear();

    SAL_WARN_IF(
        nFlags & ~nAllowedFlags, "sc.filter",
        "XclImpString::Read - unknown flag");
    bool b16BitLen = !( nFlags & XclStrFlags::EightBitLength );

    switch( rStrm.GetRoot().GetBiff() )
    {
        case EXC_BIFF2:
        case EXC_BIFF3:
        case EXC_BIFF4:
        case EXC_BIFF5:
            // no integrated formatting in BIFF2-BIFF7
            maString = rStrm.ReadByteString( b16BitLen );
        break;

        case EXC_BIFF8:
        {
            // --- string header ---
            sal_uInt16 nChars = b16BitLen ? rStrm.ReaduInt16() : rStrm.ReaduInt8();
            sal_uInt8 nFlagField = 0;
            if( nChars || !( nFlags & XclStrFlags::SmartFlags ) )
                nFlagField = rStrm.ReaduInt8();

            bool b16Bit, bRich, bFarEast;
            sal_uInt16 nRunCount;
            sal_uInt32 nExtInf;
            rStrm.ReadUniStringExtHeader( b16Bit, bRich, bFarEast, nRunCount, nExtInf, nFlagField );
            // ignore the flags, they may be wrong

            // --- character array ---
            maString = rStrm.ReadRawUniString( nChars, b16Bit );

            // --- formatting ---
            if (nRunCount)
                ReadFormats( rStrm, maFormats, nRunCount );

            // --- extended (FarEast) information ---
            rStrm.Ignore( nExtInf );
        }
        break;

        default:
            DBG_ERROR_BIFF();
    }
}

void XclImpString::AppendFormat( XclFormatRunVec& rFormats, sal_uInt16 nChar, sal_uInt16 nFontIdx )
{
    // #i33341# real life -- same character index may occur several times
    OSL_ENSURE( rFormats.empty() || (rFormats.back().mnChar <= nChar), "XclImpString::AppendFormat - wrong char order" );
    if( rFormats.empty() || (rFormats.back().mnChar < nChar) )
        rFormats.emplace_back( nChar, nFontIdx );
    else
        rFormats.back().mnFontIdx = nFontIdx;
}

void XclImpString::ReadFormats( XclImpStream& rStrm, XclFormatRunVec& rFormats )
{
    bool bBiff8 = rStrm.GetRoot().GetBiff() == EXC_BIFF8;
    sal_uInt16 nRunCount = bBiff8 ? rStrm.ReaduInt16() : rStrm.ReaduInt8();
    ReadFormats( rStrm, rFormats, nRunCount );
}

void XclImpString::ReadFormats( XclImpStream& rStrm, XclFormatRunVec& rFormats, sal_uInt16 nRunCount )
{
    rFormats.clear();

    size_t nElementSize = rStrm.GetRoot().GetBiff() == EXC_BIFF8 ? 4 : 2;
    size_t nAvailableBytes = rStrm.GetRecLeft();
    size_t nMaxElements = nAvailableBytes / nElementSize;
    if (nRunCount > nMaxElements)
    {
        SAL_WARN("sc.filter", "XclImpString::ReadFormats - more formats claimed than stream could contain");
        rStrm.SetSvStreamError(SVSTREAM_FILEFORMAT_ERROR);
        return;
    }

    rFormats.reserve( nRunCount );
    /*  #i33341# real life -- same character index may occur several times
        -> use AppendFormat() to validate formats */
    if( rStrm.GetRoot().GetBiff() == EXC_BIFF8 )
    {
        for( sal_uInt16 nIdx = 0; nIdx < nRunCount; ++nIdx )
        {
            sal_uInt16 nChar = rStrm.ReaduInt16();
            sal_uInt16 nFontIdx = rStrm.ReaduInt16();
            AppendFormat( rFormats, nChar, nFontIdx );
        }
    }
    else
    {
        for( sal_uInt16 nIdx = 0; nIdx < nRunCount; ++nIdx )
        {
            sal_uInt8 nChar = rStrm.ReaduInt8();
            sal_uInt8 nFontIdx = rStrm.ReaduInt8();
            AppendFormat( rFormats, nChar, nFontIdx );
        }
    }
}

void XclImpString::ReadObjFormats( XclImpStream& rStrm, XclFormatRunVec& rFormats, sal_uInt16 nFormatSize )
{
    // number of formatting runs, each takes 8 bytes
    sal_uInt16 nRunCount = nFormatSize / 8;
    rFormats.clear();
    rFormats.reserve( nRunCount );
    for( sal_uInt16 nIdx = 0; nIdx < nRunCount; ++nIdx )
    {
        sal_uInt16 nChar = rStrm.ReaduInt16();
        sal_uInt16 nFontIdx = rStrm.ReaduInt16();
        rStrm.Ignore( 4 );
        AppendFormat( rFormats, nChar, nFontIdx );
    }
}

// String iterator ============================================================

XclImpStringIterator::XclImpStringIterator( const XclImpString& rString ) :
    mrText( rString.GetText() ),
    mrFormats( rString.GetFormats() ),
    mnPortion( 0 ),
    mnTextBeg( 0 ),
    mnTextEnd( 0 ),
    mnFormatsBeg( 0 ),
    mnFormatsEnd( 0 )
{
    // first portion is formatted, adjust vector index to next portion
    if( !mrFormats.empty() && (mrFormats.front().mnChar == 0) )
        ++mnFormatsEnd;
    // find end position of the first portion
    mnTextEnd = (mnFormatsEnd < mrFormats.size() ?
        mrFormats[ mnFormatsEnd ].mnChar : mrText.getLength() );
}

OUString XclImpStringIterator::GetPortionText() const
{
    return mrText.copy( mnTextBeg, mnTextEnd - mnTextBeg );
}

sal_uInt16 XclImpStringIterator::GetPortionFont() const
{
    return (mnFormatsBeg < mnFormatsEnd) ? mrFormats[ mnFormatsBeg ].mnFontIdx : EXC_FONT_NOTFOUND;
}

XclImpStringIterator& XclImpStringIterator::operator++()
{
    if( Is() )
    {
        ++mnPortion;
        do
        {
            // indexes into vector of formatting runs
            if( mnFormatsBeg < mnFormatsEnd )
                ++mnFormatsBeg;
            if( mnFormatsEnd < mrFormats.size() )
                ++mnFormatsEnd;
            // character positions of next portion
            mnTextBeg = mnTextEnd;
            mnTextEnd = (mnFormatsEnd < mrFormats.size()) ?
                mrFormats[ mnFormatsEnd ].mnChar : mrText.getLength();
        }
        while( Is() && (mnTextBeg == mnTextEnd) );
    }
    return *this;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
