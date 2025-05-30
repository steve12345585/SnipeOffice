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

#include <TableCopyHelper.hxx>
#include <core_resource.hxx>
#include <strings.hrc>
#include <strings.hxx>
#include <dbaccess/genericcontroller.hxx>
#include <com/sun/star/task/InteractionHandler.hpp>
#include <com/sun/star/task/XInteractionHandler.hpp>
#include <com/sun/star/sdb/application/CopyTableOperation.hpp>
#include <com/sun/star/sdb/application/CopyTableWizard.hpp>
#include <com/sun/star/sdb/DataAccessDescriptorFactory.hpp>
#include <com/sun/star/sdb/CommandType.hpp>

#include <TokenWriter.hxx>
#include <UITools.hxx>
#include <dbaccess/dataview.hxx>
#include <svx/dbaexchange.hxx>
#include <unotools/ucbhelper.hxx>
#include <tools/urlobj.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <sal/log.hxx>
#include <toolkit/helper/vclunohelper.hxx>
#include <unotools/tempfile.hxx>
#include <cppuhelper/exc_hlp.hxx>

namespace dbaui
{
using namespace ::dbtools;
using namespace ::svx;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::task;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::sdb;
using namespace ::com::sun::star::sdb::application;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::ucb;

OTableCopyHelper::OTableCopyHelper(OGenericUnoController* _pController)
    :m_pController(_pController)
{
}

void OTableCopyHelper::insertTable( std::u16string_view i_rSourceDataSource, const Reference<XConnection>& i_rSourceConnection,
        const OUString& i_rCommand, const sal_Int32 i_nCommandType,
        const Reference< XResultSet >& i_rSourceRows, const Sequence< Any >& i_rSelection, const bool i_bBookmarkSelection,
        std::u16string_view i_rDestDataSource, const Reference<XConnection>& i_rDestConnection)
{
    if ( CommandType::QUERY != i_nCommandType && CommandType::TABLE != i_nCommandType )
    {
        SAL_WARN("dbaccess.ui", "OTableCopyHelper::insertTable: invalid call (no supported format found)!" );
        return;
    }

    try
    {
        Reference<XConnection> xSrcConnection( i_rSourceConnection );
        if ( i_rSourceDataSource == i_rDestDataSource )
            xSrcConnection = i_rDestConnection;

        if ( !xSrcConnection.is() || !i_rDestConnection.is() )
        {
            SAL_WARN("dbaccess.ui", "OTableCopyHelper::insertTable: no connection/s!" );
            return;
        }

        Reference<XComponentContext> aContext( m_pController->getORB() );

        Reference< XDataAccessDescriptorFactory > xFactory( DataAccessDescriptorFactory::get( aContext ) );

        Reference< XPropertySet > xSource( xFactory->createDataAccessDescriptor(), UNO_SET_THROW );
        xSource->setPropertyValue( PROPERTY_COMMAND_TYPE, Any( i_nCommandType ) );
        xSource->setPropertyValue( PROPERTY_COMMAND, Any( i_rCommand ) );
        xSource->setPropertyValue( PROPERTY_ACTIVE_CONNECTION, Any( xSrcConnection ) );
        xSource->setPropertyValue( PROPERTY_RESULT_SET, Any( i_rSourceRows ) );
        xSource->setPropertyValue( PROPERTY_SELECTION, Any( i_rSelection ) );
        xSource->setPropertyValue( PROPERTY_BOOKMARK_SELECTION, Any( i_bBookmarkSelection ) );

        Reference< XPropertySet > xDest( xFactory->createDataAccessDescriptor(), UNO_SET_THROW );
        xDest->setPropertyValue( PROPERTY_ACTIVE_CONNECTION, Any( i_rDestConnection ) );

        auto xInteractionHandler = InteractionHandler::createWithParent(aContext, VCLUnoHelper::GetInterface(m_pController->getView()));

        Reference<XCopyTableWizard> xWizard(CopyTableWizard::createWithInteractionHandler(aContext, xSource, xDest, xInteractionHandler), UNO_SET_THROW);

        OUString sTableNameForAppend( GetTableNameForAppend() );
        xWizard->setDestinationTableName( GetTableNameForAppend() );

        bool bAppendToExisting = !sTableNameForAppend.isEmpty();
        xWizard->setOperation( bAppendToExisting ? CopyTableOperation::AppendData : CopyTableOperation::CopyDefinitionAndData );

        (void)xWizard->execute();
    }
    catch( const SQLException& )
    {
        m_pController->showError( SQLExceptionInfo( ::cppu::getCaughtException() ) );
    }
    catch( const Exception& )
    {
        DBG_UNHANDLED_EXCEPTION("dbaccess");
    }
}

void OTableCopyHelper::pasteTable( const svx::ODataAccessDescriptor& _rPasteData, std::u16string_view i_rDestDataSourceName,
                                  const SharedConnection& i_rDestConnection )
{
    OUString sSrcDataSourceName = _rPasteData.getDataSource();

    OUString sCommand;
    _rPasteData[ DataAccessDescriptorProperty::Command ] >>= sCommand;

    Reference<XConnection> xSrcConnection;
    if ( _rPasteData.has(DataAccessDescriptorProperty::Connection) )
    {
        OSL_VERIFY( _rPasteData[DataAccessDescriptorProperty::Connection] >>= xSrcConnection );
    }

    Reference< XResultSet > xResultSet;
    if ( _rPasteData.has(DataAccessDescriptorProperty::Cursor) )
    {
        OSL_VERIFY( _rPasteData[ DataAccessDescriptorProperty::Cursor ] >>= xResultSet );
    }

    Sequence< Any > aSelection;
    if ( _rPasteData.has( DataAccessDescriptorProperty::Selection ) )
    {
        OSL_VERIFY( _rPasteData[ DataAccessDescriptorProperty::Selection ] >>= aSelection );
        OSL_ENSURE( _rPasteData.has( DataAccessDescriptorProperty::BookmarkSelection ), "OTableCopyHelper::pasteTable: you should specify BookmarkSelection, too, to be on the safe side!" );
    }

    bool bBookmarkSelection( true );
    if ( _rPasteData.has( DataAccessDescriptorProperty::BookmarkSelection ) )
    {
        OSL_VERIFY( _rPasteData[ DataAccessDescriptorProperty::BookmarkSelection ] >>= bBookmarkSelection );
    }
    OSL_ENSURE( bBookmarkSelection, "OTableCopyHelper::pasteTable: working with selection-indices (instead of bookmarks) is error-prone, and thus deprecated!" );

    sal_Int32 nCommandType = CommandType::COMMAND;
    if ( _rPasteData.has(DataAccessDescriptorProperty::CommandType) )
        _rPasteData[DataAccessDescriptorProperty::CommandType] >>= nCommandType;

    insertTable( sSrcDataSourceName, xSrcConnection, sCommand, nCommandType,
                 xResultSet, aSelection, bBookmarkSelection,
                 i_rDestDataSourceName, i_rDestConnection );
}

void OTableCopyHelper::pasteTable( SotClipboardFormatId _nFormatId
                                  ,const TransferableDataHelper& _rTransData
                                  ,std::u16string_view i_rDestDataSource
                                  ,const SharedConnection& _xConnection)
{
    if ( _nFormatId == SotClipboardFormatId::DBACCESS_TABLE || _nFormatId == SotClipboardFormatId::DBACCESS_QUERY )
    {
        if ( ODataAccessObjectTransferable::canExtractObjectDescriptor(_rTransData.GetDataFlavorExVector()) )
        {
            svx::ODataAccessDescriptor aPasteData = ODataAccessObjectTransferable::extractObjectDescriptor(_rTransData);
            pasteTable( aPasteData,i_rDestDataSource,_xConnection);
        }
    }
    else if ( _rTransData.HasFormat(_nFormatId) )
    {
        try
        {
            DropDescriptor aTrans;
            if ( _nFormatId != SotClipboardFormatId::RTF )
                aTrans.aHtmlRtfStorage = _rTransData.GetSotStorageStream(SotClipboardFormatId::HTML);
            else
                aTrans.aHtmlRtfStorage = _rTransData.GetSotStorageStream(SotClipboardFormatId::RTF);

            aTrans.nType            = E_TABLE;
            aTrans.bHtml            = SotClipboardFormatId::HTML == _nFormatId;
            aTrans.sDefaultTableName = GetTableNameForAppend();
            if ( !aTrans.aHtmlRtfStorage || !copyTagTable(aTrans,false,_xConnection) )
                m_pController->showError(SQLException(DBA_RES(STR_NO_TABLE_FORMAT_INSIDE), *m_pController, u"S1000"_ustr, 0, Any()));
        }
        catch(const SQLException&)
        {
            m_pController->showError( SQLExceptionInfo( ::cppu::getCaughtException() ) );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("dbaccess");
        }
    }
    else
        m_pController->showError(SQLException(DBA_RES(STR_NO_TABLE_FORMAT_INSIDE), *m_pController, u"S1000"_ustr, 0, Any()));
}

void OTableCopyHelper::pasteTable( const TransferableDataHelper& _rTransData
                                  ,std::u16string_view i_rDestDataSource
                                  ,const SharedConnection& _xConnection)
{
    if ( _rTransData.HasFormat(SotClipboardFormatId::DBACCESS_TABLE) || _rTransData.HasFormat(SotClipboardFormatId::DBACCESS_QUERY) )
        pasteTable( SotClipboardFormatId::DBACCESS_TABLE,_rTransData,i_rDestDataSource,_xConnection);
    else if ( _rTransData.HasFormat(SotClipboardFormatId::HTML) )
        pasteTable( SotClipboardFormatId::HTML,_rTransData,i_rDestDataSource,_xConnection);
    else if ( _rTransData.HasFormat(SotClipboardFormatId::RTF) )
        pasteTable( SotClipboardFormatId::RTF,_rTransData,i_rDestDataSource,_xConnection);
}

bool OTableCopyHelper::copyTagTable(OTableCopyHelper::DropDescriptor const & _rDesc, bool _bCheck, const SharedConnection& _xConnection)
{
    rtl::Reference<ODatabaseImportExport> pImport;
    if ( _rDesc.bHtml )
        pImport = new OHTMLImportExport(_xConnection,getNumberFormatter(_xConnection, m_pController->getORB()),m_pController->getORB());
    else
        pImport = new ORTFImportExport(_xConnection,getNumberFormatter(_xConnection, m_pController->getORB()),m_pController->getORB());

    SvStream* pStream = _rDesc.aHtmlRtfStorage.get();
    if ( _bCheck )
        pImport->enableCheckOnly();

    //set the selected tablename
    pImport->setSTableName(_rDesc.sDefaultTableName);

    pImport->setStream(pStream);
    return pImport->Read();
}

bool OTableCopyHelper::isTableFormat(const TransferableDataHelper& _rClipboard)
{
    bool bTableFormat   =   _rClipboard.HasFormat(SotClipboardFormatId::DBACCESS_TABLE)
                ||  _rClipboard.HasFormat(SotClipboardFormatId::DBACCESS_QUERY)
                ||  _rClipboard.HasFormat(SotClipboardFormatId::RTF)
                ||  _rClipboard.HasFormat(SotClipboardFormatId::HTML);

    return bTableFormat;
}

bool OTableCopyHelper::copyTagTable(const TransferableDataHelper& _aDroppedData
                                   ,DropDescriptor& _rAsyncDrop
                                   ,const SharedConnection& _xConnection)
{
    bool bRet = false;
    bool bHtml = _aDroppedData.HasFormat(SotClipboardFormatId::HTML);
    if ( bHtml || _aDroppedData.HasFormat(SotClipboardFormatId::RTF) )
    {
        _rAsyncDrop.aHtmlRtfStorage = _aDroppedData.GetSotStorageStream(bHtml ? SotClipboardFormatId::HTML : SotClipboardFormatId::RTF);

        _rAsyncDrop.bHtml           = bHtml;
        _rAsyncDrop.bError          = !copyTagTable(_rAsyncDrop,true,_xConnection);

        bRet = ( !_rAsyncDrop.bError && _rAsyncDrop.aHtmlRtfStorage );
        if ( bRet )
        {
            // now we need to copy the stream
            ::utl::TempFileNamed aTmp;
            _rAsyncDrop.aUrl = aTmp.GetURL();
            std::unique_ptr<SvStream> aNew = SotTempStream::Create( aTmp.GetFileName() );
            _rAsyncDrop.aHtmlRtfStorage->Seek(STREAM_SEEK_TO_BEGIN);
            aNew->WriteStream(*_rAsyncDrop.aHtmlRtfStorage);
            _rAsyncDrop.aHtmlRtfStorage = std::move(aNew);
        }
        else
            _rAsyncDrop.aHtmlRtfStorage = nullptr;
    }
    return bRet;
}

void OTableCopyHelper::asyncCopyTagTable(  DropDescriptor& _rDesc
                                ,std::u16string_view i_rDestDataSource
                                ,const SharedConnection& _xConnection)
{
    if ( _rDesc.aHtmlRtfStorage )
    {
        copyTagTable(_rDesc,false,_xConnection);
        _rDesc.aHtmlRtfStorage = nullptr;
        // we now have to delete the temp file created in executeDrop
        INetURLObject aURL;
        aURL.SetURL(_rDesc.aUrl);
        ::utl::UCBContentHelper::Kill(aURL.GetMainURL(INetURLObject::DecodeMechanism::NONE));
    }
    else if ( !_rDesc.bError )
        pasteTable(_rDesc.aDroppedData,i_rDestDataSource,_xConnection);
    else
        m_pController->showError(SQLException(DBA_RES(STR_NO_TABLE_FORMAT_INSIDE), *m_pController, u"S1000"_ustr, 0, Any()));
}

}   // namespace dbaui

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
