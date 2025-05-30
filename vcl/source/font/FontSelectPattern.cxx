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

#include <sal/config.h>
#include <o3tl/safeint.hxx>
#include <tools/gen.hxx>

#include <utility>
#include <vcl/font.hxx>

#include <font/FontSelectPattern.hxx>
#include <font/PhysicalFontFace.hxx>

namespace vcl::font
{

// These mustn't conflict with font name lists which use ; and ,
const char FontSelectPattern::FEAT_PREFIX = ':';
const char FontSelectPattern::FEAT_SEPARATOR = '&';

FontSelectPattern::FontSelectPattern( const vcl::Font& rFont,
    OUString aSearchName, const Size& rSize, float fExactHeight, bool bNonAntialias)
    : maSearchName(std::move( aSearchName ))
    , mnWidth( rSize.Width() )
    , mnHeight( rSize.Height() )
    , mfExactHeight( fExactHeight)
    , mnOrientation( rFont.GetOrientation() )
    , meLanguage( rFont.GetLanguage() )
    , mbVertical( rFont.IsVertical() )
    , mbNonAntialiased(bNonAntialias)
    , mbEmbolden( false )
{
    maTargetName = GetFamilyName();

    rFont.GetFontAttributes( *this );

    // normalize orientation between 0 and 3600
    if( mnOrientation < 0_deg10 || mnOrientation >= 3600_deg10 )
    {
        if( mnOrientation >= 0_deg10 )
            mnOrientation %= 3600_deg10;
        else
            mnOrientation = 3600_deg10 - (-mnOrientation % 3600_deg10);
    }

    // normalize width and height
    if( mnHeight < 0 )
        mnHeight = o3tl::saturating_toggle_sign(mnHeight);
    if( mnWidth < 0 )
        mnWidth = o3tl::saturating_toggle_sign(mnWidth);
}


// NOTE: this ctor is still used on Windows. Do not remove.
#ifdef _WIN32
FontSelectPattern::FontSelectPattern( const PhysicalFontFace& rFontData,
    const Size& rSize, float fExactHeight, int nOrientation, bool bVertical )
    : FontAttributes( rFontData )
    , mnWidth( rSize.Width() )
    , mnHeight( rSize.Height() )
    , mfExactHeight( fExactHeight )
    , mnOrientation( nOrientation )
    , meLanguage( 0 )
    , mbVertical( bVertical )
    , mbNonAntialiased( false )
    , mbEmbolden( false )
{
    maTargetName = maSearchName = GetFamilyName();
    // NOTE: no normalization for width/height/orientation
}

#endif

size_t FontSelectPattern::hashCode() const
{
    // TODO: does it pay off to improve this hash function?
    size_t nHash;
    // check for features and generate a unique hash if necessary
    if (maTargetName.indexOf(FontSelectPattern::FEAT_PREFIX)
        != -1)
    {
        nHash = maTargetName.hashCode();
    }
    else
    {
        nHash = maSearchName.hashCode();
    }
    nHash += 11U * mnHeight;
    nHash += 19 * GetWeight();
    nHash += 29 * GetItalic();
    nHash += 37 * mnOrientation.get();
    nHash += 41 * static_cast<sal_uInt16>(meLanguage);
    if( mbVertical )
        nHash += 53;
    return nHash;
}

bool FontSelectPattern::operator==(const FontSelectPattern& rOther) const
{
    if (!CompareDeviceIndependentFontAttributes(rOther))
        return false;

    if (maTargetName != rOther.maTargetName)
        return false;

    if (maSearchName != rOther.maSearchName)
        return false;

    if (mnWidth != rOther.mnWidth)
        return false;

    if (mnHeight != rOther.mnHeight)
        return false;

    if (mfExactHeight != rOther.mfExactHeight)
        return false;

    if (mnOrientation != rOther.mnOrientation)
        return false;

    if (meLanguage != rOther.meLanguage)
        return false;

    if (mbVertical != rOther.mbVertical)
        return false;

    if (mbNonAntialiased != rOther.mbNonAntialiased)
        return false;

    if (mbEmbolden != rOther.mbEmbolden)
        return false;

    if (maItalicMatrix != rOther.maItalicMatrix)
        return false;

    return true;
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
