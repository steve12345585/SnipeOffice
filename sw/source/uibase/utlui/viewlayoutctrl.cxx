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

#include <viewlayoutctrl.hxx>

#include <com/sun/star/beans/PropertyValue.hpp>

#include <comphelper/propertyvalue.hxx>
#include <vcl/event.hxx>
#include <vcl/status.hxx>
#include <vcl/image.hxx>
#include <svx/viewlayoutitem.hxx>
#include <strings.hrc>
#include <bitmaps.hlst>
#include <swtypes.hxx>

SFX_IMPL_STATUSBAR_CONTROL( SwViewLayoutControl, SvxViewLayoutItem );

struct SwViewLayoutControl::SwViewLayoutControl_Impl
{
    sal_uInt16      mnState; // 0 = auto, 1= single, 2 = book, 3 = none, 4 = off
    Image       maImageSingleColumn;
    Image       maImageSingleColumn_Active;
    Image       maImageAutomatic;
    Image       maImageAutomatic_Active;
    Image       maImageBookMode;
    Image       maImageBookMode_Active;
};

SwViewLayoutControl::SwViewLayoutControl( sal_uInt16 _nSlotId, sal_uInt16 _nId, StatusBar& rStatusBar ) :
    SfxStatusBarControl( _nSlotId, _nId, rStatusBar ),
    mpImpl( new SwViewLayoutControl_Impl )
{
    mpImpl->mnState = 1;

    mpImpl->maImageSingleColumn         = Image(StockImage::Yes, RID_BMP_VIEWLAYOUT_SINGLECOLUMN);
    mpImpl->maImageSingleColumn_Active  = Image(StockImage::Yes, RID_BMP_VIEWLAYOUT_SINGLECOLUMN_ACTIVE);
    mpImpl->maImageAutomatic            = Image(StockImage::Yes, RID_BMP_VIEWLAYOUT_AUTOMATIC);
    mpImpl->maImageAutomatic_Active     = Image(StockImage::Yes, RID_BMP_VIEWLAYOUT_AUTOMATIC_ACTIVE);
    mpImpl->maImageBookMode             = Image(StockImage::Yes, RID_BMP_VIEWLAYOUT_BOOKMODE);
    mpImpl->maImageBookMode_Active      = Image(StockImage::Yes, RID_BMP_VIEWLAYOUT_BOOKMODE_ACTIVE);
}

SwViewLayoutControl::~SwViewLayoutControl()
{
}

void SwViewLayoutControl::StateChangedAtStatusBarControl( sal_uInt16 /*nSID*/, SfxItemState eState, const SfxPoolItem* pState )
{
    if (SfxItemState::DEFAULT != eState || SfxItemState::DISABLED == eState)
    {
        GetStatusBar().SetItemText( GetId(), OUString() );
        mpImpl->mnState = 4; //tdf#148441 switch off, if disabled
    }
    else
    {
        assert( dynamic_cast< const SvxViewLayoutItem *>( pState )  && "invalid item type" );
        const sal_uInt16 nColumns  = static_cast<const SvxViewLayoutItem*>( pState )->GetValue();
        const bool   bBookMode = static_cast<const SvxViewLayoutItem*>( pState )->IsBookMode();

        // SingleColumn Mode
        if ( 1 == nColumns )
            mpImpl->mnState = 0;
        // Automatic Mode
        else if ( 0 == nColumns )
            mpImpl->mnState = 1;
        // Book Mode
        else if ( bBookMode && 2 == nColumns )
            mpImpl->mnState = 2;
        else
            mpImpl->mnState = 3;
    }

    GetStatusBar().SetItemData( GetId(), nullptr );    // force repaint
}

void SwViewLayoutControl::Paint( const UserDrawEvent& rUsrEvt )
{
    vcl::RenderContext* pDev = rUsrEvt.GetRenderContext();
    tools::Rectangle aRect(rUsrEvt.GetRect());

    const tools::Rectangle aControlRect = getControlRect();

    if (mpImpl->mnState < 4)
    {
        const bool bSingleColumn    = 0 == mpImpl->mnState;
        const bool bAutomatic       = 1 == mpImpl->mnState;
        const bool bBookMode        = 2 == mpImpl->mnState;
        const int nDistance = 6;

        const tools::Long nImageWidthSum = mpImpl->maImageSingleColumn.GetSizePixel().Width() +
                                    mpImpl->maImageAutomatic.GetSizePixel().Width() +
                                    mpImpl->maImageBookMode.GetSizePixel().Width();

        const tools::Long nXOffset = (aRect.GetWidth() - nImageWidthSum - (2* nDistance)) / 2;
        const tools::Long nYOffset = (aControlRect.GetHeight() - mpImpl->maImageSingleColumn.GetSizePixel().Height()) / 2;

        aRect.AdjustLeft( nXOffset );
        aRect.AdjustTop( nYOffset );

        // draw single column image:
        pDev->DrawImage( aRect.TopLeft(), bSingleColumn ? mpImpl->maImageSingleColumn_Active : mpImpl->maImageSingleColumn );

        // draw automatic image:
        aRect.AdjustLeft(mpImpl->maImageSingleColumn.GetSizePixel().Width() + nDistance);
        pDev->DrawImage( aRect.TopLeft(), bAutomatic ? mpImpl->maImageAutomatic_Active       : mpImpl->maImageAutomatic );

        // draw bookmode image:
        aRect.AdjustLeft(mpImpl->maImageAutomatic.GetSizePixel().Width() + nDistance);
        pDev->DrawImage( aRect.TopLeft(), bBookMode ? mpImpl->maImageBookMode_Active         : mpImpl->maImageBookMode );
    }
    else
    {
        pDev->DrawRect( aControlRect );
    }
}

bool SwViewLayoutControl::MouseButtonDown( const MouseEvent & rEvt )
{
    if (mpImpl->mnState < 4)
    {
        const tools::Rectangle aRect = getControlRect();
        const Point aPoint = rEvt.GetPosPixel();
        const tools::Long nXDiff = aPoint.X() - aRect.Left();

        sal_uInt16 nColumns = 1;
        bool bBookMode = false;

        const tools::Long nImageWidthSingle = mpImpl->maImageSingleColumn.GetSizePixel().Width();
        const tools::Long nImageWidthAuto = mpImpl->maImageAutomatic.GetSizePixel().Width();
        const tools::Long nImageWidthBook = mpImpl->maImageBookMode.GetSizePixel().Width();
        const tools::Long nImageWidthSum = nImageWidthSingle + nImageWidthAuto + nImageWidthBook;

        const tools::Long nXOffset = (aRect.GetWidth() - nImageWidthSum)/2;

        if ( nXDiff < nXOffset + nImageWidthSingle )
        {
            mpImpl->mnState = 0; // single
            nColumns = 1;
        }
        else if ( nXDiff < nXOffset + nImageWidthSingle + nImageWidthAuto )
        {
            mpImpl->mnState = 1; // auto
            nColumns = 0;
        }
        else
        {
            mpImpl->mnState = 2; // book
            nColumns = 2;
            bBookMode = true;
        }

        // commit state change
        SvxViewLayoutItem aViewLayout( nColumns, bBookMode );

        css::uno::Any a;
        aViewLayout.QueryValue( a );

        css::uno::Sequence< css::beans::PropertyValue > aArgs{ comphelper::makePropertyValue(u"ViewLayout"_ustr,
                                                                                            a) };
        execute( aArgs );
    }
    return true;
}

bool SwViewLayoutControl::MouseMove( const MouseEvent & rEvt )
{
    if (mpImpl->mnState < 4)
    {
        const tools::Rectangle aRect = getControlRect();
        const Point aPoint = rEvt.GetPosPixel();
        const tools::Long nXDiff = aPoint.X() - aRect.Left();

        const tools::Long nImageWidthSingle = mpImpl->maImageSingleColumn.GetSizePixel().Width();
        const tools::Long nImageWidthAuto = mpImpl->maImageAutomatic.GetSizePixel().Width();
        const tools::Long nImageWidthBook = mpImpl->maImageBookMode.GetSizePixel().Width();
        const tools::Long nImageWidthSum = nImageWidthSingle + nImageWidthAuto + nImageWidthBook;

        const tools::Long nXOffset = (aRect.GetWidth() - nImageWidthSum)/2;

        if ( nXDiff < nXOffset + nImageWidthSingle )
        {
            GetStatusBar().SetQuickHelpText(GetId(), SwResId(STR_VIEWLAYOUT_ONE));
        }
        else if ( nXDiff < nXOffset + nImageWidthSingle + nImageWidthAuto )
        {
            GetStatusBar().SetQuickHelpText(GetId(), SwResId(STR_VIEWLAYOUT_MULTI));
        }
        else
        {
            GetStatusBar().SetQuickHelpText(GetId(), SwResId(STR_VIEWLAYOUT_BOOK));
        }
    }
    return true;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
