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

#include <oleembobj.hxx>
#include "olepersist.hxx"
#include <com/sun/star/embed/EmbedStates.hpp>
#include <com/sun/star/embed/EmbedVerbs.hpp>
#include <com/sun/star/embed/EntryInitModes.hpp>
#include <com/sun/star/embed/WrongStateException.hpp>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/embed/XTransactedObject.hpp>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/EmbedUpdateModes.hpp>
#include <com/sun/star/embed/Aspects.hpp>
#include <com/sun/star/embed/XOptimizedStorage.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/io/TempFile.hpp>
#include <com/sun/star/io/XSeekable.hpp>
#include <com/sun/star/io/XTruncate.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/packages/WrongPasswordException.hpp>
#include <com/sun/star/ucb/SimpleFileAccess.hpp>
#include <com/sun/star/io/IOException.hpp>

#include <comphelper/storagehelper.hxx>
#include <comphelper/mimeconfighelper.hxx>
#include <comphelper/classids.hxx>
#include <osl/diagnose.h>
#include <osl/thread.hxx>
#include <rtl/ref.hxx>
#include <sal/log.hxx>

#include <closepreventer.hxx>

#if defined(_WIN32)
#include "olecomponent.hxx"
#endif

using namespace ::com::sun::star;
using namespace ::comphelper;


bool KillFile_Impl( const OUString& aURL, const uno::Reference< uno::XComponentContext >& xContext )
{
    if ( !xContext.is() )
        return false;

    bool bRet = false;

    try
    {
        uno::Reference < ucb::XSimpleFileAccess3 > xAccess(
                ucb::SimpleFileAccess::create( xContext ) );

        xAccess->kill( aURL );
        bRet = true;
    }
    catch( const uno::Exception& )
    {
    }

    return bRet;
}


OUString GetNewTempFileURL_Impl( const uno::Reference< uno::XComponentContext >& xContext )
{
    SAL_WARN_IF( !xContext.is(), "embeddedobj.ole", "No factory is provided!" );

    OUString aResult;

    uno::Reference < io::XTempFile > xTempFile(
            io::TempFile::create(xContext),
            uno::UNO_SET_THROW );

    try {
        xTempFile->setRemoveFile( false );
        aResult = xTempFile->getUri();
    }
    catch ( const uno::Exception& )
    {
    }

    if ( aResult.isEmpty() )
        throw uno::RuntimeException(u"Cannot create tempfile."_ustr);

    return aResult;
}


OUString GetNewFilledTempFile_Impl( const uno::Reference< io::XInputStream >& xInStream,
                                      const uno::Reference< uno::XComponentContext >& xContext )
{
    OSL_ENSURE( xInStream.is() && xContext.is(), "Wrong parameters are provided!" );

    OUString aResult = GetNewTempFileURL_Impl( xContext );

    if ( !aResult.isEmpty() )
    {
        try {
            uno::Reference < ucb::XSimpleFileAccess3 > xTempAccess(
                    ucb::SimpleFileAccess::create( xContext ) );

            uno::Reference< io::XOutputStream > xTempOutStream = xTempAccess->openFileWrite( aResult );
            if ( !xTempOutStream.is() )
                throw io::IOException(); // TODO:
            // copy stream contents to the file
            ::comphelper::OStorageHelper::CopyInputToOutput( xInStream, xTempOutStream );
            xTempOutStream->closeOutput();
            xTempOutStream.clear();
        }
        catch( const packages::WrongPasswordException& )
        {
            KillFile_Impl( aResult, xContext );
            throw io::IOException(); //TODO:
        }
        catch( const io::IOException& )
        {
            KillFile_Impl( aResult, xContext );
            throw;
        }
        catch( const uno::RuntimeException& )
        {
            KillFile_Impl( aResult, xContext );
            throw;
        }
        catch( const uno::Exception& )
        {
            KillFile_Impl( aResult, xContext );
            aResult.clear();
        }
    }

    return aResult;
}
#ifdef _WIN32
/// @throws io::IOException
/// @throws uno::RuntimeException
static OUString GetNewFilledTempFile_Impl( const uno::Reference< embed::XOptimizedStorage >& xParentStorage, const OUString& aEntryName, const uno::Reference< uno::XComponentContext >& xContext )
{
    OUString aResult;

    try
    {
        uno::Reference < io::XTempFile > xTempFile(
                io::TempFile::create(xContext),
                uno::UNO_SET_THROW );

        xParentStorage->copyStreamElementData( aEntryName, xTempFile );

        xTempFile->setRemoveFile( false );
        aResult = xTempFile->getUri();
    }
    catch( const uno::RuntimeException& )
    {
        throw;
    }
    catch( const uno::Exception& )
    {
    }

    if ( aResult.isEmpty() )
        throw io::IOException();

    return aResult;
}


static void SetStreamMediaType_Impl( const uno::Reference< io::XStream >& xStream, const OUString& aMediaType )
{
    uno::Reference< beans::XPropertySet > xPropSet( xStream, uno::UNO_QUERY_THROW );
    xPropSet->setPropertyValue("MediaType", uno::Any( aMediaType ) );
}
#endif

static void LetCommonStoragePassBeUsed_Impl( const uno::Reference< io::XStream >& xStream )
{
    uno::Reference< beans::XPropertySet > xPropSet( xStream, uno::UNO_QUERY_THROW );
    xPropSet->setPropertyValue(u"UseCommonStoragePasswordEncryption"_ustr,
                                uno::Any( true ) );
}
#ifdef _WIN32

void VerbExecutionController::StartControlExecution()
{
    osl::MutexGuard aGuard( m_aVerbExecutionMutex );

    // the class is used to detect STAMPIT object, that can never be active
    if ( !m_bVerbExecutionInProgress && !m_bWasEverActive )
    {
        m_bVerbExecutionInProgress = true;
        m_nVerbExecutionThreadIdentifier = osl::Thread::getCurrentIdentifier();
        m_bChangedOnVerbExecution = false;
    }
}


bool VerbExecutionController::EndControlExecution_WasModified()
{
    osl::MutexGuard aGuard( m_aVerbExecutionMutex );

    bool bResult = false;
    if ( m_bVerbExecutionInProgress && m_nVerbExecutionThreadIdentifier == osl::Thread::getCurrentIdentifier() )
    {
        bResult = m_bChangedOnVerbExecution;
        m_bVerbExecutionInProgress = false;
    }

    return bResult;
}


void VerbExecutionController::ModificationNotificationIsDone()
{
    osl::MutexGuard aGuard( m_aVerbExecutionMutex );

    if ( m_bVerbExecutionInProgress && osl::Thread::getCurrentIdentifier() == m_nVerbExecutionThreadIdentifier )
        m_bChangedOnVerbExecution = true;
}
#endif

void VerbExecutionController::LockNotification()
{
    osl::MutexGuard aGuard( m_aVerbExecutionMutex );
    if ( m_nNotificationLock < SAL_MAX_INT32 )
        m_nNotificationLock++;
}


void VerbExecutionController::UnlockNotification()
{
    osl::MutexGuard aGuard( m_aVerbExecutionMutex );
    if ( m_nNotificationLock > 0 )
        m_nNotificationLock--;
}


uno::Reference< io::XStream > OleEmbeddedObject::GetNewFilledTempStream_Impl( const uno::Reference< io::XInputStream >& xInStream )
{
    SAL_WARN_IF( !xInStream.is(), "embeddedobj.ole", "Wrong parameter is provided!" );

    uno::Reference < io::XStream > xTempFile(
            io::TempFile::create(m_xContext),
            uno::UNO_QUERY_THROW );

    uno::Reference< io::XOutputStream > xTempOutStream = xTempFile->getOutputStream();
    if ( !xTempOutStream.is() )
        throw io::IOException(); // TODO:
    ::comphelper::OStorageHelper::CopyInputToOutput( xInStream, xTempOutStream );
    xTempOutStream->flush();
    return xTempFile;
}


uno::Reference< io::XStream > OleEmbeddedObject::TryToGetAcceptableFormat_Impl( const uno::Reference< io::XStream >& xStream )
{
    // TODO/LATER: Actually this should be done by a centralized component ( may be a graphical filter )
    if ( !m_xContext.is() )
        throw uno::RuntimeException();

    uno::Reference< io::XInputStream > xInStream = xStream->getInputStream();
    if ( !xInStream.is() )
        throw uno::RuntimeException();

    uno::Reference< io::XSeekable > xSeek( xStream, uno::UNO_QUERY_THROW );
    xSeek->seek( 0 );

    uno::Sequence< sal_Int8 > aData( 8 );
    sal_Int32 nRead = xInStream->readBytes( aData, 8 );
    xSeek->seek( 0 );

    if ( ( nRead >= 2 && aData[0] == 'B' && aData[1] == 'M' )
      || ( nRead >= 4 && aData[0] == 1 && aData[1] == 0 && aData[2] == 9 && aData[3] == 0 ) )
    {
        // it should be a bitmap or a Metafile
        return xStream;
    }


    sal_uInt32 nHeaderOffset = 0;
    if ( ( nRead >= 8 && aData[0] == -1 && aData[1] == -1 && aData[2] == -1 && aData[3] == -1 )
      && ( aData[4] == 2 || aData[4] == 3 || aData[4] == 14 ) && aData[5] == 0 && aData[6] == 0 && aData[7] == 0 )
    {
        nHeaderOffset = 40;
        xSeek->seek( 8 );

        // TargetDevice might be used in future, currently the cache has specified NULL
        uno::Sequence< sal_Int8 > aHeadData( 4 );
        nRead = xInStream->readBytes( aHeadData, 4 );
        sal_uInt32 nLen = 0;
        if ( nRead == 4 && aHeadData.getLength() == 4 )
            nLen = ( ( ( static_cast<sal_uInt32>(aHeadData[3]) * 0x100 + static_cast<sal_uInt32>(aHeadData[2]) ) * 0x100 ) + static_cast<sal_uInt32>(aHeadData[1]) ) * 0x100 + static_cast<sal_uInt32>(aHeadData[0]);
        if ( nLen > 4 )
        {
            xInStream->skipBytes( nLen - 4 );
            nHeaderOffset += nLen - 4;
        }

    }
    else if ( nRead > 4 )
    {
        // check whether the first bytes represent the size
        sal_uInt32 nSize = 0;
        for ( sal_Int32 nInd = 3; nInd >= 0; nInd-- )
            nSize = ( nSize << 8 ) + static_cast<sal_uInt8>(aData[nInd]);

        if ( nSize == xSeek->getLength() - 4 )
            nHeaderOffset = 4;
    }

    if ( nHeaderOffset )
    {
        // this is either a bitmap or a metafile clipboard format, retrieve the pure stream
        uno::Reference < io::XStream > xResult(
            io::TempFile::create(m_xContext),
            uno::UNO_QUERY_THROW );
        uno::Reference < io::XSeekable > xResultSeek( xResult, uno::UNO_QUERY_THROW );
        uno::Reference < io::XOutputStream > xResultOut = xResult->getOutputStream();
        uno::Reference < io::XInputStream > xResultIn = xResult->getInputStream();
        if ( !xResultOut.is() || !xResultIn.is() )
            throw uno::RuntimeException();

        xSeek->seek( nHeaderOffset ); // header size for these formats
        ::comphelper::OStorageHelper::CopyInputToOutput( xInStream, xResultOut );
        xResultOut->closeOutput();
        xResultSeek->seek( 0 );
        xSeek->seek( 0 );

        return xResult;
    }

    return uno::Reference< io::XStream >();
}


void OleEmbeddedObject::InsertVisualCache_Impl( const uno::Reference< io::XStream >& xTargetStream,
                                                const uno::Reference< io::XStream >& xCachedVisualRepresentation,
                                                osl::ResettableMutexGuard& rGuard )
{
    OSL_ENSURE( xTargetStream.is() && xCachedVisualRepresentation.is(), "Invalid arguments!" );

    if ( !xTargetStream.is() || !xCachedVisualRepresentation.is() )
        throw uno::RuntimeException();

    uno::Sequence< uno::Any > aArgs{ uno::Any(xTargetStream),
                                     uno::Any(true) }; // do not create copy

    uno::Reference< container::XNameContainer > xNameContainer(
            m_xContext->getServiceManager()->createInstanceWithArgumentsAndContext(
                    u"com.sun.star.embed.OLESimpleStorage"_ustr,
                    aArgs, m_xContext ),
            uno::UNO_QUERY_THROW );

    uno::Reference< io::XSeekable > xCachedSeek( xCachedVisualRepresentation, uno::UNO_QUERY_THROW );
    xCachedSeek->seek( 0 );

    uno::Reference < io::XStream > xTempFile(
            io::TempFile::create(m_xContext),
            uno::UNO_QUERY_THROW );

    uno::Reference< io::XSeekable > xTempSeek( xTempFile, uno::UNO_QUERY_THROW );
    uno::Reference< io::XOutputStream > xTempOutStream = xTempFile->getOutputStream();
    if ( !xTempOutStream.is() )
        throw io::IOException(); // TODO:

    // the OlePres stream must have additional header
    // TODO/LATER: might need to be extended in future (actually makes sense only for SO7 format)
    uno::Reference< io::XInputStream > xInCacheStream = xCachedVisualRepresentation->getInputStream();
    if ( !xInCacheStream.is() )
        throw uno::RuntimeException();

    // write 0xFFFFFFFF at the beginning
    uno::Sequence< sal_Int8 > aData( 4 );
    auto pData = aData.getArray();
    * reinterpret_cast<sal_uInt32*>(pData) = 0xFFFFFFFF;

    xTempOutStream->writeBytes( aData );

    // write clipboard format
    uno::Sequence< sal_Int8 > aSigData( 2 );
    xInCacheStream->readBytes( aSigData, 2 );
    if ( aSigData.getLength() < 2 )
        throw io::IOException();

    if ( aSigData[0] == 'B' && aSigData[1] == 'M' )
    {
        // it's a bitmap
        pData[0] = 0x02; pData[1] = 0; pData[2] = 0; pData[3] = 0;
    }
    else
    {
        // treat it as a metafile
        pData[0] = 0x03; pData[1] = 0; pData[2] = 0; pData[3] = 0;
    }
    xTempOutStream->writeBytes( aData );

    // write job related information
    pData[0] = 0x04; pData[1] = 0; pData[2] = 0; pData[3] = 0;
    xTempOutStream->writeBytes( aData );

    // write aspect
    pData[0] = 0x01; pData[1] = 0; pData[2] = 0; pData[3] = 0;
    xTempOutStream->writeBytes( aData );

    // write l-index
    * reinterpret_cast<sal_uInt32*>(pData) = 0xFFFFFFFF;
    xTempOutStream->writeBytes( aData );

    // write adv. flags
    pData[0] = 0x02; pData[1] = 0; pData[2] = 0; pData[3] = 0;
    xTempOutStream->writeBytes( aData );

    // write compression
    * reinterpret_cast<sal_uInt32*>(pData) = 0x0;
    xTempOutStream->writeBytes( aData );

    // get the size
    awt::Size aSize = getVisualAreaSize_impl(embed::Aspects::MSOLE_CONTENT, rGuard);
    sal_Int32 nIndex = 0;

    // write width
    for ( nIndex = 0; nIndex < 4; nIndex++ )
    {
        pData[nIndex] = static_cast<sal_Int8>( aSize.Width % 0x100 );
        aSize.Width /= 0x100;
    }
    xTempOutStream->writeBytes( aData );

    // write height
    for ( nIndex = 0; nIndex < 4; nIndex++ )
    {
        pData[nIndex] = static_cast<sal_Int8>( aSize.Height % 0x100 );
        aSize.Height /= 0x100;
    }
    xTempOutStream->writeBytes( aData );

    // write garbage, it will be overwritten by the size
    xTempOutStream->writeBytes( aData );

    // write first bytes that was used to detect the type
    xTempOutStream->writeBytes( aSigData );

    // write the rest of the stream
    ::comphelper::OStorageHelper::CopyInputToOutput( xInCacheStream, xTempOutStream );

    // write the size of the stream
    sal_Int64 nLength = xTempSeek->getLength() - 40;
    if ( nLength < 0 || nLength >= 0xFFFFFFFF )
    {
        SAL_WARN( "embeddedobj.ole", "Length is not acceptable!" );
        return;
    }
    for ( sal_Int32 nInd = 0; nInd < 4; nInd++ )
    {
        pData[nInd] = static_cast<sal_Int8>( static_cast<sal_uInt64>(nLength) % 0x100 );
        nLength /= 0x100;
    }
    xTempSeek->seek( 36 );
    xTempOutStream->writeBytes( aData );

    xTempOutStream->flush();

    xTempSeek->seek( 0 );
    if ( xCachedSeek.is() )
        xCachedSeek->seek( 0 );

    // insert the result file as replacement image
    OUString aCacheName = u"\002OlePres000"_ustr;
    if ( xNameContainer->hasByName( aCacheName ) )
        xNameContainer->replaceByName( aCacheName, uno::Any( xTempFile ) );
    else
        xNameContainer->insertByName( aCacheName, uno::Any( xTempFile ) );

    uno::Reference< embed::XTransactedObject > xTransacted( xNameContainer, uno::UNO_QUERY_THROW );
    xTransacted->commit();
}


void OleEmbeddedObject::RemoveVisualCache_Impl( const uno::Reference< io::XStream >& xTargetStream )
{
    OSL_ENSURE( xTargetStream.is(), "Invalid argument!" );
    if ( !xTargetStream.is() )
        throw uno::RuntimeException();

    uno::Sequence< uno::Any > aArgs{ uno::Any(xTargetStream),
                                     uno::Any(true) }; // do not create copy
    uno::Reference< container::XNameContainer > xNameContainer(
            m_xContext->getServiceManager()->createInstanceWithArgumentsAndContext(
                    u"com.sun.star.embed.OLESimpleStorage"_ustr,
                    aArgs, m_xContext ),
            uno::UNO_QUERY_THROW );

    for ( sal_uInt8 nInd = 0; nInd < 10; nInd++ )
    {
        OUString aStreamName =  "\002OlePres00" + OUString::number( nInd );
        if ( xNameContainer->hasByName( aStreamName ) )
            xNameContainer->removeByName( aStreamName );
    }

    uno::Reference< embed::XTransactedObject > xTransacted( xNameContainer, uno::UNO_QUERY_THROW );
    xTransacted->commit();
}


void OleEmbeddedObject::SetVisReplInStream( bool bExists )
{
    m_bVisReplInitialized = true;
    m_bVisReplInStream = bExists;
}


bool OleEmbeddedObject::HasVisReplInStream()
{
    if ( !m_bVisReplInitialized )
    {
        if ( m_xCachedVisualRepresentation.is() )
            SetVisReplInStream( true );
        else
        {
            SAL_INFO( "embeddedobj.ole", "embeddedobj (mv76033) OleEmbeddedObject::HasVisualReplInStream, analyzing" );

            uno::Reference< io::XInputStream > xStream;

            OSL_ENSURE( !m_pOleComponent || !m_aTempURL.isEmpty(), "The temporary file must exist if there is a component!" );
            if ( !m_aTempURL.isEmpty() )
            {
                try
                {
                    // open temporary file for reading
                    uno::Reference < ucb::XSimpleFileAccess3 > xTempAccess(
                            ucb::SimpleFileAccess::create( m_xContext ) );

                    xStream = xTempAccess->openFileRead( m_aTempURL );
                }
                catch( const uno::Exception& )
                {}
            }

            if ( !xStream.is() )
                xStream = m_xObjectStream->getInputStream();

            if ( xStream.is() )
            {
                bool bExists = false;

                uno::Sequence< uno::Any > aArgs{ uno::Any(xStream),
                                                 uno::Any(true) }; // do not create copy
                uno::Reference< container::XNameContainer > xNameContainer(
                        m_xContext->getServiceManager()->createInstanceWithArgumentsAndContext(
                                u"com.sun.star.embed.OLESimpleStorage"_ustr,
                                aArgs, m_xContext ),
                        uno::UNO_QUERY );

                if ( xNameContainer.is() )
                {
                    for ( sal_uInt8 nInd = 0; nInd < 10 && !bExists; nInd++ )
                    {
                        OUString aStreamName = "\002OlePres00" + OUString::number( nInd );
                        try
                        {
                            bExists = xNameContainer->hasByName( aStreamName );
                        }
                        catch( const uno::Exception& )
                        {}
                    }
                }

                SetVisReplInStream( bExists );
            }
        }
    }

    return m_bVisReplInStream;
}


uno::Reference< io::XStream > OleEmbeddedObject::TryToRetrieveCachedVisualRepresentation_Impl(
        const uno::Reference< io::XStream >& xStream,
        osl::ResettableMutexGuard& rGuard,
        bool bAllowToRepair50 )
    noexcept
{
    uno::Reference< io::XStream > xResult;

    if ( xStream.is() )
    {
        SAL_INFO( "embeddedobj.ole", "embeddedobj (mv76033) OleEmbeddedObject::TryToRetrieveCachedVisualRepresentation, retrieving" );

        uno::Reference< container::XNameContainer > xNameContainer;
        uno::Sequence< uno::Any > aArgs{ uno::Any(xStream),
                                         uno::Any(true) }; // do not create copy
        try
        {
            xNameContainer.set(
                m_xContext->getServiceManager()->createInstanceWithArgumentsAndContext(
                        u"com.sun.star.embed.OLESimpleStorage"_ustr,
                        aArgs, m_xContext ),
                uno::UNO_QUERY );
        }
        catch( const uno::Exception& )
        {}

        if ( xNameContainer.is() )
        {
            for ( sal_uInt8 nInd = 0; nInd < 10; nInd++ )
            {
                OUString aStreamName =  "\002OlePres00" + OUString::number( nInd );
                uno::Reference< io::XStream > xCachedCopyStream;
                try
                {
                    if ( ( xNameContainer->getByName( aStreamName ) >>= xCachedCopyStream ) && xCachedCopyStream.is() )
                    {
                        xResult = TryToGetAcceptableFormat_Impl( xCachedCopyStream );
                        if ( xResult.is() )
                            break;
                    }
                }
                catch( const uno::Exception& )
                {}

                if ( nInd == 0 )
                {
                    // to be compatible with the old versions Ole10Native is checked after OlePress000
                    aStreamName = "\001Ole10Native";
                    try
                    {
                        if ( ( xNameContainer->getByName( aStreamName ) >>= xCachedCopyStream ) && xCachedCopyStream.is() )
                        {
                            xResult = TryToGetAcceptableFormat_Impl( xCachedCopyStream );
                            if ( xResult.is() )
                                break;
                        }
                    }
                    catch( const uno::Exception& )
                    {}
                }
            }

            try
            {
                if ( bAllowToRepair50 && !xResult.is() )
                {
                    OUString aOrigContName( u"Ole-Object"_ustr );
                    if ( xNameContainer->hasByName( aOrigContName ) )
                    {
                        uno::Reference< embed::XClassifiedObject > xClassified( xNameContainer, uno::UNO_QUERY_THROW );
                        if ( MimeConfigurationHelper::ClassIDsEqual( xClassified->getClassID(), MimeConfigurationHelper::GetSequenceClassID( SO3_OUT_CLASSID ) ) )
                        {
                            // this is an OLE object wrongly stored in 5.0 format
                            // this object must be repaired since SO7 has done it

                            uno::Reference< io::XOutputStream > xOutputStream = xStream->getOutputStream();
                            uno::Reference< io::XTruncate > xTruncate( xOutputStream, uno::UNO_QUERY_THROW );

                            uno::Reference< io::XInputStream > xOrigInputStream;
                            if ( ( xNameContainer->getByName( aOrigContName ) >>= xOrigInputStream )
                              && xOrigInputStream.is() )
                            {
                                // the provided input stream must be based on temporary medium and must be independent
                                // from the stream the storage is based on
                                uno::Reference< io::XSeekable > xOrigSeekable( xOrigInputStream, uno::UNO_QUERY );
                                if ( xOrigSeekable.is() )
                                    xOrigSeekable->seek( 0 );

                                uno::Reference< lang::XComponent > xNameContDisp( xNameContainer, uno::UNO_QUERY_THROW );
                                xNameContDisp->dispose(); // free the original stream

                                xTruncate->truncate();
                                ::comphelper::OStorageHelper::CopyInputToOutput( xOrigInputStream, xOutputStream );
                                xOutputStream->flush();

                                if ( xStream == m_xObjectStream )
                                {
                                    if ( !m_aTempURL.isEmpty() )
                                    {
                                        // this is the own stream, so the temporary URL must be cleaned if it exists
                                        KillFile_Impl( m_aTempURL, m_xContext );
                                        m_aTempURL.clear();
                                    }

#ifdef _WIN32
                                    // retry to create the component after recovering
                                    GetRidOfComponent(&rGuard);

                                    try
                                    {
                                        CreateOleComponentAndLoad_Impl();
                                        m_aClassID = m_pOleComponent->GetCLSID(); // was not set during construction
                                    }
                                    catch( const uno::Exception& )
                                    {
                                        GetRidOfComponent(&rGuard);
                                    }
#endif
                                }

                                xResult = TryToRetrieveCachedVisualRepresentation_Impl( xStream, rGuard );
                            }
                        }
                    }
                }
            }
            catch( const uno::Exception& )
            {}
        }
    }

    return xResult;
}


void OleEmbeddedObject::SwitchOwnPersistence( const uno::Reference< embed::XStorage >& xNewParentStorage,
                                              const uno::Reference< io::XStream >& xNewObjectStream,
                                              const OUString& aNewName )
{
    if ( xNewParentStorage == m_xParentStorage && aNewName == m_aEntryName )
    {
        SAL_WARN_IF( xNewObjectStream != m_xObjectStream, "embeddedobj.ole", "The streams must be the same!" );
        return;
    }

    uno::Reference<io::XSeekable> xNewSeekable(xNewObjectStream, uno::UNO_QUERY);
    if (xNewSeekable.is() && xNewSeekable->getLength() == 0)
    {
        uno::Reference<io::XSeekable> xOldSeekable(m_xObjectStream, uno::UNO_QUERY);
        if (xOldSeekable.is() && xOldSeekable->getLength() > 0)
        {
            SAL_WARN("embeddedobj.ole", "OleEmbeddedObject::SwitchOwnPersistence(stream version): "
                                        "empty new stream, reusing old one");
            uno::Reference<io::XInputStream> xInput = m_xObjectStream->getInputStream();
            uno::Reference<io::XOutputStream> xOutput = xNewObjectStream->getOutputStream();
            xOldSeekable->seek(0);
            comphelper::OStorageHelper::CopyInputToOutput(xInput, xOutput);
            xNewSeekable->seek(0);
        }
    }

    try {
        uno::Reference< lang::XComponent > xComponent( m_xObjectStream, uno::UNO_QUERY );
        OSL_ENSURE( !m_xObjectStream.is() || xComponent.is(), "Wrong stream implementation!" );
        if ( xComponent.is() )
            xComponent->dispose();
    }
    catch ( const uno::Exception& )
    {
    }

    m_xObjectStream = xNewObjectStream;
    m_xParentStorage = xNewParentStorage;
    m_aEntryName = aNewName;
}


void OleEmbeddedObject::SwitchOwnPersistence( const uno::Reference< embed::XStorage >& xNewParentStorage,
                                              const OUString& aNewName )
{
    if ( xNewParentStorage == m_xParentStorage && aNewName == m_aEntryName )
        return;

    sal_Int32 nStreamMode = m_bReadOnly ? embed::ElementModes::READ : embed::ElementModes::READWRITE;

    uno::Reference< io::XStream > xNewOwnStream = xNewParentStorage->openStreamElement( aNewName, nStreamMode );

    uno::Reference<io::XSeekable> xNewSeekable (xNewOwnStream, uno::UNO_QUERY);
    if (xNewSeekable.is() && xNewSeekable->getLength() == 0)
    {
        uno::Reference<io::XSeekable> xOldSeekable(m_xObjectStream, uno::UNO_QUERY);
        if (xOldSeekable.is() && xOldSeekable->getLength() > 0)
        {
            SAL_WARN("embeddedobj.ole", "OleEmbeddedObject::SwitchOwnPersistence: empty new stream, reusing old one");
            uno::Reference<io::XInputStream> xInput = m_xObjectStream->getInputStream();
            uno::Reference<io::XOutputStream> xOutput = xNewOwnStream->getOutputStream();
            comphelper::OStorageHelper::CopyInputToOutput(xInput, xOutput);
            xNewSeekable->seek(0);
        }
    }

    SAL_WARN_IF( !xNewOwnStream.is(), "embeddedobj.ole", "The method can not return empty reference!" );

    SwitchOwnPersistence( xNewParentStorage, xNewOwnStream, aNewName );
}

#ifdef _WIN32

bool OleEmbeddedObject::SaveObject_Impl()
{
    bool bResult = false;

    if ( m_xClientSite.is() )
    {
        try
        {
            m_xClientSite->saveObject();
            bResult = true;
        }
        catch( const uno::Exception& )
        {
        }
    }

    return bResult;
}


bool OleEmbeddedObject::OnShowWindow_Impl( bool bShow )
{
    osl::ResettableMutexGuard aGuard(m_aMutex);

    bool bResult = false;

    SAL_WARN_IF( m_nObjectState == -1, "embeddedobj.ole", "The object has no persistence!" );
    SAL_WARN_IF( m_nObjectState == embed::EmbedStates::LOADED, "embeddedobj.ole", "The object get OnShowWindow in loaded state!" );
    if ( m_nObjectState == -1 || m_nObjectState == embed::EmbedStates::LOADED )
        return false;

    // the object is either activated or deactivated
    sal_Int32 nOldState = m_nObjectState;
    if ( bShow && m_nObjectState == embed::EmbedStates::RUNNING )
    {
        m_nObjectState = embed::EmbedStates::ACTIVE;
        m_aVerbExecutionController.ObjectIsActive();

        StateChangeNotification_Impl( false, nOldState, m_nObjectState, aGuard );
    }
    else if ( !bShow && m_nObjectState == embed::EmbedStates::ACTIVE )
    {
        m_nObjectState = embed::EmbedStates::RUNNING;
        StateChangeNotification_Impl( false, nOldState, m_nObjectState, aGuard );
    }

    if ( m_xClientSite.is() )
    {
        try
        {
            ExecUnlocked([p = m_xClientSite, bShow] { p->visibilityChanged(bShow); }, aGuard);
            bResult = true;
        }
        catch( const uno::Exception& )
        {
        }
    }

    return bResult;
}


void OleEmbeddedObject::OnIconChanged_Impl()
{
    // TODO/LATER: currently this notification seems to be impossible
    // MakeEventListenerNotification_Impl( OUString( "OnIconChanged" ) );
}


void OleEmbeddedObject::OnViewChanged_Impl()
{
    osl::ResettableMutexGuard aGuard(m_aMutex);
    if ( m_bDisposed )
        throw lang::DisposedException();

    // For performance reasons the notification currently is ignored, STAMPIT object is the exception,
    // it can never be active and never call SaveObject, so it is the only way to detect that it is changed

    // ==== the STAMPIT related solution =============================
    // the following variable is used to detect whether the object was modified during verb execution
    m_aVerbExecutionController.ModificationNotificationIsDone();

    // The following things are controlled by VerbExecutionController:
    // - if the verb execution is in progress and the view is changed the object will be stored
    // after the execution, so there is no need to send the notification.
    // - the STAMPIT object can never be active.
    if (m_aVerbExecutionController.CanDoNotification() &&
            m_pOleComponent && m_nUpdateMode == embed::EmbedUpdateModes::ALWAYS_UPDATE &&
            (MimeConfigurationHelper::ClassIDsEqual(m_aClassID, MimeConfigurationHelper::GetSequenceClassID(0x852ee1c9, 0x9058, 0x44ba, 0x8c, 0x6c, 0x0c, 0x5f, 0xc6, 0x6b, 0xdb, 0x8d)) ||
             MimeConfigurationHelper::ClassIDsEqual(m_aClassID, MimeConfigurationHelper::GetSequenceClassID(0xcf1b4491, 0xbea3, 0x4c9f, 0xa7, 0x0f, 0x22, 0x1b, 0x1e, 0xca, 0xef, 0x3e)))
       )
    {
        // The view is changed while the object is in running state, save the new object
        m_xCachedVisualRepresentation.clear();
        SaveObject_Impl();
        MakeEventListenerNotification_Impl( "OnVisAreaChanged", aGuard );
    }

}


void OleEmbeddedObject::OnClosed_Impl()
{
    osl::ResettableMutexGuard aGuard(m_aMutex);
    if ( m_bDisposed )
        throw lang::DisposedException();

    if ( m_nObjectState != embed::EmbedStates::LOADED )
    {
        sal_Int32 nOldState = m_nObjectState;
        m_nObjectState = embed::EmbedStates::LOADED;
        StateChangeNotification_Impl( false, nOldState, m_nObjectState, aGuard );
    }
}


OUString OleEmbeddedObject::CreateTempURLEmpty_Impl()
{
    SAL_WARN_IF( !m_aTempURL.isEmpty(), "embeddedobj.ole", "The object has already the temporary file!" );
    m_aTempURL = GetNewTempFileURL_Impl( m_xContext );

    return m_aTempURL;
}


OUString OleEmbeddedObject::GetTempURL_Impl()
{
    if ( m_aTempURL.isEmpty() )
    {
        SAL_INFO( "embeddedobj.ole", "embeddedobj (mv76033) OleEmbeddedObject::GetTempURL_Impl, tempfile creation" );

        // if there is no temporary file, it will be created from the own entry
        uno::Reference< embed::XOptimizedStorage > xOptParStorage( m_xParentStorage, uno::UNO_QUERY );
        if ( xOptParStorage.is() )
        {
            m_aTempURL = GetNewFilledTempFile_Impl( xOptParStorage, m_aEntryName, m_xContext );
        }
        else if ( m_xObjectStream.is() )
        {
            // load object from the stream
            uno::Reference< io::XInputStream > xInStream = m_xObjectStream->getInputStream();
            if ( !xInStream.is() )
                throw io::IOException(); // TODO: access denied

            m_aTempURL = GetNewFilledTempFile_Impl( xInStream, m_xContext );
        }
    }

    return m_aTempURL;
}


void OleEmbeddedObject::CreateOleComponent_Impl(
    rtl::Reference<OleComponent> const & pOleComponent )
{
    if ( !m_pOleComponent )
    {
        m_pOleComponent = pOleComponent ? pOleComponent : new OleComponent( m_xContext, this );

        if ( !m_xClosePreventer.is() )
            m_xClosePreventer = new OClosePreventer;

        m_pOleComponent->addCloseListener( m_xClosePreventer );
    }
}


void OleEmbeddedObject::CreateOleComponentAndLoad_Impl(
    rtl::Reference<OleComponent> const & pOleComponent )
{
    if ( !m_pOleComponent )
    {
        if ( !m_xObjectStream.is() )
            throw uno::RuntimeException();

        CreateOleComponent_Impl( pOleComponent );

        // after the loading the object can appear as a link
        // will be detected later by olecomponent

        GetTempURL_Impl();
        if ( m_aTempURL.isEmpty() )
            throw uno::RuntimeException(); // TODO

        m_pOleComponent->LoadEmbeddedObject( m_aTempURL );
    }
}


void OleEmbeddedObject::CreateOleComponentFromClipboard_Impl( OleComponent* pOleComponent )
{
    if ( !m_pOleComponent )
    {
        if ( !m_xObjectStream.is() )
            throw uno::RuntimeException();

        CreateOleComponent_Impl( pOleComponent );

        // after the loading the object can appear as a link
        // will be detected later by olecomponent
        m_pOleComponent->CreateObjectFromClipboard();
    }
}


uno::Reference< io::XOutputStream > OleEmbeddedObject::GetStreamForSaving()
{
    if ( !m_xObjectStream.is() )
        throw uno::RuntimeException(); //TODO:

    uno::Reference< io::XOutputStream > xOutStream = m_xObjectStream->getOutputStream();
    if ( !xOutStream.is() )
        throw io::IOException(); //TODO: access denied

    uno::Reference< io::XTruncate > xTruncate( xOutStream, uno::UNO_QUERY_THROW );
    xTruncate->truncate();

    return xOutStream;
}


void OleEmbeddedObject::StoreObjectToStream(uno::Reference<io::XOutputStream> const& xOutStream,
                                            osl::ResettableMutexGuard& rGuard)
{
    // this method should be used only on windows
    if ( m_pOleComponent )
        ExecUnlocked([this] { m_pOleComponent->StoreOwnTmpIfNecessary(); }, rGuard);

    // now all the changes should be in temporary location
    if ( m_aTempURL.isEmpty() )
        throw uno::RuntimeException();

    // open temporary file for reading
    uno::Reference < ucb::XSimpleFileAccess3 > xTempAccess(
            ucb::SimpleFileAccess::create( m_xContext ) );

    uno::Reference< io::XInputStream > xTempInStream = xTempAccess->openFileRead( m_aTempURL );
    SAL_WARN_IF( !xTempInStream.is(), "embeddedobj.ole", "The object's temporary file can not be reopened for reading!" );

    // TODO: use bStoreVisReplace

    if ( !xTempInStream.is() )
    {
        throw io::IOException(); // TODO:
    }

    // write all the contents to XOutStream
    uno::Reference< io::XTruncate > xTrunc( xOutStream, uno::UNO_QUERY_THROW );
    xTrunc->truncate();

    ::comphelper::OStorageHelper::CopyInputToOutput( xTempInStream, xOutStream );

    // TODO: should the view replacement be in the stream ???
    //       probably it must be specified on storing
}
#endif

void OleEmbeddedObject::StoreToLocation_Impl(
                            const uno::Reference< embed::XStorage >& xStorage,
                            const OUString& sEntName,
                            const uno::Sequence< beans::PropertyValue >& lObjArgs,
                            bool bSaveAs, osl::ResettableMutexGuard& rGuard)
{
#ifndef _WIN32
    (void)rGuard;
#endif
    // TODO: use lObjArgs
    // TODO: exchange StoreVisualReplacement by SO file format version?

    if ( m_nObjectState == -1 )
    {
        // the object is still not loaded
        throw embed::WrongStateException( u"Can't store object without persistence!"_ustr,
                                        static_cast< ::cppu::OWeakObject* >(this) );
    }

    if ( m_bWaitSaveCompleted )
        throw embed::WrongStateException(
                    u"The object waits for saveCompleted() call!"_ustr,
                    static_cast< ::cppu::OWeakObject* >(this) );

    OSL_ENSURE( m_xParentStorage.is() && m_xObjectStream.is(), "The object has no valid persistence!" );

    bool bVisReplIsStored = false;

    bool bTryOptimization = false;
    bool bStoreVis = m_bStoreVisRepl;
    uno::Reference< io::XStream > xCachedVisualRepresentation;
    for ( beans::PropertyValue const & prop : lObjArgs )
    {
        if ( prop.Name == "StoreVisualReplacement" )
            prop.Value >>= bStoreVis;
        else if ( prop.Name == "VisualReplacement" )
            prop.Value >>= xCachedVisualRepresentation;
        else if ( prop.Name == "CanTryOptimization" )
            prop.Value >>= bTryOptimization;
    }

    // ignore visual representation provided from outside if it should not be stored
    if ( !bStoreVis )
        xCachedVisualRepresentation.clear();

    if ( bStoreVis && !HasVisReplInStream() && !xCachedVisualRepresentation.is() )
        throw io::IOException(); // TODO: there is no cached visual representation and nothing is provided from outside

    // if the representation is provided from outside it should be copied to a local stream
    bool bNeedLocalCache = xCachedVisualRepresentation.is();

    uno::Reference< io::XStream > xTargetStream;

    bool bStoreLoaded = false;
    if ( m_nObjectState == embed::EmbedStates::LOADED
#ifdef _WIN32
        // if the object was NOT modified after storing it can be just copied
        // as if it was in loaded state
        || (m_pOleComponent && !ExecUnlocked([p = m_pOleComponent] { return p->IsDirty(); }, rGuard))
#endif
    )
    {
        bool bOptimizedCopyingDone = false;

        if ( bTryOptimization && bStoreVis == HasVisReplInStream() )
        {
            try
            {
                uno::Reference< embed::XOptimizedStorage > xSourceOptStor( m_xParentStorage, uno::UNO_QUERY_THROW );
                uno::Reference< embed::XOptimizedStorage > xTargetOptStor( xStorage, uno::UNO_QUERY_THROW );
                xSourceOptStor->copyElementDirectlyTo( m_aEntryName, xTargetOptStor, sEntName );
                bOptimizedCopyingDone = true;
            }
            catch( const uno::Exception& )
            {
            }
        }

        if ( !bOptimizedCopyingDone )
        {
            // if optimized copying fails a normal one should be tried
            m_xParentStorage->copyElementTo( m_aEntryName, xStorage, sEntName );
        }

        // the locally retrieved representation is always preferable
        // since the object is in loaded state the representation is unchanged
        if ( m_xCachedVisualRepresentation.is() )
        {
            xCachedVisualRepresentation = m_xCachedVisualRepresentation;
            bNeedLocalCache = false;
        }

        bVisReplIsStored = HasVisReplInStream();
        bStoreLoaded = true;
    }
#ifdef _WIN32
    else if ( m_pOleComponent )
    {
        xTargetStream =
                xStorage->openStreamElement( sEntName, embed::ElementModes::READWRITE );
        if ( !xTargetStream.is() )
            throw io::IOException(); //TODO: access denied

        SetStreamMediaType_Impl( xTargetStream, "application/vnd.sun.star.oleobject" );
        uno::Reference< io::XOutputStream > xOutStream = xTargetStream->getOutputStream();
        if ( !xOutStream.is() )
            throw io::IOException(); //TODO: access denied

        StoreObjectToStream(xOutStream, rGuard);
        bVisReplIsStored = true;

        if ( bSaveAs )
        {
            // no need to do it on StoreTo since in this case the replacement is in the stream
            // and there is no need to cache it even if it is thrown away because the object
            // is not changed by StoreTo action

            uno::Reference< io::XStream > xTmpCVRepresentation =
                        TryToRetrieveCachedVisualRepresentation_Impl( xTargetStream, rGuard );

            // the locally retrieved representation is always preferable
            if ( xTmpCVRepresentation.is() )
            {
                xCachedVisualRepresentation = xTmpCVRepresentation;
                bNeedLocalCache = false;
            }
        }
    }
#endif
    else if (true) // loplugin:flatten
    {
        throw io::IOException(); // TODO
    }

    if ( !xTargetStream.is() )
    {
        xTargetStream =
            xStorage->openStreamElement( sEntName, embed::ElementModes::READWRITE );
        if ( !xTargetStream.is() )
            throw io::IOException(); //TODO: access denied
    }

    LetCommonStoragePassBeUsed_Impl( xTargetStream );

    if ( bStoreVis != bVisReplIsStored )
    {
        if ( bStoreVis )
        {
            if ( !xCachedVisualRepresentation.is() )
                xCachedVisualRepresentation = TryToRetrieveCachedVisualRepresentation_Impl( xTargetStream, rGuard );

            SAL_WARN_IF( !xCachedVisualRepresentation.is(), "embeddedobj.ole", "No representation is available!" );

            // the following copying will be done in case it is SaveAs anyway
            // if it is not SaveAs the seekable access is not required currently
            // TODO/LATER: may be required in future
            if ( bSaveAs )
            {
                uno::Reference< io::XSeekable > xCachedSeek( xCachedVisualRepresentation, uno::UNO_QUERY );
                if ( !xCachedSeek.is() )
                {
                    xCachedVisualRepresentation
                        = GetNewFilledTempStream_Impl( xCachedVisualRepresentation->getInputStream() );
                    bNeedLocalCache = false;
                }
            }

            InsertVisualCache_Impl(xTargetStream, xCachedVisualRepresentation, rGuard);
        }
        else
        {
            // the removed representation could be cached by this method
            if ( !xCachedVisualRepresentation.is() )
                xCachedVisualRepresentation = TryToRetrieveCachedVisualRepresentation_Impl( xTargetStream, rGuard );

            if (!m_bStreamReadOnly)
                RemoveVisualCache_Impl(xTargetStream);
        }
    }

    if ( bSaveAs )
    {
        m_bWaitSaveCompleted = true;
        m_xNewObjectStream = std::move(xTargetStream);
        m_xNewParentStorage = xStorage;
        m_aNewEntryName = sEntName;
        m_bNewVisReplInStream = bStoreVis;
        m_bStoreLoaded = bStoreLoaded;

        if ( xCachedVisualRepresentation.is() )
        {
            if ( bNeedLocalCache )
                m_xNewCachedVisRepl = GetNewFilledTempStream_Impl( xCachedVisualRepresentation->getInputStream() );
            else
                m_xNewCachedVisRepl = std::move(xCachedVisualRepresentation);
        }

        // TODO: register listeners for storages above, in case they are disposed
        //       an exception will be thrown on saveCompleted( true )
    }
    else
    {
        uno::Reference< lang::XComponent > xComp( xTargetStream, uno::UNO_QUERY );
        if ( xComp.is() )
        {
            try {
                xComp->dispose();
            } catch( const uno::Exception& )
            {
            }
        }
    }
}


void SAL_CALL OleEmbeddedObject::setPersistentEntry(
                    const uno::Reference< embed::XStorage >& xStorage,
                    const OUString& sEntName,
                    sal_Int32 nEntryConnectionMode,
                    const uno::Sequence< beans::PropertyValue >& lArguments,
                    const uno::Sequence< beans::PropertyValue >& lObjArgs )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbedPersist > xWrappedObject( m_xWrappedObject, uno::UNO_QUERY );
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->setPersistentEntry( xStorage, sEntName, nEntryConnectionMode, lArguments, lObjArgs );
        return;
    }
    // end wrapping related part ====================

    // TODO: use lObjArgs

    // the type of the object must be already set
    // a kind of typedetection should be done in the factory;
    // the only exception is object initialized from a stream,
    // the class ID will be detected from the stream

    osl::ResettableMutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( !xStorage.is() )
        throw lang::IllegalArgumentException( u"No parent storage is provided!"_ustr,
                                            static_cast< ::cppu::OWeakObject* >(this),
                                            1 );

    if ( sEntName.isEmpty() )
        throw lang::IllegalArgumentException( u"Empty element name is provided!"_ustr,
                                            static_cast< ::cppu::OWeakObject* >(this),
                                            2 );

    // May be LOADED should be forbidden here ???
    if ( ( m_nObjectState != -1 || nEntryConnectionMode == embed::EntryInitModes::NO_INIT )
      && ( m_nObjectState == -1 || nEntryConnectionMode != embed::EntryInitModes::NO_INIT ) )
    {
        // if the object is not loaded
        // it can not get persistent representation without initialization

        // if the object is loaded
        // it can switch persistent representation only without initialization

        throw embed::WrongStateException(
                    u"Can't change persistent representation of activated object!"_ustr,
                    static_cast< ::cppu::OWeakObject* >(this) );
    }

    if ( m_bWaitSaveCompleted )
    {
        if ( nEntryConnectionMode != embed::EntryInitModes::NO_INIT )
            throw embed::WrongStateException(
                        u"The object waits for saveCompleted() call!"_ustr,
                        static_cast< ::cppu::OWeakObject* >(this) );
        saveCompleted( m_xParentStorage != xStorage || m_aEntryName != sEntName );
    }

    uno::Reference< container::XNameAccess > xNameAccess( xStorage, uno::UNO_QUERY_THROW );

    // detect entry existence
    bool bElExists = xNameAccess->hasByName( sEntName );

    m_bReadOnly = false;
    for ( beans::PropertyValue const & prop : lArguments )
        if ( prop.Name == "ReadOnly" )
            prop.Value >>= m_bReadOnly;

#ifdef _WIN32
    sal_Int32 nStorageMode = m_bReadOnly ? embed::ElementModes::READ : embed::ElementModes::READWRITE;
#endif

    SwitchOwnPersistence( xStorage, sEntName );

    for ( beans::PropertyValue const & prop : lObjArgs )
        if ( prop.Name == "StoreVisualReplacement" )
            prop.Value >>= m_bStoreVisRepl;

#ifdef _WIN32
    if ( nEntryConnectionMode == embed::EntryInitModes::DEFAULT_INIT )
    {
        if ( m_bFromClipboard )
        {
            // the object should be initialized from clipboard
            // impossibility to initialize the object means error here
            CreateOleComponentFromClipboard_Impl();
            m_aClassID = m_pOleComponent->GetCLSID(); // was not set during construction
            m_pOleComponent->RunObject();
            m_nObjectState = embed::EmbedStates::RUNNING;
        }
        else if ( bElExists )
        {
            // load object from the stream
            // after the loading the object can appear as a link
            // will be detected by olecomponent
            try
            {
                CreateOleComponentAndLoad_Impl();
                m_aClassID = m_pOleComponent->GetCLSID(); // was not set during construction
            }
            catch( const uno::Exception& )
            {
                // TODO/LATER: detect classID of the object if possible
                // means that the object inprocess server could not be successfully instantiated
                GetRidOfComponent(&aGuard);
            }

            m_nObjectState = embed::EmbedStates::LOADED;
        }
        else
        {
            // create a new object
            CreateOleComponent_Impl();
            m_pOleComponent->CreateNewEmbeddedObject( m_aClassID );
            m_pOleComponent->RunObject();
            m_nObjectState = embed::EmbedStates::RUNNING;
        }
    }
    else
    {
        if ( ( nStorageMode & embed::ElementModes::READWRITE ) != embed::ElementModes::READWRITE )
            throw io::IOException();

        if ( nEntryConnectionMode == embed::EntryInitModes::NO_INIT )
        {
            // the document just already changed its stream to store to;
            // the links to OLE documents switch their persistence in the same way
            // as normal embedded objects
        }
        else if ( nEntryConnectionMode == embed::EntryInitModes::TRUNCATE_INIT )
        {
            // create a new object, that will be stored in specified stream
            CreateOleComponent_Impl();

            m_pOleComponent->CreateNewEmbeddedObject( m_aClassID );
            m_pOleComponent->RunObject();
            m_nObjectState = embed::EmbedStates::RUNNING;
        }
        else if ( nEntryConnectionMode == embed::EntryInitModes::MEDIA_DESCRIPTOR_INIT )
        {
            // use URL ( may be content or stream later ) from MediaDescriptor to initialize object
            OUString aURL;
            for ( beans::PropertyValue const & prop : lArguments )
                if ( prop.Name == "URL" )
                    prop.Value >>= aURL;

            if ( aURL.isEmpty() )
                throw lang::IllegalArgumentException(
                                    "Empty URL is provided in the media descriptor!",
                                    static_cast< ::cppu::OWeakObject* >(this),
                                    4 );

            CreateOleComponent_Impl();

            // TODO: the m_bIsLink value must be set already
            if ( !m_bIsLink )
                m_pOleComponent->CreateObjectFromFile( aURL );
            else
                m_pOleComponent->CreateLinkFromFile( aURL );

            m_pOleComponent->RunObject();
            m_aClassID = m_pOleComponent->GetCLSID(); // was not set during construction

            m_nObjectState = embed::EmbedStates::RUNNING;
        }
        //else if ( nEntryConnectionMode == embed::EntryInitModes::TRANSFERABLE_INIT )
        //{
            //TODO:
        //}
        else
            throw lang::IllegalArgumentException( "Wrong connection mode is provided!",
                                        static_cast< ::cppu::OWeakObject* >(this),
                                        3 );
    }
#else
    // On Unix the OLE object can not do anything except storing itself somewhere
    if ( nEntryConnectionMode == embed::EntryInitModes::DEFAULT_INIT && bElExists )
    {
        // TODO/LATER: detect classID of the object
        // can be a real problem for the links

        m_nObjectState = embed::EmbedStates::LOADED;
    }
    else if ( nEntryConnectionMode == embed::EntryInitModes::NO_INIT )
    {
        // do nothing, the object has already switched it's persistence
    }
    else
        throw lang::IllegalArgumentException( u"Wrong connection mode is provided!"_ustr,
                                    static_cast< ::cppu::OWeakObject* >(this),
                                    3 );

#endif
}


void SAL_CALL OleEmbeddedObject::storeToEntry( const uno::Reference< embed::XStorage >& xStorage,
                            const OUString& sEntName,
                            const uno::Sequence< beans::PropertyValue >& lArguments,
                            const uno::Sequence< beans::PropertyValue >& lObjArgs )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbedPersist > xWrappedObject( m_xWrappedObject, uno::UNO_QUERY );
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->storeToEntry( xStorage, sEntName, lArguments, lObjArgs );
        return;
    }
    // end wrapping related part ====================

    ::osl::ResettableMutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    VerbExecutionControllerGuard aVerbGuard( m_aVerbExecutionController );

    StoreToLocation_Impl( xStorage, sEntName, lObjArgs, false, aGuard );

    // TODO: should the listener notification be done?
}


void SAL_CALL OleEmbeddedObject::storeAsEntry( const uno::Reference< embed::XStorage >& xStorage,
                            const OUString& sEntName,
                            const uno::Sequence< beans::PropertyValue >& lArguments,
                            const uno::Sequence< beans::PropertyValue >& lObjArgs )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbedPersist > xWrappedObject( m_xWrappedObject, uno::UNO_QUERY );
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->storeAsEntry( xStorage, sEntName, lArguments, lObjArgs );
        return;
    }
    // end wrapping related part ====================

    ::osl::ResettableMutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    VerbExecutionControllerGuard aVerbGuard( m_aVerbExecutionController );

    StoreToLocation_Impl( xStorage, sEntName, lObjArgs, true, aGuard );

    // TODO: should the listener notification be done here or in saveCompleted?
}


void SAL_CALL OleEmbeddedObject::saveCompleted( sal_Bool bUseNew )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbedPersist > xWrappedObject( m_xWrappedObject, uno::UNO_QUERY );
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->saveCompleted( bUseNew );
        return;
    }
    // end wrapping related part ====================

    osl::ResettableMutexGuard aGuard(m_aMutex);
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_nObjectState == -1 )
    {
        // the object is still not loaded
        throw embed::WrongStateException( u"Can't store object without persistence!"_ustr,
                                        static_cast< ::cppu::OWeakObject* >(this) );
    }

    // it is allowed to call saveCompleted( false ) for nonstored objects
    if ( !m_bWaitSaveCompleted && !bUseNew )
        return;

    SAL_WARN_IF( !m_bWaitSaveCompleted, "embeddedobj.ole", "Unexpected saveCompleted() call!" );
    if ( !m_bWaitSaveCompleted )
        throw io::IOException(); // TODO: illegal call

    OSL_ENSURE( m_xNewObjectStream.is() && m_xNewParentStorage.is() , "Internal object information is broken!" );
    if ( !m_xNewObjectStream.is() || !m_xNewParentStorage.is() )
        throw uno::RuntimeException(); // TODO: broken internal information

    if ( bUseNew )
    {
        SwitchOwnPersistence( m_xNewParentStorage, m_xNewObjectStream, m_aNewEntryName );
        m_bStoreVisRepl = m_bNewVisReplInStream;
        SetVisReplInStream( m_bNewVisReplInStream );
        m_xCachedVisualRepresentation = m_xNewCachedVisRepl;
    }
    else
    {
        // close remembered stream
        try {
            uno::Reference< lang::XComponent > xComponent( m_xNewObjectStream, uno::UNO_QUERY );
            SAL_WARN_IF( !xComponent.is(), "embeddedobj.ole", "Wrong storage implementation!" );
            if ( xComponent.is() )
                xComponent->dispose();
        }
        catch ( const uno::Exception& )
        {
        }
    }

    bool bStoreLoaded = m_bStoreLoaded;

    m_xNewObjectStream.clear();
    m_xNewParentStorage.clear();
    m_aNewEntryName.clear();
    m_bWaitSaveCompleted = false;
    m_bNewVisReplInStream = false;
    m_xNewCachedVisRepl.clear();
    m_bStoreLoaded = false;

    if ( bUseNew && m_pOleComponent && m_nUpdateMode == embed::EmbedUpdateModes::ALWAYS_UPDATE && !bStoreLoaded
      && m_nObjectState != embed::EmbedStates::LOADED )
    {
        // the object replacement image should be updated, so the cached size as well
        m_bHasCachedSize = false;
        try
        {
            // the call will cache the size in case of success
            // probably it might need to be done earlier, while the object is in active state
            getVisualAreaSize_impl(embed::Aspects::MSOLE_CONTENT, aGuard);
        }
        catch( const uno::Exception& )
        {}
    }

    if ( bUseNew )
    {
        MakeEventListenerNotification_Impl( u"OnSaveAsDone"_ustr, aGuard);

        // the object can be changed only on windows
        // the notification should be done only if the object is not in loaded state
        if ( m_pOleComponent && m_nUpdateMode == embed::EmbedUpdateModes::ALWAYS_UPDATE && !bStoreLoaded )
        {
            MakeEventListenerNotification_Impl( u"OnVisAreaChanged"_ustr, aGuard);
        }
    }
}


sal_Bool SAL_CALL OleEmbeddedObject::hasEntry()
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbedPersist > xWrappedObject( m_xWrappedObject, uno::UNO_QUERY );
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        return xWrappedObject->hasEntry();
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_bWaitSaveCompleted )
        throw embed::WrongStateException(
                    u"The object waits for saveCompleted() call!"_ustr,
                    static_cast< ::cppu::OWeakObject* >(this) );

    if ( m_xObjectStream.is() )
        return true;

    return false;
}


OUString SAL_CALL OleEmbeddedObject::getEntryName()
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbedPersist > xWrappedObject( m_xWrappedObject, uno::UNO_QUERY );
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        return xWrappedObject->getEntryName();
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_nObjectState == -1 )
    {
        // the object is still not loaded
        throw embed::WrongStateException( u"The object persistence is not initialized!"_ustr,
                                        static_cast< ::cppu::OWeakObject* >(this) );
    }

    if ( m_bWaitSaveCompleted )
        throw embed::WrongStateException(
                    u"The object waits for saveCompleted() call!"_ustr,
                    static_cast< ::cppu::OWeakObject* >(this) );

    return m_aEntryName;
}


void SAL_CALL OleEmbeddedObject::storeOwn()
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbedPersist > xWrappedObject( m_xWrappedObject, uno::UNO_QUERY );
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->storeOwn();
        return;
    }
    // end wrapping related part ====================

    // during switching from Activated to Running and from Running to Loaded states the object will
    // ask container to store the object, the container has to make decision
    // to do so or not

    osl::ResettableMutexGuard aGuard(m_aMutex);
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    VerbExecutionControllerGuard aVerbGuard( m_aVerbExecutionController );

    if ( m_nObjectState == -1 )
    {
        // the object is still not loaded
        throw embed::WrongStateException( u"Can't store object without persistence!"_ustr,
                                    static_cast< ::cppu::OWeakObject* >(this) );
    }

    if ( m_bWaitSaveCompleted )
        throw embed::WrongStateException(
                    u"The object waits for saveCompleted() call!"_ustr,
                    static_cast< ::cppu::OWeakObject* >(this) );

    if ( m_bReadOnly )
        throw io::IOException(); // TODO: access denied

    LetCommonStoragePassBeUsed_Impl( m_xObjectStream );

    bool bStoreLoaded = true;

#ifdef _WIN32
    if ( m_nObjectState != embed::EmbedStates::LOADED && m_pOleComponent && ExecUnlocked([p = m_pOleComponent] { return p->IsDirty(); }, aGuard) )
    {
        bStoreLoaded = false;

        OSL_ENSURE( m_xParentStorage.is() && m_xObjectStream.is(), "The object has no valid persistence!" );

        if ( !m_xObjectStream.is() )
            throw io::IOException(); //TODO: access denied

        SetStreamMediaType_Impl( m_xObjectStream, "application/vnd.sun.star.oleobject" );
        uno::Reference< io::XOutputStream > xOutStream = m_xObjectStream->getOutputStream();
        if ( !xOutStream.is() )
            throw io::IOException(); //TODO: access denied

        // TODO: does this work for links too?
        StoreObjectToStream(GetStreamForSaving(), aGuard);

        // the replacement is changed probably, and it must be in the object stream
        if ( !m_pOleComponent->IsWorkaroundActive() )
            m_xCachedVisualRepresentation.clear();
        SetVisReplInStream( true );
    }
#endif

    if ( m_bStoreVisRepl != HasVisReplInStream() )
    {
        if ( m_bStoreVisRepl )
        {
            // the m_xCachedVisualRepresentation must be set or it should be already stored
            if ( m_xCachedVisualRepresentation.is() )
                InsertVisualCache_Impl(m_xObjectStream, m_xCachedVisualRepresentation, aGuard);
            else
            {
                m_xCachedVisualRepresentation = TryToRetrieveCachedVisualRepresentation_Impl( m_xObjectStream, aGuard );
                SAL_WARN_IF( !m_xCachedVisualRepresentation.is(), "embeddedobj.ole", "No representation is available!" );
            }
        }
        else
        {
            if ( !m_xCachedVisualRepresentation.is() )
                m_xCachedVisualRepresentation = TryToRetrieveCachedVisualRepresentation_Impl( m_xObjectStream, aGuard );
            RemoveVisualCache_Impl( m_xObjectStream );
        }

        SetVisReplInStream( m_bStoreVisRepl );
    }

    if ( m_pOleComponent && m_nUpdateMode == embed::EmbedUpdateModes::ALWAYS_UPDATE && !bStoreLoaded )
    {
        // the object replacement image should be updated, so the cached size as well
        m_bHasCachedSize = false;
        try
        {
            // the call will cache the size in case of success
            // probably it might need to be done earlier, while the object is in active state
            getVisualAreaSize_impl(embed::Aspects::MSOLE_CONTENT, aGuard);
        }
        catch( const uno::Exception& )
        {}
    }

    MakeEventListenerNotification_Impl( u"OnSaveDone"_ustr, aGuard);

    // the object can be changed only on Windows
    // the notification should be done only if the object is not in loaded state
    if ( m_pOleComponent && m_nUpdateMode == embed::EmbedUpdateModes::ALWAYS_UPDATE && !bStoreLoaded )
        MakeEventListenerNotification_Impl( u"OnVisAreaChanged"_ustr, aGuard);
}


sal_Bool SAL_CALL OleEmbeddedObject::isReadonly()
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbedPersist > xWrappedObject( m_xWrappedObject, uno::UNO_QUERY );
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        return xWrappedObject->isReadonly();
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_nObjectState == -1 )
    {
        // the object is still not loaded
        throw embed::WrongStateException( u"The object persistence is not initialized!"_ustr,
                                        static_cast< ::cppu::OWeakObject* >(this) );
    }

    if ( m_bWaitSaveCompleted )
        throw embed::WrongStateException(
                    u"The object waits for saveCompleted() call!"_ustr,
                    static_cast< ::cppu::OWeakObject* >(this) );

    return m_bReadOnly;
}


void SAL_CALL OleEmbeddedObject::reload(
                const uno::Sequence< beans::PropertyValue >& lArguments,
                const uno::Sequence< beans::PropertyValue >& lObjArgs )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XEmbedPersist > xWrappedObject( m_xWrappedObject, uno::UNO_QUERY );
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->reload( lArguments, lObjArgs );
        return;
    }
    // end wrapping related part ====================

    // TODO: use lObjArgs

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_nObjectState == -1 )
    {
        // the object is still not loaded
        throw embed::WrongStateException( u"The object persistence is not initialized!"_ustr,
                                        static_cast< ::cppu::OWeakObject* >(this) );
    }

    if ( m_bWaitSaveCompleted )
        throw embed::WrongStateException(
                    u"The object waits for saveCompleted() call!"_ustr,
                    static_cast< ::cppu::OWeakObject* >(this) );

    // TODO:
    // throw away current document
    // load new document from current storage
    // use meaningful part of lArguments
}


void SAL_CALL OleEmbeddedObject::breakLink( const uno::Reference< embed::XStorage >& xStorage,
                                                const OUString& sEntName )
{
    // begin wrapping related part ====================
    uno::Reference< embed::XLinkageSupport > xWrappedObject( m_xWrappedObject, uno::UNO_QUERY );
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        xWrappedObject->breakLink( xStorage, sEntName );
        return;
    }
    // end wrapping related part ====================

    osl::ResettableMutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( !xStorage.is() )
        throw lang::IllegalArgumentException( u"No parent storage is provided!"_ustr,
                                            static_cast< ::cppu::OWeakObject* >(this),
                                            1 );

    if ( sEntName.isEmpty() )
        throw lang::IllegalArgumentException( u"Empty element name is provided!"_ustr,
                                            static_cast< ::cppu::OWeakObject* >(this),
                                            2 );

    // TODO: The object must be at least in Running state;
    if ( !m_bIsLink || m_nObjectState == -1 || !m_pOleComponent )
    {
        // it must be a linked initialized object
        throw embed::WrongStateException(
                    u"The object is not a valid linked object!"_ustr,
                    static_cast< ::cppu::OWeakObject* >(this) );
    }

    if ( m_bReadOnly )
        throw io::IOException(); // TODO: Access denied

    if ( m_bWaitSaveCompleted )
        throw embed::WrongStateException(
                    u"The object waits for saveCompleted() call!"_ustr,
                    static_cast< ::cppu::OWeakObject* >(this) );


#ifdef _WIN32
    // TODO: create an object based on the link

    // disconnect the old temporary URL
    OUString aOldTempURL = m_aTempURL;
    m_aTempURL.clear();

    rtl::Reference<OleComponent> pNewOleComponent = new OleComponent(m_xContext, this);
    try {
        pNewOleComponent->InitEmbeddedCopyOfLink(m_pOleComponent);
    }
    catch (const uno::Exception&)
    {
        if (!m_aTempURL.isEmpty())
            KillFile_Impl(m_aTempURL, m_xContext);
        m_aTempURL = aOldTempURL;
        throw;
    }

    try {
        GetRidOfComponent(&aGuard);
    }
    catch (const uno::Exception&)
    {
        if (!m_aTempURL.isEmpty())
            KillFile_Impl(m_aTempURL, m_xContext);
        m_aTempURL = aOldTempURL;
        throw;
    }

    KillFile_Impl(aOldTempURL, m_xContext);

    CreateOleComponent_Impl(pNewOleComponent);

    if (m_xParentStorage != xStorage || !m_aEntryName.equals(sEntName))
        SwitchOwnPersistence(xStorage, sEntName);

    if (m_nObjectState != embed::EmbedStates::LOADED)
    {
        // TODO: should we activate the new object if the link was activated?

        const sal_Int32 nTargetState = m_nObjectState;
        m_nObjectState = embed::EmbedStates::LOADED;

        if (nTargetState == embed::EmbedStates::RUNNING)
            m_pOleComponent->RunObject(); // the object already was in running state, the server must be installed
        else // nTargetState == embed::EmbedStates::ACTIVE
        {
            m_pOleComponent->RunObject(); // the object already was in running state, the server must be installed
            m_pOleComponent->ExecuteVerb(embed::EmbedVerbs::MS_OLEVERB_OPEN);
        }

        m_nObjectState = nTargetState;
    }

    m_bIsLink = false;
    m_aLinkURL.clear();
#else // ! _WIN32
    throw io::IOException(); //TODO:
#endif // _WIN32
}


sal_Bool SAL_CALL  OleEmbeddedObject::isLink()
{
    // begin wrapping related part ====================
    uno::Reference< embed::XLinkageSupport > xWrappedObject( m_xWrappedObject, uno::UNO_QUERY );
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        return xWrappedObject->isLink();
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    return m_bIsLink;
}


OUString SAL_CALL OleEmbeddedObject::getLinkURL()
{
    // begin wrapping related part ====================
    uno::Reference< embed::XLinkageSupport > xWrappedObject( m_xWrappedObject, uno::UNO_QUERY );
    if ( xWrappedObject.is() )
    {
        // the object was converted to OOo embedded object, the current implementation is now only a wrapper
        return xWrappedObject->getLinkURL();
    }
    // end wrapping related part ====================

    ::osl::MutexGuard aGuard( m_aMutex );
    if ( m_bDisposed )
        throw lang::DisposedException(); // TODO

    if ( m_bWaitSaveCompleted )
        throw embed::WrongStateException(
                    u"The object waits for saveCompleted() call!"_ustr,
                    static_cast< ::cppu::OWeakObject* >(this) );

    if ( !m_bIsLink )
        throw embed::WrongStateException(
                    u"The object is not a link object!"_ustr,
                    static_cast< ::cppu::OWeakObject* >(this) );

    // TODO: probably the link URL can be retrieved from OLE

    return m_aLinkURL;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
