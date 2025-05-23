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

#include <comphelper/string.hxx>
#include <editeng/eeitem.hxx>
#include <osl/diagnose.h>

#include <editeng/editview.hxx>
#include <editeng/flditem.hxx>
#include <svx/hlnkitem.hxx>
#include <svl/srchitem.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/request.hxx>
#include <svl/stritem.hxx>

#include <tabvwsh.hxx>
#include <sc.hrc>
#include <scmod.hxx>
#include <impex.hxx>
#include <editsh.hxx>
#include <dociter.hxx>
#include <inputhdl.hxx>
#include <document.hxx>

OUString ScTabViewShell::GetSelectionText( bool bWholeWord, bool bOnlyASample )
{
    OUString aStrSelection;

    if ( pEditShell && pEditShell.get() == GetMySubShell() )
    {
        aStrSelection = pEditShell->GetSelectionText( bWholeWord );
    }
    else
    {
        ScRange aRange;

        if ( GetViewData().GetSimpleArea( aRange ) == SC_MARK_SIMPLE )
        {
            ScDocument& rDoc = GetViewData().GetDocument();
            if ( (bOnlyASample || bInFormatDialog) && aRange.aStart.Row() != aRange.aEnd.Row() )
            {
                // limit range to one data row
                // (only when  the call comes from a format dialog)
                ScHorizontalCellIterator aIter( rDoc, aRange.aStart.Tab(),
                    aRange.aStart.Col(), aRange.aStart.Row(),
                    aRange.aEnd.Col(), aRange.aEnd.Row() );
                SCCOL nCol;
                SCROW nRow;
                if ( aIter.GetNext( nCol, nRow ) )
                {
                    aRange.aStart.SetCol( nCol );
                    aRange.aStart.SetRow( nRow );
                    aRange.aEnd.SetRow( nRow );
                }
                else
                    aRange.aEnd = aRange.aStart;
            }
            else
            {
                // #i111531# with 1M rows it was necessary to limit the range
                // to the actually used data area.
                SCCOL nCol1, nCol2;
                SCROW nRow1, nRow2;
                SCTAB nTab1, nTab2;
                aRange.GetVars( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                bool bShrunk;
                rDoc.ShrinkToUsedDataArea( bShrunk, nTab1, nCol1, nRow1, nCol2, nRow2, false);
                if (bShrunk)
                {
                    aRange.aStart.SetCol( nCol1 );
                    aRange.aStart.SetRow( nRow1 );
                    aRange.aEnd.SetCol( nCol2 );
                    aRange.aEnd.SetRow( nRow2 );
                }
            }

            ScImportExport aObj( rDoc, aRange );
            // tdf#148437 - if cell contains a formula, overwrite entire content of the cell
            aObj.SetFormulas(true);
            OUString aExportOUString;
            /* TODO: STRING_TSVC under some circumstances? */
            aObj.ExportString( aExportOUString, SotClipboardFormatId::STRING );
            aStrSelection = convertLineEnd(aExportOUString, LINEEND_CR);

            // replace Tab/CR with space, if for dialog or through Basic/SelectionTextExt,
            // or when it is a single row.
            // Otherwise keep Tabs in multi-row (for instance mail or Basic/SelectionText).
            // for mail the Tabs are then later changed into (multiple) spaces.

            if ( bInFormatDialog || bWholeWord || aRange.aEnd.Row() == aRange.aStart.Row() )
            {
                aStrSelection = aStrSelection.replaceAll("\r", " ");
                aStrSelection = aStrSelection.replaceAll("\t", " ");
                aStrSelection = comphelper::string::stripEnd(aStrSelection, ' ');
            }
        }
    }

    return aStrSelection;
}

void ScTabViewShell::InsertURL( const OUString& rName, const OUString& rURL, const OUString& rTarget,
                                sal_uInt16 nMode )
{
    SvxLinkInsertMode eMode = static_cast<SvxLinkInsertMode>(nMode);
    bool bAsText = ( eMode != HLINK_BUTTON );       // default is now text

    if ( bAsText )
    {
        if ( GetViewData().IsActive() )
        {
            //  if the view is active, always use InsertURLField, which starts EditMode
            //  and selects the URL, so it can be changed from the URL bar / dialog

            InsertURLField( rName, rURL, rTarget );
        }
        else
        {
            //  if the view is not active, InsertURLField doesn't work
            //  -> use InsertBookmark to directly manipulate cell content
            //  bTryReplace=sal_True -> if cell contains only one URL, replace it

            SCCOL nPosX = GetViewData().GetCurX();
            SCROW nPosY = GetViewData().GetCurY();
            InsertBookmark( rName, rURL, nPosX, nPosY, &rTarget, true );
        }
    }
    else
    {
        ScModule::get()->InputEnterHandler();
        InsertURLButton( rName, rURL, rTarget, nullptr );
    }
}

static void lcl_SelectFieldAfterInsert( EditView& rView )
{
    ESelection aSel = rView.GetSelection();
    if (aSel.start.nIndex == aSel.end.nIndex && aSel.start.nIndex > 0)
    {
        //  Cursor is behind the inserted field -> extend selection to the left

        --aSel.start.nIndex;
        rView.SetSelection( aSel );
    }
}

void ScTabViewShell::InsertURLField( const OUString& rName, const OUString& rURL, const OUString& rTarget )
{
    SvxURLField aURLField( rURL, rName, SvxURLFormat::Repr );
    aURLField.SetTargetFrame( rTarget );
    SvxFieldItem aURLItem( aURLField, EE_FEATURE_FIELD );

    ScViewData&     rViewData   = GetViewData();
    ScModule*       pScMod      = ScModule::get();
    ScInputHandler* pHdl        = pScMod->GetInputHdl( rViewData.GetViewShell() );

    bool bSelectFirst = false;
    bool bIsEditMode = pScMod->IsEditMode();
    int nSelInd = 1;
    OUString sSeltext(GetSelectionText());

    if ( !bIsEditMode )
    {
        if ( !SelectionEditable() )
        {
            // no error message (may be called from drag&drop)
            return;
        }

        // single url in cell is shown in the dialog and replaced
        bSelectFirst = HasBookmarkAtCursor( nullptr );
        pScMod->SetInputMode( SC_INPUT_TABLE );
    }

    EditView*       pTopView    = pHdl->GetTopView();
    EditView*       pTableView  = pHdl->GetTableView();
    OSL_ENSURE( pTopView || pTableView, "No EditView" );

    // Check if user selected a whole cell by single click, and cell has content.
    // tdf#80043 - if true, replace the entire content of the selected cell instead of
    // inserting a duplicate, or appending the url.
    if (!bIsEditMode && !bSelectFirst && pTableView && !sSeltext.isEmpty())
    {
        nSelInd = sSeltext.getLength();
        bSelectFirst = true;
    }

    if ( bSelectFirst )
    {
        if ( pTopView )
            pTopView->SetSelection( ESelection(0,0,0,1) );
        if ( pTableView )
            pTableView->SetSelection( ESelection(0,0,0,nSelInd) );
    }

    pHdl->DataChanging();

    if ( pTopView )
    {
        pTopView->InsertField( aURLItem );
        lcl_SelectFieldAfterInsert( *pTopView );
    }
    if ( pTableView )
    {
        pTableView->InsertField( aURLItem );
        lcl_SelectFieldAfterInsert( *pTableView );
    }

    pHdl->DataChanged();
}

void ScTabViewShell::ExecSearch( SfxRequest& rReq )
{
    const SfxItemSet*   pReqArgs    = rReq.GetArgs();
    sal_uInt16              nSlot       = rReq.GetSlot();
    const SfxPoolItem*  pItem;

    switch ( nSlot )
    {
        case FID_SEARCH_NOW:
            {
                const SvxSearchItem* pSearchItem;
                if ( pReqArgs &&
                     (pSearchItem = pReqArgs->GetItemIfSet(SID_SEARCH_ITEM, false)) )
                {
                    ScGlobal::SetSearchItem( *pSearchItem );
                    SearchAndReplace( pSearchItem, true, rReq.IsAPI() );
                    rReq.Done();
                }
            }
            break;

        case SID_SEARCH_ITEM:
        {
            const SvxSearchItem* pSearchItem;
            if (pReqArgs && (pSearchItem =
                            pReqArgs->GetItemIfSet(SID_SEARCH_ITEM, false)))
            {
                // remember search item
                ScGlobal::SetSearchItem( *pSearchItem );
            }
            else
            {
                OSL_FAIL("SID_SEARCH_ITEM without Parameter");
            }
            break;
        }
        case FID_SEARCH:
        case FID_REPLACE:
        case FID_REPLACE_ALL:
        case FID_SEARCH_ALL:
            {
                if (pReqArgs && SfxItemState::SET == pReqArgs->GetItemState(nSlot, false, &pItem))
                {
                    // get search item

                    SvxSearchItem aSearchItem = ScGlobal::GetSearchItem();

                    // fill search item

                    aSearchItem.SetSearchString(static_cast<const SfxStringItem*>(pItem)->GetValue());
                    if(SfxItemState::SET == pReqArgs->GetItemState(FN_PARAM_1, false, &pItem))
                        aSearchItem.SetReplaceString(static_cast<const SfxStringItem*>(pItem)->GetValue());

                    if (nSlot == FID_SEARCH)
                        aSearchItem.SetCommand(SvxSearchCmd::FIND);
                    else if(nSlot == FID_REPLACE)
                        aSearchItem.SetCommand(SvxSearchCmd::REPLACE);
                    else if(nSlot == FID_REPLACE_ALL)
                        aSearchItem.SetCommand(SvxSearchCmd::REPLACE_ALL);
                    else
                        aSearchItem.SetCommand(SvxSearchCmd::FIND_ALL);

                    // execute request (which stores the SearchItem)

                    aSearchItem.SetWhich(SID_SEARCH_ITEM);
                    GetViewData().GetDispatcher().ExecuteList(FID_SEARCH_NOW,
                            rReq.IsAPI() ? SfxCallMode::API|SfxCallMode::SYNCHRON :
                                            SfxCallMode::RECORD,
                            { &aSearchItem });
                }
                else
                {
                    GetViewData().GetDispatcher().Execute(
                            SID_SEARCH_DLG, SfxCallMode::ASYNCHRON|SfxCallMode::RECORD );
                }
            }
            break;
        case FID_REPEAT_SEARCH:
            {
                // once more with ScGlobal::GetSearchItem()

                SvxSearchItem aSearchItem = ScGlobal::GetSearchItem();
                aSearchItem.SetWhich(SID_SEARCH_ITEM);
                GetViewData().GetDispatcher().ExecuteList( FID_SEARCH_NOW,
                        rReq.IsAPI() ? SfxCallMode::API|SfxCallMode::SYNCHRON :
                                        SfxCallMode::RECORD,
                        { &aSearchItem });
            }
            break;
//      case FID_SEARCH_COUNT:
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
