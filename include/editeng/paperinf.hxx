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
#ifndef INCLUDED_EDITENG_PAPERINF_HXX
#define INCLUDED_EDITENG_PAPERINF_HXX

// INCLUDE ---------------------------------------------------------------

#include <rtl/ustring.hxx>
#include <tools/mapunit.hxx>
#include <i18nutil/paper.hxx>
#include <tools/gen.hxx>
#include <editeng/editengdllapi.h>

// forward ---------------------------------------------------------------

class Printer;

// class SvxPaperInfo -----------------------------------------------------

class EDITENG_DLLPUBLIC SvxPaperInfo
{
public:
    static Size     GetDefaultPaperSize( MapUnit eUnit = MapUnit::MapTwip );
    static Size     GetPaperSize( Paper ePaper, MapUnit eUnit = MapUnit::MapTwip );
    static Size     GetPaperSize( const Printer* pPrinter );
    static Paper    GetSvxPaper( const Size &rSize, MapUnit eUnit );
    static tools::Long     GetSloppyPaperDimension( tools::Long nSize );
    static OUString GetName( Paper ePaper );
};

// INLINE -----------------------------------------------------------------

inline Size &Swap(Size &rSize)
{
    const tools::Long lVal = rSize.Width();
    rSize.setWidth( rSize.Height() );
    rSize.setHeight( lVal );
    return rSize;
}

inline Size &LandscapeSwap(Size &rSize)
{
    if ( rSize.Height() > rSize.Width() )
        Swap( rSize );
    return rSize;
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
