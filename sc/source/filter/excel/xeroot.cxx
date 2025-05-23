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

#include <officecfg/Office/Common.hxx>
#include <rtl/random.h>
#include <sal/log.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/frame.hxx>
#include <sfx2/sfxsids.hrc>
#include <sot/storage.hxx>
#include <svl/itemset.hxx>
#include <svl/stritem.hxx>
#include <xecontent.hxx>
#include <xeescher.hxx>
#include <xeformula.hxx>
#include <xehelper.hxx>
#include <xelink.hxx>
#include <xename.hxx>
#include <xepivot.hxx>
#include <xestyle.hxx>
#include <xeroot.hxx>
#include <xepivotxml.hxx>
#include <xedbdata.hxx>
#include <xlcontent.hxx>
#include <xlname.hxx>
#include <xllink.hxx>

#include <excrecds.hxx>
#include <tabprotection.hxx>
#include <document.hxx>
#include <docsh.hxx>

#include <formulabase.hxx>
#include <com/sun/star/sheet/FormulaOpCodeMapEntry.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>

using namespace ::com::sun::star;

// Global data ================================================================

XclExpRootData::XclExpRootData( XclBiff eBiff, SfxMedium& rMedium,
        const rtl::Reference<SotStorage>& xRootStrg, ScDocument& rDoc, rtl_TextEncoding eTextEnc ) :
    XclRootData( eBiff, rMedium, xRootStrg, rDoc, eTextEnc, true )
{
    mbRelUrl = mrMedium.IsRemote()
        ? officecfg::Office::Common::Save::URL::Internet::get()
        : officecfg::Office::Common::Save::URL::FileSystem::get();
    maStringBuf.setLength(0);
}

XclExpRootData::~XclExpRootData()
{
}

XclExpRoot::XclExpRoot( XclExpRootData& rExpRootData ) :
    XclRoot( rExpRootData ),
    mrExpData( rExpRootData )
{
}

XclExpTabInfo& XclExpRoot::GetTabInfo() const
{
    OSL_ENSURE( mrExpData.mxTabInfo, "XclExpRoot::GetTabInfo - missing object (wrong BIFF?)" );
    return *mrExpData.mxTabInfo;
}

XclExpAddressConverter& XclExpRoot::GetAddressConverter() const
{
    OSL_ENSURE( mrExpData.mxAddrConv, "XclExpRoot::GetAddressConverter - missing object (wrong BIFF?)" );
    return *mrExpData.mxAddrConv;
}

XclExpFormulaCompiler& XclExpRoot::GetFormulaCompiler() const
{
    OSL_ENSURE( mrExpData.mxFmlaComp, "XclExpRoot::GetFormulaCompiler - missing object (wrong BIFF?)" );
    return *mrExpData.mxFmlaComp;
}

XclExpProgressBar& XclExpRoot::GetProgressBar() const
{
    OSL_ENSURE( mrExpData.mxProgress, "XclExpRoot::GetProgressBar - missing object (wrong BIFF?)" );
    return *mrExpData.mxProgress;
}

XclExpSst& XclExpRoot::GetSst() const
{
    OSL_ENSURE( mrExpData.mxSst, "XclExpRoot::GetSst - missing object (wrong BIFF?)" );
    return *mrExpData.mxSst;
}

XclExpPalette& XclExpRoot::GetPalette() const
{
    OSL_ENSURE( mrExpData.mxPalette, "XclExpRoot::GetPalette - missing object (wrong BIFF?)" );
    return *mrExpData.mxPalette;
}

XclExpFontBuffer& XclExpRoot::GetFontBuffer() const
{
    OSL_ENSURE( mrExpData.mxFontBfr, "XclExpRoot::GetFontBuffer - missing object (wrong BIFF?)" );
    return *mrExpData.mxFontBfr;
}

XclExpNumFmtBuffer& XclExpRoot::GetNumFmtBuffer() const
{
    OSL_ENSURE( mrExpData.mxNumFmtBfr, "XclExpRoot::GetNumFmtBuffer - missing object (wrong BIFF?)" );
    return *mrExpData.mxNumFmtBfr;
}

XclExpXFBuffer& XclExpRoot::GetXFBuffer() const
{
    OSL_ENSURE( mrExpData.mxXFBfr, "XclExpRoot::GetXFBuffer - missing object (wrong BIFF?)" );
    return *mrExpData.mxXFBfr;
}

XclExpLinkManager& XclExpRoot::GetGlobalLinkManager() const
{
    OSL_ENSURE( mrExpData.mxGlobLinkMgr, "XclExpRoot::GetGlobalLinkManager - missing object (wrong BIFF?)" );
    return *mrExpData.mxGlobLinkMgr;
}

XclExpLinkManager& XclExpRoot::GetLocalLinkManager() const
{
    OSL_ENSURE( GetLocalLinkMgrRef(), "XclExpRoot::GetLocalLinkManager - missing object (wrong BIFF?)" );
    return *GetLocalLinkMgrRef();
}

XclExpNameManager& XclExpRoot::GetNameManager() const
{
    OSL_ENSURE( mrExpData.mxNameMgr, "XclExpRoot::GetNameManager - missing object (wrong BIFF?)" );
    return *mrExpData.mxNameMgr;
}

XclExpObjectManager& XclExpRoot::GetObjectManager() const
{
    OSL_ENSURE( mrExpData.mxObjMgr, "XclExpRoot::GetObjectManager - missing object (wrong BIFF?)" );
    return *mrExpData.mxObjMgr;
}

XclExpFilterManager& XclExpRoot::GetFilterManager() const
{
    OSL_ENSURE( mrExpData.mxFilterMgr, "XclExpRoot::GetFilterManager - missing object (wrong BIFF?)" );
    return *mrExpData.mxFilterMgr;
}

XclExpDxfs& XclExpRoot::GetDxfs() const
{
    OSL_ENSURE( mrExpData.mxDxfs, "XclExpRoot::GetDxfs - missing object ( wrong BIFF?)" );
    return *mrExpData.mxDxfs;
}

XclExpPivotTableManager& XclExpRoot::GetPivotTableManager() const
{
    OSL_ENSURE( mrExpData.mxPTableMgr, "XclExpRoot::GetPivotTableManager - missing object (wrong BIFF?)" );
    return *mrExpData.mxPTableMgr;
}

XclExpXmlPivotTableManager& XclExpRoot::GetXmlPivotTableManager()
{
    assert(mrExpData.mxXmlPTableMgr);
    return *mrExpData.mxXmlPTableMgr;
}

XclExpTablesManager& XclExpRoot::GetTablesManager()
{
    assert(mrExpData.mxTablesMgr);
    return *mrExpData.mxTablesMgr;
}

void XclExpRoot::InitializeConvert()
{
    mrExpData.mxTabInfo = std::make_shared<XclExpTabInfo>( GetRoot() );
    mrExpData.mxAddrConv = std::make_shared<XclExpAddressConverter>( GetRoot() );
    mrExpData.mxFmlaComp = std::make_shared<XclExpFormulaCompiler>( GetRoot() );
    mrExpData.mxProgress = std::make_shared<XclExpProgressBar>( GetRoot() );

    GetProgressBar().Initialize();
}

void XclExpRoot::InitializeGlobals()
{
    SetCurrScTab( SCTAB_GLOBAL );

    if( GetBiff() >= EXC_BIFF5 )
    {
        mrExpData.mxPalette = new XclExpPalette( GetRoot() );
        mrExpData.mxFontBfr = new XclExpFontBuffer( GetRoot() );
        mrExpData.mxNumFmtBfr = new XclExpNumFmtBuffer( GetRoot() );
        mrExpData.mxXFBfr = new XclExpXFBuffer( GetRoot() );
        mrExpData.mxGlobLinkMgr = new XclExpLinkManager( GetRoot() );
        mrExpData.mxNameMgr = new XclExpNameManager( GetRoot() );
    }

    if( GetBiff() == EXC_BIFF8 )
    {
        mrExpData.mxSst = new XclExpSst();
        mrExpData.mxObjMgr = std::make_shared<XclExpObjectManager>( GetRoot() );
        mrExpData.mxFilterMgr = std::make_shared<XclExpFilterManager>( GetRoot() );
        mrExpData.mxPTableMgr = std::make_shared<XclExpPivotTableManager>( GetRoot() );
        // BIFF8: only one link manager for all sheets
        mrExpData.mxLocLinkMgr = mrExpData.mxGlobLinkMgr;
        mrExpData.mxDxfs = new XclExpDxfs( GetRoot() );
    }

    if( GetOutput() == EXC_OUTPUT_XML_2007 )
    {
        mrExpData.mxXmlPTableMgr = std::make_shared<XclExpXmlPivotTableManager>(GetRoot());
        mrExpData.mxTablesMgr = std::make_shared<XclExpTablesManager>(GetRoot());

        do
        {
            ScDocument& rDoc = GetDoc();
            // Pass the model factory to OpCodeProvider, not the process
            // service factory, otherwise a FormulaOpCodeMapperObj would be
            // instantiated instead of a ScFormulaOpCodeMapperObj and the
            // ScCompiler virtuals not be called! Which would be the case with
            // the current (2013-01-24) rDoc.GetServiceManager()
            const ScDocShell* pShell = rDoc.GetDocumentShell();
            if (!pShell)
            {
                SAL_WARN( "sc", "XclExpRoot::InitializeGlobals - no object shell");
                break;
            }
            uno::Reference< lang::XComponent > xComponent = pShell->GetModel();
            if (!xComponent.is())
            {
                SAL_WARN( "sc", "XclExpRoot::InitializeGlobals - no component");
                break;
            }
            uno::Reference< lang::XMultiServiceFactory > xModelFactory( xComponent, uno::UNO_QUERY);
            oox::xls::OpCodeProvider aOpCodeProvider(xModelFactory, false);
            // Compiler mocks about non-matching ctor or conversion from
            // Sequence<...> to Sequence<const ...> if directly created or passed,
            // conversion through Any works around.
            uno::Any aAny( aOpCodeProvider.getOoxParserMap());
            uno::Sequence< const sheet::FormulaOpCodeMapEntry > aOpCodeMapping;
            if (!(aAny >>= aOpCodeMapping))
            {
                SAL_WARN( "sc", "XclExpRoot::InitializeGlobals - no OpCodeMap");
                break;
            }
            ScCompiler aCompiler( rDoc, ScAddress(), rDoc.GetGrammar());
            mrExpData.mxOpCodeMap = formula::FormulaCompiler::CreateOpCodeMap( aOpCodeMapping, true);
        } while(false);
    }

    GetXFBuffer().Initialize();
    GetNameManager().Initialize();
}

void XclExpRoot::InitializeTable( SCTAB nScTab )
{
    SetCurrScTab( nScTab );
    if( GetBiff() == EXC_BIFF5 )
    {
        // local link manager per sheet
        mrExpData.mxLocLinkMgr = new XclExpLinkManager( GetRoot() );
    }
}

void XclExpRoot::InitializeSave()
{
    GetPalette().Finalize();
    GetXFBuffer().Finalize();
    GetDxfs().Finalize();
}

XclExpRecordRef XclExpRoot::CreateRecord( sal_uInt16 nRecId ) const
{
    XclExpRecordRef xRec;
    switch( nRecId )
    {
        case EXC_ID_PALETTE:        xRec = mrExpData.mxPalette;     break;
        case EXC_ID_FONTLIST:       xRec = mrExpData.mxFontBfr;     break;
        case EXC_ID_FORMATLIST:     xRec = mrExpData.mxNumFmtBfr;   break;
        case EXC_ID_XFLIST:         xRec = mrExpData.mxXFBfr;       break;
        case EXC_ID_SST:            xRec = mrExpData.mxSst;         break;
        case EXC_ID_EXTERNSHEET:    xRec = GetLocalLinkMgrRef();    break;
        case EXC_ID_NAME:           xRec = mrExpData.mxNameMgr;     break;
        case EXC_ID_DXFS:           xRec = mrExpData.mxDxfs;        break;
    }
    OSL_ENSURE( xRec, "XclExpRoot::CreateRecord - unknown record ID or missing object" );
    return xRec;
}

bool XclExpRoot::IsDocumentEncrypted() const
{
    // We need to encrypt the content when the document structure is protected.
    const ScDocProtection* pDocProt = GetDoc().GetDocProtection();
    if (pDocProt && pDocProt->isProtected() && pDocProt->isOptionEnabled(ScDocProtection::STRUCTURE))
        return true;

    // Whether password is entered directly into the save dialog.
    return GetEncryptionData().hasElements();
}

uno::Sequence< beans::NamedValue > XclExpRoot::GenerateEncryptionData( std::u16string_view aPass )
{
    uno::Sequence< beans::NamedValue > aEncryptionData;

    if ( !aPass.empty() && aPass.size() < 16 )
    {
        sal_uInt8 pnDocId[16];
        if (rtl_random_getBytes(nullptr, pnDocId, 16) != rtl_Random_E_None)
        {
            throw uno::RuntimeException(u"rtl_random_getBytes failed"_ustr);
        }

        sal_uInt16 pnPasswd[16] = {};
        for( size_t nChar = 0; nChar < aPass.size(); ++nChar )
            pnPasswd[nChar] = aPass[nChar];

        ::msfilter::MSCodec_Std97 aCodec;
        aCodec.InitKey( pnPasswd, pnDocId );
        aEncryptionData = aCodec.GetEncryptionData();
    }

    return aEncryptionData;
}

uno::Sequence< beans::NamedValue > XclExpRoot::GetEncryptionData() const
{
    uno::Sequence< beans::NamedValue > aEncryptionData;
    const SfxUnoAnyItem* pEncryptionDataItem = GetMedium().GetItemSet().GetItem(SID_ENCRYPTIONDATA, false);
    if ( pEncryptionDataItem )
        pEncryptionDataItem->GetValue() >>= aEncryptionData;
    else
    {
        // try to get the encryption data from the password
        const SfxStringItem* pPasswordItem = GetMedium().GetItemSet().GetItem(SID_PASSWORD, false);
        if ( pPasswordItem && !pPasswordItem->GetValue().isEmpty() )
            aEncryptionData = GenerateEncryptionData( pPasswordItem->GetValue() );
    }

    return aEncryptionData;
}

uno::Sequence< beans::NamedValue > XclExpRoot::GenerateDefaultEncryptionData()
{
    return GenerateEncryptionData( GetDefaultPassword() );
}

XclExpRootData::XclExpLinkMgrRef const & XclExpRoot::GetLocalLinkMgrRef() const
{
    return IsInGlobals() ? mrExpData.mxGlobLinkMgr : mrExpData.mxLocLinkMgr;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
