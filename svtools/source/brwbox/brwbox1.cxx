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

#include <comphelper/diagnose_ex.hxx>
#include <svtools/brwbox.hxx>
#include <svtools/brwhead.hxx>
#include <svtools/scrolladaptor.hxx>
#include <o3tl/numeric.hxx>
#include <o3tl/safeint.hxx>
#include "datwin.hxx"
#include <osl/diagnose.h>
#include <tools/debug.hxx>
#include <tools/fract.hxx>
#include <sal/log.hxx>
#include <vcl/InterimItemWindow.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>

#include <algorithm>
#include <com/sun/star/accessibility/AccessibleTableModelChange.hpp>
#include <com/sun/star/accessibility/AccessibleTableModelChangeType.hpp>
#include <com/sun/star/accessibility/AccessibleEventId.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <tools/multisel.hxx>


#define SCROLL_FLAGS (ScrollFlags::Clip | ScrollFlags::NoChildren)

using namespace com::sun::star::accessibility::AccessibleTableModelChangeType;
using com::sun::star::accessibility::AccessibleTableModelChange;
using namespace com::sun::star::accessibility;
using namespace ::com::sun::star::uno;
using namespace svt;

namespace
{
    struct THeaderCellMapFunctorDispose
    {
        void operator()(const BrowseBox::THeaderCellMap::value_type& _aType)
        {
            css::uno::Reference<css::lang::XComponent> xComp(_aType.second, css::uno::UNO_QUERY);
            OSL_ENSURE(xComp.is() || !_aType.second.is(), "THeaderCellMapFunctorDispose: invalid accessible cell (no XComponent)!");
            if (xComp.is())
                try
                {
                    xComp->dispose();
                }
                catch(const css::uno::Exception&)
                {
                    TOOLS_WARN_EXCEPTION("svtools", "THeaderCellMapFunctorDispose");
                }
        }
    };

    void disposeAndClearHeaderCell(BrowseBox::THeaderCellMap& _rHeaderCell)
    {
        ::std::for_each(
                        _rHeaderCell.begin(),
                        _rHeaderCell.end(),
                        THeaderCellMapFunctorDispose()
                            );
        _rHeaderCell.clear();
    }
}

// we're just measuring the "real" NavigationBar
class MeasureStatusBar final : public InterimItemWindow
{
private:
    std::unique_ptr<weld::Label> m_xRecordText;
    std::unique_ptr<weld::Entry> m_xAbsolute;
    std::unique_ptr<weld::Label> m_xRecordOf;
    std::unique_ptr<weld::Label> m_xRecordCount;
public:
    MeasureStatusBar(vcl::Window *pParent)
        : InterimItemWindow(pParent, u"svx/ui/navigationbar.ui"_ustr, u"NavigationBar"_ustr)
        , m_xRecordText(m_xBuilder->weld_label(u"recordtext"_ustr))
        , m_xAbsolute(m_xBuilder->weld_entry(u"entry-noframe"_ustr))
        , m_xRecordOf(m_xBuilder->weld_label(u"recordof"_ustr))
        , m_xRecordCount(m_xBuilder->weld_label(u"recordcount"_ustr))
    {
        vcl::Font aApplFont(Application::GetSettings().GetStyleSettings().GetToolFont());
        m_xAbsolute->set_font(aApplFont);
        m_xRecordText->set_font(aApplFont);
        m_xRecordOf->set_font(aApplFont);
        m_xRecordCount->set_font(aApplFont);

        SetSizePixel(get_preferred_size());
    }

    virtual void dispose() override
    {
        m_xRecordCount.reset();
        m_xRecordOf.reset();
        m_xAbsolute.reset();
        m_xRecordText.reset();
        InterimItemWindow::dispose();
    }
};

tools::Long BrowseBox::GetBarHeight() const
{
    tools::Long nScrollBarSize = GetSettings().GetStyleSettings().GetScrollBarSize();
    if (!m_bNavigationBar)
        return nScrollBarSize;

    // tdf#115941 because some platforms have things like overlay scrollbars, take a max
    // of a statusbar height and a scrollbar height as the control area height

    // (we can't ask the scrollbars for their size cause if we're zoomed they still have to be
    // resized - which is done in UpdateScrollbars)
    return std::max(aStatusBarHeight->GetSizePixel().Height(), nScrollBarSize);
}

BrowseBox::BrowseBox( vcl::Window* pParent, WinBits nBits, BrowserMode nMode )
    :Control( pParent, nBits | WB_3DLOOK )
    ,DragSourceHelper( this )
    ,DropTargetHelper( this )
    ,aHScroll( VclPtr<ScrollAdaptor>::Create(this, true) )
    // see NavigationBar ctor, here we just want to know its height
    ,aStatusBarHeight(VclPtr<MeasureStatusBar>::Create(this))
    ,m_nCornerHeight(0)
    ,m_nCornerWidth(0)
    ,m_nActualCornerWidth(0)
    ,m_bNavigationBar(false)
{
    bMultiSelection = false;
    pColSel = nullptr;
    pVScroll = nullptr;
    pDataWin = VclPtr<BrowserDataWin>::Create( this ).get();

    InitSettings_Impl( this );
    InitSettings_Impl( pDataWin );

    bBootstrapped = false;
    m_nDataRowHeight = 0;
    nTitleLines = 1;
    nFirstCol = 0;
    nTopRow = 0;
    nCurRow = BROWSER_ENDOFSELECTION;
    nCurColId = 0;
    nResizeX = 0;
    nMinResizeX = 0;
    nDragX = 0;
    nResizeCol = 0;
    bResizing = false;
    bSelect = false;
    bSelecting = false;
    bScrolling = false;
    bSelectionIsVisible = false;
    bNotToggleSel = false;
    bRowDividerDrag = false;
    bHit = false;
    mbInteractiveRowHeight = false;
    bHideSelect = false;
    bHideCursor = TRISTATE_FALSE;
    nRowCount = 0;
    m_bFocusOnlyCursor = true;
    m_aCursorColor = COL_TRANSPARENT;
    m_nCurrentMode = BrowserMode::NONE;
    nControlAreaWidth = USHRT_MAX;
    uRow.nSel = BROWSER_ENDOFSELECTION;

    aHScroll->SetLineSize(1);
    aHScroll->SetScrollHdl( LINK( this, BrowseBox, HorzScrollHdl ) );
    pDataWin->Show();

    SetMode( nMode );
    bSelectionIsVisible = bKeepHighlight;
    bHasFocus = HasChildPathFocus();
    pDataWin->nCursorHidden =
        ( bHasFocus ? 0 : 1 ) + ( GetUpdateMode() ? 0 : 1 );
}

BrowseBox::~BrowseBox()
{
    disposeOnce();
}

void BrowseBox::DisposeAccessible()
{
    if (m_xAccessible)
    {
        disposeAndClearHeaderCell(m_aColHeaderCellMap);
        disposeAndClearHeaderCell(m_aRowHeaderCellMap);
        m_xAccessible->dispose();
        m_xAccessible = nullptr;
    }
}

void BrowseBox::dispose()
{
    SAL_INFO("svtools", "BrowseBox:dispose " << this );

    DisposeAccessible();

    Hide();
    pDataWin->pHeaderBar.disposeAndClear();
    pDataWin.disposeAndClear();
    pVScroll.disposeAndClear();
    aHScroll.disposeAndClear();
    aStatusBarHeight.disposeAndClear();

    // free columns-space
    mvCols.clear();
    pColSel.reset();
    if ( bMultiSelection )
        delete uRow.pSel;
    DragSourceHelper::dispose();
    DropTargetHelper::dispose();
    Control::dispose();
}


short BrowseBox::GetCursorHideCount() const
{
    return pDataWin->nCursorHidden;
}


void BrowseBox::DoShowCursor()
{
    if (!pDataWin)
        return;
    short nHiddenCount = --pDataWin->nCursorHidden;
    if (PaintCursorIfHiddenOnce())
    {
        if (1 == nHiddenCount)
            DrawCursor();
    }
    else
    {
        if (0 == nHiddenCount)
            DrawCursor();
    }
}


void BrowseBox::DoHideCursor()
{
    short nHiddenCount = ++pDataWin->nCursorHidden;
    if (PaintCursorIfHiddenOnce())
    {
        if (2 == nHiddenCount)
            DrawCursor();
    }
    else
    {
        if (1 == nHiddenCount)
            DrawCursor();
    }
}


void BrowseBox::SetRealRowCount( const OUString &rRealRowCount )
{
    pDataWin->aRealRowCount = rRealRowCount;
}


void BrowseBox::SetFont( const vcl::Font& rNewFont )
{
    pDataWin->SetFont( rNewFont );
    ImpGetDataRowHeight();
}

const vcl::Font& BrowseBox::GetFont() const
{
    return pDataWin->GetFont();
}

tools::Long BrowseBox::GetDefaultColumnWidth( const OUString& _rText ) const
{
    return pDataWin->GetTextWidth( _rText ) + pDataWin->GetTextWidth(OUString('0')) * 4;
}


void BrowseBox::InsertHandleColumn( tools::Long nWidth )
{

#if OSL_DEBUG_LEVEL > 0
    OSL_ENSURE( ColCount() == 0 || mvCols[0]->GetId() != HandleColumnId , "BrowseBox::InsertHandleColumn: there is already a handle column" );
    {
        for (auto const & col : mvCols)
            OSL_ENSURE( col->GetId() != HandleColumnId, "BrowseBox::InsertHandleColumn: there is a non-Handle column with handle ID" );
    }
#endif

    mvCols.insert( mvCols.begin(), std::unique_ptr<BrowserColumn>(new BrowserColumn( 0, OUString(), nWidth, GetZoom() )) );
    FreezeColumn( 0 );

    // adjust headerbar
    if ( pDataWin->pHeaderBar )
    {
        pDataWin->pHeaderBar->SetPosSizePixel(
                    Point(nWidth, 0),
                    Size( GetOutputSizePixel().Width() - nWidth, GetTitleHeight() )
                    );
    }

    ColumnInserted( 0 );
}


void BrowseBox::InsertDataColumn( sal_uInt16 nItemId, const OUString& rText,
        tools::Long nWidth, HeaderBarItemBits nBits, sal_uInt16 nPos )
{

    OSL_ENSURE( nItemId != HandleColumnId, "BrowseBox::InsertDataColumn: nItemId is HandleColumnId" );
    OSL_ENSURE( nItemId != BROWSER_INVALIDID, "BrowseBox::InsertDataColumn: nItemId is reserved value BROWSER_INVALIDID" );

#if OSL_DEBUG_LEVEL > 0
    {
        for (auto const& col : mvCols)
            OSL_ENSURE( col->GetId() != nItemId, "BrowseBox::InsertDataColumn: duplicate column Id" );
    }
#endif

    if ( nPos < mvCols.size() )
    {
        mvCols.emplace( mvCols.begin() + nPos, new BrowserColumn( nItemId, rText, nWidth, GetZoom() ) );
    }
    else
    {
        mvCols.emplace_back( new BrowserColumn( nItemId, rText, nWidth, GetZoom() ) );
    }
    if ( nCurColId == 0 )
        nCurColId = nItemId;

    if ( pDataWin->pHeaderBar )
    {
        // Handle column not in the header bar
        sal_uInt16 nHeaderPos = nPos;
        if (nHeaderPos != HEADERBAR_APPEND && GetColumnId(0) == HandleColumnId )
            nHeaderPos--;
        pDataWin->pHeaderBar->InsertItem(
                nItemId, rText, nWidth, nBits, nHeaderPos );
    }
    ColumnInserted( nPos );
}

sal_uInt16 BrowseBox::ToggleSelectedColumn()
{
    sal_uInt16 nSelectedColId = BROWSER_INVALIDID;
    if ( pColSel && pColSel->GetSelectCount() )
    {
        DoHideCursor();
        ToggleSelection();
        tools::Long nSelected = pColSel->FirstSelected();
        if (nSelected != static_cast<tools::Long>(SFX_ENDOFSELECTION))
            nSelectedColId = mvCols[nSelected]->GetId();
        pColSel->SelectAll(false);
    }
    return nSelectedColId;
}

void BrowseBox::SetToggledSelectedColumn(sal_uInt16 _nSelectedColumnId)
{
    if ( pColSel && _nSelectedColumnId != BROWSER_INVALIDID )
    {
        pColSel->Select( GetColumnPos( _nSelectedColumnId ) );
        ToggleSelection();
        SAL_INFO("svtools", "BrowseBox::SetToggledSelectedColumn " << this );
        DoShowCursor();
    }
}

void BrowseBox::FreezeColumn( sal_uInt16 nItemId )
{
    // get the position in the current array
    size_t nItemPos = GetColumnPos( nItemId );
    if ( nItemPos >= mvCols.size() )
        // not available!
        return;

    // doesn't the state change?
    if ( mvCols[ nItemPos ]->IsFrozen() )
        return;

    // remark the column selection
    sal_uInt16 nSelectedColId = ToggleSelectedColumn();

    // to be moved?
    if ( nItemPos != 0 && !mvCols[ nItemPos-1 ]->IsFrozen() )
    {
        // move to the right of the last frozen column
        sal_uInt16 nFirstScrollable = FrozenColCount();
        std::unique_ptr<BrowserColumn> pColumn = std::move(mvCols[ nItemPos ]);
        mvCols.erase( mvCols.begin() + nItemPos );
        nItemPos = nFirstScrollable;
        mvCols.insert( mvCols.begin() + nItemPos, std::move(pColumn) );
    }

    // adjust the number of the first scrollable and visible column
    if ( nFirstCol <= nItemPos )
        nFirstCol = nItemPos + 1;

    // toggle the freeze-state of the column
    mvCols[ nItemPos ]->Freeze();

    // align the scrollbar-range
    UpdateScrollbars();

    // repaint
    Control::Invalidate();
    pDataWin->Invalidate();

    // remember the column selection
    SetToggledSelectedColumn(nSelectedColId);
}


void BrowseBox::SetColumnPos( sal_uInt16 nColumnId, sal_uInt16 nPos )
{
    // never set pos of the handle column
    if ( nColumnId == HandleColumnId )
        return;

    // get the position in the current array
    sal_uInt16 nOldPos = GetColumnPos( nColumnId );
    if ( nOldPos >= mvCols.size() )
        // not available!
        return;

    // does the state change?
    if (nOldPos == nPos)
        return;

    // remark the column selection
    sal_uInt16 nSelectedColId = ToggleSelectedColumn();

    // determine old column area
    Size aDataWinSize( pDataWin->GetSizePixel() );
    if ( pDataWin->pHeaderBar )
        aDataWinSize.AdjustHeight(pDataWin->pHeaderBar->GetSizePixel().Height() );

    tools::Rectangle aFromRect( GetFieldRect( nColumnId) );
    aFromRect.AdjustRight(2*MIN_COLUMNWIDTH );

    sal_uInt16 nNextPos = nOldPos + 1;
    if ( nOldPos > nPos )
        nNextPos = nOldPos - 1;

    BrowserColumn *pNextCol = mvCols[ nNextPos ].get();
    tools::Rectangle aNextRect(GetFieldRect( pNextCol->GetId() ));

    // move column internally
    {
        std::unique_ptr<BrowserColumn> pTemp = std::move(mvCols[nOldPos]);
        mvCols.erase( mvCols.begin() + nOldPos );
        mvCols.insert( mvCols.begin() + nPos, std::move(pTemp) );
    }

    // determine new column area
    tools::Rectangle aToRect( GetFieldRect( nColumnId ) );
    aToRect.AdjustRight(2*MIN_COLUMNWIDTH );

    // do scroll, let redraw
    if( pDataWin->GetBackground().IsScrollable() )
    {
        tools::Long nScroll = -aFromRect.GetWidth();
        tools::Rectangle aScrollArea;
        if ( nOldPos > nPos )
        {
            tools::Long nFrozenWidth = GetFrozenWidth();
            if ( aToRect.Left() < nFrozenWidth )
                aToRect.SetLeft( nFrozenWidth );
            aScrollArea = tools::Rectangle(Point(aToRect.Left(),0),
                                    Point(aNextRect.Right(),aDataWinSize.Height()));
            nScroll *= -1; // reverse direction
        }
        else
            aScrollArea = tools::Rectangle(Point(aNextRect.Left(),0),
                                    Point(aToRect.Right(),aDataWinSize.Height()));

        pDataWin->Scroll( nScroll, 0, aScrollArea );
        aToRect.SetTop( 0 );
        aToRect.SetBottom( aScrollArea.Bottom() );
        Invalidate( aToRect );
    }
    else
        pDataWin->Window::Invalidate( InvalidateFlags::NoChildren );

    // adjust header bar positions
    if ( pDataWin->pHeaderBar )
    {
        sal_uInt16 nNewPos = nPos;
        if ( GetColumnId(0) == HandleColumnId )
            --nNewPos;
        pDataWin->pHeaderBar->MoveItem(nColumnId,nNewPos);
    }
    // remember the column selection
    SetToggledSelectedColumn(nSelectedColId);

    if ( !isAccessibleAlive() )
        return;

    commitTableEvent(
        AccessibleEventId::TABLE_MODEL_CHANGED,
        Any( AccessibleTableModelChange(
                    COLUMNS_REMOVED,
                    -1,
                    -1,
                    nOldPos,
                    nOldPos
                )
        ),
        Any()
    );

    commitTableEvent(
        AccessibleEventId::TABLE_MODEL_CHANGED,
        Any( AccessibleTableModelChange(
                    COLUMNS_INSERTED,
                    -1,
                    -1,
                    nPos,
                    nPos
                )
        ),
        Any()
    );

}


void BrowseBox::SetColumnTitle( sal_uInt16 nItemId, const OUString& rTitle )
{

    // never set title of the handle-column
    if ( nItemId == HandleColumnId )
        return;

    // get the position in the current array
    sal_uInt16 nItemPos = GetColumnPos( nItemId );
    if ( nItemPos >= mvCols.size() )
        // not available!
        return;

    // does the state change?
    BrowserColumn *pCol = mvCols[ nItemPos ].get();
    if ( pCol->Title() == rTitle )
        return;

    OUString sOld(pCol->Title());

    pCol->Title() = rTitle;

    // adjust headerbar column
    if ( pDataWin->pHeaderBar )
        pDataWin->pHeaderBar->SetItemText( nItemId, rTitle );
    else
    {
        // redraw visible columns
        if ( GetUpdateMode() && ( pCol->IsFrozen() || nItemPos > nFirstCol ) )
            Invalidate( tools::Rectangle( Point(0,0),
                Size( GetOutputSizePixel().Width(), GetTitleHeight() ) ) );
    }

    if ( isAccessibleAlive() )
    {
        commitTableEvent(AccessibleEventId::TABLE_COLUMN_DESCRIPTION_CHANGED,
            Any( rTitle ),
            Any( sOld )
        );
    }
}


void BrowseBox::SetColumnWidth( sal_uInt16 nItemId, tools::Long nWidth )
{

    // get the position in the current array
    size_t nItemPos = GetColumnPos( nItemId );
    if ( nItemPos >= mvCols.size() )
        return;

    // does the state change?
    if ( nWidth < LONG_MAX && mvCols[ nItemPos ]->Width() == nWidth )
        return;

    tools::Long nOldWidth = mvCols[ nItemPos ]->Width();

    // adjust last column, if necessary
    if ( IsVisible() && nItemPos == mvCols.size() - 1 )
    {
        tools::Long nMaxWidth = pDataWin->GetSizePixel().Width();
        nMaxWidth -= pDataWin->bAutoSizeLastCol
                ? GetFieldRect(nItemId).Left()
                : GetFrozenWidth();
        if ( pDataWin->bAutoSizeLastCol || nWidth > nMaxWidth )
        {
            nWidth = nMaxWidth > 16 ? nMaxWidth : nOldWidth;
        }
    }

    // OV
    // In AutoSizeLastColumn(), we call SetColumnWidth with nWidth==0xffff.
    // Thus, check here, if the width has actually changed.
    if( nOldWidth == nWidth )
        return;

    // do we want to display the change immediately?
    bool bUpdate = GetUpdateMode() &&
                   ( mvCols[ nItemPos ]->IsFrozen() || nItemPos >= nFirstCol );

    if ( bUpdate )
    {
        // Selection hidden
        DoHideCursor();
        ToggleSelection();
        //!pDataWin->Update();
        //!Control::Update();
    }

    // set width
    mvCols[ nItemPos ]->SetWidth(nWidth, GetZoom());

    // scroll and invalidate
    if ( bUpdate )
    {
        // get X-Pos of the column changed
        tools::Long nX = 0;
        for ( size_t nCol = 0; nCol < nItemPos; ++nCol )
        {
            BrowserColumn *pCol = mvCols[ nCol ].get();
            if ( pCol->IsFrozen() || nCol >= nFirstCol )
                nX += pCol->Width();
        }

        // actually scroll+invalidate
        pDataWin->GetOutDev()->SetClipRegion();
        bool bSelVis = bSelectionIsVisible;
        bSelectionIsVisible = false;
        if( GetBackground().IsScrollable() )
        {

            tools::Rectangle aScrRect( nX + std::min( nOldWidth, nWidth ), 0,
                                GetSizePixel().Width() , // the header is longer than the datawin
                                pDataWin->GetPosPixel().Y() - 1 );
            Control::Scroll( nWidth-nOldWidth, 0, aScrRect, SCROLL_FLAGS );
            aScrRect.SetBottom( pDataWin->GetSizePixel().Height() );
            pDataWin->Scroll( nWidth-nOldWidth, 0, aScrRect, SCROLL_FLAGS );
            tools::Rectangle aInvRect( nX, 0, nX + std::max( nWidth, nOldWidth ), USHRT_MAX );
            Control::Invalidate( aInvRect, InvalidateFlags::NoChildren );
            pDataWin->Invalidate( aInvRect );
        }
        else
        {
            Control::Invalidate( InvalidateFlags::NoChildren );
            pDataWin->Window::Invalidate( InvalidateFlags::NoChildren );
        }


        //!pDataWin->Update();
        //!Control::Update();
        bSelectionIsVisible = bSelVis;
        ToggleSelection();
        DoShowCursor();
    }
    UpdateScrollbars();

    // adjust headerbar column
    if ( pDataWin->pHeaderBar )
        pDataWin->pHeaderBar->SetItemSize(
                nItemId ? nItemId : USHRT_MAX - 1, nWidth );

    // adjust last column
    if ( nItemPos != mvCols.size() - 1 )
        AutoSizeLastColumn();
}


void BrowseBox::AutoSizeLastColumn()
{
    if ( pDataWin->bAutoSizeLastCol &&
         pDataWin->GetUpdateMode() )
    {
        sal_uInt16 nId = GetColumnId( static_cast<sal_uInt16>(mvCols.size()) - 1 );
        SetColumnWidth( nId, LONG_MAX );
        ColumnResized( nId );
    }
}


void BrowseBox::RemoveColumn( sal_uInt16 nItemId )
{

    // get column position
    sal_uInt16 nPos = GetColumnPos(nItemId);
    if ( nPos >= ColCount() )
        // not available
        return;

    // correct column selection
    if ( pColSel )
        pColSel->Remove( nPos );

    // correct column cursor
    if ( nCurColId == nItemId )
        nCurColId = 0;

    // delete column
    mvCols.erase( mvCols.begin() + nPos );
    if ( nFirstCol >= nPos && nFirstCol > FrozenColCount() )
    {
        OSL_ENSURE(nFirstCol > 0,"FirstCol must be greater zero!");
        --nFirstCol;
    }

    // handlecolumn not in headerbar
    if (nItemId)
    {
        if ( pDataWin->pHeaderBar )
            pDataWin->pHeaderBar->RemoveItem( nItemId );
    }
    else
    {
        // adjust headerbar
        if ( pDataWin->pHeaderBar )
        {
            pDataWin->pHeaderBar->SetPosSizePixel(
                        Point(0, 0),
                        Size( GetOutputSizePixel().Width(), GetTitleHeight() )
                        );
        }
    }

    // correct vertical scrollbar
    UpdateScrollbars();

    // trigger repaint, if necessary
    if ( GetUpdateMode() )
    {
        pDataWin->Invalidate();
        Control::Invalidate();
        if ( pDataWin->bAutoSizeLastCol && nPos ==ColCount() )
            SetColumnWidth( GetColumnId( nPos - 1 ), LONG_MAX );
    }

    if ( !isAccessibleAlive() )
        return;

    commitTableEvent(
        AccessibleEventId::TABLE_MODEL_CHANGED,
        Any( AccessibleTableModelChange(COLUMNS_REMOVED,
                                                -1,
                                                -1,
                                                nPos,
                                                nPos
                                           )
        ),
        Any()
    );

    commitHeaderBarEvent(
        AccessibleEventId::CHILD,
        Any(),
        Any( CreateAccessibleColumnHeader( nPos ) ),
        true
    );
}


void BrowseBox::RemoveColumns()
{
    size_t nOldCount = mvCols.size();

    // remove all columns
    mvCols.clear();

    // correct column selection
    if ( pColSel )
    {
        pColSel->SelectAll(false);
        pColSel->SetTotalRange( Range( 0, 0 ) );
    }

    // correct column cursor
    nCurColId = 0;
    nFirstCol = 0;

    if ( pDataWin->pHeaderBar )
        pDataWin->pHeaderBar->Clear( );

    // correct vertical scrollbar
    UpdateScrollbars();

    // trigger repaint if necessary
    if ( GetUpdateMode() )
    {
        pDataWin->Invalidate();
        Control::Invalidate();
    }

    if ( !isAccessibleAlive() )
        return;

    if ( mvCols.size() == nOldCount )
        return;

    // all columns should be removed, so we remove the column header bar and append it again
    // to avoid to notify every column remove
    commitBrowseBoxEvent(
        AccessibleEventId::CHILD,
        Any(),
        Any(getAccessibleHeaderBar(AccessibleBrowseBoxObjType::ColumnHeaderBar))
    );

    // and now append it again
    commitBrowseBoxEvent(
        AccessibleEventId::CHILD,
        Any(getAccessibleHeaderBar(AccessibleBrowseBoxObjType::ColumnHeaderBar)),
        Any()
    );

    // notify a table model change
    commitTableEvent(
        AccessibleEventId::TABLE_MODEL_CHANGED,
        Any ( AccessibleTableModelChange( COLUMNS_REMOVED,
                        -1,
                        -1,
                        0,
                        nOldCount
                    )
                ),
        Any()
    );
}


const OUString & BrowseBox::GetColumnTitle( sal_uInt16 nId ) const
{

    sal_uInt16 nItemPos = GetColumnPos( nId );
    if ( nItemPos >= mvCols.size() )
        return EMPTY_OUSTRING;
    return mvCols[ nItemPos ]->Title();
}

sal_Int32 BrowseBox::GetRowCount() const
{
    return nRowCount;
}

sal_uInt16 BrowseBox::ColCount() const
{
    return static_cast<sal_uInt16>(mvCols.size());
}

tools::Long BrowseBox::ImpGetDataRowHeight() const
{
    BrowseBox *pThis = const_cast<BrowseBox*>(this);
    pThis->m_nDataRowHeight = pThis->CalcReverseZoom(pDataWin->GetTextHeight() + 4);
    pThis->Resize();
    pDataWin->Invalidate();
    return m_nDataRowHeight;
}

void BrowseBox::SetDataRowHeight( tools::Long nPixel )
{

    m_nDataRowHeight = CalcReverseZoom(nPixel);
    Resize();
    pDataWin->Invalidate();
}

void BrowseBox::SetTitleLines( sal_uInt16 nLines )
{

    nTitleLines = nLines;
}

sal_Int32 BrowseBox::ScrollColumns( sal_Int32 nCols )
{

    if ( nFirstCol + nCols < 0 ||
         o3tl::make_unsigned(nFirstCol + nCols) >= mvCols.size() )
        return 0;

    // implicitly hides cursor while scrolling
    StartScroll();
    bScrolling = true;
    bool bScrollable = pDataWin->GetBackground().IsScrollable();
    bool bInvalidateView = false;

    // scrolling one column to the right?
    if ( nCols == 1 )
    {
        // update internal value and scrollbar
        ++nFirstCol;
        aHScroll->SetThumbPos( nFirstCol - FrozenColCount() );

        if ( !bScrollable )
        {
            bInvalidateView = true;
        }
        else
        {
            tools::Long nDelta = mvCols[ nFirstCol-1 ]->Width();
            tools::Long nFrozenWidth = GetFrozenWidth();

            tools::Rectangle aScrollRect(  Point( nFrozenWidth + nDelta, 0 ),
                                    Size ( GetOutputSizePixel().Width() - nFrozenWidth - nDelta,
                                           GetTitleHeight() - 1
                                         ) );

            // scroll the header bar area (if there is no dedicated HeaderBar control)
            if ( !pDataWin->pHeaderBar && nTitleLines )
            {
                // actually scroll
                Scroll( -nDelta, 0, aScrollRect, SCROLL_FLAGS );

                // invalidate the area of the column which was scrolled out to the left hand side
                tools::Rectangle aInvalidateRect( aScrollRect );
                aInvalidateRect.SetLeft( nFrozenWidth );
                aInvalidateRect.SetRight( nFrozenWidth + nDelta - 1 );
                Invalidate( aInvalidateRect );
            }

            // scroll the data-area
            aScrollRect.SetBottom( pDataWin->GetOutputSizePixel().Height() );

            // actually scroll
            pDataWin->Scroll( -nDelta, 0, aScrollRect, SCROLL_FLAGS );

            // invalidate the area of the column which was scrolled out to the left hand side
            aScrollRect.SetLeft( nFrozenWidth );
            aScrollRect.SetRight( nFrozenWidth + nDelta - 1 );
            pDataWin->Invalidate( aScrollRect );
        }
    }

    // scrolling one column to the left?
    else if ( nCols == -1 )
    {
        --nFirstCol;
        aHScroll->SetThumbPos( nFirstCol - FrozenColCount() );

        if ( !bScrollable )
        {
            bInvalidateView = true;
        }
        else
        {
            tools::Long nDelta = mvCols[ nFirstCol ]->Width();
            tools::Long nFrozenWidth = GetFrozenWidth();

            tools::Rectangle aScrollRect(  Point(  nFrozenWidth, 0 ),
                                    Size (  GetOutputSizePixel().Width() - nFrozenWidth,
                                            GetTitleHeight() - 1
                                         ) );

            // scroll the header bar area (if there is no dedicated HeaderBar control)
            if ( !pDataWin->pHeaderBar && nTitleLines )
            {
                Scroll( nDelta, 0, aScrollRect, SCROLL_FLAGS );
            }

            // scroll the data-area
            aScrollRect.SetBottom( pDataWin->GetOutputSizePixel().Height() );
            pDataWin->Scroll( nDelta, 0, aScrollRect, SCROLL_FLAGS );
        }
    }
    else
    {
        if ( GetUpdateMode() )
        {
            Invalidate( tools::Rectangle(
                Point( GetFrozenWidth(), 0 ),
                Size( GetOutputSizePixel().Width(), GetTitleHeight() ) ) );
            pDataWin->Invalidate( tools::Rectangle(
                Point( GetFrozenWidth(), 0 ),
                pDataWin->GetSizePixel() ) );
        }

        nFirstCol = nFirstCol + static_cast<sal_uInt16>(nCols);
        aHScroll->SetThumbPos( nFirstCol - FrozenColCount() );
    }

    // adjust external headerbar, if necessary
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

    if( bInvalidateView )
    {
        Control::Invalidate( InvalidateFlags::NoChildren );
        pDataWin->Window::Invalidate( InvalidateFlags::NoChildren );
    }

    // implicitly show cursor after scrolling
    if ( nCols )
    {
        pDataWin->Update();
        PaintImmediately();
    }
    bScrolling = false;
    EndScroll();

    return nCols;
}


sal_Int32 BrowseBox::ScrollRows( sal_Int32 nRows )
{
    // compute new top row
    sal_Int32 nTmpMin = std::min( static_cast<sal_Int32>(nTopRow + nRows), static_cast<sal_Int32>(nRowCount - 1) );

    sal_Int32 nNewTopRow = std::max<sal_Int32>( nTmpMin, 0 );

    if ( nNewTopRow == nTopRow )
        return 0;

    sal_uInt16 nVisibleRows =
        static_cast<sal_uInt16>(pDataWin->GetOutputSizePixel().Height() / GetDataRowHeight() + 1);

    VisibleRowsChanged(nNewTopRow, nVisibleRows);

    // compute new top row again (nTopRow might have changed!)
    nTmpMin = std::min( static_cast<tools::Long>(nTopRow + nRows), static_cast<tools::Long>(nRowCount - 1) );

    nNewTopRow = std::max<tools::Long>( nTmpMin, 0 );

    StartScroll();

    // scroll area on screen and/or repaint
    tools::Long nDeltaY = GetDataRowHeight() * ( nNewTopRow - nTopRow );
    sal_Int32 nOldTopRow = nTopRow;
    nTopRow = nNewTopRow;

    if ( GetUpdateMode() )
    {
        pVScroll->SetRange( Range( 0L, nRowCount ) );
        pVScroll->SetThumbPos( nTopRow );

        if( pDataWin->GetBackground().IsScrollable() &&
            std::abs( nDeltaY ) > 0 &&
            std::abs( nDeltaY ) < pDataWin->GetSizePixel().Height() )
        {
            pDataWin->Scroll( 0, static_cast<short>(-nDeltaY), SCROLL_FLAGS );
        }
        else
            pDataWin->Invalidate();

        if ( nTopRow - nOldTopRow )
            pDataWin->Update();
    }

    EndScroll();

    return nTopRow - nOldTopRow;
}


void BrowseBox::RowModified( sal_Int32 nRow, sal_uInt16 nColId )
{

    if ( !GetUpdateMode() )
        return;

    tools::Rectangle aRect;
    if ( nColId == BROWSER_INVALIDID )
        // invalidate the whole row
        aRect = tools::Rectangle( Point( 0, (nRow-nTopRow) * GetDataRowHeight() ),
                    Size( pDataWin->GetSizePixel().Width(), GetDataRowHeight() ) );
    else
    {
        // invalidate the specific field
        aRect = GetFieldRectPixel( nRow, nColId, false );
    }
    pDataWin->Invalidate( aRect );
}


void BrowseBox::Clear()
{
    // adjust the total number of rows
    DoHideCursor();
    sal_Int32 nOldRowCount = nRowCount;
    nRowCount = 0;
    if(bMultiSelection)
    {
        assert(uRow.pSel);
        uRow.pSel->Reset();
    }
    else
        uRow.nSel = BROWSER_ENDOFSELECTION;
    nCurRow = BROWSER_ENDOFSELECTION;
    nTopRow = 0;
    nCurColId = 0;

    // nFirstCol may not be reset, else the scrolling code will become confused.
    // nFirstCol may only be changed when adding or deleting columns
    // nFirstCol = 0; -> wrong!
    aHScroll->SetThumbPos( 0 );
    pVScroll->SetThumbPos( 0 );

    Invalidate();
    UpdateScrollbars();
    SetNoSelection();
    DoShowCursor();
    CursorMoved();

    if ( !isAccessibleAlive() )
        return;

    // all rows should be removed, so we remove the row header bar and append it again
    // to avoid to notify every row remove
    if ( nOldRowCount == nRowCount )
        return;

    commitBrowseBoxEvent(
        AccessibleEventId::CHILD,
        Any(),
        Any(getAccessibleHeaderBar( AccessibleBrowseBoxObjType::RowHeaderBar))
    );

    // and now append it again
    commitBrowseBoxEvent(
        AccessibleEventId::CHILD,
        Any(getAccessibleHeaderBar(AccessibleBrowseBoxObjType::RowHeaderBar)),
        Any()
    );

    // notify a table model change
    commitTableEvent(
        AccessibleEventId::TABLE_MODEL_CHANGED,
        Any( AccessibleTableModelChange(ROWS_REMOVED,
            0,
            nOldRowCount,
            -1,
            -1)
        ),
        Any()
    );
}

void BrowseBox::RowInserted( sal_Int32 nRow, sal_Int32 nNumRows, bool bDoPaint, bool bKeepSelection )
{

    if (nRow < 0)
        nRow = 0;
    else if (nRow > nRowCount) // maximal = nRowCount
        nRow = nRowCount;

    if ( nNumRows <= 0 )
        return;

    // adjust total row count
    bool bLastRow = nRow >= nRowCount;
    nRowCount += nNumRows;

    DoHideCursor();

    // must we paint the new rows?
    sal_Int32 nOldCurRow = nCurRow;
    Size aSz = pDataWin->GetOutputSizePixel();
    if ( bDoPaint && nRow >= nTopRow &&
         nRow <= nTopRow + aSz.Height() / GetDataRowHeight() )
    {
        tools::Long nY = (nRow-nTopRow) * GetDataRowHeight();
        if ( !bLastRow )
        {
            // scroll down the rows behind the new row
            pDataWin->GetOutDev()->SetClipRegion();
            if( pDataWin->GetBackground().IsScrollable() )
            {
                pDataWin->Scroll( 0, GetDataRowHeight() * nNumRows,
                                tools::Rectangle( Point( 0, nY ),
                                        Size( aSz.Width(), aSz.Height() - nY ) ),
                                SCROLL_FLAGS );
            }
            else
                pDataWin->Window::Invalidate( InvalidateFlags::NoChildren );
        }
        else
            // scroll would cause a repaint, so we must explicitly invalidate
            pDataWin->Invalidate( tools::Rectangle( Point( 0, nY ),
                         Size( aSz.Width(), nNumRows * GetDataRowHeight() ) ) );
    }

    // correct top row if necessary
    if ( nRow < nTopRow )
        nTopRow += nNumRows;

    // adjust the selection
    if ( bMultiSelection )
        uRow.pSel->Insert( nRow, nNumRows );
    else if ( uRow.nSel != BROWSER_ENDOFSELECTION && nRow <= uRow.nSel )
        uRow.nSel += nNumRows;

    // adjust the cursor
    if ( nCurRow == BROWSER_ENDOFSELECTION )
        GoToRow( 0, false, bKeepSelection );
    else if ( nRow <= nCurRow )
    {
        nCurRow += nNumRows;
        GoToRow( nCurRow, false, bKeepSelection );
    }

    // adjust the vertical scrollbar
    if ( bDoPaint )
    {
        UpdateScrollbars();
        AutoSizeLastColumn();
    }

    DoShowCursor();
    // notify accessible that rows were inserted
    if ( isAccessibleAlive() )
    {
        commitTableEvent(
            AccessibleEventId::TABLE_MODEL_CHANGED,
            Any( AccessibleTableModelChange(
                        ROWS_INSERTED,
                        nRow,
                        nRow + nNumRows,
                        -1,
                        -1
                    )
            ),
            Any()
        );

        for (tools::Long i = nRow+1 ; i <= nRowCount ; ++i)
        {
            commitHeaderBarEvent(
                AccessibleEventId::CHILD,
                Any( CreateAccessibleRowHeader( i ) ),
                Any(),
                false
            );
        }
    }

    if ( nCurRow != nOldCurRow )
        CursorMoved();

    DBG_ASSERT(nRowCount > 0,"BrowseBox: nRowCount <= 0");
    DBG_ASSERT(nCurRow >= 0,"BrowseBox: nCurRow < 0");
    DBG_ASSERT(nCurRow < nRowCount,"nCurRow >= nRowCount");
}


void BrowseBox::RowRemoved( sal_Int32 nRow, sal_Int32 nNumRows, bool bDoPaint )
{

    if ( nRow < 0 )
        nRow = 0;
    else if ( nRow >= nRowCount )
        nRow = nRowCount - 1;

    if ( nNumRows <= 0 )
        return;

    if ( nRowCount <= 0 )
        return;

    if ( bDoPaint )
    {
        // hide cursor and selection
        SAL_INFO("svtools", "BrowseBox::HideCursor " << this );
        ToggleSelection();
        DoHideCursor();
    }

    // adjust total row count
    nRowCount -= nNumRows;
    if (nRowCount < 0) nRowCount = 0;
    sal_Int32 nOldCurRow = nCurRow;

    // adjust the selection
    if ( bMultiSelection )
        // uRow.pSel->Remove( nRow, nNumRows );
        for ( tools::Long i = 0; i < nNumRows; i++ )
            uRow.pSel->Remove( nRow );
    else if ( nRow < uRow.nSel && uRow.nSel >= nNumRows )
        uRow.nSel -= nNumRows;
    else if ( nRow <= uRow.nSel )
        uRow.nSel = BROWSER_ENDOFSELECTION;

    // adjust the cursor
    if ( nRowCount == 0 )   // don't compare nRowCount with nNumRows as nNumRows already was subtracted from nRowCount
        nCurRow = BROWSER_ENDOFSELECTION;
    else if ( nRow < nCurRow )
    {
        nCurRow -= std::min( nCurRow - nRow, nNumRows );
        // with the above nCurRow points a) to the first row after the removed block or b) to the same line
        // as before, but moved up nNumRows
        // case a) needs an additional correction if the last n lines were deleted, as 'the first row after the
        // removed block' is an invalid position then
        // FS - 09/28/99 - 68429
        if (nCurRow == nRowCount)
            --nCurRow;
    }
    else if( nRow == nCurRow && nCurRow == nRowCount )
        nCurRow = nRowCount-1;

    // is the deleted row visible?
    Size aSz = pDataWin->GetOutputSizePixel();
    if ( nRow >= nTopRow &&
         nRow <= nTopRow + aSz.Height() / GetDataRowHeight() )
    {
        if ( bDoPaint )
        {
            // scroll up the rows behind the deleted row
            // if there are Rows behind
            if (nRow < nRowCount)
            {
                tools::Long nY = (nRow-nTopRow) * GetDataRowHeight();
                pDataWin->GetOutDev()->SetClipRegion();
                if( pDataWin->GetBackground().IsScrollable() )
                {
                    pDataWin->Scroll( 0, - static_cast<short>(GetDataRowHeight()) * nNumRows,
                        tools::Rectangle( Point( 0, nY ), Size( aSz.Width(),
                            aSz.Height() - nY + nNumRows*GetDataRowHeight() ) ),
                            SCROLL_FLAGS );
                }
                else
                    pDataWin->Window::Invalidate( InvalidateFlags::NoChildren );
            }
            else
            {
                // Repaint the Rect of the deleted row
                tools::Rectangle aRect(
                        Point( 0, (nRow-nTopRow)*GetDataRowHeight() ),
                        Size( pDataWin->GetSizePixel().Width(),
                              nNumRows * GetDataRowHeight() ) );
                pDataWin->Invalidate( aRect );
            }
        }
    }
    // is the deleted row above of the visible area?
    else if ( nRow < nTopRow )
        nTopRow = nTopRow >= nNumRows ? nTopRow-nNumRows : 0;

    if ( bDoPaint )
    {
        // reshow cursor and selection
        ToggleSelection();
        SAL_INFO("svtools", "BrowseBox::ShowCursor " << this );
        DoShowCursor();

        // adjust the vertical scrollbar
        UpdateScrollbars();
        AutoSizeLastColumn();
    }

    if ( isAccessibleAlive() )
    {
        if ( nRowCount == 0 )
        {
            // all columns should be removed, so we remove the column header bar and append it again
            // to avoid to notify every column remove
            commitBrowseBoxEvent(
                AccessibleEventId::CHILD,
                Any(),
                Any(getAccessibleHeaderBar(AccessibleBrowseBoxObjType::RowHeaderBar))
            );

            // and now append it again
            commitBrowseBoxEvent(
                AccessibleEventId::CHILD,
                Any(getAccessibleHeaderBar(AccessibleBrowseBoxObjType::RowHeaderBar)),
                Any()
            );
            commitBrowseBoxEvent(
                AccessibleEventId::CHILD,
                Any(),
                Any(getAccessibleTable())
            );

            // and now append it again
            commitBrowseBoxEvent(
                AccessibleEventId::CHILD,
                Any(getAccessibleTable()),
                Any()
            );
        }
        else
        {
            commitTableEvent(
                AccessibleEventId::TABLE_MODEL_CHANGED,
                Any( AccessibleTableModelChange(
                            ROWS_REMOVED,
                            nRow,
                            nRow + nNumRows,
                            -1,
                            -1
                            )
                ),
                Any()
            );

            for (tools::Long i = nRow+1 ; i <= (nRow+nNumRows) ; ++i)
            {
                commitHeaderBarEvent(
                    AccessibleEventId::CHILD,
                    Any(),
                    Any( CreateAccessibleRowHeader( i ) ),
                    false
                );
            }
        }
    }

    if ( nOldCurRow != nCurRow )
        CursorMoved();

    DBG_ASSERT(nRowCount >= 0,"BrowseBox: nRowCount < 0");
    DBG_ASSERT(nCurRow >= 0 || nRowCount == 0,"BrowseBox: nCurRow < 0 && nRowCount != 0");
    DBG_ASSERT(nCurRow < nRowCount,"nCurRow >= nRowCount");
}


bool BrowseBox::GoToRow( sal_Int32 nRow)
{
    return GoToRow(nRow, false);
}


bool BrowseBox::GoToRow( sal_Int32 nRow, bool bRowColMove, bool bKeepSelection )
{
    sal_Int32 nOldCurRow = nCurRow;

    // nothing to do?
    if ( nRow == nCurRow && ( bMultiSelection || uRow.nSel == nRow ) )
        return true;

    // out of range?
    if ( nRow < 0 || nRow >= nRowCount )
        return false;

    // not allowed?
    if ( !bRowColMove && !IsCursorMoveAllowed( nRow, nCurColId ) )
        return false;

    // compute the last visible row
    Size aSz( pDataWin->GetSizePixel() );
    sal_uInt16 nVisibleRows = sal_uInt16( aSz.Height() / GetDataRowHeight() - 1 );
    sal_Int32 nLastRow = nTopRow + nVisibleRows;

    // suspend Updates
    pDataWin->EnterUpdateLock();

    // remove old highlight, if necessary
    if ( !bMultiSelection && !bKeepSelection )
        ToggleSelection();
    DoHideCursor();

    // must we scroll?
    bool bWasVisible = bSelectionIsVisible;
    if (! bMultiSelection)
    {
        if( !bKeepSelection )
            bSelectionIsVisible = false;
    }
    if ( nRow < nTopRow )
        ScrollRows( nRow - nTopRow );
    else if ( nRow > nLastRow )
        ScrollRows( nRow - nLastRow );
    bSelectionIsVisible = bWasVisible;

    // adjust cursor (selection) and thumb
    if ( GetUpdateMode() )
        pVScroll->SetThumbPos( nTopRow );

    // relative positioning (because nCurRow might have changed in the meantime)!
    if (nCurRow != BROWSER_ENDOFSELECTION )
        nCurRow = nCurRow + (nRow - nOldCurRow);

    // make sure that the current position is valid
    if (nCurRow == BROWSER_ENDOFSELECTION && nRowCount > 0)
        nCurRow = 0;
    else if ( nCurRow >= nRowCount )
        nCurRow = nRowCount - 1;
    aSelRange = Range( nCurRow, nCurRow );

    // display new highlight if necessary
    if ( !bMultiSelection && !bKeepSelection )
        uRow.nSel = nRow;

    // resume Updates
    pDataWin->LeaveUpdateLock();

    // Cursor+Highlight
    if ( !bMultiSelection && !bKeepSelection)
        ToggleSelection();
    DoShowCursor();
    if ( !bRowColMove  && nOldCurRow != nCurRow )
        CursorMoved();

    if ( !bMultiSelection && !bKeepSelection )
    {
        if ( !bSelecting )
            Select();
        else
            bSelect = true;
    }
    return true;
}


bool BrowseBox::GoToColumnId( sal_uInt16 nColId)
{
    return GoToColumnId(nColId, true);
}


bool BrowseBox::GoToColumnId( sal_uInt16 nColId, bool bMakeVisible, bool bRowColMove)
{
    if (!bColumnCursor)
        return false;

    // allowed?
    if (!bRowColMove && !IsCursorMoveAllowed( nCurRow, nColId ) )
        return false;

    if ( nColId != nCurColId || (bMakeVisible && !IsFieldVisible(nCurRow, nColId, true)))
    {
        sal_uInt16 nNewPos = GetColumnPos(nColId);
        BrowserColumn* pColumn = (nNewPos < mvCols.size()) ? mvCols[ nNewPos ].get() : nullptr;
        DBG_ASSERT( pColumn, "no column object - invalid id?" );
        if ( !pColumn )
            return false;

        DoHideCursor();
        nCurColId = nColId;

        bool bScrolled = false;

        sal_uInt16 nFirstPos = nFirstCol;
        sal_uInt16 nWidth = static_cast<sal_uInt16>(pColumn->Width());
        sal_uInt16 nLastPos = GetColumnAtXPosPixel(
                            pDataWin->GetSizePixel().Width()-nWidth );
        sal_uInt16 nFrozen = FrozenColCount();
        if ( bMakeVisible && nLastPos &&
             nNewPos >= nFrozen && ( nNewPos < nFirstPos || nNewPos > nLastPos ) )
        {
            if ( nNewPos < nFirstPos )
                ScrollColumns( nNewPos-nFirstPos );
            else if ( nNewPos > nLastPos )
                ScrollColumns( nNewPos-nLastPos );
            bScrolled = true;
        }

        DoShowCursor();
        if (!bRowColMove)
        {
            //try to move to nCurRow, nColId
            CursorMoveAttempt aAttempt(nCurRow, nColId, bScrolled);
            //Detect if we are already in a call to BrowseBox::GoToColumnId
            //but the attempt is impossible and we are simply recursing
            //into BrowseBox::GoToColumnId with the same impossible to
            //fulfill conditions
            if (m_aGotoStack.empty() || aAttempt != m_aGotoStack.top())
            {
                m_aGotoStack.push(aAttempt);
                CursorMoved();
                m_aGotoStack.pop();
            }
        }
        return true;
    }
    return true;
}


bool BrowseBox::GoToRowColumnId( sal_Int32 nRow, sal_uInt16 nColId )
{

    // out of range?
    if ( nRow < 0 || nRow >= nRowCount )
        return false;

    if (!bColumnCursor)
        return false;

    // nothing to do ?
    if ( nRow == nCurRow && ( bMultiSelection || uRow.nSel == nRow ) &&
         nColId == nCurColId && IsFieldVisible(nCurRow, nColId, true))
        return true;

    // allowed?
    if (!IsCursorMoveAllowed(nRow, nColId))
        return false;

    DoHideCursor();
    bool bMoved = GoToRow(nRow, true) && GoToColumnId(nColId, true, true);
    DoShowCursor();

    if (bMoved)
        CursorMoved();

    return bMoved;
}


void BrowseBox::SetNoSelection()
{

    // is there no selection
    if ( ( !pColSel || !pColSel->GetSelectCount() ) &&
         ( ( !bMultiSelection && uRow.nSel == BROWSER_ENDOFSELECTION ) ||
           ( bMultiSelection && !uRow.pSel->GetSelectCount() ) ) )
        // nothing to do
        return;

    SAL_INFO("svtools", "BrowseBox::HideCursor " << this );
    ToggleSelection();

    // unselect all
    if ( bMultiSelection )
        uRow.pSel->SelectAll(false);
    else
        uRow.nSel = BROWSER_ENDOFSELECTION;
    if ( pColSel )
        pColSel->SelectAll(false);
    if ( !bSelecting )
        Select();
    else
        bSelect = true;

    // restore screen
    SAL_INFO("svtools", "BrowseBox::ShowCursor " << this );

    if ( isAccessibleAlive() )
    {
        commitTableEvent(
            AccessibleEventId::SELECTION_CHANGED,
            Any(),
            Any()
        );
    }
}


void BrowseBox::SelectAll()
{

    if ( !bMultiSelection )
        return;

    SAL_INFO("svtools", "BrowseBox::HideCursor " << this );
    ToggleSelection();

    // select all rows
    if ( pColSel )
        pColSel->SelectAll(false);
    uRow.pSel->SelectAll();

    // don't highlight handle column
    BrowserColumn *pFirstCol = mvCols[ 0 ].get();
    tools::Long nOfsX = pFirstCol->GetId() ? 0 : pFirstCol->Width();

    // highlight the row selection
    if ( !bHideSelect )
    {
        tools::Rectangle aHighlightRect;
        sal_uInt16 nVisibleRows =
            static_cast<sal_uInt16>(pDataWin->GetOutputSizePixel().Height() / GetDataRowHeight() + 1);
        for ( sal_Int32 nRow = std::max<sal_Int32>( nTopRow, uRow.pSel->FirstSelected() );
              nRow != BROWSER_ENDOFSELECTION && nRow < nTopRow + nVisibleRows;
              nRow = uRow.pSel->NextSelected() )
            aHighlightRect.Union( tools::Rectangle(
                Point( nOfsX, (nRow-nTopRow)*GetDataRowHeight() ),
                Size( pDataWin->GetSizePixel().Width(), GetDataRowHeight() ) ) );
        pDataWin->Invalidate( aHighlightRect );
    }

    if ( !bSelecting )
        Select();
    else
        bSelect = true;

    // restore screen
    SAL_INFO("svtools", "BrowseBox::ShowCursor " << this );

    if ( !isAccessibleAlive() )
        return;

    commitTableEvent(
        AccessibleEventId::SELECTION_CHANGED,
        Any(),
        Any()
    );
    commitHeaderBarEvent(
        AccessibleEventId::SELECTION_CHANGED,
        Any(),
        Any(),
        true
    ); // column header event

    commitHeaderBarEvent(
        AccessibleEventId::SELECTION_CHANGED,
        Any(),
        Any(),
        false
    ); // row header event
}


void BrowseBox::SelectRow( sal_Int32 nRow, bool _bSelect, bool bExpand )
{

    if ( !bMultiSelection )
    {
        // deselecting is impossible, selecting via cursor
        if ( _bSelect )
            GoToRow(nRow, false);
        return;
    }

    SAL_INFO("svtools", "BrowseBox::HideCursor " << this );

    // remove old selection?
    if ( !bExpand || !bMultiSelection )
    {
        ToggleSelection();
        if ( bMultiSelection )
            uRow.pSel->SelectAll(false);
        else
            uRow.nSel = BROWSER_ENDOFSELECTION;
        if ( pColSel )
            pColSel->SelectAll(false);
    }

    // set new selection
    if  (   !bHideSelect
        &&  (   (   bMultiSelection
                &&  uRow.pSel->GetTotalRange().Max() >= nRow
                &&  uRow.pSel->Select( nRow, _bSelect )
                )
            ||  (   !bMultiSelection
                &&  ( uRow.nSel = nRow ) != BROWSER_ENDOFSELECTION )
                )
            )
    {
        // don't highlight handle column
        BrowserColumn *pFirstCol = mvCols[ 0 ].get();
        tools::Long nOfsX = pFirstCol->GetId() ? 0 : pFirstCol->Width();

        // highlight only newly selected part
        tools::Rectangle aRect(
            Point( nOfsX, (nRow-nTopRow)*GetDataRowHeight() ),
            Size( pDataWin->GetSizePixel().Width(), GetDataRowHeight() ) );
        pDataWin->Invalidate( aRect );
    }

    if ( !bSelecting )
        Select();
    else
        bSelect = true;

    // restore screen
    SAL_INFO("svtools", "BrowseBox::ShowCursor " << this );

    if ( !isAccessibleAlive() )
        return;

    commitTableEvent(
        AccessibleEventId::SELECTION_CHANGED,
        Any(),
        Any()
    );
    commitHeaderBarEvent(
        AccessibleEventId::SELECTION_CHANGED,
        Any(),
        Any(),
        false
    ); // row header event
}


sal_Int32 BrowseBox::GetSelectRowCount() const
{

    return bMultiSelection ? uRow.pSel->GetSelectCount() :
           uRow.nSel == BROWSER_ENDOFSELECTION ? 0 : 1;
}


void BrowseBox::SelectColumnPos( sal_uInt16 nNewColPos, bool _bSelect, bool bMakeVisible )
{

    if ( !bColumnCursor || nNewColPos == BROWSER_INVALIDID )
        return;

    if ( !bMultiSelection )
    {
        if ( _bSelect )
            GoToColumnId( mvCols[ nNewColPos ]->GetId(), bMakeVisible );
        return;
    }
    else
    {
        if ( !GoToColumnId( mvCols[ nNewColPos ]->GetId(), bMakeVisible ) )
            return;
    }

    SAL_INFO("svtools", "BrowseBox::HideCursor " << this );
    ToggleSelection();
    if ( bMultiSelection )
        uRow.pSel->SelectAll(false);
    else
        uRow.nSel = BROWSER_ENDOFSELECTION;
    pColSel->SelectAll(false);

    if ( pColSel->Select( nNewColPos, _bSelect ) )
    {
        // GoToColumnId( mvCols->GetObject(nNewColPos)->GetId(), bMakeVisible );

        // only highlight painted areas
        pDataWin->Update();
        tools::Rectangle aFieldRectPix( GetFieldRectPixel( nCurRow, nCurColId, false ) );
        tools::Rectangle aRect(
            Point( aFieldRectPix.Left() - MIN_COLUMNWIDTH, 0 ),
            Size( mvCols[ nNewColPos ]->Width(),
                  pDataWin->GetOutputSizePixel().Height() ) );
        pDataWin->Invalidate( aRect );
        if ( !bSelecting )
            Select();
        else
            bSelect = true;

        if ( isAccessibleAlive() )
        {
            commitTableEvent(
                AccessibleEventId::SELECTION_CHANGED,
                Any(),
                Any()
            );
            commitHeaderBarEvent(
                AccessibleEventId::SELECTION_CHANGED,
                Any(),
                Any(),
                true
            ); // column header event
        }
    }

    // restore screen
    SAL_INFO("svtools", "BrowseBox::ShowCursor " << this );
}


sal_uInt16 BrowseBox::GetSelectColumnCount() const
{

    // while bAutoSelect (==!pColSel), 1 if any rows (yes rows!) else none
    return pColSel ? static_cast<sal_uInt16>(pColSel->GetSelectCount()) :
           nCurRow >= 0 ? 1 : 0;
}


sal_Int32 BrowseBox::FirstSelectedColumn( ) const
{
    return pColSel ? pColSel->FirstSelected() : BROWSER_ENDOFSELECTION;
}


sal_Int32 BrowseBox::FirstSelectedRow()
{

    return bMultiSelection ? uRow.pSel->FirstSelected() : uRow.nSel;
}


sal_Int32 BrowseBox::NextSelectedRow()
{

    return bMultiSelection ? uRow.pSel->NextSelected() : BROWSER_ENDOFSELECTION;
}


sal_Int32 BrowseBox::LastSelectedRow()
{

    return bMultiSelection ? uRow.pSel->LastSelected() : uRow.nSel;
}


bool BrowseBox::IsRowSelected( sal_Int32 nRow ) const
{

    return bMultiSelection ? uRow.pSel->IsSelected(nRow) : nRow == uRow.nSel;
}


bool BrowseBox::IsColumnSelected( sal_uInt16 nColumnId ) const
{

    return pColSel ? pColSel->IsSelected( GetColumnPos(nColumnId) ) :
                     nCurColId == nColumnId;
}


void BrowseBox::MakeFieldVisible
(
    sal_Int32   nRow,       // line number of the field (starting with 0)
    sal_uInt16  nColId     // column ID of the field
)

/*  [Description]

    Makes visible the field described in 'nRow' and 'nColId' by scrolling
    accordingly.

*/

{
    if (!pDataWin)
        return;

    Size aTestSize = pDataWin->GetSizePixel();

    if ( !bBootstrapped || aTestSize.IsEmpty() )
        return;

    // is it visible already?
    bool bVisible = IsFieldVisible( nRow, nColId, true/*bComplete*/ );
    if ( bVisible )
        return;

    // calculate column position, field rectangle and painting area
    sal_uInt16 nColPos = GetColumnPos( nColId );
    tools::Rectangle aFieldRect = GetFieldRectPixel( nRow, nColId, false );
    tools::Rectangle aDataRect( Point(0, 0), pDataWin->GetSizePixel() );

    // positioned outside on the left?
    if ( nColPos >= FrozenColCount() && nColPos < nFirstCol )
        // => scroll to the right
        ScrollColumns( nColPos - nFirstCol );

    // while outside on the right
    while ( aDataRect.Right() < aFieldRect.Right() )
    {
        // => scroll to the left
        if ( ScrollColumns( 1 ) != 1 )
            // no more need to scroll
            break;
        aFieldRect = GetFieldRectPixel( nRow, nColId, false );
    }

    // positioned outside above?
    if ( nRow < nTopRow )
        // scroll further to the bottom
        ScrollRows( nRow - nTopRow );

    // positioned outside below?
    sal_Int32 nBottomRow = nTopRow + GetVisibleRows();
    // decrement nBottomRow to make it the number of the last visible line
    // (count starts with 0!).
    // Example: BrowseBox contains exactly one entry. nBottomRow := 0 + 1 - 1
    if( nBottomRow )
        nBottomRow--;

    if ( nRow > nBottomRow )
        // scroll further to the top
        ScrollRows( nRow - nBottomRow );
}


bool BrowseBox::IsFieldVisible( sal_Int32 nRow, sal_uInt16 nColumnId,
                                bool bCompletely ) const
{

    // hidden by frozen column?
    sal_uInt16 nColPos = GetColumnPos( nColumnId );
    if ( nColPos >= FrozenColCount() && nColPos < nFirstCol )
        return false;

    tools::Rectangle aRect( ImplFieldRectPixel( nRow, nColumnId ) );
    if ( aRect.IsEmpty() )
        return false;

    // get the visible area
    tools::Rectangle aOutRect( Point(0, 0), pDataWin->GetOutputSizePixel() );

    if ( bCompletely )
        // test if the field is completely visible
        return aOutRect.Contains( aRect );
    else
        // test if the field is partly of completely visible
        return !aOutRect.Intersection( aRect ).IsEmpty();
}


tools::Rectangle BrowseBox::GetFieldRectPixel( sal_Int32 nRow, sal_uInt16 nColumnId,
                                        bool bRelToBrowser) const
{

    // get the rectangle relative to DataWin
    tools::Rectangle aRect( ImplFieldRectPixel( nRow, nColumnId ) );
    if ( aRect.IsEmpty() )
        return aRect;

    // adjust relative to BrowseBox's output area
    Point aTopLeft( aRect.TopLeft() );
    if ( bRelToBrowser )
    {
        aTopLeft = pDataWin->OutputToScreenPixel( aTopLeft );
        aTopLeft = ScreenToOutputPixel( aTopLeft );
    }

    return tools::Rectangle( aTopLeft, aRect.GetSize() );
}


tools::Rectangle BrowseBox::GetRowRectPixel( sal_Int32 nRow  ) const
{

    // get the rectangle relative to DataWin
    tools::Rectangle aRect;
    if ( nTopRow > nRow )
        // row is above visible area
        return aRect;
    aRect = tools::Rectangle(
        Point( 0, GetDataRowHeight() * (nRow-nTopRow) ),
        Size( pDataWin->GetOutputSizePixel().Width(), GetDataRowHeight() ) );
    if ( aRect.Top() > pDataWin->GetOutputSizePixel().Height() )
        // row is below visible area
        return aRect;

    // adjust relative to BrowseBox's output area
    Point aTopLeft( aRect.TopLeft() );
    aTopLeft = pDataWin->OutputToScreenPixel( aTopLeft );
    aTopLeft = ScreenToOutputPixel( aTopLeft );

    return tools::Rectangle( aTopLeft, aRect.GetSize() );
}


tools::Rectangle BrowseBox::ImplFieldRectPixel( sal_Int32 nRow, sal_uInt16 nColumnId ) const
{

    // compute the X-coordinate relative to DataWin by accumulation
    tools::Long nColX = 0;
    sal_uInt16 nFrozenCols = FrozenColCount();
    size_t nCol;
    for ( nCol = 0;
          nCol < mvCols.size() && mvCols[ nCol ]->GetId() != nColumnId;
          ++nCol )
        if ( mvCols[ nCol ]->IsFrozen() || nCol >= nFirstCol )
            nColX += mvCols[ nCol ]->Width();

    if ( nCol >= mvCols.size() || ( nCol >= nFrozenCols && nCol < nFirstCol ) )
        return tools::Rectangle();

    // compute the Y-coordinate relative to DataWin
    tools::Long nRowY = GetDataRowHeight();
    if ( nRow != BROWSER_ENDOFSELECTION ) // #105497# OJ
        nRowY = ( nRow - nTopRow ) * GetDataRowHeight();

    // assemble the Rectangle relative to DataWin
    return tools::Rectangle(
        Point( nColX + MIN_COLUMNWIDTH, nRowY ),
        Size( (mvCols[nCol]->Width() == LONG_MAX
               ? LONG_MAX - (nColX + MIN_COLUMNWIDTH) : mvCols[ nCol ]->Width() - 2*MIN_COLUMNWIDTH),
              GetDataRowHeight() - 1 ) );
}


sal_Int32 BrowseBox::GetRowAtYPosPixel( tools::Long nY, bool bRelToBrowser ) const
{

    // compute the Y-coordinate
    if ( bRelToBrowser )
    {
        Point aDataTopLeft = pDataWin->OutputToScreenPixel( Point(0, 0) );
        Point aTopLeft = OutputToScreenPixel( Point(0, 0) );
        nY -= aDataTopLeft.Y() - aTopLeft.Y();
    }

    // no row there (e.g. in the header)
    if ( nY < 0 || nY >= pDataWin->GetOutputSizePixel().Height() )
        return -1;

    return nY / GetDataRowHeight() + nTopRow;
}


tools::Rectangle BrowseBox::GetFieldRect( sal_uInt16 nColumnId ) const
{

    return GetFieldRectPixel( nCurRow, nColumnId );
}


sal_uInt16 BrowseBox::GetColumnAtXPosPixel( tools::Long nX ) const
{

    // accumulate the widths of the visible columns
    tools::Long nColX = 0;
    for ( size_t nCol = 0; nCol < mvCols.size(); ++nCol )
    {
        BrowserColumn *pCol = mvCols[ nCol ].get();
        if ( pCol->IsFrozen() || nCol >= nFirstCol )
            nColX += pCol->Width();

        if ( nColX > nX )
            return nCol;
    }

    return BROWSER_INVALIDID;
}

bool BrowseBox::ReserveControlArea(sal_uInt16 nWidth)
{
    if (nWidth != nControlAreaWidth)
    {
        OSL_ENSURE(nWidth,"Control area of 0 is not allowed, Use USHRT_MAX instead!");
        nControlAreaWidth = nWidth;
        UpdateScrollbars();
        return true;
    }
    return false;
}

tools::Rectangle BrowseBox::GetControlArea() const
{
    auto nHeight = aHScroll->GetSizePixel().Height();
    auto nEndRight = aHScroll->GetPosPixel().X();

    return tools::Rectangle(
        Point( 0, GetOutputSizePixel().Height() - nHeight ),
        Size( nEndRight, nHeight ) );
}

void BrowseBox::SetMode( BrowserMode nMode )
{

    pDataWin->bAutoHScroll = BrowserMode::AUTO_HSCROLL == ( nMode & BrowserMode::AUTO_HSCROLL );
    pDataWin->bAutoVScroll = BrowserMode::AUTO_VSCROLL == ( nMode & BrowserMode::AUTO_VSCROLL );
    pDataWin->bNoHScroll   = BrowserMode::NO_HSCROLL   == ( nMode & BrowserMode::NO_HSCROLL );
    pDataWin->bNoVScroll   = BrowserMode::NO_VSCROLL   == ( nMode & BrowserMode::NO_VSCROLL );

    DBG_ASSERT( !( pDataWin->bAutoHScroll && pDataWin->bNoHScroll ),
        "BrowseBox::SetMode: AutoHScroll *and* NoHScroll?" );
    DBG_ASSERT( !( pDataWin->bAutoVScroll && pDataWin->bNoVScroll ),
        "BrowseBox::SetMode: AutoVScroll *and* NoVScroll?" );
    if ( pDataWin->bAutoHScroll )
        pDataWin->bNoHScroll = false;
    if ( pDataWin->bAutoVScroll )
        pDataWin->bNoVScroll = false;

    if ( pDataWin->bNoHScroll )
        aHScroll->Hide();

    nControlAreaWidth = USHRT_MAX;

    tools::Long nOldRowSel = bMultiSelection ? uRow.pSel->FirstSelected() : uRow.nSel;
    MultiSelection *pOldRowSel = bMultiSelection ? uRow.pSel : nullptr;

    pVScroll.disposeAndClear();

    bMultiSelection = bool( nMode & BrowserMode::MULTISELECTION );
    bColumnCursor = bool( nMode & BrowserMode::COLUMNSELECTION );
    bKeepHighlight = bool( nMode & BrowserMode::KEEPHIGHLIGHT );

    bHideSelect = ((nMode & BrowserMode::HIDESELECT) == BrowserMode::HIDESELECT);
    // default: do not hide the cursor at all (untaken scrolling and such)
    bHideCursor = TRISTATE_FALSE;

    if ( BrowserMode::HIDECURSOR == ( nMode & BrowserMode::HIDECURSOR ) )
    {
        bHideCursor = TRISTATE_TRUE;
    }

    m_bFocusOnlyCursor = ((nMode & BrowserMode::CURSOR_WO_FOCUS) == BrowserMode::NONE);

    bHLines = ( nMode & BrowserMode::HLINES ) == BrowserMode::HLINES;
    bVLines = ( nMode & BrowserMode::VLINES ) == BrowserMode::VLINES;

    pVScroll = VclPtr<ScrollAdaptor>::Create(this, false);
    pVScroll->SetLineSize( 1 );
    pVScroll->SetPageSize(1);
    pVScroll->SetScrollHdl( LINK( this, BrowseBox, VertScrollHdl ) );

    pDataWin->bAutoSizeLastCol =
            BrowserMode::AUTOSIZE_LASTCOL == ( nMode & BrowserMode::AUTOSIZE_LASTCOL );

    // create a headerbar. what happens, if a headerbar has to be created and
    // there already are columns?
    if ( BrowserMode::HEADERBAR_NEW == ( nMode & BrowserMode::HEADERBAR_NEW ) )
    {
        if (!pDataWin->pHeaderBar)
            pDataWin->pHeaderBar = CreateHeaderBar( this );
    }
    else
    {
        pDataWin->pHeaderBar.disposeAndClear();
    }

    if ( bColumnCursor )
    {
        if (!pColSel)
            pColSel.reset(new MultiSelection);
        pColSel->SetTotalRange( Range( 0, mvCols.size()-1 ) );
    }
    else
    {
        pColSel.reset();
    }

    if ( bMultiSelection )
    {
        if ( pOldRowSel )
            uRow.pSel = pOldRowSel;
        else
            uRow.pSel = new MultiSelection;
    }
    else
    {
        uRow.nSel = nOldRowSel;
        delete pOldRowSel;
    }

    if ( bBootstrapped )
    {
        StateChanged( StateChangedType::InitShow );
        if ( bMultiSelection && !pOldRowSel &&
             nOldRowSel != BROWSER_ENDOFSELECTION )
            uRow.pSel->Select( nOldRowSel );
    }

    if ( pDataWin )
        pDataWin->Invalidate();

    // no cursor on handle column
    if ( nCurColId == HandleColumnId )
        nCurColId = GetColumnId( 1 );

    m_nCurrentMode = nMode;
}


void BrowseBox::VisibleRowsChanged( sal_Int32, sal_uInt16 )
{

    // old behavior: automatically correct NumRows:
    if ( nRowCount < GetRowCount() )
    {
        RowInserted(nRowCount,GetRowCount() - nRowCount, false);
    }
    else if ( nRowCount > GetRowCount() )
    {
        RowRemoved(GetRowCount(), nRowCount - GetRowCount(), false);
    }
}


bool BrowseBox::IsCursorMoveAllowed( sal_Int32, sal_uInt16 ) const

/*  [Description]

    This virtual method is always called before the cursor is moved directly.
    By means of 'return false', we avoid doing this if e.g. a record
    contradicts any rules.

    This method is not called, if the cursor movement results from removing or
    deleting a row/column (thus, in cases where only a "cursor correction" happens).

    The base implementation currently always returns true.
*/

{
    return true;
}


tools::Long BrowseBox::GetDataRowHeight() const
{
    return CalcZoom(m_nDataRowHeight ? m_nDataRowHeight : ImpGetDataRowHeight());
}


VclPtr<BrowserHeader> BrowseBox::CreateHeaderBar( BrowseBox* pParent )
{
    VclPtr<BrowserHeader> pNewBar = VclPtr<BrowserHeader>::Create( pParent );
    pNewBar->SetStartDragHdl( LINK( this, BrowseBox, StartDragHdl ) );
    return pNewBar;
}

void BrowseBox::SetHeaderBar( BrowserHeader* pHeaderBar )
{
    pDataWin->pHeaderBar.disposeAndClear();
    pDataWin->pHeaderBar = pHeaderBar;
    pDataWin->pHeaderBar->SetStartDragHdl( LINK( this, BrowseBox, StartDragHdl ) );
}

tools::Long BrowseBox::GetTitleHeight() const
{
    tools::Long nHeight;
    // ask the header bar for the text height (if possible), as the header bar's font is adjusted with
    // our (and the header's) zoom factor
    HeaderBar* pHeaderBar = pDataWin->pHeaderBar;
    if ( pHeaderBar )
        nHeight = pHeaderBar->GetTextHeight();
    else
        nHeight = GetTextHeight();

    return nTitleLines ? nTitleLines * nHeight + 4 : 0;
}

tools::Long BrowseBox::CalcReverseZoom(tools::Long nVal) const
{
    if (IsZoom())
    {
        const Fraction& rZoom = GetZoom();
        double n = static_cast<double>(nVal);
        n *= static_cast<double>(rZoom.GetDenominator());
        if (!rZoom.GetNumerator())
            throw o3tl::divide_by_zero();
        n /= static_cast<double>(rZoom.GetNumerator());
        nVal = n>0 ? static_cast<tools::Long>(n + 0.5) : -static_cast<tools::Long>(-n + 0.5);
    }

    return nVal;
}

void BrowseBox::CursorMoved()
{
    // before implementing more here, please adjust the EditBrowseBox

    if ( isAccessibleAlive() && HasFocus() )
        commitTableEvent(
            AccessibleEventId::ACTIVE_DESCENDANT_CHANGED,
            Any( CreateAccessibleCell( GetCurRow(),GetColumnPos( GetCurColumnId() ) ) ),
            Any()
        );
}

void BrowseBox::LoseFocus()
{
    SAL_INFO("svtools", "BrowseBox::LoseFocus " << this );

    if ( bHasFocus )
    {
        SAL_INFO("svtools", "BrowseBox::HideCursor " << this );
        DoHideCursor();

        if ( !bKeepHighlight )
        {
            ToggleSelection();
            bSelectionIsVisible = false;
        }

        bHasFocus = false;
    }
    Control::LoseFocus();
}


void BrowseBox::GetFocus()
{
    SAL_INFO("svtools", "BrowseBox::GetFocus " << this );

    if ( !bHasFocus )
    {
        if ( !bSelectionIsVisible )
        {
            bSelectionIsVisible = true;
            if ( bBootstrapped )
                ToggleSelection();
        }

        bHasFocus = true;
        DoShowCursor();
    }
    Control::GetFocus();
}


sal_uInt16 BrowseBox::GetVisibleRows() const
{
    return static_cast<sal_uInt16>((pDataWin->GetOutputSizePixel().Height() - 1 )/ GetDataRowHeight() + 1);
}

BrowserDataWin& BrowseBox::GetDataWindow() const
{
    return *pDataWin;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
