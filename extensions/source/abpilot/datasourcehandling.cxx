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


#include <strings.hrc>
#include "abptypes.hxx"
#include <componentmodule.hxx>
#include "datasourcehandling.hxx"
#include "addresssettings.hxx"

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/sdb/DatabaseContext.hpp>
#include <com/sun/star/sdb/SQLContext.hpp>
#include <com/sun/star/sdb/XCompletedConnection.hpp>
#include <com/sun/star/sdb/XDatabaseRegistrations.hpp>
#include <com/sun/star/sdb/XDocumentDataSource.hpp>
#include <com/sun/star/sdbc/XConnection.hpp>
#include <com/sun/star/sdbcx/XTablesSupplier.hpp>
#include <com/sun/star/task/InteractionHandler.hpp>
#include <com/sun/star/uri/UriReferenceFactory.hpp>
#include <com/sun/star/uri/VndSunStarPkgUrlReferenceFactory.hpp>

#include <comphelper/interaction.hxx>
#include <comphelper/processfactory.hxx>
#include <tools/debug.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <unotools/sharedunocomponent.hxx>
#include <vcl/stdtext.hxx>
#include <vcl/weld.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/viewfrm.hxx>
#include <comphelper/propertysequence.hxx>

namespace
{

/// Returns the URL of this object shell.
OUString lcl_getOwnURL(SfxObjectShell const * pObjectShell)
{
    OUString aRet;

    if (!pObjectShell)
        return aRet;

    const INetURLObject& rURLObject = pObjectShell->GetMedium()->GetURLObject();
    aRet = rURLObject.GetMainURL(INetURLObject::DecodeMechanism::NONE);
    return aRet;
}

}

namespace abp
{


    using namespace ::utl;
    using namespace ::comphelper;
    using namespace ::com::sun::star;
    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::lang;
    using namespace ::com::sun::star::sdb;
    using namespace ::com::sun::star::sdbc;
    using namespace ::com::sun::star::task;
    using namespace ::com::sun::star::beans;
    using namespace ::com::sun::star::sdbcx;
    using namespace ::com::sun::star::container;
    using namespace ::com::sun::star::frame;

    static Reference< XDatabaseContext > lcl_getDataSourceContext( const Reference< XComponentContext >& _rxContext )
    {
        Reference<XDatabaseContext> xContext = DatabaseContext::create(_rxContext);
        return xContext;
    }


    /// creates a new data source and inserts it into the context
    static void lcl_implCreateAndInsert(
        const Reference< XComponentContext >& _rxContext, const OUString& _rName,
        Reference< XPropertySet >& /* [out] */ _rxNewDataSource )
    {

        // get the data source context
        Reference< XDatabaseContext > xContext = lcl_getDataSourceContext( _rxContext );

        DBG_ASSERT( !xContext->hasByName( _rName ), "lcl_implCreateAndInsert: name already used!" );


        // create a new data source
        Reference< XPropertySet > xNewDataSource;
        if (xContext.is())
            xNewDataSource.set( xContext->createInstance(), UNO_QUERY );
        DBG_ASSERT( xNewDataSource.is(), "lcl_implCreateAndInsert: could not create a new data source!" );


        // insert the data source into the context
        DBG_ASSERT( xContext.is(), "lcl_implCreateAndInsert: missing an interface on the context (XNamingService)!" );
        if (xContext.is())
        {
            //  xDynamicContext->registerObject( _rName, xNewDataSource );
            _rxNewDataSource = std::move(xNewDataSource);
        }
    }


    /// creates and inserts a data source, and sets its URL property to the string given
    static ODataSource lcl_implCreateAndSetURL(
        const Reference< XComponentContext >& _rxORB, const OUString& _rName,
        const char* _pInitialAsciiURL )
    {
        ODataSource aReturn( _rxORB );
        try
        {
            // create the new data source
            Reference< XPropertySet > xNewDataSource;
            lcl_implCreateAndInsert( _rxORB, _rName, xNewDataSource );


            // set the URL property
            if (xNewDataSource.is())
            {
                xNewDataSource->setPropertyValue(
                    u"URL"_ustr,
                    Any( OUString::createFromAscii( _pInitialAsciiURL ) )
                );
            }

            aReturn.setDataSource( xNewDataSource, _rName );
        }
        catch(const Exception&)
        {
            TOOLS_WARN_EXCEPTION("extensions.abpilot",
                                 "caught an exception while creating the data source!");
        }

        return aReturn;
    }

    static void lcl_registerDataSource(
        const Reference< XComponentContext >& _rxORB, const OUString& _sName,
        const OUString& _sURL )
    {
        OSL_ENSURE( !_sName.isEmpty(), "lcl_registerDataSource: invalid name!" );
        OSL_ENSURE( !_sURL.isEmpty(), "lcl_registerDataSource: invalid URL!" );
        try
        {
            Reference< XDatabaseContext > xRegistrations( DatabaseContext::create(_rxORB) );
            if ( xRegistrations->hasRegisteredDatabase( _sName ) )
                xRegistrations->changeDatabaseLocation( _sName, _sURL );
            else
                xRegistrations->registerDatabaseLocation( _sName, _sURL );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("extensions.abpilot");
        }
    }

    struct ODataSourceContextImpl
    {
        Reference< XComponentContext >      xORB;
        Reference< XNameAccess >            xContext;           /// the UNO data source context
        StringBag                           aDataSourceNames;   /// for quicker name checks (without the UNO overhead)

        explicit ODataSourceContextImpl(const Reference< XComponentContext >& _rxORB)
            : xORB(_rxORB)
        {
        }
        ODataSourceContextImpl(const ODataSourceContextImpl&) = delete;
        ODataSourceContextImpl& operator=(const ODataSourceContextImpl&) = delete;
    };

    ODataSourceContext::ODataSourceContext(const Reference< XComponentContext >& _rxORB)
        :m_pImpl( new ODataSourceContextImpl( _rxORB ) )
    {
        try
        {
            // create the UNO context
            m_pImpl->xContext.set( lcl_getDataSourceContext( _rxORB ), UNO_QUERY_THROW );

            // collect the data source names
            for (auto& rDSName : m_pImpl->xContext->getElementNames())
                m_pImpl->aDataSourceNames.insert(rDSName);
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "extensions.abpilot", "ODataSourceContext::ODataSourceContext" );
        }
    }
    ODataSourceContext::~ODataSourceContext()
    {
    }


    void ODataSourceContext::disambiguate(OUString& _rDataSourceName)
    {
        OUString sCheck( _rDataSourceName );
        StringBag::const_iterator aPos = m_pImpl->aDataSourceNames.find( sCheck );

        sal_Int32 nPostfix = 1;
        while ( ( m_pImpl->aDataSourceNames.end() != aPos ) && ( nPostfix < 65535 ) )
        {   // there already is a data source with this name
            sCheck = _rDataSourceName + OUString::number( nPostfix++ );

            aPos = m_pImpl->aDataSourceNames.find( sCheck );
        }

        _rDataSourceName = sCheck;
    }


    void ODataSourceContext::getDataSourceNames( StringBag& _rNames ) const
    {
        _rNames = m_pImpl->aDataSourceNames;
    }

    ODataSource ODataSourceContext::createNewThunderbird( const OUString& _rName )
    {
        return lcl_implCreateAndSetURL( m_pImpl->xORB, _rName, "sdbc:address:thunderbird" );
    }


    ODataSource ODataSourceContext::createNewEvolutionLdap( const OUString& _rName)
    {
        return lcl_implCreateAndSetURL( m_pImpl->xORB, _rName, "sdbc:address:evolution:ldap" );
    }

    ODataSource ODataSourceContext::createNewEvolutionGroupwise( const OUString& _rName)
    {
        return lcl_implCreateAndSetURL( m_pImpl->xORB, _rName, "sdbc:address:evolution:groupwise" );
    }

    ODataSource ODataSourceContext::createNewEvolution( const OUString& _rName)
    {
        return lcl_implCreateAndSetURL( m_pImpl->xORB, _rName, "sdbc:address:evolution:local" );
    }


    ODataSource ODataSourceContext::createNewKab( const OUString& _rName)
    {
        return lcl_implCreateAndSetURL( m_pImpl->xORB, _rName, "sdbc:address:kab" );
    }


    ODataSource ODataSourceContext::createNewMacab( const OUString& _rName)
    {
        return lcl_implCreateAndSetURL( m_pImpl->xORB, _rName, "sdbc:address:macab" );
    }


    // tdf117101: Spreadsheet by default
    ODataSource ODataSourceContext::createNewOther( const OUString& _rName)
    {
        return lcl_implCreateAndSetURL( m_pImpl->xORB, _rName, "sdbc:calc:" );
    }

    struct ODataSourceImpl
    {
    public:
        Reference< XComponentContext >          xORB;               /// the service factory
        Reference< XPropertySet >               xDataSource;        /// the UNO data source
        ::utl::SharedUNOComponent< XConnection >
                                                xConnection;
        StringBag                               aTables;            // the cached table names
        OUString                                sName;

        explicit ODataSourceImpl(const Reference< XComponentContext >& _rxORB)
            : xORB(_rxORB)
        {
        }
    };


    ODataSource::ODataSource( const ODataSource& _rSource )
    {
        *this = _rSource;
    }

    ODataSource& ODataSource::operator=( const ODataSource& _rSource )
    {
        if( this != &_rSource )
        {
            m_pImpl.reset( new ODataSourceImpl( *_rSource.m_pImpl ) );
        }
        return *this;
    }

    ODataSource& ODataSource::operator=(ODataSource&& _rSource) noexcept
    {
        m_pImpl = std::move(_rSource.m_pImpl);
        return *this;
    }

    ODataSource::ODataSource( const Reference< XComponentContext >& _rxORB )
        :m_pImpl(new ODataSourceImpl(_rxORB))
    {
    }

    ODataSource::~ODataSource( )
    {
    }

    void ODataSource::store(const AddressSettings& rSettings)
    {
        if (!isValid())
            // nothing to do
            return;
        try
        {
            Reference< XDocumentDataSource > xDocAccess( m_pImpl->xDataSource, UNO_QUERY );
            Reference< XStorable > xStorable;
            if ( xDocAccess.is() )
                xStorable.set(xDocAccess->getDatabaseDocument(), css::uno::UNO_QUERY);
            OSL_ENSURE( xStorable.is(),"DataSource is no XStorable!" );
            if ( xStorable.is() )
            {
                SfxViewFrame* pFrame = SfxViewFrame::Current();
                SfxObjectShell* pObjectShell = pFrame ? pFrame->GetObjectShell() : nullptr;
                OUString aOwnURL = lcl_getOwnURL(pObjectShell); // empty if pObjectShell is nullptr
                if (aOwnURL.isEmpty() || !rSettings.bEmbedDataSource)
                {
                    // Cannot or should not embed.
                    xStorable->storeAsURL(m_pImpl->sName,Sequence<PropertyValue>());
                }
                else
                {
                    // Embed.
                    OUString aStreamRelPath = u"EmbeddedDatabase"_ustr;
                    const auto& xContext(comphelper::getProcessComponentContext());
                    auto xUri = css::uri::UriReferenceFactory::create(xContext)->parse(aOwnURL);
                    assert(xUri.is());
                    xUri = css::uri::VndSunStarPkgUrlReferenceFactory::create(xContext)->createVndSunStarPkgUrlReference(xUri);
                    assert(xUri.is());
                    OUString const sTmpName = xUri->getUriReference() + "/" + aStreamRelPath;
                    assert(pObjectShell);
                    uno::Reference<embed::XStorage> xStorage = pObjectShell->GetStorage();
                    uno::Sequence<beans::PropertyValue> aSequence = comphelper::InitPropertySequence(
                    {
                        {"TargetStorage", uno::Any(xStorage)},
                        {"StreamRelPath", uno::Any(aStreamRelPath)},
                        {"BaseURI", uno::Any(aOwnURL)}
                    });
                    xStorable->storeAsURL(sTmpName, aSequence);
                    m_pImpl->sName = sTmpName;

                    // Refer to the sub-storage name in the document settings, so
                    // we can load it again next time the file is imported.
                    uno::Reference<lang::XMultiServiceFactory> xFactory(pObjectShell->GetModel(), uno::UNO_QUERY);
                    uno::Reference<beans::XPropertySet> xPropertySet(xFactory->createInstance(u"com.sun.star.document.Settings"_ustr), uno::UNO_QUERY);
                    xPropertySet->setPropertyValue(u"EmbeddedDatabaseName"_ustr, uno::Any(aStreamRelPath));
                }
            }
        }
        catch(const Exception&)
        {
            TOOLS_WARN_EXCEPTION("extensions.abpilot",
                                 "caught an exception while creating the data source!");
        }
    }

    void ODataSource::registerDataSource( const OUString& _sRegisteredDataSourceName)
    {
        if (!isValid())
            // nothing to do
            return;

        try
        {
            // invalidate ourself
            lcl_registerDataSource(m_pImpl->xORB,_sRegisteredDataSourceName,m_pImpl->sName);
        }
        catch(const Exception&)
        {
            TOOLS_WARN_EXCEPTION("extensions.abpilot",
                                 "caught an exception while creating the data source!");
        }
    }


    void ODataSource::setDataSource( const Reference< XPropertySet >& _rxDS,const OUString& _sName )
    {
        if (m_pImpl->xDataSource.get() == _rxDS.get())
            // nothing to do
            return;

        if ( isConnected() )
            disconnect();

        m_pImpl->sName = _sName;
        m_pImpl->xDataSource = _rxDS;
    }


    void ODataSource::remove()
    {
        if (!isValid())
            // nothing to do
            return;

        try
        {
            // invalidate ourself
            m_pImpl->xDataSource.clear();
        }
        catch(const Exception&)
        {
            TOOLS_WARN_EXCEPTION("extensions.abpilot",
                                 "caught an exception while creating the data source!");
        }
    }


    bool ODataSource::rename( const OUString& _rName )
    {
        if (!isValid())
            // nothing to do
            return false;

        m_pImpl->sName = _rName;
        return true;
    }


    const OUString & ODataSource::getName() const
    {
        if ( !isValid() )
            return EMPTY_OUSTRING;
        return m_pImpl->sName;
    }


    bool ODataSource::hasTable( const OUString& _rTableName ) const
    {
        if ( !isConnected() )
            return false;

        const StringBag& aTables( getTableNames() );
        return aTables.find( _rTableName ) != aTables.end();
    }


    const StringBag& ODataSource::getTableNames() const
    {
        m_pImpl->aTables.clear();
        if ( !isConnected() )
        {
            OSL_FAIL( "ODataSource::getTableNames: not connected!" );
        }
        else
        {
            try
            {
                // get the tables container from the connection
                Reference< XTablesSupplier > xSuppTables( m_pImpl->xConnection.getTyped(), UNO_QUERY );
                Reference< XNameAccess > xTables;
                if ( xSuppTables.is( ) )
                    xTables = xSuppTables->getTables();
                DBG_ASSERT( xTables.is(), "ODataSource::getTableNames: could not retrieve the tables container!" );

                // get the names
                Sequence< OUString > aTableNames;
                if ( xTables.is( ) )
                    aTableNames = xTables->getElementNames( );

                // copy the names
                for (auto& rTableName : aTableNames)
                    m_pImpl->aTables.insert(rTableName);
            }
            catch(const Exception&)
            {
            }
        }

        // now the table cache is up-to-date
        return m_pImpl->aTables;
    }


    bool ODataSource::connect(weld::Window* _pMessageParent)
    {
        if ( isConnected( ) )
            // nothing to do
            return true;


        // create the interaction handler (needed for authentication and error handling)
        Reference< XInteractionHandler > xInteractions;
        try
        {
            xInteractions = InteractionHandler::createWithParent(m_pImpl->xORB, nullptr);
        }
        catch(const Exception&)
        {
        }


        // failure to create the interaction handler is a serious issue ...
        if (!xInteractions.is())
        {
            if ( _pMessageParent )
                ShowServiceNotAvailableError( _pMessageParent, u"com.sun.star.task.InteractionHandler", true );
            return false;
        }


        // open the connection
        Any aError;
        Reference< XConnection > xConnection;
        try
        {
            Reference< XCompletedConnection > xComplConn( m_pImpl->xDataSource, UNO_QUERY );
            DBG_ASSERT( xComplConn.is(), "ODataSource::connect: missing the XCompletedConnection interface on the data source!" );
            if ( xComplConn.is() )
                xConnection = xComplConn->connectWithCompletion( xInteractions );
        }
        catch( const SQLContext& e ) { aError <<= e; }
        catch( const SQLWarning& e ) { aError <<= e; }
        catch( const SQLException& e ) { aError <<= e; }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION("extensions.abpilot", "");
        }


        // handle errors
        if ( aError.hasValue() && _pMessageParent )
        {
            try
            {
                SQLException aException;
                aError >>= aException;
                if ( aException.Message.isEmpty() )
                {
                    // prepend some context info
                    SQLContext aDetailedError(compmodule::ModuleRes(RID_STR_NOCONNECTION), // message
                                              {}, {}, 0,
                                              aError, // next exception
                                              compmodule::ModuleRes(RID_STR_PLEASECHECKSETTINGS)); // details
                    // handle (aka display) the new context info
                    xInteractions->handle( new OInteractionRequest( Any( aDetailedError ) ) );
                }
                else
                {
                      // handle (aka display) the original error
                    xInteractions->handle( new OInteractionRequest( Any( aException ) ) );
                }
            }
            catch( const Exception& )
            {
                TOOLS_WARN_EXCEPTION("extensions.abpilot",
                                     "caught an exception while trying to display the error!");
            }
        }

        if ( !xConnection.is() )
            return false;


        // success
        m_pImpl->xConnection.reset( xConnection );
        m_pImpl->aTables.clear();

        return true;
    }


    void ODataSource::disconnect( )
    {
        m_pImpl->xConnection.clear();
        m_pImpl->aTables.clear();
    }


    bool ODataSource::isConnected( ) const
    {
        return m_pImpl->xConnection.is();
    }


    bool ODataSource::isValid() const
    {
        return m_pImpl && m_pImpl->xDataSource.is();
    }

    Reference< XPropertySet > ODataSource::getDataSource() const
    {
        return m_pImpl ? m_pImpl->xDataSource : Reference< XPropertySet >();
    }


}   // namespace abp


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
