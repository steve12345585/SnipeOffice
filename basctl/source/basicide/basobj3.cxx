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

#include <vcl/errinf.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <basic/basmgr.hxx>
#include <basic/sbmeth.hxx>
#include <unotools/moduleoptions.hxx>

#include <iderdll.hxx>
#include "iderdll2.hxx"
#include "basdoc.hxx"
#include <iderid.hxx>
#include <strings.hrc>

#include <baside3.hxx>
#include <basidesh.hxx>
#include <basobj.hxx>
#include <localizationmgr.hxx>
#include <dlged.hxx>
#include <com/sun/star/script/XLibraryContainerPassword.hpp>
#include <sfx2/app.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/sfxsids.hrc>
#include <sfx2/request.hxx>
#include <sfx2/viewfrm.hxx>
#include <sal/log.hxx>
#include <osl/diagnose.h>
#include <tools/debug.hxx>

namespace basctl
{

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::container;

extern "C" {
    SAL_DLLPUBLIC_EXPORT tools::Long basicide_handle_basic_error( void const * pPtr )
    {
        return HandleBasicError( static_cast<StarBASIC const *>(pPtr) );
    }
}

SbMethod* CreateMacro( SbModule* pModule, const OUString& rMacroName )
{
    SfxDispatcher* pDispatcher = GetDispatcher();
    if( pDispatcher )
    {
        pDispatcher->Execute( SID_BASICIDE_STOREALLMODULESOURCES );
    }

    if ( pModule->FindMethod( rMacroName, SbxClassType::Method ) )
        return nullptr;

    OUString aMacroName( rMacroName );
    if ( aMacroName.isEmpty() )
    {
        if (!pModule->GetMethods()->Count())
            aMacroName = "Main" ;
        else
        {
            bool bValid = false;
            sal_Int32 nMacro = 1;
            while ( !bValid )
            {
                aMacroName = "Macro" + OUString::number( nMacro );
                // test whether existing...
                bValid = pModule->FindMethod( aMacroName, SbxClassType::Method ) == nullptr;
                nMacro++;
            }
        }
    }

    OUString aOUSource( pModule->GetSource32() );

    // don't produce too many empty lines...
    sal_Int32 nSourceLen = aOUSource.getLength();
    if ( nSourceLen > 2 )
    {
        const sal_Unicode* pStr = aOUSource.getStr();
        if ( pStr[ nSourceLen - 1 ]  != LINE_SEP )
            aOUSource += "\n\n" ;
        else if ( pStr[ nSourceLen - 2 ] != LINE_SEP )
            aOUSource += "\n" ;
        else if ( pStr[ nSourceLen - 3 ] == LINE_SEP )
            aOUSource = aOUSource.copy( 0, nSourceLen-1 );
    }

    aOUSource += "Sub " + aMacroName + "\n\nEnd Sub";

    // update module in library
    StarBASIC* pBasic = dynamic_cast<StarBASIC*>(pModule->GetParent());
    BasicManager* pBasMgr = pBasic ? FindBasicManager(pBasic) : nullptr;
    SAL_WARN_IF(!pBasMgr, "basctl.basicide", "No BasicManager found!");
    ScriptDocument aDocument = pBasMgr
        ? ScriptDocument::getDocumentForBasicManager(pBasMgr)
        : ScriptDocument(ScriptDocument::NoDocument);

    if (aDocument.isValid())
    {
        assert(pBasic && "isValid cannot be false with !pBasic");
        const OUString& aLibName = pBasic->GetName();
        const OUString& aModName = pModule->GetName();
        OSL_VERIFY( aDocument.updateModule( aLibName, aModName, aOUSource ) );
    }

    SbMethod* pMethod = pModule->FindMethod( aMacroName, SbxClassType::Method );

    if( pDispatcher )
    {
        pDispatcher->Execute( SID_BASICIDE_UPDATEALLMODULESOURCES );
    }

    if (aDocument.isAlive())
        MarkDocumentModified(aDocument);

    return pMethod;
}

bool RenameDialog (
    weld::Widget* pErrorParent,
    ScriptDocument const& rDocument,
    OUString const& rLibName,
    OUString const& rOldName,
    OUString const& rNewName
)
{
    if ( !rDocument.hasDialog( rLibName, rOldName ) )
    {
        OSL_FAIL( "basctl::RenameDialog: old module name is invalid!" );
        return false;
    }

    if ( rDocument.hasDialog( rLibName, rNewName ) )
    {
        std::unique_ptr<weld::MessageDialog> xError(Application::CreateMessageDialog(pErrorParent,
                                                    VclMessageType::Warning, VclButtonsType::Ok, IDEResId(RID_STR_SBXNAMEALLREADYUSED2)));
        xError->run();
        return false;
    }

    // #i74440
    if ( rNewName.isEmpty() )
    {
        std::unique_ptr<weld::MessageDialog> xError(Application::CreateMessageDialog(pErrorParent,
                                                    VclMessageType::Warning, VclButtonsType::Ok, IDEResId(RID_STR_BADSBXNAME)));
        xError->run();
        return false;
    }

    Shell* pShell = GetShell();
    VclPtr<DialogWindow> pWin = pShell ? pShell->FindDlgWin(rDocument, rLibName, rOldName) : nullptr;
    Reference< XNameContainer > xExistingDialog;
    if ( pWin )
        xExistingDialog = pWin->GetEditor().GetDialog();

    if ( xExistingDialog.is() )
        LocalizationMgr::renameStringResourceIDs( rDocument, rLibName, rNewName, xExistingDialog );

    if ( !rDocument.renameDialog( rLibName, rOldName, rNewName, xExistingDialog ) )
        return false;

    if (!pWin || !pShell)
        return true;

    // set new name in window
    pWin->SetName( rNewName );

    // update property browser
    pWin->UpdateBrowser();

    // update tabwriter
    sal_uInt16 nId = pShell->GetWindowId( pWin );
    DBG_ASSERT( nId, "No entry in Tabbar!" );
    if ( nId )
    {
        TabBar& rTabBar = pShell->GetTabBar();
        rTabBar.SetPageText( nId, rNewName );
        rTabBar.Sort();
        rTabBar.MakeVisible( rTabBar.GetCurPageId() );
    }
    return true;
}

bool RemoveDialog( const ScriptDocument& rDocument, const OUString& rLibName, const OUString& rDlgName )
{
    if (Shell* pShell = GetShell())
    {
        if (VclPtr<DialogWindow> pDlgWin = pShell->FindDlgWin(rDocument, rLibName, rDlgName))
        {
            Reference< container::XNameContainer > xDialogModel = pDlgWin->GetDialog();
            LocalizationMgr::removeResourceForDialog( rDocument, rLibName, rDlgName, xDialogModel );
        }
    }

    return rDocument.removeDialog( rLibName, rDlgName );
}

StarBASIC* FindBasic( const SbxVariable* pVar )
{
    SbxVariable const* pSbx = pVar;
    while (pSbx && !dynamic_cast<StarBASIC const*>(pSbx))
        pSbx = pSbx->GetParent();
    return const_cast<StarBASIC*>(static_cast<const StarBASIC*>(pSbx));
}

BasicManager* FindBasicManager( StarBASIC const * pLib )
{
    ScriptDocuments aDocuments( ScriptDocument::getAllScriptDocuments( ScriptDocument::AllWithApplication ) );
    for (auto const& doc : aDocuments)
    {
        BasicManager* pBasicMgr = doc.getBasicManager();
        OSL_ENSURE( pBasicMgr, "basctl::FindBasicManager: no basic manager for the document!" );
        if ( !pBasicMgr )
            continue;

        for (auto& rLibName : doc.getLibraryNames())
        {
            StarBASIC* pL = pBasicMgr->GetLib(rLibName);
            if ( pL == pLib )
                return pBasicMgr;
        }
    }
    return nullptr;
}

void MarkDocumentModified( const ScriptDocument& rDocument )
{
    SfxGetpApp()->Broadcast(SfxHint(SfxHintId::ScriptDocumentChanged));

    Shell* pShell = GetShell();

    // does not have to come from a document...
    if ( rDocument.isApplication() )
    {
        if (pShell)
            pShell->SetAppBasicModified(true);
    }
    else
    {
        rDocument.setDocumentModified();
    }

    // tdf#130161 in all cases call UpdateObjectCatalog
    if (pShell)
        pShell->UpdateObjectCatalog();

    if (SfxBindings* pBindings = GetBindingsPtr())
    {
        pBindings->Invalidate( SID_SIGNATURE );
        pBindings->Invalidate( SID_SAVEDOC );
        pBindings->Update( SID_SAVEDOC );
    }
}

void RunMethod( SbMethod const * pMethod )
{
    SbxValues aRes;
    aRes.eType = SbxVOID;
    pMethod->Get( aRes );
}

void StopBasic()
{
    StarBASIC::Stop();
    if (Shell* pShell = GetShell())
    {
        Shell::WindowTable& rWindows = pShell->GetWindowTable();
        for (auto const& window : rWindows)
        {
            BaseWindow* pWin = window.second;
            // call BasicStopped manually because the Stop-Notify
            // might not get through otherwise
            pWin->BasicStopped();
        }
    }
    BasicStopped();
}

void BasicStopped(
    bool* pbAppWindowDisabled,
    bool* pbDispatcherLocked,
    sal_uInt16* pnWaitCount,
    SfxUInt16Item** ppSWActionCount, SfxUInt16Item** ppSWLockViewCount
)
{
    // maybe there are some locks to be removed after an error
    // or an explicit cancelling of the basic...
    if ( pbAppWindowDisabled )
        *pbAppWindowDisabled = false;
    if ( pbDispatcherLocked )
        *pbDispatcherLocked = false;
    if ( pnWaitCount )
        *pnWaitCount = 0;
    if ( ppSWActionCount )
        *ppSWActionCount = nullptr;
    if ( ppSWLockViewCount )
        *ppSWLockViewCount = nullptr;

    // AppWait?
    if (Shell* pShell = GetShell())
    {
        sal_uInt16 nWait = 0;
        while ( pShell->GetViewFrame().GetWindow().IsWait() )
        {
            pShell->GetViewFrame().GetWindow().LeaveWait();
            nWait++;
        }
        if ( pnWaitCount )
            *pnWaitCount = nWait;
    }

    weld::Window* pDefParent = Application::GetDefDialogParent();
    if (pDefParent && !pDefParent->get_sensitive())
    {
        pDefParent->set_sensitive(true);
        if ( pbAppWindowDisabled )
            *pbAppWindowDisabled = true;
    }

}

void InvalidateDebuggerSlots()
{
    SfxBindings* pBindings = GetBindingsPtr();
    if (!pBindings)
        return;

    pBindings->Invalidate( SID_BASICSTOP );
    pBindings->Update( SID_BASICSTOP );
    pBindings->Invalidate( SID_BASICRUN );
    pBindings->Update( SID_BASICRUN );
    pBindings->Invalidate( SID_BASICCOMPILE );
    pBindings->Update( SID_BASICCOMPILE );
    pBindings->Invalidate( SID_BASICSTEPOVER );
    pBindings->Update( SID_BASICSTEPOVER );
    pBindings->Invalidate( SID_BASICSTEPINTO );
    pBindings->Update( SID_BASICSTEPINTO );
    pBindings->Invalidate( SID_BASICSTEPOUT );
    pBindings->Update( SID_BASICSTEPOUT );
    pBindings->Invalidate( SID_BASICIDE_TOGGLEBRKPNT );
    pBindings->Update( SID_BASICIDE_TOGGLEBRKPNT );
    pBindings->Invalidate( SID_BASICIDE_STAT_POS );
    pBindings->Update( SID_BASICIDE_STAT_POS );
    pBindings->Invalidate( SID_BASICIDE_STAT_TITLE );
    pBindings->Update( SID_BASICIDE_STAT_TITLE );
}

tools::Long HandleBasicError( StarBASIC const * pBasic )
{
    EnsureIde();
    BasicStopped();

    // no error output during macro choosing
    if (GetExtraData()->ChoosingMacro())
        return 1;
    if (GetExtraData()->ShellInCriticalSection())
        return 2;

    tools::Long nRet = 0;
    Shell* pShell = nullptr;
    if (SvtModuleOptions::IsBasicIDEInstalled())
    {
        BasicManager* pBasMgr = FindBasicManager( pBasic );
        if ( pBasMgr )
        {
            bool bProtected = false;
            ScriptDocument aDocument( ScriptDocument::getDocumentForBasicManager( pBasMgr ) );
            OSL_ENSURE( aDocument.isValid(), "basctl::HandleBasicError: no document for the given BasicManager!" );
            if ( aDocument.isValid() )
            {
                const OUString& aOULibName( pBasic->GetName() );
                Reference< script::XLibraryContainer > xModLibContainer( aDocument.getLibraryContainer( E_SCRIPTS ) );
                if ( xModLibContainer.is() && xModLibContainer->hasByName( aOULibName ) )
                {
                    Reference< script::XLibraryContainerPassword > xPasswd( xModLibContainer, UNO_QUERY );
                    if ( xPasswd.is() && xPasswd->isLibraryPasswordProtected( aOULibName ) && !xPasswd->isLibraryPasswordVerified( aOULibName ) )
                    {
                        bProtected = true;
                    }
                }
            }

            if ( !bProtected )
            {
                pShell = GetShell();
                if ( !pShell )
                {
                    SfxAllItemSet aArgs( SfxGetpApp()->GetPool() );
                    SfxRequest aRequest( SID_BASICIDE_APPEAR, SfxCallMode::SYNCHRON, aArgs );
                    SfxGetpApp()->ExecuteSlot( aRequest );
                    pShell = GetShell();
                }
            }
        }
    }

    if ( pShell )
        nRet = tools::Long(pShell->CallBasicErrorHdl( pBasic ));
    else
        ErrorHandler::HandleError( StarBASIC::GetErrorCode() );

    return nRet;
}

SfxBindings* GetBindingsPtr()
{
    SfxBindings* pBindings = nullptr;

    SfxViewFrame* pFrame = nullptr;
    if (Shell* pShell = GetShell())
    {
        pFrame = &pShell->GetViewFrame();
    }
    else
    {
        SfxViewFrame* pView = SfxViewFrame::GetFirst();
        while ( pView )
        {
            if (dynamic_cast<DocShell*>(pView->GetObjectShell()))
            {
                pFrame = pView;
                break;
            }
            pView = SfxViewFrame::GetNext( *pView );
        }
    }
    if ( pFrame != nullptr )
        pBindings = &pFrame->GetBindings();

    return pBindings;
}

SfxDispatcher* GetDispatcher ()
{
    if (Shell* pShell = GetShell())
    {
        SfxViewFrame& rViewFrame = pShell->GetViewFrame();
        if (SfxDispatcher* pDispatcher = rViewFrame.GetDispatcher())
            return pDispatcher;
    }
    return nullptr;
}
} // namespace basctl

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
