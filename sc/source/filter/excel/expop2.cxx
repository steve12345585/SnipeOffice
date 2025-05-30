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
#include <sfx2/objsh.hxx>
#include <sfx2/docinf.hxx>
#include <filter/msfilter/svxmsbas.hxx>
#include <docsh.hxx>
#include <oox/ole/vbaexport.hxx>

#include <scerrors.hxx>

#include <root.hxx>
#include <excdoc.hxx>
#include <exp_op.hxx>

#include <xehelper.hxx>

#include <officecfg/Office/Calc.hxx>
#include <officecfg/Office/Common.hxx>

#include <com/sun/star/document/XDocumentPropertiesSupplier.hpp>
#include <com/sun/star/frame/XModel.hpp>

namespace com::sun::star::document { class XDocumentProperties; }

namespace {

enum class VBAExportMode
{
    NONE,
    REEXPORT_STREAM,
    FULL_EXPORT
};

}

ExportBiff5::ExportBiff5( XclExpRootData& rExpData, SvStream& rStrm ):
    ExportTyp( rStrm ),
    XclExpRoot( rExpData )
{
    // only need part of the Root data
    pExcRoot = &GetOldRoot();
    pExcRoot->pER = this;   // ExcRoot -> XclExpRoot
    pExcRoot->eDateiTyp = Biff5;
    pExcDoc.reset( new ExcDocument( *this ) );
}

ExportBiff5::~ExportBiff5()
{
}

ErrCode ExportBiff5::Write()
{
    ScDocShell* pDocShell = GetDocShell();
    OSL_ENSURE( pDocShell, "ExportBiff5::Write - no document shell" );

    rtl::Reference<SotStorage> xRootStrg = GetRootStorage();
    OSL_ENSURE( xRootStrg.is(), "ExportBiff5::Write - no root storage" );

    VBAExportMode eVbaExportMode = VBAExportMode::NONE;
    if( GetBiff() == EXC_BIFF8 )
    {
        if (officecfg::Office::Calc::Filter::Import::VBA::UseExport::get())
            eVbaExportMode = VBAExportMode::FULL_EXPORT;
        else
        {
            if ( officecfg::Office::Calc::Filter::Import::VBA::Save::get() )
                eVbaExportMode = VBAExportMode::REEXPORT_STREAM;
        }
    }

    if ( pDocShell && xRootStrg.is() && eVbaExportMode == VBAExportMode::FULL_EXPORT)
    {
        VbaExport aExport(pDocShell->GetModel());
        if (aExport.containsVBAProject())
        {
            rtl::Reference<SotStorage> xVBARoot = xRootStrg->OpenSotStorage(u"_VBA_PROJECT_CUR"_ustr);
            aExport.exportVBA( xVBARoot.get() );
        }
    }
    else if( pDocShell && xRootStrg.is() && eVbaExportMode == VBAExportMode::REEXPORT_STREAM )
    {
        SvxImportMSVBasic aBasicImport( *pDocShell, *xRootStrg );
        const ErrCode nErr = aBasicImport.SaveOrDelMSVBAStorage( true, EXC_STORAGE_VBA_PROJECT );
        if( nErr != ERRCODE_NONE )
            pDocShell->SetError(nErr);
    }

    pExcDoc->ReadDoc();         // ScDoc -> ExcDoc
    pExcDoc->Write( aOut );     // wechstreamen

    if( pDocShell && xRootStrg.is() )
    {
        using namespace ::com::sun::star;
        uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
                static_cast<cppu::OWeakObject*>(pDocShell->GetModel()), uno::UNO_QUERY_THROW);
        uno::Reference<document::XDocumentProperties> xDocProps
                = xDPS->getDocumentProperties();
        if ( officecfg::Office::Common::Filter::Microsoft::Export::EnableExcelPreview::get() )
        {
            std::shared_ptr<GDIMetaFile> xMetaFile =
                pDocShell->GetPreviewMetaFile();
            uno::Sequence<sal_Int8> metaFile(
                sfx2::convertMetaFile(xMetaFile.get()));
            sfx2::SaveOlePropertySet( xDocProps, xRootStrg.get(), &metaFile );
        }
        else
            sfx2::SaveOlePropertySet( xDocProps, xRootStrg.get() );
    }

    const XclExpAddressConverter& rAddrConv = GetAddressConverter();
    if( rAddrConv.IsRowTruncated() )
        return SCWARN_EXPORT_MAXROW;
    if( rAddrConv.IsColTruncated() )
        return SCWARN_EXPORT_MAXCOL;
    if( rAddrConv.IsTabTruncated() )
        return SCWARN_EXPORT_MAXTAB;

    return ERRCODE_NONE;
}

ExportBiff8::ExportBiff8( XclExpRootData& rExpData, SvStream& rStrm ) :
    ExportBiff5( rExpData, rStrm )
{
    pExcRoot->eDateiTyp = Biff8;
}

ExportBiff8::~ExportBiff8()
{
}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
