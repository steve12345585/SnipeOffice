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

#include <config_fuzzers.h>

#include "mdrivermanager.hxx"
#include <com/sun/star/configuration/theDefaultProvider.hpp>
#include <com/sun/star/sdbc/XDriver.hpp>
#include <com/sun/star/container/XContentEnumerationAccess.hpp>
#include <com/sun/star/container/ElementExistException.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/logging/LogLevel.hpp>

#include <comphelper/diagnose_ex.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/weak.hxx>
#include <osl/diagnose.h>
#include <officecfg/Office/DataAccess.hxx>

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

namespace drivermanager
{

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::logging;
using namespace ::osl;

constexpr OUStringLiteral SERVICE_SDBC_DRIVER = u"com.sun.star.sdbc.Driver";

class ODriverEnumeration : public ::cppu::WeakImplHelper< XEnumeration >
{
    friend class OSDBCDriverManager;

    typedef std::vector< Reference< XDriver > > DriverArray;
    DriverArray                 m_aDrivers;
    DriverArray::const_iterator m_aPos;
    // order matters!

protected:
    virtual ~ODriverEnumeration() override;
public:
    explicit ODriverEnumeration(DriverArray&& _rDriverSequence);

// XEnumeration
    virtual sal_Bool SAL_CALL hasMoreElements( ) override;
    virtual Any SAL_CALL nextElement( ) override;
};


ODriverEnumeration::ODriverEnumeration(DriverArray&& _rDriverSequence)
    :m_aDrivers( std::move(_rDriverSequence) )
    ,m_aPos( m_aDrivers.begin() )
{
}


ODriverEnumeration::~ODriverEnumeration()
{
}


sal_Bool SAL_CALL ODriverEnumeration::hasMoreElements(  )
{
    return m_aPos != m_aDrivers.end();
}


Any SAL_CALL ODriverEnumeration::nextElement(  )
{
    if ( !hasMoreElements() )
        throw NoSuchElementException();

    return Any( *m_aPos++ );
}

namespace
{
    /// an STL functor which ensures that a SdbcDriver described by a DriverAccess is loaded
    struct EnsureDriver
    {
        explicit EnsureDriver( const Reference< XComponentContext > &rxContext )
            : mxContext( rxContext ) {}

        const DriverAccess& operator()( const DriverAccess& _rDescriptor ) const
        {
            // we did not load this driver, yet
            if (_rDescriptor.xDriver.is())
                return _rDescriptor;

            // we have a factory for it
            if (_rDescriptor.xComponentFactory.is())
            {
                DriverAccess& rDesc = const_cast<DriverAccess&>(_rDescriptor);
                try
                {
                    //load driver
                    rDesc.xDriver.set(
                        rDesc.xComponentFactory->createInstanceWithContext(mxContext), css::uno::UNO_QUERY);
                }
                catch (const Exception&)
                {
                    //failure, abandon driver
                    rDesc.xComponentFactory.clear();
                }
            }
            return _rDescriptor;
        }

    private:
        Reference< XComponentContext > mxContext;
    };

    /// an STL functor which extracts a SdbcDriver from a DriverAccess
    struct ExtractDriverFromAccess
    {
        const Reference<XDriver>& operator()( const DriverAccess& _rAccess ) const
        {
            return _rAccess.xDriver;
        }
    };

    struct ExtractDriverFromCollectionElement
    {
        const Reference<XDriver>& operator()( const DriverCollection::value_type& _rElement ) const
        {
            return _rElement.second;
        }
    };

    // predicate for checking whether or not a driver accepts a given URL
    bool AcceptsURL( const OUString& _rURL, const Reference<XDriver>& _rDriver )
    {
        // ask the driver
        return _rDriver.is() && _rDriver->acceptsURL( _rURL );
    }

    /// an STL algorithm compatible predicate comparing two DriverAccess instances by their implementation names
    struct CompareDriverAccessByName
    {

        bool operator()( const DriverAccess& lhs, const DriverAccess& rhs )
        {
            return lhs.sImplementationName < rhs.sImplementationName;
        }
    };

    /// and an STL algorithm compatible predicate comparing the impl name of a DriverAccess to a string
    struct EqualDriverAccessToName
    {
        OUString m_sImplName;
        explicit EqualDriverAccessToName(OUString _sImplName) : m_sImplName(std::move(_sImplName)){}

        bool operator()( const DriverAccess& lhs)
        {
            return lhs.sImplementationName == m_sImplName;
        }
    };
}

OSDBCDriverManager::OSDBCDriverManager( const Reference< XComponentContext >& _rxContext )
    :OSDBCDriverManager_Base(m_aMutex)
    ,m_xContext( _rxContext )
    ,m_aEventLogger( _rxContext, "org.openoffice.logging.sdbc.DriverManager" )
    ,m_aDriverConfig(m_xContext)
    ,m_nLoginTimeout(0)
{
    // bootstrap all objects supporting the .sdb.Driver service
    bootstrapDrivers();

    // initialize the drivers order
    initializeDriverPrecedence();
}


OSDBCDriverManager::~OSDBCDriverManager()
{
}

void OSDBCDriverManager::bootstrapDrivers()
{
    Reference< XContentEnumerationAccess > xEnumAccess( m_xContext->getServiceManager(), UNO_QUERY );
    Reference< XEnumeration > xEnumDrivers;
    if (xEnumAccess.is())
        xEnumDrivers = xEnumAccess->createContentEnumeration(SERVICE_SDBC_DRIVER);

    OSL_ENSURE( xEnumDrivers.is(), "OSDBCDriverManager::bootstrapDrivers: no enumeration for the drivers available!" );
    if (!xEnumDrivers.is())
        return;

    Reference< XSingleComponentFactory > xFactory;
    Reference< XServiceInfo > xSI;
    while (xEnumDrivers->hasMoreElements())
    {
        xFactory.set(xEnumDrivers->nextElement(), css::uno::UNO_QUERY);
        OSL_ENSURE( xFactory.is(), "OSDBCDriverManager::bootstrapDrivers: no factory extracted" );

        if ( xFactory.is() )
        {
            // we got a factory for the driver
            DriverAccess aDriverDescriptor;
            bool bValidDescriptor = false;

            // can it tell us something about the implementation name?
            xSI.set(xFactory, css::uno::UNO_QUERY);
            if ( xSI.is() )
            {   // yes -> no need to load the driver immediately (load it later when needed)
                aDriverDescriptor.sImplementationName = xSI->getImplementationName();
                aDriverDescriptor.xComponentFactory = xFactory;
                bValidDescriptor = true;

                m_aEventLogger.log( LogLevel::CONFIG,
                    "found SDBC driver $1$, no need to load it",
                    aDriverDescriptor.sImplementationName
                );
            }
            else
            {
                // no -> create the driver
                Reference< XDriver > xDriver( xFactory->createInstanceWithContext( m_xContext ), UNO_QUERY );
                OSL_ENSURE( xDriver.is(), "OSDBCDriverManager::bootstrapDrivers: a driver which is no driver?!" );

                if ( xDriver.is() )
                {
                    aDriverDescriptor.xDriver = xDriver;
                    // and obtain its implementation name
                    xSI.set(xDriver, css::uno::UNO_QUERY);
                    OSL_ENSURE( xSI.is(), "OSDBCDriverManager::bootstrapDrivers: a driver without service info?" );
                    if ( xSI.is() )
                    {
                        aDriverDescriptor.sImplementationName = xSI->getImplementationName();
                        bValidDescriptor = true;

                        m_aEventLogger.log( LogLevel::CONFIG,
                            "found SDBC driver $1$, needed to load it",
                            aDriverDescriptor.sImplementationName
                        );
                    }
                }
            }

            if ( bValidDescriptor )
            {
                m_aDriversBS.push_back( aDriverDescriptor );
            }
        }
    }
}


void OSDBCDriverManager::initializeDriverPrecedence()
{
#if !ENABLE_FUZZERS
    if ( m_aDriversBS.empty() )
        // nothing to do
        return;

    try
    {
        // get the precedence of the drivers from the configuration
        Sequence< OUString > aDriverOrder = officecfg::Office::DataAccess::DriverManager::DriverPrecedence::get();
        if ( 0 == aDriverOrder.getLength() )
            // nothing to do
            return;

        // aDriverOrder now is the list of driver implementation names in the order they should be used

        if ( m_aEventLogger.isLoggable( LogLevel::CONFIG ) )
        {
            sal_Int32 nOrderedCount = aDriverOrder.getLength();
            for ( sal_Int32 i=0; i<nOrderedCount; ++i )
            m_aEventLogger.log( LogLevel::CONFIG,
                "configuration's driver order: driver $1$ of $2$: $3$",
                static_cast<sal_Int32>(i + 1), nOrderedCount, aDriverOrder[i]
            );
        }

        // sort our bootstrapped drivers
        std::sort( m_aDriversBS.begin(), m_aDriversBS.end(), CompareDriverAccessByName() );

        // the first driver for which there is no preference
        DriverAccessArray::iterator aNoPrefDriversStart = m_aDriversBS.begin();
            // at the moment this is the first of all drivers we know

        // loop through the names in the precedence order
        for (const OUString& rDriverOrder : aDriverOrder)
        {
            if (aNoPrefDriversStart == m_aDriversBS.end())
                break;

            DriverAccess driver_order;
            driver_order.sImplementationName = rDriverOrder;

            // look for the impl name in the DriverAccess array
            std::pair< DriverAccessArray::iterator, DriverAccessArray::iterator > aPos =
                std::equal_range( aNoPrefDriversStart, m_aDriversBS.end(), driver_order, CompareDriverAccessByName() );

            if ( aPos.first != aPos.second )
            {   // we have a DriverAccess with this impl name

                OSL_ENSURE( std::distance( aPos.first, aPos.second ) == 1,
                    "OSDBCDriverManager::initializeDriverPrecedence: more than one driver with this impl name? How this?" );
                // move the DriverAccess pointed to by aPos.first to the position pointed to by aNoPrefDriversStart

                if ( aPos.first != aNoPrefDriversStart )
                {   // if this does not hold, the DriverAccess already has the correct position

                    // rotate the range [aNoPrefDriversStart, aPos.second) right 1 element
                    std::rotate( aNoPrefDriversStart, aPos.second - 1, aPos.second );
                }

                // next round we start searching and pos right
                ++aNoPrefDriversStart;
            }
        }
    }
    catch (Exception&)
    {
        TOOLS_WARN_EXCEPTION( "connectivity.hsqldb", "OSDBCDriverManager::initializeDriverPrecedence: caught an exception while sorting the drivers!");
    }
#endif
}


Reference< XConnection > SAL_CALL OSDBCDriverManager::getConnection( const OUString& _rURL )
{
    MutexGuard aGuard(m_aMutex);

    m_aEventLogger.log( LogLevel::INFO,
        "connection requested for URL $1$",
        _rURL
    );

    Reference< XConnection > xConnection;
    Reference< XDriver > xDriver = implGetDriverForURL(_rURL);
    if (xDriver.is())
    {
        // TODO : handle the login timeout
        xConnection = xDriver->connect(_rURL, Sequence< PropertyValue >());
        // may throw an exception
        m_aEventLogger.log( LogLevel::INFO,
            "connection retrieved for URL $1$",
            _rURL
        );
    }

    return xConnection;
}


Reference< XConnection > SAL_CALL OSDBCDriverManager::getConnectionWithInfo( const OUString& _rURL, const Sequence< PropertyValue >& _rInfo )
{
    MutexGuard aGuard(m_aMutex);

    m_aEventLogger.log( LogLevel::INFO,
        "connection with info requested for URL $1$",
        _rURL
    );

    Reference< XConnection > xConnection;
    Reference< XDriver > xDriver = implGetDriverForURL(_rURL);
    if (xDriver.is())
    {
        // TODO : handle the login timeout
        xConnection = xDriver->connect(_rURL, _rInfo);
        // may throw an exception
        m_aEventLogger.log( LogLevel::INFO,
            "connection with info retrieved for URL $1$",
            _rURL
        );
    }

    return xConnection;
}


void SAL_CALL OSDBCDriverManager::setLoginTimeout( sal_Int32 seconds )
{
    MutexGuard aGuard(m_aMutex);
    m_nLoginTimeout = seconds;
}


sal_Int32 SAL_CALL OSDBCDriverManager::getLoginTimeout(  )
{
    MutexGuard aGuard(m_aMutex);
    return m_nLoginTimeout;
}


Reference< XEnumeration > SAL_CALL OSDBCDriverManager::createEnumeration(  )
{
    MutexGuard aGuard(m_aMutex);

    ODriverEnumeration::DriverArray aDrivers;

    // ensure that all our bootstrapped drivers are instantiated
    std::for_each( m_aDriversBS.begin(), m_aDriversBS.end(), EnsureDriver( m_xContext ) );

    // copy the bootstrapped drivers
    std::transform(
        m_aDriversBS.begin(),               // "copy from" start
        m_aDriversBS.end(),                 // "copy from" end
        std::back_inserter( aDrivers ),   // insert into
        ExtractDriverFromAccess()           // transformation to apply (extract a driver from a driver access)
    );

    // append the runtime drivers
    std::transform(
        m_aDriversRT.begin(),                   // "copy from" start
        m_aDriversRT.end(),                     // "copy from" end
        std::back_inserter( aDrivers ),       // insert into
        ExtractDriverFromCollectionElement()    // transformation to apply (extract a driver from a driver access)
    );

    return new ODriverEnumeration( std::move(aDrivers) );
}


css::uno::Type SAL_CALL OSDBCDriverManager::getElementType(  )
{
    return cppu::UnoType<XDriver>::get();
}


sal_Bool SAL_CALL OSDBCDriverManager::hasElements(  )
{
    MutexGuard aGuard(m_aMutex);
    return !(m_aDriversBS.empty() && m_aDriversRT.empty());
}


OUString SAL_CALL OSDBCDriverManager::getImplementationName(  )
{
    return u"com.sun.star.comp.sdbc.OSDBCDriverManager"_ustr;
}

sal_Bool SAL_CALL OSDBCDriverManager::supportsService( const OUString& _rServiceName )
{
    return cppu::supportsService(this, _rServiceName);
}


Sequence< OUString > SAL_CALL OSDBCDriverManager::getSupportedServiceNames(  )
{
    return { u"com.sun.star.sdbc.DriverManager"_ustr };
}


Reference< XInterface > SAL_CALL OSDBCDriverManager::getRegisteredObject( const OUString& _rName )
{
    MutexGuard aGuard(m_aMutex);
    DriverCollection::const_iterator aSearch = m_aDriversRT.find(_rName);
    if (aSearch == m_aDriversRT.end())
        throw NoSuchElementException();

    return aSearch->second;
}


void SAL_CALL OSDBCDriverManager::registerObject( const OUString& _rName, const Reference< XInterface >& _rxObject )
{
    MutexGuard aGuard(m_aMutex);

    m_aEventLogger.log( LogLevel::INFO,
        "attempt to register new driver for name $1$",
        _rName
    );

    DriverCollection::const_iterator aSearch = m_aDriversRT.find(_rName);
    if (aSearch != m_aDriversRT.end())
        throw ElementExistException();
    Reference< XDriver > xNewDriver(_rxObject, UNO_QUERY);
    if (!xNewDriver.is())
        throw IllegalArgumentException();

    m_aDriversRT.emplace(_rName, xNewDriver);

    m_aEventLogger.log( LogLevel::INFO,
        "new driver registered for name $1$",
        _rName
    );
}


void SAL_CALL OSDBCDriverManager::revokeObject( const OUString& _rName )
{
    MutexGuard aGuard(m_aMutex);

    m_aEventLogger.log( LogLevel::INFO,
        "attempt to revoke driver for name $1$",
        _rName
    );

    DriverCollection::iterator aSearch = m_aDriversRT.find(_rName);
    if (aSearch == m_aDriversRT.end())
        throw NoSuchElementException();

    m_aDriversRT.erase(aSearch); // we already have the iterator so we could use it

    m_aEventLogger.log( LogLevel::INFO,
        "driver revoked for name $1$",
        _rName
    );
}


Reference< XDriver > SAL_CALL OSDBCDriverManager::getDriverByURL( const OUString& _rURL )
{
    m_aEventLogger.log( LogLevel::INFO,
        "driver requested for URL $1$",
        _rURL
    );

    Reference< XDriver > xDriver( implGetDriverForURL( _rURL ) );

    if ( xDriver.is() )
        m_aEventLogger.log( LogLevel::INFO,
            "driver obtained for URL $1$",
            _rURL
        );

    return xDriver;
}


Reference< XDriver > OSDBCDriverManager::implGetDriverForURL(const OUString& _rURL)
{
    Reference< XDriver > xReturn;

    {
        const OUString sDriverFactoryName = m_aDriverConfig.getDriverFactoryName(_rURL);

        DriverAccessArray::const_iterator aFind = std::find_if(m_aDriversBS.begin(), m_aDriversBS.end(),
                                                               EqualDriverAccessToName(sDriverFactoryName));
        if ( aFind == m_aDriversBS.end() )
        {
            // search all bootstrapped drivers
            aFind = std::find_if(
                m_aDriversBS.begin(),       // begin of search range
                m_aDriversBS.end(),         // end of search range
                [&_rURL, this] (const DriverAccessArray::value_type& driverAccess) {
                    // extract the driver from the access, then ask the resulting driver for acceptance
#if defined __GNUC__ && !defined __clang__ && __GNUC__ >= 13 && __GNUC__ <= 15
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-reference"
#endif
                    const DriverAccess& ensuredAccess = EnsureDriver(m_xContext)(driverAccess);
#if defined __GNUC__ && !defined __clang__ && __GNUC__ >= 13 && __GNUC__ <= 15
#pragma GCC diagnostic pop
#endif
                    const Reference<XDriver> driver = ExtractDriverFromAccess()(ensuredAccess);
                    return AcceptsURL(_rURL, driver);
                });
        } // if ( m_aDriversBS.find(sDriverFactoryName ) == m_aDriversBS.end() )
        else
        {
            EnsureDriver aEnsure( m_xContext );
            aEnsure(*aFind);
        }

        // found something?
        if ( m_aDriversBS.end() != aFind && aFind->xDriver.is() && aFind->xDriver->acceptsURL(_rURL) )
            xReturn = aFind->xDriver;
    }

    if ( !xReturn.is() )
    {
        // no -> search the runtime drivers
        DriverCollection::const_iterator aPos = std::find_if(
            m_aDriversRT.begin(),       // begin of search range
            m_aDriversRT.end(),         // end of search range
            [&_rURL] (const DriverCollection::value_type& element) {
                // extract the driver from the collection element, then ask the resulting driver for acceptance
                const Reference<XDriver> driver = ExtractDriverFromCollectionElement()(element);
                return AcceptsURL(_rURL, driver);
            });

        if ( m_aDriversRT.end() != aPos )
            xReturn = aPos->second;
    }

    return xReturn;
}

}   // namespace drivermanager

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
connectivity_OSDBCDriverManager_get_implementation(
    css::uno::XComponentContext* context , css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new drivermanager::OSDBCDriverManager(context));
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
