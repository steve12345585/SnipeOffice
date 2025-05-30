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


/**************************************************************************
                                TODO
 **************************************************************************

 *************************************************************************/

#include <sal/config.h>

#include <string_view>

#include <o3tl/string_view.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <rtl/ustrbuf.hxx>
#include <com/sun/star/beans/IllegalTypeException.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/embed/InvalidStorageException.hpp>
#include <com/sun/star/embed/StorageWrappedTargetException.hpp>
#include <com/sun/star/embed/XTransactedObject.hpp>
#include <com/sun/star/io/BufferSizeExceededException.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/io/NotConnectedException.hpp>
#include <com/sun/star/io/XActiveDataSink.hpp>
#include <com/sun/star/io/XActiveDataStreamer.hpp>
#include <com/sun/star/lang/IllegalAccessException.hpp>
#include <com/sun/star/packages/WrongPasswordException.hpp>
#include <com/sun/star/task/DocumentPasswordRequest.hpp>
#include <com/sun/star/task/XInteractionPassword.hpp>
#include <com/sun/star/ucb/CommandFailedException.hpp>
#include <com/sun/star/ucb/ContentAction.hpp>
#include <com/sun/star/ucb/ContentInfoAttribute.hpp>
#include <com/sun/star/ucb/IllegalIdentifierException.hpp>
#include <com/sun/star/ucb/InsertCommandArgument.hpp>
#include <com/sun/star/ucb/InteractiveBadTransferURLException.hpp>
#include <com/sun/star/ucb/MissingInputStreamException.hpp>
#include <com/sun/star/ucb/NameClash.hpp>
#include <com/sun/star/ucb/NameClashException.hpp>
#include <com/sun/star/ucb/OpenCommandArgument2.hpp>
#include <com/sun/star/ucb/OpenMode.hpp>
#include <com/sun/star/ucb/TransferInfo.hpp>
#include <com/sun/star/ucb/UnsupportedCommandException.hpp>
#include <com/sun/star/ucb/UnsupportedDataSinkException.hpp>
#include <com/sun/star/ucb/UnsupportedNameClashException.hpp>
#include <com/sun/star/ucb/UnsupportedOpenModeException.hpp>
#include <com/sun/star/ucb/XCommandInfo.hpp>
#include <com/sun/star/ucb/XPersistentPropertySet.hpp>

#include <comphelper/propertysequence.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <ucbhelper/cancelcommandexecution.hxx>
#include <ucbhelper/contentidentifier.hxx>
#include <ucbhelper/propertyvalueset.hxx>
#include <ucbhelper/macros.hxx>
#include <utility>

#include "tdoc_content.hxx"
#include "tdoc_resultset.hxx"
#include "tdoc_passwordrequest.hxx"

#include "../inc/urihelper.hxx"

using namespace com::sun::star;
using namespace tdoc_ucp;


static ContentType lcl_getContentType( std::u16string_view rType )
{
    if ( rType == TDOC_ROOT_CONTENT_TYPE )
        return ROOT;
    else if ( rType == TDOC_DOCUMENT_CONTENT_TYPE )
        return DOCUMENT;
    else if ( rType == TDOC_FOLDER_CONTENT_TYPE )
        return FOLDER;
    else if ( rType == TDOC_STREAM_CONTENT_TYPE )
        return STREAM;
    else
    {
        OSL_FAIL( "Content::Content - unsupported content type string" );
        return STREAM;
    }
}


// Content Implementation.


// static ( "virtual" ctor )
rtl::Reference<Content> Content::create(
            const uno::Reference< uno::XComponentContext >& rxContext,
            ContentProvider* pProvider,
            const uno::Reference< ucb::XContentIdentifier >& Identifier )
{
    // Fail, if resource does not exist.
    ContentProperties aProps;
    if ( !Content::loadData( pProvider,
                             Uri( Identifier->getContentIdentifier() ),
                             aProps ) )
        return nullptr;

    return new Content( rxContext, pProvider, Identifier, std::move(aProps) );
}


// static ( "virtual" ctor )
rtl::Reference<Content> Content::create(
            const uno::Reference< uno::XComponentContext >& rxContext,
            ContentProvider* pProvider,
            const uno::Reference< ucb::XContentIdentifier >& Identifier,
            const ucb::ContentInfo& Info )
{
    if ( Info.Type.isEmpty() )
        return nullptr;

    if ( Info.Type != TDOC_FOLDER_CONTENT_TYPE && Info.Type != TDOC_STREAM_CONTENT_TYPE )
    {
        OSL_FAIL( "Content::create - unsupported content type!" );
        return nullptr;
    }

    return new Content( rxContext, pProvider, Identifier, Info );
}


Content::Content(
            const uno::Reference< uno::XComponentContext > & rxContext,
            ContentProvider * pProvider,
            const uno::Reference< ucb::XContentIdentifier > & Identifier,
            ContentProperties aProps )
: ContentImplHelper( rxContext, pProvider, Identifier ),
  m_aProps(std::move( aProps )),
  m_eState( PERSISTENT ),
  m_pProvider( pProvider )
{
}


// ctor for a content just created via XContentCreator::createNewContent()
Content::Content(
            const uno::Reference< uno::XComponentContext >& rxContext,
            ContentProvider* pProvider,
            const uno::Reference< ucb::XContentIdentifier >& Identifier,
            const ucb::ContentInfo& Info )
  : ContentImplHelper( rxContext, pProvider, Identifier ),
  m_aProps( lcl_getContentType( Info.Type ), OUString() ), // no Title (yet)
  m_eState( TRANSIENT ),
  m_pProvider( pProvider )
{
}


// virtual
Content::~Content()
{
}


// XInterface methods.


// virtual
void SAL_CALL Content::acquire()
    noexcept
{
    ContentImplHelper::acquire();
}


// virtual
void SAL_CALL Content::release()
    noexcept
{
    ContentImplHelper::release();
}


// virtual
uno::Any SAL_CALL Content::queryInterface( const uno::Type & rType )
{
    uno::Any aRet = ContentImplHelper::queryInterface( rType );

    if ( !aRet.hasValue() )
    {
        aRet = cppu::queryInterface(
                rType, static_cast< ucb::XContentCreator * >( this ) );
        if ( aRet.hasValue() )
        {
            if ( !m_aProps.isContentCreator() )
                return uno::Any();
        }
    }

    return aRet;
}


// XTypeProvider methods.


XTYPEPROVIDER_COMMON_IMPL( Content );


// virtual
uno::Sequence< uno::Type > SAL_CALL Content::getTypes()
{
    if ( m_aProps.isContentCreator() )
    {
        static cppu::OTypeCollection s_aFolderTypes(
                    CPPU_TYPE_REF( lang::XTypeProvider ),
                    CPPU_TYPE_REF( lang::XServiceInfo ),
                    CPPU_TYPE_REF( lang::XComponent ),
                    CPPU_TYPE_REF( ucb::XContent ),
                    CPPU_TYPE_REF( ucb::XCommandProcessor ),
                    CPPU_TYPE_REF( beans::XPropertiesChangeNotifier ),
                    CPPU_TYPE_REF( ucb::XCommandInfoChangeNotifier ),
                    CPPU_TYPE_REF( beans::XPropertyContainer ),
                    CPPU_TYPE_REF( beans::XPropertySetInfoChangeNotifier ),
                    CPPU_TYPE_REF( container::XChild ),
                    CPPU_TYPE_REF( ucb::XContentCreator ) );

        return s_aFolderTypes.getTypes();
    }
    else
    {
        static cppu::OTypeCollection s_aDocumentTypes(
                    CPPU_TYPE_REF( lang::XTypeProvider ),
                    CPPU_TYPE_REF( lang::XServiceInfo ),
                    CPPU_TYPE_REF( lang::XComponent ),
                    CPPU_TYPE_REF( ucb::XContent ),
                    CPPU_TYPE_REF( ucb::XCommandProcessor ),
                    CPPU_TYPE_REF( beans::XPropertiesChangeNotifier ),
                    CPPU_TYPE_REF( ucb::XCommandInfoChangeNotifier ),
                    CPPU_TYPE_REF( beans::XPropertyContainer ),
                    CPPU_TYPE_REF( beans::XPropertySetInfoChangeNotifier ),
                    CPPU_TYPE_REF( container::XChild ) );

        return s_aDocumentTypes.getTypes();
    }
}


// XServiceInfo methods.


// virtual
OUString SAL_CALL Content::getImplementationName()
{
    return u"com.sun.star.comp.ucb.TransientDocumentsContent"_ustr;
}


// virtual
uno::Sequence< OUString > SAL_CALL Content::getSupportedServiceNames()
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    uno::Sequence< OUString > aSNS( 1 );

    if ( m_aProps.getType() == STREAM )
        aSNS.getArray()[ 0 ] = "com.sun.star.ucb.TransientDocumentsStreamContent";
    else if ( m_aProps.getType() == FOLDER )
        aSNS.getArray()[ 0 ] = "com.sun.star.ucb.TransientDocumentsFolderContent";
    else if ( m_aProps.getType() == DOCUMENT )
        aSNS.getArray()[ 0 ] = "com.sun.star.ucb.TransientDocumentsDocumentContent";
    else
        aSNS.getArray()[ 0 ] = "com.sun.star.ucb.TransientDocumentsRootContent";

    return aSNS;
}


// XContent methods.


// virtual
OUString SAL_CALL Content::getContentType()
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );
    return m_aProps.getContentType();
}


// virtual
uno::Reference< ucb::XContentIdentifier > SAL_CALL
Content::getIdentifier()
{
    {
        osl::Guard< osl::Mutex > aGuard( m_aMutex );

        // Transient?
        if ( m_eState == TRANSIENT )
        {
            // Transient contents have no identifier.
            return uno::Reference< ucb::XContentIdentifier >();
        }
    }
    return ContentImplHelper::getIdentifier();
}


// XCommandProcessor methods.


// virtual
uno::Any SAL_CALL Content::execute(
        const ucb::Command& aCommand,
        sal_Int32 /*CommandId*/,
        const uno::Reference< ucb::XCommandEnvironment >& Environment )
{
    uno::Any aRet;

    if ( aCommand.Name == "getPropertyValues" )
    {

        // getPropertyValues


        uno::Sequence< beans::Property > Properties;
        if ( !( aCommand.Argument >>= Properties ) )
        {
            ucbhelper::cancelCommandExecution(
                uno::Any( lang::IllegalArgumentException(
                                    u"Wrong argument type!"_ustr,
                                    getXWeak(),
                                    -1 ) ),
                Environment );
            // Unreachable
        }

        aRet <<= getPropertyValues( Properties );
    }
    else if ( aCommand.Name == "setPropertyValues" )
    {

        // setPropertyValues


        uno::Sequence< beans::PropertyValue > aProperties;
        if ( !( aCommand.Argument >>= aProperties ) )
        {
            ucbhelper::cancelCommandExecution(
                uno::Any( lang::IllegalArgumentException(
                                    u"Wrong argument type!"_ustr,
                                    getXWeak(),
                                    -1 ) ),
                Environment );
            // Unreachable
        }

        if ( !aProperties.hasElements() )
        {
            ucbhelper::cancelCommandExecution(
                uno::Any( lang::IllegalArgumentException(
                                    u"No properties!"_ustr,
                                    getXWeak(),
                                    -1 ) ),
                Environment );
            // Unreachable
        }

        aRet <<= setPropertyValues( aProperties, Environment );
    }
    else if ( aCommand.Name == "getPropertySetInfo" )
    {

        // getPropertySetInfo


        aRet <<= getPropertySetInfo( Environment );
    }
    else if ( aCommand.Name == "getCommandInfo" )
    {

        // getCommandInfo


        aRet <<= getCommandInfo( Environment );
    }
    else if ( aCommand.Name == "open" )
    {

        // open


        ucb::OpenCommandArgument2 aOpenCommand;
        if ( !( aCommand.Argument >>= aOpenCommand ) )
        {
            ucbhelper::cancelCommandExecution(
                uno::Any( lang::IllegalArgumentException(
                                    u"Wrong argument type!"_ustr,
                                    getXWeak(),
                                    -1 ) ),
                Environment );
            // Unreachable
        }

        aRet = open( aOpenCommand, Environment );
    }
    else if ( aCommand.Name == "insert" )
    {

        // insert ( Supported by folders and streams only )


        ContentType eType = m_aProps.getType();
        if ( ( eType != FOLDER ) && ( eType != STREAM ) )
        {
            ucbhelper::cancelCommandExecution(
                uno::Any( ucb::UnsupportedCommandException(
                                u"insert command only supported by "
                                "folders and streams!"_ustr,
                                getXWeak() ) ),
                Environment );
            // Unreachable
        }

        if ( eType == STREAM )
        {
            Uri aUri( m_xIdentifier->getContentIdentifier() );
            Uri aParentUri( aUri.getParentUri() );
            if ( aParentUri.isDocument() )
            {
                ucbhelper::cancelCommandExecution(
                    uno::Any( ucb::UnsupportedCommandException(
                                    u"insert command not supported by "
                                    "streams that are direct children "
                                    "of document root!"_ustr,
                                    getXWeak() ) ),
                    Environment );
                // Unreachable
            }
        }

        ucb::InsertCommandArgument aArg;
        if ( !( aCommand.Argument >>= aArg ) )
        {
            ucbhelper::cancelCommandExecution(
                uno::Any( lang::IllegalArgumentException(
                                    u"Wrong argument type!"_ustr,
                                    getXWeak(),
                                    -1 ) ),
                Environment );
            // Unreachable
        }

        sal_Int32 nNameClash = aArg.ReplaceExisting
                             ? ucb::NameClash::OVERWRITE
                             : ucb::NameClash::ERROR;
        insert( aArg.Data, nNameClash, Environment );
    }
    else if ( aCommand.Name == "delete" )
    {

        // delete ( Supported by folders and streams only )


        {
            osl::MutexGuard aGuard( m_aMutex );

            ContentType eType = m_aProps.getType();
            if ( ( eType != FOLDER ) && ( eType != STREAM ) )
            {
                ucbhelper::cancelCommandExecution(
                    uno::Any( ucb::UnsupportedCommandException(
                                    u"delete command only supported by "
                                    "folders and streams!"_ustr,
                                    getXWeak() ) ),
                    Environment );
                // Unreachable
            }
        }

        bool bDeletePhysical = false;
        aCommand.Argument >>= bDeletePhysical;
        destroy( bDeletePhysical, Environment );

        // Remove own and all children's persistent data.
        if ( !removeData() )
        {
            uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
            {
                 {"Uri", uno::Any(m_xIdentifier->getContentIdentifier())}
            }));
            ucbhelper::cancelCommandExecution(
                ucb::IOErrorCode_CANT_WRITE,
                aArgs,
                Environment,
                u"Cannot remove persistent data!"_ustr,
                this );
            // Unreachable
        }

        // Remove own and all children's Additional Core Properties.
        removeAdditionalPropertySet();
    }
    else if ( aCommand.Name == "transfer" )
    {

        // transfer ( Supported by document and folders only )


        {
            osl::MutexGuard aGuard( m_aMutex );

            ContentType eType = m_aProps.getType();
            if ( ( eType != FOLDER ) && ( eType != DOCUMENT ) )
            {
                ucbhelper::cancelCommandExecution(
                    uno::Any( ucb::UnsupportedCommandException(
                                    u"transfer command only supported "
                                    "by folders and documents!"_ustr,
                                    getXWeak() ) ),
                    Environment );
                // Unreachable
            }
        }

        ucb::TransferInfo aInfo;
        if ( !( aCommand.Argument >>= aInfo ) )
        {
            OSL_FAIL( "Wrong argument type!" );
            ucbhelper::cancelCommandExecution(
                uno::Any( lang::IllegalArgumentException(
                                    u"Wrong argument type!"_ustr,
                                    getXWeak(),
                                    -1 ) ),
                Environment );
            // Unreachable
        }

        transfer( aInfo, Environment );
    }
    else if ( aCommand.Name == "createNewContent" )
    {

        // createNewContent ( Supported by document and folders only )


        {
            osl::MutexGuard aGuard( m_aMutex );

            ContentType eType = m_aProps.getType();
            if ( ( eType != FOLDER ) && ( eType != DOCUMENT ) )
            {
                ucbhelper::cancelCommandExecution(
                    uno::Any( ucb::UnsupportedCommandException(
                                    u"createNewContent command only "
                                    "supported by folders and "
                                    "documents!"_ustr,
                                    getXWeak() ) ),
                    Environment );
                // Unreachable
            }
        }

        ucb::ContentInfo aInfo;
        if ( !( aCommand.Argument >>= aInfo ) )
        {
            OSL_FAIL( "Wrong argument type!" );
            ucbhelper::cancelCommandExecution(
                uno::Any( lang::IllegalArgumentException(
                                    u"Wrong argument type!"_ustr,
                                    getXWeak(),
                                    -1 ) ),
                Environment );
            // Unreachable
        }

        aRet <<= createNewContent( aInfo );
    }
    else
    {

        // Unsupported command


        ucbhelper::cancelCommandExecution(
            uno::Any( ucb::UnsupportedCommandException(
                                OUString(),
                                getXWeak() ) ),
            Environment );
        // Unreachable
    }

    return aRet;
}


// virtual
void SAL_CALL Content::abort( sal_Int32 /*CommandId*/ )
{
}


// XContentCreator methods.


// virtual
uno::Sequence< ucb::ContentInfo > SAL_CALL
Content::queryCreatableContentsInfo()
{
    return m_aProps.getCreatableContentsInfo();
}


// virtual
uno::Reference< ucb::XContent > SAL_CALL
Content::createNewContent( const ucb::ContentInfo& Info )
{
    if ( m_aProps.isContentCreator() )
    {
        osl::Guard< osl::Mutex > aGuard( m_aMutex );

        if ( Info.Type.isEmpty() )
            return uno::Reference< ucb::XContent >();

        bool bCreateFolder = Info.Type == TDOC_FOLDER_CONTENT_TYPE;

        // streams cannot be created as direct children of document root
        if ( !bCreateFolder && ( m_aProps.getType() == DOCUMENT ) )
        {
            OSL_FAIL( "Content::createNewContent - streams cannot be "
                        "created as direct children of document root!" );
            return uno::Reference< ucb::XContent >();
        }
        if ( !bCreateFolder && Info.Type != TDOC_STREAM_CONTENT_TYPE )
        {
            OSL_FAIL( "Content::createNewContent - unsupported type!" );
            return uno::Reference< ucb::XContent >();
        }

        OUString aURL = m_xIdentifier->getContentIdentifier();

        OSL_ENSURE( !aURL.isEmpty(),
                    "Content::createNewContent - empty identifier!" );

        if ( ( aURL.lastIndexOf( '/' ) + 1 ) != aURL.getLength() )
            aURL += "/";

        if ( bCreateFolder )
            aURL += "New_Folder";
        else
            aURL += "New_Stream";

        uno::Reference< ucb::XContentIdentifier > xId
            = new ::ucbhelper::ContentIdentifier( aURL );

        return create( m_xContext, m_pProvider, xId, Info );
    }
    else
    {
        OSL_FAIL( "createNewContent called on non-contentcreator object!" );
        return uno::Reference< ucb::XContent >();
    }
}


// virtual
OUString Content::getParentURL()
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );
    Uri aUri( m_xIdentifier->getContentIdentifier() );
    return aUri.getParentUri();
}


uno::Reference< ucb::XContentIdentifier >
Content::makeNewIdentifier( const OUString& rTitle )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    // Assemble new content identifier...
    Uri aUri( m_xIdentifier->getContentIdentifier() );
    OUString aNewURL = aUri.getParentUri() + ::ucb_impl::urihelper::encodeSegment( rTitle );

    return
        uno::Reference< ucb::XContentIdentifier >(
            new ::ucbhelper::ContentIdentifier( aNewURL ) );
}


void Content::queryChildren( ContentRefList& rChildren )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    // Only folders (root, documents, folders) have children.
    if ( !m_aProps.getIsFolder() )
        return;

    // Obtain a list with a snapshot of all currently instantiated contents
    // from provider and extract the contents which are direct children
    // of this content.

    ::ucbhelper::ContentRefList aAllContents;
    m_xProvider->queryExistingContents( aAllContents );

    OUString aURL = m_xIdentifier->getContentIdentifier();
    sal_Int32 nURLPos = aURL.lastIndexOf( '/' );

    if ( nURLPos != ( aURL.getLength() - 1 ) )
    {
        // No trailing slash found. Append.
        aURL += "/";
    }

    sal_Int32 nLen = aURL.getLength();

    for ( const auto& rContent : aAllContents )
    {
        ::ucbhelper::ContentImplHelperRef xChild = rContent;
        OUString aChildURL
            = xChild->getIdentifier()->getContentIdentifier();

        // Is aURL a prefix of aChildURL?
        if ( ( aChildURL.getLength() > nLen ) &&
             ( aChildURL.startsWith( aURL ) ) )
        {
            sal_Int32 nPos = aChildURL.indexOf( '/', nLen );

            if ( ( nPos == -1 ) ||
                 ( nPos == ( aChildURL.getLength() - 1 ) ) )
            {
                // No further slashes / only a final slash. It's a child!
                rChildren.emplace_back(
                        static_cast< Content * >( xChild.get() ) );
            }
        }
    }
}


bool Content::exchangeIdentity(
            const uno::Reference< ucb::XContentIdentifier >& xNewId )
{
    if ( !xNewId.is() )
        return false;

    osl::ClearableGuard< osl::Mutex > aGuard( m_aMutex );

    uno::Reference< ucb::XContent > xThis = this;

    // Already persistent?
    if ( m_eState != PERSISTENT )
    {
        OSL_FAIL( "Content::exchangeIdentity - Not persistent!" );
        return false;
    }

    // Only folders and streams can be renamed -> exchange identity.
    ContentType eType = m_aProps.getType();
    if ( ( eType == ROOT ) || ( eType == DOCUMENT ) )
    {
        OSL_FAIL( "Content::exchangeIdentity - "
                               "Not supported by root or document!" );
        return false;
    }

    // Exchange own identity.

    // Fail, if a content with given id already exists.
    if ( !hasData( Uri( xNewId->getContentIdentifier() ) ) )
    {
        OUString aOldURL = m_xIdentifier->getContentIdentifier();

        aGuard.clear();
        if ( exchange( xNewId ) )
        {
            if ( eType == FOLDER )
            {
                // Process instantiated children...

                ContentRefList aChildren;
                queryChildren( aChildren );

                for ( const auto& rChild : aChildren )
                {
                    ContentRef xChild = rChild;

                    // Create new content identifier for the child...
                    uno::Reference< ucb::XContentIdentifier > xOldChildId
                                                    = xChild->getIdentifier();
                    OUString aOldChildURL
                        = xOldChildId->getContentIdentifier();
                    OUString aNewChildURL
                        = aOldChildURL.replaceAt(
                                        0,
                                        aOldURL.getLength(),
                                        xNewId->getContentIdentifier() );
                    uno::Reference< ucb::XContentIdentifier > xNewChildId
                        = new ::ucbhelper::ContentIdentifier( aNewChildURL );

                    if ( !xChild->exchangeIdentity( xNewChildId ) )
                        return false;
                }
            }
            return true;
        }
    }

    OSL_FAIL( "Content::exchangeIdentity - "
                "Panic! Cannot exchange identity!" );
    return false;
}


// static
uno::Reference< sdbc::XRow > Content::getPropertyValues(
                const uno::Reference< uno::XComponentContext >& rxContext,
                const uno::Sequence< beans::Property >& rProperties,
                ContentProvider* pProvider,
                const OUString& rContentId )
{
    ContentProperties aData;
    if ( loadData( pProvider, Uri(rContentId), aData ) )
    {
        return getPropertyValues(
            rxContext, rProperties, aData, pProvider, rContentId );
    }
    else
    {
        rtl::Reference< ::ucbhelper::PropertyValueSet > xRow
            = new ::ucbhelper::PropertyValueSet( rxContext );

        for ( const beans::Property& rProp : rProperties )
            xRow->appendVoid( rProp );

        return xRow;
    }
}


// static
uno::Reference< sdbc::XRow > Content::getPropertyValues(
                const uno::Reference< uno::XComponentContext >& rxContext,
                const uno::Sequence< beans::Property >& rProperties,
                const ContentProperties& rData,
                ContentProvider* pProvider,
                const OUString& rContentId )
{
    // Note: Empty sequence means "get values of all supported properties".

    rtl::Reference< ::ucbhelper::PropertyValueSet > xRow
        = new ::ucbhelper::PropertyValueSet( rxContext );

    if ( rProperties.hasElements() )
    {
        uno::Reference< beans::XPropertySet > xAdditionalPropSet;
        bool bTriedToGetAdditionalPropSet = false;

        for ( const beans::Property& rProp : rProperties )
        {
            // Process Core properties.

            if ( rProp.Name == "ContentType" )
            {
                xRow->appendString ( rProp, rData.getContentType() );
            }
            else if ( rProp.Name == "Title" )
            {
                xRow->appendString ( rProp, rData.getTitle() );
            }
            else if ( rProp.Name == "IsDocument" )
            {
                xRow->appendBoolean( rProp, rData.getIsDocument() );
            }
            else if ( rProp.Name == "IsFolder" )
            {
                xRow->appendBoolean( rProp, rData.getIsFolder() );
            }
            else if ( rProp.Name == "CreatableContentsInfo" )
            {
                xRow->appendObject(
                    rProp, uno::Any( rData.getCreatableContentsInfo() ) );
            }
            else if ( rProp.Name == "DateModified" )
            {
                // DateModified is only supported by streams.
                ContentType eType = rData.getType();
                if ( eType == STREAM )
                {
                    xRow->appendObject(
                        rProp,
                        uno::Any(
                            pProvider->queryStreamDateModified( rContentId ) ) );
                }
                else
                    xRow->appendVoid( rProp );
            }
            else if ( rProp.Name == "Storage" )
            {
                // Storage is only supported by folders.
                ContentType eType = rData.getType();
                if ( eType == FOLDER )
                    xRow->appendObject(
                        rProp,
                        uno::Any(
                            pProvider->queryStorageClone( rContentId ) ) );
                else
                    xRow->appendVoid( rProp );
            }
            else if ( rProp.Name == "DocumentModel" )
            {
                // DocumentModel is only supported by documents.
                ContentType eType = rData.getType();
                if ( eType == DOCUMENT )
                    xRow->appendObject(
                        rProp,
                        uno::Any(
                            pProvider->queryDocumentModel( rContentId ) ) );
                else
                    xRow->appendVoid( rProp );
            }
            else
            {
                // Not a Core Property! Maybe it's an Additional Core Property?!

                if ( !bTriedToGetAdditionalPropSet && !xAdditionalPropSet.is() )
                {
                    xAdditionalPropSet =
                            pProvider->getAdditionalPropertySet( rContentId,
                                                                 false );
                    bTriedToGetAdditionalPropSet = true;
                }

                if ( xAdditionalPropSet.is() )
                {
                    if ( !xRow->appendPropertySetValue(
                                                xAdditionalPropSet,
                                                rProp ) )
                    {
                        // Append empty entry.
                        xRow->appendVoid( rProp );
                    }
                }
                else
                {
                    // Append empty entry.
                    xRow->appendVoid( rProp );
                }
            }
        }
    }
    else
    {
        // Append all Core Properties.
        xRow->appendString (
            beans::Property( u"ContentType"_ustr,
                      -1,
                      cppu::UnoType<OUString>::get(),
                      beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY ),
            rData.getContentType() );

        ContentType eType = rData.getType();

        xRow->appendString (
            beans::Property( u"Title"_ustr,
                      -1,
                      cppu::UnoType<OUString>::get(),
                      // Title is read-only for root and documents.
                      beans::PropertyAttribute::BOUND |
                      ( ( eType == ROOT ) || ( eType == DOCUMENT )
                        ? beans::PropertyAttribute::READONLY
                        : 0 ) ),
            rData.getTitle() );
        xRow->appendBoolean(
            beans::Property( u"IsDocument"_ustr,
                      -1,
                      cppu::UnoType<bool>::get(),
                      beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY ),
            rData.getIsDocument() );
        xRow->appendBoolean(
            beans::Property( u"IsFolder"_ustr,
                      -1,
                      cppu::UnoType<bool>::get(),
                      beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY ),
            rData.getIsFolder() );
        xRow->appendObject(
            beans::Property(
                u"CreatableContentsInfo"_ustr,
                -1,
                cppu::UnoType<uno::Sequence< ucb::ContentInfo >>::get(),
                beans::PropertyAttribute::BOUND
                | beans::PropertyAttribute::READONLY ),
            uno::Any( rData.getCreatableContentsInfo() ) );

        // DateModified is only supported by streams.
        if ( eType == STREAM )
        {
            xRow->appendObject(
                beans::Property( u"DateModified"_ustr,
                          -1,
                          cppu::UnoType<css::util::DateTime>::get(),
                          beans::PropertyAttribute::BOUND
                            | beans::PropertyAttribute::READONLY ),
                uno::Any( pProvider->queryStreamDateModified( rContentId ) ) );
        }

        // Storage is only supported by folders.
        if ( eType == FOLDER )
            xRow->appendObject(
                beans::Property( u"Storage"_ustr,
                          -1,
                          cppu::UnoType<embed::XStorage>::get(),
                          beans::PropertyAttribute::BOUND
                            | beans::PropertyAttribute::READONLY ),
                uno::Any( pProvider->queryStorageClone( rContentId ) ) );

        // DocumentModel is only supported by documents.
        if ( eType == DOCUMENT )
            xRow->appendObject(
                beans::Property( u"DocumentModel"_ustr,
                          -1,
                          cppu::UnoType<frame::XModel>::get(),
                          beans::PropertyAttribute::BOUND
                            | beans::PropertyAttribute::READONLY ),
                uno::Any(
                    pProvider->queryDocumentModel( rContentId ) ) );

        // Append all Additional Core Properties.

        uno::Reference< beans::XPropertySet > xSet =
            pProvider->getAdditionalPropertySet( rContentId, false );
        xRow->appendPropertySet( xSet );
    }

    return xRow;
}


uno::Reference< sdbc::XRow > Content::getPropertyValues(
                        const uno::Sequence< beans::Property >& rProperties )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );
    return getPropertyValues( m_xContext,
                              rProperties,
                              m_aProps,
                              m_pProvider,
                              m_xIdentifier->getContentIdentifier() );
}


uno::Sequence< uno::Any > Content::setPropertyValues(
        const uno::Sequence< beans::PropertyValue >& rValues,
        const uno::Reference< ucb::XCommandEnvironment > & xEnv )
{
    osl::ClearableGuard< osl::Mutex > aGuard( m_aMutex );

    uno::Sequence< uno::Any > aRet( rValues.getLength() );
    auto aRetRange = asNonConstRange(aRet);
    uno::Sequence< beans::PropertyChangeEvent > aChanges( rValues.getLength() );
    sal_Int32 nChanged = 0;

    beans::PropertyChangeEvent aEvent;
    aEvent.Source         = getXWeak();
    aEvent.Further        = false;
    aEvent.PropertyHandle = -1;

    const beans::PropertyValue* pValues = rValues.getConstArray();
    sal_Int32 nCount = rValues.getLength();

    uno::Reference< ucb::XPersistentPropertySet > xAdditionalPropSet;
    bool bTriedToGetAdditionalPropSet = false;

    bool bExchange = false;
    OUString aOldTitle;
    sal_Int32 nTitlePos = -1;

    for ( sal_Int32 n = 0; n < nCount; ++n )
    {
        const beans::PropertyValue& rValue = pValues[ n ];

        if ( rValue.Name == "ContentType" )
        {
            // Read-only property!
            aRetRange[ n ] <<= lang::IllegalAccessException(
                            u"Property is read-only!"_ustr,
                            getXWeak() );
        }
        else if ( rValue.Name == "IsDocument" )
        {
            // Read-only property!
            aRetRange[ n ] <<= lang::IllegalAccessException(
                            u"Property is read-only!"_ustr,
                            getXWeak() );
        }
        else if ( rValue.Name == "IsFolder" )
        {
            // Read-only property!
            aRetRange[ n ] <<= lang::IllegalAccessException(
                            u"Property is read-only!"_ustr,
                            getXWeak() );
        }
        else if ( rValue.Name == "CreatableContentsInfo" )
        {
            // Read-only property!
            aRetRange[ n ] <<= lang::IllegalAccessException(
                            u"Property is read-only!"_ustr,
                            getXWeak() );
        }
        else if ( rValue.Name == "Title" )
        {
            // Title is read-only for root and documents.
            ContentType eType = m_aProps.getType();
            if ( ( eType == ROOT ) || ( eType == DOCUMENT ) )
            {
                aRetRange[ n ] <<= lang::IllegalAccessException(
                                u"Property is read-only!"_ustr,
                                getXWeak() );
            }
            else
            {
                OUString aNewValue;
                if ( rValue.Value >>= aNewValue )
                {
                    // No empty titles!
                    if ( !aNewValue.isEmpty() )
                    {
                        if ( aNewValue != m_aProps.getTitle() )
                        {
                            // modified title -> modified URL -> exchange !
                            if ( m_eState == PERSISTENT )
                                bExchange = true;

                            aOldTitle = m_aProps.getTitle();
                            m_aProps.setTitle( aNewValue );

                            // property change event will be sent later...

                            // remember position within sequence of values
                            // (for error handling).
                            nTitlePos = n;
                        }
                    }
                    else
                    {
                        aRetRange[ n ] <<= lang::IllegalArgumentException(
                                    u"Empty Title not allowed!"_ustr,
                                    getXWeak(),
                                    -1 );
                    }
                }
                else
                {
                    aRetRange[ n ] <<= beans::IllegalTypeException(
                                u"Title Property value has wrong type!"_ustr,
                                getXWeak() );
                }
            }
        }
        else if ( rValue.Name == "Storage" )
        {
            ContentType eType = m_aProps.getType();
            if ( eType == FOLDER )
            {
                aRetRange[ n ] <<= lang::IllegalAccessException(
                                u"Property is read-only!"_ustr,
                                getXWeak() );
            }
            else
            {
                // Storage is only supported by folders.
                aRetRange[ n ] <<= beans::UnknownPropertyException(
                            u"Storage property only supported by folders"_ustr,
                            getXWeak() );
            }
        }
        else if ( rValue.Name == "DocumentModel" )
        {
            ContentType eType = m_aProps.getType();
            if ( eType == DOCUMENT )
            {
                aRetRange[ n ] <<= lang::IllegalAccessException(
                                u"Property is read-only!"_ustr,
                                getXWeak() );
            }
            else
            {
                // Storage is only supported by folders.
                aRetRange[ n ] <<= beans::UnknownPropertyException(
                            u"DocumentModel property only supported by documents"_ustr,
                            getXWeak() );
            }
        }
        else
        {
            // Not a Core Property! Maybe it's an Additional Core Property?!

            if ( !bTriedToGetAdditionalPropSet && !xAdditionalPropSet.is() )
            {
                xAdditionalPropSet = getAdditionalPropertySet( false );
                bTriedToGetAdditionalPropSet = true;
            }

            if ( xAdditionalPropSet.is() )
            {
                try
                {
                    uno::Any aOldValue = xAdditionalPropSet->getPropertyValue(
                                                                rValue.Name );
                    if ( aOldValue != rValue.Value )
                    {
                        xAdditionalPropSet->setPropertyValue(
                                                rValue.Name, rValue.Value );

                        aEvent.PropertyName = rValue.Name;
                        aEvent.OldValue     = std::move(aOldValue);
                        aEvent.NewValue     = rValue.Value;

                        aChanges.getArray()[ nChanged ] = aEvent;
                        nChanged++;
                    }
                }
                catch ( beans::UnknownPropertyException const & e )
                {
                    aRetRange[ n ] <<= e;
                }
                catch ( lang::WrappedTargetException const & e )
                {
                    aRetRange[ n ] <<= e;
                }
                catch ( beans::PropertyVetoException const & e )
                {
                    aRetRange[ n ] <<= e;
                }
                catch ( lang::IllegalArgumentException const & e )
                {
                    aRetRange[ n ] <<= e;
                }
            }
            else
            {
                aRetRange[ n ] <<= uno::Exception(
                                u"No property set for storing the value!"_ustr,
                                getXWeak() );
            }
        }
    }

    if ( bExchange )
    {
        uno::Reference< ucb::XContentIdentifier > xOldId
            = m_xIdentifier;
        uno::Reference< ucb::XContentIdentifier > xNewId
            = makeNewIdentifier( m_aProps.getTitle() );

        aGuard.clear();
        if ( exchangeIdentity( xNewId ) )
        {
            // Adapt persistent data.
            renameData( xOldId, xNewId );

            // Adapt Additional Core Properties.
            renameAdditionalPropertySet( xOldId->getContentIdentifier(),
                                         xNewId->getContentIdentifier() );
        }
        else
        {
            // Roll-back.
            m_aProps.setTitle( aOldTitle );
            aOldTitle.clear();

            // Set error .
            aRetRange[ nTitlePos ] <<= uno::Exception(
                    u"Exchange failed!"_ustr,
                    getXWeak() );
        }
    }

    if ( !aOldTitle.isEmpty() )
    {
        aEvent.PropertyName = "Title";
        aEvent.OldValue     <<= aOldTitle;
        aEvent.NewValue     <<= m_aProps.getTitle();

        aChanges.getArray()[ nChanged ] = std::move(aEvent);
        nChanged++;
    }

    if ( nChanged > 0 )
    {
        // Save changes, if content was already made persistent.
        if ( !bExchange && ( m_eState == PERSISTENT ) )
        {
            if ( !storeData( uno::Reference< io::XInputStream >(), xEnv ) )
            {
                uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
                {
                     {"Uri", uno::Any(m_xIdentifier->getContentIdentifier())}
                }));
                ucbhelper::cancelCommandExecution(
                    ucb::IOErrorCode_CANT_WRITE,
                    aArgs,
                    xEnv,
                    u"Cannot store persistent data!"_ustr,
                    this );
                // Unreachable
            }
        }

        aChanges.realloc( nChanged );

        aGuard.clear();
        notifyPropertiesChange( aChanges );
    }

    return aRet;
}


uno::Any Content::open(
                const ucb::OpenCommandArgument2& rArg,
                const uno::Reference< ucb::XCommandEnvironment >& xEnv )
{
    if ( rArg.Mode == ucb::OpenMode::ALL ||
         rArg.Mode == ucb::OpenMode::FOLDERS ||
         rArg.Mode == ucb::OpenMode::DOCUMENTS )
    {

        // open command for a folder content


        uno::Reference< ucb::XDynamicResultSet > xSet
            = new DynamicResultSet( m_xContext, this, rArg );
        return uno::Any( xSet );
    }
    else
    {

        // open command for a document content


        if ( ( rArg.Mode == ucb::OpenMode::DOCUMENT_SHARE_DENY_NONE ) ||
             ( rArg.Mode == ucb::OpenMode::DOCUMENT_SHARE_DENY_WRITE ) )
        {
            // Currently(?) unsupported.
            ucbhelper::cancelCommandExecution(
                uno::Any( ucb::UnsupportedOpenModeException(
                                    OUString(),
                                    getXWeak(),
                                    sal_Int16( rArg.Mode ) ) ),
                xEnv );
            // Unreachable
        }

        osl::Guard< osl::Mutex > aGuard( m_aMutex );

        uno::Reference< io::XActiveDataStreamer > xDataStreamer(
                                        rArg.Sink, uno::UNO_QUERY );
        if ( xDataStreamer.is() )
        {
            // May throw CommandFailedException, DocumentPasswordRequest!
            uno::Reference< io::XStream > xStream = getStream( xEnv );
            if ( !xStream.is() )
            {
                // No interaction if we are not persistent!
                uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
                {
                    {"Uri", uno::Any(m_xIdentifier->getContentIdentifier())}
                }));
                ucbhelper::cancelCommandExecution(
                    ucb::IOErrorCode_CANT_READ,
                    aArgs,
                    m_eState == PERSISTENT
                        ? xEnv
                        : uno::Reference< ucb::XCommandEnvironment >(),
                    u"Got no data stream!"_ustr,
                    this );
                // Unreachable
            }

            // Done.
            xDataStreamer->setStream( xStream );
        }
        else
        {
            uno::Reference< io::XOutputStream > xOut( rArg.Sink, uno::UNO_QUERY );
            if ( xOut.is() )
            {
                // PUSH: write data into xOut

                // May throw CommandFailedException, DocumentPasswordRequest!
                uno::Reference< io::XInputStream > xIn = getInputStream( xEnv );
                if ( !xIn.is() )
                {
                    // No interaction if we are not persistent!
                    uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
                    {
                        {"Uri", uno::Any(m_xIdentifier->getContentIdentifier())}
                    }));
                    ucbhelper::cancelCommandExecution(
                        ucb::IOErrorCode_CANT_READ,
                        aArgs,
                        m_eState == PERSISTENT
                            ? xEnv
                            : uno::Reference< ucb::XCommandEnvironment >(),
                        u"Got no data stream!"_ustr,
                        this );
                    // Unreachable
                }

                try
                {
                    uno::Sequence< sal_Int8 > aBuffer;

                    while (true)
                    {
                        sal_Int32 nRead = xIn->readSomeBytes( aBuffer, 65536 );
                        if (!nRead)
                            break;
                        aBuffer.realloc( nRead );
                        xOut->writeBytes( aBuffer );
                    }

                    xOut->closeOutput();
                }
                catch ( io::NotConnectedException const & )
                {
                    // closeOutput, readSomeBytes, writeBytes
                }
                catch ( io::BufferSizeExceededException const & )
                {
                    // closeOutput, readSomeBytes, writeBytes
                }
                catch ( io::IOException const & )
                {
                    // closeOutput, readSomeBytes, writeBytes
                }
            }
            else
            {
                uno::Reference< io::XActiveDataSink > xDataSink(
                                                rArg.Sink, uno::UNO_QUERY );
                if ( xDataSink.is() )
                {
                    // PULL: wait for client read

                    // May throw CommandFailedException, DocumentPasswordRequest!
                    uno::Reference< io::XInputStream > xIn = getInputStream( xEnv );
                    if ( !xIn.is() )
                    {
                        // No interaction if we are not persistent!
                        uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
                        {
                            {"Uri", uno::Any(m_xIdentifier->getContentIdentifier())}
                        }));
                        ucbhelper::cancelCommandExecution(
                            ucb::IOErrorCode_CANT_READ,
                            aArgs,
                            m_eState == PERSISTENT
                                ? xEnv
                                : uno::Reference<
                                      ucb::XCommandEnvironment >(),
                            u"Got no data stream!"_ustr,
                            this );
                        // Unreachable
                    }

                    // Done.
                    xDataSink->setInputStream( xIn );
                }
                else
                {
                    ucbhelper::cancelCommandExecution(
                        uno::Any(
                            ucb::UnsupportedDataSinkException(
                                    OUString(),
                                    getXWeak(),
                                    rArg.Sink ) ),
                        xEnv );
                    // Unreachable
                }
            }
        }
    }

    return uno::Any();
}


void Content::insert( const uno::Reference< io::XInputStream >& xData,
                      sal_Int32 nNameClashResolve,
                      const uno::Reference<
                          ucb::XCommandEnvironment > & xEnv )
{
    osl::ClearableGuard< osl::Mutex > aGuard( m_aMutex );

    ContentType eType = m_aProps.getType();

    OSL_ENSURE( ( eType == FOLDER ) || ( eType == STREAM ),
                "insert command only supported by streams and folders!" );

    Uri aUri( m_xIdentifier->getContentIdentifier() );

#if OSL_DEBUG_LEVEL > 0
    if ( eType == STREAM )
    {
        Uri aParentUri( aUri.getParentUri() );
        OSL_ENSURE( !aParentUri.isDocument(),
                    "insert command not supported by streams that are direct "
                    "children of document root!" );
    }
#endif

    // Check, if all required properties were set.
    if ( eType == FOLDER )
    {
        // Required: Title

        if ( m_aProps.getTitle().isEmpty() )
            m_aProps.setTitle( aUri.getDecodedName() );
    }
    else // stream
    {
        // Required: data

        if ( !xData.is() )
        {
            ucbhelper::cancelCommandExecution(
                uno::Any( ucb::MissingInputStreamException(
                                OUString(),
                                getXWeak() ) ),
                xEnv );
            // Unreachable
        }

        // Required: Title

        if ( m_aProps.getTitle().isEmpty() )
            m_aProps.setTitle( aUri.getDecodedName() );
    }

    Uri aNewUri( aUri.getParentUri() + m_aProps.getTitle() );

    // Handle possible name clash...
    switch ( nNameClashResolve )
    {
        // fail.
        case ucb::NameClash::ERROR:
            if ( hasData( aNewUri ) )
            {
                ucbhelper::cancelCommandExecution(
                    uno::Any( ucb::NameClashException(
                                    OUString(),
                                    getXWeak(),
                                    task::InteractionClassification_ERROR,
                                    m_aProps.getTitle() ) ),
                    xEnv );
                // Unreachable
            }
            break;

        // replace (possibly) existing object.
        case ucb::NameClash::OVERWRITE:
            break;

        // "invent" a new valid title.
        case ucb::NameClash::RENAME:
            if ( hasData( aNewUri ) )
            {
                sal_Int32 nTry = 0;

                do
                {
                    aNewUri.setUri( aNewUri.getUri() + "_" + OUString::number(++nTry) );
                }
                while ( hasData( aNewUri ) && ( nTry < 1000 ) );

                if ( nTry == 1000 )
                {
                    ucbhelper::cancelCommandExecution(
                        uno::Any(
                            ucb::UnsupportedNameClashException(
                                u"Unable to resolve name clash!"_ustr,
                                getXWeak(),
                                nNameClashResolve ) ),
                        xEnv );
                    // Unreachable
                }
                else
                {
                    m_aProps.setTitle( m_aProps.getTitle() + "_" + OUString::number( ++nTry ) );
                }
            }
            break;

        case ucb::NameClash::KEEP: // deprecated
        case ucb::NameClash::ASK:
        default:
            if ( hasData( aNewUri ) )
            {
                ucbhelper::cancelCommandExecution(
                    uno::Any(
                        ucb::UnsupportedNameClashException(
                            OUString(),
                            getXWeak(),
                            nNameClashResolve ) ),
                    xEnv );
                // Unreachable
            }
            break;
    }

    // Identifier changed?
    bool bNewId = ( aUri != aNewUri );

    if ( bNewId )
    {
        m_xIdentifier
            = new ::ucbhelper::ContentIdentifier( aNewUri.getUri() );
    }

    if ( !storeData( xData, xEnv ) )
    {
        uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
        {
            {"Uri", uno::Any(m_xIdentifier->getContentIdentifier())}
        }));
        ucbhelper::cancelCommandExecution(
            ucb::IOErrorCode_CANT_WRITE,
            aArgs,
            xEnv,
            u"Cannot store persistent data!"_ustr,
            this );
        // Unreachable
    }

    m_eState = PERSISTENT;

    if ( bNewId )
    {
        //loadData( m_pProvider, m_aUri, m_aProps );

        aGuard.clear();
        inserted();
    }
}


void Content::destroy( bool bDeletePhysical,
                       const uno::Reference<
                           ucb::XCommandEnvironment > & xEnv )
{
    // @@@ take care about bDeletePhysical -> trashcan support

    osl::ClearableGuard< osl::Mutex > aGuard( m_aMutex );

    ContentType eType = m_aProps.getType();

    OSL_ENSURE( ( eType == FOLDER ) || ( eType == STREAM ),
                "delete command only supported by streams and folders!" );

    uno::Reference< ucb::XContent > xThis = this;

    // Persistent?
    if ( m_eState != PERSISTENT )
    {
        ucbhelper::cancelCommandExecution(
            uno::Any( ucb::UnsupportedCommandException(
                                u"Not persistent!"_ustr,
                                getXWeak() ) ),
            xEnv );
        // Unreachable
    }

    m_eState = DEAD;

    aGuard.clear();
    deleted();

    if ( eType == FOLDER )
    {
        // Process instantiated children...

        ContentRefList aChildren;
        queryChildren( aChildren );

        for ( auto& rChild : aChildren )
        {
            rChild->destroy( bDeletePhysical, xEnv );
        }
    }
}


void Content::notifyDocumentClosed()
{
    osl::ClearableGuard< osl::Mutex > aGuard( m_aMutex );

    m_eState = DEAD;

    // @@@ anything else to reset or such?

    // callback follows!
    aGuard.clear();

    // Propagate destruction to content event listeners
    // Remove this from provider's content list.
    deleted();
}


uno::Reference< ucb::XContent >
Content::queryChildContent( std::u16string_view rRelativeChildUri )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    const OUString aMyId = getIdentifier()->getContentIdentifier();
    OUStringBuffer aBuf( aMyId );
    if ( !aMyId.endsWith("/") )
        aBuf.append( "/" );
    if ( !o3tl::starts_with(rRelativeChildUri, u"/") )
        aBuf.append( rRelativeChildUri );
    else
        aBuf.append( rRelativeChildUri.substr(1) );

    uno::Reference< ucb::XContentIdentifier > xChildId
        = new ::ucbhelper::ContentIdentifier( aBuf.makeStringAndClear() );

    uno::Reference< ucb::XContent > xChild;
    try
    {
        xChild = m_pProvider->queryContent( xChildId );
    }
    catch ( ucb::IllegalIdentifierException const & )
    {
        // handled below.
    }

    OSL_ENSURE( xChild.is(),
                "Content::queryChildContent - unable to create child content!" );
    return xChild;
}


void Content::notifyChildRemoved( std::u16string_view rRelativeChildUri )
{
    osl::ClearableGuard< osl::Mutex > aGuard( m_aMutex );

    // Ugly! Need to create child content object, just to fill event properly.
    uno::Reference< ucb::XContent > xChild
        = queryChildContent( rRelativeChildUri );

    if ( !xChild.is() )
        return;

    // callback follows!
    aGuard.clear();

    // Notify "REMOVED" event.
    ucb::ContentEvent aEvt(
        getXWeak(),
        ucb::ContentAction::REMOVED,
        xChild,
        getIdentifier() );
    notifyContentEvent( aEvt );
}


void Content::notifyChildInserted( std::u16string_view rRelativeChildUri )
{
    osl::ClearableGuard< osl::Mutex > aGuard( m_aMutex );

    // Ugly! Need to create child content object, just to fill event properly.
    uno::Reference< ucb::XContent > xChild
        = queryChildContent( rRelativeChildUri );

    if ( !xChild.is() )
        return;

    // callback follows!
    aGuard.clear();

    // Notify "INSERTED" event.
    ucb::ContentEvent aEvt(
        getXWeak(),
        ucb::ContentAction::INSERTED,
        xChild,
        getIdentifier() );
    notifyContentEvent( aEvt );
}


void Content::transfer(
            const ucb::TransferInfo& rInfo,
            const uno::Reference< ucb::XCommandEnvironment > & xEnv )
{
    osl::ClearableGuard< osl::Mutex > aGuard( m_aMutex );

    // Persistent?
    if ( m_eState != PERSISTENT )
    {
        ucbhelper::cancelCommandExecution(
            uno::Any( ucb::UnsupportedCommandException(
                                u"Not persistent!"_ustr,
                                getXWeak() ) ),
            xEnv );
        // Unreachable
    }

    // Does source URI scheme match? Only vnd.sun.star.tdoc is supported.

    if ( rInfo.SourceURL.getLength() < TDOC_URL_SCHEME_LENGTH + 2 )
    {
        // Invalid length (to short).
        ucbhelper::cancelCommandExecution(
            uno::Any( ucb::InteractiveBadTransferURLException(
                            OUString(),
                            getXWeak() ) ),
            xEnv );
        // Unreachable
    }

    OUString aScheme
        = rInfo.SourceURL.copy( 0, TDOC_URL_SCHEME_LENGTH + 2 )
            .toAsciiLowerCase();
    if ( aScheme != TDOC_URL_SCHEME ":/" )
    {
        // Invalid scheme.
        ucbhelper::cancelCommandExecution(
            uno::Any( ucb::InteractiveBadTransferURLException(
                            OUString(),
                            getXWeak() ) ),
            xEnv );
        // Unreachable
    }

    // Does source URI describe a tdoc folder or stream?
    Uri aSourceUri( rInfo.SourceURL );
    if ( !aSourceUri.isValid() )
    {
        ucbhelper::cancelCommandExecution(
            uno::Any( lang::IllegalArgumentException(
                                u"Invalid source URI! Syntax!"_ustr,
                                getXWeak(),
                                -1 ) ),
            xEnv );
        // Unreachable
    }

    if ( aSourceUri.isRoot() || aSourceUri.isDocument() )
    {
        ucbhelper::cancelCommandExecution(
            uno::Any( lang::IllegalArgumentException(
                                u"Invalid source URI! Must describe a folder or stream!"_ustr,
                                getXWeak(),
                                -1 ) ),
            xEnv );
        // Unreachable
    }

    // Is source not a parent of me / not me?
    OUString aId = m_xIdentifier->getContentIdentifier();
    sal_Int32 nPos = aId.lastIndexOf( '/' );
    if ( nPos != ( aId.getLength() - 1 ) )
    {
        // No trailing slash found. Append.
        aId += "/";
    }

    if ( rInfo.SourceURL.getLength() <= aId.getLength() )
    {
        if ( aId.startsWith( rInfo.SourceURL ) )
        {
            uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
            {
                {"Uri", uno::Any(rInfo.SourceURL)}
            }));
            ucbhelper::cancelCommandExecution(
                ucb::IOErrorCode_RECURSIVE,
                aArgs,
                xEnv,
                u"Target is equal to or is a child of source!"_ustr,
                this );
            // Unreachable
        }
    }

    if ( m_aProps.getType() == DOCUMENT )
    {
        bool bOK = false;

        uno::Reference< embed::XStorage > xStorage
            = m_pProvider->queryStorage(
                aSourceUri.getParentUri(), READ_WRITE_NOCREATE );
        if ( xStorage.is() )
        {
            try
            {
                if ( xStorage->isStreamElement( aSourceUri.getDecodedName() ) )
                {
                    ucbhelper::cancelCommandExecution(
                        uno::Any( lang::IllegalArgumentException(
                                        u"Invalid source URI! "
                                        "Streams cannot be created as "
                                        "children of document root!"_ustr,
                                        getXWeak(),
                                        -1 ) ),
                        xEnv );
                    // Unreachable
                }
                bOK = true;
            }
            catch ( container::NoSuchElementException const & )
            {
                // handled below.
            }
            catch ( lang::IllegalArgumentException const & )
            {
                // handled below.
            }
            catch ( embed::InvalidStorageException const & )
            {
                // handled below.
            }
        }

        if ( !bOK )
        {
            ucbhelper::cancelCommandExecution(
                uno::Any( lang::IllegalArgumentException(
                                    u"Invalid source URI! Unable to determine source type!"_ustr,
                                    getXWeak(),
                                    -1 ) ),
                xEnv );
            // Unreachable
        }
    }


    // Copy data.


    OUString aNewName( !rInfo.NewTitle.isEmpty()
                                ? rInfo.NewTitle
                                : aSourceUri.getDecodedName() );

    if ( !copyData( aSourceUri, aNewName ) )
    {
        uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
        {
            {"Uri", uno::Any(rInfo.SourceURL)}
        }));
        ucbhelper::cancelCommandExecution(
            ucb::IOErrorCode_CANT_WRITE,
            aArgs,
            xEnv,
            u"Cannot copy data!"_ustr,
            this );
        // Unreachable
    }


    // Copy own and all children's Additional Core Properties.


    OUString aTargetUri = m_xIdentifier->getContentIdentifier();
    if ( ( aTargetUri.lastIndexOf( '/' ) + 1 ) != aTargetUri.getLength() )
        aTargetUri += "/";

    if ( !rInfo.NewTitle.isEmpty() )
        aTargetUri += ::ucb_impl::urihelper::encodeSegment( rInfo.NewTitle );
    else
        aTargetUri += aSourceUri.getName();

    if ( !copyAdditionalPropertySet( aSourceUri.getUri(), aTargetUri ) )
    {
        uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
        {
            {"Uri", uno::Any(rInfo.SourceURL)}
        }));
        ucbhelper::cancelCommandExecution(
            ucb::IOErrorCode_CANT_WRITE,
            aArgs,
            xEnv,
            u"Cannot copy additional properties!"_ustr,
            this );
        // Unreachable
    }


    // Propagate new content.


    rtl::Reference< Content > xTarget;
    try
    {
        uno::Reference< ucb::XContentIdentifier > xTargetId
            = new ::ucbhelper::ContentIdentifier( aTargetUri );

        // Note: The static cast is okay here, because its sure that
        //       m_xProvider is always the WebDAVContentProvider.
        xTarget = static_cast< Content * >(
            m_pProvider->queryContent( xTargetId ).get() );

    }
    catch ( ucb::IllegalIdentifierException const & )
    {
        // queryContent
    }

    if ( !xTarget.is() )
    {
        uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
        {
            {"Uri", uno::Any(aTargetUri)}
        }));
        ucbhelper::cancelCommandExecution(
            ucb::IOErrorCode_CANT_READ,
            aArgs,
            xEnv,
            u"Cannot instantiate target object!"_ustr,
            this );
        // Unreachable
    }

    // Announce transferred content in its new folder.
    xTarget->inserted();


    // Remove source, if requested


    if ( !rInfo.MoveData )
        return;

    rtl::Reference< Content > xSource;
    try
    {
        uno::Reference< ucb::XContentIdentifier >
            xSourceId = new ::ucbhelper::ContentIdentifier( rInfo.SourceURL );

        // Note: The static cast is okay here, because its sure
        //       that m_xProvider is always the ContentProvider.
        xSource = static_cast< Content * >(
            m_xProvider->queryContent( xSourceId ).get() );
    }
    catch ( ucb::IllegalIdentifierException const & )
    {
        // queryContent
    }

    if ( !xSource.is() )
    {
        uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
        {
            {"Uri", uno::Any(rInfo.SourceURL)}
        }));
        ucbhelper::cancelCommandExecution(
            ucb::IOErrorCode_CANT_READ,
            aArgs,
            xEnv,
            u"Cannot instantiate target object!"_ustr,
            this );
        // Unreachable
    }

    // Propagate destruction (recursively).
    xSource->destroy( true, xEnv );

    // Remove all persistent data of source and its children.
    if ( !xSource->removeData() )
    {
        uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
        {
            {"Uri", uno::Any(rInfo.SourceURL)}
        }));
        ucbhelper::cancelCommandExecution(
            ucb::IOErrorCode_CANT_WRITE,
            aArgs,
            xEnv,
            u"Cannot remove persistent data of source object!"_ustr,
            this );
        // Unreachable
    }

    // Remove own and all children's Additional Core Properties.
    if ( xSource->removeAdditionalPropertySet() )
        return;

    uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
    {
        {"Uri", uno::Any(rInfo.SourceURL)}
    }));
    ucbhelper::cancelCommandExecution(
        ucb::IOErrorCode_CANT_WRITE,
        aArgs,
        xEnv,
        u"Cannot remove additional properties of source object!"_ustr,
        this );
    // Unreachable
}


//static
bool Content::hasData( ContentProvider const * pProvider, const Uri & rUri )
{
    if ( rUri.isRoot() )
    {
        return true; // root has no storage
    }
    else if ( rUri.isDocument() )
    {
        uno::Reference< embed::XStorage > xStorage
            = pProvider->queryStorage( rUri.getUri(), READ );
        return xStorage.is();
    }
    else
    {
        // folder or stream

        // Ask parent storage. In case that rUri describes a stream,
        // ContentProvider::queryStorage( rUri ) would return null.

        uno::Reference< embed::XStorage > xStorage
            = pProvider->queryStorage( rUri.getParentUri(), READ );

        if ( !xStorage.is() )
            return false;

        return xStorage->hasByName( rUri.getDecodedName() );
    }
}


//static
bool Content::loadData( ContentProvider const * pProvider,
                        const Uri & rUri,
                        ContentProperties& rProps )
{
    if ( rUri.isRoot() ) // root has no storage, but can always be created
    {
        rProps
            = ContentProperties(
                ROOT, pProvider->queryStorageTitle( rUri.getUri() ) );
    }
    else if ( rUri.isDocument() ) // document must have storage
    {
        uno::Reference< embed::XStorage > xStorage
            = pProvider->queryStorage( rUri.getUri(), READ );

        if ( !xStorage.is() )
            return false;

        rProps
            = ContentProperties(
                DOCUMENT, pProvider->queryStorageTitle( rUri.getUri() ) );
    }
    else // stream or folder; stream has no storage; folder has storage
    {
        uno::Reference< embed::XStorage > xStorage
            = pProvider->queryStorage( rUri.getParentUri(), READ );

        if ( !xStorage.is() )
            return false;

        // Check whether exists at all, is stream or folder
        try
        {
            // return: true  -> folder
            // return: false -> stream
            // NoSuchElementException -> neither folder nor stream
            bool bIsFolder
                = xStorage->isStorageElement( rUri.getDecodedName() );

            rProps
                = ContentProperties(
                    bIsFolder ? FOLDER : STREAM,
                    pProvider->queryStorageTitle( rUri.getUri() ) );
        }
        catch ( container::NoSuchElementException const & )
        {
            // there is no element with such name
            //OSL_ENSURE( false, "Caught NoSuchElementException!" );
            return false;
        }
        catch ( lang::IllegalArgumentException const & )
        {
            // an illegal argument is provided
            TOOLS_WARN_EXCEPTION("ucb.ucp", "");
            return false;
        }
        catch ( embed::InvalidStorageException const & )
        {
            // this storage is in invalid state for any reason
            TOOLS_WARN_EXCEPTION("ucb.ucp", "");
            return false;
        }
    }
    return true;
}


bool Content::storeData( const uno::Reference< io::XInputStream >& xData,
                         const uno::Reference<
                            ucb::XCommandEnvironment >& xEnv )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    ContentType eType = m_aProps.getType();
    if ( ( eType == ROOT ) || ( eType == DOCUMENT ) )
    {
        OSL_FAIL( "storeData not supported by root and documents!" );
        return false;
    }

    Uri aUri( m_xIdentifier->getContentIdentifier() );

    if ( eType == FOLDER )
    {
        uno::Reference< embed::XStorage > xStorage
            = m_pProvider->queryStorage( aUri.getUri(), READ_WRITE_CREATE );

        if ( !xStorage.is() )
            return false;

        uno::Reference< beans::XPropertySet > xPropSet(
            xStorage, uno::UNO_QUERY );
        OSL_ENSURE( xPropSet.is(),
                    "Content::storeData - Got no XPropertySet interface!" );
        if ( !xPropSet.is() )
            return false;

        try
        {
            // According to MBA, if no mediatype is set, folder and all
            // its contents will be lost on save of the document!!!
            xPropSet->setPropertyValue(
                u"MediaType"_ustr,
                uno::Any(
                    u"application/binary"_ustr ) );
        }
        catch ( beans::UnknownPropertyException const & )
        {
            OSL_FAIL( "Property MediaType not supported!" );
            return false;
        }
        catch ( beans::PropertyVetoException const & )
        {
            TOOLS_WARN_EXCEPTION("ucb.ucp", "");
            return false;
        }
        catch ( lang::IllegalArgumentException const & )
        {
            TOOLS_WARN_EXCEPTION("ucb.ucp", "");
            return false;
        }
        catch ( lang::WrappedTargetException const & )
        {
            TOOLS_WARN_EXCEPTION("ucb.ucp", "");
            return false;
        }

        if ( !commitStorage( xStorage ) )
            return false;
    }
    else if ( eType == STREAM )
    {
        // stream

        // Important: Parent storage and output stream must be kept alive until
        //            changes have been committed!
        uno::Reference< embed::XStorage > xStorage
            = m_pProvider->queryStorage(
                aUri.getParentUri(), READ_WRITE_CREATE );
        uno::Reference< io::XOutputStream > xOut;

        if ( !xStorage.is() )
            return false;

        if ( xData.is() )
        {
            // May throw CommandFailedException, DocumentPasswordRequest!
            xOut = getTruncatedOutputStream( xEnv );

            OSL_ENSURE( xOut.is(), "No target data stream!" );

            try
            {
                uno::Sequence< sal_Int8 > aBuffer;
                while (true)
                {
                    sal_Int32 nRead = xData->readSomeBytes( aBuffer, 65536 );
                    if (!nRead)
                        break;
                    aBuffer.realloc( nRead );
                    xOut->writeBytes( aBuffer );
                }

                closeOutputStream( xOut );
            }
            catch ( io::NotConnectedException const & )
            {
                // readSomeBytes, writeBytes
                TOOLS_WARN_EXCEPTION("ucb.ucp", "");
                closeOutputStream( xOut );
                return false;
            }
            catch ( io::BufferSizeExceededException const & )
            {
                // readSomeBytes, writeBytes
                TOOLS_WARN_EXCEPTION("ucb.ucp", "");
                closeOutputStream( xOut );
                return false;
            }
            catch ( io::IOException const & )
            {
                // readSomeBytes, writeBytes
                TOOLS_WARN_EXCEPTION("ucb.ucp", "");
                closeOutputStream( xOut );
                return false;
            }
            catch ( ... )
            {
                closeOutputStream( xOut );
                throw;
            }
        }

        // Commit changes.
        if ( !commitStorage( xStorage ) )
            return false;
    }
    else
    {
        OSL_FAIL( "Unknown content type!" );
        return false;
    }
    return true;
}


void Content::renameData(
            const uno::Reference< ucb::XContentIdentifier >& xOldId,
            const uno::Reference< ucb::XContentIdentifier >& xNewId )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    ContentType eType = m_aProps.getType();
    if ( ( eType == ROOT ) || ( eType == DOCUMENT ) )
    {
        OSL_FAIL( "renameData not supported by root and documents!" );
        return;
    }

    Uri aOldUri( xOldId->getContentIdentifier() );
    uno::Reference< embed::XStorage > xStorage
        = m_pProvider->queryStorage(
            aOldUri.getParentUri(), READ_WRITE_NOCREATE );

    if ( !xStorage.is() )
        return;

    try
    {
        Uri aNewUri( xNewId->getContentIdentifier() );
        xStorage->renameElement(
            aOldUri.getDecodedName(), aNewUri.getDecodedName() );
    }
    catch ( embed::InvalidStorageException const & )
    {
        // this storage is in invalid state for any reason
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return;
    }
    catch ( lang::IllegalArgumentException const & )
    {
        // an illegal argument is provided
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return;
    }
    catch ( container::NoSuchElementException const & )
    {
        // there is no element with old name in this storage
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return;
    }
    catch ( container::ElementExistException const & )
    {
        // an element with new name already exists in this storage
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return;
    }
    catch ( io::IOException const & )
    {
        // in case of io errors during renaming
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return;
    }
    catch ( embed::StorageWrappedTargetException const & )
    {
        // wraps other exceptions
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return;
    }

    commitStorage( xStorage );
}


bool Content::removeData()
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    ContentType eType = m_aProps.getType();
    if ( ( eType == ROOT ) || ( eType == DOCUMENT ) )
    {
        OSL_FAIL( "removeData not supported by root and documents!" );
        return false;
    }

    Uri aUri( m_xIdentifier->getContentIdentifier() );
    uno::Reference< embed::XStorage > xStorage
        = m_pProvider->queryStorage(
            aUri.getParentUri(), READ_WRITE_NOCREATE );

    if ( !xStorage.is() )
        return false;

    try
    {
        xStorage->removeElement( aUri.getDecodedName() );
    }
    catch ( embed::InvalidStorageException const & )
    {
        // this storage is in invalid state for any reason
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return false;
    }
    catch ( lang::IllegalArgumentException const & )
    {
        // an illegal argument is provided
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return false;
    }
    catch ( container::NoSuchElementException const & )
    {
        // there is no element with this name in this storage
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return false;
    }
    catch ( io::IOException const & )
    {
        // in case of io errors during renaming
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return false;
    }
    catch ( embed::StorageWrappedTargetException const & )
    {
        // wraps other exceptions
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return false;
    }

    return commitStorage( xStorage );
}


bool Content::copyData( const Uri & rSourceUri, const OUString & rNewName )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    ContentType eType = m_aProps.getType();
    if ( ( eType == ROOT ) || ( eType == STREAM ) )
    {
        OSL_FAIL( "copyData not supported by root and streams!" );
        return false;
    }

    Uri aDestUri( m_xIdentifier->getContentIdentifier() );
    uno::Reference< embed::XStorage > xDestStorage
        = m_pProvider->queryStorage( aDestUri.getUri(), READ_WRITE_NOCREATE );

    if ( !xDestStorage.is() )
        return false;

    uno::Reference< embed::XStorage > xSourceStorage
        = m_pProvider->queryStorage( rSourceUri.getParentUri(), READ );

    if ( !xSourceStorage.is() )
        return false;

    try
    {
        xSourceStorage->copyElementTo( rSourceUri.getDecodedName(),
                                       xDestStorage,
                                       rNewName );
    }
    catch ( embed::InvalidStorageException const & )
    {
        // this storage is in invalid state for any reason
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return false;
    }
    catch ( lang::IllegalArgumentException const & )
    {
        // an illegal argument is provided
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return false;
    }
    catch ( container::NoSuchElementException const & )
    {
        // there is no element with this name in this storage
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return false;
    }
    catch ( container::ElementExistException const & )
    {
        // there is no element with this name in this storage
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return false;
    }
    catch ( io::IOException const & )
    {
        // in case of io errors during renaming
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return false;
    }
    catch ( embed::StorageWrappedTargetException const & )
    {
        // wraps other exceptions
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return false;
    }

    return commitStorage( xDestStorage );
}


// static
bool Content::commitStorage( const uno::Reference< embed::XStorage > & xStorage )
{
    // Commit changes
    uno::Reference< embed::XTransactedObject > xTO( xStorage, uno::UNO_QUERY );

    OSL_ENSURE( xTO.is(),
                "Required interface css.embed.XTransactedObject missing!" );
    try
    {
        xTO->commit();
    }
    catch ( io::IOException const & )
    {
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return false;
    }
    catch ( lang::WrappedTargetException const & )
    {
        TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        return false;
    }

    return true;
}


// static
bool Content::closeOutputStream(
    const uno::Reference< io::XOutputStream > & xOut )
{
    if ( xOut.is() )
    {
        try
        {
            xOut->closeOutput();
            return true;
        }
        catch ( io::NotConnectedException const & )
        {
            TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        }
        catch ( io::BufferSizeExceededException const & )
        {
            TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        }
        catch ( io::IOException const & )
        {
            TOOLS_WARN_EXCEPTION("ucb.ucp", "");
        }
    }
    return false;
}

/// @throws ucb::CommandFailedException
/// @throws task::DocumentPasswordRequest
static OUString obtainPassword(
        const OUString & rName,
        task::PasswordRequestMode eMode,
        const uno::Reference< ucb::XCommandEnvironment > & xEnv )
{
    rtl::Reference< DocumentPasswordRequest > xRequest
        = new DocumentPasswordRequest( eMode, rName );

    if ( xEnv.is() )
    {
        uno::Reference< task::XInteractionHandler > xIH
            = xEnv->getInteractionHandler();
        if ( xIH.is() )
        {
            xIH->handle( xRequest );

            rtl::Reference< ucbhelper::InteractionContinuation > xSelection
                = xRequest->getSelection();

            if ( xSelection.is() )
            {
                // Handler handled the request.
                uno::Reference< task::XInteractionAbort > xAbort(
                    xSelection->getXWeak(), uno::UNO_QUERY );
                if ( xAbort.is() )
                {
                    throw ucb::CommandFailedException(
                        u"Abort requested by Interaction Handler."_ustr,
                        uno::Reference< uno::XInterface >(),
                        xRequest->getRequest() );
                }

                uno::Reference< task::XInteractionPassword > xPassword(
                    xSelection->getXWeak(), uno::UNO_QUERY );
                if ( xPassword.is() )
                {
                    return xPassword->getPassword();
                }

                // Unknown selection. Should never happen.
                throw ucb::CommandFailedException(
                    u"Interaction Handler selected unknown continuation!"_ustr,
                    uno::Reference< uno::XInterface >(),
                    xRequest->getRequest() );
            }
        }
    }

    // No IH or IH did not handle exception.
    task::DocumentPasswordRequest aRequest;
    xRequest->getRequest() >>= aRequest;
    throw aRequest;
}


uno::Reference< io::XInputStream > Content::getInputStream(
        const uno::Reference< ucb::XCommandEnvironment > & xEnv )
{
    OUString aUri;
    OUString aPassword;
    bool bPasswordRequested = false;

    {
        osl::Guard< osl::Mutex > aGuard( m_aMutex );

        OSL_ENSURE( m_aProps.getType() == STREAM,
                    "Content::getInputStream - content is no stream!" );

        aUri = Uri( m_xIdentifier->getContentIdentifier() ).getUri();
    }

    for ( ;; )
    {
        try
        {
            osl::Guard< osl::Mutex > aGuard( m_aMutex );
            return m_pProvider->queryInputStream( aUri, aPassword );
        }
        catch ( packages::WrongPasswordException const & )
        {
            // Obtain (new) password.
            aPassword
                = obtainPassword( aUri, /* @@@ find better title */
                                  bPasswordRequested
                                   ? task::PasswordRequestMode_PASSWORD_REENTER
                                   : task::PasswordRequestMode_PASSWORD_ENTER,
                                   xEnv );
            bPasswordRequested = true;
        }
    }
}

/// @throws ucb::CommandFailedException
/// @throws task::DocumentPasswordRequest
/// @throws uno::RuntimeException
static uno::Reference< io::XOutputStream > lcl_getTruncatedOutputStream(
                const OUString & rUri,
                ContentProvider const * pProvider,
                const uno::Reference< ucb::XCommandEnvironment > & xEnv )
{
    OUString aPassword;
    bool bPasswordRequested = false;
    for ( ;; )
    {
        try
        {
            return pProvider->queryOutputStream(
                    rUri, aPassword, true /* truncate */ );
        }
        catch ( packages::WrongPasswordException const & )
        {
            // Obtain (new) password.
            aPassword
                = obtainPassword( rUri, /* @@@ find better title */
                                  bPasswordRequested
                                   ? task::PasswordRequestMode_PASSWORD_REENTER
                                   : task::PasswordRequestMode_PASSWORD_ENTER,
                                   xEnv );
            bPasswordRequested = true;
        }
    }
}


uno::Reference< io::XOutputStream > Content::getTruncatedOutputStream(
        const uno::Reference< ucb::XCommandEnvironment > & xEnv )
{
    OSL_ENSURE( m_aProps.getType() == STREAM,
                "Content::getTruncatedOutputStream - content is no stream!" );

    return lcl_getTruncatedOutputStream(
            Uri( m_xIdentifier->getContentIdentifier() ).getUri(),
            m_pProvider,
            xEnv );
}


uno::Reference< io::XStream > Content::getStream(
        const uno::Reference< ucb::XCommandEnvironment > & xEnv )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    OSL_ENSURE( m_aProps.getType() == STREAM,
                "Content::getStream - content is no stream!" );

    OUString aUri( Uri( m_xIdentifier->getContentIdentifier() ).getUri() );
    OUString aPassword;
    bool bPasswordRequested = false;
    for ( ;; )
    {
        try
        {
            return m_pProvider->queryStream(
                    aUri, aPassword, false /* no truncate */ );
        }
        catch ( packages::WrongPasswordException const & )
        {
            // Obtain (new) password.
            aPassword
                = obtainPassword( aUri, /* @@@ find better title */
                                  bPasswordRequested
                                   ? task::PasswordRequestMode_PASSWORD_REENTER
                                   : task::PasswordRequestMode_PASSWORD_ENTER,
                                   xEnv );
            bPasswordRequested = true;
        }
    }
}


// ContentProperties Implementation.


uno::Sequence< ucb::ContentInfo >
ContentProperties::getCreatableContentsInfo() const
{
    if ( isContentCreator() )
    {
        uno::Sequence< beans::Property > aProps( 1 );
        aProps.getArray()[ 0 ] = beans::Property(
                    u"Title"_ustr,
                    -1,
                    cppu::UnoType<OUString>::get(),
                    beans::PropertyAttribute::BOUND );

        if ( getType() == DOCUMENT )
        {
            // streams cannot be created as direct children of document root
            uno::Sequence< ucb::ContentInfo > aSeq( 1 );

            // Folder.
            aSeq.getArray()[ 0 ].Type = TDOC_FOLDER_CONTENT_TYPE;
            aSeq.getArray()[ 0 ].Attributes = ucb::ContentInfoAttribute::KIND_FOLDER;
            aSeq.getArray()[ 0 ].Properties = std::move(aProps);

            return aSeq;
        }
        else
        {
            uno::Sequence< ucb::ContentInfo > aSeq( 2 );

            // Folder.
            aSeq.getArray()[ 0 ].Type = TDOC_FOLDER_CONTENT_TYPE;
            aSeq.getArray()[ 0 ].Attributes
                = ucb::ContentInfoAttribute::KIND_FOLDER;
            aSeq.getArray()[ 0 ].Properties = aProps;

            // Stream.
            aSeq.getArray()[ 1 ].Type = TDOC_STREAM_CONTENT_TYPE;
            aSeq.getArray()[ 1 ].Attributes
                = ucb::ContentInfoAttribute::INSERT_WITH_INPUTSTREAM
                  | ucb::ContentInfoAttribute::KIND_DOCUMENT;
            aSeq.getArray()[ 1 ].Properties = std::move(aProps);

            return aSeq;
        }
    }
    else
    {
        OSL_FAIL( "getCreatableContentsInfo called on non-contentcreator "
                    "object!" );

        return uno::Sequence< ucb::ContentInfo >( 0 );
    }
}


bool ContentProperties::isContentCreator() const
{
    return ( getType() == FOLDER ) || ( getType() == DOCUMENT );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
