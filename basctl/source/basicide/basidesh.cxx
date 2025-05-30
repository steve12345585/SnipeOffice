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

#include <config_options.h>

#include <comphelper/diagnose_ex.hxx>
#include <basic/basmgr.hxx>
#include <svx/zoomsliderctrl.hxx>
#include <svx/zoomslideritem.hxx>
#include <svx/svxids.hrc>
#include <iderid.hxx>
#include <strings.hrc>
#include "baside2.hxx"
#include <baside3.hxx>
#include "basdoc.hxx"
#include <IDEComboBox.hxx>
#include <editeng/sizeitem.hxx>
#include "iderdll2.hxx"
#include <basidectrlr.hxx>
#include <basidesh.hxx>
#include <basobj.hxx>
#include <localizationmgr.hxx>
#include <sfx2/app.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/dinfdlg.hxx>
#include <sfx2/infobar.hxx>
#include <sfx2/minfitem.hxx>
#include <sfx2/objface.hxx>
#include <sfx2/viewfrm.hxx>
#include <svl/srchitem.hxx>
#include <tools/debug.hxx>
#include <unotools/viewoptions.hxx>

#if defined(DISABLE_DYNLOADING) || ENABLE_MERGELIBS
/* Avoid clash with the ones from svx/source/form/typemap.cxx */
#define aSfxDocumentInfoItem_Impl basctl_source_basicide_basidesh_aSfxDocumentInfoItem_Impl
#define aSfxUnoAnyItem_Impl basctl_source_basicide_basidesh_aSfxUnoAnyItem_Impl
#endif

#define ShellClass_basctl_Shell
#define SFX_TYPEMAP
#include <basslots.hxx>

#if defined(DISABLE_DYNLOADING) || ENABLE_MERGELIBS
#undef aSfxDocumentInfoItem_Impl
#undef aSfxUnoAnyItem_Impl
#endif

#include <iderdll.hxx>
#include <svx/pszctrl.hxx>
#include <svx/insctrl.hxx>
#include <svx/srchdlg.hxx>
#include <com/sun/star/script/XLibraryContainerPassword.hpp>
#include <com/sun/star/container/XContainer.hpp>
#include <svx/xmlsecctrl.hxx>
#include <sfx2/viewfac.hxx>
#include <vcl/weld.hxx>
#include <vcl/settings.hxx>
#include <vcl/svapp.hxx>
#include <cppuhelper/implbase.hxx>
#include <BasicColorConfig.hxx>
#include <officecfg/Office/BasicIDE.hxx>
#include <LineStatusControl.hxx>

namespace basctl
{
constexpr OUString BASIC_IDE_EDITOR_WINDOW = u"BasicIDEEditorWindow"_ustr;
constexpr OUString BASIC_IDE_CURRENT_ZOOM = u"CurrentZoom"_ustr;

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star;

class ContainerListenerImpl : public ::cppu::WeakImplHelper< container::XContainerListener >
{
    Shell* mpShell;
public:
    explicit ContainerListenerImpl(Shell* pShell)
        : mpShell(pShell)
    {
    }

    void addContainerListener( const ScriptDocument& rScriptDocument, const OUString& aLibName )
    {
        try
        {
            uno::Reference< container::XContainer > xContainer( rScriptDocument.getLibrary( E_SCRIPTS, aLibName, false ), uno::UNO_QUERY );
            if ( xContainer.is() )
            {
                uno::Reference< container::XContainerListener > xContainerListener( this );
                xContainer->addContainerListener( xContainerListener );
            }
        }
        catch(const uno::Exception& ) {}
    }
    void removeContainerListener( const ScriptDocument& rScriptDocument, const OUString& aLibName )
    {
        try
        {
            uno::Reference< container::XContainer > xContainer( rScriptDocument.getLibrary( E_SCRIPTS, aLibName, false ), uno::UNO_QUERY );
            if ( xContainer.is() )
            {
                uno::Reference< container::XContainerListener > xContainerListener( this );
                xContainer->removeContainerListener( xContainerListener );
            }
        }
        catch(const uno::Exception& ) {}
    }

    // XEventListener
    virtual void SAL_CALL disposing( const lang::EventObject& ) override {}

    // XContainerListener
    virtual void SAL_CALL elementInserted( const container::ContainerEvent& Event ) override
    {
        OUString sModuleName;
        if( mpShell && ( Event.Accessor >>= sModuleName ) )
            mpShell->FindBasWin( mpShell->m_aCurDocument, mpShell->m_aCurLibName, sModuleName, true );
    }
    virtual void SAL_CALL elementReplaced( const container::ContainerEvent& ) override { }
    virtual void SAL_CALL elementRemoved( const container::ContainerEvent& Event ) override
    {
        OUString sModuleName;
        if( mpShell && ( Event.Accessor >>= sModuleName ) )
        {
            VclPtr<ModulWindow> pWin = mpShell->FindBasWin(mpShell->m_aCurDocument, mpShell->m_aCurLibName, sModuleName, false, true);
            if( pWin )
                mpShell->RemoveWindow( pWin, true );
        }
    }

};

SFX_IMPL_NAMED_VIEWFACTORY( Shell, "Default" )
{
    SFX_VIEW_REGISTRATION( DocShell );
}

SFX_IMPL_INTERFACE(basctl_Shell, SfxViewShell)

void basctl_Shell::InitInterface_Impl()
{
    GetStaticInterface()->RegisterChildWindow(SID_SEARCH_DLG);
    GetStaticInterface()->RegisterChildWindow(SID_SHOW_PROPERTYBROWSER, false, SfxShellFeature::BasicShowBrowser);
    GetStaticInterface()->RegisterChildWindow(SfxInfoBarContainerChild::GetChildWindowId());

    GetStaticInterface()->RegisterPopupMenu(u"dialog"_ustr);
}

unsigned Shell::nShellCount = 0;

Shell::Shell( SfxViewFrame& rFrame_, SfxViewShell* /* pOldShell */ ) :
    SfxViewShell( rFrame_, SfxViewShellFlags::NO_NEWWINDOW ),
    m_aCurDocument( ScriptDocument::getApplicationScriptDocument() ),
    aHScrollBar( VclPtr<ScrollAdaptor>::Create(&GetViewFrame().GetWindow(), true) ),
    aVScrollBar( VclPtr<ScrollAdaptor>::Create(&GetViewFrame().GetWindow(), false) ),
    pLayout(nullptr),
    aObjectCatalog(VclPtr<ObjectCatalog>::Create(&GetViewFrame().GetWindow())),
    m_bAppBasicModified( false ),
    m_aNotifier( *this )
{
    m_xLibListener = new ContainerListenerImpl( this );
    Init();
    nShellCount++;
}

void Shell::Init()
{
    SvxPosSizeStatusBarControl::RegisterControl();
    SvxInsertStatusBarControl::RegisterControl();
    XmlSecStatusBarControl::RegisterControl( SID_SIGNATURE );

    SvxSearchDialogWrapper::RegisterChildWindow();

    GetExtraData()->ShellInCriticalSection() = true;

    SetName( u"BasicIDE"_ustr );

    LibBoxControl::RegisterControl( SID_BASICIDE_LIBSELECTOR );
    LanguageBoxControl::RegisterControl( SID_BASICIDE_CURRENT_LANG );
    SvxZoomSliderControl::RegisterControl( SID_ATTR_ZOOMSLIDER );
    LineStatusControl::RegisterControl(SID_BASICIDE_STAT_POS);

    GetViewFrame().GetWindow().SetBackground(
        GetViewFrame().GetWindow().GetSettings().GetStyleSettings().GetWindowColor()
    );

    // Used to access color settings of the Basic code editor
    m_aColorConfig = std::make_shared<BasicColorConfig>();

    pCurWin = nullptr;
    m_aCurDocument = ScriptDocument::getApplicationScriptDocument();
    bCreatingWindow = false;

    pTabBar.reset(VclPtr<TabBar>::Create(&GetViewFrame().GetWindow()));

    nCurKey = 100;
    InitScrollBars();
    InitTabBar();
    InitZoomLevel();

    // Initialize the visibility of the Object Catalog
    bool bObjCatVisible = ::officecfg::Office::BasicIDE::EditorSettings::ObjectCatalog::get();
    if (!bObjCatVisible)
        aObjectCatalog->Show(bObjCatVisible);

    SetCurLib( ScriptDocument::getApplicationScriptDocument(), u"Standard"_ustr, false, false );

    ShellCreated(this);

    GetExtraData()->ShellInCriticalSection() = false;

    // It's enough to create the controller ...
    // It will be public by using magic :-)
    new Controller(this);

    // Force updating the title ! Because it must be set to the controller
    // it has to be called directly after creating those controller.
    SetMDITitle ();

    UpdateWindows();
}

Shell::~Shell()
{
    m_aNotifier.dispose();

    ShellDestroyed(this);

    // so that on a basic saving error, the shell doesn't pop right up again
    GetExtraData()->ShellInCriticalSection() = true;

    SetWindow( nullptr );
    SetCurWindow( nullptr );

    aObjectCatalog.disposeAndClear();
    aVScrollBar.disposeAndClear();
    aHScrollBar.disposeAndClear();

    for (auto & window : aWindowTable)
    {
        // no store; does already happen when the BasicManagers are destroyed
        window.second.disposeAndClear();
    }

    // no store; does already happen when the BasicManagers are destroyed
    aWindowTable.clear();

    // Destroy all ContainerListeners for Basic Container.
    if (m_xLibListener)
        m_xLibListener->removeContainerListener(m_aCurDocument, m_aCurLibName);

    GetExtraData()->ShellInCriticalSection() = false;

    nShellCount--;

    pDialogLayout.disposeAndClear();
    pModulLayout.disposeAndClear();
    pTabBar.disposeAndClear();

    // Remember current zoom level
    SvtViewOptions(EViewType::Window, BASIC_IDE_EDITOR_WINDOW).SetUserItem(
        BASIC_IDE_CURRENT_ZOOM, Any(m_nCurrentZoomSliderValue));
}

void Shell::onDocumentCreated( const ScriptDocument& /*_rDocument*/ )
{
    if (pCurWin)
        pCurWin->OnNewDocument();

    UpdateWindows();
}

void Shell::onDocumentOpened( const ScriptDocument& /*_rDocument*/ )
{
    if (pCurWin)
        pCurWin->OnNewDocument();
    UpdateWindows();
}

void Shell::onDocumentSave( const ScriptDocument& /*_rDocument*/ )
{
    StoreAllWindowData();
}

void Shell::onDocumentSaveDone( const ScriptDocument& /*_rDocument*/ )
{
    // #i115671: Update SID_SAVEDOC after saving is completed
    if (SfxBindings* pBindings = GetBindingsPtr())
        pBindings->Invalidate( SID_SAVEDOC );
}

void Shell::onDocumentSaveAs( const ScriptDocument& /*_rDocument*/ )
{
    StoreAllWindowData();
}

void Shell::onDocumentSaveAsDone( const ScriptDocument& /*_rDocument*/ )
{
    // not interested in
}

void Shell::onDocumentClosed( const ScriptDocument& _rDocument )
{
    if ( !_rDocument.isValid() )
        return;

    bool bSetCurWindow = false;
    bool bSetCurLib = ( _rDocument == m_aCurDocument );
    std::vector<VclPtr<BaseWindow> > aDeleteVec;

    // remove all windows which belong to this document
    for (auto const& window : aWindowTable)
    {
        BaseWindow* pWin = window.second;
        if ( pWin->IsDocument( _rDocument ) )
        {
            if ( pWin->GetStatus() & (BASWIN_RUNNINGBASIC|BASWIN_INRESCHEDULE) )
            {
                pWin->AddStatus( BASWIN_TOBEKILLED );
                pWin->Hide();
                StarBASIC::Stop();
                // there's no notify
                pWin->BasicStopped();
            }
            else
                aDeleteVec.emplace_back(pWin );
        }
    }
    // delete windows outside main loop so we don't invalidate the original iterator
    for (VclPtr<BaseWindow> const & pWin : aDeleteVec)
    {
        pWin->StoreData();
        if ( pWin == pCurWin )
            bSetCurWindow = true;
        RemoveWindow( pWin, true, false );
    }

    // remove lib info
    if (ExtraData* pData = GetExtraData())
        pData->GetLibInfo().RemoveInfoFor( _rDocument );

    if ( bSetCurLib )
        SetCurLib( ScriptDocument::getApplicationScriptDocument(), u"Standard"_ustr, true, false );
    else if ( bSetCurWindow )
        SetCurWindow( FindApplicationWindow(), true );
}

void Shell::onDocumentTitleChanged( const ScriptDocument& /*_rDocument*/ )
{
    if (SfxBindings* pBindings = GetBindingsPtr())
        pBindings->Invalidate( SID_BASICIDE_LIBSELECTOR, true );
    SetMDITitle();
}

void Shell::onDocumentModeChanged( const ScriptDocument& _rDocument )
{
    for (auto const& window : aWindowTable)
    {
        BaseWindow* pWin = window.second;
        if ( pWin->IsDocument( _rDocument ) && _rDocument.isDocument() )
            pWin->SetReadOnly( _rDocument.isReadOnly() );
    }
}

void Shell::InitZoomLevel()
{
    m_nCurrentZoomSliderValue = DEFAULT_ZOOM_LEVEL;
    SvtViewOptions aWinOpt(EViewType::Window, BASIC_IDE_EDITOR_WINDOW);
    if (aWinOpt.Exists())
    {
        try
        {
            aWinOpt.GetUserItem(BASIC_IDE_CURRENT_ZOOM) >>= m_nCurrentZoomSliderValue;
        }
        catch(const css::container::NoSuchElementException&)
        { TOOLS_WARN_EXCEPTION("basctl.basicide", "Zoom level not defined"); }
    }
}

// Applies the new zoom level to all open editor windows
void Shell::SetGlobalEditorZoomLevel(sal_uInt16 nNewZoomLevel)
{
    for (auto const& window : aWindowTable)
    {
        ModulWindow* pModuleWindow = dynamic_cast<ModulWindow*>(window.second.get());
        if (pModuleWindow)
        {
            EditorWindow& pEditorWindow = pModuleWindow->GetEditorWindow();
            pEditorWindow.SetEditorZoomLevel(nNewZoomLevel);
        }
    }

    // Update the zoom slider value based on the new global zoom level
    m_nCurrentZoomSliderValue = nNewZoomLevel;

    if (SfxBindings* pBindings = GetBindingsPtr())
    {
        pBindings->Invalidate( SID_BASICIDE_CURRENT_ZOOM );
        pBindings->Invalidate( SID_ATTR_ZOOMSLIDER );
    }
}

void Shell::StoreAllWindowData( bool bPersistent )
{
    for (auto const& window : aWindowTable)
    {
        BaseWindow* pWin = window.second;
        assert(pWin && "PrepareClose: NULL-Pointer in Table?");
        if ( !pWin->IsSuspended() )
            pWin->StoreData();
    }

    if ( bPersistent  )
    {
        SfxGetpApp()->SaveBasicAndDialogContainer();
        SetAppBasicModified(false);

        if (SfxBindings* pBindings = GetBindingsPtr())
        {
            pBindings->Invalidate( SID_SAVEDOC );
            pBindings->Update( SID_SAVEDOC );
        }
    }
}

bool Shell::PrepareClose( bool bUI )
{
    // reset here because it's modified after printing etc. (DocInfo)
    GetViewFrame().GetObjectShell()->SetModified(false);

    if ( StarBASIC::IsRunning() )
    {
        if( bUI )
        {
            std::unique_ptr<weld::MessageDialog> xInfoBox(Application::CreateMessageDialog(GetViewFrame().GetFrameWeld(),
                                                          VclMessageType::Info, VclButtonsType::Ok,
                                                          IDEResId(RID_STR_CANNOTCLOSE)));
            xInfoBox->run();
        }
        return false;
    }
    else
    {
        StoreAllWindowData( false );    // don't write on the disk, that will be done later automatically
        return true;
    }
}

void Shell::InitScrollBars()
{
    aVScrollBar->SetLineSize( 300 );
    aVScrollBar->SetPageSize( 2000 );
    aHScrollBar->SetLineSize( 300 );
    aHScrollBar->SetPageSize( 2000 );
}

void Shell::InitTabBar()
{
    pTabBar->Enable();
    pTabBar->Show();
    pTabBar->SetSelectHdl( LINK( this, Shell, TabBarHdl ) );
}

void Shell::OuterResizePixel( const Point &rPos, const Size &rSize )
{
    AdjustPosSizePixel( rPos, rSize );
}

IMPL_LINK( Shell, TabBarHdl, ::TabBar *, pCurTabBar, void )
{
    sal_uInt16 nCurId = pCurTabBar->GetCurPageId();
    BaseWindow* pWin = aWindowTable[ nCurId ].get();
    DBG_ASSERT( pWin, "Entry in TabBar is not matching a window!" );
    SetCurWindow( pWin );
}


bool Shell::NextPage( bool bPrev )
{
    bool bRet = false;
    sal_uInt16 nPos = pTabBar->GetPagePos( pTabBar->GetCurPageId() );

    if ( bPrev )
        --nPos;
    else
        ++nPos;

    if ( nPos < pTabBar->GetPageCount() )
    {
        VclPtr<BaseWindow> pWin = aWindowTable[ pTabBar->GetPageId( nPos ) ];
        SetCurWindow( pWin, true );
        bRet = true;
    }

    return bRet;
}

SfxUndoManager* Shell::GetUndoManager()
{
    SfxUndoManager* pMgr = nullptr;
    if( pCurWin )
        pMgr = pCurWin->GetUndoManager();

    return pMgr;
}


void Shell::Notify( SfxBroadcaster& rBC, const SfxHint& rHint )
{
    if (!GetShell())
        return;

    if (rHint.GetId() == SfxHintId::Dying)
    {
        EndListening( rBC, true /* log off all */ );
        aObjectCatalog->UpdateEntries();
    }

    const SfxHintId nHintId = rHint.GetId();
    if ( ( nHintId != SfxHintId::BasicStart ) &&
         ( nHintId != SfxHintId::BasicStop ) )
        return;

    if (SfxBindings* pBindings = GetBindingsPtr())
    {
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
        pBindings->Invalidate( SID_BASICSTOP );
        pBindings->Update( SID_BASICSTOP );
        pBindings->Invalidate( SID_BASICIDE_TOGGLEBRKPNT );
        pBindings->Update( SID_BASICIDE_TOGGLEBRKPNT );
        pBindings->Invalidate( SID_BASICIDE_MANAGEBRKPNTS );
        pBindings->Update( SID_BASICIDE_MANAGEBRKPNTS );
        pBindings->Invalidate( SID_BASICIDE_MODULEDLG );
        pBindings->Update( SID_BASICIDE_MODULEDLG );
        pBindings->Invalidate( SID_BASICLOAD );
        pBindings->Update( SID_BASICLOAD );
    }

    if ( nHintId == SfxHintId::BasicStop )
    {
        // not only at error/break or explicit stoppage,
        // if the update is turned off due to a programming bug
        BasicStopped();
        if (pLayout)
            pLayout->UpdateDebug(true); // clear...
        if( m_pCurLocalizationMgr )
            m_pCurLocalizationMgr->handleBasicStopped();
    }
    else if( m_pCurLocalizationMgr )
    {
        m_pCurLocalizationMgr->handleBasicStarted();
    }

    for (auto const& window : aWindowTable)
    {
        BaseWindow* pWin = window.second;
        if ( nHintId == SfxHintId::BasicStart )
            pWin->BasicStarted();
        else
            pWin->BasicStopped();
    }
}


void Shell::CheckWindows()
{
    bool bSetCurWindow = false;
    std::vector<VclPtr<BaseWindow> > aDeleteVec;
    for (auto const& window : aWindowTable)
    {
        BaseWindow* pWin = window.second;
        if ( pWin->GetStatus() & BASWIN_TOBEKILLED )
            aDeleteVec.emplace_back(pWin );
    }
    for ( VclPtr<BaseWindow> const & pWin : aDeleteVec )
    {
        pWin->StoreData();
        if ( pWin == pCurWin )
            bSetCurWindow = true;
        RemoveWindow( pWin, true, false );
    }
    if ( bSetCurWindow )
        SetCurWindow( FindApplicationWindow(), true );
}


void Shell::RemoveWindows( const ScriptDocument& rDocument, std::u16string_view rLibName )
{
    bool bChangeCurWindow = pCurWin;
    std::vector<VclPtr<BaseWindow> > aDeleteVec;
    for (auto const& window : aWindowTable)
    {
        BaseWindow* pWin = window.second;
        if ( pWin->IsDocument( rDocument ) && pWin->GetLibName() == rLibName )
            aDeleteVec.emplace_back(pWin );
    }
    for ( VclPtr<BaseWindow> const & pWin : aDeleteVec )
    {
        if ( pWin == pCurWin )
            bChangeCurWindow = true;
        pWin->StoreData();
        RemoveWindow( pWin, true/*bDestroy*/, false );
    }
    if ( bChangeCurWindow )
        SetCurWindow( FindApplicationWindow(), true );
}


void Shell::UpdateWindows()
{
    // remove all windows that may not be displayed
    bool bChangeCurWindow = pCurWin == nullptr;
    // stores the total number of modules and dialogs visible
    sal_uInt16 nTotalTabs = 0;

    if ( !m_aCurLibName.isEmpty() )
    {
        std::vector<VclPtr<BaseWindow> > aDeleteVec;
        for (auto const& window : aWindowTable)
        {
            BaseWindow* pWin = window.second;
            if ( !pWin->IsDocument( m_aCurDocument ) || pWin->GetLibName() != m_aCurLibName )
            {
                if ( pWin == pCurWin )
                    bChangeCurWindow = true;
                pWin->StoreData();
                // The request of RUNNING prevents the crash when in reschedule.
                // Window is frozen at first, later the windows should be changed
                // anyway to be marked as hidden instead of being deleted.
                if ( !(pWin->GetStatus() & ( BASWIN_TOBEKILLED | BASWIN_RUNNINGBASIC | BASWIN_SUSPENDED ) ) )
                    aDeleteVec.emplace_back(pWin );
            }
        }
        for (auto const& elem : aDeleteVec)
        {
            RemoveWindow( elem, false, false );
        }
    }

    if ( bCreatingWindow )
        return;

    BaseWindow* pNextActiveWindow = nullptr;

    // show all windows that are to be shown
    ScriptDocuments aDocuments( ScriptDocument::getAllScriptDocuments( ScriptDocument::AllWithApplication ) );
    for (auto const& doc : aDocuments)
    {
        StartListening(*doc.getBasicManager(), DuplicateHandling::Prevent /* log on only once */);

        // libraries
        for (auto& aLibName : doc.getLibraryNames())
        {
            if ( m_aCurLibName.isEmpty() || ( doc == m_aCurDocument && aLibName == m_aCurLibName ) )
            {
                // check, if library is password protected and not verified
                bool bProtected = false;
                Reference< script::XLibraryContainer > xModLibContainer( doc.getLibraryContainer( E_SCRIPTS ) );
                if ( xModLibContainer.is() && xModLibContainer->hasByName( aLibName ) )
                {
                    Reference< script::XLibraryContainerPassword > xPasswd( xModLibContainer, UNO_QUERY );
                    if ( xPasswd.is() && xPasswd->isLibraryPasswordProtected( aLibName ) && !xPasswd->isLibraryPasswordVerified( aLibName ) )
                    {
                        bProtected = true;
                    }
                }

                if ( !bProtected )
                {
                    LibInfo::Item const* pLibInfoItem = nullptr;
                    if (ExtraData* pData = GetExtraData())
                        pLibInfoItem = pData->GetLibInfo().GetInfo(doc, aLibName);

                    // modules
                    if ( xModLibContainer.is() && xModLibContainer->hasByName( aLibName ) )
                    {
                        StarBASIC* pLib = doc.getBasicManager()->GetLib( aLibName );
                        if ( pLib )
                            StartListening(pLib->GetBroadcaster(), DuplicateHandling::Prevent /* log on only once */);

                        try
                        {
                            Sequence< OUString > aModNames( doc.getObjectNames( E_SCRIPTS, aLibName ) );
                            nTotalTabs += aModNames.getLength();

                            for (auto& aModName : aModNames)
                            {
                                VclPtr<ModulWindow> pWin = FindBasWin( doc, aLibName, aModName );
                                if ( !pWin )
                                    pWin = CreateBasWin( doc, aLibName, aModName );
                                if ( !pNextActiveWindow && pLibInfoItem && pLibInfoItem->GetCurrentName() == aModName &&
                                     pLibInfoItem->GetCurrentType() == SBX_TYPE_MODULE )
                                {
                                    pNextActiveWindow = pWin;
                                }
                            }
                        }
                        catch (const container::NoSuchElementException& )
                        {
                            DBG_UNHANDLED_EXCEPTION("basctl.basicide");
                        }
                    }

                    // dialogs
                    Reference< script::XLibraryContainer > xDlgLibContainer( doc.getLibraryContainer( E_DIALOGS ) );
                    if ( xDlgLibContainer.is() && xDlgLibContainer->hasByName( aLibName ) )
                    {
                        try
                        {
                            Sequence< OUString > aDlgNames = doc.getObjectNames( E_DIALOGS, aLibName );
                            nTotalTabs += aDlgNames.getLength();

                            for (auto& aDlgName : aDlgNames)
                            {
                                // this find only looks for non-suspended windows;
                                // suspended windows are handled in CreateDlgWin
                                VclPtr<DialogWindow> pWin = FindDlgWin( doc, aLibName, aDlgName );
                                if ( !pWin )
                                    pWin = CreateDlgWin( doc, aLibName, aDlgName );
                                if ( !pNextActiveWindow && pLibInfoItem && pLibInfoItem->GetCurrentName() == aDlgName &&
                                     pLibInfoItem->GetCurrentType() == SBX_TYPE_DIALOG )
                                {
                                    pNextActiveWindow = pWin;
                                }
                            }
                        }
                        catch (const container::NoSuchElementException& )
                        {
                            DBG_UNHANDLED_EXCEPTION("basctl.basicide");
                        }
                    }
                }
            }
        }
    }

    if ( bChangeCurWindow )
    {
        if ( nTotalTabs == 0 )
        {
            // If no tabs are opened, create a generic module and make it visible
            pNextActiveWindow = CreateBasWin( m_aCurDocument, m_aCurLibName, OUString() );
        }
        else if ( !pNextActiveWindow )
        {
            pNextActiveWindow = FindApplicationWindow().get();
        }
        SetCurWindow( pNextActiveWindow, true );
    }
}

void Shell::RemoveWindow( BaseWindow* pWindow_, bool bDestroy, bool bAllowChangeCurWindow )
{
    VclPtr<BaseWindow> pWindowTmp( pWindow_ );

    assert(pWindow_ && "Cannot delete NULL-Pointer!");
    sal_uInt16 nKey = GetWindowId( pWindow_ );
    pTabBar->RemovePage( nKey );
    aWindowTable.erase( nKey );
    if ( pWindow_ == pCurWin )
    {
        if ( bAllowChangeCurWindow )
        {
            SetCurWindow( FindApplicationWindow(), true );
        }
        else
        {
            SetCurWindow( nullptr );
        }
    }
    if ( bDestroy )
    {
        if ( !( pWindow_->GetStatus() & BASWIN_INRESCHEDULE ) )
        {
            pWindowTmp.disposeAndClear();
        }
        else
        {
            pWindow_->AddStatus( BASWIN_TOBEKILLED );
            pWindow_->Hide();
            // In normal mode stop basic in windows to be deleted
            // In VBA stop basic only if the running script is trying to delete
            // its parent module
            bool bStop = true;
            if ( pWindow_->GetDocument().isInVBAMode() )
            {
                SbModule* pMod = StarBASIC::GetActiveModule();
                if ( !pMod || pMod->GetName() != pWindow_->GetName() )
                {
                    bStop = false;
                }
            }
            if ( bStop )
            {
                StarBASIC::Stop();
                // there will be no notify...
                pWindow_->BasicStopped();
            }
            aWindowTable[ nKey ] = pWindow_;   // jump in again
        }
    }
    else
    {
        pWindow_->AddStatus( BASWIN_SUSPENDED );
        pWindow_->Deactivating();
        aWindowTable[ nKey ] = pWindow_;   // jump in again
    }

}


sal_uInt16 Shell::InsertWindowInTable( BaseWindow* pNewWin )
{
    nCurKey++;
    aWindowTable[ nCurKey ] = pNewWin;
    return nCurKey;
}


void Shell::InvalidateBasicIDESlots()
{
    // only those that have an optic effect...

    if (!GetShell())
        return;

    SfxBindings* pBindings = GetBindingsPtr();
    if (!pBindings)
        return;

    pBindings->Invalidate( SID_COPY );
    pBindings->Invalidate( SID_CUT );
    pBindings->Invalidate( SID_PASTE );
    pBindings->Invalidate( SID_UNDO );
    pBindings->Invalidate( SID_REDO );
    pBindings->Invalidate( SID_SAVEDOC );
    pBindings->Invalidate( SID_SIGNATURE );
    pBindings->Invalidate( SID_BASICIDE_CHOOSEMACRO );
    pBindings->Invalidate( SID_BASICIDE_MODULEDLG );
    pBindings->Invalidate( SID_BASICIDE_OBJCAT );
    pBindings->Invalidate( SID_BASICSTOP );
    pBindings->Invalidate( SID_BASICRUN );
    pBindings->Invalidate( SID_BASICCOMPILE );
    pBindings->Invalidate( SID_BASICLOAD );
    pBindings->Invalidate( SID_BASICSAVEAS );
    pBindings->Invalidate( SID_BASICIDE_MATCHGROUP );
    pBindings->Invalidate( SID_BASICSTEPINTO );
    pBindings->Invalidate( SID_BASICSTEPOVER );
    pBindings->Invalidate( SID_BASICSTEPOUT );
    pBindings->Invalidate( SID_BASICIDE_TOGGLEBRKPNT );
    pBindings->Invalidate( SID_BASICIDE_MANAGEBRKPNTS );
    pBindings->Invalidate( SID_BASICIDE_ADDWATCH );
    pBindings->Invalidate( SID_BASICIDE_REMOVEWATCH );

    pBindings->Invalidate( SID_PRINTDOC );
    pBindings->Invalidate( SID_PRINTDOCDIRECT );
    pBindings->Invalidate( SID_SETUPPRINTER );
    pBindings->Invalidate( SID_DIALOG_TESTMODE );

    pBindings->Invalidate( SID_DOC_MODIFIED );
    pBindings->Invalidate( SID_BASICIDE_STAT_TITLE );
    pBindings->Invalidate( SID_BASICIDE_STAT_POS );
    pBindings->Invalidate( SID_ATTR_INSERT );
    pBindings->Invalidate( SID_ATTR_SIZE );
}

void Shell::InvalidateControlSlots()
{
    if (!GetShell())
        return;

    SfxBindings* pBindings = GetBindingsPtr();
    if (!pBindings)
        return;

    pBindings->Invalidate( SID_INSERT_FORM_RADIO );
    pBindings->Invalidate( SID_INSERT_FORM_CHECK );
    pBindings->Invalidate( SID_INSERT_FORM_LIST );
    pBindings->Invalidate( SID_INSERT_FORM_COMBO );
    pBindings->Invalidate( SID_INSERT_FORM_VSCROLL );
    pBindings->Invalidate( SID_INSERT_FORM_HSCROLL );
    pBindings->Invalidate( SID_INSERT_FORM_SPIN );

    pBindings->Invalidate( SID_INSERT_SELECT );
    pBindings->Invalidate( SID_INSERT_PUSHBUTTON );
    pBindings->Invalidate( SID_INSERT_RADIOBUTTON );
    pBindings->Invalidate( SID_INSERT_CHECKBOX );
    pBindings->Invalidate( SID_INSERT_LISTBOX );
    pBindings->Invalidate( SID_INSERT_COMBOBOX );
    pBindings->Invalidate( SID_INSERT_GROUPBOX );
    pBindings->Invalidate( SID_INSERT_EDIT );
    pBindings->Invalidate( SID_INSERT_FIXEDTEXT );
    pBindings->Invalidate( SID_INSERT_IMAGECONTROL );
    pBindings->Invalidate( SID_INSERT_PROGRESSBAR );
    pBindings->Invalidate( SID_INSERT_HSCROLLBAR );
    pBindings->Invalidate( SID_INSERT_VSCROLLBAR );
    pBindings->Invalidate( SID_INSERT_HFIXEDLINE );
    pBindings->Invalidate( SID_INSERT_VFIXEDLINE );
    pBindings->Invalidate( SID_INSERT_DATEFIELD );
    pBindings->Invalidate( SID_INSERT_TIMEFIELD );
    pBindings->Invalidate( SID_INSERT_NUMERICFIELD );
    pBindings->Invalidate( SID_INSERT_CURRENCYFIELD );
    pBindings->Invalidate( SID_INSERT_FORMATTEDFIELD );
    pBindings->Invalidate( SID_INSERT_PATTERNFIELD );
    pBindings->Invalidate( SID_INSERT_FILECONTROL );
    pBindings->Invalidate( SID_INSERT_SPINBUTTON );
    pBindings->Invalidate( SID_INSERT_GRIDCONTROL );
    pBindings->Invalidate( SID_INSERT_HYPERLINKCONTROL );
    pBindings->Invalidate( SID_INSERT_TREECONTROL );
    pBindings->Invalidate( SID_CHOOSE_CONTROLS );
}

void Shell::SetCurLib( const ScriptDocument& rDocument, const OUString& aLibName, bool bUpdateWindows, bool bCheck )
{
    if ( bCheck && rDocument == m_aCurDocument && aLibName == m_aCurLibName )
        return;

    if (m_xLibListener)
        m_xLibListener->removeContainerListener(m_aCurDocument, m_aCurLibName);

    m_aCurDocument = rDocument;
    m_aCurLibName = aLibName;

    if ( m_xLibListener )
        m_xLibListener->addContainerListener( m_aCurDocument, aLibName );

    if ( bUpdateWindows )
        UpdateWindows();

    SetMDITitle();

    SetCurLibForLocalization( rDocument, aLibName );

    if (SfxBindings* pBindings = GetBindingsPtr())
    {
        pBindings->Invalidate( SID_BASICIDE_LIBSELECTOR );
        pBindings->Invalidate( SID_BASICIDE_CURRENT_LANG );
        pBindings->Invalidate( SID_BASICIDE_MANAGE_LANG );
    }
}

void Shell::SetCurLibForLocalization( const ScriptDocument& rDocument, const OUString& aLibName )
{
    // Create LocalizationMgr
    Reference< resource::XStringResourceManager > xStringResourceManager;
    try
    {
        if( !aLibName.isEmpty() )
        {
            Reference< container::XNameContainer > xDialogLib( rDocument.getLibrary( E_DIALOGS, aLibName, true ) );
            xStringResourceManager = LocalizationMgr::getStringResourceFromDialogLibrary( xDialogLib );
        }
    }
    catch (const container::NoSuchElementException& )
    {}

    m_pCurLocalizationMgr = std::make_shared<LocalizationMgr>(this, rDocument, aLibName, xStringResourceManager);
    m_pCurLocalizationMgr->handleTranslationbar();
}

} // namespace basctl

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
