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

#include <config_features.h>
#include <config_fuzzers.h>

#include <map>

#include <cppuhelper/implbase.hxx>
#include <cppuhelper/weakref.hxx>

#include <com/sun/star/util/XCloseable.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/util/CloseVetoException.hpp>
#include <com/sun/star/util/XCloseListener.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/frame/XTitle.hpp>
#include <osl/file.hxx>
#include <sal/log.hxx>
#include <vcl/weld.hxx>
#include <vcl/svapp.hxx>
#include <svl/eitem.hxx>
#include <basic/sbstar.hxx>
#include <svl/stritem.hxx>
#include <unotools/configmgr.hxx>
#include <unotools/eventcfg.hxx>

#include <sfx2/objsh.hxx>
#include <sfx2/signaturestate.hxx>
#include <sfx2/sfxmodelfactory.hxx>

#include <comphelper/configuration.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/servicehelper.hxx>

#include <com/sun/star/document/XStorageBasedDocument.hpp>
#include <com/sun/star/script/DocumentDialogLibraryContainer.hpp>
#include <com/sun/star/script/DocumentScriptLibraryContainer.hpp>
#include <com/sun/star/document/XEmbeddedScripts.hpp>
#include <com/sun/star/document/XScriptInvocationContext.hpp>
#include <com/sun/star/ucb/ContentCreationException.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>

#include <unotools/ucbhelper.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <tools/globname.hxx>
#include <tools/debug.hxx>

#include <sfx2/app.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/event.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/sfxresid.hxx>
#include <objshimp.hxx>
#include <sfx2/strings.hrc>
#include <sfx2/sfxsids.hrc>
#include <basic/basmgr.hxx>
#include <sfx2/QuerySaveDocument.hxx>
#include <appbaslib.hxx>
#include <sfx2/sfxbasemodel.hxx>
#include <sfx2/sfxuno.hxx>
#include <sfx2/notebookbar/SfxNotebookBar.hxx>
#include <sfx2/infobar.hxx>
#include <svtools/svparser.hxx>

#include <basic/basicmanagerrepository.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::script;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::document;

using ::basic::BasicManagerRepository;

namespace {

WeakReference< XInterface > theCurrentComponent;

#if HAVE_FEATURE_SCRIPTING

// remember all registered components for VBA compatibility, to be able to remove them on disposing the model
typedef ::std::map< XInterface*, OUString > VBAConstantNameMap;
VBAConstantNameMap s_aRegisteredVBAConstants;

OUString lclGetVBAGlobalConstName( const Reference< XInterface >& rxComponent )
{
    OSL_ENSURE( rxComponent.is(), "lclGetVBAGlobalConstName - missing component" );

    VBAConstantNameMap::iterator aIt = s_aRegisteredVBAConstants.find( rxComponent.get() );
    if( aIt != s_aRegisteredVBAConstants.end() )
        return aIt->second;

    uno::Reference< beans::XPropertySet > xProps( rxComponent, uno::UNO_QUERY );
    if( xProps.is() ) try
    {
        OUString aConstName;
        xProps->getPropertyValue(u"VBAGlobalConstantName"_ustr) >>= aConstName;
        return aConstName;
    }
    catch (const uno::Exception&) // not supported
    {
    }
    return OUString();
}

#endif

class SfxModelListener_Impl : public ::cppu::WeakImplHelper< css::util::XCloseListener >
{
    SfxObjectShell* mpDoc;
public:
    explicit SfxModelListener_Impl( SfxObjectShell* pDoc ) : mpDoc(pDoc) {};
    virtual void SAL_CALL queryClosing( const css::lang::EventObject& aEvent, sal_Bool bDeliverOwnership ) override ;
    virtual void SAL_CALL notifyClosing( const css::lang::EventObject& aEvent ) override ;
    virtual void SAL_CALL disposing( const css::lang::EventObject& aEvent ) override ;

};

} // namespace

void SAL_CALL SfxModelListener_Impl::queryClosing( const css::lang::EventObject& , sal_Bool bDeliverOwnership)
{
    if (mpDoc->Get_Impl()->m_nClosingLockLevel)
    {
        if (bDeliverOwnership)
            mpDoc->Get_Impl()->m_bCloseModelScheduled = true;
        throw util::CloseVetoException(u"Closing document is blocked"_ustr, getXWeak());
    }
}

void SAL_CALL SfxModelListener_Impl::notifyClosing( const css::lang::EventObject& )
{
    SolarMutexGuard aSolarGuard;
    mpDoc->Broadcast( SfxHint(SfxHintId::Deinitializing) );
}

void SAL_CALL SfxModelListener_Impl::disposing( const css::lang::EventObject& _rEvent )
{
    // am I ThisComponent in AppBasic?
    SolarMutexGuard aSolarGuard;
    if ( SfxObjectShell::GetCurrentComponent() == _rEvent.Source )
    {
        // remove ThisComponent reference from AppBasic
        SfxObjectShell::SetCurrentComponent( Reference< XInterface >() );
    }

#if HAVE_FEATURE_SCRIPTING
    /*  Remove VBA component from AppBasic. As every application registers its
        own current component, the disposed component may not be the "current
        component" of the SfxObjectShell. */
    if ( _rEvent.Source.is() )
    {
        VBAConstantNameMap::iterator aIt = s_aRegisteredVBAConstants.find( _rEvent.Source.get() );
        if ( aIt != s_aRegisteredVBAConstants.end() )
        {
            if ( BasicManager* pAppMgr = SfxApplication::GetBasicManager() )
                pAppMgr->SetGlobalUNOConstant( aIt->second, Any( Reference< XInterface >() ) );
            s_aRegisteredVBAConstants.erase( aIt );
        }
    }
#endif

    if ( !mpDoc->Get_Impl()->bClosing )
        // GCC crashes when already in the destructor, so first query the Flag
        mpDoc->DoClose();
}


SfxObjectShell_Impl::SfxObjectShell_Impl( SfxObjectShell& _rDocShell )
    :rDocShell( _rDocShell )
    ,aMacroMode( *this )
    ,pProgress( nullptr)
    ,nTime( DateTime::SYSTEM )
    ,nVisualDocumentNumber( USHRT_MAX)
    ,nDocumentSignatureState( SignatureState::UNKNOWN )
    ,nScriptingSignatureState( SignatureState::UNKNOWN )
    ,bClosing( false)
    ,bIsSaving( false)
    ,bIsNamedVisible( false)
    ,bIsAbortingImport ( false)
    ,bInPrepareClose( false )
    ,bPreparedForClose( false )
    ,bForbidReload( false )
    ,bBasicInitialized( false )
    ,bIsPrintJobCancelable( true )
    ,bOwnsStorage( true )
    ,bInitialized( false )
    ,bModelInitialized( false )
    ,bPreserveVersions( true )
    ,m_bMacroSignBroken( false )
    ,m_bNoBasicCapabilities( false )
    ,m_bDocRecoverySupport( true )
    ,bQueryLoadTemplate( true )
    ,bLoadReadonly( false )
    ,bUseUserData( true )
    ,bUseThumbnailSave( true )
    ,bSaveVersionOnClose( false )
    ,m_bSharedXMLFlag( false )
    ,m_bAllowShareControlFileClean( true )
    ,m_bConfigOptionsChecked( false )
    ,m_bMacroCallsSeenWhileLoading( false )
    ,m_bHadCheckedMacrosOnLoad( false )
    ,lErr(ERRCODE_NONE)
    ,nEventId ( SfxEventHintId::NONE )
    ,nLoadedFlags ( SfxLoadedFlags::ALL )
    ,nFlagsInProgress( SfxLoadedFlags::NONE )
    ,bModalMode( false )
    ,bRunningMacro( false )
    ,bReadOnlyUI( false )
    ,nStyleFilter( 0 )
    ,m_bEnableSetModified( true )
    ,m_bIsModified( false )
    ,m_nMapUnit( MapUnit::Map100thMM )
    ,m_bCreateTempStor( false )
    ,m_bIsInit( false )
    ,m_bIncomplEncrWarnShown( false )
    ,m_nModifyPasswordHash( 0 )
    ,m_bModifyPasswordEntered( false )
    ,m_bSavingForSigning( false )
    ,m_bAllowModifiedBackAfterSigning( false )
{
    SfxObjectShell* pDoc = &_rDocShell;
    std::vector<SfxObjectShell*> &rArr = SfxGetpApp()->GetObjectShells_Impl();
    rArr.push_back( pDoc );
}


SfxObjectShell_Impl::~SfxObjectShell_Impl()
{
}


SfxObjectShell::SfxObjectShell( const SfxModelFlags i_nCreationFlags )
    : pImpl(new SfxObjectShell_Impl(*this))
    , pMedium(nullptr)
    , eCreateMode(SfxObjectCreateMode::STANDARD)
    , bHasName(false)
    , bIsInGenerateThumbnail (false)
    , mbAvoidRecentDocs(false)
    , bRememberSignature(false)
{
    if (i_nCreationFlags & SfxModelFlags::EMBEDDED_OBJECT)
        eCreateMode = SfxObjectCreateMode::EMBEDDED;
    else if (i_nCreationFlags & SfxModelFlags::EXTERNAL_LINK)
        eCreateMode = SfxObjectCreateMode::INTERNAL;

    const bool bScriptSupport = ( i_nCreationFlags & SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS ) == SfxModelFlags::NONE;
    if ( !bScriptSupport )
        pImpl->m_bNoBasicCapabilities = true;

    const bool bDocRecovery = ( i_nCreationFlags & SfxModelFlags::DISABLE_DOCUMENT_RECOVERY ) == SfxModelFlags::NONE;
    if ( !bDocRecovery )
        pImpl->m_bDocRecoverySupport = false;
}

/** Constructor of the class SfxObjectShell.

    @param eMode Purpose, to which the SfxObjectShell is created:
                 SfxObjectCreateMode::EMBEDDED (default) as SO-Server from within another Document
                 SfxObjectCreateMode::STANDARD, as a normal Document open stand-alone
                 SfxObjectCreateMode::ORGANIZER to be displayed in the Organizer, here nothing of the contents is used
*/
SfxObjectShell::SfxObjectShell(SfxObjectCreateMode eMode)
    : pImpl(new SfxObjectShell_Impl(*this))
    , pMedium(nullptr)
    , eCreateMode(eMode)
    , bHasName(false)
    , bIsInGenerateThumbnail(false)
    , mbAvoidRecentDocs(false)
    , bRememberSignature(false)
{
}

SfxObjectShell::~SfxObjectShell()
{

    if ( IsEnableSetModified() )
        EnableSetModified( false );

    SfxObjectShell::CloseInternal();
    pImpl->pBaseModel.clear();

    pImpl->pReloadTimer.reset();

    SfxApplication *pSfxApp = SfxGetpApp();
    if ( USHRT_MAX != pImpl->nVisualDocumentNumber && pSfxApp )
        pSfxApp->ReleaseIndex(pImpl->nVisualDocumentNumber);

    // Destroy Basic-Manager
    pImpl->aBasicManager.reset(nullptr);

    if ( pSfxApp && pSfxApp->GetDdeService() )
        pSfxApp->RemoveDdeTopic( this );

    InternalCloseAndRemoveFiles();
}

void SfxObjectShell::InternalCloseAndRemoveFiles()
{
    // don't call GetStorage() here, in case of Load Failure it's possible that a storage was never assigned!
    if ( pMedium && pMedium->HasStorage_Impl() && pMedium->GetStorage( false ) == pImpl->m_xDocStorage )
        pMedium->CanDisposeStorage_Impl( false );

    if ( pImpl->mxObjectContainer )
    {
        pImpl->mxObjectContainer->CloseEmbeddedObjects();
        pImpl->mxObjectContainer.reset();
    }

    if ( pImpl->bOwnsStorage && pImpl->m_xDocStorage.is() )
        pImpl->m_xDocStorage->dispose();

    if ( pMedium )
    {
        pMedium->CloseAndReleaseStreams_Impl();

#if HAVE_FEATURE_MULTIUSER_ENVIRONMENT
        if (IsDocShared())
            FreeSharedFile( pMedium->GetURLObject().GetMainURL( INetURLObject::DecodeMechanism::NONE ) );
#endif
        delete pMedium;
        pMedium = nullptr;
    }

    // The removing of the temporary file must be done as the latest step in the document destruction
    if ( !pImpl->aTempName.isEmpty() )
    {
        OUString aTmp;
        osl::FileBase::getFileURLFromSystemPath( pImpl->aTempName, aTmp );
        ::utl::UCBContentHelper::Kill( aTmp );
    }
}

SfxCloseVetoLock::SfxCloseVetoLock(const SfxObjectShell* pDocShell)
    : mpDocShell(pDocShell)
{
    if (mpDocShell)
        osl_atomic_increment(&mpDocShell->Get_Impl()->m_nClosingLockLevel);
}

SfxCloseVetoLock::~SfxCloseVetoLock()
{
    if (mpDocShell && osl_atomic_decrement(&mpDocShell->Get_Impl()->m_nClosingLockLevel) == 0)
    {
        if (mpDocShell->Get_Impl()->m_bCloseModelScheduled)
        {
            mpDocShell->Get_Impl()->m_bCloseModelScheduled = false; // pass ownership
            if (rtl::Reference model = static_cast<SfxBaseModel*>(mpDocShell->GetBaseModel().get()))
            {
                try
                {
                    model->close(true);
                }
                catch (const util::CloseVetoException&)
                {
                    DBG_UNHANDLED_EXCEPTION("sfx.doc");
                }
            }
        }
    }
}

void SfxObjectShell::Stamp_SetPrintCancelState(bool bState)
{
    pImpl->bIsPrintJobCancelable = bState;
}


bool SfxObjectShell::Stamp_GetPrintCancelState() const
{
    return pImpl->bIsPrintJobCancelable;
}


// closes the Object and all its views

bool SfxObjectShell::Close()
{
    SfxObjectShellRef xKeepAlive(this);
    return CloseInternal();
}

// variant that does not take a reference to itself, so we can call it during object destruction
bool SfxObjectShell::CloseInternal()
{
    if ( !pImpl->bClosing )
    {
        // Do not close if a progress is still running
        if ( GetProgress() )
            return false;

        pImpl->bClosing = true;
        Reference< util::XCloseable > xCloseable( GetBaseModel(), UNO_QUERY );

        if ( xCloseable.is() )
        {
            try
            {
                xCloseable->close( true );
            }
            catch (const Exception&)
            {
                pImpl->bClosing = false;
            }
        }

        if ( pImpl->bClosing )
        {
            // remove from Document list
            // If there is no App, there is no document to remove
            // no need to call GetOrCreate here
            SfxApplication *pSfxApp = SfxApplication::Get();
            if(pSfxApp)
            {
                std::vector<SfxObjectShell*> &rDocs = pSfxApp->GetObjectShells_Impl();
                auto it = std::find( rDocs.begin(), rDocs.end(), this );
                if ( it != rDocs.end() )
                    rDocs.erase( it );
            }
        }
    }

    return true;
}

OUString SfxObjectShell::CreateShellID( const SfxObjectShell* pShell )
{
    if (!pShell)
        return OUString();

    OUString aShellID;

    SfxMedium* pMedium = pShell->GetMedium();
    if (pMedium)
        aShellID = pMedium->GetBaseURL();

    if (!aShellID.isEmpty())
        return aShellID;

    sal_Int64 nShellID = reinterpret_cast<sal_Int64>(pShell);
    aShellID = "0x" + OUString::number(nShellID, 16);
    return aShellID;
}

// returns a pointer the first SfxDocument of specified type

SfxObjectShell* SfxObjectShell::GetFirst
(
    const std::function<bool ( const SfxObjectShell* )>& isObjectShell,
    bool          bOnlyVisible
)
{
    std::vector<SfxObjectShell*> &rDocs = SfxGetpApp()->GetObjectShells_Impl();

    // search for a SfxDocument of the specified type
    for (SfxObjectShell* pSh : rDocs)
    {
        if ( bOnlyVisible && pSh->IsPreview() && pSh->IsReadOnly() )
            continue;

        if ( (!isObjectShell || isObjectShell( pSh)) &&
             ( !bOnlyVisible || SfxViewFrame::GetFirst( pSh  )))
            return pSh;
    }

    return nullptr;
}


// returns a pointer to the next SfxDocument of specified type behind *pDoc

SfxObjectShell* SfxObjectShell::GetNext
(
    const SfxObjectShell&   rPrev,
    const std::function<bool ( const SfxObjectShell* )>& isObjectShell,
    bool                    bOnlyVisible
)
{
    std::vector<SfxObjectShell*> &rDocs = SfxGetpApp()->GetObjectShells_Impl();

    // refind the specified predecessor
    size_t nPos;
    for ( nPos = 0; nPos < rDocs.size(); ++nPos )
        if ( rDocs[nPos] == &rPrev )
            break;

    // search for the next SfxDocument of the specified type
    for ( ++nPos; nPos < rDocs.size(); ++nPos )
    {
        SfxObjectShell* pSh = rDocs[ nPos ];
        if ( bOnlyVisible && pSh->IsPreview() && pSh->IsReadOnly() )
            continue;

        if ( (!isObjectShell || isObjectShell( pSh)) &&
             ( !bOnlyVisible || SfxViewFrame::GetFirst( pSh )))
            return pSh;
    }
    return nullptr;
}

SfxObjectShell* SfxObjectShell::Current()
{
    SfxViewFrame *pFrame = SfxViewFrame::Current();
    return pFrame ? pFrame->GetObjectShell() : nullptr;
}

bool SfxObjectShell::IsInPrepareClose() const
{
    return pImpl->bInPrepareClose;
}

namespace {

struct BoolEnv_Impl
{
    SfxObjectShell_Impl& rImpl;
    explicit BoolEnv_Impl( SfxObjectShell_Impl& rImplP) : rImpl( rImplP )
    { rImplP.bInPrepareClose = true; }
    ~BoolEnv_Impl() { rImpl.bInPrepareClose = false; }
};

}

bool SfxObjectShell::PrepareClose
(
    bool bUI   // true: Dialog and so on is allowed
               // false: silent-mode
)
{
    if( pImpl->bInPrepareClose || pImpl->bPreparedForClose )
        return true;
    BoolEnv_Impl aBoolEnv( *pImpl );

    // DocModalDialog?
    if ( IsInModalMode() )
        return false;

    SfxViewFrame* pFirst = SfxViewFrame::GetFirst( this );
    if( pFirst && !pFirst->GetFrame().PrepareClose_Impl( bUI ) )
        return false;

    // prepare views for closing
    for ( SfxViewFrame* pFrm = SfxViewFrame::GetFirst( this );
          pFrm; pFrm = SfxViewFrame::GetNext( *pFrm, this ) )
    {
        DBG_ASSERT(pFrm->GetViewShell(),"No Shell");
        if ( pFrm->GetViewShell() )
        {
            bool bRet = pFrm->GetViewShell()->PrepareClose( bUI );
            if ( !bRet )
                return bRet;
        }
    }

    SfxApplication *pSfxApp = SfxGetpApp();
    pSfxApp->NotifyEvent( SfxEventHint(SfxEventHintId::PrepareCloseDoc, GlobalEventConfig::GetEventName(GlobalEventId::PREPARECLOSEDOC), this) );

    if( GetCreateMode() == SfxObjectCreateMode::EMBEDDED )
    {
        pImpl->bPreparedForClose = true;
        return true;
    }

    // Ask if possible if it should be saved
    // only ask for the Document in the visible window
    SfxViewFrame *pFrame = SfxObjectShell::Current() == this
        ? SfxViewFrame::Current() : SfxViewFrame::GetFirst( this );

    if ( bUI && IsModified() && pFrame )
    {
        // restore minimized
        SfxFrame& rTop = pFrame->GetFrame();
        SfxViewFrame::SetViewFrame( rTop.GetCurrentViewFrame() );
        pFrame->GetFrame().Appear();

        // Ask if to save
        short nRet = RET_YES;
        {
            const Reference<XTitle> xTitle(*pImpl->pBaseModel, UNO_QUERY_THROW);
            const OUString     sTitle = xTitle->getTitle ();
            nRet = ExecuteQuerySaveDocument(pFrame->GetFrameWeld(), sTitle);
        }
        /*HACK for plugin::destroy()*/

        if ( RET_YES == nRet )
        {
            // Save by each Dispatcher
            SfxPoolItemHolder aPoolItem;
            if (IsReadOnly())
            {
                SfxBoolItem aWarnItem( SID_FAIL_ON_WARNING, bUI );
                const SfxPoolItem* ppArgs[] = { &aWarnItem, nullptr };
                aPoolItem = pFrame->GetBindings().ExecuteSynchron(SID_SAVEASDOC, ppArgs);
            }
            else if (IsSaveVersionOnClose())
            {
                SfxStringItem aItem( SID_DOCINFO_COMMENTS, SfxResId(STR_AUTOMATICVERSION) );
                SfxBoolItem aWarnItem( SID_FAIL_ON_WARNING, bUI );
                const SfxPoolItem* ppArgs[] = { &aItem, &aWarnItem, nullptr };
                aPoolItem = pFrame->GetBindings().ExecuteSynchron( SID_SAVEDOC, ppArgs );
            }
            else
            {
                SfxBoolItem aWarnItem( SID_FAIL_ON_WARNING, bUI );
                const SfxPoolItem* ppArgs[] = { &aWarnItem, nullptr };
                aPoolItem = pFrame->GetBindings().ExecuteSynchron( SID_SAVEDOC, ppArgs );
            }

            if (!aPoolItem || IsDisabledItem(aPoolItem.getItem()) )
                return false;
            if ( auto pBoolItem = dynamic_cast< const SfxBoolItem *>( aPoolItem.getItem() ) )
                if ( !pBoolItem->GetValue() )
                    return false;
        }
        else if ( RET_CANCEL == nRet )
            // Cancelled
            return false;
    }

    if ( pFrame )
        sfx2::SfxNotebookBar::CloseMethod(pFrame->GetBindings());
    pImpl->bPreparedForClose = true;
    return true;
}


#if HAVE_FEATURE_SCRIPTING
namespace
{
    BasicManager* lcl_getBasicManagerForDocument( const SfxObjectShell& _rDocument )
    {
        if ( !_rDocument.Get_Impl()->m_bNoBasicCapabilities )
        {
            if ( !_rDocument.Get_Impl()->bBasicInitialized )
                const_cast< SfxObjectShell& >( _rDocument ).InitBasicManager_Impl();
            return _rDocument.Get_Impl()->aBasicManager.get();
        }

        // assume we do not have Basic ourself, but we can refer to another
        // document which does (by our model's XScriptInvocationContext::getScriptContainer).
        // In this case, we return the BasicManager of this other document.

        OSL_ENSURE( !Reference< XEmbeddedScripts >( _rDocument.GetModel(), UNO_QUERY ).is(),
            "lcl_getBasicManagerForDocument: inconsistency: no Basic, but an XEmbeddedScripts?" );
        Reference< XModel > xForeignDocument;
        Reference< XScriptInvocationContext > xContext( _rDocument.GetModel(), UNO_QUERY );
        if ( xContext.is() )
        {
            xForeignDocument.set( xContext->getScriptContainer(), UNO_QUERY );
            OSL_ENSURE( xForeignDocument.is() && xForeignDocument != _rDocument.GetModel(),
                "lcl_getBasicManagerForDocument: no Basic, but providing ourself as script container?" );
        }

        BasicManager* pBasMgr = nullptr;
        if ( xForeignDocument.is() )
            pBasMgr = ::basic::BasicManagerRepository::getDocumentBasicManager( xForeignDocument );

        return pBasMgr;
    }
}
#endif

BasicManager* SfxObjectShell::GetBasicManager() const
{
    BasicManager* pBasMgr = nullptr;
#if HAVE_FEATURE_SCRIPTING
    try
    {
        pBasMgr = lcl_getBasicManagerForDocument( *this );
        if ( !pBasMgr )
            pBasMgr = SfxApplication::GetBasicManager();
    }
    catch (const css::ucb::ContentCreationException&)
    {
        TOOLS_WARN_EXCEPTION("sfx.doc", "");
    }
#endif
    return pBasMgr;
}

bool SfxObjectShell::HasBasic() const
{
#if !HAVE_FEATURE_SCRIPTING
    return false;
#else
    if ( pImpl->m_bNoBasicCapabilities )
        return false;

    if ( !pImpl->bBasicInitialized )
        const_cast< SfxObjectShell* >( this )->InitBasicManager_Impl();

    return pImpl->aBasicManager.isValid();
#endif
}


#if HAVE_FEATURE_SCRIPTING
namespace
{
    const Reference< XStorageBasedLibraryContainer >&
    lcl_getOrCreateLibraryContainer( bool _bScript, Reference< XStorageBasedLibraryContainer >& _rxContainer,
        const Reference< XModel >& _rxDocument )
    {
        if ( !_rxContainer.is() )
        {
            try
            {
                Reference< XStorageBasedDocument > xStorageDoc( _rxDocument, UNO_QUERY );
                const Reference< XComponentContext >& xContext(
                    ::comphelper::getProcessComponentContext() );
                _rxContainer.set (   _bScript
                                ?   DocumentScriptLibraryContainer::create(
                                        xContext, xStorageDoc )
                                :   DocumentDialogLibraryContainer::create(
                                        xContext, xStorageDoc ));
            }
            catch (const Exception&)
            {
                DBG_UNHANDLED_EXCEPTION("sfx.doc");
            }
        }
        return _rxContainer;
    }
}
#endif

Reference< XStorageBasedLibraryContainer > SfxObjectShell::GetDialogContainer()
{
#if HAVE_FEATURE_SCRIPTING
    try
    {
        if ( !pImpl->m_bNoBasicCapabilities )
            return lcl_getOrCreateLibraryContainer( false, pImpl->xDialogLibraries, GetModel() );

        BasicManager* pBasMgr = lcl_getBasicManagerForDocument( *this );
        if ( pBasMgr )
            return pBasMgr->GetDialogLibraryContainer();
    }
    catch (const css::ucb::ContentCreationException&)
    {
        TOOLS_WARN_EXCEPTION("sfx.doc", "");
    }

    SAL_WARN("sfx.doc", "SfxObjectShell::GetDialogContainer: falling back to the application - is this really expected here?");
#endif
    return SfxGetpApp()->GetDialogContainer();
}

Reference< XStorageBasedLibraryContainer > SfxObjectShell::GetBasicContainer()
{
#if HAVE_FEATURE_SCRIPTING
    if (!comphelper::IsFuzzing())
    {
        try
        {
            if ( !pImpl->m_bNoBasicCapabilities )
                return lcl_getOrCreateLibraryContainer( true, pImpl->xBasicLibraries, GetModel() );

            BasicManager* pBasMgr = lcl_getBasicManagerForDocument( *this );
            if ( pBasMgr )
                return pBasMgr->GetScriptLibraryContainer();
        }
        catch (const css::ucb::ContentCreationException&)
        {
            TOOLS_WARN_EXCEPTION("sfx.doc", "");
        }
    }
    SAL_WARN("sfx.doc", "SfxObjectShell::GetBasicContainer: falling back to the application - is this really expected here?");
#endif
    return SfxGetpApp()->GetBasicContainer();
}

StarBASIC* SfxObjectShell::GetBasic() const
{
#if !HAVE_FEATURE_SCRIPTING
    return nullptr;
#else
    BasicManager * pMan = GetBasicManager();
    return pMan ? pMan->GetLib(0) : nullptr;
#endif
}

void SfxObjectShell::InitBasicManager_Impl()
/*  [Description]

    Creates a document's BasicManager and loads it, if we are already based on
    a storage.

    [Note]

    This method has to be called by implementations of <SvPersist::Load()>
    (with its pStor parameter) and by implementations of <SvPersist::InitNew()>
    (with pStor = 0).
*/

{
    /*  #163556# (DR) - Handling of recursive calls while creating the Basic
        manager.

        It is possible that (while creating the Basic manager) the code that
        imports the Basic storage wants to access the Basic manager again.
        Especially in VBA compatibility mode, there is code that wants to
        access the "VBA Globals" object which is stored as global UNO constant
        in the Basic manager.

        To achieve correct handling of the recursive calls of this function
        from lcl_getBasicManagerForDocument(), the implementation of the
        function BasicManagerRepository::getDocumentBasicManager() has been
        changed to return the Basic manager currently under construction, when
        called repeatedly.

        The variable pImpl->bBasicInitialized will be set to sal_True after
        construction now, to ensure that the recursive call of the function
        lcl_getBasicManagerForDocument() will be routed into this function too.

        Calling BasicManagerHolder::reset() twice is not a big problem, as it
        does not take ownership but stores only the raw pointer. Owner of all
        Basic managers is the global BasicManagerRepository instance.
     */
#if HAVE_FEATURE_SCRIPTING
    DBG_ASSERT( !pImpl->bBasicInitialized && !pImpl->aBasicManager.isValid(), "Local BasicManager already exists");
    try
    {
        pImpl->aBasicManager.reset( BasicManagerRepository::getDocumentBasicManager( GetModel() ) );
    }
    catch (const css::ucb::ContentCreationException&)
    {
        TOOLS_WARN_EXCEPTION("sfx.doc", "");
    }
    DBG_ASSERT( pImpl->aBasicManager.isValid(), "SfxObjectShell::InitBasicManager_Impl: did not get a BasicManager!" );
    pImpl->bBasicInitialized = true;
#endif
}


bool SfxObjectShell::DoClose()
{
    return Close();
}


SfxObjectShell* SfxObjectShell::GetObjectShell()
{
    return this;
}


uno::Sequence< OUString > SfxObjectShell::GetEventNames()
{
    static uno::Sequence< OUString > s_EventNameContainer(rtl::Reference<GlobalEventConfig>(new GlobalEventConfig)->getElementNames());

    return s_EventNameContainer;
}


css::uno::Reference< css::frame::XModel3 > SfxObjectShell::GetModel() const
{
    return GetBaseModel();
}

void SfxObjectShell::SetBaseModel( SfxBaseModel* pModel )
{
    OSL_ENSURE( !pImpl->pBaseModel.is() || pModel == nullptr, "Model already set!" );
    pImpl->pBaseModel.set( pModel );
    if ( pImpl->pBaseModel.is() )
    {
        pImpl->pBaseModel->addCloseListener( new SfxModelListener_Impl(this) );
    }
}


css::uno::Reference< css::frame::XModel3 > SfxObjectShell::GetBaseModel() const
{
    return pImpl->pBaseModel;
}

void SfxObjectShell::SetAutoStyleFilterIndex(sal_uInt16 nSet)
{
    pImpl->nStyleFilter = nSet;
}

sal_uInt16 SfxObjectShell::GetAutoStyleFilterIndex() const
{
    return pImpl->nStyleFilter;
}


void SfxObjectShell::SetCurrentComponent( const Reference< XInterface >& _rxComponent )
{
    WeakReference< XInterface >& rTheCurrentComponent = theCurrentComponent;

    Reference< XInterface > xOldCurrentComp(rTheCurrentComponent);
    if ( _rxComponent == xOldCurrentComp )
        // nothing to do
        return;
    // note that "_rxComponent.get() == s_xCurrentComponent.get().get()" is /sufficient/, but not
    // /required/ for "_rxComponent == s_xCurrentComponent.get()".
    // In other words, it's still possible that we here do something which is not necessary,
    // but we should have filtered quite some unnecessary calls already.

#if HAVE_FEATURE_SCRIPTING
    BasicManager* pAppMgr = SfxApplication::GetBasicManager();
    rTheCurrentComponent = _rxComponent;
    if ( !pAppMgr )
        return;

    // set "ThisComponent" for Basic
    pAppMgr->SetGlobalUNOConstant( u"ThisComponent"_ustr, Any( _rxComponent ) );

    // set new current component for VBA compatibility
    if ( _rxComponent.is() )
    {
        OUString aVBAConstName = lclGetVBAGlobalConstName( _rxComponent );
        if ( !aVBAConstName.isEmpty() )
        {
            pAppMgr->SetGlobalUNOConstant( aVBAConstName, Any( _rxComponent ) );
            s_aRegisteredVBAConstants[ _rxComponent.get() ] = aVBAConstName;
        }
    }
    // no new component passed -> remove last registered VBA component
    else if ( xOldCurrentComp.is() )
    {
        OUString aVBAConstName = lclGetVBAGlobalConstName( xOldCurrentComp );
        if ( !aVBAConstName.isEmpty() )
        {
            pAppMgr->SetGlobalUNOConstant( aVBAConstName, Any( Reference< XInterface >() ) );
            s_aRegisteredVBAConstants.erase( xOldCurrentComp.get() );
        }
    }
#endif
}

Reference< XInterface > SfxObjectShell::GetCurrentComponent()
{
    return theCurrentComponent;
}


OUString SfxObjectShell::GetServiceNameFromFactory( const OUString& rFact )
{
    //! Remove everything behind name!
    OUString aFact( rFact );
    OUString aPrefix(u"private:factory/"_ustr);
    if ( aFact.startsWith( aPrefix ) )
        aFact = aFact.copy( aPrefix.getLength() );
    sal_Int32 nPos = aFact.indexOf( '?' );
    if ( nPos != -1 )
    {
        aFact = aFact.copy( 0, nPos );
    }
    aFact = aFact.replaceAll("4", "");
    aFact = aFact.toAsciiLowerCase();

    // HACK: sometimes a real document service name is given here instead of
    // a factory short name. Set return value directly to this service name as fallback
    // in case next lines of code does nothing ...
    // use rFact instead of normed aFact value !
    OUString aServiceName = rFact;

    if ( aFact == "swriter" )
    {
        aServiceName = "com.sun.star.text.TextDocument";
    }
    else if ( aFact == "sweb" || aFact == "swriter/web" )
    {
        aServiceName = "com.sun.star.text.WebDocument";
    }
    else if ( aFact == "sglobal" || aFact == "swriter/globaldocument" )
    {
        aServiceName = "com.sun.star.text.GlobalDocument";
    }
    else if ( aFact == "scalc" )
    {
        aServiceName = "com.sun.star.sheet.SpreadsheetDocument";
    }
    else if ( aFact == "sdraw" )
    {
        aServiceName = "com.sun.star.drawing.DrawingDocument";
    }
    else if ( aFact == "simpress" )
    {
        aServiceName = "com.sun.star.presentation.PresentationDocument";
    }
    else if ( aFact == "schart" )
    {
        aServiceName = "com.sun.star.chart.ChartDocument";
    }
    else if ( aFact == "smath" )
    {
        aServiceName = "com.sun.star.formula.FormulaProperties";
    }
#if HAVE_FEATURE_SCRIPTING
    else if ( aFact == "sbasic" )
    {
        aServiceName = "com.sun.star.script.BasicIDE";
    }
#endif
#if HAVE_FEATURE_DBCONNECTIVITY && !ENABLE_FUZZERS
    else if ( aFact == "sdatabase" )
    {
        aServiceName = "com.sun.star.sdb.OfficeDatabaseDocument";
    }
#endif

    return aServiceName;
}

SfxObjectShell* SfxObjectShell::CreateObjectByFactoryName( const OUString& rFact, SfxObjectCreateMode eMode )
{
    return CreateObject( GetServiceNameFromFactory( rFact ), eMode );
}


SfxObjectShell* SfxObjectShell::CreateObject( const OUString& rServiceName, SfxObjectCreateMode eCreateMode )
{
    if ( !rServiceName.isEmpty() )
    {
        uno::Reference < frame::XModel > xDoc( ::comphelper::getProcessServiceFactory()->createInstance( rServiceName ), UNO_QUERY );
        if (SfxObjectShell* pRet = SfxObjectShell::GetShellFromComponent(xDoc))
        {
            pRet->SetCreateMode_Impl(eCreateMode);
            return pRet;
        }
    }

    return nullptr;
}

Reference<lang::XComponent> SfxObjectShell::CreateAndLoadComponent( const SfxItemSet& rSet )
{
    uno::Sequence < beans::PropertyValue > aProps;
    TransformItems( SID_OPENDOC, rSet, aProps );
    const SfxStringItem* pFileNameItem = rSet.GetItem<SfxStringItem>(SID_FILE_NAME, false);
    const SfxStringItem* pTargetItem = rSet.GetItem<SfxStringItem>(SID_TARGETNAME, false);
    OUString aURL;
    OUString aTarget(u"_blank"_ustr);
    if ( pFileNameItem )
        aURL = pFileNameItem->GetValue();
    if ( pTargetItem )
        aTarget = pTargetItem->GetValue();

    uno::Reference < frame::XComponentLoader > xLoader =
            frame::Desktop::create(comphelper::getProcessComponentContext());

    Reference <lang::XComponent> xComp;
    try
    {
        xComp = xLoader->loadComponentFromURL(aURL, aTarget, 0, aProps);
    }
    catch (const uno::Exception&)
    {
    }

    return xComp;
}

SfxObjectShell* SfxObjectShell::GetShellFromComponent(const Reference<uno::XInterface>& xComp)
{
    try
    {
        Reference<lang::XUnoTunnel> xTunnel(xComp, UNO_QUERY);
        if (!xTunnel)
            return nullptr;
        static const Sequence <sal_Int8> aSeq( SvGlobalName( SFX_GLOBAL_CLASSID ).GetByteSequence() );
        return comphelper::getSomething_cast<SfxObjectShell>(xTunnel->getSomething(aSeq));
    }
    catch (const Exception&)
    {
    }

    return nullptr;
}

SfxObjectShell* SfxObjectShell::GetParentShell(const css::uno::Reference<css::uno::XInterface>& xChild)
{
    SfxObjectShell* pResult = nullptr;

    try
    {
        if (css::uno::Reference<css::container::XChild> xChildModel{ xChild, css::uno::UNO_QUERY })
            pResult = GetShellFromComponent(xChildModel->getParent());
    }
    catch (const Exception&)
    {
    }

    return pResult;
}

void SfxObjectShell::SetInitialized_Impl( const bool i_fromInitNew )
{
    pImpl->bInitialized = true;
    if (comphelper::IsFuzzing())
        return;
    if ( i_fromInitNew )
    {
        SetActivateEvent_Impl( SfxEventHintId::CreateDoc );
        SfxGetpApp()->NotifyEvent( SfxEventHint( SfxEventHintId::DocCreated, GlobalEventConfig::GetEventName(GlobalEventId::DOCCREATED), this ) );
    }
    else
    {
        SfxGetpApp()->NotifyEvent( SfxEventHint( SfxEventHintId::LoadFinished, GlobalEventConfig::GetEventName(GlobalEventId::LOADFINISHED), this ) );
    }
}


bool SfxObjectShell::IsChangeRecording(SfxViewShell* /*pViewShell*/, bool /*bRecordAllViews*/) const
{
    // currently this function needs to be overwritten by Writer and Calc only
    SAL_WARN( "sfx.doc", "function not implemented" );
    return false;
}


bool SfxObjectShell::HasChangeRecordProtection() const
{
    // currently this function needs to be overwritten by Writer and Calc only
    SAL_WARN( "sfx.doc", "function not implemented" );
    return false;
}


void SfxObjectShell::SetChangeRecording( bool /*bActivate*/, bool /*bLockAllViews*/, SfxRedlineRecordingMode /*eRedlineRecordingMode*/)
{
    // currently this function needs to be overwritten by Writer and Calc only
    SAL_WARN( "sfx.doc", "function not implemented" );
}


void SfxObjectShell::SetProtectionPassword( const OUString & /*rPassword*/ )
{
    // currently this function needs to be overwritten by Writer and Calc only
    SAL_WARN( "sfx.doc", "function not implemented" );
}


bool SfxObjectShell::GetProtectionHash( /*out*/ css::uno::Sequence< sal_Int8 > & /*rPasswordHash*/ )
{
    // currently this function needs to be overwritten by Writer and Calc only
    SAL_WARN( "sfx.doc", "function not implemented" );
    return false;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
