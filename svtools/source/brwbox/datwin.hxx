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

#pragma once

#include <svtools/brwbox.hxx>
#include <tools/long.hxx>
#include <utility>

#include <limits>

#define MIN_COLUMNWIDTH  2

class ButtonFrame
{
    tools::Rectangle   aRect;
    tools::Rectangle   aInnerRect;
    OUString    aText;
    bool        m_bDrawDisabled;

public:
               ButtonFrame( const Point& rPt, const Size& rSz,
                            OUString _aText,
                            bool _bDrawDisabled)
                :aRect( rPt, rSz )
                ,aInnerRect( Point( aRect.Left()+1, aRect.Top()+1 ),
                            Size( aRect.GetWidth()-2, aRect.GetHeight()-2 ) )
                ,aText(std::move(_aText))
                ,m_bDrawDisabled(_bDrawDisabled)
            {
            }

    void    Draw( OutputDevice& rDev );
};


class BrowserColumn final
{
    sal_uInt16          _nId;
    tools::Long         _nOriginalWidth;
    tools::Long         _nWidth;
    OUString            _aTitle;
    bool                _bFrozen;

public:
                        BrowserColumn( sal_uInt16 nItemId,
                                        OUString aTitle, tools::Long nWidthPixel, const Fraction& rCurrentZoom );
                        ~BrowserColumn();

    sal_uInt16          GetId() const { return _nId; }

    tools::Long         Width() const { return _nWidth; }
    OUString&           Title() { return _aTitle; }

    bool                IsFrozen() const { return _bFrozen; }
    void                Freeze() { _bFrozen = true; }

    void                Draw( BrowseBox const & rBox, OutputDevice& rDev,
                              const Point& rPos  );

    void                SetWidth(tools::Long nNewWidthPixel, const Fraction& rCurrentZoom);
    void                ZoomChanged(const Fraction& rNewZoom);
};

void InitSettings_Impl( vcl::Window *pWin );

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
