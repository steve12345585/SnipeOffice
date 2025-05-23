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

#include <memory>
#include <iderid.hxx>
#include <strings.hrc>
#include <helpids.h>

#include "baside2.hxx"
#include <baside3.hxx>
#include <basidesh.hxx>
#include <basobj.hxx>
#include <docsignature.hxx>
#include <iderdll.hxx>
#include "iderdll2.hxx"
#include <localizationmgr.hxx>
#include <managelang.hxx>
#include <ColorSchemeDialog.hxx>

#include <basic/basmgr.hxx>
#include <com/sun/star/script/ModuleType.hpp>
#include <com/sun/star/script/XLibraryContainerPassword.hpp>
#include <com/sun/star/script/XLibraryContainer2.hpp>
#include <com/sun/star/frame/XLayoutManager.hpp>
#include <svl/srchdefs.hxx>
#include <sal/log.hxx>
#include <osl/diagnose.h>
#include <sfx2/app.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/childwin.hxx>
#include <sfx2/dinfdlg.hxx>
#include <sfx2/minfitem.hxx>
#include <sfx2/request.hxx>
#include <sfx2/viewfrm.hxx>
#include <svx/svxids.hrc>
#include <svl/eitem.hxx>
#include <svl/intitem.hxx>
#include <svl/visitem.hxx>
#include <svl/whiter.hxx>
#include <vcl/texteng.hxx>
#include <vcl/textview.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <svx/zoomsliderctrl.hxx>
#include <svx/zoomslideritem.hxx>
#include <basegfx/utils/zoomtools.hxx>
#include <officecfg/Office/BasicIDE.hxx>

constexpr sal_Int32 TAB_HEIGHT_MARGIN = 10;

namespace basctl
{

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::frame;

static void lcl_InvalidateZoomSlots(SfxBindings* pBindings)
{
    if (!pBindings)
        return;

    static sal_uInt16 const aInval[] = {
        SID_ZOOM_OUT, SID_ZOOM_IN, SID_ATTR_ZOOMSLIDER, 0
    };
    pBindings->Invalidate(aInval);
}

void Shell::ExecuteSearch( SfxRequest& rReq )
{
    if ( !pCurWin )
        return;

    const SfxItemSet* pArgs = rReq.GetArgs();
    sal_uInt16 nSlot = rReq.GetSlot();

    // if searching has not been done before this time
    if (nSlot == SID_BASICIDE_REPEAT_SEARCH && !mpSearchItem)
    {
        rReq.SetReturnValue(SfxBoolItem(nSlot, false));
        nSlot = 0;
    }

    switch ( nSlot )
    {
        case SID_SEARCH_OPTIONS:
            break;
        case SID_SEARCH_ITEM:
            mpSearchItem.reset(pArgs->Get(SID_SEARCH_ITEM).Clone());
            break;
        case FID_SEARCH_ON:
            mbJustOpened = true;
            GetViewFrame().GetBindings().Invalidate(SID_SEARCH_ITEM);
            break;
        case SID_BASICIDE_REPEAT_SEARCH:
        case FID_SEARCH_NOW:
        {
            if (!pCurWin->HasActiveEditor())
                break;

            // If it is a repeat searching
            if ( nSlot == SID_BASICIDE_REPEAT_SEARCH )
            {
                if( !mpSearchItem )
                    mpSearchItem.reset( new SvxSearchItem( SID_SEARCH_ITEM ));
            }
            else
            {
                // Get SearchItem from request if it is the first searching
                if ( pArgs )
                {
                    mpSearchItem.reset(pArgs->Get(SID_SEARCH_ITEM).Clone());
                }
            }

            sal_Int32 nFound = 0;

            if ( mpSearchItem->GetCommand() == SvxSearchCmd::REPLACE_ALL )
            {
                sal_uInt16 nActModWindows = 0;
                for (auto const& window : aWindowTable)
                {
                    BaseWindow* pWin = window.second;
                    if (pWin->HasActiveEditor())
                        nActModWindows++;
                }

                bool bAllModules = nActModWindows <= 1;
                if (!bAllModules)
                {
                    std::unique_ptr<weld::MessageDialog> xQueryBox(Application::CreateMessageDialog(pCurWin ? pCurWin->GetFrameWeld() : nullptr,
                                                                   VclMessageType::Question, VclButtonsType::YesNo,
                                                                   IDEResId(RID_STR_SEARCHALLMODULES)));
                    xQueryBox->set_default_response(RET_YES);
                    bAllModules = xQueryBox->run() == RET_YES;
                }

                if (bAllModules)
                {
                    for (auto const& window : aWindowTable)
                    {
                        BaseWindow* pWin = window.second;
                        nFound += pWin->StartSearchAndReplace( *mpSearchItem );
                    }
                }
                else
                    nFound = pCurWin->StartSearchAndReplace( *mpSearchItem );

                OUString aReplStr(IDEResId(RID_STR_SEARCHREPLACES));
                aReplStr = aReplStr.replaceAll("XX", OUString::number(nFound));

                std::unique_ptr<weld::MessageDialog> xInfoBox(Application::CreateMessageDialog(pCurWin->GetFrameWeld(),
                                                              VclMessageType::Info, VclButtonsType::Ok,
                                                              aReplStr));
                xInfoBox->run();
            }
            else
            {
                bool bCanceled = false;
                nFound = pCurWin->StartSearchAndReplace( *mpSearchItem );
                if ( !nFound && !mpSearchItem->GetSelection() )
                {
                    // search other modules...
                    bool bChangeCurWindow = false;
                    auto it = std::find_if(aWindowTable.cbegin(), aWindowTable.cend(),
                                           [this](const WindowTable::value_type& item) { return item.second == pCurWin; });
                    if (it != aWindowTable.cend())
                        ++it;
                    BaseWindow* pWin = it != aWindowTable.cend() ? it->second.get() : nullptr;

                    bool bSearchedFromStart = false;
                    while ( !nFound && !bCanceled && ( pWin || !bSearchedFromStart ) )
                    {
                        if ( !pWin )
                        {
                            SfxViewFrame& rViewFrame = GetViewFrame();
                            SfxChildWindow* pChildWin = rViewFrame.GetChildWindow(SID_SEARCH_DLG);
                            auto xParent = pChildWin ? pChildWin->GetController() : nullptr;

                            std::unique_ptr<weld::MessageDialog> xQueryBox(Application::CreateMessageDialog(xParent ? xParent->getDialog() : nullptr,
                                                                           VclMessageType::Question, VclButtonsType::YesNo,
                                                                           IDEResId(RID_STR_SEARCHFROMSTART)));
                            xQueryBox->set_default_response(RET_YES);
                            if (xQueryBox->run() == RET_YES)
                            {
                                it = aWindowTable.cbegin();
                                if ( it != aWindowTable.cend() )
                                    pWin = it->second;
                                bSearchedFromStart = true;
                            }
                            else
                                bCanceled = true;
                        }

                        if (pWin && pWin->HasActiveEditor())
                        {
                            if ( pWin != pCurWin )
                            {
                                if ( pCurWin )
                                    pWin->SetSizePixel( pCurWin->GetSizePixel() );
                                nFound = pWin->StartSearchAndReplace( *mpSearchItem, true );
                            }
                            if ( nFound )
                            {
                                bChangeCurWindow = true;
                                break;
                            }
                        }
                        if ( pWin && ( pWin != pCurWin ) )
                        {
                            if ( it != aWindowTable.cend() )
                                ++it;
                            pWin = it != aWindowTable.cend() ? it->second.get() : nullptr;
                        }
                        else
                            pWin = nullptr;
                    }
                    if ( !nFound && bSearchedFromStart )
                        nFound = pCurWin->StartSearchAndReplace( *mpSearchItem, true );
                    if ( bChangeCurWindow )
                        SetCurWindow( pWin, true );
                }
                if ( !nFound && !bCanceled )
                {
                    std::unique_ptr<weld::MessageDialog> xInfoBox(Application::CreateMessageDialog(pCurWin->GetFrameWeld(),
                                                                  VclMessageType::Info, VclButtonsType::Ok,
                                                                  IDEResId(RID_STR_SEARCHNOTFOUND)));
                    xInfoBox->run();
                }
            }

            rReq.Done();
            break;
        }
        default:
            pCurWin->ExecuteCommand( rReq );
    }
}

void Shell::ExecuteCurrent( SfxRequest& rReq )
{
    if ( !pCurWin )
        return;

    switch ( rReq.GetSlot() )
    {
        case SID_BASICIDE_HIDECURPAGE:
        {
            pCurWin->StoreData();
            RemoveWindow( pCurWin, false );
        }
        break;
        case SID_BASICIDE_RENAMECURRENT:
        {
            pTabBar->StartEditMode( pTabBar->GetCurPageId() );
        }
        break;
        case SID_UNDO:
        case SID_REDO:
            if ( GetUndoManager() && pCurWin->AllowUndo() )
                GetViewFrame().ExecuteSlot( rReq );
            break;
        default:
            pCurWin->ExecuteCommand( rReq );
    }
}

//  no matter who's at the top, influence on the shell:
void Shell::ExecuteGlobal( SfxRequest& rReq )
{
    sal_uInt16 nSlot = rReq.GetSlot();
    switch ( nSlot )
    {
        case SID_NEWDOCDIRECT:
        {
            // we do not have a new document factory,
            // so just forward to a fallback method.
            SfxGetpApp()->ExecuteSlot(rReq);
        }
        break;

        case SID_BASICSTOP:
        {
            // maybe do not simply stop if on breakpoint!
            if (ModulWindow* pMCurWin = dynamic_cast<ModulWindow*>(pCurWin.get()))
                pMCurWin->BasicStop();
            StopBasic();
        }
        break;

        case SID_SAVEDOC:
        {
            if ( pCurWin )
            {
                // rewrite date into the BASIC
                StoreAllWindowData();

                // document basic
                ScriptDocument aDocument( pCurWin->GetDocument() );
                if ( aDocument.isDocument() )
                {
                    uno::Reference< task::XStatusIndicator > xStatusIndicator;

                    const SfxUnoAnyItem* pStatusIndicatorItem = rReq.GetArg<SfxUnoAnyItem>(SID_PROGRESS_STATUSBAR_CONTROL);
                    if ( pStatusIndicatorItem )
                        OSL_VERIFY( pStatusIndicatorItem->GetValue() >>= xStatusIndicator );
                    else
                    {
                        // get statusindicator
                        SfxViewFrame *pFrame_ = GetFrame();
                        if ( pFrame_ )
                        {
                            uno::Reference< task::XStatusIndicatorFactory > xStatFactory(
                                                                        pFrame_->GetFrame().GetFrameInterface(),
                                                                        uno::UNO_QUERY );
                            if( xStatFactory.is() )
                                xStatusIndicator = xStatFactory->createStatusIndicator();
                        }

                        if ( xStatusIndicator.is() )
                            rReq.AppendItem( SfxUnoAnyItem( SID_PROGRESS_STATUSBAR_CONTROL, uno::Any( xStatusIndicator ) ) );
                    }

                    aDocument.saveDocument( xStatusIndicator );
                }

                if (SfxBindings* pBindings = GetBindingsPtr())
                {
                    pBindings->Invalidate( SID_DOC_MODIFIED );
                    pBindings->Invalidate( SID_SAVEDOC );
                    pBindings->Invalidate( SID_SIGNATURE );
                }
            }
        }
        break;
        case SID_BASICIDE_MODULEDLG:
        {
            if ( rReq.GetArgs() )
            {
                const SfxUInt16Item &rTabId = rReq.GetArgs()->Get(SID_BASICIDE_ARG_TABID );
                Organize(rReq.GetFrameWeld(), nullptr, rTabId.GetValue());
            }
            else
                Organize(rReq.GetFrameWeld(), nullptr, 0);
        }
        break;
        case SID_BASICIDE_CHOOSEMACRO:
        {
            ChooseMacro(rReq.GetFrameWeld(), nullptr);
        }
        break;
        case SID_BASICIDE_CREATEMACRO:
        case SID_BASICIDE_EDITMACRO:
        {
            DBG_ASSERT( rReq.GetArgs(), "arguments expected" );
            const SfxMacroInfoItem& rInfo = rReq.GetArgs()->Get(SID_BASICIDE_ARG_MACROINFO );
            BasicManager* pBasMgr = const_cast<BasicManager*>(rInfo.GetBasicManager());
            DBG_ASSERT( pBasMgr, "Nothing selected in basic tree?" );

            ScriptDocument aDocument( ScriptDocument::getDocumentForBasicManager( pBasMgr ) );

            StartListening(*pBasMgr, DuplicateHandling::Prevent /* log on only once */);
            OUString aLibName( rInfo.GetLib() );
            if ( aLibName.isEmpty() )
                aLibName = "Standard" ;
            StarBASIC* pBasic = pBasMgr->GetLib( aLibName );
            if ( !pBasic )
            {
                // load module and dialog library (if not loaded)
                aDocument.loadLibraryIfExists( E_SCRIPTS, aLibName );
                aDocument.loadLibraryIfExists( E_DIALOGS, aLibName );

                // get Basic
                pBasic = pBasMgr->GetLib( aLibName );
            }
            DBG_ASSERT( pBasic, "No Basic!" );

            SetCurLib( aDocument, aLibName );

            if ( pBasic && rReq.GetSlot() == SID_BASICIDE_CREATEMACRO )
            {
                SbModule* pModule = pBasic->FindModule( rInfo.GetModule() );
                if ( !pModule )
                {
                    if ( !rInfo.GetModule().isEmpty() || pBasic->GetModules().empty() )
                    {
                        const OUString& aModName = rInfo.GetModule();

                        OUString sModuleCode;
                        if ( aDocument.createModule( aLibName, aModName, false, sModuleCode ) )
                            pModule = pBasic->FindModule( aModName );
                    }
                    else
                        pModule = pBasic->GetModules().front().get();
                }
                DBG_ASSERT( pModule, "No Module!" );
                if ( pModule && !pModule->GetMethods()->Find( rInfo.GetMethod(), SbxClassType::Method ) )
                    CreateMacro( pModule, rInfo.GetMethod() );
            }
            SfxViewFrame& rViewFrame = GetViewFrame();
            rViewFrame.ToTop();
            VclPtr<ModulWindow> pWin = FindBasWin( aDocument, aLibName, rInfo.GetModule(), true );
            DBG_ASSERT( pWin, "Edit/Create Macro: Window was not created/found!" );
            SetCurWindow( pWin, true );
            pWin->EditMacro( rInfo.GetMethod() );
        }
        break;

        case SID_BASICIDE_OBJCAT:
        {
            // Toggle the visibility of the object catalog
            bool bVisible = aObjectCatalog->IsVisible();
            aObjectCatalog->Show(!bVisible);
            if (pLayout)
                pLayout->ArrangeWindows();
            // refresh the button state
            if (SfxBindings* pBindings = GetBindingsPtr())
                pBindings->Invalidate(SID_BASICIDE_OBJCAT);

            std::shared_ptr<comphelper::ConfigurationChanges> batch(comphelper::ConfigurationChanges::create());
            officecfg::Office::BasicIDE::EditorSettings::ObjectCatalog::set(!bVisible, batch);
            batch->commit();
        }
        break;

        case SID_BASICIDE_WATCH:
        {
            // Toggling the watch window can only be done from a ModulWindow
            if (!dynamic_cast<ModulWindowLayout*>(pLayout.get()))
                return;

            bool bVisible = pModulLayout->IsWatchWindowVisible();
            pModulLayout->ShowWatchWindow(!bVisible);
            if (SfxBindings* pBindings = GetBindingsPtr())
                pBindings->Invalidate(SID_BASICIDE_WATCH);

            std::shared_ptr<comphelper::ConfigurationChanges> batch(comphelper::ConfigurationChanges::create());
            officecfg::Office::BasicIDE::EditorSettings::WatchWindow::set(!bVisible, batch);
            batch->commit();
        }
        break;

        case SID_BASICIDE_STACK:
        {
            // Toggling the stack window can only be done from a ModulWindow
            if (!dynamic_cast<ModulWindowLayout*>(pLayout.get()))
                return;

            bool bVisible = pModulLayout->IsStackWindowVisible();
            pModulLayout->ShowStackWindow(!bVisible);
            if (SfxBindings* pBindings = GetBindingsPtr())
                pBindings->Invalidate(SID_BASICIDE_STACK);

            std::shared_ptr<comphelper::ConfigurationChanges> batch(comphelper::ConfigurationChanges::create());
            officecfg::Office::BasicIDE::EditorSettings::StackWindow::set(!bVisible, batch);
            batch->commit();
        }
        break;

        case SID_BASICIDE_NAMECHANGEDONTAB:
        {
            DBG_ASSERT( rReq.GetArgs(), "arguments expected" );
            const SfxUInt16Item &rTabId = rReq.GetArgs()->Get(SID_BASICIDE_ARG_TABID );
            const SfxStringItem &rModName = rReq.GetArgs()->Get(SID_BASICIDE_ARG_MODULENAME );
            if ( aWindowTable.find( rTabId.GetValue() ) !=  aWindowTable.end() )
            {
                VclPtr<BaseWindow> pWin = aWindowTable[ rTabId.GetValue() ];
                const OUString& aNewName( rModName.GetValue() );
                OUString aOldName( pWin->GetName() );
                if ( aNewName != aOldName )
                {
                    bool bRenameOk = false;
                    if (ModulWindow* pModWin = dynamic_cast<ModulWindow*>(pWin.get()))
                    {
                        const OUString& aLibName = pModWin->GetLibName();
                        ScriptDocument aDocument( pWin->GetDocument() );

                        if (RenameModule(pModWin->GetFrameWeld(), aDocument, aLibName, aOldName, aNewName))
                        {
                            bRenameOk = true;
                            // Because we listen for container events for script
                            // modules, rename will delete the 'old' window
                            // pWin has been invalidated, restore now
                            pWin = FindBasWin( aDocument, aLibName, aNewName, true );
                        }

                    }
                    else if (DialogWindow* pDlgWin = dynamic_cast<DialogWindow*>(pWin.get()))
                    {
                        bRenameOk = pDlgWin->RenameDialog( aNewName );
                    }
                    if ( bRenameOk )
                    {
                        MarkDocumentModified( pWin->GetDocument() );
                    }
                    else
                    {
                        // set old name in TabWriter
                        sal_uInt16 nId = GetWindowId( pWin );
                        DBG_ASSERT( nId, "No entry in Tabbar!" );
                        if ( nId )
                            pTabBar->SetPageText( nId, aOldName );
                    }
                }

                // set focus to current window
                pWin->GrabFocus();
            }
        }
        break;
        case SID_BASICIDE_STOREMODULESOURCE:
        case SID_BASICIDE_UPDATEMODULESOURCE:
        {
            DBG_ASSERT( rReq.GetArgs(), "arguments expected" );
            const SfxMacroInfoItem& rInfo = rReq.GetArgs()->Get(SID_BASICIDE_ARG_MACROINFO );
            BasicManager* pBasMgr = const_cast<BasicManager*>(rInfo.GetBasicManager());
            DBG_ASSERT( pBasMgr, "Store source: No BasMgr?" );
            ScriptDocument aDocument( ScriptDocument::getDocumentForBasicManager( pBasMgr ) );
            VclPtr<ModulWindow> pWin = FindBasWin( aDocument, rInfo.GetLib(), rInfo.GetModule(), false, true );
            if ( pWin )
            {
                if ( rReq.GetSlot() == SID_BASICIDE_STOREMODULESOURCE )
                    pWin->StoreData();
                else
                    pWin->UpdateData();
            }
        }
        break;
        case SID_BASICIDE_STOREALLMODULESOURCES:
        case SID_BASICIDE_UPDATEALLMODULESOURCES:
        {
            for (auto const& window : aWindowTable)
            {
                BaseWindow* pWin = window.second;
                if (!pWin->IsSuspended() && dynamic_cast<ModulWindow*>(pWin))
                {
                    if ( rReq.GetSlot() == SID_BASICIDE_STOREALLMODULESOURCES )
                        pWin->StoreData();
                    else
                        pWin->UpdateData();
                }
            }
        }
        break;
        case SID_BASICIDE_LIBSELECTED:
        case SID_BASICIDE_LIBREMOVED:
        case SID_BASICIDE_LIBLOADED:
        {
            DBG_ASSERT( rReq.GetArgs(), "arguments expected" );
            const SfxUnoAnyItem& rShellItem = rReq.GetArgs()->Get( SID_BASICIDE_ARG_DOCUMENT_MODEL );
            uno::Reference< frame::XModel > xModel( rShellItem.GetValue(), UNO_QUERY );
            ScriptDocument aDocument( xModel.is() ? ScriptDocument( xModel ) : ScriptDocument::getApplicationScriptDocument() );
            const SfxStringItem& rLibNameItem = rReq.GetArgs()->Get( SID_BASICIDE_ARG_LIBNAME );
            const OUString& aLibName( rLibNameItem.GetValue() );

            if ( nSlot == SID_BASICIDE_LIBSELECTED )
            {
                // load module and dialog library (if not loaded)
                aDocument.loadLibraryIfExists( E_SCRIPTS, aLibName );
                aDocument.loadLibraryIfExists( E_DIALOGS, aLibName );

                // check password, if library is password protected and not verified
                bool bOK = true;
                Reference< script::XLibraryContainer > xModLibContainer( aDocument.getLibraryContainer( E_SCRIPTS ) );
                if ( xModLibContainer.is() && xModLibContainer->hasByName( aLibName ) )
                {
                    Reference< script::XLibraryContainerPassword > xPasswd( xModLibContainer, UNO_QUERY );
                    if ( xPasswd.is() && xPasswd->isLibraryPasswordProtected( aLibName ) && !xPasswd->isLibraryPasswordVerified( aLibName ) )
                    {
                        OUString aPassword;
                        bOK = QueryPassword(rReq.GetFrameWeld(), xModLibContainer, aLibName, aPassword);
                    }
                }

                if ( bOK )
                {
                    SetCurLib( aDocument, aLibName, true, false );
                }
                else
                {
                    // adjust old value...
                    if (SfxBindings* pBindings = GetBindingsPtr())
                        pBindings->Invalidate(SID_BASICIDE_LIBSELECTOR, true);
                }
            }
            else if ( nSlot == SID_BASICIDE_LIBREMOVED )
            {
                if ( m_aCurLibName.isEmpty() || ( aDocument == m_aCurDocument && aLibName == m_aCurLibName ) )
                {
                    RemoveWindows( aDocument, aLibName );
                    if ( aDocument == m_aCurDocument && aLibName == m_aCurLibName )
                    {
                        m_aCurDocument = ScriptDocument::getApplicationScriptDocument();
                        m_aCurLibName.clear();
                        // no UpdateWindows!
                        if (SfxBindings* pBindings = GetBindingsPtr())
                            pBindings->Invalidate( SID_BASICIDE_LIBSELECTOR );
                    }
                }
            }
            else    // Loaded...
                UpdateWindows();
        }
        break;
        case SID_BASICIDE_NEWMODULE:
        {
            VclPtr<ModulWindow> pWin = CreateBasWin( m_aCurDocument, m_aCurLibName, OUString() );
            DBG_ASSERT( pWin, "New Module: Could not create window!" );
            SetCurWindow( pWin, true );
        }
        break;
        case SID_BASICIDE_NEWDIALOG:
        {
            VclPtr<DialogWindow> pWin = CreateDlgWin( m_aCurDocument, m_aCurLibName, OUString() );
            DBG_ASSERT( pWin, "New Module: Could not create window!" );
            SetCurWindow( pWin, true );
        }
        break;
        case SID_BASICIDE_SBXRENAMED:
        {
            DBG_ASSERT( rReq.GetArgs(), "arguments expected" );
        }
        break;
        case SID_BASICIDE_SBXINSERTED:
        {
            DBG_ASSERT( rReq.GetArgs(), "arguments expected" );
            const SbxItem& rSbxItem = rReq.GetArgs()->Get(SID_BASICIDE_ARG_SBX );
            const ScriptDocument& aDocument( rSbxItem.GetDocument() );
            const OUString& aLibName( rSbxItem.GetLibName() );
            const OUString& aName( rSbxItem.GetName() );
            if ( m_aCurLibName.isEmpty() || ( aDocument == m_aCurDocument && aLibName == m_aCurLibName ) )
            {
                if ( rSbxItem.GetSbxType() == SBX_TYPE_MODULE )
                    FindBasWin( aDocument, aLibName, aName, true );
                else if ( rSbxItem.GetSbxType() == SBX_TYPE_DIALOG )
                    FindDlgWin( aDocument, aLibName, aName, true );
            }
        }
        break;
        case SID_BASICIDE_SBXDELETED:
        {
            DBG_ASSERT( rReq.GetArgs(), "arguments expected" );
            const SbxItem& rSbxItem = rReq.GetArgs()->Get(SID_BASICIDE_ARG_SBX );
            const ScriptDocument& aDocument( rSbxItem.GetDocument() );
            VclPtr<BaseWindow> pWin = FindWindow( aDocument, rSbxItem.GetLibName(), rSbxItem.GetName(), rSbxItem.GetSbxType(), true );
            if ( pWin )
                RemoveWindow( pWin, true );
        }
        break;
        case SID_BASICIDE_SHOWSBX:
        {
            DBG_ASSERT( rReq.GetArgs(), "arguments expected" );
            const SbxItem& rSbxItem = rReq.GetArgs()->Get(SID_BASICIDE_ARG_SBX );
            const ScriptDocument& aDocument( rSbxItem.GetDocument() );
            const OUString& aLibName( rSbxItem.GetLibName() );
            const OUString& aName( rSbxItem.GetName() );
            SetCurLib( aDocument, aLibName );
            BaseWindow* pWin = nullptr;
            if ( rSbxItem.GetSbxType() == SBX_TYPE_DIALOG )
            {
                pWin = FindDlgWin( aDocument, aLibName, aName, true );
            }
            else if ( rSbxItem.GetSbxType() == SBX_TYPE_MODULE )
            {
                pWin = FindBasWin( aDocument, aLibName, aName, true );
            }
            else if ( rSbxItem.GetSbxType() == SBX_TYPE_METHOD )
            {
                pWin = FindBasWin( aDocument, aLibName, aName, true );
                static_cast<ModulWindow*>(pWin)->EditMacro( rSbxItem.GetMethodName() );
            }
            DBG_ASSERT( pWin, "Window was not created!" );
            SetCurWindow( pWin, true );
            pTabBar->MakeVisible( pTabBar->GetCurPageId() );
        }
        break;
        case SID_BASICIDE_SHOWWINDOW:
        {
            std::unique_ptr< ScriptDocument > pDocument;

            const SfxStringItem* pDocumentItem = rReq.GetArg<SfxStringItem>(SID_BASICIDE_ARG_DOCUMENT);
            if ( pDocumentItem )
            {
                const OUString& sDocumentCaption = pDocumentItem->GetValue();
                if ( !sDocumentCaption.isEmpty() )
                    pDocument.reset( new ScriptDocument( ScriptDocument::getDocumentWithURLOrCaption( sDocumentCaption ) ) );
            }

            const SfxUnoAnyItem* pDocModelItem = rReq.GetArg<SfxUnoAnyItem>(SID_BASICIDE_ARG_DOCUMENT_MODEL);
            if (!pDocument && pDocModelItem)
            {
                uno::Reference< frame::XModel > xModel( pDocModelItem->GetValue(), UNO_QUERY );
                if ( xModel.is() )
                    pDocument.reset( new ScriptDocument( xModel ) );
            }

            if (!pDocument)
                break;

            const SfxStringItem* pLibNameItem = rReq.GetArg<SfxStringItem>(SID_BASICIDE_ARG_LIBNAME);
            if ( !pLibNameItem )
                break;

            OUString aLibName( pLibNameItem->GetValue() );
            pDocument->loadLibraryIfExists( E_SCRIPTS, aLibName );
            SetCurLib( *pDocument, aLibName );
            const SfxStringItem* pNameItem = rReq.GetArg<SfxStringItem>(SID_BASICIDE_ARG_NAME);
            if ( pNameItem )
            {
                const OUString& aName( pNameItem->GetValue() );
                OUString aModType( u"Module"_ustr );
                OUString aType( aModType );
                const SfxStringItem* pTypeItem = rReq.GetArg<SfxStringItem>(SID_BASICIDE_ARG_TYPE);
                if ( pTypeItem )
                    aType = pTypeItem->GetValue();

                BaseWindow* pWin = nullptr;
                if ( aType == aModType )
                    pWin = FindBasWin( *pDocument, aLibName, aName );
                else if ( aType == "Dialog" )
                    pWin = FindDlgWin( *pDocument, aLibName, aName );

                if ( pWin )
                {
                    SetCurWindow( pWin, true );
                    if ( pTabBar )
                        pTabBar->MakeVisible( pTabBar->GetCurPageId() );

                    if (ModulWindow* pModWin = dynamic_cast<ModulWindow*>(pWin))
                    {
                        const SfxUInt32Item* pLineItem = rReq.GetArg<SfxUInt32Item>(SID_BASICIDE_ARG_LINE);
                        if ( pLineItem )
                        {
                            pModWin->AssertValidEditEngine();
                            TextView* pTextView = pModWin->GetEditView();
                            if ( pTextView )
                            {
                                TextEngine* pTextEngine = pTextView->GetTextEngine();
                                if ( pTextEngine )
                                {
                                    sal_uInt32 nLine = pLineItem->GetValue();
                                    sal_uInt32 nLineCount = 0;
                                    for ( sal_uInt32 i = 0, nCount = pTextEngine->GetParagraphCount(); i < nCount; ++i )
                                        nLineCount += pTextEngine->GetLineCount( i );
                                    if ( nLine > nLineCount )
                                        nLine = nLineCount;
                                    if ( nLine > 0 )
                                        --nLine;

                                    // scroll window and set selection
                                    tools::Long nVisHeight = pModWin->GetOutputSizePixel().Height();
                                    tools::Long nTextHeight = pTextEngine->GetTextHeight();
                                    if ( nTextHeight > nVisHeight )
                                    {
                                        tools::Long nMaxY = nTextHeight - nVisHeight;
                                        tools::Long nOldY = pTextView->GetStartDocPos().Y();
                                        tools::Long nNewY = nLine * pTextEngine->GetCharHeight() - nVisHeight / 2;
                                        nNewY = std::min( nNewY, nMaxY );
                                        pTextView->Scroll( 0, -( nNewY - nOldY ) );
                                        pTextView->ShowCursor( false );
                                        pModWin->GetEditVScrollBar().SetThumbPos( pTextView->GetStartDocPos().Y() );
                                    }
                                    sal_uInt16 nCol1 = 0, nCol2 = 0;
                                    const SfxUInt16Item* pCol1Item = rReq.GetArg<SfxUInt16Item>(SID_BASICIDE_ARG_COLUMN1);
                                    if ( pCol1Item )
                                    {
                                        nCol1 = pCol1Item->GetValue();
                                        if ( nCol1 > 0 )
                                            --nCol1;
                                        nCol2 = nCol1;
                                    }
                                    const SfxUInt16Item* pCol2Item = rReq.GetArg<SfxUInt16Item>(SID_BASICIDE_ARG_COLUMN2);
                                    if ( pCol2Item )
                                    {
                                        nCol2 = pCol2Item->GetValue();
                                        if ( nCol2 > 0 )
                                            --nCol2;
                                    }
                                    TextSelection aSel( TextPaM( nLine, nCol1 ), TextPaM( nLine, nCol2 ) );
                                    pTextView->SetSelection( aSel );
                                    pTextView->ShowCursor();
                                    vcl::Window* pWindow_ = pTextView->GetWindow();
                                    if ( pWindow_ )
                                        pWindow_->GrabFocus();
                                }
                            }
                        }
                    }
                }
            }
            rReq.Done();
        }
        break;

        case SID_BASICIDE_COLOR_SCHEME_DLG:
        {
            ModulWindowLayout* pMyLayout = dynamic_cast<ModulWindowLayout*>(pLayout.get());
            if (!pMyLayout)
                return;

            OUString curScheme = pMyLayout->GetActiveColorSchemeId();
            auto xDlg = std::make_shared<ColorSchemeDialog>(pCurWin ? pCurWin->GetFrameWeld() : nullptr,
                                                            pMyLayout);
            weld::DialogController::runAsync(xDlg, [xDlg, pMyLayout, curScheme](sal_Int32 nResult){
                OUString sNewScheme(xDlg->GetColorSchemeId());
                // If the user canceled the dialog, restores the original color scheme
                if (nResult != RET_OK)
                {
                    if (curScheme != sNewScheme)
                        pMyLayout->ApplyColorSchemeToCurrentWindow(curScheme);
                }

                // If the user selects OK, apply the color scheme to all open ModulWindow
                if (nResult == RET_OK)
                {
                    // Set the global color scheme in ModulWindowLayout and update definitions in SyntaxColors
                    pMyLayout->ApplyColorSchemeToCurrentWindow(sNewScheme);

                    // Update color scheme for all windows
                    for (auto const& window : GetShell()->GetWindowTable())
                    {
                        ModulWindow* pModuleWindow = dynamic_cast<ModulWindow*>(window.second.get());
                        if (pModuleWindow)
                        {
                            // We need to set the current scheme for each window
                            pModuleWindow->SetEditorColorScheme(sNewScheme);
                        }
                    }

                    // Update registry with the new color scheme ID
                    std::shared_ptr<comphelper::ConfigurationChanges> batch(comphelper::ConfigurationChanges::create());
                    officecfg::Office::BasicIDE::EditorSettings::ColorScheme::set(sNewScheme, batch);
                    batch->commit();
                }
            });
        }
        break;

        case SID_BASICIDE_MANAGE_LANG:
        {
            auto xRequest = std::make_shared<SfxRequest>(rReq);
            rReq.Ignore(); // the 'old' request is not relevant any more
            auto xDlg = std::make_shared<ManageLanguageDialog>(pCurWin ? pCurWin->GetFrameWeld() : nullptr, m_pCurLocalizationMgr);
            weld::DialogController::runAsync(xDlg, [xRequest=std::move(xRequest)](sal_Int32 /*nResult*/){
                    xRequest->Done();
                });
        }
        break;

        case SID_ATTR_ZOOMSLIDER:
        {
            const SfxItemSet *pArgs = rReq.GetArgs();
            const SfxPoolItem* pItem;

            if (pArgs && pArgs->GetItemState(SID_ATTR_ZOOMSLIDER, true, &pItem ) == SfxItemState::SET)
                SetGlobalEditorZoomLevel(static_cast<const SvxZoomSliderItem*>(pItem)->GetValue());

            lcl_InvalidateZoomSlots(GetBindingsPtr());
        }
        break;

        case SID_ZOOM_IN:
        case SID_ZOOM_OUT:
        {
            const sal_uInt16 nOldZoom = GetCurrentZoomSliderValue();
            sal_uInt16 nNewZoom;
            if (nSlot == SID_ZOOM_IN)
                nNewZoom = std::min<sal_uInt16>(GetMaxZoom(), basegfx::zoomtools::zoomIn(nOldZoom));
            else
                nNewZoom = std::max<sal_uInt16>(GetMinZoom(), basegfx::zoomtools::zoomOut(nOldZoom));
            SetGlobalEditorZoomLevel(nNewZoom);
            lcl_InvalidateZoomSlots(GetBindingsPtr());
        }
        break;

        default:
            if (pLayout)
                pLayout->ExecuteGlobal(rReq);
            if (pCurWin)
                pCurWin->ExecuteGlobal(rReq);
            break;
    }
}

void Shell::GetState(SfxItemSet &rSet)
{
    SfxWhichIter aIter(rSet);
    for ( sal_uInt16 nWh = aIter.FirstWhich(); nWh != 0; nWh = aIter.NextWhich() )
    {
        switch ( nWh )
        {
            case SID_NEWDOCDIRECT:
            {
                // we do not have a new document factory,
                // so just forward to a fallback method.
                SfxGetpApp()->GetSlotState(nWh, nullptr, &rSet);
            }
            break;
            case SID_DOCINFO:
            case SID_NEWWINDOW:
            case SID_SAVEASDOC:
            {
                rSet.DisableItem( nWh );
            }
            break;
            case SID_SAVEDOC:
            {
                bool bDisable = false;

                if ( pCurWin )
                {
                    if ( !pCurWin->IsModified() )
                    {
                        ScriptDocument aDocument( pCurWin->GetDocument() );
                        bDisable =  ( !aDocument.isAlive() )
                                ||  ( aDocument.isDocument() ? !aDocument.isDocumentModified() : !IsAppBasicModified() );
                    }
                }
                else
                {
                    bDisable = true;
                }

                if ( bDisable )
                    rSet.DisableItem( nWh );
            }
            break;
            case SID_SIGNATURE:
            {
                SignatureState nState = SignatureState::NOSIGNATURES;
                if ( pCurWin )
                {
                    DocumentSignature aSignature( pCurWin->GetDocument() );
                    nState = aSignature.getScriptingSignatureState();
                }
                rSet.Put( SfxUInt16Item( SID_SIGNATURE, static_cast<sal_uInt16>(nState) ) );
            }
            break;
            case SID_BASICIDE_MODULEDLG:
            {
                if ( StarBASIC::IsRunning() )
                    rSet.DisableItem( nWh );
            }
            break;

            case SID_BASICIDE_OBJCAT:
            {
                if (pLayout)
                    rSet.Put(SfxBoolItem(nWh, aObjectCatalog->IsVisible()));
                else
                    rSet.Put(SfxVisibilityItem(nWh, false));
            }
            break;

            case SID_BASICIDE_WATCH:
            {
                if (pLayout)
                {
                    rSet.Put(SfxBoolItem(nWh, pModulLayout->IsWatchWindowVisible()));
                    // Disable command if the visible window is not a ModulWindow
                    if (!dynamic_cast<ModulWindowLayout*>(pLayout.get()))
                        rSet.DisableItem(nWh);
                }
                else
                    rSet.Put(SfxVisibilityItem(nWh, false));
            }
            break;

            case SID_BASICIDE_STACK:
            {
                if (pLayout)
                {
                    rSet.Put(SfxBoolItem(nWh, pModulLayout->IsStackWindowVisible()));
                    // Disable command if the visible window is not a ModulWindow
                    if (!dynamic_cast<ModulWindowLayout*>(pLayout.get()))
                        rSet.DisableItem(nWh);
                }
                else
                    rSet.Put(SfxVisibilityItem(nWh, false));
            }
            break;

            case SID_BASICIDE_SHOWSBX:
            case SID_BASICIDE_CREATEMACRO:
            case SID_BASICIDE_EDITMACRO:
            case SID_BASICIDE_NAMECHANGEDONTAB:
            {
                ;
            }
            break;

            case SID_BASICIDE_ADDWATCH:
            case SID_BASICIDE_REMOVEWATCH:
            case SID_BASICLOAD:
            case SID_BASICSAVEAS:
            case SID_BASICIDE_MATCHGROUP:
            {
                if (!dynamic_cast<ModulWindow*>(pCurWin.get()))
                    rSet.DisableItem( nWh );
                else if ( ( nWh == SID_BASICLOAD ) && ( StarBASIC::IsRunning() || ( pCurWin && pCurWin->IsReadOnly() ) ) )
                    rSet.DisableItem( nWh );
            }
            break;
            case SID_BASICRUN:
            case SID_BASICSTEPINTO:
            case SID_BASICSTEPOVER:
            case SID_BASICSTEPOUT:
            case SID_BASICIDE_TOGGLEBRKPNT:
            case SID_BASICIDE_MANAGEBRKPNTS:
            {
                if (ModulWindow* pMCurWin = dynamic_cast<ModulWindow*>(pCurWin.get()))
                {
                    if (StarBASIC::IsRunning() && !pMCurWin->GetBasicStatus().bIsInReschedule)
                        rSet.DisableItem(nWh);
                }
                else
                    rSet.DisableItem( nWh );
            }
            break;
            case SID_BASICCOMPILE:
            {
                if (StarBASIC::IsRunning() || !dynamic_cast<ModulWindow*>(pCurWin.get()))
                    rSet.DisableItem( nWh );
            }
            break;
            case SID_BASICSTOP:
            {
                // stop is always possible when some Basic is running...
                if (!StarBASIC::IsRunning())
                    rSet.DisableItem( nWh );
            }
            break;
            case SID_CHOOSE_CONTROLS:
            case SID_DIALOG_TESTMODE:
            case SID_INSERT_SELECT:
            case SID_INSERT_PUSHBUTTON:
            case SID_INSERT_RADIOBUTTON:
            case SID_INSERT_CHECKBOX:
            case SID_INSERT_LISTBOX:
            case SID_INSERT_COMBOBOX:
            case SID_INSERT_GROUPBOX:
            case SID_INSERT_EDIT:
            case SID_INSERT_FIXEDTEXT:
            case SID_INSERT_IMAGECONTROL:
            case SID_INSERT_PROGRESSBAR:
            case SID_INSERT_HSCROLLBAR:
            case SID_INSERT_VSCROLLBAR:
            case SID_INSERT_HFIXEDLINE:
            case SID_INSERT_VFIXEDLINE:
            case SID_INSERT_DATEFIELD:
            case SID_INSERT_TIMEFIELD:
            case SID_INSERT_NUMERICFIELD:
            case SID_INSERT_CURRENCYFIELD:
            case SID_INSERT_FORMATTEDFIELD:
            case SID_INSERT_PATTERNFIELD:
            case SID_INSERT_FILECONTROL:
            case SID_INSERT_SPINBUTTON:
            case SID_INSERT_GRIDCONTROL:
            case SID_INSERT_HYPERLINKCONTROL:
            case SID_INSERT_TREECONTROL:
            case SID_INSERT_FORM_RADIO:
            case SID_INSERT_FORM_CHECK:
            case SID_INSERT_FORM_LIST:
            case SID_INSERT_FORM_COMBO:
            case SID_INSERT_FORM_VSCROLL:
            case SID_INSERT_FORM_HSCROLL:
            case SID_INSERT_FORM_SPIN:
            {
                if (!dynamic_cast<DialogWindow*>(pCurWin.get()))
                    rSet.DisableItem( nWh );
            }
            break;
            case SID_SEARCH_OPTIONS:
            {
                SearchOptionFlags nOptions = SearchOptionFlags::NONE;
                if( pCurWin )
                    nOptions = pCurWin->GetSearchOptions();
                rSet.Put( SfxUInt16Item( SID_SEARCH_OPTIONS, static_cast<sal_uInt16>(nOptions) ) );
            }
            break;
            case SID_BASICIDE_LIBSELECTOR:
            {
                OUString aName;
                if ( !m_aCurLibName.isEmpty() )
                {
                    LibraryLocation eLocation = m_aCurDocument.getLibraryLocation( m_aCurLibName );
                    aName = CreateMgrAndLibStr( m_aCurDocument.getTitle( eLocation ), m_aCurLibName );
                }
                SfxStringItem aItem( SID_BASICIDE_LIBSELECTOR, aName );
                rSet.Put( aItem );
            }
            break;
            case SID_SEARCH_ITEM:
            {
                if ( !mpSearchItem )
                {
                    mpSearchItem.reset( new SvxSearchItem( SID_SEARCH_ITEM ));
                    mpSearchItem->SetSearchString( GetSelectionText( true ));
                }

                if ( mbJustOpened && HasSelection() )
                {
                    OUString aText = GetSelectionText( true );

                    if ( !aText.isEmpty() )
                    {
                        mpSearchItem->SetSearchString( aText );
                        mpSearchItem->SetSelection( false );
                    }
                    else
                        mpSearchItem->SetSelection( true );
                }

                mbJustOpened = false;
                rSet.Put( *mpSearchItem );
            }
            break;
            case SID_BASICIDE_STAT_DATE:
            {
                SfxStringItem aItem( SID_BASICIDE_STAT_DATE, u"Datum?!"_ustr );
                rSet.Put( aItem );
            }
            break;
            case SID_DOC_MODIFIED:
            {
                bool bModified = false;

                if ( pCurWin )
                {
                    if ( pCurWin->IsModified() )
                        bModified = true;
                    else
                    {
                        ScriptDocument aDocument( pCurWin->GetDocument() );
                        bModified = aDocument.isDocument() ? aDocument.isDocumentModified() : IsAppBasicModified();
                    }
                }

                SfxBoolItem aItem(SID_DOC_MODIFIED, bModified);
                rSet.Put( aItem );
            }
            break;
            case SID_BASICIDE_STAT_TITLE:
            {
                if ( pCurWin )
                {
                    OUString aTitle = pCurWin->CreateQualifiedName();
                    if (pCurWin->IsReadOnly())
                        aTitle += " (" + IDEResId(RID_STR_READONLY) + ")";
                    SfxStringItem aItem( SID_BASICIDE_STAT_TITLE, aTitle );
                    rSet.Put( aItem );
                }
            }
            break;
            case SID_BASICIDE_CURRENT_ZOOM:
            {
                // The current zoom value is only visible in a module window
                ModulWindow* pModuleWindow = dynamic_cast<ModulWindow*>(pCurWin.get());
                if (pModuleWindow)
                {
                    OUString sZoom;
                    sZoom = OUString::number(m_nCurrentZoomSliderValue) + "%";
                    SfxStringItem aItem( SID_BASICIDE_CURRENT_ZOOM, sZoom );
                    rSet.Put( aItem );
                }
            }
            break;
            // are interpreted by the controller:
            case SID_ATTR_SIZE:
            case SID_ATTR_INSERT:
            break;
            case SID_UNDO:
            case SID_REDO:
            {
                if( GetUndoManager() )  // recursive GetState else
                    GetViewFrame().GetSlotState( nWh, nullptr, &rSet );
            }
            break;
            case SID_BASICIDE_CURRENT_LANG:
            {
                if( (pCurWin && pCurWin->IsReadOnly()) || GetCurLibName().isEmpty() )
                    rSet.DisableItem( nWh );
                else
                {
                    OUString aItemStr;
                    std::shared_ptr<LocalizationMgr> pCurMgr(GetCurLocalizationMgr());
                    if ( pCurMgr->isLibraryLocalized() )
                    {
                        Sequence< lang::Locale > aLocaleSeq = pCurMgr->getStringResourceManager()->getLocales();
                        sal_Int32 i, nCount = aLocaleSeq.getLength();

                        // Force different results for any combination of locales and default locale
                        OUString aLangStr;
                        for ( i = 0;  i <= nCount;  ++i )
                        {
                            lang::Locale aLocale;
                            if( i < nCount )
                                aLocale = aLocaleSeq[i];
                            else
                                aLocale = pCurMgr->getStringResourceManager()->getDefaultLocale();

                            aLangStr += aLocale.Language + aLocale.Country + aLocale.Variant;
                        }
                        aItemStr = aLangStr;
                    }
                    rSet.Put( SfxStringItem( nWh, aItemStr ) );
                }
            }
            break;

            case SID_BASICIDE_MANAGE_LANG:
            {
                if( (pCurWin && pCurWin->IsReadOnly()) || GetCurLibName().isEmpty() )
                    rSet.DisableItem( nWh );
            }
            break;
            case SID_TOGGLE_COMMENT:
            {
                // Only available in a ModulWindow if the document can be edited
                if (pCurWin && (!dynamic_cast<ModulWindow*>(pCurWin.get()) || pCurWin->IsReadOnly()))
                    rSet.DisableItem(nWh);
            }
            break;
            case SID_GOTOLINE:
            {
                // if this is not a module window hide the
                // setting, doesn't make sense for example if the
                // dialog editor is open
                if (pCurWin && !dynamic_cast<ModulWindow*>(pCurWin.get()))
                {
                    rSet.DisableItem( nWh );
                    rSet.Put(SfxVisibilityItem(nWh, false));
                }
                break;
            }
            case SID_BASICIDE_HIDECURPAGE:
            {
                if (pTabBar->GetPageCount() == 0)
                    rSet.DisableItem(nWh);
            }
            break;
            case SID_BASICIDE_DELETECURRENT:
            case SID_BASICIDE_RENAMECURRENT:
            {
                if (pTabBar->GetPageCount() == 0 || StarBASIC::IsRunning())
                    rSet.DisableItem(nWh);
                else if (m_aCurDocument.isInVBAMode())
                {
                    // disable to delete or rename object modules in IDE
                    BasicManager* pBasMgr = m_aCurDocument.getBasicManager();
                    StarBASIC* pBasic = pBasMgr ? pBasMgr->GetLib(m_aCurLibName) : nullptr;
                    if (pBasic && dynamic_cast<ModulWindow*>(pCurWin.get()))
                    {
                        SbModule* pActiveModule = pBasic->FindModule( pCurWin->GetName() );
                        if ( pActiveModule && ( pActiveModule->GetModuleType() == script::ModuleType::DOCUMENT ) )
                            rSet.DisableItem(nWh);
                    }
                }
            }
            [[fallthrough]];

            case SID_BASICIDE_NEWMODULE:
            case SID_BASICIDE_NEWDIALOG:
            {
                Reference< script::XLibraryContainer2 > xModLibContainer( m_aCurDocument.getLibraryContainer( E_SCRIPTS ) );
                Reference< script::XLibraryContainer2 > xDlgLibContainer( m_aCurDocument.getLibraryContainer( E_DIALOGS ) );
                if ( ( xModLibContainer.is() && xModLibContainer->hasByName( m_aCurLibName ) && xModLibContainer->isLibraryReadOnly( m_aCurLibName ) ) ||
                     ( xDlgLibContainer.is() && xDlgLibContainer->hasByName( m_aCurLibName ) && xDlgLibContainer->isLibraryReadOnly( m_aCurLibName ) ) )
                    rSet.DisableItem(nWh);
            }
            break;

            case SID_ZOOM_IN:
            case SID_ZOOM_OUT:
            {
                const sal_uInt16 nCurrentZoom = GetCurrentZoomSliderValue();
                if ((nWh == SID_ZOOM_IN && nCurrentZoom >= GetMaxZoom()) ||
                    (nWh == SID_ZOOM_OUT && nCurrentZoom <= GetMinZoom()))
                    rSet.DisableItem(nWh);
            }
            break;

            case SID_BASICIDE_COLOR_SCHEME_DLG:
            {
                if (!dynamic_cast<ModulWindowLayout*>(pLayout.get()))
                    rSet.DisableItem(nWh);
            }
            break;

            case SID_ATTR_ZOOMSLIDER:
            {
                // The zoom slider is only visible in a module window
                ModulWindow* pModuleWindow = dynamic_cast<ModulWindow*>(pCurWin.get());
                if (pModuleWindow)
                {
                    SvxZoomSliderItem aZoomSliderItem(GetCurrentZoomSliderValue(), GetMinZoom(), GetMaxZoom());
                    aZoomSliderItem.AddSnappingPoint(100);
                    rSet.Put( aZoomSliderItem );
                }
            }
            break;

            default:
                if (pLayout)
                    pLayout->GetState(rSet, nWh);
        }
    }
    if ( pCurWin )
        pCurWin->GetState( rSet );
}

bool Shell::HasUIFeature(SfxShellFeature nFeature) const
{
    assert((nFeature & ~SfxShellFeature::BasicMask) == SfxShellFeature::NONE);
    bool bResult = false;

    if (nFeature & SfxShellFeature::BasicShowBrowser)
    {
        // fade out (in) property browser in module (dialog) windows
        if (dynamic_cast<DialogWindow*>(pCurWin.get()) && !pCurWin->IsReadOnly())
            bResult = true;
    }

    return bResult;
}

void Shell::SetCurWindow( BaseWindow* pNewWin, bool bUpdateTabBar, bool bRememberAsCurrent )
{
    if ( pNewWin == pCurWin )
        return;

    pCurWin = pNewWin;
    if (pLayout)
        pLayout->Deactivating();
    if (pCurWin)
    {
        if (pCurWin->GetSbxType() == SBX_TYPE_MODULE)
            pLayout = pModulLayout.get();
        else
            pLayout = pDialogLayout.get();
        AdjustPosSizePixel(Point(0, 0), GetViewFrame().GetWindow().GetOutputSizePixel());
        pLayout->Activating(*pCurWin);
        GetViewFrame().GetWindow().SetHelpId(pCurWin->GetHid());
        if (bRememberAsCurrent)
            pCurWin->InsertLibInfo();
        if (GetViewFrame().GetWindow().IsVisible()) // SFX will do it later otherwise
            pCurWin->Show();
        pCurWin->Init();
        if (!GetExtraData()->ShellInCriticalSection())
        {
            vcl::Window* pFrameWindow = &GetViewFrame().GetWindow();
            vcl::Window* pFocusWindow = Application::GetFocusWindow();
            while ( pFocusWindow && ( pFocusWindow != pFrameWindow ) )
                pFocusWindow = pFocusWindow->GetParent();
            if ( pFocusWindow ) // Focus in BasicIDE
                pCurWin->GrabFocus();
        }
    }
    else
    {
        SetWindow(pLayout);
        pLayout = nullptr;
    }
    if ( bUpdateTabBar )
    {
        sal_uInt16 nKey = GetWindowId( pCurWin );
        if ( pCurWin && ( pTabBar->GetPagePos( nKey ) == TabBar::PAGE_NOT_FOUND ) )
            pTabBar->InsertPage( nKey, pCurWin->GetTitle() );   // has just been faded in
        pTabBar->SetCurPageId( nKey );
    }
    if ( pCurWin && pCurWin->IsSuspended() )    // if the window is shown in the case of an error...
        pCurWin->SetStatus( pCurWin->GetStatus() & ~BASWIN_SUSPENDED );
    if ( pCurWin )
    {
        SetWindow( pCurWin );
        if ( pCurWin->GetDocument().isDocument() )
            SfxObjectShell::SetCurrentComponent( pCurWin->GetDocument().getDocument() );
    }
    else if (pLayout)
    {
        SetWindow(pLayout);
        GetViewFrame().GetWindow().SetHelpId( HID_BASICIDE_MODULWINDOW );
        SfxObjectShell::SetCurrentComponent(nullptr);
    }
    aObjectCatalog->SetCurrentEntry(pCurWin);
    SetUndoManager( pCurWin ? pCurWin->GetUndoManager() : nullptr );
    InvalidateBasicIDESlots();
    InvalidateControlSlots();

    if ( m_pCurLocalizationMgr )
        m_pCurLocalizationMgr->handleTranslationbar();

    ManageToolbars();

    // fade out (in) property browser in module (dialog) windows
    UIFeatureChanged();
}

void Shell::ManageToolbars()
{
    static constexpr OUString aMacroBarResName = u"private:resource/toolbar/macrobar"_ustr;
    static constexpr OUString aDialogBarResName = u"private:resource/toolbar/dialogbar"_ustr;
    static constexpr OUString aInsertControlsBarResName
        = u"private:resource/toolbar/insertcontrolsbar"_ustr;
    static constexpr OUString aFormControlsBarResName
        = u"private:resource/toolbar/formcontrolsbar"_ustr;

    if( !pCurWin )
        return;

    Reference< beans::XPropertySet > xFrameProps
        ( GetViewFrame().GetFrame().GetFrameInterface(), uno::UNO_QUERY );
    if ( !xFrameProps.is() )
        return;

    Reference< css::frame::XLayoutManager > xLayoutManager;
    uno::Any a = xFrameProps->getPropertyValue( u"LayoutManager"_ustr );
    a >>= xLayoutManager;
    if ( !xLayoutManager.is() )
        return;

    xLayoutManager->lock();
    if (dynamic_cast<DialogWindow*>(pCurWin.get()))
    {
        xLayoutManager->destroyElement( aMacroBarResName );

        xLayoutManager->requestElement( aDialogBarResName );
        xLayoutManager->requestElement( aInsertControlsBarResName );
        xLayoutManager->requestElement( aFormControlsBarResName );
    }
    else
    {
        xLayoutManager->destroyElement( aDialogBarResName );
        xLayoutManager->destroyElement( aInsertControlsBarResName );
        xLayoutManager->destroyElement( aFormControlsBarResName );

        xLayoutManager->requestElement( aMacroBarResName );
    }
    xLayoutManager->unlock();
}

VclPtr<BaseWindow> Shell::FindApplicationWindow()
{
    return FindWindow( ScriptDocument::getApplicationScriptDocument(), u"", u"", SBX_TYPE_UNKNOWN );
}

VclPtr<BaseWindow> Shell::FindWindow(
    ScriptDocument const& rDocument,
    std::u16string_view rLibName, std::u16string_view rName,
    SbxItemType eSbxItemType, bool bFindSuspended
)
{
    for (auto const& window : aWindowTable)
    {
        BaseWindow* const pWin = window.second;
        if (pWin->Is(rDocument, rLibName, rName, eSbxItemType, bFindSuspended))
            return pWin;
    }
    return nullptr;
}

bool Shell::CallBasicErrorHdl( StarBASIC const * pBasic )
{
    VclPtr<ModulWindow> pModWin = ShowActiveModuleWindow( pBasic );
    if ( pModWin )
        pModWin->BasicErrorHdl( pBasic );
    return false;
}

BasicDebugFlags Shell::CallBasicBreakHdl( StarBASIC const * pBasic )
{
    BasicDebugFlags nRet = BasicDebugFlags::NONE;
    VclPtr<ModulWindow> pModWin = ShowActiveModuleWindow( pBasic );
    if ( pModWin )
    {
        bool bAppWindowDisabled, bDispatcherLocked;
        sal_uInt16 nWaitCount;
        SfxUInt16Item *pSWActionCount, *pSWLockViewCount;
        BasicStopped( &bAppWindowDisabled, &bDispatcherLocked,
                                &nWaitCount, &pSWActionCount, &pSWLockViewCount );

        nRet = pModWin->BasicBreakHdl();

        if ( StarBASIC::IsRunning() )   // if cancelled...
        {
            if ( bAppWindowDisabled )
                Application::GetDefDialogParent()->set_sensitive(false);

            if ( nWaitCount )
            {
                Shell* pShell = GetShell();
                for ( sal_uInt16 n = 0; n < nWaitCount; n++ )
                    pShell->GetViewFrame().GetWindow().EnterWait();
            }
        }
    }
    return nRet;
}

VclPtr<ModulWindow> Shell::ShowActiveModuleWindow( StarBASIC const * pBasic )
{
    SetCurLib( ScriptDocument::getApplicationScriptDocument(), OUString(), false );

    SbModule* pActiveModule = StarBASIC::GetActiveModule();
    if (SbClassModuleObject* pCMO = dynamic_cast<SbClassModuleObject*>(pActiveModule))
        pActiveModule = &pCMO->getClassModule();

    DBG_ASSERT( pActiveModule, "No active module in ErrorHdl!?" );
    if ( pActiveModule )
    {
        VclPtr<ModulWindow> pWin;
        SbxObject* pParent = pActiveModule->GetParent();
        if (StarBASIC* pLib = dynamic_cast<StarBASIC*>(pParent))
        {
            if (BasicManager* pBasMgr = FindBasicManager(pLib))
            {
                ScriptDocument aDocument( ScriptDocument::getDocumentForBasicManager( pBasMgr ) );
                const OUString& aLibName = pLib->GetName();
                pWin = FindBasWin( aDocument, aLibName, pActiveModule->GetName(), true );
                DBG_ASSERT( pWin, "Error/Step-Hdl: Window was not created/found!" );
                SetCurLib( aDocument, aLibName );
                SetCurWindow( pWin, true );
            }
        }
        else
            SAL_WARN( "basctl.basicide", "No BASIC!");
        if (BasicManager* pBasicMgr = FindBasicManager(pBasic))
            StartListening(*pBasicMgr, DuplicateHandling::Prevent /* log on only once */);
        return pWin;
    }
    return nullptr;
}

void Shell::AdjustPosSizePixel( const Point &rPos, const Size &rSize )
{
    // not if iconified because the whole text would be displaced then at restore
    if ( GetViewFrame().GetWindow().GetOutputSizePixel().Height() == 0 )
        return;

    Size aTabBarSize;
    aTabBarSize.setHeight( GetViewFrame().GetWindow().GetFont().GetFontHeight() + TAB_HEIGHT_MARGIN );
    aTabBarSize.setWidth( rSize.Width() );

    Size aSz( rSize );
    auto nScrollBarSz(Application::GetSettings().GetStyleSettings().GetScrollBarSize());
    aSz.AdjustHeight(-aTabBarSize.Height());

    Size aOutSz( aSz );
    aSz.AdjustWidth(-nScrollBarSz);
    aSz.AdjustHeight(-nScrollBarSz);
    aVScrollBar->SetPosSizePixel( Point( rPos.X()+aSz.Width(), rPos.Y() ), Size( nScrollBarSz, aSz.Height() ) );
    aHScrollBar->SetPosSizePixel( Point( rPos.X(), rPos.Y()+aSz.Height() ), Size( aOutSz.Width(), nScrollBarSz ) );
    pTabBar->SetPosSizePixel( Point( rPos.X(), rPos.Y() + nScrollBarSz + aSz.Height()), aTabBarSize );

    // The size to be applied depends on whether it is a DialogWindow or a ModulWindow
    if (pLayout)
    {
        if (dynamic_cast<DialogWindow*>(pCurWin.get()))
        {
            pCurWin->ShowShellScrollBars();
            pLayout->SetPosSizePixel(rPos, aSz);
        }
        else
        {
            pCurWin->ShowShellScrollBars(false);
            pLayout->SetPosSizePixel(rPos, aOutSz);
        }
    }
}

Reference< XModel > Shell::GetCurrentDocument() const
{
    Reference< XModel > xDocument;
    if ( pCurWin && pCurWin->GetDocument().isDocument() )
        xDocument = pCurWin->GetDocument().getDocument();
    return xDocument;
}

void Shell::Activate( bool bMDI )
{
    SfxViewShell::Activate( bMDI );

    if ( bMDI )
    {
        if (DialogWindow* pDCurWin = dynamic_cast<DialogWindow*>(pCurWin.get()))
            pDCurWin->UpdateBrowser();
    }
}

void Shell::Deactivate( bool bMDI )
{
    // bMDI == true means that another MDI has been activated; in case of a
    // deactivate due to a MessageBox bMDI is false
    if ( bMDI )
    {
        if (DialogWindow* pXDlgWin = dynamic_cast<DialogWindow*>(pCurWin.get()))
        {
            pXDlgWin->DisableBrowser();
            if( pXDlgWin->IsModified() )
                MarkDocumentModified( pXDlgWin->GetDocument() );
        }
    }
}

} // namespace basctl

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
