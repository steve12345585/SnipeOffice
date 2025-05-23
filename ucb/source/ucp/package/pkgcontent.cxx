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

#include <osl/diagnose.h>

#include <rtl/ustring.hxx>
#include <com/sun/star/beans/IllegalTypeException.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/beans/PropertyExistException.hpp>
#include <com/sun/star/beans/PropertyState.hpp>
#include <com/sun/star/container/XEnumerationAccess.hpp>
#include <com/sun/star/container/XHierarchicalNameAccess.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/container/XNamed.hpp>
#include <com/sun/star/io/BufferSizeExceededException.hpp>
#include <com/sun/star/io/NotConnectedException.hpp>
#include <com/sun/star/io/XActiveDataSink.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/lang/IllegalAccessException.hpp>
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
#include <com/sun/star/util/XChangesBatch.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <comphelper/propertysequence.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <ucbhelper/contentidentifier.hxx>
#include <ucbhelper/propertyvalueset.hxx>
#include <ucbhelper/cancelcommandexecution.hxx>
#include <ucbhelper/macros.hxx>
#include <utility>
#include "pkgcontent.hxx"
#include "pkgprovider.hxx"
#include "pkgresultset.hxx"

#include "../inc/urihelper.hxx"

using namespace com::sun::star;
using namespace package_ucp;

#define NONE_MODIFIED           sal_uInt32( 0x00 )
#define MEDIATYPE_MODIFIED      sal_uInt32( 0x01 )
#define COMPRESSED_MODIFIED     sal_uInt32( 0x02 )
#define ENCRYPTED_MODIFIED      sal_uInt32( 0x04 )
#define ENCRYPTIONKEY_MODIFIED  sal_uInt32( 0x08 )


// ContentProperties Implementation.


ContentProperties::ContentProperties( const OUString& rContentType )
: aContentType( rContentType ),
  nSize( 0 ),
  bCompressed( true ),
  bEncrypted( false ),
  bHasEncryptedEntries( false )
{
    bIsFolder = rContentType == PACKAGE_FOLDER_CONTENT_TYPE || rContentType == PACKAGE_ZIP_FOLDER_CONTENT_TYPE;
    bIsDocument = !bIsFolder;

    OSL_ENSURE( bIsFolder || rContentType == PACKAGE_STREAM_CONTENT_TYPE || rContentType == PACKAGE_ZIP_STREAM_CONTENT_TYPE,
                "ContentProperties::ContentProperties - Unknown type!" );
}


uno::Sequence< ucb::ContentInfo >
ContentProperties::getCreatableContentsInfo( PackageUri const & rUri ) const
{
    if ( bIsFolder )
    {
        uno::Sequence< beans::Property > aProps( 1 );
        aProps.getArray()[ 0 ] = beans::Property(
                    u"Title"_ustr,
                    -1,
                    cppu::UnoType<OUString>::get(),
                    beans::PropertyAttribute::BOUND );

        uno::Sequence< ucb::ContentInfo > aSeq( 2 );

        // Folder.
        aSeq.getArray()[ 0 ].Type
            = Content::getContentType( rUri.getScheme(), true );
        aSeq.getArray()[ 0 ].Attributes
            = ucb::ContentInfoAttribute::KIND_FOLDER;
        aSeq.getArray()[ 0 ].Properties = aProps;

        // Stream.
        aSeq.getArray()[ 1 ].Type
            = Content::getContentType( rUri.getScheme(), false );
        aSeq.getArray()[ 1 ].Attributes
            = ucb::ContentInfoAttribute::INSERT_WITH_INPUTSTREAM
              | ucb::ContentInfoAttribute::KIND_DOCUMENT;
        aSeq.getArray()[ 1 ].Properties = std::move(aProps);

        return aSeq;
    }
    else
    {
        return uno::Sequence< ucb::ContentInfo >( 0 );
    }
}


// Content Implementation.


// static ( "virtual" ctor )
rtl::Reference<Content> Content::create(
            const uno::Reference< uno::XComponentContext >& rxContext,
            ContentProvider* pProvider,
            const uno::Reference< ucb::XContentIdentifier >& Identifier )
{
    OUString aURL = Identifier->getContentIdentifier();
    PackageUri aURI( aURL );
    ContentProperties aProps;
    uno::Reference< container::XHierarchicalNameAccess > xPackage;

    if ( loadData( pProvider, aURI, aProps, xPackage ) )
    {
        // resource exists

        sal_Int32 nLastSlash = aURL.lastIndexOf( '/' );
        if ( ( nLastSlash + 1 ) == aURL.getLength() )
        {
            // Client explicitly requested a folder!
            if ( !aProps.bIsFolder )
                return nullptr;
        }

        uno::Reference< ucb::XContentIdentifier > xId
            = new ::ucbhelper::ContentIdentifier( aURI.getUri() );
        return new Content( rxContext, pProvider, xId, xPackage, std::move(aURI), std::move(aProps) );
    }
    else
    {
        // resource doesn't exist

        bool bFolder = false;

        // Guess type according to URI.
        sal_Int32 nLastSlash = aURL.lastIndexOf( '/' );
        if ( ( nLastSlash + 1 ) == aURL.getLength() )
            bFolder = true;

        uno::Reference< ucb::XContentIdentifier > xId
            = new ::ucbhelper::ContentIdentifier( aURI.getUri() );

        ucb::ContentInfo aInfo;
        if ( bFolder || aURI.isRootFolder() )
            aInfo.Type = getContentType( aURI.getScheme(), true );
        else
            aInfo.Type = getContentType( aURI.getScheme(), false );

        return new Content( rxContext, pProvider, xId, xPackage, std::move(aURI), std::move(aInfo) );
    }
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

    PackageUri aURI( Identifier->getContentIdentifier() );

    if ( !Info.Type.equalsIgnoreAsciiCase(
                getContentType( aURI.getScheme(), true ) ) &&
         !Info.Type.equalsIgnoreAsciiCase(
                getContentType( aURI.getScheme(), false ) ) )
        return nullptr;

    uno::Reference< container::XHierarchicalNameAccess > xPackage = pProvider->createPackage( aURI );

    uno::Reference< ucb::XContentIdentifier > xId
        = new ::ucbhelper::ContentIdentifier( aURI.getUri() );
    return new Content( rxContext, pProvider, xId, xPackage, std::move(aURI), Info );
}


// static
OUString Content::getContentType(
    std::u16string_view aScheme, bool bFolder )
{
    return ( OUString::Concat("application/")
             + aScheme
             + ( bFolder
                 ? std::u16string_view(u"-folder")
                 : std::u16string_view(u"-stream") ) );
}


Content::Content(
        const uno::Reference< uno::XComponentContext >& rxContext,
        ContentProvider* pProvider,
        const uno::Reference< ucb::XContentIdentifier >& Identifier,
        uno::Reference< container::XHierarchicalNameAccess > Package,
        PackageUri aUri,
        ContentProperties aProps )
: ContentImplHelper( rxContext, pProvider, Identifier ),
  m_aUri(std::move( aUri )),
  m_aProps(std::move( aProps )),
  m_eState( PERSISTENT ),
  m_xPackage(std::move( Package )),
  m_pProvider( pProvider ),
  m_nModifiedProps( NONE_MODIFIED )
{
}


Content::Content(
        const uno::Reference< uno::XComponentContext >& rxContext,
        ContentProvider* pProvider,
        const uno::Reference< ucb::XContentIdentifier >& Identifier,
        uno::Reference< container::XHierarchicalNameAccess > Package,
        PackageUri aUri,
        const ucb::ContentInfo& Info )
  : ContentImplHelper( rxContext, pProvider, Identifier ),
  m_aUri(std::move( aUri )),
  m_aProps( Info.Type ),
  m_eState( TRANSIENT ),
  m_xPackage(std::move( Package )),
  m_pProvider( pProvider ),
  m_nModifiedProps( NONE_MODIFIED )
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
    uno::Any aRet;

    if ( isFolder() )
        aRet = cppu::queryInterface(
                rType, static_cast< ucb::XContentCreator * >( this ) );

    return aRet.hasValue() ? aRet : ContentImplHelper::queryInterface( rType );
}


// XTypeProvider methods.


XTYPEPROVIDER_COMMON_IMPL( Content );


// virtual
uno::Sequence< uno::Type > SAL_CALL Content::getTypes()
{
    if ( isFolder() )
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
    return u"com.sun.star.comp.ucb.PackageContent"_ustr;
}


// virtual
uno::Sequence< OUString > SAL_CALL Content::getSupportedServiceNames()
{
    return { isFolder()? u"com.sun.star.ucb.PackageFolderContent"_ustr:u"com.sun.star.ucb.PackageStreamContent"_ustr } ;
}


// XContent methods.


// virtual
OUString SAL_CALL Content::getContentType()
{
    return m_aProps.aContentType;
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


        // Note: Implemented by base class.
        aRet <<= getPropertySetInfo( Environment );
    }
    else if ( aCommand.Name == "getCommandInfo" )
    {

        // getCommandInfo


        // Note: Implemented by base class.
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
    else if ( !m_aUri.isRootFolder() && aCommand.Name == "insert" )
    {

        // insert


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
    else if ( !m_aUri.isRootFolder() && aCommand.Name == "delete" )
    {

        // delete


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

        // transfer
        //      ( Not available at stream objects )


        ucb::TransferInfo aInfo;
        if ( !( aCommand.Argument >>= aInfo ) )
        {
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
    else if ( aCommand.Name == "createNewContent" && isFolder() )
    {

        // createNewContent
        //      ( Not available at stream objects )


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
    else if ( aCommand.Name == "flush" )
    {

        // flush
        //      ( Not available at stream objects )


        if( !flushData() )
        {
            uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
            {
                {"Uri", uno::Any(m_xIdentifier->getContentIdentifier())}
            }));
            ucbhelper::cancelCommandExecution(
                ucb::IOErrorCode_CANT_WRITE,
                aArgs,
                Environment,
                u"Cannot write file to disk!"_ustr,
                this );
            // Unreachable
        }
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
    // @@@ Implement logic to abort running commands, if this makes
    //     sense for your content.
}


// XContentCreator methods.


// virtual
uno::Sequence< ucb::ContentInfo > SAL_CALL
Content::queryCreatableContentsInfo()
{
    return m_aProps.getCreatableContentsInfo( m_aUri );
}


// virtual
uno::Reference< ucb::XContent > SAL_CALL
Content::createNewContent( const ucb::ContentInfo& Info )
{
    if ( isFolder() )
    {
        osl::Guard< osl::Mutex > aGuard( m_aMutex );

        if ( Info.Type.isEmpty() )
            return uno::Reference< ucb::XContent >();

        if ( !Info.Type.equalsIgnoreAsciiCase(
                getContentType( m_aUri.getScheme(), true ) ) &&
             !Info.Type.equalsIgnoreAsciiCase(
                getContentType( m_aUri.getScheme(), false ) ) )
            return uno::Reference< ucb::XContent >();

        OUString aURL = m_aUri.getUri() + "/";

        if ( Info.Type.equalsIgnoreAsciiCase(
                getContentType( m_aUri.getScheme(), true ) ) )
            aURL += "New_Folder";
        else
            aURL += "New_Stream";

        uno::Reference< ucb::XContentIdentifier > xId(
            new ::ucbhelper::ContentIdentifier( aURL ) );

        return create( m_xContext, m_pProvider, xId, Info );
    }
    else
    {
        OSL_FAIL( "createNewContent called on non-folder object!" );
        return uno::Reference< ucb::XContent >();
    }
}


// Non-interface methods.


// virtual
OUString Content::getParentURL()
{
    return m_aUri.getParentUri();
}


// static
uno::Reference< sdbc::XRow > Content::getPropertyValues(
                const uno::Reference< uno::XComponentContext >& rxContext,
                const uno::Sequence< beans::Property >& rProperties,
                ContentProvider* pProvider,
                const OUString& rContentId )
{
    ContentProperties aData;
    uno::Reference< container::XHierarchicalNameAccess > xPackage;
    if ( loadData( pProvider, PackageUri( rContentId ), aData, xPackage ) )
    {
        return getPropertyValues( rxContext,
                                  rProperties,
                                  aData,
                                  rtl::Reference<
                                    ::ucbhelper::ContentProviderImplHelper >(
                                        pProvider ),
                                  rContentId );
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
        const rtl::Reference< ::ucbhelper::ContentProviderImplHelper >&
            rProvider,
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
                xRow->appendString ( rProp, rData.aContentType );
            }
            else if ( rProp.Name == "Title" )
            {
                xRow->appendString ( rProp, rData.aTitle );
            }
            else if ( rProp.Name == "IsDocument" )
            {
                xRow->appendBoolean( rProp, rData.bIsDocument );
            }
            else if ( rProp.Name == "IsFolder" )
            {
                xRow->appendBoolean( rProp, rData.bIsFolder );
            }
            else if ( rProp.Name == "CreatableContentsInfo" )
            {
                xRow->appendObject(
                    rProp, uno::Any(
                        rData.getCreatableContentsInfo(
                            PackageUri( rContentId ) ) ) );
            }
            else if ( rProp.Name == "MediaType" )
            {
                xRow->appendString ( rProp, rData.aMediaType );
            }
            else if ( rProp.Name == "Size" )
            {
                // Property only available for streams.
                if ( rData.bIsDocument )
                    xRow->appendLong( rProp, rData.nSize );
                else
                    xRow->appendVoid( rProp );
            }
            else if ( rProp.Name == "Compressed" )
            {
                // Property only available for streams.
                if ( rData.bIsDocument )
                    xRow->appendBoolean( rProp, rData.bCompressed );
                else
                    xRow->appendVoid( rProp );
            }
            else if ( rProp.Name == "Encrypted" )
            {
                // Property only available for streams.
                if ( rData.bIsDocument )
                    xRow->appendBoolean( rProp, rData.bEncrypted );
                else
                    xRow->appendVoid( rProp );
            }
            else if ( rProp.Name == "HasEncryptedEntries" )
            {
                // Property only available for root folder.
                PackageUri aURI( rContentId );
                if ( aURI.isRootFolder() )
                    xRow->appendBoolean( rProp, rData.bHasEncryptedEntries );
                else
                    xRow->appendVoid( rProp );
            }
            else
            {
                // Not a Core Property! Maybe it's an Additional Core Property?!

                if ( !bTriedToGetAdditionalPropSet && !xAdditionalPropSet.is() )
                {
                    xAdditionalPropSet =
                            rProvider->getAdditionalPropertySet( rContentId,
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
            beans::Property(
                u"ContentType"_ustr,
                -1,
                cppu::UnoType<OUString>::get(),
                beans::PropertyAttribute::BOUND
                    | beans::PropertyAttribute::READONLY ),
            rData.aContentType );
        xRow->appendString(
            beans::Property(
                u"Title"_ustr,
                -1,
                cppu::UnoType<OUString>::get(),
                beans::PropertyAttribute::BOUND ),
            rData.aTitle );
        xRow->appendBoolean(
            beans::Property(
                u"IsDocument"_ustr,
                -1,
                cppu::UnoType<bool>::get(),
                beans::PropertyAttribute::BOUND
                    | beans::PropertyAttribute::READONLY ),
            rData.bIsDocument );
        xRow->appendBoolean(
            beans::Property(
                u"IsFolder"_ustr,
                -1,
                cppu::UnoType<bool>::get(),
                beans::PropertyAttribute::BOUND
                    | beans::PropertyAttribute::READONLY ),
            rData.bIsFolder );
        xRow->appendObject(
            beans::Property(
                u"CreatableContentsInfo"_ustr,
                -1,
                cppu::UnoType<uno::Sequence< ucb::ContentInfo >>::get(),
                beans::PropertyAttribute::BOUND
                | beans::PropertyAttribute::READONLY ),
            uno::Any(
                rData.getCreatableContentsInfo( PackageUri( rContentId ) ) ) );
        xRow->appendString(
            beans::Property(
                u"MediaType"_ustr,
                -1,
                cppu::UnoType<OUString>::get(),
                beans::PropertyAttribute::BOUND ),
            rData.aMediaType );

        // Properties only available for streams.
        if ( rData.bIsDocument )
        {
            xRow->appendLong(
                beans::Property(
                    u"Size"_ustr,
                    -1,
                    cppu::UnoType<sal_Int64>::get(),
                    beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY ),
                rData.nSize );

            xRow->appendBoolean(
                beans::Property(
                    u"Compressed"_ustr,
                    -1,
                    cppu::UnoType<bool>::get(),
                    beans::PropertyAttribute::BOUND ),
                rData.bCompressed );

            xRow->appendBoolean(
                beans::Property(
                    u"Encrypted"_ustr,
                    -1,
                    cppu::UnoType<bool>::get(),
                    beans::PropertyAttribute::BOUND ),
                rData.bEncrypted );
        }

        // Properties only available for root folder.
        PackageUri aURI( rContentId );
        if ( aURI.isRootFolder() )
        {
            xRow->appendBoolean(
                beans::Property(
                    u"HasEncryptedEntries"_ustr,
                    -1,
                    cppu::UnoType<bool>::get(),
                    beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY ),
                rData.bHasEncryptedEntries );
        }

        // Append all Additional Core Properties.

        uno::Reference< beans::XPropertySet > xSet =
            rProvider->getAdditionalPropertySet( rContentId, false );
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
                              m_xProvider,
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
//    aEvent.PropertyName   =
    aEvent.PropertyHandle = -1;
//    aEvent.OldValue       =
//    aEvent.NewValue       =

    const beans::PropertyValue* pValues = rValues.getConstArray();
    sal_Int32 nCount = rValues.getLength();

    uno::Reference< ucb::XPersistentPropertySet > xAdditionalPropSet;
    bool bTriedToGetAdditionalPropSet = false;
    bool bExchange = false;
    bool bStore    = false;
    OUString aNewTitle;
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
            if ( m_aUri.isRootFolder() )
            {
                // Read-only property!
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
                        if ( aNewValue != m_aProps.aTitle )
                        {
                            // modified title -> modified URL -> exchange !
                            if ( m_eState == PERSISTENT )
                                bExchange = true;

                            // new value will be set later...
                            aNewTitle = aNewValue;

                            // remember position within sequence of values
                            // (for error handling).
                            nTitlePos = n;
                        }
                    }
                    else
                    {
                        aRetRange[ n ] <<=
                            lang::IllegalArgumentException(
                                u"Empty title not allowed!"_ustr,
                                getXWeak(),
                                -1 );
                    }
                }
                else
                {
                    aRetRange[ n ] <<=
                        beans::IllegalTypeException(
                            u"Property value has wrong type!"_ustr,
                            getXWeak() );
                }
            }
        }
        else if ( rValue.Name == "MediaType" )
        {
            OUString aNewValue;
            if ( rValue.Value >>= aNewValue )
            {
                if ( aNewValue != m_aProps.aMediaType )
                {
                    aEvent.PropertyName = rValue.Name;
                    aEvent.OldValue     <<= m_aProps.aMediaType;
                    aEvent.NewValue     <<= aNewValue;

                    m_aProps.aMediaType = aNewValue;
                    nChanged++;
                    bStore = true;
                    m_nModifiedProps |= MEDIATYPE_MODIFIED;
                }
            }
            else
            {
                aRetRange[ n ] <<= beans::IllegalTypeException(
                                u"Property value has wrong type!"_ustr,
                                getXWeak() );
            }
        }
        else if ( rValue.Name == "Size" )
        {
            // Read-only property!
            aRetRange[ n ] <<= lang::IllegalAccessException(
                            u"Property is read-only!"_ustr,
                            getXWeak() );
        }
        else if ( rValue.Name == "Compressed" )
        {
            // Property only available for streams.
            if ( m_aProps.bIsDocument )
            {
                bool bNewValue;
                if ( rValue.Value >>= bNewValue )
                {
                    if ( bNewValue != m_aProps.bCompressed )
                    {
                        aEvent.PropertyName = rValue.Name;
                        aEvent.OldValue <<= m_aProps.bCompressed;
                        aEvent.NewValue <<= bNewValue;

                        m_aProps.bCompressed = bNewValue;
                        nChanged++;
                        bStore = true;
                        m_nModifiedProps |= COMPRESSED_MODIFIED;
                    }
                }
                else
                {
                    aRetRange[ n ] <<= beans::IllegalTypeException(
                                u"Property value has wrong type!"_ustr,
                                getXWeak() );
                }
            }
            else
            {
                aRetRange[ n ] <<= beans::UnknownPropertyException(
                                u"Compressed only supported by streams!"_ustr,
                                getXWeak() );
            }
        }
        else if ( rValue.Name == "Encrypted" )
        {
            // Property only available for streams.
            if ( m_aProps.bIsDocument )
            {
                bool bNewValue;
                if ( rValue.Value >>= bNewValue )
                {
                    if ( bNewValue != m_aProps.bEncrypted )
                    {
                        aEvent.PropertyName = rValue.Name;
                        aEvent.OldValue <<= m_aProps.bEncrypted;
                        aEvent.NewValue <<= bNewValue;

                        m_aProps.bEncrypted = bNewValue;
                        nChanged++;
                        bStore = true;
                        m_nModifiedProps |= ENCRYPTED_MODIFIED;
                    }
                }
                else
                {
                    aRetRange[ n ] <<= beans::IllegalTypeException(
                                u"Property value has wrong type!"_ustr,
                                getXWeak() );
                }
            }
            else
            {
                aRetRange[ n ] <<= beans::UnknownPropertyException(
                                u"Encrypted only supported by streams!"_ustr,
                                getXWeak() );
            }
        }
        else if ( rValue.Name == "HasEncryptedEntries" )
        {
            // Read-only property!
            aRetRange[ n ] <<= lang::IllegalAccessException(
                            u"Property is read-only!"_ustr,
                            getXWeak() );
        }
        else if ( rValue.Name == "EncryptionKey" )
        {
            // @@@ This is a temporary solution. In the future submitting
            //     the key should be done using an interaction handler!

            // Write-Only property. Only supported by root folder and streams
            // (all non-root folders of a package have the same encryption key).
            if ( m_aUri.isRootFolder() || m_aProps.bIsDocument )
            {
                uno::Sequence < sal_Int8 > aNewValue;
                if ( rValue.Value >>= aNewValue )
                {
                    if ( aNewValue != m_aProps.aEncryptionKey )
                    {
                        aEvent.PropertyName = rValue.Name;
                        aEvent.OldValue     <<= m_aProps.aEncryptionKey;
                        aEvent.NewValue     <<= aNewValue;

                        m_aProps.aEncryptionKey = std::move(aNewValue);
                        nChanged++;
                        bStore = true;
                        m_nModifiedProps |= ENCRYPTIONKEY_MODIFIED;
                    }
                }
                else
                {
                    aRetRange[ n ] <<= beans::IllegalTypeException(
                                u"Property value has wrong type!"_ustr,
                                getXWeak() );
                }
            }
            else
            {
                aRetRange[ n ] <<= beans::UnknownPropertyException(
                        u"EncryptionKey not supported by non-root folder!"_ustr,
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
                    uno::Any aOldValue
                        = xAdditionalPropSet->getPropertyValue( rValue.Name );
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
        uno::Reference< ucb::XContentIdentifier > xOldId = m_xIdentifier;

        // Assemble new content identifier...
        OUString aNewURL = m_aUri.getParentUri() + "/" +
            ::ucb_impl::urihelper::encodeSegment( aNewTitle );
        uno::Reference< ucb::XContentIdentifier > xNewId
            = new ::ucbhelper::ContentIdentifier( aNewURL );

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
            // Do not set new title!
            aNewTitle.clear();

            // Set error .
            aRetRange[ nTitlePos ] <<= uno::Exception(
                    u"Exchange failed!"_ustr,
                    getXWeak() );
        }
    }

    if ( !aNewTitle.isEmpty() )
    {
        aEvent.PropertyName = "Title";
        aEvent.OldValue     <<= m_aProps.aTitle;
        aEvent.NewValue     <<= aNewTitle;

        m_aProps.aTitle = aNewTitle;

        aChanges.getArray()[ nChanged ] = std::move(aEvent);
        nChanged++;
    }

    if ( nChanged > 0 )
    {
        // Save changes, if content was already made persistent.
        if ( ( m_nModifiedProps & ENCRYPTIONKEY_MODIFIED ) ||
             ( bStore && ( m_eState == PERSISTENT ) ) )
        {
            if ( !storeData( uno::Reference< io::XInputStream >() ) )
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

        aGuard.clear();
        aChanges.realloc( nChanged );
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
            = new DynamicResultSet( m_xContext, this, rArg, xEnv );
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

        uno::Reference< io::XOutputStream > xOut( rArg.Sink, uno::UNO_QUERY );
        if ( xOut.is() )
        {
            // PUSH: write data into xOut

            uno::Reference< io::XInputStream > xIn = getInputStream();
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

                uno::Reference< io::XInputStream > xIn = getInputStream();
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
                // Note: aOpenCommand.Sink may contain an XStream
                //       implementation. Support for this type of
                //       sink is optional...
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

    return uno::Any();
}


void Content::insert(
            const uno::Reference< io::XInputStream >& xStream,
            sal_Int32 nNameClashResolve,
            const uno::Reference< ucb::XCommandEnvironment >& xEnv )
{
    osl::ClearableGuard< osl::Mutex > aGuard( m_aMutex );

    // Check, if all required properties were set.
    if ( isFolder() )
    {
        // Required: Title

        if ( m_aProps.aTitle.isEmpty() )
            m_aProps.aTitle = m_aUri.getName();
    }
    else
    {
        // Required: rArg.Data

        if ( !xStream.is() )
        {
            ucbhelper::cancelCommandExecution(
                uno::Any( ucb::MissingInputStreamException(
                                OUString(),
                                getXWeak() ) ),
                xEnv );
            // Unreachable
        }

        // Required: Title

        if ( m_aProps.aTitle.isEmpty() )
            m_aProps.aTitle = m_aUri.getName();
    }

    OUString aNewURL = m_aUri.getParentUri();
    if (1 + aNewURL.lastIndexOf('/') != aNewURL.getLength())
        aNewURL += "/";
    aNewURL += ::ucb_impl::urihelper::encodeSegment( m_aProps.aTitle );
    PackageUri aNewUri( aNewURL );

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
                                    m_aProps.aTitle ) ),
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
                    OUString aNew = aNewUri.getUri() + "_" + OUString::number( ++nTry );
                    aNewUri.setUri( aNew );
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
                    m_aProps.aTitle += "_";
                    m_aProps.aTitle += OUString::number( nTry );
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
    bool bNewId = ( m_aUri.getUri() != aNewUri.getUri() );

    if ( bNewId )
    {
        m_xIdentifier = new ::ucbhelper::ContentIdentifier( aNewURL );
        m_aUri = std::move(aNewUri);
    }

    if ( !storeData( xStream ) )
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
        // Take over correct default values from underlying packager...
        uno::Reference< container::XHierarchicalNameAccess > xXHierarchicalNameAccess;
        loadData( m_pProvider,
                  m_aUri,
                  m_aProps,
                  xXHierarchicalNameAccess );

        aGuard.clear();
        inserted();
    }
}


void Content::destroy(
                bool bDeletePhysical,
                const uno::Reference< ucb::XCommandEnvironment >& xEnv )
{
    // @@@ take care about bDeletePhysical -> trashcan support

    osl::ClearableGuard< osl::Mutex > aGuard( m_aMutex );

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

    if ( isFolder() )
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

    // Is source a package content?
    if ( ( rInfo.SourceURL.isEmpty() ) ||
         ( rInfo.SourceURL.compareTo(
            m_aUri.getUri(), PACKAGE_URL_SCHEME_LENGTH + 3 ) != 0 ) )
    {
        ucbhelper::cancelCommandExecution(
            uno::Any( ucb::InteractiveBadTransferURLException(
                                OUString(),
                                getXWeak() ) ),
            xEnv );
        // Unreachable
    }

    // Is source not a parent of me / not me?
    OUString aId = m_aUri.getParentUri() + "/";

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


    // 0) Obtain content object for source.


    uno::Reference< ucb::XContentIdentifier > xId
        = new ::ucbhelper::ContentIdentifier( rInfo.SourceURL );

    // Note: The static cast is okay here, because its sure that
    //       m_xProvider is always the PackageContentProvider.
    rtl::Reference< Content > xSource;

    try
    {
        xSource = static_cast< Content * >(
                        m_xProvider->queryContent( xId ).get() );
    }
    catch ( ucb::IllegalIdentifierException const & )
    {
        // queryContent
    }

    if ( !xSource.is() )
    {
        uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
        {
            {"Uri", uno::Any(xId->getContentIdentifier())}
        }));
        ucbhelper::cancelCommandExecution(
            ucb::IOErrorCode_CANT_READ,
            aArgs,
            xEnv,
            u"Cannot instantiate source object!"_ustr,
            this );
        // Unreachable
    }


    // 1) Create new child content.


    ucb::ContentInfo aContentInfo;
    aContentInfo.Type = xSource->isFolder()
            ? getContentType( m_aUri.getScheme(), true )
            : getContentType( m_aUri.getScheme(), false );
    aContentInfo.Attributes = 0;

    // Note: The static cast is okay here, because its sure that
    //       createNewContent always creates a Content.
    rtl::Reference< Content > xTarget
        = static_cast< Content * >( createNewContent( aContentInfo ).get() );
    if ( !xTarget.is() )
    {
        uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
        {
            {"Folder", uno::Any(aId)}
        }));
        ucbhelper::cancelCommandExecution(
            ucb::IOErrorCode_CANT_CREATE,
            aArgs,
            xEnv,
            u"XContentCreator::createNewContent failed!"_ustr,
            this );
        // Unreachable
    }


    // 2) Copy data from source content to child content.


    uno::Sequence< beans::Property > aSourceProps
                    = xSource->getPropertySetInfo( xEnv )->getProperties();
    sal_Int32 nCount = aSourceProps.getLength();

    if ( nCount )
    {
        bool bHadTitle = rInfo.NewTitle.isEmpty();

        // Get all source values.
        uno::Reference< sdbc::XRow > xRow
            = xSource->getPropertyValues( aSourceProps );

        uno::Sequence< beans::PropertyValue > aValues( nCount );
        beans::PropertyValue* pValues = aValues.getArray();

        const beans::Property* pProps = aSourceProps.getConstArray();
        for ( sal_Int32 n = 0; n < nCount; ++n )
        {
            const beans::Property& rProp  = pProps[ n ];
            beans::PropertyValue&  rValue = pValues[ n ];

            rValue.Name   = rProp.Name;
            rValue.Handle = rProp.Handle;

            if ( !bHadTitle && rProp.Name == "Title" )
            {
                // Set new title instead of original.
                bHadTitle = true;
                rValue.Value <<= rInfo.NewTitle;
            }
            else
                rValue.Value
                    = xRow->getObject( n + 1,
                                       uno::Reference<
                                            container::XNameAccess >() );

            rValue.State = beans::PropertyState_DIRECT_VALUE;

            if ( rProp.Attributes & beans::PropertyAttribute::REMOVABLE )
            {
                // Add Additional Core Property.
                try
                {
                    xTarget->addProperty( rProp.Name,
                                          rProp.Attributes,
                                          rValue.Value );
                }
                catch ( beans::PropertyExistException const & )
                {
                }
                catch ( beans::IllegalTypeException const & )
                {
                }
                catch ( lang::IllegalArgumentException const & )
                {
                }
            }
        }

        // Set target values.
        xTarget->setPropertyValues( aValues, xEnv );
    }


    // 3) Commit (insert) child.


    xTarget->insert( xSource->getInputStream(), rInfo.NameClash, xEnv );


    // 4) Transfer (copy) children of source.


    if ( xSource->isFolder() )
    {
        uno::Reference< container::XEnumeration > xIter
            = xSource->getIterator();
        if ( xIter.is() )
        {
            while ( xIter->hasMoreElements() )
            {
                try
                {
                    uno::Reference< container::XNamed > xNamed;
                    xIter->nextElement() >>= xNamed;

                    if ( !xNamed.is() )
                    {
                        OSL_FAIL( "Content::transfer - Got no XNamed!" );
                        break;
                    }

                    OUString aName = xNamed->getName();

                    if ( aName.isEmpty() )
                    {
                        OSL_FAIL( "Content::transfer - Empty name!" );
                        break;
                    }

                    OUString aChildId = xId->getContentIdentifier();
                    if ( ( aChildId.lastIndexOf( '/' ) + 1 )
                                                != aChildId.getLength() )
                        aChildId += "/";

                    aChildId += ::ucb_impl::urihelper::encodeSegment( aName );

                    ucb::TransferInfo aInfo;
                    aInfo.MoveData  = false;
                    aInfo.NewTitle.clear();
                    aInfo.SourceURL = aChildId;
                    aInfo.NameClash = rInfo.NameClash;

                    // Transfer child to target.
                    xTarget->transfer( aInfo, xEnv );
                }
                catch ( container::NoSuchElementException const & )
                {
                }
                catch ( lang::WrappedTargetException const & )
                {
                }
            }
        }
    }


    // 5) Destroy source ( when moving only ) .


    if ( !rInfo.MoveData )
        return;

    xSource->destroy( true, xEnv );

    // Remove all persistent data of source and its children.
    if ( !xSource->removeData() )
    {
        uno::Sequence<uno::Any> aArgs(comphelper::InitAnyPropertySequence(
        {
            {"Uri", uno::Any(xSource->m_xIdentifier->getContentIdentifier())}
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
    xSource->removeAdditionalPropertySet();
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

    // Exchange own identity.

    // Fail, if a content with given id already exists.
    PackageUri aNewUri( xNewId->getContentIdentifier() );
    if ( !hasData( aNewUri ) )
    {
        OUString aOldURL = m_xIdentifier->getContentIdentifier();

        aGuard.clear();
        if ( exchange( xNewId ) )
        {
            m_aUri = std::move(aNewUri);
            if ( isFolder() )
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

    OSL_FAIL( "Content::exchangeIdentity - Panic! Cannot exchange identity!" );
    return false;
}


void Content::queryChildren( ContentRefList& rChildren )
{
    // Obtain a list with a snapshot of all currently instantiated contents
    // from provider and extract the contents which are direct children
    // of this content.

    ::ucbhelper::ContentRefList aAllContents;
    m_xProvider->queryExistingContents( aAllContents );

    OUString aURL = m_xIdentifier->getContentIdentifier();

    OSL_ENSURE( aURL.lastIndexOf( '/' ) != ( aURL.getLength() - 1 ),
                "Content::queryChildren - Invalid URL!" );

    aURL += "/";

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
            if ( aChildURL.indexOf( '/', nLen ) == -1 )
            {
                // No further slashes. It's a child!
                rChildren.emplace_back(
                        static_cast< Content * >( xChild.get() ) );
            }
        }
    }
}


uno::Reference< container::XHierarchicalNameAccess > Content::getPackage(
                                                const PackageUri& rURI )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    if ( rURI.getPackage() == m_aUri.getPackage() )
    {
        if ( !m_xPackage.is() )
            m_xPackage = m_pProvider->createPackage( m_aUri );

        return m_xPackage;
    }

    return m_pProvider->createPackage( rURI );
}


uno::Reference< container::XHierarchicalNameAccess > Content::getPackage()
{
    return getPackage( m_aUri );
}


// static
bool Content::hasData(
            ContentProvider* pProvider,
            const PackageUri& rURI,
            uno::Reference< container::XHierarchicalNameAccess > & rxPackage )
{
    rxPackage = pProvider->createPackage( rURI );
    return rxPackage->hasByHierarchicalName( rURI.getPath() );
}


bool Content::hasData( const PackageUri& rURI )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    uno::Reference< container::XHierarchicalNameAccess > xPackage;
    if ( rURI.getPackage() == m_aUri.getPackage() )
    {
        xPackage = getPackage();
        return xPackage->hasByHierarchicalName( rURI.getPath() );
    }

    return hasData( m_pProvider, rURI, xPackage );
}


//static
bool Content::loadData(
            ContentProvider* pProvider,
            const PackageUri& rURI,
            ContentProperties& rProps,
            uno::Reference< container::XHierarchicalNameAccess > & rxPackage )
{
    rxPackage = pProvider->createPackage( rURI );

    if ( rURI.isRootFolder() )
    {
        // Properties available only from package
        uno::Reference< beans::XPropertySet > xPackagePropSet(
                                                rxPackage, uno::UNO_QUERY );

        OSL_ENSURE( xPackagePropSet.is(),
                    "Content::loadData - "
                    "Got no XPropertySet interface from package!" );

        if ( xPackagePropSet.is() )
        {
            // HasEncryptedEntries (only available at root folder)
            try
            {
                uno::Any aHasEncryptedEntries
                    = xPackagePropSet->getPropertyValue( u"HasEncryptedEntries"_ustr );
                if ( !( aHasEncryptedEntries >>= rProps.bHasEncryptedEntries ) )
                {
                    OSL_FAIL( "Content::loadData - "
                                "Got no HasEncryptedEntries value!" );
                    return false;
                }
            }
            catch ( beans::UnknownPropertyException const & )
            {
                OSL_FAIL( "Content::loadData - "
                            "Got no HasEncryptedEntries value!" );
                return false;
            }
            catch ( lang::WrappedTargetException const & )
            {
                OSL_FAIL( "Content::loadData - "
                            "Got no HasEncryptedEntries value!" );
                return false;
            }
        }
    }

    if ( !rxPackage->hasByHierarchicalName( rURI.getPath() ) )
        return false;

    try
    {
        uno::Any aEntry = rxPackage->getByHierarchicalName( rURI.getPath() );
        if ( aEntry.hasValue() )
        {
            uno::Reference< beans::XPropertySet > xPropSet;
            aEntry >>= xPropSet;

            if ( !xPropSet.is() )
            {
                OSL_FAIL( "Content::loadData - Got no XPropertySet interface!" );
                return false;
            }

            // Title
            rProps.aTitle = rURI.getName();

            // MediaType
            try
            {
                uno::Any aMediaType = xPropSet->getPropertyValue(u"MediaType"_ustr);
                if ( !( aMediaType >>= rProps.aMediaType ) )
                {
                    OSL_FAIL( "Content::loadData - Got no MediaType value!" );
                    return false;
                }
            }
            catch ( beans::UnknownPropertyException const & )
            {
                OSL_FAIL( "Content::loadData - Got no MediaType value!" );
                return false;
            }
            catch ( lang::WrappedTargetException const & )
            {
                OSL_FAIL( "Content::loadData - Got no MediaType value!" );
                return false;
            }

            uno::Reference< container::XEnumerationAccess > xEnumAccess;
            aEntry >>= xEnumAccess;

            // ContentType / IsFolder / IsDocument
            if ( xEnumAccess.is() )
            {
                // folder
                rProps.aContentType = getContentType( rURI.getScheme(), true );
                rProps.bIsDocument = false;
                rProps.bIsFolder = true;
            }
            else
            {
                // stream
                rProps.aContentType = getContentType( rURI.getScheme(), false );
                rProps.bIsDocument = true;
                rProps.bIsFolder = false;
            }

            if ( rProps.bIsDocument )
            {
                // Size ( only available for streams )
                try
                {
                    uno::Any aSize = xPropSet->getPropertyValue(u"Size"_ustr);
                    if ( !( aSize >>= rProps.nSize ) )
                    {
                        OSL_FAIL( "Content::loadData - Got no Size value!" );
                        return false;
                    }
                }
                catch ( beans::UnknownPropertyException const & )
                {
                    OSL_FAIL( "Content::loadData - Got no Size value!" );
                    return false;
                }
                catch ( lang::WrappedTargetException const & )
                {
                    OSL_FAIL( "Content::loadData - Got no Size value!" );
                    return false;
                }

                // Compressed ( only available for streams )
                try
                {
                    uno::Any aCompressed = xPropSet->getPropertyValue(u"Compressed"_ustr);
                    if ( !( aCompressed >>= rProps.bCompressed ) )
                    {
                        OSL_FAIL( "Content::loadData - Got no Compressed value!" );
                        return false;
                    }
                }
                catch ( beans::UnknownPropertyException const & )
                {
                    OSL_FAIL( "Content::loadData - Got no Compressed value!" );
                    return false;
                }
                catch ( lang::WrappedTargetException const & )
                {
                    OSL_FAIL( "Content::loadData - Got no Compressed value!" );
                    return false;
                }

                // Encrypted ( only available for streams )
                try
                {
                    uno::Any aEncrypted = xPropSet->getPropertyValue(u"Encrypted"_ustr);
                    if ( !( aEncrypted >>= rProps.bEncrypted ) )
                    {
                        OSL_FAIL( "Content::loadData - Got no Encrypted value!" );
                        return false;
                    }
                }
                catch ( beans::UnknownPropertyException const & )
                {
                    OSL_FAIL( "Content::loadData - Got no Encrypted value!" );
                    return false;
                }
                catch ( lang::WrappedTargetException const & )
                {
                    OSL_FAIL( "Content::loadData - Got no Encrypted value!" );
                    return false;
                }
            }
            return true;
        }
    }
    catch ( container::NoSuchElementException const & )
    {
        // getByHierarchicalName
    }

    return false;
}


void Content::renameData(
            const uno::Reference< ucb::XContentIdentifier >& xOldId,
            const uno::Reference< ucb::XContentIdentifier >& xNewId )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    PackageUri aURI( xOldId->getContentIdentifier() );
    uno::Reference< container::XHierarchicalNameAccess > xNA = getPackage(
                                                                        aURI );

    if ( !xNA->hasByHierarchicalName( aURI.getPath() ) )
        return;

    try
    {
        uno::Any aEntry = xNA->getByHierarchicalName( aURI.getPath() );
        uno::Reference< container::XNamed > xNamed;
        aEntry >>= xNamed;

        if ( !xNamed.is() )
        {
            OSL_FAIL( "Content::renameData - Got no XNamed interface!" );
            return;
        }

        PackageUri aNewURI( xNewId->getContentIdentifier() );

        // No success indicator!? No return value / exceptions specified.
        xNamed->setName( aNewURI.getName() );
    }
    catch ( container::NoSuchElementException const & )
    {
        // getByHierarchicalName
    }
}


bool Content::storeData( const uno::Reference< io::XInputStream >& xStream )
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    uno::Reference< container::XHierarchicalNameAccess > xNA = getPackage();

    uno::Reference< beans::XPropertySet > xPackagePropSet(
                                                    xNA, uno::UNO_QUERY );
    OSL_ENSURE( xPackagePropSet.is(),
                "Content::storeData - "
                "Got no XPropertySet interface from package!" );

    if ( !xPackagePropSet.is() )
        return false;

    if ( m_nModifiedProps & ENCRYPTIONKEY_MODIFIED )
    {
        if ( m_aUri.isRootFolder() )
        {
            // Property available only from package and from streams (see below)
            try
            {
                xPackagePropSet->setPropertyValue(
                        u"EncryptionKey"_ustr,
                        uno::Any( m_aProps.aEncryptionKey ) );
                m_nModifiedProps &= ~ENCRYPTIONKEY_MODIFIED;
            }
            catch ( beans::UnknownPropertyException const & )
            {
                // setPropertyValue
            }
            catch ( beans::PropertyVetoException const & )
            {
                // setPropertyValue
            }
            catch ( lang::IllegalArgumentException const & )
            {
                // setPropertyValue
            }
            catch ( lang::WrappedTargetException const & )
            {
                // setPropertyValue
            }
        }
    }

    if ( !xNA->hasByHierarchicalName( m_aUri.getPath() ) )
    {
//        if ( !bCreate )
//            return sal_True;

        try
        {
            // Create new resource...
            uno::Reference< lang::XSingleServiceFactory > xFac(
                                                    xNA, uno::UNO_QUERY );
            if ( !xFac.is() )
            {
                OSL_FAIL( "Content::storeData - "
                            "Got no XSingleServiceFactory interface!" );
                return false;
            }

            uno::Sequence< uno::Any > aArgs{ uno::Any(isFolder()) };

            uno::Reference< uno::XInterface > xNew
                = xFac->createInstanceWithArguments( aArgs );

            if ( !xNew.is() )
            {
                OSL_FAIL( "Content::storeData - createInstance failed!" );
                return false;
            }

            PackageUri aParentUri( getParentURL() );
            uno::Any aEntry
                = xNA->getByHierarchicalName( aParentUri.getPath() );
            uno::Reference< container::XNameContainer > xParentContainer;
            aEntry >>= xParentContainer;

            if ( !xParentContainer.is() )
            {
                OSL_FAIL( "Content::storeData - "
                            "Got no XNameContainer interface!" );
                return false;
            }

            xParentContainer->insertByName( m_aProps.aTitle,
                                            uno::Any( xNew ) );
        }
        catch ( lang::IllegalArgumentException const & )
        {
            // insertByName
            OSL_FAIL( "Content::storeData - insertByName failed!" );
            return false;
        }
        catch ( uno::RuntimeException const & )
        {
            throw;
        }
        catch ( container::ElementExistException const & )
        {
            // insertByName
            OSL_FAIL( "Content::storeData - insertByName failed!" );
            return false;
        }
        catch ( lang::WrappedTargetException const & )
        {
            // insertByName
            OSL_FAIL( "Content::storeData - insertByName failed!" );
            return false;
        }
        catch ( container::NoSuchElementException const & )
        {
            // getByHierarchicalName
            OSL_FAIL( "Content::storeData - getByHierarchicalName failed!" );
            return false;
        }
        catch ( uno::Exception const & )
        {
            // createInstanceWithArguments
            OSL_FAIL( "Content::storeData - Error!" );
            return false;
        }
    }

    if ( !xNA->hasByHierarchicalName( m_aUri.getPath() ) )
        return false;

    try
    {
        uno::Reference< beans::XPropertySet > xPropSet;
        xNA->getByHierarchicalName( m_aUri.getPath() ) >>= xPropSet;

        if ( !xPropSet.is() )
        {
            OSL_FAIL( "Content::storeData - Got no XPropertySet interface!" );
            return false;
        }


        // Store property values...


        if ( m_nModifiedProps & MEDIATYPE_MODIFIED )
        {
            xPropSet->setPropertyValue(
                                u"MediaType"_ustr,
                                uno::Any( m_aProps.aMediaType ) );
            m_nModifiedProps &= ~MEDIATYPE_MODIFIED;
        }

        if ( m_nModifiedProps & COMPRESSED_MODIFIED )
        {
            if ( !isFolder() )
                xPropSet->setPropertyValue(
                                u"Compressed"_ustr,
                                uno::Any( m_aProps.bCompressed ) );

            m_nModifiedProps &= ~COMPRESSED_MODIFIED;
        }

        if ( m_nModifiedProps & ENCRYPTED_MODIFIED )
        {
            if ( !isFolder() )
                xPropSet->setPropertyValue(
                                u"Encrypted"_ustr,
                                uno::Any( m_aProps.bEncrypted ) );

            m_nModifiedProps &= ~ENCRYPTED_MODIFIED;
        }

        if ( m_nModifiedProps & ENCRYPTIONKEY_MODIFIED )
        {
            if ( !isFolder() )
                xPropSet->setPropertyValue(
                            u"EncryptionKey"_ustr,
                            uno::Any( m_aProps.aEncryptionKey ) );

            m_nModifiedProps &= ~ENCRYPTIONKEY_MODIFIED;
        }


        // Store data stream...


        if ( xStream.is() && !isFolder() )
        {
            uno::Reference< io::XActiveDataSink > xSink(
                                                xPropSet, uno::UNO_QUERY );

            if ( !xSink.is() )
            {
                OSL_FAIL( "Content::storeData - "
                            "Got no XActiveDataSink interface!" );
                return false;
            }

            xSink->setInputStream( xStream );
        }

        return true;
    }
    catch ( container::NoSuchElementException const & )
    {
        // getByHierarchicalName
    }
    catch ( beans::UnknownPropertyException const & )
    {
        // setPropertyValue
    }
    catch ( beans::PropertyVetoException const & )
    {
        // setPropertyValue
    }
    catch ( lang::IllegalArgumentException const & )
    {
        // setPropertyValue
    }
    catch ( lang::WrappedTargetException const & )
    {
        // setPropertyValue
    }

    OSL_FAIL( "Content::storeData - Error!" );
    return false;
}


bool Content::removeData()
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    uno::Reference< container::XHierarchicalNameAccess > xNA = getPackage();

    PackageUri aParentUri( getParentURL() );
    if ( !xNA->hasByHierarchicalName( aParentUri.getPath() ) )
        return false;

    try
    {
        uno::Any aEntry = xNA->getByHierarchicalName( aParentUri.getPath() );
        uno::Reference< container::XNameContainer > xContainer;
        aEntry >>= xContainer;

        if ( !xContainer.is() )
        {
            OSL_FAIL( "Content::removeData - "
                        "Got no XNameContainer interface!" );
            return false;
        }

        xContainer->removeByName( m_aUri.getName() );
        return true;
    }
    catch ( container::NoSuchElementException const & )
    {
        // getByHierarchicalName, removeByName
    }
    catch ( lang::WrappedTargetException const & )
    {
        // removeByName
    }

    OSL_FAIL( "Content::removeData - Error!" );
    return false;
}


bool Content::flushData()
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    // Note: XChangesBatch is only implemented by the package itself, not
    //       by the single entries. Maybe this has to change...

    uno::Reference< container::XHierarchicalNameAccess > xNA = getPackage();

    uno::Reference< util::XChangesBatch > xBatch( xNA, uno::UNO_QUERY );
    if ( !xBatch.is() )
    {
        OSL_FAIL( "Content::flushData - Got no XChangesBatch interface!" );
        return false;
    }

    try
    {
        xBatch->commitChanges();
        return true;
    }
    catch ( lang::WrappedTargetException const & )
    {
    }

    OSL_FAIL( "Content::flushData - Error!" );
    return false;
}


uno::Reference< io::XInputStream > Content::getInputStream()
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    uno::Reference< io::XInputStream > xStream;
    uno::Reference< container::XHierarchicalNameAccess > xNA = getPackage();

    if ( !xNA->hasByHierarchicalName( m_aUri.getPath() ) )
        return xStream;

    try
    {
        uno::Any aEntry = xNA->getByHierarchicalName( m_aUri.getPath() );
        uno::Reference< io::XActiveDataSink > xSink;
        aEntry >>= xSink;

        if ( !xSink.is() )
        {
            OSL_FAIL( "Content::getInputStream - "
                        "Got no XActiveDataSink interface!" );
            return xStream;
        }

        xStream = xSink->getInputStream();

        OSL_ENSURE( xStream.is(),
                    "Content::getInputStream - Got no stream!" );
    }
    catch ( container::NoSuchElementException const & )
    {
        // getByHierarchicalName
    }

    return xStream;
}


uno::Reference< container::XEnumeration > Content::getIterator()
{
    osl::Guard< osl::Mutex > aGuard( m_aMutex );

    uno::Reference< container::XEnumeration > xIter;
    uno::Reference< container::XHierarchicalNameAccess > xNA = getPackage();

    if ( !xNA->hasByHierarchicalName( m_aUri.getPath() ) )
        return xIter;

    try
    {
        uno::Any aEntry = xNA->getByHierarchicalName( m_aUri.getPath() );
        uno::Reference< container::XEnumerationAccess > xIterFac;
        aEntry >>= xIterFac;

        if ( !xIterFac.is() )
        {
            OSL_FAIL( "Content::getIterator - "
                        "Got no XEnumerationAccess interface!" );
            return xIter;
        }

        xIter = xIterFac->createEnumeration();

        OSL_ENSURE( xIter.is(),
                    "Content::getIterator - Got no iterator!" );
    }
    catch ( container::NoSuchElementException const & )
    {
        // getByHierarchicalName
    }

    return xIter;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
