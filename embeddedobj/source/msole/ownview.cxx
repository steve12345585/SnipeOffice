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

#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/awt/XTopWindow.hpp>
#include <com/sun/star/embed/XClassifiedObject.hpp>
#include <com/sun/star/io/TempFile.hpp>
#include <com/sun/star/io/XStream.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/io/XSeekable.hpp>
#include <com/sun/star/task/XInteractionHandler.hpp>
#include <com/sun/star/ucb/SimpleFileAccess.hpp>
#include <com/sun/star/util/XCloseable.hpp>

#include <com/sun/star/document/XEventBroadcaster.hpp>
#include <com/sun/star/document/XEventListener.hpp>
#include <com/sun/star/document/XTypeDetection.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <cppuhelper/implbase.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/storagehelper.hxx>
#include <comphelper/mimeconfighelper.hxx>
#include <comphelper/diagnose_ex.hxx>

#include "olepersist.hxx"
#include "ownview.hxx"


using namespace ::com::sun::star;
using namespace ::comphelper;

namespace {

class DummyHandler_Impl : public ::cppu::WeakImplHelper< task::XInteractionHandler >
{
public:
    DummyHandler_Impl() {}

    virtual void SAL_CALL handle( const uno::Reference< task::XInteractionRequest >& xRequest ) override;
};

}

void SAL_CALL DummyHandler_Impl::handle( const uno::Reference< task::XInteractionRequest >& )
{
}


// Object viewer


OwnView_Impl::OwnView_Impl( const uno::Reference< uno::XComponentContext >& xContext,
                            const uno::Reference< io::XInputStream >& xInputStream )
: m_xContext( xContext )
, m_bBusy( false )
, m_bUseNative( false )
{
    if ( !xContext.is() || !xInputStream.is() )
        throw uno::RuntimeException();

    m_aTempFileURL = GetNewFilledTempFile_Impl( xInputStream, m_xContext );
}


OwnView_Impl::~OwnView_Impl()
{
    try {
        KillFile_Impl( m_aTempFileURL, m_xContext );
    } catch( uno::Exception& ) {}

    try {
        if ( !m_aNativeTempURL.isEmpty() )
            KillFile_Impl( m_aNativeTempURL, m_xContext );
    } catch( uno::Exception& ) {}
}


bool OwnView_Impl::CreateModelFromURL( const OUString& aFileURL )
{
    bool bResult = false;

    if ( !aFileURL.isEmpty() )
    {
        try {
            uno::Reference < frame::XDesktop2 > xDocumentLoader = frame::Desktop::create(m_xContext);

            uno::Sequence< beans::PropertyValue > aArgs( m_aFilterName.isEmpty() ? 4 : 5 );
            auto pArgs = aArgs.getArray();

            pArgs[0].Name = "URL";
            pArgs[0].Value <<= aFileURL;

            pArgs[1].Name = "ReadOnly";
            pArgs[1].Value <<= true;

            pArgs[2].Name = "InteractionHandler";
            pArgs[2].Value <<= uno::Reference< task::XInteractionHandler >( new DummyHandler_Impl() );

            pArgs[3].Name = "DontEdit";
            pArgs[3].Value <<= true;

            if ( !m_aFilterName.isEmpty() )
            {
                pArgs[4].Name = "FilterName";
                pArgs[4].Value <<= m_aFilterName;
            }

            uno::Reference< frame::XModel > xModel( xDocumentLoader->loadComponentFromURL(
                                                            aFileURL,
                                                            u"_blank"_ustr,
                                                            0,
                                                            aArgs ),
                                                        uno::UNO_QUERY );

            if ( xModel.is() )
            {
                uno::Reference< document::XEventBroadcaster > xBroadCaster( xModel, uno::UNO_QUERY );
                if ( xBroadCaster.is() )
                    xBroadCaster->addEventListener( uno::Reference< document::XEventListener >(this) );

                uno::Reference< util::XCloseable > xCloseable( xModel, uno::UNO_QUERY );
                if ( xCloseable.is() )
                {
                    xCloseable->addCloseListener( uno::Reference< util::XCloseListener >(this) );

                    ::osl::MutexGuard aGuard( m_aMutex );
                    m_xModel = std::move(xModel);
                    bResult = true;
                }
            }
        }
        catch (uno::Exception const&)
        {
            TOOLS_WARN_EXCEPTION("embeddedobj.ole", "OwnView_Impl::CreateModelFromURL:");
        }
    }

    return bResult;
}


bool OwnView_Impl::CreateModel( bool bUseNative )
{
    bool bResult = false;

    try {
        bResult = CreateModelFromURL( bUseNative ? m_aNativeTempURL : m_aTempFileURL );
    }
    catch( uno::Exception& )
    {
    }

    return bResult;
}


OUString OwnView_Impl::GetFilterNameFromExtentionAndInStream(
                                                    const css::uno::Reference< css::uno::XComponentContext >& xContext,
                                                    std::u16string_view aNameWithExtention,
                                                    const uno::Reference< io::XInputStream >& xInputStream )
{
    if ( !xInputStream.is() )
        throw uno::RuntimeException();

    uno::Reference< document::XTypeDetection > xTypeDetection(
            xContext->getServiceManager()->createInstanceWithContext(u"com.sun.star.document.TypeDetection"_ustr, xContext),
            uno::UNO_QUERY_THROW );

    OUString aTypeName;

    if ( !aNameWithExtention.empty() )
    {
        OUString aURLToAnalyze = OUString::Concat("file:///") + aNameWithExtention;
        aTypeName = xTypeDetection->queryTypeByURL( aURLToAnalyze );
    }

    uno::Sequence< beans::PropertyValue > aArgs( aTypeName.isEmpty() ? 2 : 3 );
    auto pArgs = aArgs.getArray();
    pArgs[0].Name = "URL";
    pArgs[0].Value <<= u"private:stream"_ustr;
    pArgs[1].Name = "InputStream";
    pArgs[1].Value <<= xInputStream;
    if ( !aTypeName.isEmpty() )
    {
        pArgs[2].Name = "TypeName";
        pArgs[2].Value <<= aTypeName;
    }

    aTypeName = xTypeDetection->queryTypeByDescriptor( aArgs, true );

    OUString aFilterName;
    for (beans::PropertyValue const& prop : aArgs)
        if ( prop.Name == "FilterName" )
            prop.Value >>= aFilterName;

    if ( aFilterName.isEmpty() && !aTypeName.isEmpty() )
    {
        // get the default filter name for the type
        uno::Reference< container::XNameAccess > xNameAccess( xTypeDetection, uno::UNO_QUERY_THROW );
        uno::Sequence< beans::PropertyValue > aTypes;

        if ( xNameAccess.is() && ( xNameAccess->getByName( aTypeName ) >>= aTypes ) )
        {
            for (beans::PropertyValue const& prop : aTypes)
            {
                if ( prop.Name == "PreferredFilter" && ( prop.Value >>= aFilterName ) )
                {
                    prop.Value >>= aFilterName;
                    break;
                }
            }
        }
    }

    return aFilterName;
}


bool OwnView_Impl::ReadContentsAndGenerateTempFile( const uno::Reference< io::XInputStream >& xInStream,
                                                        bool bParseHeader )
{
    uno::Reference< io::XSeekable > xSeekable( xInStream, uno::UNO_QUERY_THROW );
    xSeekable->seek( 0 );

    // create m_aNativeTempURL
    OUString aNativeTempURL;
    uno::Reference < io::XTempFile > xNativeTempFile(
            io::TempFile::create(m_xContext),
            uno::UNO_SET_THROW );
    uno::Reference < io::XOutputStream > xNativeOutTemp = xNativeTempFile->getOutputStream();
    uno::Reference < io::XInputStream > xNativeInTemp = xNativeTempFile->getInputStream();
    if ( !xNativeOutTemp.is() || !xNativeInTemp.is() )
        throw uno::RuntimeException();

    try {
        xNativeTempFile->setRemoveFile( false );
        aNativeTempURL = xNativeTempFile->getUri();
    }
    catch ( uno::Exception& )
    {
    }

    bool bFailed = false;
    OUString aFileSuffix;

    if ( bParseHeader )
    {
        uno::Sequence< sal_Int8 > aReadSeq( 4 );
        // read the complete size of the Object Package
        if ( xInStream->readBytes( aReadSeq, 4 ) != 4 )
            return false;
        // read the first header ( have no idea what does this header mean )
        if ( xInStream->readBytes( aReadSeq, 2 ) != 2 || aReadSeq[0] != 2 || aReadSeq[1] != 0 )
            return false;

        // read file name
        // only extension is interesting so only subset of symbols is accepted
        do
        {
            if ( xInStream->readBytes( aReadSeq, 1 ) != 1 )
                return false;

            if (
                (aReadSeq[0] >= '0' && aReadSeq[0] <= '9') ||
                (aReadSeq[0] >= 'a' && aReadSeq[0] <= 'z') ||
                (aReadSeq[0] >= 'A' && aReadSeq[0] <= 'Z') ||
                aReadSeq[0] == '.'
               )
            {
                aFileSuffix += OUStringChar( sal_Unicode(aReadSeq[0]) );
            }

        } while( aReadSeq[0] );

        // skip url
        do
        {
            if ( xInStream->readBytes( aReadSeq, 1 ) != 1 )
                return false;
        } while( aReadSeq[0] );

        // check the next header
        if ( xInStream->readBytes( aReadSeq, 4 ) != 4
          || aReadSeq[0] || aReadSeq[1] || aReadSeq[2] != 3 || aReadSeq[3] )
            return false;

        // get the size of the next entry
        if ( xInStream->readBytes( aReadSeq, 4 ) != 4 )
            return false;

        sal_uInt32 nUrlSize = static_cast<sal_uInt8>(aReadSeq[0])
                            + static_cast<sal_uInt8>(aReadSeq[1]) * 0x100
                            + static_cast<sal_uInt8>(aReadSeq[2]) * 0x10000
                            + static_cast<sal_uInt8>(aReadSeq[3]) * 0x1000000;
        sal_Int64 nTargetPos = xSeekable->getPosition() + nUrlSize;

        xSeekable->seek( nTargetPos );

        // get the size of stored data
        if ( xInStream->readBytes( aReadSeq, 4 ) != 4 )
            return false;

        sal_uInt32 nDataSize = static_cast<sal_uInt8>(aReadSeq[0])
                            + static_cast<sal_uInt8>(aReadSeq[1]) * 0x100
                            + static_cast<sal_uInt8>(aReadSeq[2]) * 0x10000
                            + static_cast<sal_uInt8>(aReadSeq[3]) * 0x1000000;

        aReadSeq.realloc( 32000 );
        sal_uInt32 nRead = 0;
        while ( nRead < nDataSize )
        {
            sal_uInt32 nToRead = std::min<sal_uInt32>( nDataSize - nRead, 32000 );
            sal_uInt32 nLocalRead = xInStream->readBytes( aReadSeq, nToRead );


            if ( !nLocalRead )
            {
                bFailed = true;
                break;
            }
            else if ( nLocalRead == 32000 )
                xNativeOutTemp->writeBytes( aReadSeq );
            else
            {
                uno::Sequence< sal_Int8 > aToWrite( aReadSeq );
                aToWrite.realloc( nLocalRead );
                xNativeOutTemp->writeBytes( aToWrite );
            }

            nRead += nLocalRead;
        }
    }
    else
    {
        uno::Sequence< sal_Int8 > aData( 8 );
        if ( xInStream->readBytes( aData, 8 ) == 8
          && aData[0] == -1 && aData[1] == -1 && aData[2] == -1 && aData[3] == -1
          && ( aData[4] == 2 || aData[4] == 3 ) && aData[5] == 0 && aData[6] == 0 && aData[7] == 0 )
        {
            // the header has to be removed
            xSeekable->seek( 40 );
        }
        else
        {
            // the usual Ole10Native format
            xSeekable->seek( 4 );
        }

        ::comphelper::OStorageHelper::CopyInputToOutput( xInStream, xNativeOutTemp );
    }

    xNativeOutTemp->closeOutput();

    // The temporary native file is created, now the filter must be detected
    if ( !bFailed )
    {
        m_aFilterName = GetFilterNameFromExtentionAndInStream( m_xContext, aFileSuffix, xNativeInTemp );
        m_aNativeTempURL = aNativeTempURL;
    }

    return !bFailed;
}


void OwnView_Impl::CreateNative()
{
    if ( !m_aNativeTempURL.isEmpty() )
        return;

    try
    {
        uno::Reference < ucb::XSimpleFileAccess3 > xAccess(
                ucb::SimpleFileAccess::create( m_xContext ) );

        uno::Reference< io::XInputStream > xInStream = xAccess->openFileRead( m_aTempFileURL );
        if ( !xInStream.is() )
            throw uno::RuntimeException();

        uno::Sequence< uno::Any > aArgs{ uno::Any(xInStream) };
        uno::Reference< container::XNameAccess > xNameAccess(
                m_xContext->getServiceManager()->createInstanceWithArgumentsAndContext(
                        u"com.sun.star.embed.OLESimpleStorage"_ustr,
                        aArgs, m_xContext ),
                uno::UNO_QUERY_THROW );

        static constexpr OUString aSubStreamName(u"\1Ole10Native"_ustr);
        uno::Reference< embed::XClassifiedObject > xStor( xNameAccess, uno::UNO_QUERY_THROW );
        uno::Sequence< sal_Int8 > aStorClassID = xStor->getClassID();

        if ( xNameAccess->hasByName( aSubStreamName ) )
        {
            sal_uInt8 const aClassID[] =
                { 0x00, 0x03, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 };
            // coverity[overrun-buffer-arg : FALSE] - coverity has difficulty with css::uno::Sequence
            uno::Sequence< sal_Int8 > aPackageClassID( reinterpret_cast<sal_Int8 const *>(aClassID), 16 );

            uno::Reference< io::XStream > xSubStream;
            xNameAccess->getByName( aSubStreamName ) >>= xSubStream;
            if ( xSubStream.is() )
            {
                bool bOk = false;

                if ( MimeConfigurationHelper::ClassIDsEqual( aPackageClassID, aStorClassID ) )
                {
                    // the storage represents Object Package

                    bOk = ReadContentsAndGenerateTempFile( xSubStream->getInputStream(), true );

                    if ( !bOk && !m_aNativeTempURL.isEmpty() )
                    {
                        KillFile_Impl( m_aNativeTempURL, m_xContext );
                        m_aNativeTempURL.clear();
                    }
                }

                if ( !bOk )
                {
                    bOk = ReadContentsAndGenerateTempFile( xSubStream->getInputStream(), false );

                    if ( !bOk && !m_aNativeTempURL.isEmpty() )
                    {
                        KillFile_Impl( m_aNativeTempURL, m_xContext );
                        m_aNativeTempURL.clear();
                    }
                }
            }
        }
        else
        {
            // TODO/LATER: No native stream, needs a new solution
        }
    }
    catch( uno::Exception& )
    {}
}


bool OwnView_Impl::Open()
{
    bool bResult = false;

    uno::Reference< frame::XModel > xExistingModel;

    {
        ::osl::MutexGuard aGuard( m_aMutex );
        xExistingModel = m_xModel;
        if ( m_bBusy )
            return false;

        m_bBusy = true;
    }

    if ( xExistingModel.is() )
    {
        try {
            uno::Reference< frame::XController > xController = xExistingModel->getCurrentController();
            if ( xController.is() )
            {
                uno::Reference< frame::XFrame > xFrame = xController->getFrame();
                if ( xFrame.is() )
                {
                    xFrame->activate();
                    uno::Reference<awt::XTopWindow> xTopWindow( xFrame->getContainerWindow(), uno::UNO_QUERY );
                    if(xTopWindow.is())
                        xTopWindow->toFront();

                    bResult = true;
                }
            }
        }
        catch( uno::Exception& )
        {
        }
    }
    else
    {
        bResult = CreateModel( m_bUseNative );

        if ( !bResult && !m_bUseNative )
        {
            // the original storage can not be recognized
            if ( m_aNativeTempURL.isEmpty() )
            {
                // create a temporary file for the native representation if there is no
                CreateNative();
            }

            if ( !m_aNativeTempURL.isEmpty() )
            {
                bResult = CreateModel( true );
                if ( bResult )
                    m_bUseNative = true;
            }
        }
    }

    m_bBusy = false;

    return bResult;
}


void OwnView_Impl::Close()
{
    uno::Reference< frame::XModel > xModel;

    {
        ::osl::MutexGuard aGuard( m_aMutex );
        if ( !m_xModel.is() )
            return;
        xModel = m_xModel;
        m_xModel.clear();

        if ( m_bBusy )
            return;

        m_bBusy = true;
    }

    try {
        uno::Reference< document::XEventBroadcaster > xBroadCaster( xModel, uno::UNO_QUERY );
        if ( xBroadCaster.is() )
            xBroadCaster->removeEventListener( uno::Reference< document::XEventListener >( this ) );

        uno::Reference< util::XCloseable > xCloseable( xModel, uno::UNO_QUERY );
        if ( xCloseable.is() )
        {
            xCloseable->removeCloseListener( uno::Reference< util::XCloseListener >( this ) );
            xCloseable->close( true );
        }
    }
    catch( uno::Exception& )
    {}

    m_bBusy = false;
}


void SAL_CALL OwnView_Impl::notifyEvent( const document::EventObject& aEvent )
{

    uno::Reference< frame::XModel > xModel;

    {
        ::osl::MutexGuard aGuard( m_aMutex );
        if ( aEvent.Source == m_xModel && aEvent.EventName == "OnSaveAsDone" )
        {
            // SaveAs operation took place, so just forget the model and deregister listeners
            xModel = m_xModel;
            m_xModel.clear();
        }
    }

    if ( !xModel.is() )
        return;

    try {
        uno::Reference< document::XEventBroadcaster > xBroadCaster( xModel, uno::UNO_QUERY );
        if ( xBroadCaster.is() )
            xBroadCaster->removeEventListener( uno::Reference< document::XEventListener >( this ) );

        uno::Reference< util::XCloseable > xCloseable( xModel, uno::UNO_QUERY );
        if ( xCloseable.is() )
            xCloseable->removeCloseListener( uno::Reference< util::XCloseListener >( this ) );
    }
    catch( uno::Exception& )
    {}
}


void SAL_CALL OwnView_Impl::queryClosing( const lang::EventObject&, sal_Bool )
{
}


void SAL_CALL OwnView_Impl::notifyClosing( const lang::EventObject& Source )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    if ( Source.Source == m_xModel )
        m_xModel.clear();
}


void SAL_CALL OwnView_Impl::disposing( const lang::EventObject& Source )
{
    ::osl::MutexGuard aGuard( m_aMutex );
    if ( Source.Source == m_xModel )
        m_xModel.clear();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
