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

#include <com/sun/star/document/XDocumentProperties.hpp>
#include <com/sun/star/view/XRenderable.hpp>
#include <com/sun/star/view/XSelectionSupplier.hpp>

#include <comphelper/propertyvalue.hxx>
#include <officecfg/Office/Common.hxx>
#include <sal/log.hxx>
#include <utility>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <svtools/prnsetup.hxx>
#include <svl/flagitem.hxx>
#include <svl/stritem.hxx>
#include <svl/eitem.hxx>
#include <unotools/useroptions.hxx>
#include <tools/datetime.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/objface.hxx>
#include <sfx2/viewsh.hxx>
#include "viewimp.hxx"
#include <sfx2/viewfrm.hxx>
#include <sfx2/printer.hxx>
#include <sfx2/sfxresid.hxx>
#include <sfx2/request.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/event.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/docfilt.hxx>
#include <sfx2/sfxsids.hrc>
#include <sfx2/strings.hrc>
#include <sfx2/sfxuno.hxx>
#include <sfx2/tabdlg.hxx>

#include <toolkit/awt/vclxdevice.hxx>

#include "prnmon.hxx"

using namespace com::sun::star;
using namespace com::sun::star::uno;

class SfxPrinterController : public vcl::PrinterController, public SfxListener
{
    Any                                     maCompleteSelection;
    Any                                     maSelection;
    Reference< view::XRenderable >          mxRenderable;
    mutable VclPtr<Printer>                 mpLastPrinter;
    mutable Reference<awt::XDevice>         mxDevice;
    SfxViewShell*                           mpViewShell;
    SfxObjectShell*                         mpObjectShell;
    bool        m_bJobStarted;
    bool        m_bOrigStatus;
    bool        m_bNeedsChange;
    bool        m_bApi;
    bool        m_bTempPrinter;
    util::DateTime  m_aLastPrinted;
    OUString m_aLastPrintedBy;

    Sequence< beans::PropertyValue > getMergedOptions() const;
    const Any& getSelectionObject() const;

public:
    SfxPrinterController( const VclPtr<Printer>& i_rPrinter,
                          Any i_Complete,
                          Any i_Selection,
                          const Any& i_rViewProp,
                          const Reference< view::XRenderable >& i_xRender,
                          bool i_bApi, bool i_bDirect,
                          SfxViewShell* pView,
                          const uno::Sequence< beans::PropertyValue >& rProps
                        );

    virtual void Notify( SfxBroadcaster&, const SfxHint& ) override;

    virtual int  getPageCount() const override;
    virtual Sequence< beans::PropertyValue > getPageParameters( int i_nPage ) const override;
    virtual void printPage( int i_nPage ) const override;
    virtual void jobStarted() override;
    virtual void jobFinished( css::view::PrintableState ) override;
};

SfxPrinterController::SfxPrinterController( const VclPtr<Printer>& i_rPrinter,
                                            Any i_Complete,
                                            Any i_Selection,
                                            const Any& i_rViewProp,
                                            const Reference< view::XRenderable >& i_xRender,
                                            bool i_bApi, bool i_bDirect,
                                            SfxViewShell* pView,
                                            const uno::Sequence< beans::PropertyValue >& rProps
                                          )
    : PrinterController(i_rPrinter, pView ? pView->GetFrameWeld() : nullptr)
    , maCompleteSelection(std::move( i_Complete ))
    , maSelection(std::move( i_Selection ))
    , mxRenderable( i_xRender )
    , mpLastPrinter( nullptr )
    , mpViewShell( pView )
    , mpObjectShell(nullptr)
    , m_bJobStarted( false )
    , m_bOrigStatus( false )
    , m_bNeedsChange( false )
    , m_bApi(i_bApi)
    , m_bTempPrinter( i_rPrinter )
{
    if ( mpViewShell )
    {
        StartListening( *mpViewShell );
        mpObjectShell = mpViewShell->GetObjectShell();
        StartListening( *mpObjectShell );
    }

    // initialize extra ui options
    if( mxRenderable.is() )
    {
        for (const auto& rProp : rProps)
            setValue( rProp.Name, rProp.Value );

        Sequence< beans::PropertyValue > aRenderOptions{
            comphelper::makePropertyValue(u"ExtraPrintUIOptions"_ustr, Any{}),
            comphelper::makePropertyValue(u"View"_ustr, i_rViewProp),
            comphelper::makePropertyValue(u"IsPrinter"_ustr, true)
        };
        try
        {
            const Sequence< beans::PropertyValue > aRenderParms( mxRenderable->getRenderer( 0 , getSelectionObject(), aRenderOptions ) );
            for( const auto& rRenderParm : aRenderParms )
            {
                if ( rRenderParm.Name == "ExtraPrintUIOptions" )
                {
                    Sequence< beans::PropertyValue > aUIProps;
                    rRenderParm.Value >>= aUIProps;
                    setUIOptions( aUIProps );
                }
                else if( rRenderParm.Name == "NUp" )
                {
                    setValue( rRenderParm.Name, rRenderParm.Value );
                }
            }
        }
        catch( lang::IllegalArgumentException& )
        {
            // the first renderer should always be available for the UI options,
            // but catch the exception to be safe
        }
    }

    // set some job parameters
    setValue( u"IsApi"_ustr, Any( i_bApi ) );
    setValue( u"IsDirect"_ustr, Any( i_bDirect ) );
    setValue( u"IsPrinter"_ustr, Any( true ) );
    setValue( u"View"_ustr, i_rViewProp );
}

void SfxPrinterController::Notify( SfxBroadcaster& , const SfxHint& rHint )
{
    if ( rHint.GetId() == SfxHintId::Dying )
    {
        EndListening(*mpViewShell);
        EndListening(*mpObjectShell);
        dialogsParentClosing();
        mpViewShell = nullptr;
        mpObjectShell = nullptr;
    }
}

const Any& SfxPrinterController::getSelectionObject() const
{
    const beans::PropertyValue* pVal = getValue( u"PrintSelectionOnly"_ustr );
    if( pVal )
    {
        bool bSel = false;
        pVal->Value >>= bSel;
        return bSel ? maSelection : maCompleteSelection;
    }

    sal_Int32 nChoice = 0;
    pVal = getValue( u"PrintContent"_ustr );
    if( pVal )
        pVal->Value >>= nChoice;

    return (nChoice > 1) ? maSelection : maCompleteSelection;
}

Sequence< beans::PropertyValue > SfxPrinterController::getMergedOptions() const
{
    VclPtr<Printer> xPrinter( getPrinter() );
    if( xPrinter.get() != mpLastPrinter )
    {
        mpLastPrinter = xPrinter.get();
        rtl::Reference<VCLXDevice> pXDevice = new VCLXDevice();
        pXDevice->SetOutputDevice( mpLastPrinter );
        mxDevice.set( pXDevice );
    }

    Sequence< beans::PropertyValue > aRenderOptions{ comphelper::makePropertyValue(
        u"RenderDevice"_ustr, mxDevice) };

    aRenderOptions = getJobProperties( aRenderOptions );
    return aRenderOptions;
}

int SfxPrinterController::getPageCount() const
{
    int nPages = 0;
    VclPtr<Printer> xPrinter( getPrinter() );
    if( mxRenderable.is() && xPrinter )
    {
        Sequence< beans::PropertyValue > aJobOptions( getMergedOptions() );
        try
        {
            nPages = mxRenderable->getRendererCount( getSelectionObject(), aJobOptions );
        }
        catch (lang::DisposedException &)
        {
            SAL_WARN("sfx", "SfxPrinterController: document disposed while printing");
            const_cast<SfxPrinterController*>(this)->setJobState(
                    view::PrintableState_JOB_ABORTED);
        }
    }
    return nPages;
}

Sequence< beans::PropertyValue > SfxPrinterController::getPageParameters( int i_nPage ) const
{
    VclPtr<Printer> xPrinter( getPrinter() );
    Sequence< beans::PropertyValue > aResult;

    if (mxRenderable.is() && xPrinter)
    {
        Sequence< beans::PropertyValue > aJobOptions( getMergedOptions() );
        try
        {
            aResult = mxRenderable->getRenderer( i_nPage, getSelectionObject(), aJobOptions );
        }
        catch( lang::IllegalArgumentException& )
        {
        }
        catch (lang::DisposedException &)
        {
            SAL_WARN("sfx", "SfxPrinterController: document disposed while printing");
            const_cast<SfxPrinterController*>(this)->setJobState(
                    view::PrintableState_JOB_ABORTED);
        }
    }
    return aResult;
}

void SfxPrinterController::printPage( int i_nPage ) const
{
    VclPtr<Printer> xPrinter( getPrinter() );
    if( !mxRenderable.is() || !xPrinter )
        return;

    Sequence< beans::PropertyValue > aJobOptions( getMergedOptions() );
    try
    {
        mxRenderable->render( i_nPage, getSelectionObject(), aJobOptions );
    }
    catch( lang::IllegalArgumentException& )
    {
        // don't care enough about nonexistent page here
        // to provoke a crash
    }
    catch (lang::DisposedException &)
    {
        SAL_WARN("sfx", "SfxPrinterController: document disposed while printing");
        const_cast<SfxPrinterController*>(this)->setJobState(
                view::PrintableState_JOB_ABORTED);
    }
}

void SfxPrinterController::jobStarted()
{
    if ( !mpObjectShell )
        return;

    m_bJobStarted = true;

    m_bOrigStatus = mpObjectShell->IsEnableSetModified();

    // check configuration: shall update of printing information in DocInfo set the document to "modified"?
    if (m_bOrigStatus && !officecfg::Office::Common::Print::PrintingModifiesDocument::get())
    {
        mpObjectShell->EnableSetModified( false );
        m_bNeedsChange = true;
    }

    // refresh document info
    uno::Reference<document::XDocumentProperties> xDocProps(mpObjectShell->getDocProperties());
    m_aLastPrintedBy = xDocProps->getPrintedBy();
    m_aLastPrinted = xDocProps->getPrintDate();

    xDocProps->setPrintedBy( mpObjectShell->IsUseUserData()
        ? SvtUserOptions().GetFullName()
        : OUString() );
    ::DateTime now( ::DateTime::SYSTEM );

    xDocProps->setPrintDate( now.GetUNODateTime() );

    uno::Sequence < beans::PropertyValue > aOpts;
    aOpts = getJobProperties( aOpts );

    uno::Reference< frame::XController2 > xController;
    if ( mpViewShell )
        xController.set( mpViewShell->GetController(), uno::UNO_QUERY );

    mpObjectShell->Broadcast( SfxPrintingHint(
        view::PrintableState_JOB_STARTED, aOpts, mpObjectShell, xController ) );
}

void SfxPrinterController::jobFinished( css::view::PrintableState nState )
{
    if ( !mpObjectShell )
        return;

    bool bCopyJobSetup = false;
    mpObjectShell->Broadcast( SfxPrintingHint( nState ) );
    switch ( nState )
    {
        case view::PrintableState_JOB_SPOOLING_FAILED :
        case view::PrintableState_JOB_FAILED :
        {
            // "real" problem (not simply printing cancelled by user)
            OUString aMsg( SfxResId(STR_NOSTARTPRINTER) );
            if ( !m_bApi && mpViewShell )
            {
                std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(mpViewShell->GetFrameWeld(),
                                                                         VclMessageType::Warning, VclButtonsType::Ok,
                                                                         aMsg));
                xBox->run();
            }
            [[fallthrough]];
        }
        case view::PrintableState_JOB_ABORTED :
        {
            // printing not successful, reset DocInfo if the job started and so DocInfo was modified
            if (m_bJobStarted)
            {
                uno::Reference<document::XDocumentProperties> xDocProps(mpObjectShell->getDocProperties());
                xDocProps->setPrintedBy(m_aLastPrintedBy);
                xDocProps->setPrintDate(m_aLastPrinted);
            }
            break;
        }

        case view::PrintableState_JOB_SPOOLED :
        case view::PrintableState_JOB_COMPLETED :
        {
            if (mpViewShell)
            {
                SfxBindings& rBind = mpViewShell->GetViewFrame().GetBindings();
                rBind.Invalidate( SID_PRINTDOC );
                rBind.Invalidate( SID_PRINTDOCDIRECT );
                rBind.Invalidate( SID_SETUPPRINTER );
                bCopyJobSetup = ! m_bTempPrinter;
            }
            break;
        }

        default:
            break;
    }

    if( bCopyJobSetup && mpViewShell )
    {
        // #i114306#
        // Note: this possibly creates a printer that gets immediately replaced
        // by a new one. The reason for this is that otherwise we would not get
        // the printer's SfxItemSet here to copy. Awkward, but at the moment there is no
        // other way here to get the item set.
        SfxPrinter* pDocPrt = mpViewShell->GetPrinter(true);
        if( pDocPrt )
        {
            if( pDocPrt->GetName() == getPrinter()->GetName() )
                pDocPrt->SetJobSetup( getPrinter()->GetJobSetup() );
            else
            {
                VclPtr<SfxPrinter> pNewPrt = VclPtr<SfxPrinter>::Create( pDocPrt->GetOptions().Clone(), getPrinter()->GetName() );
                pNewPrt->SetJobSetup( getPrinter()->GetJobSetup() );
                mpViewShell->SetPrinter( pNewPrt, SfxPrinterChangeFlags::PRINTER | SfxPrinterChangeFlags::JOBSETUP );
            }
        }
    }

    if ( m_bNeedsChange )
        mpObjectShell->EnableSetModified( m_bOrigStatus );

    if ( mpViewShell )
    {
        mpViewShell->pImpl->m_xPrinterController.reset();
    }
}

namespace {

/**
    An instance of this class is created for the life span of the
    printer dialogue, to create in its click handler for the additions by the
    virtual method of the derived SfxViewShell generated print options dialogue
    and to cache the options set there as SfxItemSet.
*/
class SfxDialogExecutor_Impl
{
private:
    SfxViewShell*           _pViewSh;
    PrinterSetupDialog&  _rSetupParent;
    std::unique_ptr<SfxItemSet> _pOptions;
    bool                    _bHelpDisabled;

    DECL_LINK( Execute, weld::Button&, void );

public:
    SfxDialogExecutor_Impl( SfxViewShell* pViewSh, PrinterSetupDialog& rParent );

    Link<weld::Button&, void> GetLink() const { return LINK(const_cast<SfxDialogExecutor_Impl*>(this), SfxDialogExecutor_Impl, Execute); }
    const SfxItemSet*   GetOptions() const { return _pOptions.get(); }
    void                DisableHelp() { _bHelpDisabled = true; }
};

}

SfxDialogExecutor_Impl::SfxDialogExecutor_Impl( SfxViewShell* pViewSh, PrinterSetupDialog& rParent ) :

    _pViewSh        ( pViewSh ),
    _rSetupParent   ( rParent ),
    _bHelpDisabled  ( false )

{
}

IMPL_LINK_NOARG(SfxDialogExecutor_Impl, Execute, weld::Button&, void)
{
    // Options noted locally
    if ( !_pOptions )
    {
        _pOptions = static_cast<SfxPrinter*>( _rSetupParent.GetPrinter() )->GetOptions().Clone();
    }

    assert(_pOptions);
    if (!_pOptions)
        return;

    // Create Dialog
    SfxPrintOptionsDialog aDlg(_rSetupParent.GetFrameWeld(), _pViewSh, _pOptions.get() );
    if (_bHelpDisabled)
        aDlg.DisableHelp();
    if (aDlg.run() == RET_OK)
    {
        _pOptions = aDlg.GetOptions().Clone();
    }
}

/**
   Internal method for setting the differences between 'pNewPrinter' to the
   current printer. pNewPrinter is either taken over or deleted.
*/
void SfxViewShell::SetPrinter_Impl( VclPtr<SfxPrinter>& pNewPrinter )
{
    // get current Printer
    SfxPrinter *pDocPrinter = GetPrinter();

    // Evaluate Printer Options
    const SfxFlagItem *pFlagItem = pDocPrinter->GetOptions().GetItemIfSet( SID_PRINTER_CHANGESTODOC, false );
    bool bOriToDoc = pFlagItem && (static_cast<SfxPrinterChangeFlags>(pFlagItem->GetValue()) & SfxPrinterChangeFlags::CHG_ORIENTATION);
    bool bSizeToDoc = pFlagItem && (static_cast<SfxPrinterChangeFlags>(pFlagItem->GetValue()) & SfxPrinterChangeFlags::CHG_SIZE);

    // Determine the previous format and size
    Orientation eOldOri = pDocPrinter->GetOrientation();
    Size aOldPgSz = pDocPrinter->GetPaperSizePixel();

    // Determine the new format and size
    Orientation eNewOri = pNewPrinter->GetOrientation();
    Size aNewPgSz = pNewPrinter->GetPaperSizePixel();

    // Determine the changes in page format
    bool bOriChg = (eOldOri != eNewOri) && bOriToDoc;
    bool bPgSzChg = ( aOldPgSz.Height() !=
            ( bOriChg ? aNewPgSz.Width() : aNewPgSz.Height() ) ||
            aOldPgSz.Width() !=
            ( bOriChg ? aNewPgSz.Height() : aNewPgSz.Width() ) ) &&
            bSizeToDoc;

    // Message and Flags for page format changes
    OUString aMsg;
    SfxPrinterChangeFlags nNewOpt = SfxPrinterChangeFlags::NONE;
    if( bOriChg && bPgSzChg )
    {
        aMsg = SfxResId(STR_PRINT_NEWORISIZE);
        nNewOpt = SfxPrinterChangeFlags::CHG_ORIENTATION | SfxPrinterChangeFlags::CHG_SIZE;
    }
    else if (bOriChg )
    {
        aMsg = SfxResId(STR_PRINT_NEWORI);
        nNewOpt = SfxPrinterChangeFlags::CHG_ORIENTATION;
    }
    else if (bPgSzChg)
    {
        aMsg = SfxResId(STR_PRINT_NEWSIZE);
        nNewOpt = SfxPrinterChangeFlags::CHG_SIZE;
    }

    // Summarize in this variable what has been changed.
    SfxPrinterChangeFlags nChangedFlags = SfxPrinterChangeFlags::NONE;

    // Ask if possible, if page format should be taken over from printer.
    if (bOriChg || bPgSzChg)
    {
        std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(nullptr,
                                                                 VclMessageType::Question, VclButtonsType::YesNo,
                                                                 aMsg));
        if (RET_YES == xBox->run())
        {
            // Flags with changes for  <SetPrinter(SfxPrinter*)> are maintained
            nChangedFlags |= nNewOpt;
        }
    }

    // Was the printer selection changed from Default to Specific
    // or the other way around?
    if ( (pNewPrinter->GetName() != pDocPrinter->GetName())
         || (pDocPrinter->IsDefPrinter() != pNewPrinter->IsDefPrinter()) )
    {
        nChangedFlags |= SfxPrinterChangeFlags::PRINTER|SfxPrinterChangeFlags::JOBSETUP;
        if ( ! (pNewPrinter->GetOptions() == pDocPrinter->GetOptions()) )
        {
            nChangedFlags |= SfxPrinterChangeFlags::OPTIONS;
        }

        pDocPrinter = pNewPrinter;
    }
    else
    {
        // Compare extra options
        if ( ! (pNewPrinter->GetOptions() == pDocPrinter->GetOptions()) )
        {
            // Option have changed
            pDocPrinter->SetOptions( pNewPrinter->GetOptions() );
            nChangedFlags |= SfxPrinterChangeFlags::OPTIONS;
        }

        // Compare JobSetups
        JobSetup aNewJobSetup = pNewPrinter->GetJobSetup();
        JobSetup aOldJobSetup = pDocPrinter->GetJobSetup();
        if ( aNewJobSetup != aOldJobSetup )
        {
            nChangedFlags |= SfxPrinterChangeFlags::JOBSETUP;
        }

        // Keep old changed Printer.
        pDocPrinter->SetPrinterProps( pNewPrinter );
        pNewPrinter.disposeAndClear();
    }

    if ( SfxPrinterChangeFlags::NONE != nChangedFlags )
        // SetPrinter will delete the old printer if it changes
        SetPrinter( pDocPrinter, nChangedFlags );
}

void SfxViewShell::StartPrint( const uno::Sequence < beans::PropertyValue >& rProps, bool bIsAPI, bool bIsDirect )
{
    assert( !pImpl->m_xPrinterController );

    // get the current selection; our controller should know it
    Reference< frame::XController > xController( GetController() );
    Reference< view::XSelectionSupplier > xSupplier( xController, UNO_QUERY );

    Any aSelection;
    if( xSupplier.is() )
        aSelection = xSupplier->getSelection();
    else
        aSelection <<= GetObjectShell()->GetModel();
    Any aComplete( Any( GetObjectShell()->GetModel() ) );
    Any aViewProp( xController );
    VclPtr<Printer> aPrt;

    const beans::PropertyValue* pVal = std::find_if(rProps.begin(), rProps.end(),
        [](const beans::PropertyValue& rVal) { return rVal.Name == "PrinterName"; });
    if (pVal != rProps.end())
    {
        OUString aPrinterName;
        pVal->Value >>= aPrinterName;
        aPrt.reset( VclPtr<Printer>::Create( aPrinterName ) );
    }

    std::shared_ptr<vcl::PrinterController> xNewController(std::make_shared<SfxPrinterController>(
                                                                               aPrt,
                                                                               aComplete,
                                                                               aSelection,
                                                                               aViewProp,
                                                                               GetRenderable(),
                                                                               bIsAPI,
                                                                               bIsDirect,
                                                                               this,
                                                                               rProps
                                                                               ));
    pImpl->m_xPrinterController = xNewController;

    // When no JobName was specified via com::sun::star::view::PrintOptions::JobName ,
    // use the document title as default job name
    css::beans::PropertyValue* pJobNameVal = xNewController->getValue(u"JobName"_ustr);
    if (!pJobNameVal)
    {
        if (SfxObjectShell* pDoc = GetObjectShell())
        {
            xNewController->setValue(u"JobName"_ustr, Any(pDoc->GetTitle(1)));
            xNewController->setPrinterModified(mbPrinterSettingsModified);
        }
    }
}

void SfxViewShell::ExecPrint( const uno::Sequence < beans::PropertyValue >& rProps, bool bIsAPI, bool bIsDirect )
{
    StartPrint( rProps, bIsAPI, bIsDirect );
    // FIXME: job setup
    SfxPrinter* pDocPrt = GetPrinter();
    JobSetup aJobSetup = pDocPrt ? pDocPrt->GetJobSetup() : JobSetup();
    Printer::PrintJob( GetPrinterController(), aJobSetup );
}

const std::shared_ptr< vcl::PrinterController >& SfxViewShell::GetPrinterController() const
{
    return pImpl->m_xPrinterController;
}

Printer* SfxViewShell::GetActivePrinter() const
{
    return pImpl->m_xPrinterController
        ?  pImpl->m_xPrinterController->getPrinter().get() : nullptr;
}

void SfxViewShell::ExecPrint_Impl( SfxRequest &rReq )
{
    sal_uInt16              nDialogRet = RET_CANCEL;
    VclPtr<SfxPrinter>      pPrinter;
    bool                    bSilent = false;

    // does the function have been called by the user interface or by an API call
    bool bIsAPI = rReq.GetArgs() && rReq.GetArgs()->Count();
    if ( bIsAPI )
    {
        // the function have been called by the API

        // Should it be visible on the user interface,
        // should it launch popup dialogue ?
        const SfxBoolItem* pSilentItem = rReq.GetArg<SfxBoolItem>(SID_SILENT);
        bSilent = pSilentItem && pSilentItem->GetValue();
    }

    // no help button in dialogs if called from the help window
    // (pressing help button would exchange the current page inside the help
    // document that is going to be printed!)
    SfxMedium* pMedium = GetViewFrame().GetObjectShell()->GetMedium();
    std::shared_ptr<const SfxFilter> pFilter = pMedium ? pMedium->GetFilter() : nullptr;
    bool bPrintOnHelp = ( pFilter && pFilter->GetFilterName() == "writer_web_HTML_help" );

    const sal_uInt16 nId = rReq.GetSlot();
    switch( nId )
    {
        case SID_PRINTDOC: // display the printer selection and properties dialogue : File > Print...
        case SID_PRINTDOCDIRECT: // Print the document directly, without displaying the dialogue
        {
            SfxObjectShell* pDoc = GetObjectShell();

            // derived class may decide to abort this
            if( pDoc == nullptr || !pDoc->QuerySlotExecutable( nId ) )
            {
                rReq.SetReturnValue( SfxBoolItem( 0, false ) );
                return;
            }

            pDoc->QueryHiddenInformation(HiddenWarningFact::WhenPrinting);

            // should we print only the selection or the whole document
            const SfxBoolItem* pSelectItem = rReq.GetArg<SfxBoolItem>(SID_SELECTION);
            bool bSelection = ( pSelectItem != nullptr && pSelectItem->GetValue() );
            // detect non api call from writer ( that adds SID_SELECTION ) and reset bIsAPI
            if ( pSelectItem && rReq.GetArgs()->Count() == 1 )
                bIsAPI = false;

            uno::Sequence < beans::PropertyValue > aProps;
            if ( bIsAPI )
            {
                // supported properties:
                // String PrinterName
                // String FileName
                // Int16 From
                // Int16 To
                // In16 Copies
                // String RangeText
                // bool Selection
                // bool Asynchron
                // bool Collate
                // bool Silent

                // the TransformItems function overwrite aProps
                TransformItems( nId, *rReq.GetArgs(), aProps, GetInterface()->GetSlot(nId) );

                for ( auto& rProp : asNonConstRange(aProps) )
                {
                    if ( rProp.Name == "Copies" )
                    {
                        rProp.Name = "CopyCount";
                    }
                    else if ( rProp.Name == "RangeText" )
                    {
                        rProp.Name = "Pages";
                    }
                    else if ( rProp.Name == "Asynchron" )
                    {
                        rProp.Name = "Wait";
                        bool bAsynchron = false;
                        rProp.Value >>= bAsynchron;
                        rProp.Value <<= !bAsynchron;
                    }
                    else if ( rProp.Name == "Silent" )
                    {
                        rProp.Name = "MonitorVisible";
                        bool bPrintSilent = false;
                        rProp.Value >>= bPrintSilent;
                        rProp.Value <<= !bPrintSilent;
                    }
                }
            }

            // we will add the "PrintSelectionOnly" or "HideHelpButton" properties
            // we have to increase the capacity of aProps
            sal_Int32 nLen = aProps.getLength();
            aProps.realloc( nLen + 1 );
            auto pProps = aProps.getArray();

            // HACK: writer sets the SID_SELECTION item when printing directly and expects
            // to get only the selection document in that case (see getSelectionObject)
            // however it also reacts to the PrintContent property. We need this distinction here, too,
            // else one of the combinations print / print direct and selection / all will not work.
            // it would be better if writer handled this internally
            if( nId == SID_PRINTDOCDIRECT )
            {
                pProps[nLen].Name = "PrintSelectionOnly";
                pProps[nLen].Value <<= bSelection;
            }
            else // if nId == SID_PRINTDOC ; nothing to do with the previous HACK
            {
                // should the printer selection and properties dialogue display an help button
                pProps[nLen].Name = "HideHelpButton";
                pProps[nLen].Value <<= bPrintOnHelp;
            }

            ExecPrint( aProps, bIsAPI, (nId == SID_PRINTDOCDIRECT) );

            // FIXME: Recording
            rReq.Done();
            break;
        }

        case SID_PRINTER_NAME: // for recorded macros
        {
            // get printer and printer settings from the document
            SfxPrinter* pDocPrinter = GetPrinter(true);
            const SfxStringItem* pPrinterItem = rReq.GetArg<SfxStringItem>(SID_PRINTER_NAME);
            if (!pPrinterItem)
            {
                rReq.Ignore();
                break;
            }
            // use PrinterName parameter to create a printer
            pPrinter = VclPtr<SfxPrinter>::Create(pDocPrinter->GetOptions().Clone(),
                                                  pPrinterItem->GetValue());

            if (!pPrinter->IsKnown())
            {
                pPrinter.disposeAndClear();
                rReq.Ignore();
                break;
            }
            SetPrinter(pPrinter, SfxPrinterChangeFlags::PRINTER);
            rReq.Done();
            break;
        }
        case SID_SETUPPRINTER : // display the printer settings dialog : File > Printer Settings...
        {
            // get printer and printer settings from the document
            SfxPrinter *pDocPrinter = GetPrinter(true);

            // look for printer in parameters
            const SfxStringItem* pPrinterItem = rReq.GetArg<SfxStringItem>(SID_PRINTER_NAME);
            if ( pPrinterItem )
            {
                // use PrinterName parameter to create a printer
                pPrinter = VclPtr<SfxPrinter>::Create( pDocPrinter->GetOptions().Clone(), pPrinterItem->GetValue() );

                // if printer is unknown, it can't be used - now printer from document will be used
                if ( !pPrinter->IsKnown() )
                    pPrinter.disposeAndClear();
            }

            // no PrinterName parameter in ItemSet or the PrinterName points to an unknown printer
            if ( !pPrinter )
                // use default printer from document
                pPrinter = pDocPrinter;

            if( !pPrinter || !pPrinter->IsValid() )
            {
                // no valid printer either in ItemSet or at the document
                if ( !bSilent )
                {
                    std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(nullptr,
                                                                             VclMessageType::Warning, VclButtonsType::Ok,
                                                                             SfxResId(STR_NODEFPRINTER)));
                    xBox->run();
                }

                rReq.SetReturnValue(SfxBoolItem(0,false));

                break;
            }

            // FIXME: printer isn't used for printing anymore!
            if( pPrinter->IsPrinting() )
            {
                // if printer is busy, abort configuration
                if ( !bSilent )
                {
                    std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(nullptr,
                                                                             VclMessageType::Info, VclButtonsType::Ok,
                                                                             SfxResId(STR_ERROR_PRINTER_BUSY)));
                    xBox->run();
                }
                rReq.SetReturnValue(SfxBoolItem(0,false));

                return;
            }

            // Open Printer Setup dialog (needs a temporary printer)
            VclPtr<SfxPrinter> pDlgPrinter = pPrinter->Clone();
            PrinterSetupDialog aPrintSetupDlg(GetFrameWeld());
            std::unique_ptr<SfxDialogExecutor_Impl> pExecutor;

            if (pImpl->m_bHasPrintOptions && HasPrintOptionsPage())
            {
                // additional controls for dialog
                pExecutor.reset(new SfxDialogExecutor_Impl(this, aPrintSetupDlg));
                if (bPrintOnHelp)
                    pExecutor->DisableHelp();
                aPrintSetupDlg.SetOptionsHdl(pExecutor->GetLink());
            }

            aPrintSetupDlg.SetPrinter(pDlgPrinter);
            nDialogRet = aPrintSetupDlg.run();

            if (pExecutor && pExecutor->GetOptions())
            {
                if (nDialogRet == RET_OK)
                    // remark: have to be recorded if possible!
                    pDlgPrinter->SetOptions(*pExecutor->GetOptions());
                else
                {
                    pPrinter->SetOptions(*pExecutor->GetOptions());
                    SetPrinter(pPrinter, SfxPrinterChangeFlags::OPTIONS);
                }
            }

            // no recording of PrinterSetup except printer name (is printer dependent)
            rReq.Ignore();

            if (nDialogRet == RET_OK)
            {
                if (pPrinter->GetName() != pDlgPrinter->GetName())
                {
                    // user has changed the printer -> macro recording
                    SfxRequest aReq(GetViewFrame(), SID_PRINTER_NAME);
                    aReq.AppendItem(SfxStringItem(SID_PRINTER_NAME, pDlgPrinter->GetName()));
                    aReq.Done();
                }

                // take the changes made in the dialog
                SetPrinter_Impl(pDlgPrinter);

                // forget new printer, it was taken over (as pPrinter) or deleted
                pDlgPrinter = nullptr;
                mbPrinterSettingsModified = true;
            }
            else
            {
                // PrinterDialog is used to transfer information on printing,
                // so it will only be deleted here if dialog was cancelled
                pDlgPrinter.disposeAndClear();
                rReq.Ignore();
            }
            break;
        }
    }
}

SfxPrinter* SfxViewShell::GetPrinter( bool /*bCreate*/ )
{
    return nullptr;
}

sal_uInt16 SfxViewShell::SetPrinter( SfxPrinter* /*pNewPrinter*/, SfxPrinterChangeFlags /*nDiffFlags*/ )
{
    return 0;
}

std::unique_ptr<SfxTabPage> SfxViewShell::CreatePrintOptionsPage(weld::Container*, weld::DialogController*, const SfxItemSet&)
{
    return nullptr;
}

bool SfxViewShell::HasPrintOptionsPage() const
{
    return false;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
