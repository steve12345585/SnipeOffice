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

#include <osl/diagnose.h>
#include <sfx2/viewfrm.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/docfile.hxx>
#include <tools/urlobj.hxx>
#include <vcl/commandevent.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weldutils.hxx>
#include <tabcont.hxx>
#include <tabvwsh.hxx>
#include <docsh.hxx>
#include <scmod.hxx>
#include <sc.hrc>
#include <globstr.hrc>
#include <transobj.hxx>
#include <clipparam.hxx>
#include <dragdata.hxx>
#include <markdata.hxx>
#include <gridwin.hxx>
#include <LibreOfficeKit/LibreOfficeKitEnums.h>
#include <comphelper/lok.hxx>

ScTabControl::ScTabControl( vcl::Window* pParent, ScViewData* pData )
    : TabBar(pParent, WB_3DLOOK | WB_MINSCROLL | WB_SCROLL | WB_RANGESELECT | WB_MULTISELECT | WB_DRAG, true)
    , DropTargetHelper(this)
    , DragSourceHelper(this)
    , pViewData(pData)
    , nMouseClickPageId(TabBar::PAGE_NOT_FOUND)
    , nSelPageIdByMouse(TabBar::PAGE_NOT_FOUND)
    , bErrorShown(false)
{
    ScDocument& rDoc = pViewData->GetDocument();

    OUString aString;
    Color aTabBgColor;
    SCTAB nCount = rDoc.GetTableCount();
    for (SCTAB i=0; i<nCount; i++)
    {
        if (!rDoc.IsVisible(i))
            continue;

        if (!rDoc.GetName(i,aString))
            continue;

        if ( rDoc.IsScenario(i) )
            InsertPage( static_cast<sal_uInt16>(i)+1, aString, TabBarPageBits::Blue);
        else
            InsertPage( static_cast<sal_uInt16>(i)+1, aString );

        if ( rDoc.IsTabProtected(i) )
            SetProtectionSymbol(static_cast<sal_uInt16>(i)+1, true);

        if ( !rDoc.IsDefaultTabBgColor(i) )
        {
            aTabBgColor = rDoc.GetTabBgColor(i);
            SetTabBgColor( static_cast<sal_uInt16>(i)+1, aTabBgColor );
        }
    }

    SetCurPageId( static_cast<sal_uInt16>(pViewData->GetTabNo()) + 1 );

    SetSizePixel( Size(SC_TABBAR_DEFWIDTH, 0) );

    SetSplitHdl( LINK( pViewData->GetView(), ScTabView, TabBarResize ) );

    EnableEditMode();
    UpdateInputContext();

    SetScrollAlwaysEnabled(false);

    SetScrollAreaContextHdl( LINK( this, ScTabControl, ShowPageList ) );
}

IMPL_LINK(ScTabControl, ShowPageList, const CommandEvent &, rEvent, void)
{
    tools::Rectangle aRect(rEvent.GetMousePosPixel(), Size(1, 1));
    weld::Window* pPopupParent = weld::GetPopupParent(*this, aRect);
    std::unique_ptr<weld::Builder> xBuilder(Application::CreateBuilder(pPopupParent, u"modules/scalc/ui/pagelistmenu.ui"_ustr));
    std::unique_ptr<weld::Menu> xPopup(xBuilder->weld_menu(u"menu"_ustr));

    sal_uInt16 nCurPageId = GetCurPageId();

    ScDocument& rDoc = pViewData->GetDocument();
    SCTAB nCount = rDoc.GetTableCount();
    for (SCTAB i=0; i<nCount; ++i)
    {
        if (!rDoc.IsVisible(i))
            continue;
        OUString aString;
        if (!rDoc.GetName(i, aString))
            continue;
        sal_uInt16 nId = static_cast<sal_uInt16>(i)+1;
        OUString sId = OUString::number(nId);
        xPopup->append_radio(sId, aString);
        if (nId == nCurPageId)
            xPopup->set_active(sId, true);
    }

    OUString sIdent(xPopup->popup_at_rect(pPopupParent, aRect));
    if (!sIdent.isEmpty())
        SwitchToPageId(sIdent.toUInt32());
}

ScTabControl::~ScTabControl()
{
    disposeOnce();
}

void ScTabControl::dispose()
{
    DragSourceHelper::dispose();
    DropTargetHelper::dispose();
    TabBar::dispose();
}

sal_uInt16 ScTabControl::GetMaxId() const
{
    sal_uInt16 nVisCnt = GetPageCount();
    if (nVisCnt)
        return GetPageId(nVisCnt-1);

    return 0;
}

SCTAB ScTabControl::GetPrivatDropPos(const Point& rPos )
{
    sal_uInt16 nPos = ShowDropPos(rPos);

    SCTAB nRealPos = static_cast<SCTAB>(nPos);

    if(nPos !=0 )
    {
        ScDocument& rDoc = pViewData->GetDocument();

        SCTAB nCount = rDoc.GetTableCount();

        sal_uInt16 nViewPos=0;
        nRealPos = nCount;
        for (SCTAB i=0; i<nCount; i++)
        {
            if (rDoc.IsVisible(i))
            {
                nViewPos++;
                if(nViewPos==nPos)
                {
                    SCTAB j;
                    for (j=i+1; j<nCount; j++)
                    {
                        if (rDoc.IsVisible(j))
                        {
                            break;
                        }
                    }
                    nRealPos =j;
                    break;
                }
            }
        }
    }
    return nRealPos ;
}

void ScTabControl::MouseButtonDown( const MouseEvent& rMEvt )
{
    ScModule* pScMod = ScModule::get();
    if ( !pScMod->IsModalMode() && !pScMod->IsFormulaMode() && !IsInEditMode() )
    {
        // activate View
        pViewData->GetViewShell()->SetActive();         // Appear and SetViewFrame
        pViewData->GetView()->ActiveGrabFocus();
    }

    if (rMEvt.IsLeft() && rMEvt.GetModifier() == 0)
        nMouseClickPageId = GetPageId(rMEvt.GetPosPixel());

    TabBar::MouseButtonDown( rMEvt );
}

void ScTabControl::MouseButtonUp( const MouseEvent& rMEvt )
{
    Point aPos = PixelToLogic( rMEvt.GetPosPixel() );

    // mouse button down and up on same page?
    if( nMouseClickPageId != GetPageId(aPos))
        nMouseClickPageId = TabBar::PAGE_NOT_FOUND;

    if ( rMEvt.GetClicks() == 2 && rMEvt.IsLeft() && nMouseClickPageId != 0 && nMouseClickPageId != TabBar::PAGE_NOT_FOUND )
    {
        SfxDispatcher* pDispatcher = pViewData->GetViewShell()->GetViewFrame().GetDispatcher();
        pDispatcher->Execute( FID_TAB_MENU_RENAME, SfxCallMode::SYNCHRON | SfxCallMode::RECORD );
        return;
    }

    if( nMouseClickPageId == 0 )
    {
        // Click in the area next to the existing tabs:
        SfxDispatcher* pDispatcher = pViewData->GetViewShell()->GetViewFrame().GetDispatcher();
        pDispatcher->Execute( FID_TAB_DESELECTALL, SfxCallMode::SYNCHRON | SfxCallMode::RECORD );
        // forget page ID, to be really sure that the dialog is not called twice
        nMouseClickPageId = TabBar::PAGE_NOT_FOUND;
    }

    TabBar::MouseButtonUp( rMEvt );
}

void ScTabControl::AddTabClick()
{
    TabBar::AddTabClick();

    // Insert a new sheet at the right end, with default name.
    ScDocument& rDoc = pViewData->GetDocument();
    ScModule* pScMod = ScModule::get();
    if (!rDoc.IsDocEditable() || pScMod->IsTableLocked())
        return;

    // auto-accept any in-process input - which would otherwise end up on the new sheet
    if (!pScMod->IsFormulaMode())
        pScMod->InputEnterHandler();

    OUString aName;
    rDoc.CreateValidTabName(aName);
    SCTAB nTabCount = rDoc.GetTableCount();
    pViewData->GetViewShell()->InsertTable(aName, nTabCount);
    if (!pScMod->IsModalMode() && !pScMod->IsFormulaMode() && !IsInEditMode())
        pViewData->GetViewShell()->SetActive();
}

void ScTabControl::Select()
{
    /*  Remember last clicked page ID. */
    nSelPageIdByMouse = nMouseClickPageId;
    /*  Reset nMouseClickPageId, so that next Select() call may invalidate
        nSelPageIdByMouse (i.e. if called from keyboard). */
    nMouseClickPageId = TabBar::PAGE_NOT_FOUND;

    ScModule* pScMod = ScModule::get();
    ScDocument& rDoc = pViewData->GetDocument();
    ScMarkData& rMark = pViewData->GetMarkData();
    SCTAB nCount = rDoc.GetTableCount();
    SCTAB i;

    if ( pScMod->IsTableLocked() )      // may not be switched now ?
    {
        // restore the old state of TabControls

        for (i=0; i<nCount; i++)
            SelectPage( static_cast<sal_uInt16>(i)+1, rMark.GetTableSelect(i) );
        SetCurPageId( static_cast<sal_uInt16>(pViewData->GetTabNo()) + 1 );

        return;
    }

    sal_uInt16 nCurId = GetCurPageId();
    if (!nCurId) return;            // for Excel import it can happen that everything is hidden
    sal_uInt16 nPage = nCurId - 1;

    // OLE-inplace deactivate
    if ( nPage != static_cast<sal_uInt16>(pViewData->GetTabNo()) )
        pViewData->GetView()->DrawMarkListHasChanged();

    //  InputEnterHandler onlw when not reference input

    bool bRefMode = pScMod->IsFormulaMode();
    if (!bRefMode)
        pScMod->InputEnterHandler();

    for (i=0; i<nCount; i++)
        rMark.SelectTable( i, IsPageSelected(static_cast<sal_uInt16>(i)+1) );

    SfxDispatcher& rDisp = pViewData->GetDispatcher();
    if (rDisp.IsLocked())
        pViewData->GetView()->SetTabNo( static_cast<SCTAB>(nPage) );
    else
    {
        // sheet for basic is 1-based
        SfxUInt16Item aItem( SID_CURRENTTAB, nPage + 1 );
        rDisp.ExecuteList(SID_CURRENTTAB,
                SfxCallMode::SLOT | SfxCallMode::RECORD, { &aItem });
    }

    SfxBindings& rBind = pViewData->GetBindings();
    rBind.Invalidate( FID_FILL_TAB );
    rBind.Invalidate( FID_TAB_DESELECTALL );

    rBind.Invalidate( FID_INS_TABLE );
    rBind.Invalidate( FID_TAB_APPEND );
    rBind.Invalidate( FID_TAB_MOVE );
    rBind.Invalidate( FID_TAB_DUPLICATE );
    rBind.Invalidate( FID_TAB_RENAME );
    rBind.Invalidate( FID_DELETE_TABLE );
    rBind.Invalidate( FID_TABLE_SHOW );
    rBind.Invalidate( FID_TABLE_HIDE );
    rBind.Invalidate( FID_TAB_SET_TAB_BG_COLOR );

    // Recalculate status bar functions.
    rBind.Invalidate( SID_TABLE_CELL );

        // SetReference onlw when the consolidate dialog is open
        // (for references over multiple sheets)
        // for others this is only needed fidgeting

    if ( bRefMode && pViewData->GetRefType() == SC_REFTYPE_REF )
        if ( pViewData->GetViewShell()->GetViewFrame().HasChildWindow(SID_OPENDLG_CONSOLIDATE) )
        {
            ScRange aRange(
                    pViewData->GetRefStartX(), pViewData->GetRefStartY(), pViewData->GetRefStartZ(),
                    pViewData->GetRefEndX(), pViewData->GetRefEndY(), pViewData->GetRefEndZ() );
            pScMod->SetReference( aRange, rDoc, &rMark );
            pScMod->EndReference();                     // due to Auto-Hide
        }
}

void ScTabControl::UpdateInputContext()
{
    ScDocument& rDoc = pViewData->GetDocument();
    WinBits nStyle = GetStyle();
    if (rDoc.GetDocumentShell()->IsReadOnly())
        // no insert sheet tab for readonly doc.
        SetStyle(nStyle & ~WB_INSERTTAB);
    else
        SetStyle(nStyle | WB_INSERTTAB);
}

void ScTabControl::UpdateStatus()
{
    ScDocument& rDoc = pViewData->GetDocument();
    ScMarkData& rMark = pViewData->GetMarkData();
    bool bActive = pViewData->IsActive();

    SCTAB nCount = rDoc.GetTableCount();
    SCTAB i;
    OUString aString;
    SCTAB nMaxCnt = std::max( nCount, static_cast<SCTAB>(GetMaxId()) );
    Color aTabBgColor;

    bool bModified = false;                                     // sheet name
    for (i=0; i<nMaxCnt && !bModified; i++)
    {
        if (rDoc.IsVisible(i))
        {
            rDoc.GetName(i,aString);
            aTabBgColor = rDoc.GetTabBgColor(i);
        }
        else
        {
            aString.clear();
        }

        if ( aString != GetPageText(static_cast<sal_uInt16>(i)+1) || (GetTabBgColor(static_cast<sal_uInt16>(i)+1) != aTabBgColor) )
            bModified = true;
    }

    if (bModified)
    {
        Clear();
        for (i=0; i<nCount; i++)
        {
            if (rDoc.IsVisible(i))
            {
                if (rDoc.GetName(i,aString))
                {
                    if ( rDoc.IsScenario(i) )
                        InsertPage(static_cast<sal_uInt16>(i)+1, aString, TabBarPageBits::Blue);
                    else
                        InsertPage( static_cast<sal_uInt16>(i)+1, aString );

                    if ( rDoc.IsTabProtected(i) )
                        SetProtectionSymbol(static_cast<sal_uInt16>(i)+1, true);

                    if ( !rDoc.IsDefaultTabBgColor(i) )
                    {
                        aTabBgColor = rDoc.GetTabBgColor(i);
                        SetTabBgColor(static_cast<sal_uInt16>(i)+1, aTabBgColor );
                    }
                }
            }
        }
    }
    SetCurPageId( static_cast<sal_uInt16>(pViewData->GetTabNo()) + 1 );

    if (bActive)
    {
        bModified = false;                                          // selection
        for (i=0; i<nMaxCnt && !bModified; i++)
            if ( rMark.GetTableSelect(i) != IsPageSelected(static_cast<sal_uInt16>(i)+1) )
                bModified = true;

        if ( bModified )
            for (i=0; i<nCount; i++)
                SelectPage( static_cast<sal_uInt16>(i)+1, rMark.GetTableSelect(i) );
    }
}

void ScTabControl::SetSheetLayoutRTL( bool bSheetRTL )
{
    SetEffectiveRTL( bSheetRTL );
    nSelPageIdByMouse = TabBar::PAGE_NOT_FOUND;
}

void ScTabControl::SwitchToPageId(sal_uInt16 nId)
{
    if (!nId)
        return;

    bool bAlreadySelected = IsPageSelected( nId );
    //make the clicked page the current one
    SetCurPageId( nId );
    //change the selection when the current one is not already
    //selected or part of a multi selection
    if(bAlreadySelected)
        return;

    sal_uInt16 nCount = GetMaxId();

    for (sal_uInt16 i=1; i<=nCount; i++)
        SelectPage( i, i==nId );
    Select();

    if (comphelper::LibreOfficeKit::isActive())
    {
        // notify LibreOfficeKit about changed page
        OString aPayload = OString::number(nId - 1);
        pViewData->GetViewShell()->libreOfficeKitViewCallback(LOK_CALLBACK_SET_PART, aPayload);
    }
}

void ScTabControl::Command( const CommandEvent& rCEvt )
{
    ScModule* pScMod = ScModule::get();
    ScTabViewShell* pViewSh  = pViewData->GetViewShell();
    bool            bDisable = pScMod->IsFormulaMode() || pScMod->IsModalMode();

    // first activate ViewFrame (Bug 19493):
    pViewSh->SetActive();

    if (rCEvt.GetCommand() != CommandEventId::ContextMenu || bDisable)
        return;

    // #i18735# select the page that is under the mouse cursor
    // if multiple tables are selected and the one under the cursor
    // is not part of them then unselect them
    sal_uInt16 nId = GetPageId( rCEvt.GetMousePosPixel() );
    SwitchToPageId(nId);

    // #i52073# OLE inplace editing has to be stopped before showing the sheet tab context menu
    pViewSh->DeactivateOle();

    //  Popup-Menu:
    //  get Dispatcher from ViewData (ViewFrame) instead of Shell (Frame), so it can't be null
    pViewData->GetDispatcher().ExecutePopup( u"sheettab"_ustr );
}

void ScTabControl::StartDrag( sal_Int8 /* nAction */, const Point& rPosPixel )
{
    ScModule* pScMod = ScModule::get();
    bool bDisable = pScMod->IsFormulaMode() || pScMod->IsModalMode();

    if (!bDisable)
    {
        vcl::Region aRegion( tools::Rectangle(0,0,0,0) );
        CommandEvent aCEvt( rPosPixel, CommandEventId::StartDrag, true );   // needed for StartDrag
        if (TabBar::StartDrag( aCEvt, aRegion ))
            DoDrag();
    }
}

void ScTabControl::DoDrag()
{
    ScDocShell* pDocSh = pViewData->GetDocShell();
    ScDocument& rDoc = pDocSh->GetDocument();

    SCTAB nTab = pViewData->GetTabNo();
    ScRange aTabRange( 0, 0, nTab, rDoc.MaxCol(), rDoc.MaxRow(), nTab );
    ScMarkData aTabMark = pViewData->GetMarkData();
    aTabMark.ResetMark();   // doesn't change marked table information
    aTabMark.SetMarkArea( aTabRange );

    ScDocumentUniquePtr pClipDoc(new ScDocument( SCDOCMODE_CLIP ));
    ScClipParam aClipParam(aTabRange, false);
    rDoc.CopyToClip(aClipParam, pClipDoc.get(), &aTabMark, false, false);

    TransferableObjectDescriptor aObjDesc;
    pDocSh->FillTransferableObjectDescriptor( aObjDesc );
    aObjDesc.maDisplayName = pDocSh->GetMedium()->GetURLObject().GetURLNoPass();
    // maSize is set in ScTransferObj ctor

    rtl::Reference<ScTransferObj> pTransferObj = new ScTransferObj( std::move(pClipDoc), std::move(aObjDesc) );

    pTransferObj->SetDragSourceFlags(ScDragSrc::Table);

    pTransferObj->SetDragSource( pDocSh, aTabMark );

    pTransferObj->SetSourceCursorPos( pViewData->GetCurX(), pViewData->GetCurY() );

    vcl::Window* pWindow = pViewData->GetActiveWin();
    ScModule::get()->SetDragObject(pTransferObj.get(), nullptr); // for internal D&D
    pTransferObj->StartDrag( pWindow, DND_ACTION_COPYMOVE | DND_ACTION_LINK );
}

static sal_uInt16 lcl_DocShellNr( const ScDocument& rDoc )
{
    sal_uInt16 nShellCnt = 0;
    SfxObjectShell* pShell = SfxObjectShell::GetFirst();
    while ( pShell )
    {
        if ( auto pDocShell = dynamic_cast<const ScDocShell *>(pShell) )
        {
            if ( &pDocShell->GetDocument() == &rDoc )
                return nShellCnt;

            ++nShellCnt;
        }
        pShell = SfxObjectShell::GetNext( *pShell );
    }

    OSL_FAIL("Document not found");
    return 0;
}

sal_Int8 ScTabControl::ExecuteDrop( const ExecuteDropEvent& rEvt )
{
    EndSwitchPage();

    ScDocument& rDoc = pViewData->GetDocument();
    const ScDragData& rData = ScModule::get()->GetDragData();
    if ( rData.pCellTransfer && (rData.pCellTransfer->GetDragSourceFlags() & ScDragSrc::Table) &&
            rData.pCellTransfer->GetSourceDocument() == &rDoc )
    {
        // moving of tables within the document
        SCTAB nPos = GetPrivatDropPos( rEvt.maPosPixel );
        HideDropPos();

        if ( nPos == rData.pCellTransfer->GetVisibleTab() && rEvt.mnAction == DND_ACTION_MOVE )
        {
            // #i83005# do nothing - don't move to the same position
            // (too easily triggered unintentionally, and might take a long time in large documents)
        }
        else
        {
            if ( !rDoc.GetChangeTrack() && rDoc.IsDocEditable() )
            {
                //! use table selection from the tab control where dragging was started?
                pViewData->GetView()->MoveTable( lcl_DocShellNr(rDoc), nPos, rEvt.mnAction != DND_ACTION_MOVE );

                rData.pCellTransfer->SetDragWasInternal();          // don't delete
                return DND_ACTION_COPY;
            }
        }
    }

    return DND_ACTION_NONE;
}

sal_Int8 ScTabControl::AcceptDrop( const AcceptDropEvent& rEvt )
{
    if ( rEvt.mbLeaving )
    {
        EndSwitchPage();
        HideDropPos();
        return rEvt.mnAction;
    }

    const ScDocument& rDoc = pViewData->GetDocument();
    const ScDragData& rData = ScModule::get()->GetDragData();
    if ( rData.pCellTransfer && (rData.pCellTransfer->GetDragSourceFlags() & ScDragSrc::Table) &&
            rData.pCellTransfer->GetSourceDocument() == &rDoc )
    {
        // moving of tables within the document
        if ( !rDoc.GetChangeTrack() && rDoc.IsDocEditable() )
        {
            ShowDropPos( rEvt.maPosPixel );
            return rEvt.mnAction;
        }
    }
    else                    // switch sheets for all formats
    {
        SwitchPage( rEvt.maPosPixel );      // switch sheet after timeout
        return 0;                           // nothing can be dropped here
    }

    return 0;
}

bool ScTabControl::StartRenaming()
{
    return pViewData->GetDocument().IsDocEditable();
}

TabBarAllowRenamingReturnCode ScTabControl::AllowRenaming()
{
    ScTabViewShell* pViewSh = pViewData->GetViewShell();
    OSL_ENSURE( pViewSh, "pViewData->GetViewShell()" );

    TabBarAllowRenamingReturnCode nRet = TABBAR_RENAMING_CANCEL;
    sal_uInt16 nId = GetEditPageId();
    if ( nId )
    {
        SCTAB nTab = nId - 1;
        OUString aNewName = GetEditText();
        bool bDone = pViewSh->RenameTable( aNewName, nTab );
        if ( bDone )
            nRet = TABBAR_RENAMING_YES;
        else if ( bErrorShown )
        {
            //  if the error message from this TabControl is currently visible,
            //  don't end edit mode now, to avoid problems when returning to
            //  the other call (showing the error) - this should not happen
            OSL_FAIL("ScTabControl::AllowRenaming: nested calls");
            nRet = TABBAR_RENAMING_NO;
        }
        else if (pViewData->GetDocShell()->IsInModalMode())
        {
            //  don't show error message above any modal dialog
            //  instead cancel renaming without error message
            //  e.g. start with default Sheet1, add another sheet
            //  alt+left click on Sheet2 tab, edit to say Sheet1
            //  ctrl+S to trigger modal file save dialog
            nRet = TABBAR_RENAMING_CANCEL;
        }
        else
        {
            bErrorShown = true;
            pViewSh->ErrorMessage( STR_INVALIDTABNAME );
            bErrorShown = false;
            nRet = TABBAR_RENAMING_NO;
        }
    }
    return nRet;
}

void ScTabControl::EndRenaming()
{
    if ( HasFocus() )
        pViewData->GetView()->ActiveGrabFocus();
}

void ScTabControl::Mirror()
{
    TabBar::Mirror();
    if( nSelPageIdByMouse != TabBar::PAGE_NOT_FOUND )
    {
        tools::Rectangle aRect( GetPageRect( GetCurPageId() ) );
        if( !aRect.IsEmpty() )
            SetPointerPosPixel( aRect.Center() );
        nSelPageIdByMouse = TabBar::PAGE_NOT_FOUND;  // only once after a Select()
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
