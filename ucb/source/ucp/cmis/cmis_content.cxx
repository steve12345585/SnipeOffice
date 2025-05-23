/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <string_view>

#include <boost/make_shared.hpp>

#include <com/sun/star/beans/IllegalTypeException.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySetInfo.hpp>
#include <com/sun/star/document/CmisProperty.hpp>
#include <com/sun/star/io/XActiveDataSink.hpp>
#include <com/sun/star/io/XActiveDataStreamer.hpp>
#include <com/sun/star/lang/IllegalAccessException.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/WrappedTargetRuntimeException.hpp>
#include <com/sun/star/task/InteractionClassification.hpp>
#include <com/sun/star/ucb/ContentInfo.hpp>
#include <com/sun/star/ucb/ContentInfoAttribute.hpp>
#include <com/sun/star/ucb/InsertCommandArgument2.hpp>
#include <com/sun/star/ucb/InteractiveBadTransferURLException.hpp>
#include <com/sun/star/ucb/InteractiveAugmentedIOException.hpp>
#include <com/sun/star/ucb/InteractiveNetworkResolveNameException.hpp>
#include <com/sun/star/ucb/InteractiveNetworkConnectException.hpp>
#include <com/sun/star/ucb/InteractiveNetworkReadException.hpp>
#include <com/sun/star/ucb/MissingInputStreamException.hpp>
#include <com/sun/star/ucb/OpenMode.hpp>
#include <com/sun/star/ucb/UnsupportedCommandException.hpp>
#include <com/sun/star/ucb/UnsupportedDataSinkException.hpp>
#include <com/sun/star/ucb/UnsupportedOpenModeException.hpp>
#include <com/sun/star/ucb/XCommandInfo.hpp>
#include <com/sun/star/ucb/XDynamicResultSet.hpp>

#include <comphelper/processfactory.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/exc_hlp.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <config_oauth2.h>
#include <o3tl/runtimetooustring.hxx>
#include <sal/log.hxx>
#include <tools/urlobj.hxx>
#include <tools/long.hxx>
#include <ucbhelper/cancelcommandexecution.hxx>
#include <ucbhelper/content.hxx>
#include <ucbhelper/contentidentifier.hxx>
#include <ucbhelper/propertyvalueset.hxx>
#include <ucbhelper/proxydecider.hxx>
#include <ucbhelper/macros.hxx>
#include <sax/tools/converter.hxx>
#include <systools/curlinit.hxx>

#include <utility>

#include "auth_provider.hxx"
#include "cmis_content.hxx"
#include "cmis_provider.hxx"
#include "cmis_resultset.hxx"
#include "cmis_strings.hxx"
#include "std_inputstream.hxx"
#include "std_outputstream.hxx"

#define OUSTR_TO_STDSTR(s) std::string( OUStringToOString( s, RTL_TEXTENCODING_UTF8 ) )
#define STD_TO_OUSTR( str ) OStringToOUString( str, RTL_TEXTENCODING_UTF8 )

using namespace com::sun::star;

namespace
{
    util::DateTime lcl_boostToUnoTime(const boost::posix_time::ptime& boostTime)
    {
        util::DateTime unoTime;
        unoTime.Year = boostTime.date().year();
        unoTime.Month = boostTime.date().month();
        unoTime.Day = boostTime.date().day();
        unoTime.Hours = boostTime.time_of_day().hours();
        unoTime.Minutes = boostTime.time_of_day().minutes();
        unoTime.Seconds = boostTime.time_of_day().seconds();

        // TODO FIXME maybe we should compile with BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG
        //            to actually get nanosecond precision in boostTime?
        // use this way rather than total_nanos to avoid overflows with 32-bit long
        const tools::Long ticks = boostTime.time_of_day().fractional_seconds();
        tools::Long nanoSeconds = ticks * ( 1000000000 / boost::posix_time::time_duration::ticks_per_second());

        unoTime.NanoSeconds = nanoSeconds;

        return unoTime;
    }

    uno::Any lcl_cmisPropertyToUno( const libcmis::PropertyPtr& pProperty )
    {
        uno::Any aValue;
        switch ( pProperty->getPropertyType( )->getType( ) )
        {
            default:
            case libcmis::PropertyType::String:
                {
                    auto aCmisStrings = pProperty->getStrings( );
                    uno::Sequence< OUString > aStrings( aCmisStrings.size( ) );
                    OUString* aStringsArr = aStrings.getArray( );
                    sal_Int32 i = 0;
                    for ( const auto& rCmisStr : aCmisStrings )
                    {
                        aStringsArr[i++] = STD_TO_OUSTR( rCmisStr );
                    }
                    aValue <<= aStrings;
                }
                break;
            case libcmis::PropertyType::Integer:
                {
                    auto aCmisLongs = pProperty->getLongs( );
                    uno::Sequence< sal_Int64 > aLongs( aCmisLongs.size( ) );
                    sal_Int64* aLongsArr = aLongs.getArray( );
                    sal_Int32 i = 0;
                    for ( const auto& rCmisLong : aCmisLongs )
                    {
                        aLongsArr[i++] = rCmisLong;
                    }
                    aValue <<= aLongs;
                }
                break;
            case libcmis::PropertyType::Decimal:
                {
                    auto aCmisDoubles = pProperty->getDoubles( );
                    uno::Sequence< double > aDoubles = comphelper::containerToSequence(aCmisDoubles);
                    aValue <<= aDoubles;
                }
                break;
            case libcmis::PropertyType::Bool:
                {
                    auto aCmisBools = pProperty->getBools( );
                    uno::Sequence< sal_Bool > aBools( aCmisBools.size( ) );
                    sal_Bool* aBoolsArr = aBools.getArray( );
                    sal_Int32 i = 0;
                    for ( bool bCmisBool : aCmisBools )
                    {
                        aBoolsArr[i++] = bCmisBool;
                    }
                    aValue <<= aBools;
                }
                break;
            case libcmis::PropertyType::DateTime:
                {
                    auto aCmisTimes = pProperty->getDateTimes( );
                    uno::Sequence< util::DateTime > aTimes( aCmisTimes.size( ) );
                    util::DateTime* aTimesArr = aTimes.getArray( );
                    sal_Int32 i = 0;
                    for ( const auto& rCmisTime : aCmisTimes )
                    {
                        aTimesArr[i++] = lcl_boostToUnoTime( rCmisTime );
                    }
                    aValue <<= aTimes;
                }
                break;
        }
        return aValue;
    }

    libcmis::PropertyPtr lcl_unoToCmisProperty(const document::CmisProperty& prop )
    {
        libcmis::PropertyTypePtr propertyType( new libcmis::PropertyType( ) );

        OUString id = prop.Id;
        OUString name = prop.Name;
        bool bUpdatable = prop.Updatable;
        bool bRequired = prop.Required;
        bool bMultiValued = prop.MultiValued;
        bool bOpenChoice = prop.OpenChoice;
        uno::Any value = prop.Value;
        std::vector< std::string > values;

        libcmis::PropertyType::Type type = libcmis::PropertyType::String;
        if ( prop.Type == CMIS_TYPE_STRING )
        {
            uno::Sequence< OUString > seqValue;
            value >>= seqValue;
            std::transform(std::cbegin(seqValue), std::cend(seqValue), std::back_inserter(values),
                [](const OUString& rValue) -> std::string { return OUSTR_TO_STDSTR( rValue ); });
            type = libcmis::PropertyType::String;
        }
        else if ( prop.Type == CMIS_TYPE_BOOL )
        {
            uno::Sequence< sal_Bool > seqValue;
            value >>= seqValue;
            std::transform(std::cbegin(seqValue), std::cend(seqValue), std::back_inserter(values),
                [](const bool nValue) -> std::string { return std::string( OString::boolean( nValue ) ); });
            type = libcmis::PropertyType::Bool;
        }
        else if ( prop.Type == CMIS_TYPE_INTEGER )
        {
            uno::Sequence< sal_Int64 > seqValue;
            value >>= seqValue;
            std::transform(std::cbegin(seqValue), std::cend(seqValue), std::back_inserter(values),
                [](const sal_Int64 nValue) -> std::string { return std::string( OString::number( nValue ) ); });
            type = libcmis::PropertyType::Integer;
        }
        else if ( prop.Type == CMIS_TYPE_DECIMAL )
        {
            uno::Sequence< double > seqValue;
            value >>= seqValue;
            std::transform(std::cbegin(seqValue), std::cend(seqValue), std::back_inserter(values),
                [](const double fValue) -> std::string { return std::string( OString::number( fValue ) ); });
            type = libcmis::PropertyType::Decimal;
        }
        else if ( prop.Type == CMIS_TYPE_DATETIME )
        {
            uno::Sequence< util::DateTime > seqValue;
            value >>= seqValue;
            std::transform(std::cbegin(seqValue), std::cend(seqValue), std::back_inserter(values),
                [](const util::DateTime& rValue) -> std::string {
                    OUStringBuffer aBuffer;
                    ::sax::Converter::convertDateTime( aBuffer, rValue, nullptr );
                    return OUSTR_TO_STDSTR( aBuffer );
                });
            type = libcmis::PropertyType::DateTime;
        }

        propertyType->setId( OUSTR_TO_STDSTR( id ));
        propertyType->setDisplayName( OUSTR_TO_STDSTR( name ) );
        propertyType->setUpdatable( bUpdatable );
        propertyType->setRequired( bRequired );
        propertyType->setMultiValued( bMultiValued );
        propertyType->setOpenChoice( bOpenChoice );
        propertyType->setType( type );

        libcmis::PropertyPtr property( new libcmis::Property( std::move(propertyType),
                                                              std::move(values) ) );

        return property;
    }

    uno::Sequence< uno::Any > generateErrorArguments( const cmis::URL & rURL )
    {
        uno::Sequence< uno::Any > aArguments{ uno::Any(beans::PropertyValue(
                                                           u"Binding URL"_ustr,
                                                           - 1,
                                                           uno::Any( rURL.getBindingUrl() ),
                                                           beans::PropertyState_DIRECT_VALUE )),
                                              uno::Any(beans::PropertyValue(
                                                           u"Username"_ustr,
                                                           -1,
                                                           uno::Any( rURL.getUsername() ),
                                                           beans::PropertyState_DIRECT_VALUE )),
                                              uno::Any(beans::PropertyValue(
                                                           u"Repository Id"_ustr,
                                                           -1,
                                                           uno::Any( rURL.getRepositoryId() ),
                                                           beans::PropertyState_DIRECT_VALUE )) };

        return aArguments;
    }
}

namespace cmis
{
    Content::Content( const uno::Reference< uno::XComponentContext >& rxContext,
        ContentProvider *pProvider, const uno::Reference< ucb::XContentIdentifier >& Identifier,
        libcmis::ObjectPtr pObject )
        : ContentImplHelper( rxContext, pProvider, Identifier ),
        m_pProvider( pProvider ),
        m_pSession( nullptr ),
        m_pObject(std::move( pObject )),
        m_sURL( Identifier->getContentIdentifier( ) ),
        m_aURL( m_sURL ),
        m_bTransient( false ),
        m_bIsFolder( false )
    {
        SAL_INFO( "ucb.ucp.cmis", "Content::Content() " << m_sURL );

        m_sObjectPath = m_aURL.getObjectPath( );
        m_sObjectId = m_aURL.getObjectId( );
    }

    Content::Content( const uno::Reference< uno::XComponentContext >& rxContext, ContentProvider *pProvider,
        const uno::Reference< ucb::XContentIdentifier >& Identifier,
        bool bIsFolder )
        : ContentImplHelper( rxContext, pProvider, Identifier ),
        m_pProvider( pProvider ),
        m_pSession( nullptr ),
        m_sURL( Identifier->getContentIdentifier( ) ),
        m_aURL( m_sURL ),
        m_bTransient( true ),
        m_bIsFolder( bIsFolder )
    {
        SAL_INFO( "ucb.ucp.cmis", "Content::Content() " << m_sURL );

        m_sObjectPath = m_aURL.getObjectPath( );
        m_sObjectId = m_aURL.getObjectId( );
    }

    Content::~Content()
    {
    }

    libcmis::Session* Content::getSession( const uno::Reference< ucb::XCommandEnvironment >& xEnv )
    {
        // Set the proxy if needed. We are doing that all times as the proxy data shouldn't be cached.
        ucbhelper::InternetProxyDecider aProxyDecider( m_xContext );
        INetURLObject aBindingUrl( m_aURL.getBindingUrl( ) );
        const OUString sProxy = aProxyDecider.getProxy(
                INetURLObject::GetScheme( aBindingUrl.GetProtocol( ) ), aBindingUrl.GetHost(), aBindingUrl.GetPort() );
        libcmis::SessionFactory::setProxySettings( OUSTR_TO_STDSTR( sProxy ), std::string(), std::string(), std::string() );

        // Look for a cached session, key is binding url + repo id
        OUString sSessionId = m_aURL.getBindingUrl( ) + m_aURL.getRepositoryId( );
        if ( nullptr == m_pSession )
            m_pSession = m_pProvider->getSession( sSessionId, m_aURL.getUsername( ) );

        if ( nullptr == m_pSession )
        {
            // init libcurl callback
            libcmis::SessionFactory::setCurlInitProtocolsFunction(&::InitCurl_easy);

            // Get the auth credentials
            AuthProvider aAuthProvider(xEnv, m_xIdentifier->getContentIdentifier(), m_aURL.getBindingUrl());
            AuthProvider::setXEnv( xEnv );

            auto rUsername = OUSTR_TO_STDSTR( m_aURL.getUsername( ) );
            auto rPassword = OUSTR_TO_STDSTR( m_aURL.getPassword( ) );

            bool bSkipInitialPWAuth = false;
            if (m_aURL.getBindingUrl() == ONEDRIVE_BASE_URL
                || m_aURL.getBindingUrl() == GDRIVE_BASE_URL)
            {
                // skip the initial username and pw-auth prompt, the only supported method is the
                // auth-code-fallback one (login with your browser, copy code into the dialog)
                // TODO: if LO were to listen on localhost for the request, it would be much nicer
                // user experience
                bSkipInitialPWAuth = true;
                rPassword = aAuthProvider.getRefreshToken(rUsername);
            }

            bool bIsDone = false;

            while ( !bIsDone )
            {
                if (bSkipInitialPWAuth || aAuthProvider.authenticationQuery(rUsername, rPassword))
                {
                    // Initiate a CMIS session and register it as we found nothing
                    libcmis::OAuth2DataPtr oauth2Data;
                    if ( m_aURL.getBindingUrl( ) == GDRIVE_BASE_URL )
                    {
                        // reset the skip, so user gets a chance to cancel
                        bSkipInitialPWAuth = false;
                        libcmis::SessionFactory::setOAuth2AuthCodeProvider(AuthProvider::copyWebAuthCodeFallback);
                        oauth2Data = boost::make_shared<libcmis::OAuth2Data>(
                            GDRIVE_AUTH_URL, GDRIVE_TOKEN_URL,
                            GDRIVE_SCOPE, GDRIVE_REDIRECT_URI,
                            GDRIVE_CLIENT_ID, GDRIVE_CLIENT_SECRET );
                    }
                    if ( m_aURL.getBindingUrl().startsWith( ALFRESCO_CLOUD_BASE_URL ) )
                        oauth2Data = boost::make_shared<libcmis::OAuth2Data>(
                            ALFRESCO_CLOUD_AUTH_URL, ALFRESCO_CLOUD_TOKEN_URL,
                            ALFRESCO_CLOUD_SCOPE, ALFRESCO_CLOUD_REDIRECT_URI,
                            ALFRESCO_CLOUD_CLIENT_ID, ALFRESCO_CLOUD_CLIENT_SECRET );
                    if ( m_aURL.getBindingUrl( ) == ONEDRIVE_BASE_URL )
                    {
                        // reset the skip, so user gets a chance to cancel
                        bSkipInitialPWAuth = false;
                        libcmis::SessionFactory::setOAuth2AuthCodeProvider(AuthProvider::copyWebAuthCodeFallback);
                        oauth2Data = boost::make_shared<libcmis::OAuth2Data>(
                            ONEDRIVE_AUTH_URL, ONEDRIVE_TOKEN_URL,
                            ONEDRIVE_SCOPE, ONEDRIVE_REDIRECT_URI,
                            ONEDRIVE_CLIENT_ID, ONEDRIVE_CLIENT_SECRET );
                    }
                    try
                    {
                        m_pSession = libcmis::SessionFactory::createSession(
                            OUSTR_TO_STDSTR( m_aURL.getBindingUrl( ) ),
                            rUsername, rPassword, OUSTR_TO_STDSTR( m_aURL.getRepositoryId( ) ), false, std::move(oauth2Data) );

                        if ( m_pSession == nullptr )
                        {
                            // Fail: session was not created
                            ucbhelper::cancelCommandExecution(
                                ucb::IOErrorCode_INVALID_DEVICE,
                                generateErrorArguments(m_aURL),
                                xEnv);
                        }
                        else if ( m_pSession->getRepository() == nullptr )
                        {
                            // Fail: no repository or repository is invalid
                            ucbhelper::cancelCommandExecution(
                                ucb::IOErrorCode_INVALID_DEVICE,
                                generateErrorArguments(m_aURL),
                                xEnv,
                                u"error accessing a repository"_ustr);
                        }
                        else
                        {
                            m_pProvider->registerSession(sSessionId, m_aURL.getUsername( ), m_pSession);
                            if (m_aURL.getBindingUrl() == ONEDRIVE_BASE_URL
                                || m_aURL.getBindingUrl() == GDRIVE_BASE_URL)
                            {
                                aAuthProvider.storeRefreshToken(rUsername, rPassword,
                                                                m_pSession->getRefreshToken());
                            }
                        }

                        bIsDone = true;
                    }
                    catch( const libcmis::Exception & e )
                    {
                        if (e.getType() == "dnsFailed")
                        {
                            uno::Any ex;
                            ex <<= ucb::InteractiveNetworkResolveNameException(
                                    OStringToOUString(e.what(), RTL_TEXTENCODING_UTF8),
                                    getXWeak(),
                                    task::InteractionClassification_ERROR,
                                    m_aURL.getBindingUrl());
                            ucbhelper::cancelCommandExecution(ex, xEnv);
                        }
                        else if (e.getType() == "connectFailed" || e.getType() == "connectTimeout")
                        {
                            uno::Any ex;
                            ex <<= ucb::InteractiveNetworkConnectException(
                                    OStringToOUString(e.what(), RTL_TEXTENCODING_UTF8),
                                    getXWeak(),
                                    task::InteractionClassification_ERROR,
                                    m_aURL.getBindingUrl());
                            ucbhelper::cancelCommandExecution(ex, xEnv);
                        }
                        else if (e.getType() == "transferFailed")
                        {
                            uno::Any ex;
                            ex <<= ucb::InteractiveNetworkReadException(
                                    OStringToOUString(e.what(), RTL_TEXTENCODING_UTF8),
                                    getXWeak(),
                                    task::InteractionClassification_ERROR,
                                    m_aURL.getBindingUrl());
                            ucbhelper::cancelCommandExecution(ex, xEnv);
                        }
                        else if (e.getType() != "permissionDenied")
                        {
                            SAL_INFO("ucb.ucp.cmis", "Unexpected libcmis exception: " << e.what());
                            throw;
                        }
                    }
                }
                else
                {
                    // Silently fail as the user cancelled the authentication
                    ucbhelper::cancelCommandExecution(
                                        ucb::IOErrorCode_ABORT,
                                        uno::Sequence< uno::Any >( 0 ),
                                        xEnv );
                    throw uno::RuntimeException( );
                }
            }
        }
        return m_pSession;
    }

    libcmis::ObjectTypePtr const & Content::getObjectType( const uno::Reference< ucb::XCommandEnvironment >& xEnv )
    {
        if ( nullptr == m_pObjectType.get( ) && m_bTransient )
        {
            const std::string typeId = m_bIsFolder ? "cmis:folder" : "cmis:document";
            // The type to create needs to be fetched from the possible children types
            // defined in the parent folder. Then, we'll pick up the first one we find matching
            // cmis:folder or cmis:document (depending what we need to create).
            // The easy case will work in most cases, but not on some servers (like Lotus Live)
            libcmis::Folder* pParent = nullptr;
            bool bTypeRestricted = false;
            try
            {
                pParent = dynamic_cast< libcmis::Folder* >( getObject( xEnv ).get( ) );
            }
            catch ( const libcmis::Exception& )
            {
            }

            if ( pParent )
            {
                std::map< std::string, libcmis::PropertyPtr >& aProperties = pParent->getProperties( );
                std::map< std::string, libcmis::PropertyPtr >::iterator it = aProperties.find( "cmis:allowedChildObjectTypeIds" );
                if ( it != aProperties.end( ) )
                {
                    libcmis::PropertyPtr pProperty = it->second;
                    if ( pProperty )
                    {
                        std::vector< std::string > typesIds = pProperty->getStrings( );
                        for ( const auto& rType : typesIds )
                        {
                            bTypeRestricted = true;
                            libcmis::ObjectTypePtr type = getSession( xEnv )->getType( rType );

                            // FIXME Improve performances by adding getBaseTypeId( ) method to libcmis
                            if ( type->getBaseType( )->getId( ) == typeId )
                            {
                                m_pObjectType = std::move(type);
                                break;
                            }
                        }
                    }
                }
            }

            if ( !bTypeRestricted )
                m_pObjectType = getSession( xEnv )->getType( typeId );
        }
        return m_pObjectType;
    }


    libcmis::ObjectPtr const & Content::getObject( const uno::Reference< ucb::XCommandEnvironment >& xEnv )
    {
        // can't get the session for some reason
        // the recent file opening at start up is an example.
        try
        {
            if ( !getSession( xEnv ) )
                return m_pObject;
        }
        catch ( uno::RuntimeException& )
        {
            return m_pObject;
        }
        if ( !m_pObject.get() )
        {
            if ( !m_sObjectId.isEmpty( ) )
            {
                try
                {
                    m_pObject = getSession( xEnv )->getObject( OUSTR_TO_STDSTR( m_sObjectId ) );
                }
                catch ( const libcmis::Exception& )
                {
                    SAL_INFO( "ucb.ucp.cmis", "object: " << OUSTR_TO_STDSTR(m_sObjectId));
                    throw libcmis::Exception( "Object not found" );
                }
            }
            else if (!(m_sObjectPath.isEmpty() || m_sObjectPath == "/"))
            {
                try
                {
                    m_pObject = getSession( xEnv )->getObjectByPath( OUSTR_TO_STDSTR( m_sObjectPath ) );
                }
                catch ( const libcmis::Exception& )
                {
                    // In some cases, getting the object from the path doesn't work,
                    // but getting the parent from its path and the get the child in the list is OK.
                    // It's weird, but needed to handle case where the path isn't the folders/files
                    // names separated by '/' (as in Lotus Live)
                    INetURLObject aParentUrl( m_sURL );
                    std::string sName = OUSTR_TO_STDSTR( aParentUrl.getName( INetURLObject::LAST_SEGMENT, true, INetURLObject::DecodeMechanism::WithCharset ) );
                    aParentUrl.removeSegment( );
                    OUString sParentUrl = aParentUrl.GetMainURL( INetURLObject::DecodeMechanism::NONE );
                    // Avoid infinite recursion if sParentUrl == m_sURL
                    if (sParentUrl != m_sURL)
                    {
                        rtl::Reference<Content> xParent(new Content(m_xContext, m_pProvider, new ucbhelper::ContentIdentifier(sParentUrl)));
                        libcmis::FolderPtr pParentFolder = boost::dynamic_pointer_cast< libcmis::Folder >(xParent->getObject(xEnv));
                        if (pParentFolder)
                        {
                            std::vector< libcmis::ObjectPtr > children = pParentFolder->getChildren();
                            auto it = std::find_if(children.begin(), children.end(),
                                [&sName](const libcmis::ObjectPtr& rChild) { return rChild->getName() == sName; });
                            if (it != children.end())
                                m_pObject = *it;
                        }
                    }

                    if ( !m_pObject )
                        throw libcmis::Exception( "Object not found" );
                }
            }
            else
            {
                m_pObject = getSession( xEnv )->getRootFolder( );
                m_sObjectPath = "/";
                m_sObjectId = OUString( );
            }
        }

        return m_pObject;
    }

    bool Content::isFolder(const uno::Reference< ucb::XCommandEnvironment >& xEnv )
    {
        bool bIsFolder = false;
        try
        {
            libcmis::ObjectPtr obj = getObject( xEnv );
            if ( obj )
                bIsFolder = obj->getBaseType( ) == "cmis:folder";
        }
        catch ( const libcmis::Exception& e )
        {
            SAL_INFO( "ucb.ucp.cmis", "Unexpected libcmis exception: " << e.what( ) );

            ucbhelper::cancelCommandExecution(
                            ucb::IOErrorCode_GENERAL,
                            uno::Sequence< uno::Any >( 0 ),
                            xEnv,
                            OUString::createFromAscii( e.what( ) ) );

        }
        return bIsFolder;
    }

    uno::Any Content::getBadArgExcept()
    {
        return uno::Any( lang::IllegalArgumentException(
            u"Wrong argument type!"_ustr,
            getXWeak(), -1) );
    }

    libcmis::ObjectPtr Content::updateProperties(
         const uno::Any& iCmisProps,
         const uno::Reference< ucb::XCommandEnvironment >& xEnv )
    {
        // Convert iCmisProps to Cmis Properties;
        uno::Sequence< document::CmisProperty > aPropsSeq;
        iCmisProps >>= aPropsSeq;
        std::map< std::string, libcmis::PropertyPtr > aProperties;

        for (const auto& rProp : aPropsSeq)
        {
            std::string id = OUSTR_TO_STDSTR( rProp.Id );
            libcmis::PropertyPtr prop = lcl_unoToCmisProperty( rProp );
            aProperties.insert( std::pair<std::string, libcmis::PropertyPtr>( id, prop ) );
        }
        libcmis::ObjectPtr updateObj;
        try
        {
            updateObj = getObject( xEnv )->updateProperties( aProperties );
        }
        catch ( const libcmis::Exception& e )
        {
            SAL_INFO( "ucb.ucp.cmis", "Unexpected libcmis exception: "<< e.what( ) );
        }

        return updateObj;
    }

    uno::Reference< sdbc::XRow > Content::getPropertyValues(
            const uno::Sequence< beans::Property >& rProperties,
            const uno::Reference< ucb::XCommandEnvironment >& xEnv )
    {
        rtl::Reference< ::ucbhelper::PropertyValueSet > xRow = new ::ucbhelper::PropertyValueSet( m_xContext );

        for( const beans::Property& rProp : rProperties )
        {
            try
            {
                if ( rProp.Name == "IsDocument" )
                {
                    try
                    {
                        libcmis::ObjectPtr obj = getObject( xEnv );
                        if ( obj )
                            xRow->appendBoolean( rProp, obj->getBaseType( ) == "cmis:document" );
                    }
                    catch ( const libcmis::Exception& )
                    {
                        if ( m_pObjectType.get( ) )
                            xRow->appendBoolean( rProp, getObjectType( xEnv )->getBaseType()->getId( ) == "cmis:document" );
                        else
                            xRow->appendVoid( rProp );
                    }
                }
                else if ( rProp.Name == "IsFolder" )
                {
                    try
                    {
                        libcmis::ObjectPtr obj = getObject( xEnv );
                        if ( obj )
                            xRow->appendBoolean( rProp, obj->getBaseType( ) == "cmis:folder" );
                        else
                            xRow->appendBoolean( rProp, false );
                    }
                    catch ( const libcmis::Exception& )
                    {
                        if ( m_pObjectType.get( ) )
                            xRow->appendBoolean( rProp, getObjectType( xEnv )->getBaseType()->getId( ) == "cmis:folder" );
                        else
                            xRow->appendVoid( rProp );
                    }
                }
                else if ( rProp.Name == "Title" )
                {
                    OUString sTitle;
                    try
                    {
                        sTitle = STD_TO_OUSTR( getObject( xEnv )->getName() );
                    }
                    catch ( const libcmis::Exception& )
                    {
                        if ( !m_pObjectProps.empty() )
                        {
                            std::map< std::string, libcmis::PropertyPtr >::iterator it = m_pObjectProps.find( "cmis:name" );
                            if ( it != m_pObjectProps.end( ) )
                            {
                                std::vector< std::string > values = it->second->getStrings( );
                                if ( !values.empty() )
                                    sTitle = STD_TO_OUSTR( values.front( ) );
                            }
                        }
                    }

                    // Nothing worked... get it from the path
                    if ( sTitle.isEmpty( ) )
                    {
                        OUString sPath = m_sObjectPath;

                        // Get rid of the trailing slash problem
                        if ( sPath.endsWith("/") )
                            sPath = sPath.copy( 0, sPath.getLength() - 1 );

                        // Get the last segment
                        sal_Int32 nPos = sPath.lastIndexOf( '/' );
                        if ( nPos >= 0 )
                            sTitle = sPath.copy( nPos + 1 );
                    }

                    if ( !sTitle.isEmpty( ) )
                        xRow->appendString( rProp, sTitle );
                    else
                        xRow->appendVoid( rProp );
                }
                else if ( rProp.Name == "ObjectId" )
                {
                    OUString sId;
                    try
                    {
                        sId = STD_TO_OUSTR( getObject( xEnv )->getId() );
                    }
                    catch ( const libcmis::Exception& )
                    {
                        if ( !m_pObjectProps.empty() )
                        {
                            std::map< std::string, libcmis::PropertyPtr >::iterator it = m_pObjectProps.find( "cmis:objectId" );
                            if ( it != m_pObjectProps.end( ) )
                            {
                                std::vector< std::string > values = it->second->getStrings( );
                                if ( !values.empty() )
                                    sId = STD_TO_OUSTR( values.front( ) );
                            }
                        }
                    }

                    if ( !sId.isEmpty( ) )
                        xRow->appendString( rProp, sId );
                    else
                        xRow->appendVoid( rProp );
                }
                else if ( rProp.Name == "TitleOnServer" )
                {
                    xRow->appendString( rProp, m_sObjectPath);
                }
                else if ( rProp.Name == "IsReadOnly" )
                {
                    boost::shared_ptr< libcmis::AllowableActions > allowableActions = getObject( xEnv )->getAllowableActions( );
                    bool bReadOnly = false;
                    if ( !allowableActions->isAllowed( libcmis::ObjectAction::SetContentStream ) &&
                         !allowableActions->isAllowed( libcmis::ObjectAction::CheckIn ) )
                        bReadOnly = true;

                    xRow->appendBoolean( rProp, bReadOnly );
                }
                else if ( rProp.Name == "DateCreated" )
                {
                    util::DateTime aTime = lcl_boostToUnoTime( getObject( xEnv )->getCreationDate( ) );
                    xRow->appendTimestamp( rProp, aTime );
                }
                else if ( rProp.Name == "DateModified" )
                {
                    util::DateTime aTime = lcl_boostToUnoTime( getObject( xEnv )->getLastModificationDate( ) );
                    xRow->appendTimestamp( rProp, aTime );
                }
                else if ( rProp.Name == "Size" )
                {
                    try
                    {
                        libcmis::Document* document = dynamic_cast< libcmis::Document* >( getObject( xEnv ).get( ) );
                        if ( nullptr != document )
                            xRow->appendLong( rProp, document->getContentLength() );
                        else
                            xRow->appendVoid( rProp );
                    }
                    catch ( const libcmis::Exception& )
                    {
                        xRow->appendVoid( rProp );
                    }
                }
                else if ( rProp.Name == "CreatableContentsInfo" )
                {
                    xRow->appendObject( rProp, uno::Any( queryCreatableContentsInfo( xEnv ) ) );
                }
                else if ( rProp.Name == "MediaType" )
                {
                    try
                    {
                        libcmis::Document* document = dynamic_cast< libcmis::Document* >( getObject( xEnv ).get( ) );
                        if ( nullptr != document )
                            xRow->appendString( rProp, STD_TO_OUSTR( document->getContentType() ) );
                        else
                            xRow->appendVoid( rProp );
                    }
                    catch ( const libcmis::Exception& )
                    {
                        xRow->appendVoid( rProp );
                    }
                }
                else if ( rProp.Name == "IsVolume" )
                {
                    xRow->appendBoolean( rProp, false );
                }
                else if ( rProp.Name == "IsRemote" )
                {
                    xRow->appendBoolean( rProp, false );
                }
                else if ( rProp.Name == "IsRemoveable" )
                {
                    xRow->appendBoolean( rProp, false );
                }
                else if ( rProp.Name == "IsFloppy" )
                {
                    xRow->appendBoolean( rProp, false );
                }
                else if ( rProp.Name == "IsCompactDisc" )
                {
                    xRow->appendBoolean( rProp, false );
                }
                else if ( rProp.Name == "IsHidden" )
                {
                    xRow->appendBoolean( rProp, false );
                }
                else if ( rProp.Name == "TargetURL" )
                {
                    xRow->appendString( rProp, u""_ustr );
                }
                else if ( rProp.Name == "BaseURI" )
                {
                    xRow->appendString( rProp, m_aURL.getBindingUrl( ) );
                }
                else if ( rProp.Name == "CmisProperties" )
                {
                    try
                    {
                        libcmis::ObjectPtr object = getObject( xEnv );
                        std::map< std::string, libcmis::PropertyPtr >& aProperties = object->getProperties( );
                        uno::Sequence< document::CmisProperty > aCmisProperties( aProperties.size( ) );
                        document::CmisProperty* pCmisProps = aCmisProperties.getArray( );
                        sal_Int32 i = 0;
                        for ( const auto& [sId, rProperty] : aProperties )
                        {
                            auto sDisplayName = rProperty->getPropertyType()->getDisplayName( );
                            bool bUpdatable = rProperty->getPropertyType()->isUpdatable( );
                            bool bRequired = rProperty->getPropertyType()->isRequired( );
                            bool bMultiValued = rProperty->getPropertyType()->isMultiValued();
                            bool bOpenChoice = rProperty->getPropertyType()->isOpenChoice();

                            pCmisProps[i].Id = STD_TO_OUSTR( sId );
                            pCmisProps[i].Name = STD_TO_OUSTR( sDisplayName );
                            pCmisProps[i].Updatable = bUpdatable;
                            pCmisProps[i].Required = bRequired;
                            pCmisProps[i].MultiValued = bMultiValued;
                            pCmisProps[i].OpenChoice = bOpenChoice;
                            pCmisProps[i].Value = lcl_cmisPropertyToUno( rProperty );
                            switch ( rProperty->getPropertyType( )->getType( ) )
                            {
                                default:
                                case libcmis::PropertyType::String:
                                    pCmisProps[i].Type = CMIS_TYPE_STRING;
                                break;
                                case libcmis::PropertyType::Integer:
                                    pCmisProps[i].Type = CMIS_TYPE_INTEGER;
                                break;
                                case libcmis::PropertyType::Decimal:
                                    pCmisProps[i].Type = CMIS_TYPE_DECIMAL;
                                break;
                                case libcmis::PropertyType::Bool:
                                    pCmisProps[i].Type = CMIS_TYPE_BOOL;
                                break;
                                case libcmis::PropertyType::DateTime:
                                    pCmisProps[i].Type = CMIS_TYPE_DATETIME;
                                break;
                            }
                            ++i;
                        }
                        xRow->appendObject( rProp.Name, uno::Any( aCmisProperties ) );
                    }
                    catch ( const libcmis::Exception& )
                    {
                        xRow->appendVoid( rProp );
                    }
                }
                else if ( rProp.Name == "IsVersionable" )
                {
                    try
                    {
                        libcmis::ObjectPtr object = getObject( xEnv );
                        bool bIsVersionable = object->getTypeDescription( )->isVersionable( );
                        xRow->appendBoolean( rProp, bIsVersionable );
                    }
                    catch ( const libcmis::Exception& )
                    {
                        xRow->appendVoid( rProp );
                    }
                }
                else if ( rProp.Name == "CanCheckOut" )
                {
                    try
                    {
                        libcmis::ObjectPtr pObject = getObject( xEnv );
                        libcmis::AllowableActionsPtr aAllowables = pObject->getAllowableActions( );
                        bool bAllowed = false;
                        if ( aAllowables )
                        {
                            bAllowed = aAllowables->isAllowed( libcmis::ObjectAction::CheckOut );
                        }
                        xRow->appendBoolean( rProp, bAllowed );
                    }
                    catch ( const libcmis::Exception& )
                    {
                        xRow->appendVoid( rProp );
                    }
                }
                else if ( rProp.Name == "CanCancelCheckOut" )
                {
                    try
                    {
                        libcmis::ObjectPtr pObject = getObject( xEnv );
                        libcmis::AllowableActionsPtr aAllowables = pObject->getAllowableActions( );
                        bool bAllowed = false;
                        if ( aAllowables )
                        {
                            bAllowed = aAllowables->isAllowed( libcmis::ObjectAction::CancelCheckOut );
                        }
                        xRow->appendBoolean( rProp, bAllowed );
                    }
                    catch ( const libcmis::Exception& )
                    {
                        xRow->appendVoid( rProp );
                    }
                }
                else if ( rProp.Name == "CanCheckIn" )
                {
                    try
                    {
                        libcmis::ObjectPtr pObject = getObject( xEnv );
                        libcmis::AllowableActionsPtr aAllowables = pObject->getAllowableActions( );
                        bool bAllowed = false;
                        if ( aAllowables )
                        {
                            bAllowed = aAllowables->isAllowed( libcmis::ObjectAction::CheckIn );
                        }
                        xRow->appendBoolean( rProp, bAllowed );
                    }
                    catch ( const libcmis::Exception& )
                    {
                        xRow->appendVoid( rProp );
                    }
                }
                else
                    SAL_INFO( "ucb.ucp.cmis", "Looking for unsupported property " << rProp.Name );
            }
            catch (const libcmis::Exception&)
            {
                xRow->appendVoid( rProp );
            }
        }

        return xRow;
    }

    uno::Any Content::open(const ucb::OpenCommandArgument2 & rOpenCommand,
        const uno::Reference< ucb::XCommandEnvironment > & xEnv )
    {
        bool bIsFolder = isFolder( xEnv );

        // Handle the case of the non-existing file
        if ( !getObject( xEnv ) )
        {
            uno::Sequence< uno::Any > aArgs{ uno::Any(m_xIdentifier->getContentIdentifier()) };
            uno::Any aErr(
                ucb::InteractiveAugmentedIOException(OUString(), getXWeak(),
                    task::InteractionClassification_ERROR,
                    bIsFolder ? ucb::IOErrorCode_NOT_EXISTING_PATH : ucb::IOErrorCode_NOT_EXISTING, aArgs)
            );

            ucbhelper::cancelCommandExecution(aErr, xEnv);
        }

        uno::Any aRet;

        bool bOpenFolder = (
            ( rOpenCommand.Mode == ucb::OpenMode::ALL ) ||
            ( rOpenCommand.Mode == ucb::OpenMode::FOLDERS ) ||
            ( rOpenCommand.Mode == ucb::OpenMode::DOCUMENTS )
         );

        if ( bOpenFolder && bIsFolder )
        {
            uno::Reference< ucb::XDynamicResultSet > xSet
                = new DynamicResultSet(m_xContext, this, rOpenCommand, xEnv );
            aRet <<= xSet;
        }
        else if ( rOpenCommand.Sink.is() )
        {
            if (
                ( rOpenCommand.Mode == ucb::OpenMode::DOCUMENT_SHARE_DENY_NONE ) ||
                ( rOpenCommand.Mode == ucb::OpenMode::DOCUMENT_SHARE_DENY_WRITE )
               )
            {
                ucbhelper::cancelCommandExecution(
                    uno::Any ( ucb::UnsupportedOpenModeException
                        ( OUString(), getXWeak(),
                          sal_Int16( rOpenCommand.Mode ) ) ),
                        xEnv );
            }

            if ( !feedSink( rOpenCommand.Sink, xEnv ) )
            {
                // Note: rOpenCommand.Sink may contain an XStream
                //       implementation. Support for this type of
                //       sink is optional...
                SAL_INFO( "ucb.ucp.cmis", "Failed to copy data to sink" );

                ucbhelper::cancelCommandExecution(
                    uno::Any (ucb::UnsupportedDataSinkException
                        ( OUString(), getXWeak(),
                          rOpenCommand.Sink ) ),
                        xEnv );
            }
        }
        else
            SAL_INFO( "ucb.ucp.cmis", "Open falling through ..." );

        return aRet;
    }

    OUString Content::checkIn( const ucb::CheckinArgument& rArg,
        const uno::Reference< ucb::XCommandEnvironment > & xEnv )
    {
        ucbhelper::Content aSourceContent( rArg.SourceURL, xEnv, comphelper::getProcessComponentContext( ) );
        uno::Reference< io::XInputStream > xIn = aSourceContent.openStream( );

        libcmis::ObjectPtr object;
        try
        {
            object = getObject( xEnv );
        }
        catch ( const libcmis::Exception& e )
        {
            SAL_INFO( "ucb.ucp.cmis", "Unexpected libcmis exception: " << e.what( ) );
            ucbhelper::cancelCommandExecution(
                                ucb::IOErrorCode_GENERAL,
                                uno::Sequence< uno::Any >( 0 ),
                                xEnv,
                                OUString::createFromAscii( e.what() ) );
        }

        libcmis::Document* pPwc = dynamic_cast< libcmis::Document* >( object.get( ) );
        if ( !pPwc )
        {
            ucbhelper::cancelCommandExecution(
                                ucb::IOErrorCode_GENERAL,
                                uno::Sequence< uno::Any >( 0 ),
                                xEnv,
                                u"Checkin only supported by documents"_ustr );
        }

        boost::shared_ptr< std::ostream > pOut( new std::ostringstream ( std::ios_base::binary | std::ios_base::in | std::ios_base::out ) );
        uno::Reference < io::XOutputStream > xOutput = new StdOutputStream( pOut );
        copyData( xIn, xOutput );

        std::map< std::string, libcmis::PropertyPtr > newProperties;
        libcmis::DocumentPtr pDoc;

        try
        {
            pDoc = pPwc->checkIn( rArg.MajorVersion, OUSTR_TO_STDSTR( rArg.VersionComment ), newProperties,
                                  std::move(pOut), OUSTR_TO_STDSTR( rArg.MimeType ), OUSTR_TO_STDSTR( rArg.NewTitle ) );
        }
        catch ( const libcmis::Exception& e )
        {
            SAL_INFO( "ucb.ucp.cmis", "Unexpected libcmis exception: " << e.what( ) );
            ucbhelper::cancelCommandExecution(
                                ucb::IOErrorCode_GENERAL,
                                uno::Sequence< uno::Any >( 0 ),
                                xEnv,
                                OUString::createFromAscii( e.what() ) );
        }

        // Get the URL and send it back as a result
        URL aCmisUrl( m_sURL );
        std::vector< std::string > aPaths = pDoc->getPaths( );
        if ( !aPaths.empty() )
        {
            aCmisUrl.setObjectPath(STD_TO_OUSTR(aPaths.front()));
        }
        else
        {
            // We may have unfiled document depending on the server, those
            // won't have any path, use their ID instead
            aCmisUrl.setObjectId(STD_TO_OUSTR(pDoc->getId()));
        }
        return aCmisUrl.asString( );
    }

    OUString Content::checkOut( const uno::Reference< ucb::XCommandEnvironment > & xEnv )
    {
        OUString aRet;
        try
        {
            // Checkout the document if possible
            libcmis::DocumentPtr pDoc = boost::dynamic_pointer_cast< libcmis::Document >( getObject( xEnv ) );
            if ( pDoc.get( ) == nullptr )
            {
                ucbhelper::cancelCommandExecution(
                                    ucb::IOErrorCode_GENERAL,
                                    uno::Sequence< uno::Any >( 0 ),
                                    xEnv,
                                    u"Checkout only supported by documents"_ustr );
            }
            libcmis::DocumentPtr pPwc = pDoc->checkOut( );

            // Compute the URL of the Private Working Copy (PWC)
            URL aCmisUrl( m_sURL );
            std::vector< std::string > aPaths = pPwc->getPaths( );
            if ( !aPaths.empty() )
            {
                aCmisUrl.setObjectPath(STD_TO_OUSTR(aPaths.front()));
            }
            else
            {
                // We may have unfiled PWC depending on the server, those
                // won't have any path, use their ID instead
                auto sId = pPwc->getId( );
                aCmisUrl.setObjectId( STD_TO_OUSTR( sId ) );
            }
            aRet = aCmisUrl.asString( );
        }
        catch ( const libcmis::Exception& e )
        {
            SAL_INFO( "ucb.ucp.cmis", "Unexpected libcmis exception: " << e.what( ) );
            ucbhelper::cancelCommandExecution(
                                ucb::IOErrorCode_GENERAL,
                                uno::Sequence< uno::Any >( 0 ),
                                xEnv,
                                o3tl::runtimeToOUString(e.what()));
        }
        return aRet;
    }

    OUString Content::cancelCheckOut( const uno::Reference< ucb::XCommandEnvironment > & xEnv )
    {
        OUString aRet;
        try
        {
            libcmis::DocumentPtr pPwc = boost::dynamic_pointer_cast< libcmis::Document >( getObject( xEnv ) );
            if ( pPwc.get( ) == nullptr )
            {
                ucbhelper::cancelCommandExecution(
                                    ucb::IOErrorCode_GENERAL,
                                    uno::Sequence< uno::Any >( 0 ),
                                    xEnv,
                                    u"CancelCheckout only supported by documents"_ustr );
            }
            pPwc->cancelCheckout( );

            // Get the Original document (latest version)
            std::vector< libcmis::DocumentPtr > aVersions = pPwc->getAllVersions( );
            for ( const auto& rVersion : aVersions )
            {
                libcmis::DocumentPtr pVersion = rVersion;
                std::map< std::string, libcmis::PropertyPtr > aProps = pVersion->getProperties( );
                bool bIsLatestVersion = false;
                std::map< std::string, libcmis::PropertyPtr >::iterator propIt = aProps.find( std::string( "cmis:isLatestVersion" ) );
                if ( propIt != aProps.end( ) && !propIt->second->getBools( ).empty( ) )
                {
                    bIsLatestVersion = propIt->second->getBools( ).front( );
                }

                if ( bIsLatestVersion )
                {
                    // Compute the URL of the Document
                    URL aCmisUrl( m_sURL );
                    std::vector< std::string > aPaths = pVersion->getPaths( );
                    if ( !aPaths.empty() )
                    {
                        aCmisUrl.setObjectPath(STD_TO_OUSTR(aPaths.front()));
                    }
                    else
                    {
                        // We may have unfiled doc depending on the server, those
                        // won't have any path, use their ID instead
                        auto sId = pVersion->getId( );
                        aCmisUrl.setObjectId( STD_TO_OUSTR( sId ) );
                    }
                    aRet = aCmisUrl.asString( );
                    break;
                }
            }
        }
        catch ( const libcmis::Exception& e )
        {
            SAL_INFO( "ucb.ucp.cmis", "Unexpected libcmis exception: " << e.what( ) );
            ucbhelper::cancelCommandExecution(
                                ucb::IOErrorCode_GENERAL,
                                uno::Sequence< uno::Any >( 0 ),
                                xEnv,
                                o3tl::runtimeToOUString(e.what()));
        }
        return aRet;
    }

    uno::Sequence< document::CmisVersion> Content::getAllVersions( const uno::Reference< ucb::XCommandEnvironment > & xEnv )
    {
        try
        {
            // get the document
            libcmis::DocumentPtr pDoc = boost::dynamic_pointer_cast< libcmis::Document >( getObject( xEnv ) );
            if ( pDoc.get( ) == nullptr )
            {
                ucbhelper::cancelCommandExecution(
                                    ucb::IOErrorCode_GENERAL,
                                    uno::Sequence< uno::Any >( 0 ),
                                    xEnv,
                                    u"Can not get the document"_ustr );
            }
            std::vector< libcmis::DocumentPtr > aCmisVersions = pDoc->getAllVersions( );
            uno::Sequence< document::CmisVersion > aVersions( aCmisVersions.size( ) );
            auto aVersionsRange = asNonConstRange(aVersions);
            int i = 0;
            for ( const auto& rVersion : aCmisVersions )
            {
                libcmis::DocumentPtr pVersion = rVersion;
                aVersionsRange[i].Id = STD_TO_OUSTR( pVersion->getId( ) );
                aVersionsRange[i].Author = STD_TO_OUSTR( pVersion->getCreatedBy( ) );
                aVersionsRange[i].TimeStamp = lcl_boostToUnoTime( pVersion->getLastModificationDate( ) );
                aVersionsRange[i].Comment = STD_TO_OUSTR( pVersion->getStringProperty("cmis:checkinComment") );
                ++i;
            }
            return aVersions;
        }
        catch ( const libcmis::Exception& e )
        {
            SAL_INFO( "ucb.ucp.cmis", "Unexpected libcmis exception: " << e.what( ) );
            ucbhelper::cancelCommandExecution(
                    ucb::IOErrorCode_GENERAL,
                    uno::Sequence< uno::Any >( 0 ),
                    xEnv,
                    o3tl::runtimeToOUString(e.what()));
        }
        return uno::Sequence< document::CmisVersion > ( );
    }

    void Content::transfer( const ucb::TransferInfo& rTransferInfo,
        const uno::Reference< ucb::XCommandEnvironment > & xEnv )
    {
        // If the source isn't on the same CMIS repository, then simply copy
        INetURLObject aSourceUrl( rTransferInfo.SourceURL );
        if ( aSourceUrl.GetProtocol() != INetProtocol::Cmis )
        {
            OUString sSrcBindingUrl = URL( rTransferInfo.SourceURL ).getBindingUrl( );
            if ( sSrcBindingUrl != m_aURL.getBindingUrl( ) )
            {
                ucbhelper::cancelCommandExecution(
                    uno::Any(
                        ucb::InteractiveBadTransferURLException(
                            u"Unsupported URL scheme!"_ustr,
                            getXWeak() ) ),
                    xEnv );
            }
        }

        SAL_INFO( "ucb.ucp.cmis", "TODO - Content::transfer()" );
    }

    void Content::insert( const uno::Reference< io::XInputStream > & xInputStream,
        bool bReplaceExisting, std::u16string_view rMimeType,
        const uno::Reference< ucb::XCommandEnvironment >& xEnv )
    {
        if ( !xInputStream.is() )
        {
            ucbhelper::cancelCommandExecution( uno::Any
                ( ucb::MissingInputStreamException
                  ( OUString(), getXWeak() ) ),
                xEnv );
        }

        // For transient content, the URL is the one of the parent
        if ( !m_bTransient )
            return;

        OUString sNewPath;

        // Try to get the object from the server if there is any
        libcmis::FolderPtr pFolder;
        try
        {
            pFolder = boost::dynamic_pointer_cast< libcmis::Folder >( getObject( xEnv ) );
        }
        catch ( const libcmis::Exception& )
        {
        }

        if ( pFolder == nullptr )
            return;

        libcmis::ObjectPtr object;
        std::map< std::string, libcmis::PropertyPtr >::iterator it = m_pObjectProps.find( "cmis:name" );
        if ( it == m_pObjectProps.end( ) )
        {
            ucbhelper::cancelCommandExecution( uno::Any
                ( uno::RuntimeException( u"Missing name property"_ustr,
                    getXWeak() ) ),
                xEnv );
        }
        auto newPath = OUSTR_TO_STDSTR( m_sObjectPath );
        if ( !newPath.empty( ) && newPath[ newPath.size( ) - 1 ] != '/' )
            newPath += "/";
        newPath += it->second->getStrings( ).front( );
        try
        {
            if ( !m_sObjectId.isEmpty( ) )
                object = getSession( xEnv )->getObject( OUSTR_TO_STDSTR( m_sObjectId) );
            else
                object = getSession( xEnv )->getObjectByPath( newPath );
            sNewPath = STD_TO_OUSTR( newPath );
        }
        catch ( const libcmis::Exception& )
        {
            // Nothing matched the path
        }

        if ( nullptr != object.get( ) )
        {
            // Are the base type matching?
            if ( object->getBaseType( ) != m_pObjectType->getBaseType( )->getId() )
            {
                ucbhelper::cancelCommandExecution( uno::Any
                    ( uno::RuntimeException( u"Can't change a folder into a document and vice-versa."_ustr,
                        getXWeak() ) ),
                    xEnv );
            }

            // Update the existing object if it's a document
            libcmis::Document* document = dynamic_cast< libcmis::Document* >( object.get( ) );
            if ( nullptr != document )
            {
                boost::shared_ptr< std::ostream > pOut( new std::ostringstream ( std::ios_base::binary | std::ios_base::in | std::ios_base::out ) );
                uno::Reference < io::XOutputStream > xOutput = new StdOutputStream( pOut );
                copyData( xInputStream, xOutput );
                try
                {
                    document->setContentStream( std::move(pOut), OUSTR_TO_STDSTR( rMimeType ), std::string( ), bReplaceExisting );
                }
                catch ( const libcmis::Exception& )
                {
                    ucbhelper::cancelCommandExecution( uno::Any
                        ( uno::RuntimeException( u"Error when setting document content"_ustr,
                            getXWeak() ) ),
                        xEnv );
                }
            }
        }
        else
        {
            // We need to create a brand new object... either folder or document
            bool bIsFolder = getObjectType( xEnv )->getBaseType( )->getId( ) == "cmis:folder";
            setCmisProperty( "cmis:objectTypeId", getObjectType( xEnv )->getId( ), xEnv );

            if ( bIsFolder )
            {
                try
                {
                    pFolder->createFolder( m_pObjectProps );
                    sNewPath = STD_TO_OUSTR( newPath );
                }
                catch ( const libcmis::Exception& )
                {
                    ucbhelper::cancelCommandExecution( uno::Any
                        ( uno::RuntimeException( u"Error when creating folder"_ustr,
                            getXWeak() ) ),
                        xEnv );
                }
            }
            else
            {
                boost::shared_ptr< std::ostream > pOut( new std::ostringstream ( std::ios_base::binary | std::ios_base::in | std::ios_base::out ) );
                uno::Reference < io::XOutputStream > xOutput = new StdOutputStream( pOut );
                copyData( xInputStream, xOutput );
                try
                {
                    pFolder->createDocument( m_pObjectProps, std::move(pOut), OUSTR_TO_STDSTR( rMimeType ), std::string() );
                    sNewPath = STD_TO_OUSTR( newPath );
                }
                catch ( const libcmis::Exception& )
                {
                    ucbhelper::cancelCommandExecution( uno::Any
                        ( uno::RuntimeException( u"Error when creating document"_ustr,
                            getXWeak() ) ),
                        xEnv );
                }
            }
        }

        if ( sNewPath.isEmpty( ) && m_sObjectId.isEmpty( ) )
            return;

        // Update the current content: it's no longer transient
        m_sObjectPath = sNewPath;
        URL aUrl( m_sURL );
        aUrl.setObjectPath( m_sObjectPath );
        aUrl.setObjectId( m_sObjectId );
        m_sURL = aUrl.asString( );
        m_pObject.reset( );
        m_pObjectType.reset( );
        m_pObjectProps.clear( );
        m_bTransient = false;
        inserted();
    }

    const int TRANSFER_BUFFER_SIZE = 65536;

    void Content::copyData(
        const uno::Reference< io::XInputStream >& xIn,
        const uno::Reference< io::XOutputStream >& xOut )
    {
        uno::Sequence< sal_Int8 > theData( TRANSFER_BUFFER_SIZE );

        while ( xIn->readBytes( theData, TRANSFER_BUFFER_SIZE ) > 0 )
            xOut->writeBytes( theData );

        xOut->closeOutput();
    }

    uno::Sequence< uno::Any > Content::setPropertyValues(
            const uno::Sequence< beans::PropertyValue >& rValues,
            const uno::Reference< ucb::XCommandEnvironment >& xEnv )
    {
        try
        {
            // Get the already set properties if possible
            if ( !m_bTransient && getObject( xEnv ).get( ) )
            {
                m_pObjectProps.clear( );
                m_pObjectType = getObject( xEnv )->getTypeDescription();
            }
        }
        catch ( const libcmis::Exception& e )
        {
            SAL_INFO( "ucb.ucp.cmis", "Unexpected libcmis exception: " << e.what( ) );
            ucbhelper::cancelCommandExecution(
                                ucb::IOErrorCode_GENERAL,
                                uno::Sequence< uno::Any >( 0 ),
                                xEnv,
                                o3tl::runtimeToOUString(e.what()));
        }

        sal_Int32 nCount = rValues.getLength();
        uno::Sequence< uno::Any > aRet( nCount );
        auto aRetRange = asNonConstRange(aRet);
        bool bChanged = false;
        const beans::PropertyValue* pValues = rValues.getConstArray();
        for ( sal_Int32 n = 0; n < nCount; ++n )
        {
            const beans::PropertyValue& rValue = pValues[ n ];
            if ( rValue.Name == "ContentType" ||
                 rValue.Name == "MediaType" ||
                 rValue.Name == "IsDocument" ||
                 rValue.Name == "IsFolder" ||
                 rValue.Name == "Size" ||
                 rValue.Name == "CreatableContentsInfo" )
            {
                lang::IllegalAccessException e ( u"Property is read-only!"_ustr,
                       getXWeak() );
                aRetRange[ n ] <<= e;
            }
            else if ( rValue.Name == "Title" )
            {
                OUString aNewTitle;
                if (!( rValue.Value >>= aNewTitle ))
                {
                    aRetRange[ n ] <<= beans::IllegalTypeException
                        ( u"Property value has wrong type!"_ustr,
                          getXWeak() );
                    continue;
                }

                if ( aNewTitle.isEmpty() )
                {
                    aRetRange[ n ] <<= lang::IllegalArgumentException
                        ( u"Empty title not allowed!"_ustr,
                          getXWeak(), -1 );
                    continue;

                }

                setCmisProperty( "cmis:name", OUSTR_TO_STDSTR( aNewTitle ), xEnv );
                bChanged = true;
            }
            else
            {
                SAL_INFO( "ucb.ucp.cmis", "Couldn't set property: " << rValue.Name );
                lang::IllegalAccessException e ( u"Property is read-only!"_ustr,
                       getXWeak() );
                aRetRange[ n ] <<= e;
            }
        }

        try
        {
            if ( !m_bTransient && bChanged )
            {
                getObject( xEnv )->updateProperties( m_pObjectProps );
            }
        }
        catch ( const libcmis::Exception& e )
        {
            SAL_INFO( "ucb.ucp.cmis", "Unexpected libcmis exception: " << e.what( ) );
            ucbhelper::cancelCommandExecution(
                                ucb::IOErrorCode_GENERAL,
                                uno::Sequence< uno::Any >( 0 ),
                                xEnv,
                                o3tl::runtimeToOUString(e.what()));
        }

        return aRet;
    }

    bool Content::feedSink( const uno::Reference< uno::XInterface>& xSink,
        const uno::Reference< ucb::XCommandEnvironment >& xEnv )
    {
        if ( !xSink.is() )
            return false;

        uno::Reference< io::XOutputStream > xOut(xSink, uno::UNO_QUERY );
        uno::Reference< io::XActiveDataSink > xDataSink(xSink, uno::UNO_QUERY );
        uno::Reference< io::XActiveDataStreamer > xDataStreamer( xSink, uno::UNO_QUERY );

        if ( !xOut.is() && !xDataSink.is() && ( !xDataStreamer.is() || !xDataStreamer->getStream().is() ) )
            return false;

        if ( xDataStreamer.is() && !xOut.is() )
            xOut = xDataStreamer->getStream()->getOutputStream();

        try
        {
            libcmis::Document* document = dynamic_cast< libcmis::Document* >( getObject( xEnv ).get() );

            if (!document)
                return false;

            uno::Reference< io::XInputStream > xIn = new StdInputStream(document->getContentStream());
            if( !xIn.is( ) )
                return false;

            if ( xDataSink.is() )
                xDataSink->setInputStream( xIn );
            else if ( xOut.is() )
                copyData( xIn, xOut );
        }
        catch ( const libcmis::Exception& e )
        {
            SAL_INFO( "ucb.ucp.cmis", "Unexpected libcmis exception: " << e.what( ) );
            ucbhelper::cancelCommandExecution(
                                ucb::IOErrorCode_GENERAL,
                                uno::Sequence< uno::Any >( 0 ),
                                xEnv,
                                o3tl::runtimeToOUString(e.what()));
        }

        return true;
    }

    uno::Sequence< beans::Property > Content::getProperties(
            const uno::Reference< ucb::XCommandEnvironment > & )
    {
        static const beans::Property aGenericProperties[] =
        {
            beans::Property( u"IsDocument"_ustr,
                -1, cppu::UnoType<bool>::get(),
                beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY ),
            beans::Property( u"IsFolder"_ustr,
                -1, cppu::UnoType<bool>::get(),
                beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY ),
            beans::Property( u"Title"_ustr,
                -1, cppu::UnoType<OUString>::get(),
                beans::PropertyAttribute::BOUND ),
            beans::Property( u"ObjectId"_ustr,
                -1, cppu::UnoType<OUString>::get(),
                beans::PropertyAttribute::BOUND ),
            beans::Property( u"TitleOnServer"_ustr,
                -1, cppu::UnoType<OUString>::get(),
                beans::PropertyAttribute::BOUND ),
            beans::Property( u"IsReadOnly"_ustr,
                -1, cppu::UnoType<bool>::get(),
                beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY ),
            beans::Property( u"DateCreated"_ustr,
                -1, cppu::UnoType<util::DateTime>::get(),
                beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY ),
            beans::Property( u"DateModified"_ustr,
                -1, cppu::UnoType<util::DateTime>::get(),
                beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY ),
            beans::Property( u"Size"_ustr,
                -1, cppu::UnoType<sal_Int64>::get(),
                beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY ),
            beans::Property( u"CreatableContentsInfo"_ustr,
                -1, cppu::UnoType<uno::Sequence< ucb::ContentInfo >>::get(),
                beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY ),
            beans::Property( u"MediaType"_ustr,
                -1, cppu::UnoType<OUString>::get(),
                beans::PropertyAttribute::BOUND ),
            beans::Property( u"CmisProperties"_ustr,
                -1, cppu::UnoType<uno::Sequence< document::CmisProperty>>::get(),
                beans::PropertyAttribute::BOUND ),
            beans::Property( u"IsVersionable"_ustr,
                -1, cppu::UnoType<bool>::get(),
                beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY ),
            beans::Property( u"CanCheckOut"_ustr,
                -1, cppu::UnoType<bool>::get(),
                beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY ),
            beans::Property( u"CanCancelCheckOut"_ustr,
                -1, cppu::UnoType<bool>::get(),
                beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY ),
            beans::Property( u"CanCheckIn"_ustr,
                -1, cppu::UnoType<bool>::get(),
                beans::PropertyAttribute::BOUND | beans::PropertyAttribute::READONLY ),
        };

        const int nProps = SAL_N_ELEMENTS(aGenericProperties);
        return uno::Sequence< beans::Property > ( aGenericProperties, nProps );
    }

    uno::Sequence< ucb::CommandInfo > Content::getCommands(
            const uno::Reference< ucb::XCommandEnvironment > & xEnv )
    {
        static const ucb::CommandInfo aCommandInfoTable[] =
        {
            // Required commands
            ucb::CommandInfo
            ( u"getCommandInfo"_ustr,
              -1, cppu::UnoType<void>::get() ),
            ucb::CommandInfo
            ( u"getPropertySetInfo"_ustr,
              -1, cppu::UnoType<void>::get() ),
            ucb::CommandInfo
            ( u"getPropertyValues"_ustr,
              -1, cppu::UnoType<uno::Sequence< beans::Property >>::get() ),
            ucb::CommandInfo
            ( u"setPropertyValues"_ustr,
              -1, cppu::UnoType<uno::Sequence< beans::PropertyValue >>::get() ),

            // Optional standard commands
            ucb::CommandInfo
            ( u"delete"_ustr,
              -1, cppu::UnoType<bool>::get() ),
            ucb::CommandInfo
            ( u"insert"_ustr,
              -1, cppu::UnoType<ucb::InsertCommandArgument2>::get() ),
            ucb::CommandInfo
            ( u"open"_ustr,
              -1, cppu::UnoType<ucb::OpenCommandArgument2>::get() ),

            // Mandatory CMIS-only commands
            ucb::CommandInfo ( u"checkout"_ustr, -1, cppu::UnoType<void>::get() ),
            ucb::CommandInfo ( u"cancelCheckout"_ustr, -1, cppu::UnoType<void>::get() ),
            ucb::CommandInfo ( u"checkIn"_ustr, -1,
                    cppu::UnoType<ucb::TransferInfo>::get() ),
            ucb::CommandInfo ( u"updateProperties"_ustr, -1, cppu::UnoType<void>::get() ),
            ucb::CommandInfo
            ( u"getAllVersions"_ustr,
              -1, cppu::UnoType<uno::Sequence< document::CmisVersion >>::get() ),


            // Folder Only, omitted if not a folder
            ucb::CommandInfo
            ( u"transfer"_ustr,
              -1, cppu::UnoType<ucb::TransferInfo>::get() ),
            ucb::CommandInfo
            ( u"createNewContent"_ustr,
              -1, cppu::UnoType<ucb::ContentInfo>::get() )
        };

        const int nProps = SAL_N_ELEMENTS( aCommandInfoTable );
        return uno::Sequence< ucb::CommandInfo >(aCommandInfoTable, isFolder( xEnv ) ? nProps : nProps - 2);
    }

    OUString Content::getParentURL( )
    {
        SAL_INFO( "ucb.ucp.cmis", "Content::getParentURL()" );
        OUString parentUrl = u"/"_ustr;
        if ( m_sObjectPath == "/" )
            return parentUrl;
        else
        {
            INetURLObject aUrl( m_sURL );
            if ( aUrl.getSegmentCount( ) > 0 )
            {
                URL aCmisUrl( m_sURL );
                aUrl.removeSegment( );
                aCmisUrl.setObjectPath( aUrl.GetURLPath( INetURLObject::DecodeMechanism::WithCharset ) );
                parentUrl = aCmisUrl.asString( );
            }
        }
        return parentUrl;
    }

    XTYPEPROVIDER_COMMON_IMPL( Content );

    void SAL_CALL Content::acquire() noexcept
    {
        ContentImplHelper::acquire();
    }

    void SAL_CALL Content::release() noexcept
    {
        ContentImplHelper::release();
    }

    uno::Any SAL_CALL Content::queryInterface( const uno::Type & rType )
    {
        uno::Any aRet = cppu::queryInterface( rType, static_cast< ucb::XContentCreator * >( this ) );
        return aRet.hasValue() ? aRet : ContentImplHelper::queryInterface(rType);
    }

    OUString SAL_CALL Content::getImplementationName()
    {
       return u"com.sun.star.comp.CmisContent"_ustr;
    }

    uno::Sequence< OUString > SAL_CALL Content::getSupportedServiceNames()
    {
           uno::Sequence<OUString> aSNS { u"com.sun.star.ucb.CmisContent"_ustr };
           return aSNS;
    }

    OUString SAL_CALL Content::getContentType()
    {
        OUString sRet;
        try
        {
            if (isFolder( uno::Reference< ucb::XCommandEnvironment >() ))
                sRet = CMIS_FOLDER_TYPE;
            else
                sRet = CMIS_FILE_TYPE;
        }
        catch (const uno::RuntimeException&)
        {
            throw;
        }
        catch (const uno::Exception& e)
        {
            uno::Any a(cppu::getCaughtException());
            throw lang::WrappedTargetRuntimeException(
                "wrapped Exception " + e.Message,
                uno::Reference<uno::XInterface>(), a);
        }
        return sRet;
    }

    uno::Any SAL_CALL Content::execute(
        const ucb::Command& aCommand,
        sal_Int32 /*CommandId*/,
        const uno::Reference< ucb::XCommandEnvironment >& xEnv )
    {
        SAL_INFO( "ucb.ucp.cmis", "Content::execute( ) - " << aCommand.Name );
        uno::Any aRet;

        if ( aCommand.Name == "getPropertyValues" )
        {
            uno::Sequence< beans::Property > Properties;
            if ( !( aCommand.Argument >>= Properties ) )
                ucbhelper::cancelCommandExecution ( getBadArgExcept (), xEnv );
            aRet <<= getPropertyValues( Properties, xEnv );
        }
        else if ( aCommand.Name == "getPropertySetInfo" )
            aRet <<= getPropertySetInfo( xEnv, false );
        else if ( aCommand.Name == "getCommandInfo" )
            aRet <<= getCommandInfo( xEnv, false );
        else if ( aCommand.Name == "open" )
        {
            ucb::OpenCommandArgument2 aOpenCommand;
            if ( !( aCommand.Argument >>= aOpenCommand ) )
                ucbhelper::cancelCommandExecution ( getBadArgExcept (), xEnv );
            aRet = open( aOpenCommand, xEnv );
        }
        else if ( aCommand.Name == "transfer" )
        {
            ucb::TransferInfo transferArgs;
            if ( !( aCommand.Argument >>= transferArgs ) )
                ucbhelper::cancelCommandExecution ( getBadArgExcept (), xEnv );
            transfer( transferArgs, xEnv );
        }
        else if ( aCommand.Name == "setPropertyValues" )
        {
            uno::Sequence< beans::PropertyValue > aProperties;
            if ( !( aCommand.Argument >>= aProperties ) || !aProperties.hasElements() )
                ucbhelper::cancelCommandExecution ( getBadArgExcept (), xEnv );
            aRet <<= setPropertyValues( aProperties, xEnv );
        }
        else if (aCommand.Name == "createNewContent"
                 && isFolder( xEnv ) )
        {
            ucb::ContentInfo arg;
            if ( !( aCommand.Argument >>= arg ) )
                    ucbhelper::cancelCommandExecution ( getBadArgExcept (), xEnv );
            aRet <<= createNewContent( arg );
        }
        else if ( aCommand.Name == "insert" )
        {
            ucb::InsertCommandArgument2 arg;
            if ( !( aCommand.Argument >>= arg ) )
            {
                ucb::InsertCommandArgument insertArg;
                if ( !( aCommand.Argument >>= insertArg ) )
                    ucbhelper::cancelCommandExecution ( getBadArgExcept (), xEnv );

                arg.Data = insertArg.Data;
                arg.ReplaceExisting = insertArg.ReplaceExisting;
            }
            // store the document id
            m_sObjectId = arg.DocumentId;
            insert( arg.Data, arg.ReplaceExisting, arg.MimeType, xEnv );
        }
        else if ( aCommand.Name == "delete" )
        {
            try
            {
                if ( !isFolder( xEnv ) )
                {
                    getObject( xEnv )->remove( );
                }
                else
                {
                    libcmis::Folder* folder = dynamic_cast< libcmis::Folder* >( getObject( xEnv ).get() );
                    if (folder)
                        folder->removeTree( );
                }
            }
            catch ( const libcmis::Exception& e )
            {
                SAL_INFO( "ucb.ucp.cmis", "Unexpected libcmis exception: " << e.what( ) );
                ucbhelper::cancelCommandExecution(
                                    ucb::IOErrorCode_GENERAL,
                                    uno::Sequence< uno::Any >( 0 ),
                                    xEnv,
                                    o3tl::runtimeToOUString(e.what()));
            }
        }
        else if ( aCommand.Name == "checkout" )
        {
            aRet <<= checkOut( xEnv );
        }
        else if ( aCommand.Name == "cancelCheckout" )
        {
            aRet <<= cancelCheckOut( xEnv );
        }
        else if ( aCommand.Name == "checkin" )
        {
            ucb::CheckinArgument aArg;
            if ( !( aCommand.Argument >>= aArg ) )
            {
                ucbhelper::cancelCommandExecution ( getBadArgExcept(), xEnv );
            }
            aRet <<= checkIn( aArg, xEnv );
        }
        else if ( aCommand.Name == "getAllVersions" )
        {
            aRet <<= getAllVersions( xEnv );
        }
        else if ( aCommand.Name == "updateProperties" )
        {
            updateProperties( aCommand.Argument, xEnv );
        }
        else
        {
            SAL_INFO( "ucb.ucp.cmis", "Unknown command to execute" );

            ucbhelper::cancelCommandExecution
                ( uno::Any( ucb::UnsupportedCommandException
                  ( OUString(),
                    getXWeak() ) ),
                  xEnv );
        }

        return aRet;
    }

    void SAL_CALL Content::abort( sal_Int32 /*CommandId*/ )
    {
        SAL_INFO( "ucb.ucp.cmis", "TODO - Content::abort()" );
        // TODO Implement me
    }

    uno::Sequence< ucb::ContentInfo > SAL_CALL Content::queryCreatableContentsInfo()
    {
        return queryCreatableContentsInfo( uno::Reference< ucb::XCommandEnvironment >() );
    }

    uno::Reference< ucb::XContent > SAL_CALL Content::createNewContent(
            const ucb::ContentInfo& Info )
    {
        bool create_document;

        if ( Info.Type == CMIS_FILE_TYPE )
            create_document = true;
        else if ( Info.Type == CMIS_FOLDER_TYPE )
            create_document = false;
        else
        {
            SAL_INFO( "ucb.ucp.cmis", "Unknown type of content to create" );
            return uno::Reference< ucb::XContent >();
        }

        OUString sParentURL = m_xIdentifier->getContentIdentifier();

        // Set the parent URL for the transient objects
        uno::Reference< ucb::XContentIdentifier > xId(new ::ucbhelper::ContentIdentifier(sParentURL));

        try
        {
            return new ::cmis::Content( m_xContext, m_pProvider, xId, !create_document );
        }
        catch ( ucb::ContentCreationException & )
        {
            return uno::Reference< ucb::XContent >();
        }
    }

    uno::Sequence< uno::Type > SAL_CALL Content::getTypes()
    {
        try
        {
            if ( isFolder( uno::Reference< ucb::XCommandEnvironment >() ) )
            {
                static cppu::OTypeCollection s_aFolderCollection
                    (CPPU_TYPE_REF( lang::XTypeProvider ),
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
                return s_aFolderCollection.getTypes();
            }
        }
        catch (const uno::RuntimeException&)
        {
            throw;
        }
        catch (const uno::Exception& e)
        {
            uno::Any a(cppu::getCaughtException());
            throw lang::WrappedTargetRuntimeException(
                "wrapped Exception " + e.Message,
                uno::Reference<uno::XInterface>(), a);
        }

        static cppu::OTypeCollection s_aFileCollection
            (CPPU_TYPE_REF( lang::XTypeProvider ),
             CPPU_TYPE_REF( lang::XServiceInfo ),
             CPPU_TYPE_REF( lang::XComponent ),
             CPPU_TYPE_REF( ucb::XContent ),
             CPPU_TYPE_REF( ucb::XCommandProcessor ),
             CPPU_TYPE_REF( beans::XPropertiesChangeNotifier ),
             CPPU_TYPE_REF( ucb::XCommandInfoChangeNotifier ),
             CPPU_TYPE_REF( beans::XPropertyContainer ),
             CPPU_TYPE_REF( beans::XPropertySetInfoChangeNotifier ),
             CPPU_TYPE_REF( container::XChild ) );

        return s_aFileCollection.getTypes();
    }

    uno::Sequence< ucb::ContentInfo > Content::queryCreatableContentsInfo(
        const uno::Reference< ucb::XCommandEnvironment >& xEnv)
    {
        try
        {
            if ( isFolder( xEnv ) )
            {

                // Minimum set of props we really need
                uno::Sequence< beans::Property > props
                {
                    {
                        u"Title"_ustr,
                        -1,
                        cppu::UnoType<OUString>::get(),
                        beans::PropertyAttribute::MAYBEVOID | beans::PropertyAttribute::BOUND
                    }
                };

                return
                {
                    {
                        CMIS_FILE_TYPE,
                        ( ucb::ContentInfoAttribute::INSERT_WITH_INPUTSTREAM |
                                      ucb::ContentInfoAttribute::KIND_DOCUMENT ),
                        props
                    },
                    {
                        CMIS_FOLDER_TYPE,
                        ucb::ContentInfoAttribute::KIND_FOLDER,
                        props
                    }
                };
            }
        }
        catch (const uno::RuntimeException&)
        {
            throw;
        }
        catch (const uno::Exception& e)
        {
            uno::Any a(cppu::getCaughtException());
            throw lang::WrappedTargetRuntimeException(
                "wrapped Exception " + e.Message,
                uno::Reference<uno::XInterface>(), a);
        }
        return {};
    }

    std::vector< uno::Reference< ucb::XContent > > Content::getChildren( )
    {
        std::vector< uno::Reference< ucb::XContent > > results;
        SAL_INFO( "ucb.ucp.cmis", "Content::getChildren() " << m_sURL );

        libcmis::FolderPtr pFolder = boost::dynamic_pointer_cast< libcmis::Folder >( getObject( uno::Reference< ucb::XCommandEnvironment >() ) );
        if ( nullptr != pFolder )
        {
            // Get the children from pObject
            try
            {
                std::vector< libcmis::ObjectPtr > children = pFolder->getChildren( );

                // Loop over the results
                for ( const auto& rChild : children )
                {
                    // TODO Cache the objects

                    INetURLObject aURL( m_sURL );
                    OUString sUser = aURL.GetUser( INetURLObject::DecodeMechanism::WithCharset );

                    URL aUrl( m_sURL );
                    OUString sPath( m_sObjectPath );
                    if ( !sPath.endsWith("/") )
                        sPath += "/";
                    sPath += STD_TO_OUSTR( rChild->getName( ) );
                    OUString sId = STD_TO_OUSTR( rChild->getId( ) );

                    aUrl.setObjectId( sId );
                    aUrl.setObjectPath( sPath );
                    aUrl.setUsername( sUser );

                    uno::Reference< ucb::XContentIdentifier > xId = new ucbhelper::ContentIdentifier( aUrl.asString( ) );
                    uno::Reference< ucb::XContent > xContent = new Content( m_xContext, m_pProvider, xId, rChild );

                    results.push_back( xContent );
                }
            }
            catch ( const libcmis::Exception& e )
            {
                SAL_INFO( "ucb.ucp.cmis", "Exception thrown: " << e.what() );
            }
        }

        return results;
    }

    void Content::setCmisProperty(const std::string& rName, const std::string& rValue, const uno::Reference< ucb::XCommandEnvironment >& xEnv )
    {
        if ( !getObjectType( xEnv ).get( ) )
            return;

        std::map< std::string, libcmis::PropertyPtr >::iterator propIt = m_pObjectProps.find(rName);

        if ( propIt == m_pObjectProps.end( ) && getObjectType( xEnv ).get( ) )
        {
            std::map< std::string, libcmis::PropertyTypePtr > propsTypes = getObjectType( xEnv )->getPropertiesTypes( );
            std::map< std::string, libcmis::PropertyTypePtr >::iterator typeIt = propsTypes.find(rName);

            if ( typeIt != propsTypes.end( ) )
            {
                libcmis::PropertyPtr property( new libcmis::Property( typeIt->second, { rValue }) );
                m_pObjectProps.insert(std::pair< std::string, libcmis::PropertyPtr >(rName, property));
            }
        }
        else if ( propIt != m_pObjectProps.end( ) )
        {
            propIt->second->setValues( { rValue } );
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
