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

#include <sal/log.hxx>
#include <o3tl/safeint.hxx>
#include <osl/diagnose.h>
#include <tools/debug.hxx>
#include <svtools/brwbox.hxx>
#include <svtools/brwhead.hxx>
#include <svtools/colorcfg.hxx>
#include <svtools/scrolladaptor.hxx>
#include "datwin.hxx"
#include <vcl/commandevent.hxx>
#include <vcl/help.hxx>
#include <vcl/ptrstyle.hxx>
#include <vcl/settings.hxx>

#include <tools/multisel.hxx>
#include <tools/fract.hxx>
#include <algorithm>
#include <memory>

using namespace ::com::sun::star::datatransfer;


void BrowseBox::StartDrag( sal_Int8 /* _nAction */, const Point& /* _rPosPixel */ )
{
    // not interested in this event
}


sal_Int8 BrowseBox::AcceptDrop( const AcceptDropEvent& _rEvt )
{
    AcceptDropEvent aTransformed( _rEvt );
    aTransformed.maPosPixel = pDataWin->ScreenToOutputPixel( OutputToScreenPixel( _rEvt.maPosPixel ) );
    return pDataWin->AcceptDrop( aTransformed );
}


sal_Int8 BrowseBox::ExecuteDrop( const ExecuteDropEvent& _rEvt )
{
    ExecuteDropEvent aTransformed( _rEvt );
    aTransformed.maPosPixel = pDataWin->ScreenToOutputPixel( OutputToScreenPixel( _rEvt.maPosPixel ) );
    return pDataWin->ExecuteDrop( aTransformed );
}


sal_Int8 BrowseBox::AcceptDrop( const BrowserAcceptDropEvent& )
{
    // not interested in this event
    return DND_ACTION_NONE;
}


sal_Int8 BrowseBox::ExecuteDrop( const BrowserExecuteDropEvent& )
{
    // not interested in this event
    return DND_ACTION_NONE;
}


const DataFlavorExVector& BrowseBox::GetDataFlavors() const
{
    if (pDataWin->bCallingDropCallback)
        return pDataWin->GetDataFlavorExVector();
    return GetDataFlavorExVector();
}


bool BrowseBox::IsDropFormatSupported( SotClipboardFormatId _nFormat ) const
{
    if ( pDataWin->bCallingDropCallback )
        return pDataWin->IsDropFormatSupported( _nFormat );

    return DropTargetHelper::IsDropFormatSupported( _nFormat );
}


void BrowseBox::Command( const CommandEvent& rEvt )
{
    if ( !pDataWin->bInCommand )
        Control::Command( rEvt );
}


void BrowseBox::StateChanged( StateChangedType nStateChange )
{
    Control::StateChanged( nStateChange );

    if ( StateChangedType::Mirroring == nStateChange )
    {
        pDataWin->EnableRTL( IsRTLEnabled() );

        HeaderBar* pHeaderBar = pDataWin->pHeaderBar;
        if ( pHeaderBar )
            pHeaderBar->EnableRTL( IsRTLEnabled() );
        aHScroll->EnableRTL( IsRTLEnabled() );
        if( pVScroll )
            pVScroll->EnableRTL( IsRTLEnabled() );
        Resize();
    }
    else if ( StateChangedType::InitShow == nStateChange )
    {
        bBootstrapped = true; // must be set first!

        Resize();
        if ( bMultiSelection )
            uRow.pSel->SetTotalRange( Range( 0, nRowCount - 1 ) );
        if ( nRowCount == 0 )
            nCurRow = BROWSER_ENDOFSELECTION;
        else if ( nCurRow == BROWSER_ENDOFSELECTION )
            nCurRow = 0;


        if ( HasFocus() )
        {
            bSelectionIsVisible = true;
            bHasFocus = true;
        }
        UpdateScrollbars();
        AutoSizeLastColumn();
        CursorMoved();
    }
    else if (StateChangedType::Zoom == nStateChange)
    {
        pDataWin->SetZoom(GetZoom());
        HeaderBar* pHeaderBar = pDataWin->pHeaderBar;
        if (pHeaderBar)
            pHeaderBar->SetZoom(GetZoom());

        // let the columns calculate their new widths and adjust the header bar
        for (auto & pCol : mvCols)
        {
            pCol->ZoomChanged(GetZoom());
            if ( pHeaderBar )
                pHeaderBar->SetItemSize( pCol->GetId(), pCol->Width() );
        }

        // all our controls have to be repositioned
        Resize();
    }
    else if (StateChangedType::Enable == nStateChange)
    {
        // do we have a handle column?
        bool bHandleCol = !mvCols.empty() && (0 == mvCols[ 0 ]->GetId());
        // do we have a header bar?
        bool bHeaderBar(pDataWin->pHeaderBar);

        if  (   nTitleLines
            &&  (   !bHeaderBar
                ||  bHandleCol
                )
            )
            // we draw the text in our header bar in a color dependent on the enabled state. So if this state changed
            // -> redraw
            Invalidate(tools::Rectangle(Point(0, 0), Size(GetOutputSizePixel().Width(), GetTitleHeight() - 1)));
    }
}


void BrowseBox::Select()
{
}


void BrowseBox::DoubleClick( const BrowserMouseEvent & )
{
}


tools::Long BrowseBox::QueryMinimumRowHeight()
{
    return CalcZoom( 5 );
}


void BrowseBox::ImplStartTracking()
{
}


void BrowseBox::ImplEndTracking()
{
}


void BrowseBox::RowHeightChanged()
{
}


void BrowseBox::ColumnResized( sal_uInt16 )
{
}


void BrowseBox::ColumnMoved( sal_uInt16 )
{
}


void BrowseBox::StartScroll()
{
    DoHideCursor();
}


void BrowseBox::EndScroll()
{
    UpdateScrollbars();
    AutoSizeLastColumn();
    DoShowCursor();
}


void BrowseBox::ToggleSelection()
{

    // selection highlight-toggling allowed?
    if ( bHideSelect )
        return;
    if ( bNotToggleSel || !IsUpdateMode() || !bSelectionIsVisible )
        return;

    // only highlight painted areas!
    bNotToggleSel = true;

    // accumulate areas of rows to highlight
    std::vector<tools::Rectangle> aHighlightList;
    sal_Int32 nLastRowInRect = 0; // for the CFront

    // don't highlight handle column
    BrowserColumn *pFirstCol = mvCols.empty() ? nullptr : mvCols[ 0 ].get();
    tools::Long nOfsX = (!pFirstCol || pFirstCol->GetId()) ? 0 : pFirstCol->Width();

    // accumulate old row selection
    sal_Int32 nBottomRow = nTopRow +
        pDataWin->GetOutputSizePixel().Height() / GetDataRowHeight();
    if ( nBottomRow > GetRowCount() && GetRowCount() )
        nBottomRow = GetRowCount();
    for ( sal_Int32 nRow = bMultiSelection ? uRow.pSel->FirstSelected() : uRow.nSel;
          nRow != BROWSER_ENDOFSELECTION && nRow <= nBottomRow;
          nRow = bMultiSelection ? uRow.pSel->NextSelected() : BROWSER_ENDOFSELECTION )
    {
        if ( nRow < nTopRow )
            continue;

        tools::Rectangle aAddRect(
            Point( nOfsX, (nRow-nTopRow)*GetDataRowHeight() ),
            Size( pDataWin->GetSizePixel().Width(), GetDataRowHeight() ) );
        if ( !aHighlightList.empty() && nLastRowInRect == ( nRow - 1 ) )
            aHighlightList[ 0 ].Union( aAddRect );
        else
            aHighlightList.emplace( aHighlightList.begin(), aAddRect );
        nLastRowInRect = nRow;
    }

    // unhighlight the old selection (if any)
    while ( !aHighlightList.empty() )
    {
        pDataWin->Invalidate( aHighlightList.back() );
        aHighlightList.pop_back();
    }

    // unhighlight old column selection (if any)
    for ( tools::Long nColId = pColSel ? pColSel->FirstSelected() : BROWSER_ENDOFSELECTION;
          nColId != BROWSER_ENDOFSELECTION;
          nColId = pColSel->NextSelected() )
    {
        tools::Rectangle aRect( GetFieldRectPixel(nCurRow,
                                           mvCols[ nColId ]->GetId(),
                                           false ) );
        aRect.AdjustLeft( -(MIN_COLUMNWIDTH) );
        aRect.AdjustRight(MIN_COLUMNWIDTH );
        aRect.SetTop( 0 );
        aRect.SetBottom( pDataWin->GetOutputSizePixel().Height() );
        pDataWin->Invalidate( aRect );
    }

    bNotToggleSel = false;
}


void BrowseBox::DrawCursor()
{
    bool bReallyHide = false;
    if ( bHideCursor == TRISTATE_INDET )
    {
        if ( !GetSelectRowCount() && !GetSelectColumnCount() )
            bReallyHide = true;
    }
    else if ( bHideCursor == TRISTATE_TRUE )
    {
        bReallyHide = true;
    }

    bReallyHide |= !bSelectionIsVisible || !IsUpdateMode() || bScrolling || nCurRow < 0;

    if (PaintCursorIfHiddenOnce())
        bReallyHide |= ( GetCursorHideCount() > 1 );
    else
        bReallyHide |= ( GetCursorHideCount() > 0 );

    // no cursor on handle column
    if ( nCurColId == HandleColumnId )
        nCurColId = GetColumnId(1);

    // calculate cursor rectangle
    tools::Rectangle aCursor;
    if ( bColumnCursor )
    {
        aCursor = GetFieldRectPixel( nCurRow, nCurColId, false );
        aCursor.AdjustLeft( -(MIN_COLUMNWIDTH) );
        aCursor.AdjustRight(1 );
        aCursor.AdjustBottom(1 );
    }
    else
        aCursor = tools::Rectangle(
            Point( ( !mvCols.empty() && mvCols[ 0 ]->GetId() == 0 ) ?
                        mvCols[ 0 ]->Width() : 0,
                        (nCurRow - nTopRow) * GetDataRowHeight() + 1 ),
            Size( pDataWin->GetOutputSizePixel().Width() + 1,
                  GetDataRowHeight() - 2 ) );
    if ( bHLines )
    {
        if ( !bMultiSelection )
            aCursor.AdjustTop( -1 );
        aCursor.AdjustBottom( -1 );
    }

    if (m_aCursorColor == COL_TRANSPARENT)
    {
        // on these platforms, the StarView focus works correctly
        if ( bReallyHide )
            static_cast<Control*>(pDataWin.get())->HideFocus();
        else
            static_cast<Control*>(pDataWin.get())->ShowFocus( aCursor );
    }
    else
    {
        Color rCol = bReallyHide ? pDataWin->GetOutDev()->GetFillColor() : m_aCursorColor;
        Color aOldFillColor = pDataWin->GetOutDev()->GetFillColor();
        Color aOldLineColor = pDataWin->GetOutDev()->GetLineColor();
        pDataWin->GetOutDev()->SetFillColor();
        pDataWin->GetOutDev()->SetLineColor( rCol );
        pDataWin->GetOutDev()->DrawRect( aCursor );
        pDataWin->GetOutDev()->SetLineColor( aOldLineColor );
        pDataWin->GetOutDev()->SetFillColor( aOldFillColor );
    }
}


tools::Long BrowseBox::GetColumnWidth( sal_uInt16 nId ) const
{

    sal_uInt16 nItemPos = GetColumnPos( nId );
    if ( nItemPos >= mvCols.size() )
        return 0;
    return mvCols[ nItemPos ]->Width();
}


sal_uInt16 BrowseBox::GetColumnId( sal_uInt16 nPos ) const
{

    if ( nPos >= mvCols.size() )
        return BROWSER_INVALIDID;
    return mvCols[ nPos ]->GetId();
}


sal_uInt16 BrowseBox::GetColumnPos( sal_uInt16 nId ) const
{
    for ( size_t nPos = 0; nPos < mvCols.size(); ++nPos )
        if ( mvCols[ nPos ]->GetId() == nId )
            return nPos;
    return BROWSER_INVALIDID;
}


bool BrowseBox::IsFrozen( sal_uInt16 nColumnId ) const
{
    for (auto const & pCol : mvCols)
        if ( pCol->GetId() == nColumnId )
            return pCol->IsFrozen();
    return false;
}


void BrowseBox::ExpandRowSelection( const BrowserMouseEvent& rEvt )
{
    DoHideCursor();

    // expand the last selection
    if ( bMultiSelection )
    {
        Range aJustifiedRange( aSelRange );
        aJustifiedRange.Normalize();

        bool bSelectThis = ( bSelect != aJustifiedRange.Contains( rEvt.GetRow() ) );

        if ( aJustifiedRange.Contains( rEvt.GetRow() ) )
        {
            // down and up
            while ( rEvt.GetRow() < aSelRange.Max() )
            {   // ZTC/Mac bug - don't put these statements together!
                SelectRow( aSelRange.Max(), bSelectThis );
                --aSelRange.Max();
            }
            while ( rEvt.GetRow() > aSelRange.Max() )
            {   // ZTC/Mac bug - don't put these statements together!
                SelectRow( aSelRange.Max(), bSelectThis );
                ++aSelRange.Max();
            }
        }
        else
        {
            // up and down
            bool bOldSelecting = bSelecting;
            bSelecting = true;
            while ( rEvt.GetRow() < aSelRange.Max() )
            {   // ZTC/Mac bug - don't put these statements together!
                --aSelRange.Max();
                if ( !IsRowSelected( aSelRange.Max() ) )
                {
                    SelectRow( aSelRange.Max(), bSelectThis );
                    bSelect = true;
                }
            }
            while ( rEvt.GetRow() > aSelRange.Max() )
            {   // ZTC/Mac bug - don't put these statements together!
                ++aSelRange.Max();
                if ( !IsRowSelected( aSelRange.Max() ) )
                {
                    SelectRow( aSelRange.Max(), bSelectThis );
                    bSelect = true;
                }
            }
            bSelecting = bOldSelecting;
            if ( bSelect )
                Select();
        }
    }
    else
        if (!IsRowSelected(rEvt.GetRow()))
            SelectRow( rEvt.GetRow() );

    GoToRow( rEvt.GetRow(), false );
    DoShowCursor();
}


void BrowseBox::Resize()
{
    if ( !bBootstrapped && IsReallyVisible() )
        BrowseBox::StateChanged( StateChangedType::InitShow );
    if ( mvCols.empty() )
    {
        pDataWin->bResizeOnPaint = true;
        return;
    }
    pDataWin->bResizeOnPaint = false;

    // calc the size of the scrollbars
    tools::Long nSBHeight = GetBarHeight();
    tools::Long nSBWidth = GetSettings().GetStyleSettings().GetScrollBarSize();
    if (IsZoom())
    {
        nSBHeight = static_cast<tools::Long>(nSBHeight * static_cast<double>(GetZoom()));
        nSBWidth = static_cast<tools::Long>(nSBWidth * static_cast<double>(GetZoom()));
    }

    DoHideCursor();
    sal_uInt16 nOldVisibleRows = 0;
    //fdo#42694, post #i111125# GetDataRowHeight() can be 0
    if (GetDataRowHeight())
        nOldVisibleRows = static_cast<sal_uInt16>(pDataWin->GetOutputSizePixel().Height() / GetDataRowHeight() + 1);

    // did we need a horizontal scroll bar or is there a Control Area?
    if ( !pDataWin->bNoHScroll &&
         ( ( mvCols.size() - FrozenColCount() ) > 1 ) )
        aHScroll->Show();
    else
        aHScroll->Hide();

    // calculate the size of the data window
    tools::Long nDataHeight = GetOutputSizePixel().Height() - GetTitleHeight();
    if ( aHScroll->IsVisible() || ( nControlAreaWidth != USHRT_MAX ) )
        nDataHeight -= nSBHeight;

    tools::Long nDataWidth = GetOutputSizePixel().Width();
    if ( pVScroll->IsVisible() )
        nDataWidth -= nSBWidth;

    // adjust position and size of data window
    pDataWin->SetPosSizePixel(
        Point( 0, GetTitleHeight() ),
        Size( nDataWidth, nDataHeight ) );

    sal_uInt16 nVisibleRows = 0;

    if (GetDataRowHeight())
        nVisibleRows = static_cast<sal_uInt16>(pDataWin->GetOutputSizePixel().Height() / GetDataRowHeight() + 1);

    // TopRow is unchanged, but the number of visible lines has changed.
    if ( nVisibleRows != nOldVisibleRows )
        VisibleRowsChanged(nTopRow, nVisibleRows);

    UpdateScrollbars();

    // Control-Area
    tools::Rectangle aInvalidArea( GetControlArea() );
    aInvalidArea.SetRight( GetOutputSizePixel().Width() );
    aInvalidArea.SetLeft( 0 );
    Invalidate( aInvalidArea );

    // external header-bar
    HeaderBar* pHeaderBar = pDataWin->pHeaderBar;
    if ( pHeaderBar )
    {
        // take the handle column into account
        BrowserColumn *pFirstCol = mvCols[ 0 ].get();
        tools::Long nOfsX = pFirstCol->GetId() ? 0 : pFirstCol->Width();
        pHeaderBar->SetPosSizePixel( Point( nOfsX, 0 ), Size( GetOutputSizePixel().Width() - nOfsX, GetTitleHeight() ) );
    }

    AutoSizeLastColumn(); // adjust last column width
    DoShowCursor();
}


void BrowseBox::Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle& rRect)
{
    // initializations
    if (!bBootstrapped && IsReallyVisible())
        BrowseBox::StateChanged(StateChangedType::InitShow);
    if (mvCols.empty())
        return;

    BrowserColumn *pFirstCol = mvCols[ 0 ].get();
    bool bHandleCol = pFirstCol && pFirstCol->GetId() == 0;
    bool bHeaderBar(pDataWin->pHeaderBar);

    // draw delimitational lines
    if (!pDataWin->bNoHScroll)
        rRenderContext.DrawLine(Point(0, aHScroll->GetPosPixel().Y()),
                                Point(GetOutputSizePixel().Width(),
                                      aHScroll->GetPosPixel().Y()));

    if (nTitleLines)
    {
        if (!bHeaderBar)
        {
            rRenderContext.DrawLine(Point(0, GetTitleHeight() - 1),
                                    Point(GetOutputSizePixel().Width(), GetTitleHeight() - 1));
        }
        else if (bHandleCol)
        {
            rRenderContext.DrawLine(Point(0, GetTitleHeight() - 1),
                                    Point(pFirstCol->Width(), GetTitleHeight() - 1));
        }
    }

    // Title Bar
    // If there is a handle column and if the  header bar is available, only
    // take the HandleColumn into account
    if (!(nTitleLines && (!bHeaderBar || bHandleCol)))
        return;

    // iterate through columns to redraw
    tools::Long nX = 0;
    size_t nCol;
    for (nCol = 0; nCol < mvCols.size() && nX < rRect.Right(); ++nCol)
    {
        // skip invisible columns between frozen and scrollable area
        if (nCol < nFirstCol && !mvCols[nCol]->IsFrozen())
            nCol = nFirstCol;

        // only the handle column?
        if (bHeaderBar && bHandleCol && nCol > 0)
            break;

        BrowserColumn* pCol = mvCols[nCol].get();

        // draw the column and increment position
        if ( pCol->Width() > 4 )
        {
            ButtonFrame aButtonFrame( Point( nX, 0 ),
                Size( pCol->Width()-1, GetTitleHeight()-1 ),
                pCol->Title(), !IsEnabled());
            aButtonFrame.Draw(rRenderContext);
            rRenderContext.DrawLine(Point(nX + pCol->Width() - 1, 0),
                                    Point(nX + pCol->Width() - 1, GetTitleHeight() - 1));
        }
        else
        {
            rRenderContext.Push(vcl::PushFlags::FILLCOLOR);
            rRenderContext.SetFillColor(COL_BLACK);
            rRenderContext.DrawRect(tools::Rectangle(Point(nX, 0), Size(pCol->Width(), GetTitleHeight() - 1)));
            rRenderContext.Pop();
        }

        // skip column
        nX += pCol->Width();
    }

    // retouching
    if ( !bHeaderBar && nCol == mvCols.size() )
    {
        const StyleSettings &rSettings = rRenderContext.GetSettings().GetStyleSettings();
        Color aColFace(rSettings.GetFaceColor());
        rRenderContext.Push(vcl::PushFlags::FILLCOLOR | vcl::PushFlags::LINECOLOR);
        rRenderContext.SetFillColor(aColFace);
        rRenderContext.SetLineColor(aColFace);
        rRenderContext.DrawRect(tools::Rectangle(Point(nX, 0),
                                          Point(rRect.Right(), GetTitleHeight() - 2 )));
        rRenderContext.Pop();
    }

    if (m_nActualCornerWidth)
    {
        const StyleSettings &rSettings = rRenderContext.GetSettings().GetStyleSettings();
        Color aColFace(rSettings.GetFaceColor());
        rRenderContext.Push(vcl::PushFlags::FILLCOLOR | vcl::PushFlags::LINECOLOR);
        rRenderContext.SetFillColor(aColFace);
        rRenderContext.SetLineColor(aColFace);
        rRenderContext.DrawRect(tools::Rectangle(Point(GetOutputSizePixel().Width() - m_nActualCornerWidth, aHScroll->GetPosPixel().Y()),
                                                 Size(m_nActualCornerWidth, m_nCornerHeight)));
        rRenderContext.Pop();
    }
}

void BrowseBox::Draw( OutputDevice* pDev, const Point& rPos, SystemTextColorFlags nFlags )
{
    // we need pixel coordinates
    Size aRealSize = GetSizePixel();
    Point aRealPos = pDev->LogicToPixel(rPos);

    if ((aRealSize.Width() < 3) || (aRealSize.Height() < 3))
        // we want to have two pixels frame ...
        return;

    vcl::Font aFont = pDataWin->GetDrawPixelFont( pDev );
        // the 'normal' painting uses always the data window as device to output to, so we have to calc the new font
        // relative to the data wins current settings

    pDev->Push();
    pDev->SetMapMode();
    pDev->SetFont( aFont );
    if (nFlags & SystemTextColorFlags::Mono)
        pDev->SetTextColor(COL_BLACK);
    else
        pDev->SetTextColor(pDataWin->GetTextColor());

    // draw a frame
    const StyleSettings& rStyleSettings = GetSettings().GetStyleSettings();
    pDev->SetLineColor(rStyleSettings.GetDarkShadowColor());
    pDev->DrawLine(Point(aRealPos.X(), aRealPos.Y()),
                   Point(aRealPos.X(), aRealPos.Y() + aRealSize.Height() - 1));
    pDev->DrawLine(Point(aRealPos.X(), aRealPos.Y()),
                   Point(aRealPos.X() + aRealSize.Width() - 1, aRealPos.Y()));
    pDev->SetLineColor(rStyleSettings.GetShadowColor());
    pDev->DrawLine(Point(aRealPos.X() + aRealSize.Width() - 1, aRealPos.Y() + 1),
                   Point(aRealPos.X() + aRealSize.Width() - 1, aRealPos.Y() + aRealSize.Height() - 1));
    pDev->DrawLine(Point(aRealPos.X() + aRealSize.Width() - 1, aRealPos.Y() + aRealSize.Height() - 1),
                   Point(aRealPos.X() + 1, aRealPos.Y() + aRealSize.Height() - 1));

    HeaderBar* pBar = pDataWin->pHeaderBar;

    // we're drawing onto a foreign device, so we have to fake the DataRowHeight for the subsequent ImplPaintData
    // (as it is based on the settings of our data window, not the foreign device)
    if (!m_nDataRowHeight)
        ImpGetDataRowHeight();
    tools::Long nHeightLogic = PixelToLogic(Size(0, m_nDataRowHeight), MapMode(MapUnit::Map10thMM)).Height();
    tools::Long nForeignHeightPixel = pDev->LogicToPixel(Size(0, nHeightLogic), MapMode(MapUnit::Map10thMM)).Height();

    tools::Long nOriginalHeight = m_nDataRowHeight;
    m_nDataRowHeight = nForeignHeightPixel;

    // this counts for the column widths, too
    size_t nPos;
    for ( nPos = 0; nPos < mvCols.size(); ++nPos )
    {
        BrowserColumn* pCurrent = mvCols[ nPos ].get();

        tools::Long nWidthLogic = PixelToLogic(Size(pCurrent->Width(), 0), MapMode(MapUnit::Map10thMM)).Width();
        tools::Long nForeignWidthPixel = pDev->LogicToPixel(Size(nWidthLogic, 0), MapMode(MapUnit::Map10thMM)).Width();

        pCurrent->SetWidth(nForeignWidthPixel, GetZoom());
        if ( pBar )
            pBar->SetItemSize( pCurrent->GetId(), pCurrent->Width() );
    }

    // a smaller area for the content
    aRealPos.AdjustX( 1 );
    aRealPos.AdjustY( 1 );
    aRealSize.AdjustWidth( -2 );
    aRealSize.AdjustHeight( -2 );

    // let the header bar draw itself
    if ( pBar )
    {
        // the title height with respect to the font set for the given device
        tools::Long nTitleHeight = PixelToLogic(Size(0, GetTitleHeight()), MapMode(MapUnit::Map10thMM)).Height();
        nTitleHeight = pDev->LogicToPixel(Size(0, nTitleHeight), MapMode(MapUnit::Map10thMM)).Height();

        BrowserColumn* pFirstCol = !mvCols.empty() ? mvCols[ 0 ].get() : nullptr;

        Point aHeaderPos(pFirstCol && (pFirstCol->GetId() == 0) ? pFirstCol->Width() : 0, 0);
        Size aHeaderSize(aRealSize.Width() - aHeaderPos.X(), nTitleHeight);

        aHeaderPos += aRealPos;
            // do this before converting to logics !

        // the header's draw expects logic coordinates, again
        aHeaderPos = pDev->PixelToLogic(aHeaderPos);

        Size aOrigSize(pBar->GetSizePixel());
        pBar->SetSizePixel(aHeaderSize);
        pBar->Draw(pDev, aHeaderPos, nFlags);
        pBar->SetSizePixel(aOrigSize);

        // draw the "upper left cell" (the intersection between the header bar and the handle column)
        if (pFirstCol && (pFirstCol->GetId() == 0) && (pFirstCol->Width() > 4))
        {
            ButtonFrame aButtonFrame( aRealPos,
                Size( pFirstCol->Width()-1, nTitleHeight-1 ),
                pFirstCol->Title(), !IsEnabled());
            aButtonFrame.Draw( *pDev );

            pDev->Push( vcl::PushFlags::LINECOLOR );
            pDev->SetLineColor( COL_BLACK );

            pDev->DrawLine( Point( aRealPos.X(), aRealPos.Y() + nTitleHeight-1 ),
               Point( aRealPos.X() + pFirstCol->Width() - 1, aRealPos.Y() + nTitleHeight-1 ) );
            pDev->DrawLine( Point( aRealPos.X() + pFirstCol->Width() - 1, aRealPos.Y() ),
               Point( aRealPos.X() + pFirstCol->Width() - 1, aRealPos.Y() + nTitleHeight-1 ) );

            pDev->Pop();
        }

        aRealPos.AdjustY(aHeaderSize.Height() );
        aRealSize.AdjustHeight( -(aHeaderSize.Height()) );
    }

    // draw our own content (with clipping)
    vcl::Region aRegion(tools::Rectangle(aRealPos, aRealSize));
    pDev->SetClipRegion( pDev->PixelToLogic( aRegion ) );

    // do we have to paint the background
    bool bBackground = pDataWin->IsControlBackground();
    if ( bBackground )
    {
        tools::Rectangle aRect( aRealPos, aRealSize );
        pDev->SetFillColor( pDataWin->GetControlBackground() );
        pDev->DrawRect( aRect );
    }

    ImplPaintData( *pDev, tools::Rectangle( aRealPos, aRealSize ), true );

    // restore the column widths/data row height
    m_nDataRowHeight = nOriginalHeight;
    for ( nPos = 0; nPos < mvCols.size(); ++nPos )
    {
        BrowserColumn* pCurrent = mvCols[ nPos ].get();

        tools::Long nForeignWidthLogic = pDev->PixelToLogic(Size(pCurrent->Width(), 0), MapMode(MapUnit::Map10thMM)).Width();
        tools::Long nWidthPixel = LogicToPixel(Size(nForeignWidthLogic, 0), MapMode(MapUnit::Map10thMM)).Width();

        pCurrent->SetWidth(nWidthPixel, GetZoom());
        if ( pBar )
            pBar->SetItemSize( pCurrent->GetId(), pCurrent->Width() );
    }

    pDev->Pop();
}

void BrowseBox::ImplPaintData(OutputDevice& _rOut, const tools::Rectangle& _rRect, bool _bForeignDevice)
{
    Point aOverallAreaPos = _bForeignDevice ? _rRect.TopLeft() : Point(0,0);
    Size aOverallAreaSize = _bForeignDevice ? _rRect.GetSize() : pDataWin->GetOutputSizePixel();
    Point aOverallAreaBRPos = _bForeignDevice ? _rRect.BottomRight() : Point( aOverallAreaSize.Width(), aOverallAreaSize.Height() );

    tools::Long nDataRowHeight = GetDataRowHeight();

    // compute relative rows to redraw
    sal_Int32 nRelTopRow = 0;
    sal_Int32 nRelBottomRow = aOverallAreaSize.Height();
    if (!_bForeignDevice && nDataRowHeight)
    {
        nRelTopRow = static_cast<sal_Int32>((_rRect.Top()) / nDataRowHeight);
        nRelBottomRow = static_cast<sal_Int32>((_rRect.Bottom()) / nDataRowHeight);
    }

    // cache frequently used values
    Point aPos( aOverallAreaPos.X(), nRelTopRow * nDataRowHeight + aOverallAreaPos.Y() );
    _rOut.SetLineColor( COL_WHITE );
    const AllSettings& rAllSets = _rOut.GetSettings();
    const StyleSettings &rSettings = rAllSets.GetStyleSettings();
    const Color &rHighlightTextColor = rSettings.GetHighlightTextColor();
    const Color &rHighlightFillColor = rSettings.GetHighlightColor();
    Color aOldTextColor = _rOut.GetTextColor();
    Color aOldFillColor = _rOut.GetFillColor();
    Color aOldLineColor = _rOut.GetLineColor();
    tools::Long nHLineX = 0 == mvCols[ 0 ]->GetId() ? mvCols[ 0 ]->Width() : 0;
    nHLineX += aOverallAreaPos.X();

    Color aDelimiterLineColor( ::svtools::ColorConfig().GetColorValue( ::svtools::CALCGRID ).nColor );

    // redraw the invalid fields
    for ( sal_Int32 nRelRow = nRelTopRow;
          nRelRow <= nRelBottomRow && nTopRow+nRelRow < nRowCount;
          ++nRelRow, aPos.AdjustY(nDataRowHeight ) )
    {
        // get row
        // check valid area, to be on the safe side:
        DBG_ASSERT( static_cast<sal_uInt16>(nTopRow+nRelRow) < nRowCount, "BrowseBox::ImplPaintData: invalid seek" );
        if ( (nTopRow+tools::Long(nRelRow)) < 0 || static_cast<sal_uInt16>(nTopRow+nRelRow) >= nRowCount )
            continue;

        // prepare row
        sal_Int32 nRow = nTopRow+nRelRow;
        if ( !SeekRow( nRow) ) {
            OSL_FAIL("BrowseBox::ImplPaintData: SeekRow failed");
        }
        _rOut.SetClipRegion();
        aPos.setX( aOverallAreaPos.X() );


        // #73325# don't paint the row outside the painting rectangle (DG)
        // prepare auto-highlight
        tools::Rectangle aRowRect( Point( _rRect.Left(), aPos.Y() ),
                Size( _rRect.GetSize().Width(), nDataRowHeight ) );

        bool bRowSelected   =   !bHideSelect
                            &&  IsRowSelected( nRow );
        if ( bRowSelected )
        {
            _rOut.SetTextColor( rHighlightTextColor );
            _rOut.SetFillColor( rHighlightFillColor );
            _rOut.SetLineColor();
            _rOut.DrawRect( aRowRect );
        }

        // iterate through columns to redraw
        size_t nCol;
        for ( nCol = 0; nCol < mvCols.size(); ++nCol )
        {
            // get column
            BrowserColumn *pCol = mvCols[ nCol ].get();

            // at end of invalid area
            if ( aPos.X() >= _rRect.Right() )
                break;

            // skip invisible columns between frozen and scrollable area
            if ( nCol < nFirstCol && !pCol->IsFrozen() )
            {
                nCol = nFirstCol;
                pCol = (nCol < mvCols.size() ) ? mvCols[ nCol ].get() : nullptr;
                if (!pCol)
                {   // FS - 21.05.99 - 66325
                    // actually this has been fixed elsewhere (in the right place),
                    // but let's make sure...
                    OSL_FAIL("BrowseBox::PaintData : nFirstCol is probably invalid !");
                    break;
                }
            }

            // prepare Column-AutoHighlight
            bool bColAutoHighlight  =   bColumnCursor
                                    &&  IsColumnSelected( pCol->GetId() );
            if ( bColAutoHighlight )
            {
                _rOut.SetClipRegion();
                _rOut.SetTextColor( rHighlightTextColor );
                _rOut.SetFillColor( rHighlightFillColor );
                _rOut.SetLineColor();
                tools::Rectangle aFieldRect( aPos,
                        Size( pCol->Width(), nDataRowHeight ) );
                _rOut.DrawRect( aFieldRect );
            }

            if (!m_bFocusOnlyCursor && (pCol->GetId() == GetCurColumnId()) && (nRow == GetCurRow()))
                DrawCursor();

            // draw a single field.
            // else something is drawn to, e.g. handle column
            if (pCol->Width())
            {
                // clip the column's output to the field area
                if (_bForeignDevice)
                {   // (not necessary if painting onto the data window)
                    Size aFieldSize(pCol->Width(), nDataRowHeight);

                    if (aPos.X() + aFieldSize.Width() > aOverallAreaBRPos.X())
                        aFieldSize.setWidth( aOverallAreaBRPos.X() - aPos.X() );

                    if (aPos.Y() + aFieldSize.Height() > aOverallAreaBRPos.Y() + 1)
                    {
                        // for non-handle cols we don't clip vertically : we just don't draw the cell if the line isn't completely visible
                        if (pCol->GetId() != 0)
                            continue;
                        aFieldSize.setHeight( aOverallAreaBRPos.Y() + 1 - aPos.Y() );
                    }

                    vcl::Region aClipToField(tools::Rectangle(aPos, aFieldSize));
                    _rOut.SetClipRegion(aClipToField);
                }
                pCol->Draw( *this, _rOut, aPos );
                if (_bForeignDevice)
                    _rOut.SetClipRegion();
            }

            // reset Column-auto-highlight
            if ( bColAutoHighlight )
            {
                _rOut.SetTextColor( aOldTextColor );
                _rOut.SetFillColor( aOldFillColor );
                _rOut.SetLineColor( aOldLineColor );
            }

            // skip column
            aPos.AdjustX(pCol->Width() );
        }

        // reset auto-highlight
        if ( bRowSelected )
        {
            _rOut.SetTextColor( aOldTextColor );
            _rOut.SetFillColor( aOldFillColor );
            _rOut.SetLineColor( aOldLineColor );
        }

        if ( bHLines )
        {
            // draw horizontal delimitation lines
            _rOut.SetClipRegion();
            _rOut.Push( vcl::PushFlags::LINECOLOR );
            _rOut.SetLineColor( aDelimiterLineColor );
            tools::Long nY = aPos.Y() + nDataRowHeight - 1;
            if (nY <= aOverallAreaBRPos.Y())
                _rOut.DrawLine( Point( nHLineX, nY ),
                                Point( bVLines
                                        ? std::min(tools::Long(aPos.X() - 1), aOverallAreaBRPos.X())
                                        : aOverallAreaBRPos.X(),
                                      nY ) );
            _rOut.Pop();
        }
    }

    if (aPos.Y() > aOverallAreaBRPos.Y() + 1)
        aPos.setY( aOverallAreaBRPos.Y() + 1 );
        // needed for some of the following drawing

    // retouching
    _rOut.SetClipRegion();
    aOldLineColor = _rOut.GetLineColor();
    aOldFillColor = _rOut.GetFillColor();
    _rOut.SetFillColor( rSettings.GetFaceColor() );
    if ( !mvCols.empty() && ( mvCols[ 0 ]->GetId() == 0 ) && ( aPos.Y() <= _rRect.Bottom() ) )
    {
        // fill rectangle gray below handle column
        // DG: fill it only until the end of the drawing rect and not to the end, as this may overpaint handle columns
        _rOut.SetLineColor( COL_BLACK );
        _rOut.DrawRect( tools::Rectangle(
            Point( aOverallAreaPos.X() - 1, aPos.Y() - 1 ),
            Point( aOverallAreaPos.X() + mvCols[ 0 ]->Width() - 1,
                   _rRect.Bottom() + 1) ) );
    }
    _rOut.SetFillColor( aOldFillColor );

    // draw vertical delimitational line between frozen and scrollable cols
    _rOut.SetLineColor( COL_BLACK );
    tools::Long nFrozenWidth = GetFrozenWidth()-1;
    _rOut.DrawLine( Point( aOverallAreaPos.X() + nFrozenWidth, aPos.Y() ),
                   Point( aOverallAreaPos.X() + nFrozenWidth, bHLines
                            ? aPos.Y() - 1
                            : aOverallAreaBRPos.Y() ) );

    // draw vertical delimitational lines?
    if ( bVLines )
    {
        _rOut.SetLineColor( aDelimiterLineColor );
        Point aVertPos( aOverallAreaPos.X() - 1, aOverallAreaPos.Y() );
        tools::Long nDeltaY = aOverallAreaBRPos.Y();
        for ( size_t nCol = 0; nCol < mvCols.size(); ++nCol )
        {
            // get column
            BrowserColumn *pCol = mvCols[ nCol ].get();

            // skip invisible columns between frozen and scrollable area
            if ( nCol < nFirstCol && !pCol->IsFrozen() )
            {
                nCol = nFirstCol;
                pCol = mvCols[ nCol ].get();
            }

            // skip column
            aVertPos.AdjustX(pCol->Width() );

            // at end of invalid area
            // invalid area is first reached when X > Right
            // and not >=
            if ( aVertPos.X() > _rRect.Right() )
                break;

            // draw a single line
            if ( pCol->GetId() != 0 )
                _rOut.DrawLine( aVertPos, Point( aVertPos.X(),
                               bHLines
                                ? aPos.Y() - 1
                                : aPos.Y() + nDeltaY ) );
        }
    }

    _rOut.SetLineColor( aOldLineColor );
}

void BrowseBox::PaintData( vcl::Window const & rWin, vcl::RenderContext& rRenderContext, const tools::Rectangle& rRect )
{
    if (!bBootstrapped && IsReallyVisible())
        BrowseBox::StateChanged(StateChangedType::InitShow);

    // initializations
    if (mvCols.empty() || !rWin.IsUpdateMode())
        return;
    if (pDataWin->bResizeOnPaint)
        Resize();
    // MI: who was that? Window::Update();

    ImplPaintData(rRenderContext, rRect, false);
}

void BrowseBox::UpdateScrollbars()
{

    if ( !bBootstrapped || !IsUpdateMode() )
        return;

    // protect against recursion
    if ( pDataWin->bInUpdateScrollbars )
    {
        pDataWin->bHadRecursion = true;
        return;
    }
    pDataWin->bInUpdateScrollbars = true;

    // the size of the corner window (and the width of the VSB/height of the HSB)
    m_nCornerHeight = GetBarHeight();
    m_nCornerWidth = GetSettings().GetStyleSettings().GetScrollBarSize();
    if (IsZoom())
    {
        m_nCornerHeight = static_cast<tools::Long>(m_nCornerHeight * static_cast<double>(GetZoom()));
        m_nCornerWidth = static_cast<tools::Long>(m_nCornerWidth * static_cast<double>(GetZoom()));
    }

    bool bNeedsVScroll = false;
    sal_Int32 nMaxRows = 0;
    if (GetDataRowHeight())
    {
        // needs VScroll?
        nMaxRows = (pDataWin->GetSizePixel().Height()) / GetDataRowHeight();
        bNeedsVScroll =    pDataWin->bAutoVScroll
                            ?   nTopRow || ( nRowCount > nMaxRows )
                            :   !pDataWin->bNoVScroll;
    }
    Size aDataWinSize = pDataWin->GetSizePixel();
    if ( !bNeedsVScroll )
    {
        if ( pVScroll->IsVisible() )
        {
            pVScroll->Hide();
            Size aNewSize( aDataWinSize );
            aNewSize.setWidth( GetOutputSizePixel().Width() );
            aDataWinSize = aNewSize;
        }
    }
    else if ( !pVScroll->IsVisible() )
    {
        Size aNewSize( aDataWinSize );
        aNewSize.setWidth( GetOutputSizePixel().Width() - m_nCornerWidth );
        aDataWinSize = aNewSize;
    }

    // needs HScroll?
    sal_uInt16 nLastCol = GetColumnAtXPosPixel( aDataWinSize.Width() - 1 );

    sal_uInt16 nFrozenCols = FrozenColCount();
    bool bNeedsHScroll =    pDataWin->bAutoHScroll
                        ?   ( nFirstCol > nFrozenCols ) || ( nLastCol <= mvCols.size() )
                        :   !pDataWin->bNoHScroll;
    if ( !bNeedsHScroll )
    {
        if ( aHScroll->IsVisible() )
        {
            aHScroll->Hide();
        }
        aDataWinSize.setHeight( GetOutputSizePixel().Height() - GetTitleHeight() );
        if ( nControlAreaWidth != USHRT_MAX )
            aDataWinSize.AdjustHeight( -sal_Int32(m_nCornerHeight) );
    }
    else if ( !aHScroll->IsVisible() )
    {
        Size aNewSize( aDataWinSize );
        aNewSize.setHeight( GetOutputSizePixel().Height() - GetTitleHeight() - m_nCornerHeight );
        aDataWinSize = aNewSize;
    }

    // adjust position and Width of horizontal scrollbar
    tools::Long nHScrX = nControlAreaWidth == USHRT_MAX
        ? 0
        : nControlAreaWidth;

    aHScroll->SetPosSizePixel(
        Point( nHScrX, GetOutputSizePixel().Height() - m_nCornerHeight ),
        Size( aDataWinSize.Width() - nHScrX, m_nCornerHeight ) );

    // total scrollable columns
    short nScrollCols = short(mvCols.size()) - static_cast<short>(nFrozenCols);

    // visible columns
    short nVisibleHSize = nLastCol == BROWSER_INVALIDID
        ? static_cast<short>( mvCols.size() - nFirstCol )
        : static_cast<short>( nLastCol - nFirstCol );

    if (nVisibleHSize)
    {
        short nRange = std::max( nScrollCols, short(0) );
        aHScroll->SetVisibleSize( nVisibleHSize );
        aHScroll->SetRange( Range( 0, nRange ));
    }
    else
    {
        // ensure scrollbar is shown as fully filled
        aHScroll->SetVisibleSize(1);
        aHScroll->SetRange(Range(0, 1));
    }
    if ( bNeedsHScroll && !aHScroll->IsVisible() )
        aHScroll->Show();

    // adjust position and height of vertical scrollbar
    pVScroll->SetPageSize( nMaxRows );

    if ( nTopRow > nRowCount )
    {
        nTopRow = nRowCount - 1;
        OSL_FAIL("BrowseBox: nTopRow > nRowCount");
    }

    if ( pVScroll->GetThumbPos() != nTopRow )
        pVScroll->SetThumbPos( nTopRow );
    tools::Long nVisibleSize = std::min( std::min( nRowCount, nMaxRows ), (nRowCount-nTopRow) );
    pVScroll->SetVisibleSize( nVisibleSize ? nVisibleSize : 1 );
    pVScroll->SetRange( Range( 0, nRowCount ) );
    pVScroll->SetPosSizePixel(
        Point( aDataWinSize.Width(), GetTitleHeight() ),
        Size( m_nCornerWidth, aDataWinSize.Height()) );
    tools::Long nLclDataRowHeight = GetDataRowHeight();
    if ( nLclDataRowHeight > 0 && nRowCount < tools::Long( aDataWinSize.Height() / nLclDataRowHeight ) )
        ScrollRows( -nTopRow );
    if ( bNeedsVScroll && !pVScroll->IsVisible() )
        pVScroll->Show();

    pDataWin->SetPosSizePixel(
        Point( 0, GetTitleHeight() ),
        aDataWinSize );

    // needs corner-window?
    // (do that AFTER positioning BOTH scrollbars)
    m_nActualCornerWidth = 0;
    if (aHScroll->IsVisible() && pVScroll && pVScroll->IsVisible() )
    {
        // if we have both scrollbars, the corner window fills the point of intersection of these two
        m_nActualCornerWidth = m_nCornerWidth;
    }
    else if ( !aHScroll->IsVisible() && ( nControlAreaWidth != USHRT_MAX ) )
    {
        // if we have no horizontal scrollbar, but a control area, we need the corner window to
        // fill the space between the control are and the right border
        m_nActualCornerWidth = GetOutputSizePixel().Width() - nControlAreaWidth;
    }

    // scroll headerbar, if necessary
    if ( pDataWin->pHeaderBar )
    {
        tools::Long nWidth = 0;
        for ( size_t nCol = 0;
              nCol < mvCols.size() && nCol < nFirstCol;
              ++nCol )
        {
            // not the handle column
            if ( mvCols[ nCol ]->GetId() )
                nWidth += mvCols[ nCol ]->Width();
        }

        pDataWin->pHeaderBar->SetOffset( nWidth );
    }

    pDataWin->bInUpdateScrollbars = false;
    if ( pDataWin->bHadRecursion )
    {
        pDataWin->bHadRecursion = false;
        UpdateScrollbars();
    }
}


void BrowseBox::SetUpdateMode( bool bUpdate )
{

    bool bWasUpdate = IsUpdateMode();
    if ( bWasUpdate == bUpdate )
        return;

    Control::SetUpdateMode( bUpdate );
    // If WB_CLIPCHILDREN is st at the BrowseBox (to minimize flicker),
    // the data window is not invalidated by SetUpdateMode.
    if( bUpdate )
        pDataWin->Invalidate();
    pDataWin->SetUpdateMode( bUpdate );


    if ( bUpdate )
    {
        if ( bBootstrapped )
        {
            UpdateScrollbars();
            AutoSizeLastColumn();
        }
        DoShowCursor();
    }
    else
        DoHideCursor();
}


bool BrowseBox::GetUpdateMode() const
{

    return pDataWin->IsUpdateMode();
}


tools::Long BrowseBox::GetFrozenWidth() const
{

    tools::Long nWidth = 0;
    for ( size_t nCol = 0;
          nCol < mvCols.size() && mvCols[ nCol ]->IsFrozen();
          ++nCol )
        nWidth += mvCols[ nCol ]->Width();
    return nWidth;
}

void BrowseBox::ColumnInserted( sal_uInt16 nPos )
{
    if ( pColSel )
        pColSel->Insert( nPos );
    UpdateScrollbars();
}

sal_uInt16 BrowseBox::FrozenColCount() const
{
    std::size_t nCol;
    for ( nCol = 0;
          nCol < mvCols.size() && mvCols[ nCol ]->IsFrozen();
          ++nCol )
        /* empty loop */;
    return nCol; //TODO: BrowserColumns::size_type -> sal_uInt16!
}

IMPL_LINK(BrowseBox, VertScrollHdl, weld::Scrollbar&, rScrollbar, void)
{
    auto nCurScrollRow = nTopRow;
    auto nPos = rScrollbar.adjustment_get_value();
    ScrollRows(nPos - nCurScrollRow);

    bool bShowTooltip = ((m_nCurrentMode & BrowserMode::TRACKING_TIPS) == BrowserMode::TRACKING_TIPS);
    if (bShowTooltip &&
        rScrollbar.get_scroll_type() == ScrollType::Drag &&
        Help::IsQuickHelpEnabled())
    {
        OUString aTip = OUString::number(nPos) + "/";
        if (!pDataWin->GetRealRowCount().isEmpty())
            aTip += pDataWin->GetRealRowCount();
        else
            aTip += OUString::number(rScrollbar.adjustment_get_upper());
        tools::Rectangle aRect(GetPointerPosPixel(), Size(GetTextWidth(aTip), GetTextHeight()));
        Help::ShowQuickHelp(this, aRect, aTip);
    }
}

IMPL_LINK(BrowseBox, HorzScrollHdl, weld::Scrollbar&, rScrollbar, void)
{
    auto nCurScrollCol = nFirstCol - FrozenColCount();
    ScrollColumns(rScrollbar.adjustment_get_value() - nCurScrollCol);
}

IMPL_LINK( BrowseBox, StartDragHdl, HeaderBar*, pBar, void )
{
    pBar->SetDragSize( pDataWin->GetOutputSizePixel().Height() );
}

// usually only the first column was resized
void BrowseBox::MouseButtonDown( const MouseEvent& rEvt )
{

    GrabFocus();

    // only mouse events in the title-line are supported
    const Point &rEvtPos = rEvt.GetPosPixel();
    if ( rEvtPos.Y() >= GetTitleHeight() )
        return;

    tools::Long nX = 0;
    tools::Long nWidth = GetOutputSizePixel().Width();
    for ( size_t nCol = 0; nCol < mvCols.size() && nX < nWidth; ++nCol )
    {
        // is this column visible?
        BrowserColumn *pCol = mvCols[ nCol ].get();
        if ( pCol->IsFrozen() || nCol >= nFirstCol )
        {
            // compute right end of column
            tools::Long nR = nX + pCol->Width() - 1;

            // at the end of a column (and not handle column)?
            if ( pCol->GetId() && std::abs( nR - rEvtPos.X() ) < 2 )
            {
                // start resizing the column
                bResizing = true;
                nResizeCol = nCol;
                nDragX = nResizeX = rEvtPos.X();
                SetPointer( PointerStyle::HSplit );
                CaptureMouse();
                pDataWin->GetOutDev()->DrawLine( Point( nDragX, 0 ),
                    Point( nDragX, pDataWin->GetSizePixel().Height() ) );
                nMinResizeX = nX + MIN_COLUMNWIDTH;
                return;
            }
            else if ( nX < rEvtPos.X() && nR > rEvtPos.X() )
            {
                MouseButtonDown( BrowserMouseEvent(
                    this, rEvt, -1, nCol, pCol->GetId(), tools::Rectangle() ) );
                return;
            }
            nX = nR + 1;
        }
    }

    // event occurred out of data area
    if ( rEvt.IsRight() )
        pDataWin->Command(
            CommandEvent( Point( 1, LONG_MAX ), CommandEventId::ContextMenu, true ) );
    else
        SetNoSelection();
}


void BrowseBox::MouseMove( const MouseEvent& rEvt )
{
    SAL_INFO("svtools", "BrowseBox::MouseMove( MouseEvent )" );

    PointerStyle aNewPointer = PointerStyle::Arrow;

    sal_uInt16 nX = 0;
    for ( size_t nCol = 0;
          nCol < mvCols.size() &&
            ( nX + mvCols[ nCol ]->Width() ) < GetOutputSizePixel().Width();
          ++nCol )
        // is this column visible?
        if ( mvCols[ nCol ]->IsFrozen() || nCol >= nFirstCol )
        {
            // compute right end of column
            BrowserColumn *pCol = mvCols[ nCol ].get();
            sal_uInt16 nR = static_cast<sal_uInt16>(nX + pCol->Width() - 1);

            // show resize-pointer?
            if ( bResizing || ( pCol->GetId() &&
                 std::abs( static_cast<tools::Long>(nR) - rEvt.GetPosPixel().X() ) < MIN_COLUMNWIDTH ) )
            {
                aNewPointer = PointerStyle::HSplit;
                if ( bResizing )
                {
                    // delete old auxiliary line
                    pDataWin->HideTracking() ;

                    // check allowed width and new delta
                    nDragX = std::max( rEvt.GetPosPixel().X(), nMinResizeX );
                    tools::Long nDeltaX = nDragX - nResizeX;
                    sal_uInt16 nId = GetColumnId(nResizeCol);
                    tools::Long nOldWidth = GetColumnWidth(nId);
                    nDragX = nOldWidth + nDeltaX + nResizeX - nOldWidth;

                    // draw new auxiliary line
                    pDataWin->ShowTracking( tools::Rectangle( Point( nDragX, 0 ),
                            Size( 1, pDataWin->GetSizePixel().Height() ) ),
                            ShowTrackFlags::Split|ShowTrackFlags::TrackWindow );
                }

            }

            nX = nR + 1;
        }

    SetPointer( aNewPointer );
}


void BrowseBox::MouseButtonUp( const MouseEvent & rEvt )
{

    if ( bResizing )
    {
        // delete auxiliary line
        pDataWin->HideTracking();

        // width changed?
        nDragX = std::max( rEvt.GetPosPixel().X(), nMinResizeX );
        if ( (nDragX - nResizeX) != mvCols[ nResizeCol ]->Width() )
        {
            // resize column
            tools::Long nMaxX = pDataWin->GetSizePixel().Width();
            nDragX = std::min( nDragX, nMaxX );
            tools::Long nDeltaX = nDragX - nResizeX;
            sal_uInt16 nId = GetColumnId(nResizeCol);
            SetColumnWidth( GetColumnId(nResizeCol), GetColumnWidth(nId) + nDeltaX );
            ColumnResized( nId );
        }

        // end action
        SetPointer( PointerStyle::Arrow );
        ReleaseMouse();
        bResizing = false;
    }
    else
        MouseButtonUp( BrowserMouseEvent( pDataWin,
                MouseEvent( Point( rEvt.GetPosPixel().X(),
                        rEvt.GetPosPixel().Y() - pDataWin->GetPosPixel().Y() ),
                    rEvt.GetClicks(), rEvt.GetMode(), rEvt.GetButtons(),
                    rEvt.GetModifier() ) ) );
}


static bool bExtendedMode = false;
static bool bFieldMode = false;

void BrowseBox::MouseButtonDown( const BrowserMouseEvent& rEvt )
{

    GrabFocus();

    // adjust selection while and after double-click
    if ( rEvt.GetClicks() == 2 )
    {
        SetNoSelection();
        if ( rEvt.GetRow() >= 0 )
        {
            GoToRow( rEvt.GetRow() );
            SelectRow( rEvt.GetRow(), true, false );
        }
        else
        {
            if ( bColumnCursor && rEvt.GetColumn() != 0 )
            {
                if ( rEvt.GetColumn() < mvCols.size() )
                    SelectColumnPos( rEvt.GetColumn(), true, false);
            }
        }
        DoubleClick( rEvt );
    }
    // selections
    else if ( ( rEvt.GetMode() & ( MouseEventModifiers::SELECT | MouseEventModifiers::SIMPLECLICK ) ) &&
         ( bColumnCursor || rEvt.GetRow() >= 0 ) )
    {
        if ( rEvt.GetClicks() == 1 )
        {
            // initialise flags
            bHit            = false;

            // selection out of range?
            if ( rEvt.GetRow() >= nRowCount ||
                 rEvt.GetColumnId() == BROWSER_INVALIDID )
            {
                SetNoSelection();
                return;
            }

            // while selecting, no cursor
            bSelecting = true;
            DoHideCursor();

            // DataRow?
            if ( rEvt.GetRow() >= 0 )
            {
                // line selection?
                if ( rEvt.GetColumnId() == HandleColumnId || !bColumnCursor )
                {
                    if ( bMultiSelection )
                    {
                        // remove column-selection, if exists
                        if ( pColSel && pColSel->GetSelectCount() )
                        {
                            ToggleSelection();
                            if ( bMultiSelection )
                                uRow.pSel->SelectAll(false);
                            else
                                uRow.nSel = BROWSER_ENDOFSELECTION;
                            if ( pColSel )
                                pColSel->SelectAll(false);
                            bSelect = true;
                        }

                        // expanding mode?
                        if ( rEvt.GetMode() & MouseEventModifiers::RANGESELECT )
                        {
                            // select the further touched rows too
                            bSelect = true;
                            ExpandRowSelection( rEvt );
                            return;
                        }

                        // click in the selected area?
                        else if ( IsRowSelected( rEvt.GetRow() ) )
                        {
                            // wait for Drag&Drop
                            bHit = true;
                            bExtendedMode = bool( rEvt.GetMode() & MouseEventModifiers::MULTISELECT );
                            return;
                        }

                        // extension mode?
                        else if ( rEvt.GetMode() & MouseEventModifiers::MULTISELECT )
                        {
                            // determine the new selection range
                            // and selection/deselection
                            aSelRange = Range( rEvt.GetRow(), rEvt.GetRow() );
                            SelectRow( rEvt.GetRow(),
                                    !uRow.pSel->IsSelected( rEvt.GetRow() ) );
                            bSelect = true;
                            return;
                        }
                    }

                    // select directly
                    SetNoSelection();
                    GoToRow( rEvt.GetRow() );
                    SelectRow( rEvt.GetRow() );
                    aSelRange = Range( rEvt.GetRow(), rEvt.GetRow() );
                    bSelect = true;
                }
                else // Column/Field-Selection
                {
                    // click in selected column
                    if ( IsColumnSelected( rEvt.GetColumn() ) ||
                         IsRowSelected( rEvt.GetRow() ) )
                    {
                        bHit = true;
                        bFieldMode = true;
                        return;
                    }

                    SetNoSelection();
                    GoToRowColumnId( rEvt.GetRow(), rEvt.GetColumnId() );
                    bSelect = true;
                }
            }
            else
            {
                if ( bMultiSelection && rEvt.GetColumnId() == HandleColumnId )
                {
                    // toggle all-selection
                    if ( uRow.pSel->GetSelectCount() > ( GetRowCount() / 2 ) )
                        SetNoSelection();
                    else
                        SelectAll();
                }
                else
                    SelectColumnPos( GetColumnPos(rEvt.GetColumnId()), true, false);
            }

            // turn cursor on again, if necessary
            bSelecting = false;
            DoShowCursor();
            if ( bSelect )
                Select();
        }
    }
}


void BrowseBox::MouseButtonUp( const BrowserMouseEvent &rEvt )
{

    // D&D was possible, but did not occur
    if ( bHit )
    {
        aSelRange = Range( rEvt.GetRow(), rEvt.GetRow() );
        if ( bExtendedMode )
            SelectRow( rEvt.GetRow(), false );
        else
        {
            SetNoSelection();
            if ( bFieldMode )
                GoToRowColumnId( rEvt.GetRow(), rEvt.GetColumnId() );
            else
            {
                GoToRow( rEvt.GetRow() );
                SelectRow( rEvt.GetRow() );
            }
        }
        bSelect = true;
        bExtendedMode = false;
        bFieldMode = false;
        bHit = false;
    }

    // activate cursor
    if ( bSelecting )
    {
        bSelecting = false;
        DoShowCursor();
        if ( bSelect )
            Select();
    }
}


void BrowseBox::KeyInput( const KeyEvent& rEvt )
{
    if ( !ProcessKey( rEvt ) )
        Control::KeyInput( rEvt );
}


bool BrowseBox::ProcessKey( const KeyEvent& rEvt )
{

    sal_uInt16 nCode = rEvt.GetKeyCode().GetCode();
    bool       bShift = rEvt.GetKeyCode().IsShift();
    bool       bCtrl = rEvt.GetKeyCode().IsMod1();
    bool       bAlt = rEvt.GetKeyCode().IsMod2();

    BrowserDispatchId eId = BrowserDispatchId::NONE;

    if ( !bAlt && !bCtrl && !bShift )
    {
        switch ( nCode )
        {
            case KEY_DOWN:
                eId = BrowserDispatchId::CURSORDOWN;
                break;
            case KEY_UP:
                eId = BrowserDispatchId::CURSORUP;
                break;
            case KEY_HOME:
                eId = BrowserDispatchId::CURSORHOME;
                break;
            case KEY_END:
                eId = BrowserDispatchId::CURSOREND;
                break;
            case KEY_TAB:
                if ( !bColumnCursor )
                    break;
                [[fallthrough]];
            case KEY_RIGHT:
                eId = BrowserDispatchId::CURSORRIGHT;
                break;
            case KEY_LEFT:
                eId = BrowserDispatchId::CURSORLEFT;
                break;
            case KEY_SPACE:
                eId = BrowserDispatchId::SELECT;
                break;
        }
        if (BrowserDispatchId::NONE != eId)
            SetNoSelection();

        switch ( nCode )
        {
            case KEY_PAGEDOWN:
                eId = BrowserDispatchId::CURSORPAGEDOWN;
                break;
            case KEY_PAGEUP:
                eId = BrowserDispatchId::CURSORPAGEUP;
                break;
        }
    }

    if ( !bAlt && !bCtrl && bShift )
        switch ( nCode )
        {
            case KEY_DOWN:
                eId = BrowserDispatchId::SELECTDOWN;
                break;
            case KEY_UP:
                eId = BrowserDispatchId::SELECTUP;
                break;
            case KEY_TAB:
                if ( !bColumnCursor )
                    break;
                eId = BrowserDispatchId::CURSORLEFT;
                break;
            case KEY_HOME:
                eId = BrowserDispatchId::SELECTHOME;
                break;
            case KEY_END:
                eId = BrowserDispatchId::SELECTEND;
                break;
        }


    if ( !bAlt && bCtrl && !bShift )
        switch ( nCode )
        {
            case KEY_DOWN:
                eId = BrowserDispatchId::CURSORDOWN;
                break;
            case KEY_UP:
                eId = BrowserDispatchId::CURSORUP;
                break;
            case KEY_PAGEDOWN:
                eId = BrowserDispatchId::CURSORENDOFFILE;
                break;
            case KEY_PAGEUP:
                eId = BrowserDispatchId::CURSORTOPOFFILE;
                break;
            case KEY_HOME:
                eId = BrowserDispatchId::CURSORTOPOFSCREEN;
                break;
            case KEY_END:
                eId = BrowserDispatchId::CURSORENDOFSCREEN;
                break;
            case KEY_SPACE:
                eId = BrowserDispatchId::ENHANCESELECTION;
                break;
            case KEY_LEFT:
                eId = BrowserDispatchId::MOVECOLUMNLEFT;
                break;
            case KEY_RIGHT:
                eId = BrowserDispatchId::MOVECOLUMNRIGHT;
                break;
        }

    if (eId != BrowserDispatchId::NONE)
        Dispatch( eId );
    return eId != BrowserDispatchId::NONE;
}

void BrowseBox::ChildFocusIn()
{
}

void BrowseBox::ChildFocusOut()
{
}

void BrowseBox::Dispatch(BrowserDispatchId eId)
{

    tools::Long nRowsOnPage = pDataWin->GetSizePixel().Height() / GetDataRowHeight();

    switch (eId)
    {
        case BrowserDispatchId::SELECTCOLUMN:
            if ( ColCount() )
                SelectColumnId( GetCurColumnId() );
            break;

        case BrowserDispatchId::CURSORDOWN:
            if ( ( GetCurRow() + 1 ) < nRowCount )
                GoToRow( GetCurRow() + 1, false );
            break;
        case BrowserDispatchId::CURSORUP:
            if ( GetCurRow() > 0 )
                GoToRow( GetCurRow() - 1, false );
            break;
        case BrowserDispatchId::SELECTHOME:
            if ( GetRowCount() )
            {
                DoHideCursor();
                for ( sal_Int32 nRow = GetCurRow(); nRow >= 0; --nRow )
                    SelectRow( nRow );
                GoToRow( 0, true );
                DoShowCursor();
            }
            break;
        case BrowserDispatchId::SELECTEND:
            if ( GetRowCount() )
            {
                DoHideCursor();
                sal_Int32 nRows = GetRowCount();
                for ( sal_Int32 nRow = GetCurRow(); nRow < nRows; ++nRow )
                    SelectRow( nRow );
                GoToRow( GetRowCount() - 1, true );
                DoShowCursor();
            }
            break;
        case BrowserDispatchId::SELECTDOWN:
        {
            if ( GetRowCount() && ( GetCurRow() + 1 ) < nRowCount )
            {
                // deselect the current row, if it isn't the first
                // and there is no other selected row above
                sal_Int32 nRow = GetCurRow();
                bool bLocalSelect = ( !IsRowSelected( nRow ) ||
                                 GetSelectRowCount() == 1 || IsRowSelected( nRow - 1 ) );
                SelectRow( nRow, bLocalSelect );
                bool bDone = GoToRow( GetCurRow() + 1, false );
                if ( bDone )
                    SelectRow( GetCurRow() );
            }
            else
                ScrollRows( 1 );
            break;
        }
        case BrowserDispatchId::SELECTUP:
            if ( GetRowCount() )
            {
                // deselect the current row, if it isn't the first
                // and there is no other selected row under
                sal_Int32 nRow = GetCurRow();
                bool bLocalSelect = ( !IsRowSelected( nRow ) ||
                                 GetSelectRowCount() == 1 || IsRowSelected( nRow + 1 ) );
                SelectRow( nCurRow, bLocalSelect );
                bool bDone = GoToRow( nRow - 1, false );
                if ( bDone )
                    SelectRow( GetCurRow() );
            }
            break;
        case BrowserDispatchId::CURSORPAGEDOWN:
            ScrollRows( nRowsOnPage );
            break;
        case BrowserDispatchId::CURSORPAGEUP:
            ScrollRows( -nRowsOnPage );
            break;
        case BrowserDispatchId::CURSOREND:
            if ( bColumnCursor )
            {
                sal_uInt16 nNewId = GetColumnId(ColCount() -1);
                nNewId != HandleColumnId && GoToColumnId( nNewId );
                break;
            }
            [[fallthrough]];
        case BrowserDispatchId::CURSORENDOFFILE:
            GoToRow( nRowCount - 1, false );
            break;
        case BrowserDispatchId::CURSORRIGHT:
            if ( bColumnCursor )
            {
                sal_uInt16 nNewPos = GetColumnPos( GetCurColumnId() ) + 1;
                sal_uInt16 nNewId = GetColumnId( nNewPos );
                if (nNewId != BROWSER_INVALIDID)    // At end of row ?
                    GoToColumnId( nNewId );
                else
                {
                    sal_uInt16 nColId = GetColumnId(0);
                    if ( nColId == BROWSER_INVALIDID || nColId == HandleColumnId )
                        nColId = GetColumnId(1);
                    if ( GetRowCount() )
                    {
                        if ( nCurRow < GetRowCount() - 1 )
                        {
                            GoToRowColumnId( nCurRow + 1, nColId );
                        }
                    }
                    else if ( ColCount() )
                        GoToColumnId( nColId );
                }
            }
            else
                ScrollColumns( 1 );
            break;
        case BrowserDispatchId::CURSORHOME:
            if ( bColumnCursor )
            {
                sal_uInt16 nNewId = GetColumnId(1);
                if (nNewId != HandleColumnId)
                {
                    GoToColumnId( nNewId );
                }
                break;
            }
            [[fallthrough]];
        case BrowserDispatchId::CURSORTOPOFFILE:
            GoToRow( 0, false );
            break;
        case BrowserDispatchId::CURSORLEFT:
            if ( bColumnCursor )
            {
                sal_uInt16 nNewPos = GetColumnPos( GetCurColumnId() ) - 1;
                sal_uInt16 nNewId = GetColumnId( nNewPos );
                if (nNewId != HandleColumnId)
                    GoToColumnId( nNewId );
                else
                {
                    if ( GetRowCount() )
                    {
                        if (nCurRow > 0)
                        {
                            GoToRowColumnId(nCurRow - 1, GetColumnId(ColCount() -1));
                        }
                    }
                    else if ( ColCount() )
                        GoToColumnId( GetColumnId(ColCount() -1) );
                }
            }
            else
                ScrollColumns( -1 );
            break;
        case BrowserDispatchId::ENHANCESELECTION:
            if ( GetRowCount() )
                SelectRow( GetCurRow(), !IsRowSelected( GetCurRow() ) );
            break;
        case BrowserDispatchId::SELECT:
            if ( GetRowCount() )
                SelectRow( GetCurRow(), !IsRowSelected( GetCurRow() ), false );
            break;
        case BrowserDispatchId::MOVECOLUMNLEFT:
        case BrowserDispatchId::MOVECOLUMNRIGHT:
            { // check if column moving is allowed
                BrowserHeader* pHeaderBar = pDataWin->pHeaderBar;
                if ( pHeaderBar && pHeaderBar->IsDragable() )
                {
                    sal_uInt16 nColId = GetCurColumnId();
                    bool bColumnSelected = IsColumnSelected(nColId);
                    sal_uInt16 nNewPos = GetColumnPos(nColId);
                    bool bMoveAllowed = false;
                    if (BrowserDispatchId::MOVECOLUMNLEFT == eId && nNewPos > 1)
                    {
                        --nNewPos;
                        bMoveAllowed = true;
                    }
                    else if (BrowserDispatchId::MOVECOLUMNRIGHT == eId && nNewPos < (ColCount() - 1))
                    {
                        ++nNewPos;
                        bMoveAllowed = true;
                    }

                    if ( bMoveAllowed )
                    {
                        SetColumnPos( nColId, nNewPos );
                        ColumnMoved( nColId );
                        MakeFieldVisible(GetCurRow(), nColId);
                        if ( bColumnSelected )
                            SelectColumnId(nColId);
                    }
                }
            }
            break;
        default:
            break;
    }
}


void BrowseBox::SetCursorColor(const Color& _rCol)
{
    if (_rCol == m_aCursorColor)
        return;

    // ensure the cursor is hidden
    DoHideCursor();
    if (!m_bFocusOnlyCursor)
        DoHideCursor();

    m_aCursorColor = _rCol;

    if (!m_bFocusOnlyCursor)
        DoShowCursor();
    DoShowCursor();
}

tools::Rectangle BrowseBox::calcHeaderRect(bool _bIsColumnBar)
{
    Point aTopLeft;
    tools::Long nWidth;
    tools::Long nHeight;
    if ( _bIsColumnBar )
    {
        nWidth = pDataWin->GetOutputSizePixel().Width();
        nHeight = GetDataRowHeight();
    }
    else
    {
        aTopLeft.setY( GetDataRowHeight() );
        nWidth = GetColumnWidth(0);
        nHeight = GetWindowExtentsAbsolute().GetHeight() - aTopLeft.Y() - GetControlArea().GetSize().Height();
    }
    return tools::Rectangle(aTopLeft,Size(nWidth,nHeight));
}

tools::Rectangle BrowseBox::calcTableRect()
{
    tools::Rectangle aRect(GetWindowExtentsAbsolute());
    aRect.SetPos(Point(0, 0));
    tools::Rectangle aRowBar = calcHeaderRect(false);

    tools::Long nX = aRowBar.Right() - aRect.Left();
    tools::Long nY = aRowBar.Top() - aRect.Top();
    Size aSize(aRect.GetSize());

    return tools::Rectangle(aRowBar.TopRight(), Size(aSize.Width() - nX, aSize.Height() - nY - GetBarHeight()) );
}

tools::Rectangle BrowseBox::calcFieldRectPixel(sal_Int32 _nRowId, sal_uInt16 _nColId, bool /*_bIsHeader*/)
{
    return GetFieldRectPixel(_nRowId, _nColId, true);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
