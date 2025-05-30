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

#include <com/sun/star/embed/EmbedVerbs.hpp>
#include <editeng/flditem.hxx>
#include <svx/svddrgmt.hxx>
#include <svx/svdoole2.hxx>
#include <sfx2/dispatch.hxx>
#include <vcl/imapobj.hxx>
#include <svx/svdouno.hxx>
#include <svx/svdomedia.hxx>
#include <svx/svdpagv.hxx>
#include <svx/ImageMapInfo.hxx>
#include <editeng/outlobj.hxx>
#include <sfx2/app.hxx>
#include <sfx2/ipclient.hxx>
#include <sfx2/viewfrm.hxx>
#include <comphelper/lok.hxx>

#include <fusel.hxx>
#include <sc.hrc>
#include <fudraw.hxx>
#include <futext.hxx>
#include <drawview.hxx>
#include <tabvwsh.hxx>
#include <drwlayer.hxx>
#include <userdat.hxx>
#include <scmod.hxx>
#include <charthelper.hxx>
#include <docuno.hxx>
#include <docsh.hxx>
#include <stlpool.hxx>

//  maximal permitted mouse movement to start Drag&Drop
//! fusel,fuconstr,futext - combine them!
#define SC_MAXDRAGMOVE  3
// Min necessary mouse motion for normal dragging
#define SC_MINDRAGMOVE 2

using namespace com::sun::star;

FuSelection::FuSelection(ScTabViewShell& rViewSh, vcl::Window* pWin, ScDrawView* pViewP,
                         SdrModel* pDoc, const SfxRequest& rReq)
    : FuDraw(rViewSh, pWin, pViewP, pDoc, rReq)
{
}

FuSelection::~FuSelection()
{
}

bool FuSelection::MouseButtonDown(const MouseEvent& rMEvt)
{
    // remember button state for creation of own MouseEvents
    SetMouseButtonCode(rMEvt.GetButtons());
    const bool bSelectionOnly = rMEvt.IsRight();
    if ( pView->IsAction() )
    {
        if ( bSelectionOnly )
            pView->BckAction();
        return true;
    }

    bIsInDragMode = false;      //  somewhere it has to be reset (#50033#)

    bool bReturn = FuDraw::MouseButtonDown(rMEvt);
    auto aLogicPosition = rMEvt.getLogicPosition();
    if (aLogicPosition)
        aMDPos = *aLogicPosition;
    else
        aMDPos = pWindow->PixelToLogic(rMEvt.GetPosPixel());

    if (comphelper::LibreOfficeKit::isActive())
    {
        ScViewData& rViewData = rViewShell.GetViewData();
        ScDocument& rDocument = rViewData.GetDocument();
        if (rDocument.IsNegativePage(rViewData.GetTabNo()))
            aMDPos.setX(-aMDPos.X());
    }

    if ( rMEvt.IsLeft() )
    {
        SdrHdl* pHdl = pView->PickHandle(aMDPos);

        if ( pHdl!=nullptr || pView->IsMarkedHit(aMDPos) )
        {
            // Determine if this is the tail of a SdrCaptionObj i.e.
            // we need to disable the drag option on the tail of a note
            // object. Also, disable the ability to use the circular
            // drag of a note object.
            bool bDrag = false;
            const SdrMarkList& rMarkList = pView->GetMarkedObjectList();
            if( rMarkList.GetMarkCount() == 1 )
            {
                SdrObject* pMarkedObj = rMarkList.GetMark( 0 )->GetMarkedSdrObj();
                if( ScDrawLayer::IsNoteCaption( pMarkedObj ) )
                {
                    // move using the valid caption handles for note text box.
                    if(pHdl && (pHdl->GetKind() != SdrHdlKind::Poly && pHdl->GetKind() != SdrHdlKind::Circle))
                        bDrag = true;
                    // move the complete note box.
                    else if(!pHdl)
                        bDrag = true;
                }
                else
                    bDrag = true;   // different object
            }
            else
                bDrag = true;       // several objects

            if ( bDrag )
            {
                aDragTimer.Start();
                if (pView->BegDragObj(aMDPos, nullptr, pHdl))
                    pView->GetDragMethod()->SetShiftPressed( rMEvt.IsShift() );
                bReturn = true;
            }
        }
        else
        {
            SdrPageView* pPV = nullptr;
            bool bAlt = rMEvt.IsMod2();
            SdrObject* pObj = !bAlt ? pView->PickObj(aMDPos, pView->getHitTolLog(), pPV, SdrSearchOptions::PICKMACRO) : nullptr;
            if (pObj)
            {
                pView->BegMacroObj(aMDPos, pObj, pPV, pWindow);
                bReturn = true;
            }
            else
            {
                OUString sURL, sTarget;
                pObj = !bAlt ? pView->PickObj(aMDPos, pView->getHitTolLog(), pPV, SdrSearchOptions::ALSOONMASTER) : nullptr;
                if (pObj)
                {
                   // Support for imported Excel docs
                   // Excel is of course not consistent and allows
                   // a hyperlink to be assigned for an object group
                   // and even though the hyperlink is exported in the Escher layer
                   // its never used, when dealing with a group object the link
                   // associated with the clicked object is used only

                   // additionally you can also select a macro in Excel for a grouped
                   // objects and this *usually* results in the macro being set
                   // for the elements in the group and no macro is exported
                   // for the group itself ( this however is not always true )
                   // if a macro and hlink are defined favour the hlink
                   // If a group object has no hyperlink use the hyperlink of the
                   // object clicked

                   if ( pObj->IsGroupObject() )
                   {
                       ScMacroInfo* pTmpInfo = ScDrawLayer::GetMacroInfo( pObj );
                       if ( !pTmpInfo || pTmpInfo->GetMacro().isEmpty() )
                       {
                           SdrObject* pHit = pView->PickObj(aMDPos, pView->getHitTolLog(), pPV, SdrSearchOptions::DEEP);
                           if (pHit)
                               pObj = pHit;
                       }
                   }

                   ScMacroInfo* pInfo = ScDrawLayer::GetMacroInfo( pObj, true );
                   // For interoperability favour links over macros if both are defined
                   if ( !pObj->getHyperlink().isEmpty() )
                   {
                       sURL = pObj->getHyperlink();
                   }
                   else if ( !pInfo->GetMacro().isEmpty() )
                   {
                       SfxObjectShell* pObjSh = SfxObjectShell::Current();
                       if ( pObjSh && SfxApplication::IsXScriptURL( pInfo->GetMacro() ) )
                       {
                           uno::Reference< beans::XPropertySet > xProps( pObj->getUnoShape(), uno::UNO_QUERY );
                           uno::Any aCaller;
                           if ( xProps.is() )
                           {
                               try
                               {
                                   aCaller = xProps->getPropertyValue(u"Name"_ustr);
                               }
                               catch( uno::Exception& ) {}
                           }
                           uno::Any aRet;
                           uno::Sequence< sal_Int16 > aOutArgsIndex;
                           uno::Sequence< uno::Any > aOutArgs;
                           uno::Sequence< uno::Any > aInArgs;
                           pObjSh->CallXScript( pInfo->GetMacro(),
                               aInArgs, aRet, aOutArgsIndex, aOutArgs, true, &aCaller );
                           rViewShell.FakeButtonUp( rViewShell.GetViewData().GetActivePart() );
                           return true;        // no CaptureMouse etc.
                       }
                   }
                }

                //  URL / ImageMap

                SdrViewEvent aVEvt;
                if ( !bAlt &&
                    pView->PickAnything( rMEvt, SdrMouseEventKind::BUTTONDOWN, aVEvt ) != SdrHitKind::NONE &&
                    aVEvt.mpObj != nullptr )
                {
                    if ( SvxIMapInfo::GetIMapInfo( aVEvt.mpObj ) )       // ImageMap
                    {
                        const IMapObject* pIMapObj =
                                SvxIMapInfo::GetHitIMapObject( aVEvt.mpObj, aMDPos, pWindow->GetOutDev() );
                        if ( pIMapObj && !pIMapObj->GetURL().isEmpty() )
                        {
                            sURL = pIMapObj->GetURL();
                            sTarget = pIMapObj->GetTarget();
                        }
                    }
                    if ( aVEvt.meEvent == SdrEventKind::ExecuteUrl && aVEvt.mpURLField )   // URL
                    {
                        sURL = aVEvt.mpURLField->GetURL();
                        sTarget = aVEvt.mpURLField->GetTargetFrame();
                    }
                }

                // open hyperlink, if found at object or in object's text
                // Fragments pointing into the current document should be always opened.
                if ( !sURL.isEmpty() && (ScGlobal::ShouldOpenURL() || sURL.startsWith("#")) )
                {
                    ScGlobal::OpenURL( sURL, sTarget );
                    rViewShell.FakeButtonUp( rViewShell.GetViewData().GetActivePart() );
                    return true;        // no CaptureMouse etc.
                }

                //  Is another object being edited in this view?
                //  (Editing is ended in MarkListHasChanged - test before UnmarkAll)
                SfxInPlaceClient* pClient = rViewShell.GetIPClient();
                bool bWasOleActive = ( pClient && pClient->IsObjectInPlaceActive() );

                //  Selection

                // do not allow multiselection with note caption
                bool bCaptionClicked = IsNoteCaptionClicked( aMDPos );
                if ( !rMEvt.IsShift() || bCaptionClicked || IsNoteCaptionMarked() )
                    pView->UnmarkAll();

                /*  Unlock internal layer, if a note caption is clicked. The
                    layer will be relocked in ScDrawView::MarkListHasChanged(). */
                if( bCaptionClicked )
                    pView->UnlockInternalLayer();

                // try to select the clicked object
                if ( pView->MarkObj( aMDPos, -2, false, rMEvt.IsMod1() ) )
                {

                    // move object

                    if (pView->IsMarkedHit(aMDPos))
                    {
                        //  Don't start drag timer if inplace editing of an OLE object
                        //  was just ended with this mouse click - the view will be moved
                        //  (different tool bars) and the object that was clicked on would
                        //  be moved unintentionally.
                        if ( !bWasOleActive )
                            aDragTimer.Start();

                        pHdl=pView->PickHandle(aMDPos);
                        pView->BegDragObj(aMDPos, nullptr, pHdl);
                        bReturn = true;
                    }
                    else                                    // object at the edge
                        if (rViewShell.IsDrawSelMode())
                            bReturn = true;
                }
                else
                {
                    if (rViewShell.IsDrawSelMode())
                    {

                        // select object

                        pView->BegMarkObj(aMDPos);
                        bReturn = true;
                    }
                }
            }
        }

    }

    if (!bIsInDragMode)
    {
        // VC calls CaptureMouse itself
        pWindow->CaptureMouse();
        ForcePointer(&rMEvt);
    }

    return bReturn;
}

bool FuSelection::MouseMove(const MouseEvent& rMEvt)
{
    bool bReturn = FuDraw::MouseMove(rMEvt);

    if (aDragTimer.IsActive() )
    {
        Point aOldPixel = pWindow->LogicToPixel( aMDPos );
        Point aNewPixel = rMEvt.GetPosPixel();
        if ( std::abs( aOldPixel.X() - aNewPixel.X() ) > SC_MAXDRAGMOVE ||
             std::abs( aOldPixel.Y() - aNewPixel.Y() ) > SC_MAXDRAGMOVE )
            aDragTimer.Stop();
    }

    if ( pView->IsAction() )
    {
        Point aPix(rMEvt.GetPosPixel());
        Point aPnt(pWindow->PixelToLogic(aPix));

        ForceScroll(aPix);
        pView->MovAction(aPnt);
        bReturn = true;
    }

    ForcePointer(&rMEvt);

    return bReturn;
}

bool FuSelection::MouseButtonUp(const MouseEvent& rMEvt)
{
    // remember button state for creation of own MouseEvents
    SetMouseButtonCode(rMEvt.GetButtons());

    bool bReturn = FuDraw::MouseButtonUp(rMEvt);
    bool bOle = rViewShell.GetViewFrame().GetFrame().IsInPlace();

    SdrObject* pObj = nullptr;
    if (aDragTimer.IsActive() )
    {
        aDragTimer.Stop();
    }

    sal_uInt16 nDrgLog = sal_uInt16 ( pWindow->PixelToLogic(Size(SC_MINDRAGMOVE,0)).Width() );
    auto aLogicPosition = rMEvt.getLogicPosition();
    Point aPnt(aLogicPosition ? *aLogicPosition : pWindow->PixelToLogic(rMEvt.GetPosPixel()));

    bool bCopy = false;
    ScViewData& rViewData = rViewShell.GetViewData();
    ScDocument& rDocument = rViewData.GetDocument();
    SdrPageView* pPageView = ( pView ? pView->GetSdrPageView() : nullptr );
    SdrPage* pPage = ( pPageView ? pPageView->GetPage() : nullptr );
    ::std::vector< OUString > aExcludedChartNames;
    ScRangeListVector aProtectedChartRangesVector;

    if (comphelper::LibreOfficeKit::isActive() && rDocument.IsNegativePage(rViewData.GetTabNo()))
        aPnt.setX(-aPnt.X());

    if (pView && rMEvt.IsLeft())
    {
        if ( pView->IsDragObj() )
        {
            // object was moved
            if ( rMEvt.IsMod1() )
            {
                if ( pPage )
                {
                    ScChartHelper::GetChartNames( aExcludedChartNames, pPage );
                }
                if ( pView )
                {
                    const SdrMarkList& rMarkList = pView->GetMarkedObjectList();
                    const size_t nMarkCount = rMarkList.GetMarkCount();
                    for ( size_t i = 0; i < nMarkCount; ++i )
                    {
                        SdrMark* pMark = rMarkList.GetMark( i );
                        pObj = ( pMark ? pMark->GetMarkedSdrObj() : nullptr );
                        if ( pObj )
                        {
                            ScChartHelper::AddRangesIfProtectedChart( aProtectedChartRangesVector, rDocument, pObj );
                        }
                    }
                }
                bCopy = true;
            }

            if (!rMEvt.IsShift() && !rMEvt.IsMod1() && !rMEvt.IsMod2() &&
                std::abs(aPnt.X() - aMDPos.X()) < nDrgLog &&
                std::abs(aPnt.Y() - aMDPos.Y()) < nDrgLog)
            {
                /* If a user wants to click on an object in front of a marked
                   one, he releases the mouse button immediately */
                SdrPageView* pPV = nullptr;
                pObj = pView->PickObj(aMDPos, pView->getHitTolLog(), pPV, SdrSearchOptions::ALSOONMASTER | SdrSearchOptions::BEFOREMARK);
                if (pObj)
                {
                    pView->UnmarkAllObj();
                    pView->MarkObj(pObj,pPV);
                    return true;
                }
            }
            pView->EndDragObj( rMEvt.IsMod1() );
            pView->ForceMarkedToAnotherPage();

            bReturn = true;
        }
        else if (pView->IsAction() )
        {
            // unlock internal layer to include note captions
            pView->UnlockInternalLayer();
            pView->EndAction();
            const SdrMarkList& rMarkList = pView->GetMarkedObjectList();
            if ( rMarkList.GetMarkCount() != 0 )
            {
                bReturn = true;

                /*  if multi-selection contains a note caption object, remove
                    all other objects from selection. */
                const size_t nCount = rMarkList.GetMarkCount();
                if( nCount > 1 )
                {
                    bool bFound = false;
                    for( size_t nIdx = 0; !bFound && (nIdx < nCount); ++nIdx )
                    {
                        pObj = rMarkList.GetMark( nIdx )->GetMarkedSdrObj();
                        bFound = ScDrawLayer::IsNoteCaption( pObj );
                        if( bFound )
                        {
                            pView->UnMarkAll();
                            pView->MarkObj( pObj, pView->GetSdrPageView() );
                        }
                    }
                }
            }
        }

        if (ScModule::get()->GetIsWaterCan())
        {
            auto pStyleSheet = rViewData.GetDocument().GetStyleSheetPool()->GetActualStyleSheet();
            if (pStyleSheet && pStyleSheet->GetFamily() == SfxStyleFamily::Frame)
                pView->SetStyleSheet(static_cast<SfxStyleSheet*>(pStyleSheet), false);
        }
    }

    // maybe consider OLE object
    SfxInPlaceClient* pIPClient = rViewShell.GetIPClient();

    if (pIPClient)
    {
        ScModule* pScMod = ScModule::get();
        bool bUnoRefDialog = pScMod->IsRefDialogOpen() && pScMod->GetCurRefDlgId() == WID_SIMPLE_REF;

        if ( pIPClient->IsObjectInPlaceActive() && !bUnoRefDialog )
            pIPClient->DeactivateObject();
    }

    sal_uInt16 nClicks = rMEvt.GetClicks();
    if (pView && nClicks == 2 && rMEvt.IsLeft())
    {
        const SdrMarkList& rMarkList = pView->GetMarkedObjectList();
        if ( rMarkList.GetMarkCount() != 0 )
        {
            if (rMarkList.GetMarkCount() == 1)
            {
                SdrMark* pMark = rMarkList.GetMark(0);
                pObj = pMark->GetMarkedSdrObj();

                //  only activate, when the mouse also is over the selected object

                SdrViewEvent aVEvt;
                SdrHitKind eHit = pView->PickAnything( rMEvt, SdrMouseEventKind::BUTTONDOWN, aVEvt );
                if (eHit != SdrHitKind::NONE && aVEvt.mpObj == pObj)
                {
                    assert(pObj);

                    SdrObjKind nSdrObjKind = pObj->GetObjIdentifier();

                    //  OLE: activate

                    if (nSdrObjKind == SdrObjKind::OLE2)
                    {
                        if (!bOle)
                        {
                            if (static_cast<SdrOle2Obj*>(pObj)->GetObjRef().is())
                            {
                                // release so if ActivateObject launches a warning dialog, then that dialog
                                // can get mouse events
                                if (pWindow->IsMouseCaptured())
                                    pWindow->ReleaseMouse();
                                rViewShell.ActivateObject(static_cast<SdrOle2Obj*>(pObj), css::embed::EmbedVerbs::MS_OLEVERB_PRIMARY);
                            }
                        }
                    }

                    //  Edit text
                    //  not in UNO controls
                    //  #i32352# not in media objects

                    else if ( DynCastSdrTextObj( pObj) != nullptr && dynamic_cast<const SdrUnoObj*>( pObj) == nullptr && dynamic_cast<const SdrMediaObj*>( pObj) ==  nullptr )
                    {
                        OutlinerParaObject* pOPO = pObj->GetOutlinerParaObject();
                        bool bVertical = ( pOPO && pOPO->IsEffectivelyVertical() );
                        sal_uInt16 nTextSlotId = bVertical ? SID_DRAW_TEXT_VERTICAL : SID_DRAW_TEXT;

                        rViewShell.GetViewData().GetDispatcher().
                            Execute(nTextSlotId, SfxCallMode::SYNCHRON | SfxCallMode::RECORD);

                        // Get the created FuText now and change into EditMode
                        FuPoor* pPoor = rViewShell.GetViewData().GetView()->GetDrawFuncPtr();
                        if ( pPoor && pPoor->GetSlotID() == nTextSlotId )    // has no RTTI
                        {
                            FuText* pText = static_cast<FuText*>(pPoor);
                            Point aMousePixel = rMEvt.GetPosPixel();
                            pText->SetInEditMode( pObj, &aMousePixel );
                        }
                        bReturn = true;
                    }
                }
            }
        }
        else if ( TestDetective( pView->GetSdrPageView(), aPnt ) )
            bReturn = true;
    }

    ForcePointer(&rMEvt);

    if (pWindow->IsMouseCaptured())
        pWindow->ReleaseMouse();

    //  command handler for context menu follows after MouseButtonUp,
    //  therefore here the hard IsLeft call
    if ( !bReturn && rMEvt.IsLeft() )
        if (rViewShell.IsDrawSelMode())
            rViewShell.GetViewData().GetDispatcher().
                Execute(SID_OBJECT_SELECT, SfxCallMode::SLOT | SfxCallMode::RECORD);

    if ( bCopy && pPage )
    {
        ScDocShell* pDocShell = rViewData.GetDocShell();
        ScModelObj* pModelObj = ( pDocShell ? pDocShell->GetModel() : nullptr );
        if ( pModelObj )
        {
            SCTAB nTab = rViewData.GetTabNo();
            ScChartHelper::CreateProtectedChartListenersAndNotify( rDocument, pPage, pModelObj, nTab,
                aProtectedChartRangesVector, aExcludedChartNames );
        }
    }

    return bReturn;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
