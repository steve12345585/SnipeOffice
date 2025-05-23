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

#include <vcl/event.hxx>
#include <vcl/status.hxx>
#include <vcl/toolkit/prgsbar.hxx>
#include <vcl/settings.hxx>
#include <sal/log.hxx>
#include <vcl/svapp.hxx>
#include <vcl/idle.hxx>
#include <tools/json_writer.hxx>

#define PROGRESSBAR_OFFSET          3
#define PROGRESSBAR_WIN_OFFSET      2

void ProgressBar::ImplInit()
{
    mnPrgsWidth = 0;
    mnPrgsHeight = 0;
    mnPercent = 0;
    mnPercentCount = 0;
    mbCalcNew = true;
    SetType(WindowType::PROGRESSBAR);

    ImplInitSettings( true, true, true );
}

static WinBits clearProgressBarBorder( vcl::Window const * pParent, WinBits nOrgStyle, ProgressBar::BarStyle eBarStyle )
{
    WinBits nOutStyle = nOrgStyle;
    if( pParent && (nOrgStyle & WB_BORDER) != 0 )
    {
        if (pParent->IsNativeControlSupported(eBarStyle == ProgressBar::BarStyle::Progress
                                                  ? ControlType::Progress
                                                  : ControlType::LevelBar,
                                              ControlPart::Entire))
            nOutStyle &= WB_BORDER;
    }
    return nOutStyle;
}

Size ProgressBar::GetOptimalSize() const
{
    return meBarStyle == BarStyle::Progress ? Size(150, 20) : Size(150,10);
}

ProgressBar::ProgressBar( vcl::Window* pParent, WinBits nWinStyle, BarStyle eBarStyle ) :
        Window( pParent, clearProgressBarBorder( pParent, nWinStyle, eBarStyle ) ),
        meBarStyle(eBarStyle)
{
    SetOutputSizePixel( GetOptimalSize() );
    ImplInit();
}

void ProgressBar::ImplInitSettings( bool bFont,
                                    bool bForeground, bool bBackground )
{
    const StyleSettings& rStyleSettings = GetSettings().GetStyleSettings();

/* FIXME: !!! We do not support text output at the moment
    if ( bFont )
        ApplyControlFont(*this, rStyleSettings.GetAppFont());
*/

    if ( bBackground )
    {
        if (!IsControlBackground()
            && IsNativeControlSupported(meBarStyle == BarStyle::Progress ? ControlType::Progress
                                                                         : ControlType::LevelBar,
                                        ControlPart::Entire))
        {
            if( GetStyle() & WB_BORDER )
                SetBorderStyle( WindowBorderStyle::REMOVEBORDER );
            EnableChildTransparentMode();
            SetPaintTransparent( true );
            SetBackground();
            SetParentClipMode( ParentClipMode::NoClip );
        }
        else
        {
            Color aColor;
            if ( IsControlBackground() )
                aColor = GetControlBackground();
            else
                aColor = rStyleSettings.GetFaceColor();
            SetBackground( aColor );
        }
    }

    if ( !(bForeground || bFont) )
        return;

    Color aColor = rStyleSettings.GetHighlightColor();
    if ( IsControlForeground() )
        aColor = GetControlForeground();
    if ( aColor.IsRGBEqual( GetBackground().GetColor() ) )
    {
        if ( aColor.GetLuminance() > 100 )
            aColor.DecreaseLuminance( 64 );
        else
            aColor.IncreaseLuminance( 64 );
    }
    GetOutDev()->SetLineColor();
    GetOutDev()->SetFillColor( aColor );
/* FIXME: !!! We do not support text output at the moment
    SetTextColor( aColor );
    SetTextFillColor();
*/
}

void ProgressBar::ImplDrawProgress(vcl::RenderContext& rRenderContext, sal_uInt16 nNewPerc)
{
    if (mbCalcNew)
    {
        mbCalcNew = false;

        Size aSize(GetOutputSizePixel());
        mnPrgsHeight = aSize.Height() - (PROGRESSBAR_WIN_OFFSET * 2);
        mnPrgsWidth = (mnPrgsHeight * 2) / 3;
        maPos.setY( PROGRESSBAR_WIN_OFFSET );
        tools::Long nMaxWidth = aSize.Width() - (PROGRESSBAR_WIN_OFFSET * 2) + PROGRESSBAR_OFFSET;
        sal_uInt16 nMaxCount = static_cast<sal_uInt16>(nMaxWidth / (mnPrgsWidth+PROGRESSBAR_OFFSET));
        if (nMaxCount <= 1)
        {
            nMaxCount = 1;
        }
        else
        {
            while (((10000 / (10000 / nMaxCount)) * (mnPrgsWidth + PROGRESSBAR_OFFSET)) > nMaxWidth)
            {
                nMaxCount--;
            }
        }
        mnPercentCount = 10000 / nMaxCount;
        nMaxWidth = ((10000 / (10000 / nMaxCount)) * (mnPrgsWidth + PROGRESSBAR_OFFSET)) - PROGRESSBAR_OFFSET;
        maPos.setX( (aSize.Width() - nMaxWidth) / 2 );
    }

    ::DrawProgress(
        this, rRenderContext, maPos, PROGRESSBAR_OFFSET, mnPrgsWidth, mnPrgsHeight,
        /*nPercent1=*/0, nNewPerc * 100, mnPercentCount, tools::Rectangle(Point(), GetSizePixel()),
        meBarStyle == BarStyle::Progress ? ControlType::Progress : ControlType::LevelBar);
}

void ProgressBar::Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle& /*rRect*/)
{
    ImplDrawProgress(rRenderContext, mnPercent);
}

void ProgressBar::Resize()
{
    mbCalcNew = true;
    if ( IsReallyVisible() )
        Invalidate();
}

void ProgressBar::SetValue( sal_uInt16 nNewPercent )
{
    SAL_WARN_IF( nNewPercent > 100, "vcl", "StatusBar::SetProgressValue(): nPercent > 100" );

    if ( nNewPercent < mnPercent )
    {
        mbCalcNew = true;
        mnPercent = nNewPercent;
        if ( IsReallyVisible() )
        {
            Invalidate();
            PaintImmediately();
        }
    }
    else if ( mnPercent != nNewPercent )
    {
        mnPercent = nNewPercent;
        Invalidate();

        // Make sure the progressbar is actually painted even if the caller is busy with its task,
        // so the main loop would not be invoked.
        Idle aIdle("ProgressBar::SetValue aIdle");
        aIdle.SetPriority(TaskPriority::POST_PAINT);
        aIdle.Start();
        while (aIdle.IsActive() && !Application::IsQuit())
        {
            Application::Yield();
        }
    }
}

void ProgressBar::StateChanged( StateChangedType nType )
{
/* FIXME: !!! We do not support text output at the moment
    if ( (nType == StateChangedType::Zoom) ||
         (nType == StateChangedType::ControlFont) )
    {
        ImplInitSettings( true, false, false );
        Invalidate();
    }
    else
*/
    if ( nType == StateChangedType::ControlForeground )
    {
        ImplInitSettings( false, true, false );
        Invalidate();
    }
    else if ( nType == StateChangedType::ControlBackground )
    {
        ImplInitSettings( false, false, true );
        Invalidate();
    }

    Window::StateChanged( nType );
}

void ProgressBar::DataChanged( const DataChangedEvent& rDCEvt )
{
    if ( (rDCEvt.GetType() == DataChangedEventType::SETTINGS) &&
         (rDCEvt.GetFlags() & AllSettingsFlags::STYLE) )
    {
        ImplInitSettings( true, true, true );
        Invalidate();
    }

    Window::DataChanged( rDCEvt );
}

void ProgressBar::DumpAsPropertyTree(tools::JsonWriter& rJsonWriter)
{
    vcl::Window::DumpAsPropertyTree(rJsonWriter);
    rJsonWriter.put("value", mnPercent);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
