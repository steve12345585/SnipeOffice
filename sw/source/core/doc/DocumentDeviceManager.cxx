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

#include <DocumentDeviceManager.hxx>

#include <memory>
#include <utility>

#include <doc.hxx>
#include <DocumentSettingManager.hxx>
#include <IDocumentDrawModelAccess.hxx>
#include <IDocumentState.hxx>
#include <IDocumentLayoutAccess.hxx>
#include <osl/diagnose.h>
#include <sfx2/printer.hxx>
#include <vcl/virdev.hxx>
#include <vcl/outdev.hxx>
#include <vcl/jobset.hxx>
#include <printdata.hxx>
#include <vcl/mapmod.hxx>
#include <svl/itemset.hxx>
#include <cfgitems.hxx>
#include <cmdid.h>
#include <drawdoc.hxx>
#include <wdocsh.hxx>
#include <prtopt.hxx>
#include <viewsh.hxx>
#include <rootfrm.hxx>
#include <viewopt.hxx>
#include <swwait.hxx>
#include <fntcache.hxx>

class SwDocShell;
class SwWait;

namespace sw {

DocumentDeviceManager::DocumentDeviceManager( SwDoc& i_rSwdoc ) : m_rDoc( i_rSwdoc ), mpPrt(nullptr), mpVirDev(nullptr) {}

SfxPrinter* DocumentDeviceManager::getPrinter(/*[in]*/ bool bCreate ) const
{
    SfxPrinter* pRet = nullptr;
    if ( !bCreate || mpPrt )
        pRet = mpPrt;
    else
        pRet = &CreatePrinter_();

    return pRet;
}

void DocumentDeviceManager::setPrinter(/*[in]*/ SfxPrinter *pP,/*[in]*/ bool bDeleteOld,/*[in]*/ bool bCallPrtDataChanged )
{
    assert ( !pP || !pP->isDisposed() );
    if ( pP != mpPrt )
    {
        if ( bDeleteOld )
            mpPrt.disposeAndClear();
        mpPrt = pP;

        // our printer should always use TWIP. Don't rely on this being set in SwViewShell::InitPrt, there
        // are situations where this isn't called. #i108712#
        if ( mpPrt )
        {
            MapMode aMapMode( mpPrt->GetMapMode() );
            aMapMode.SetMapUnit( MapUnit::MapTwip );
            mpPrt->SetMapMode( aMapMode );
        }

        if ( m_rDoc.getIDocumentDrawModelAccess().GetDrawModel() && !m_rDoc.GetDocumentSettingManager().get( DocumentSettingId::USE_VIRTUAL_DEVICE ) )
            m_rDoc.getIDocumentDrawModelAccess().GetDrawModel()->SetRefDevice( mpPrt );
    }

    if ( bCallPrtDataChanged &&
         // #i41075# Do not call PrtDataChanged() if we do not
         // use the printer for formatting:
         !m_rDoc.GetDocumentSettingManager().get(DocumentSettingId::USE_VIRTUAL_DEVICE) )
        PrtDataChanged();
}

VirtualDevice* DocumentDeviceManager::getVirtualDevice(/*[in]*/ bool bCreate ) const
{
    VirtualDevice* pRet = nullptr;
    if ( !bCreate || mpVirDev )
        pRet = mpVirDev;
    else
        pRet = &CreateVirtualDevice_();

    assert ( !pRet || !pRet->isDisposed() );

    return pRet;
}

void DocumentDeviceManager::setVirtualDevice(/*[in]*/ VirtualDevice* pVd )
{
    assert ( !pVd->isDisposed() );

    if ( mpVirDev.get() != pVd )
    {
        mpVirDev.disposeAndClear();
        mpVirDev = pVd;

        if ( m_rDoc.getIDocumentDrawModelAccess().GetDrawModel() && m_rDoc.GetDocumentSettingManager().get( DocumentSettingId::USE_VIRTUAL_DEVICE ) )
            m_rDoc.getIDocumentDrawModelAccess().GetDrawModel()->SetRefDevice( mpVirDev );
    }
}

OutputDevice* DocumentDeviceManager::getReferenceDevice(/*[in]*/ bool bCreate ) const
{
    OutputDevice* pRet = nullptr;
    if ( !m_rDoc.GetDocumentSettingManager().get(DocumentSettingId::USE_VIRTUAL_DEVICE) )
    {
        pRet = getPrinter( bCreate );

        if ( bCreate && !mpPrt->IsValid() )
        {
            pRet = getVirtualDevice( true );
        }
    }
    else
    {
        pRet = getVirtualDevice( bCreate );
    }

    assert ( !pRet || !pRet->isDisposed() );

    return pRet;
}

void DocumentDeviceManager::setReferenceDeviceType(/*[in]*/ bool bNewVirtual, /*[in]*/ bool bNewHiRes )
{
    if ( m_rDoc.GetDocumentSettingManager().get(DocumentSettingId::USE_VIRTUAL_DEVICE) == bNewVirtual &&
         m_rDoc.GetDocumentSettingManager().get(DocumentSettingId::USE_HIRES_VIRTUAL_DEVICE) == bNewHiRes )
        return;

    if ( bNewVirtual )
    {
        VirtualDevice* pMyVirDev = getVirtualDevice( true );
        if ( !bNewHiRes )
            pMyVirDev->SetReferenceDevice( VirtualDevice::RefDevMode::Dpi600 );
        else
            pMyVirDev->SetReferenceDevice( VirtualDevice::RefDevMode::MSO1 );

        if( m_rDoc.getIDocumentDrawModelAccess().GetDrawModel() )
            m_rDoc.getIDocumentDrawModelAccess().GetDrawModel()->SetRefDevice( pMyVirDev );
    }
    else
    {
        // #i41075#
        // We have to take care that a printer exists before calling
        // PrtDataChanged() in order to prevent that PrtDataChanged()
        // triggers this funny situation:
        // getReferenceDevice()->getPrinter()->CreatePrinter_()
        // ->setPrinter()-> PrtDataChanged()
        SfxPrinter* pPrinter = getPrinter( true );
        if( m_rDoc.getIDocumentDrawModelAccess().GetDrawModel() )
            m_rDoc.getIDocumentDrawModelAccess().GetDrawModel()->SetRefDevice( pPrinter );
    }

    m_rDoc.GetDocumentSettingManager().set(DocumentSettingId::USE_VIRTUAL_DEVICE, bNewVirtual );
    m_rDoc.GetDocumentSettingManager().set(DocumentSettingId::USE_HIRES_VIRTUAL_DEVICE, bNewHiRes );
    PrtDataChanged();
    m_rDoc.getIDocumentState().SetModified();
}

const JobSetup* DocumentDeviceManager::getJobsetup() const
{
    return mpPrt ? &mpPrt->GetJobSetup() : nullptr;
}

void DocumentDeviceManager::setJobsetup(/*[in]*/ const JobSetup &rJobSetup )
{
    bool bCheckPageDescs = !mpPrt;
    bool bDataChanged = false;

    if ( mpPrt )
    {
        if ( mpPrt->GetName() == rJobSetup.GetPrinterName() )
        {
            if ( mpPrt->GetJobSetup() != rJobSetup )
            {
                mpPrt->SetJobSetup( rJobSetup );
                bDataChanged = true;
            }
        }
        else
            mpPrt.disposeAndClear();
    }

    if( !mpPrt )
    {
        //The ItemSet is deleted by Sfx!
        auto pSet = std::make_unique<SfxItemSet>(SfxItemSet::makeFixedSfxItemSet<
                        SID_PRINTER_NOTFOUND_WARN, SID_PRINTER_NOTFOUND_WARN,
                        SID_PRINTER_CHANGESTODOC, SID_PRINTER_CHANGESTODOC,
                        SID_HTML_MODE, SID_HTML_MODE,
                        FN_PARAM_ADDPRINTER, FN_PARAM_ADDPRINTER>(m_rDoc.GetAttrPool()));
        VclPtr<SfxPrinter> p = VclPtr<SfxPrinter>::Create( std::move(pSet), rJobSetup );
        if ( bCheckPageDescs )
            setPrinter( p, true, true );
        else
        {
            mpPrt = std::move(p);
            bDataChanged = true;
        }
    }
    if ( bDataChanged && !m_rDoc.GetDocumentSettingManager().get(DocumentSettingId::USE_VIRTUAL_DEVICE) )
        PrtDataChanged();
}

const SwPrintData & DocumentDeviceManager::getPrintData() const
{
    if(!mpPrtData)
    {
        DocumentDeviceManager * pThis = const_cast< DocumentDeviceManager * >(this);
        pThis->mpPrtData.reset(new SwPrintData);

        // SwPrintData should be initialized from the configuration,
        // the respective config item is implemented by SwPrintOptions which
        // is also derived from SwPrintData
        const SwDocShell *pDocSh = m_rDoc.GetDocShell();
        OSL_ENSURE( pDocSh, "pDocSh is 0, can't determine if this is a WebDoc or not" );
        bool bWeb = dynamic_cast< const SwWebDocShell * >(pDocSh) !=  nullptr;
        *pThis->mpPrtData = SwPrintOptions(bWeb);
    }
    assert(mpPrtData && "this will always be set by now");
    return *mpPrtData;
}

void DocumentDeviceManager::setPrintData(/*[in]*/ const SwPrintData& rPrtData )
{
    if(!mpPrtData)
        mpPrtData.reset(new SwPrintData);
    *mpPrtData = rPrtData;
}

DocumentDeviceManager::~DocumentDeviceManager()
{
    mpPrtData.reset();
    mpVirDev.disposeAndClear();
    mpPrt.disposeAndClear();
}

VirtualDevice& DocumentDeviceManager::CreateVirtualDevice_() const
{
#ifdef IOS
    VclPtr<VirtualDevice> pNewVir = VclPtr<VirtualDevice>::Create(DeviceFormat::GRAYSCALE);
#else
    VclPtr<VirtualDevice> pNewVir = VclPtr<VirtualDevice>::Create(DeviceFormat::WITHOUT_ALPHA);
#endif

    pNewVir->SetReferenceDevice( VirtualDevice::RefDevMode::MSO1 );

    // #i60945# External leading compatibility for unix systems.
    if ( m_rDoc.GetDocumentSettingManager().get(DocumentSettingId::UNIX_FORCE_ZERO_EXT_LEADING ) )
        pNewVir->Compat_ZeroExtleadBug();

    MapMode aMapMode( pNewVir->GetMapMode() );
    aMapMode.SetMapUnit( MapUnit::MapTwip );
    pNewVir->SetMapMode( aMapMode );

    const_cast<DocumentDeviceManager*>(this)->setVirtualDevice( pNewVir );
    return *mpVirDev;
}

SfxPrinter& DocumentDeviceManager::CreatePrinter_() const
{
    OSL_ENSURE( ! mpPrt, "Do not call CreatePrinter_(), call getPrinter() instead" );

    // We create a default SfxPrinter.
    // The ItemSet is deleted by Sfx!
    auto pSet = std::make_unique<SfxItemSet>(SfxItemSet::makeFixedSfxItemSet<
                    SID_PRINTER_NOTFOUND_WARN, SID_PRINTER_NOTFOUND_WARN,
                    SID_PRINTER_CHANGESTODOC, SID_PRINTER_CHANGESTODOC,
                    SID_HTML_MODE, SID_HTML_MODE,
                    FN_PARAM_ADDPRINTER, FN_PARAM_ADDPRINTER>(m_rDoc.GetAttrPool()));
    VclPtr<SfxPrinter> pNewPrt = VclPtr<SfxPrinter>::Create( std::move(pSet) );

    // assign PrintData to newly created printer
    const SwPrintData& rPrtData = getPrintData();
    SwAddPrinterItem aAddPrinterItem(rPrtData);
    SfxItemSet aOptions(pNewPrt->GetOptions());
    aOptions.Put(aAddPrinterItem);
    pNewPrt->SetOptions(aOptions);

    const_cast<DocumentDeviceManager*>(this)->setPrinter( pNewPrt, true, true );
    return *mpPrt;
}

void DocumentDeviceManager::PrtDataChanged()
{
// If you change this, also modify InJobSetup in Sw3io if appropriate.

    // #i41075#
    OSL_ENSURE( m_rDoc.getIDocumentSettingAccess().get(DocumentSettingId::USE_VIRTUAL_DEVICE) ||
            nullptr != getPrinter( false ), "PrtDataChanged will be called recursively!" );
    SwRootFrame* pTmpRoot = m_rDoc.getIDocumentLayoutAccess().GetCurrentLayout();
    std::optional<SwWait> oWait;
    bool bEndAction = false;

    if( m_rDoc.GetDocShell() )
        m_rDoc.GetDocShell()->UpdateFontList();

    bool bDraw = true;
    if ( pTmpRoot )
    {
        SwViewShell *pSh = m_rDoc.getIDocumentLayoutAccess().GetCurrentViewShell();
        if( pSh &&
            (!pSh->GetViewOptions()->getBrowseMode() ||
             pSh->GetViewOptions()->IsPrtFormat()) )
        {
            if ( m_rDoc.GetDocShell() )
                oWait.emplace( *m_rDoc.GetDocShell(), true );

            pTmpRoot->StartAllAction();
            bEndAction = true;

            bDraw = false;
            if( m_rDoc.getIDocumentDrawModelAccess().GetDrawModel() )
            {
                m_rDoc.getIDocumentDrawModelAccess().GetDrawModel()->SetAddExtLeading( m_rDoc.GetDocumentSettingManager().get(DocumentSettingId::ADD_EXT_LEADING) );
                m_rDoc.getIDocumentDrawModelAccess().GetDrawModel()->SetRefDevice( getReferenceDevice( false ) );
            }

            pFntCache->Flush();

            for(SwRootFrame* aLayout : m_rDoc.GetAllLayouts())
                aLayout->InvalidateAllContent(SwInvalidateFlags::Size);

            for(SwViewShell& rShell : pSh->GetRingContainer())
                rShell.InitPrt(getPrinter(false));
        }
    }
    if ( bDraw && m_rDoc.getIDocumentDrawModelAccess().GetDrawModel() )
    {
        const bool bTmpAddExtLeading = m_rDoc.GetDocumentSettingManager().get(DocumentSettingId::ADD_EXT_LEADING);
        if ( bTmpAddExtLeading != m_rDoc.getIDocumentDrawModelAccess().GetDrawModel()->IsAddExtLeading() )
            m_rDoc.getIDocumentDrawModelAccess().GetDrawModel()->SetAddExtLeading( bTmpAddExtLeading );

        OutputDevice* pOutDev = getReferenceDevice( false );
        if ( pOutDev != m_rDoc.getIDocumentDrawModelAccess().GetDrawModel()->GetRefDevice() )
            m_rDoc.getIDocumentDrawModelAccess().GetDrawModel()->SetRefDevice( pOutDev );
    }

    m_rDoc.PrtOLENotify( true );

    if ( bEndAction )
        pTmpRoot->EndAllAction();
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
