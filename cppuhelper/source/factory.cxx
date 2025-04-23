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

#include <sal/log.hxx>
#include <osl/diagnose.h>
#include <osl/mutex.hxx>
#include <cppuhelper/basemutex.hxx>
#include <cppuhelper/weak.hxx>
#include <cppuhelper/compbase.hxx>
#include <cppuhelper/factory.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <rtl/unload.h>

#include <cppuhelper/propshlp.hxx>
#include <o3tl/string_view.hxx>

#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/lang/XSingleComponentFactory.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/loader/XImplementationLoader.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/uno/XUnloadingPreference.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>

#include <memory>
#include <utility>


using namespace osl;
using namespace com::sun::star;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::loader;
using namespace com::sun::star::registry;

namespace cppu
{

namespace {

class OFactoryComponentHelper
    : public cppu::BaseMutex
    , public WeakComponentImplHelper<
          XServiceInfo,
          XSingleServiceFactory,
          lang::XSingleComponentFactory,
          XUnloadingPreference>
{
public:
    OFactoryComponentHelper(
        const Reference<XMultiServiceFactory > & rServiceManager,
        OUString aImplementationName_,
        ComponentInstantiation pCreateFunction_,
        ComponentFactoryFunc fptr,
        const Sequence< OUString > * pServiceNames_,
        bool bOneInstance_ )
        : WeakComponentImplHelper( m_aMutex )
        , m_bOneInstance( bOneInstance_ )
        , m_xSMgr( rServiceManager )
        , m_pCreateFunction( pCreateFunction_ )
        , m_fptr( fptr )
        , m_aImplementationName(std::move( aImplementationName_ ))
        {
            if( pServiceNames_ )
                m_aServiceNames = *pServiceNames_;
        }

    // XSingleServiceFactory
    Reference<XInterface > SAL_CALL createInstance() override;
    Reference<XInterface > SAL_CALL createInstanceWithArguments( const Sequence<Any>& Arguments ) override;
    // XSingleComponentFactory
    virtual Reference< XInterface > SAL_CALL createInstanceWithContext(
        Reference< XComponentContext > const & xContext ) override;
    virtual Reference< XInterface > SAL_CALL createInstanceWithArgumentsAndContext(
        Sequence< Any > const & rArguments,
        Reference< XComponentContext > const & xContext ) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override;
    sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
    Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XTypeProvider
    virtual Sequence< Type > SAL_CALL getTypes() override;

    // XUnloadingPreference
    virtual sal_Bool SAL_CALL releaseOnNotification() override;

    // WeakComponentImplHelper
    void SAL_CALL disposing() override;

private:
    css::uno::Reference<css::uno::XInterface> createInstanceWithArgumentsEveryTime(
        css::uno::Sequence<css::uno::Any> const & rArguments,
        css::uno::Reference<css::uno::XComponentContext> const & xContext);

    Reference<XInterface >  m_xTheInstance;
    bool                m_bOneInstance;
protected:
    // needed for implementing XUnloadingPreference in inheriting classes
    bool isOneInstance() const {return m_bOneInstance;}
    bool isInstance() const {return m_xTheInstance.is();}

    /**
     * Create an instance specified by the factory. The one instance logic is implemented
     * in the createInstance and createInstanceWithArguments methods.
     * @return the newly created instance. Do not return a previous (one instance) instance.
     * @throw css::uno::Exception
     * @throw css::uno::RuntimeException
     */
    virtual Reference<XInterface >  createInstanceEveryTime(
        Reference< XComponentContext > const & xContext );

    Reference<XMultiServiceFactory > m_xSMgr;
    ComponentInstantiation           m_pCreateFunction;
    ComponentFactoryFunc             m_fptr;
    Sequence< OUString >             m_aServiceNames;
    OUString                         m_aImplementationName;
};

}

// XTypeProvider
Sequence< Type > OFactoryComponentHelper::getTypes()
{
    Type ar[ 4 ];
    ar[ 0 ] = cppu::UnoType<XSingleServiceFactory>::get();
    ar[ 1 ] = cppu::UnoType<XServiceInfo>::get();
    ar[ 2 ] = cppu::UnoType<XUnloadingPreference>::get();

    if (m_fptr)
        ar[ 3 ] = cppu::UnoType<XSingleComponentFactory>::get();

    return Sequence< Type >( ar, m_fptr ? 4 : 3 );
}

// OFactoryComponentHelper
Reference<XInterface > OFactoryComponentHelper::createInstanceEveryTime(
    Reference< XComponentContext > const & xContext )
{
    if (m_fptr)
    {
        return (*m_fptr)( xContext );
    }
    if( m_pCreateFunction )
    {
        if (xContext.is())
        {
            Reference< lang::XMultiServiceFactory > xContextMgr(
                xContext->getServiceManager(), UNO_QUERY );
            if (xContextMgr.is())
                return (*m_pCreateFunction)( xContextMgr );
        }
        return (*m_pCreateFunction)( m_xSMgr );
    }
    return Reference< XInterface >();
}

// XSingleServiceFactory
Reference<XInterface > OFactoryComponentHelper::createInstance()
{
    if ( m_bOneInstance )
    {
        if( !m_xTheInstance.is() )
        {
            MutexGuard aGuard( m_aMutex );
            if( !m_xTheInstance.is() )
                m_xTheInstance = createInstanceEveryTime( Reference< XComponentContext >() );
        }
        return m_xTheInstance;
    }
    return createInstanceEveryTime( Reference< XComponentContext >() );
}

Reference<XInterface > OFactoryComponentHelper::createInstanceWithArguments(
    const Sequence<Any>& Arguments )
{
    if ( m_bOneInstance )
    {
        if( !m_xTheInstance.is() )
        {
            MutexGuard aGuard( m_aMutex );
//          OSL_ENSURE( !xTheInstance.is(), "### arguments will be ignored!" );
            if( !m_xTheInstance.is() )
                m_xTheInstance = createInstanceWithArgumentsEveryTime(
                    Arguments, Reference< XComponentContext >() );
        }
        return m_xTheInstance;
    }
    return createInstanceWithArgumentsEveryTime( Arguments, Reference< XComponentContext >() );
}

// XSingleComponentFactory

Reference< XInterface > OFactoryComponentHelper::createInstanceWithContext(
    Reference< XComponentContext > const & xContext )
{
    if ( m_bOneInstance )
    {
        if( !m_xTheInstance.is() )
        {
            MutexGuard aGuard( m_aMutex );
//          OSL_ENSURE( !xTheInstance.is(), "### context will be ignored!" );
            if( !m_xTheInstance.is() )
                m_xTheInstance = createInstanceEveryTime( xContext );
        }
        return m_xTheInstance;
    }
    return createInstanceEveryTime( xContext );
}

Reference< XInterface > OFactoryComponentHelper::createInstanceWithArgumentsAndContext(
    Sequence< Any > const & rArguments,
    Reference< XComponentContext > const & xContext )
{
    if ( m_bOneInstance )
    {
        if( !m_xTheInstance.is() )
        {
            MutexGuard aGuard( m_aMutex );
//          OSL_ENSURE( !xTheInstance.is(), "### context and arguments will be ignored!" );
            if( !m_xTheInstance.is() )
                m_xTheInstance = createInstanceWithArgumentsEveryTime( rArguments, xContext );
        }
        return m_xTheInstance;
    }
    return createInstanceWithArgumentsEveryTime( rArguments, xContext );
}

css::uno::Reference<css::uno::XInterface>
OFactoryComponentHelper::createInstanceWithArgumentsEveryTime(
    css::uno::Sequence<css::uno::Any> const & rArguments,
    css::uno::Reference<css::uno::XComponentContext> const & xContext)
{
    Reference< XInterface > xRet( createInstanceEveryTime( xContext ) );

    Reference< lang::XInitialization > xInit( xRet, UNO_QUERY );
    // always call initialize, even if there are no arguments. #i63511#
    if (xInit.is())
    {
        xInit->initialize( rArguments );
    }
    else
    {
        if ( rArguments.hasElements() )
        {
            // dispose the here created UNO object before throwing out exception
            // to avoid risk of memory leaks #i113722#
            Reference<XComponent> xComp( xRet, UNO_QUERY );
            if (xComp.is())
                xComp->dispose();

            throw lang::IllegalArgumentException(
                u"cannot pass arguments to component => no XInitialization implemented!"_ustr,
                Reference< XInterface >(), 0 );
        }
    }

    return xRet;
}


// WeakComponentImplHelper
void OFactoryComponentHelper::disposing()
{
    Reference<XInterface > x;
    {
        // do not delete in the guard section
        MutexGuard aGuard( m_aMutex );
        x = m_xTheInstance;
        m_xTheInstance.clear();
    }
    // if it is a component call dispose at the component
    Reference<XComponent > xComp( x, UNO_QUERY );
    if( xComp.is() )
        xComp->dispose();
}

// XServiceInfo
OUString OFactoryComponentHelper::getImplementationName()
{
    return m_aImplementationName;
}

// XServiceInfo
sal_Bool OFactoryComponentHelper::supportsService(
    const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}

// XServiceInfo
Sequence< OUString > OFactoryComponentHelper::getSupportedServiceNames()
{
    return m_aServiceNames;
}

// XUnloadingPreference
// This class is used for single factories, component factories and
// one-instance factories. Depending on the usage this function has
// to return different values.
// one-instance factory: sal_False
// single factory: sal_True
// component factory: sal_True
sal_Bool SAL_CALL OFactoryComponentHelper::releaseOnNotification()
{
    if (m_bOneInstance)
        return false;
    return true;
}

namespace {

class ORegistryFactoryHelper : public OFactoryComponentHelper,
                               public OPropertySetHelper

{
public:
    ORegistryFactoryHelper(
        const Reference<XMultiServiceFactory > & rServiceManager,
        const OUString & rImplementationName_,
        const Reference<XRegistryKey > & xImplementationKey_,
        bool bOneInstance_ )
            : OFactoryComponentHelper(
                rServiceManager, rImplementationName_, nullptr, nullptr, nullptr, bOneInstance_ ),
              OPropertySetHelper( WeakComponentImplHelper::rBHelper ),
              xImplementationKey( xImplementationKey_ )
        {}

    // XInterface
    virtual Any SAL_CALL queryInterface( Type const & type ) override;
    virtual void SAL_CALL acquire() noexcept override;
    virtual void SAL_CALL release() noexcept override;
    // XTypeProvider
    virtual Sequence< Type > SAL_CALL getTypes() override;
    // XPropertySet
    virtual Reference< beans::XPropertySetInfo > SAL_CALL getPropertySetInfo() override;

    // OPropertySetHelper
    virtual IPropertyArrayHelper & SAL_CALL getInfoHelper() override;
    virtual sal_Bool SAL_CALL convertFastPropertyValue(
        Any & rConvertedValue, Any & rOldValue,
        sal_Int32 nHandle, Any const & rValue ) override;
    virtual void SAL_CALL setFastPropertyValue_NoBroadcast(
        sal_Int32 nHandle, Any const & rValue ) override;
    using OPropertySetHelper::getFastPropertyValue;
    virtual void SAL_CALL getFastPropertyValue(
        Any & rValue, sal_Int32 nHandle ) const override;

    // OFactoryComponentHelper
    Reference<XInterface > createInstanceEveryTime(
        Reference< XComponentContext > const & xContext ) override;

    // XSingleServiceFactory
    Reference<XInterface > SAL_CALL createInstanceWithArguments(const Sequence<Any>& Arguments) override;
    // XSingleComponentFactory
    Reference< XInterface > SAL_CALL createInstanceWithArgumentsAndContext(
        Sequence< Any > const & rArguments,
        Reference< XComponentContext > const & xContext ) override;

    // XServiceInfo
    Sequence< OUString > SAL_CALL getSupportedServiceNames() override;
    // XUnloadingPreference
    sal_Bool SAL_CALL releaseOnNotification() override;


private:
    /// @throws css::uno::Exception
    /// @throws css::uno::RuntimeException
    Reference< XInterface > createModuleFactory();

    /** The registry key of the implementation section */
    Reference<XRegistryKey >    xImplementationKey;
    /** The factory created with the loader. */
    Reference<XSingleComponentFactory > xModuleFactory;
    Reference<XSingleServiceFactory >   xModuleFactoryDepr;
    Reference< beans::XPropertySetInfo > m_xInfo;
    std::unique_ptr< IPropertyArrayHelper > m_property_array_helper;
protected:
    using OPropertySetHelper::getTypes;
};

}

// XInterface

Any SAL_CALL ORegistryFactoryHelper::queryInterface(
    Type const & type )
{
    Any ret( OFactoryComponentHelper::queryInterface( type ) );
    if (ret.hasValue())
        return ret;
    return OPropertySetHelper::queryInterface( type );
}


void ORegistryFactoryHelper::acquire() noexcept
{
    OFactoryComponentHelper::acquire();
}


void ORegistryFactoryHelper::release() noexcept
{
    OFactoryComponentHelper::release();
}

// XTypeProvider

Sequence< Type > ORegistryFactoryHelper::getTypes()
{
    Sequence< Type > types( OFactoryComponentHelper::getTypes() );
    sal_Int32 pos = types.getLength();
    types.realloc( pos + 3 );
    Type * p = types.getArray();
    p[ pos++ ] = cppu::UnoType<beans::XMultiPropertySet>::get();
    p[ pos++ ] = cppu::UnoType<beans::XFastPropertySet>::get();
    p[ pos++ ] = cppu::UnoType<beans::XPropertySet>::get();
    return types;
}

// XPropertySet

Reference< beans::XPropertySetInfo >
ORegistryFactoryHelper::getPropertySetInfo()
{
    ::osl::MutexGuard guard( m_aMutex );
    if (! m_xInfo.is())
        m_xInfo = createPropertySetInfo( getInfoHelper() );
    return m_xInfo;
}

// OPropertySetHelper

IPropertyArrayHelper & ORegistryFactoryHelper::getInfoHelper()
{
    ::osl::MutexGuard guard( m_aMutex );
    if (m_property_array_helper == nullptr)
    {
        beans::Property prop(
            u"ImplementationKey"_ustr /* name */,
            0 /* handle */,
            cppu::UnoType<decltype(xImplementationKey)>::get(),
            beans::PropertyAttribute::READONLY |
            beans::PropertyAttribute::OPTIONAL );
        m_property_array_helper.reset(
            new ::cppu::OPropertyArrayHelper( &prop, 1 ) );
    }
    return *m_property_array_helper;
}


sal_Bool ORegistryFactoryHelper::convertFastPropertyValue(
    Any &, Any &, sal_Int32, Any const & )
{
    OSL_FAIL( "unexpected!" );
    return false;
}


void ORegistryFactoryHelper::setFastPropertyValue_NoBroadcast(
    sal_Int32, Any const & )
{
    throw beans::PropertyVetoException(
        u"unexpected: only readonly properties!"_ustr,
        static_cast< OWeakObject * >(this) );
}


void ORegistryFactoryHelper::getFastPropertyValue(
    Any & rValue, sal_Int32 nHandle ) const
{
    if (nHandle == 0)
    {
        rValue <<= xImplementationKey;
    }
    else
    {
        rValue.clear();
        throw beans::UnknownPropertyException(
            u"unknown property!"_ustr, static_cast< OWeakObject * >(
                const_cast< ORegistryFactoryHelper * >(this) ) );
    }
}

Reference<XInterface > ORegistryFactoryHelper::createInstanceEveryTime(
    Reference< XComponentContext > const & xContext )
{
    if( !xModuleFactory.is() && !xModuleFactoryDepr.is() )
    {
        Reference< XInterface > x( createModuleFactory() );
        if (x.is())
        {
            MutexGuard aGuard( m_aMutex );
            if( !xModuleFactory.is() && !xModuleFactoryDepr.is() )
            {
                xModuleFactory.set( x, UNO_QUERY );
                xModuleFactoryDepr.set( x, UNO_QUERY );
            }
        }
    }
    if( xModuleFactory.is() )
    {
        return xModuleFactory->createInstanceWithContext( xContext );
    }
    if( xModuleFactoryDepr.is() )
    {
        return xModuleFactoryDepr->createInstance();
    }

    return Reference<XInterface >();
}

Reference<XInterface > SAL_CALL ORegistryFactoryHelper::createInstanceWithArguments(
    const Sequence<Any>& Arguments )
{
    if( !xModuleFactory.is() && !xModuleFactoryDepr.is() )
    {
        Reference< XInterface > x( createModuleFactory() );
        if (x.is())
        {
            MutexGuard aGuard( m_aMutex );
            if( !xModuleFactory.is() && !xModuleFactoryDepr.is() )
            {
                xModuleFactory.set( x, UNO_QUERY );
                xModuleFactoryDepr.set( x, UNO_QUERY );
            }
        }
    }
    if( xModuleFactoryDepr.is() )
    {
        return xModuleFactoryDepr->createInstanceWithArguments( Arguments );
    }
    if( xModuleFactory.is() )
    {
        SAL_INFO("cppuhelper", "no context ORegistryFactoryHelper::createInstanceWithArgumentsAndContext()!");
        return xModuleFactory->createInstanceWithArgumentsAndContext( Arguments, Reference< XComponentContext >() );
    }

    return Reference<XInterface >();
}

Reference< XInterface > ORegistryFactoryHelper::createInstanceWithArgumentsAndContext(
    Sequence< Any > const & rArguments,
    Reference< XComponentContext > const & xContext )
{
    if( !xModuleFactory.is() && !xModuleFactoryDepr.is() )
    {
        Reference< XInterface > x( createModuleFactory() );
        if (x.is())
        {
            MutexGuard aGuard( m_aMutex );
            if( !xModuleFactory.is() && !xModuleFactoryDepr.is() )
            {
                xModuleFactory.set( x, UNO_QUERY );
                xModuleFactoryDepr.set( x, UNO_QUERY );
            }
        }
    }
    if( xModuleFactory.is() )
    {
        return xModuleFactory->createInstanceWithArgumentsAndContext( rArguments, xContext );
    }
    if( xModuleFactoryDepr.is() )
    {
        SAL_INFO_IF(xContext.is(), "cppuhelper", "ignoring context calling ORegistryFactoryHelper::createInstanceWithArgumentsAndContext()!");
        return xModuleFactoryDepr->createInstanceWithArguments( rArguments );
    }

    return Reference<XInterface >();
}


Reference< XInterface > ORegistryFactoryHelper::createModuleFactory()
{
    OUString aActivatorUrl;
    OUString aActivatorName;
    OUString aLocation;

    Reference<XRegistryKey > xActivatorKey = xImplementationKey->openKey(
        u"/UNO/ACTIVATOR"_ustr );
    if( xActivatorKey.is() && xActivatorKey->getValueType() == RegistryValueType_ASCII )
    {
        aActivatorUrl = xActivatorKey->getAsciiValue();

        aActivatorName = o3tl::getToken(aActivatorUrl, 0, ':');

        Reference<XRegistryKey > xLocationKey = xImplementationKey->openKey(
            u"/UNO/LOCATION"_ustr );
        if( xLocationKey.is() && xLocationKey->getValueType() == RegistryValueType_ASCII )
            aLocation = xLocationKey->getAsciiValue();
    }
    else
    {
        // old style"url"
        // the location of the program code of the implementation
        Reference<XRegistryKey > xLocationKey = xImplementationKey->openKey(
            u"/UNO/URL"_ustr );
        // is the key of the right type ?
        if( xLocationKey.is() && xLocationKey->getValueType() == RegistryValueType_ASCII )
        {
            // one implementation found -> try to activate
            aLocation = xLocationKey->getAsciiValue();

            // search protocol delimiter
            sal_Int32 nPos = aLocation.indexOf("://");
            if( nPos != -1 )
            {
                aActivatorName = aLocation.subView( 0, nPos );
                if( aActivatorName == u"java" )
                    aActivatorName = u"com.sun.star.loader.Java"_ustr;
                else if( aActivatorName == u"module" )
                    aActivatorName = u"com.sun.star.loader.SharedLibrary"_ustr;
                aLocation = aLocation.copy( nPos + 3 );
            }
        }
    }

    Reference< XInterface > xFactory;
    if( !aActivatorName.isEmpty() )
    {
        Reference<XInterface > x = m_xSMgr->createInstance( aActivatorName );
        Reference<XImplementationLoader > xLoader( x, UNO_QUERY );
        if (xLoader.is())
        {
            xFactory = xLoader->activate( m_aImplementationName, aActivatorUrl, aLocation, xImplementationKey );
        }
    }
    return xFactory;
}

// XServiceInfo
Sequence< OUString > ORegistryFactoryHelper::getSupportedServiceNames()
{
    MutexGuard aGuard( m_aMutex );
    if( !m_aServiceNames.hasElements() )
    {
        // not yet loaded
        try
        {
            Reference<XRegistryKey > xKey = xImplementationKey->openKey( u"UNO/SERVICES"_ustr );

            if (xKey.is())
            {
                // length of prefix. +1 for the '/' at the end
                sal_Int32 nPrefixLen = xKey->getKeyName().getLength() + 1;

                // Full qualified names like "IMPLEMENTATIONS/TEST/UNO/SERVICES/com.sun.star..."
                Sequence<OUString> seqKeys = xKey->getKeyNames();
                for( OUString & key : asNonConstRange(seqKeys) )
                    key = key.copy(nPrefixLen);

                m_aServiceNames = std::move(seqKeys);
            }
        }
        catch (InvalidRegistryException &)
        {
        }
    }
    return m_aServiceNames;
}

sal_Bool SAL_CALL ORegistryFactoryHelper::releaseOnNotification()
{
    bool retVal= true;
    if( isOneInstance() && isInstance())
    {
        retVal= false;
    }
    else if( ! isOneInstance())
    {
        // try to delegate
        if( xModuleFactory.is())
        {
            Reference<XUnloadingPreference> xunloading( xModuleFactory, UNO_QUERY);
            if( xunloading.is())
                retVal= xunloading->releaseOnNotification();
        }
        else if( xModuleFactoryDepr.is())
        {
            Reference<XUnloadingPreference> xunloading( xModuleFactoryDepr, UNO_QUERY);
            if( xunloading.is())
                retVal= xunloading->releaseOnNotification();
        }
    }
    return retVal;
}

namespace {

class OFactoryProxyHelper : public WeakImplHelper< XServiceInfo, XSingleServiceFactory,
                                                    XUnloadingPreference >
{
    Reference<XSingleServiceFactory >   xFactory;

public:

    explicit OFactoryProxyHelper( const Reference<XSingleServiceFactory > & rFactory )
        : xFactory( rFactory )
        {}

    // XSingleServiceFactory
    Reference<XInterface > SAL_CALL createInstance() override;
    Reference<XInterface > SAL_CALL createInstanceWithArguments(const Sequence<Any>& Arguments) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override;
    sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
    Sequence< OUString > SAL_CALL getSupportedServiceNames() override;
    //XUnloadingPreference
    sal_Bool SAL_CALL releaseOnNotification() override;

};

}

// XSingleServiceFactory
Reference<XInterface > OFactoryProxyHelper::createInstance()
{
    return xFactory->createInstance();
}

// XSingleServiceFactory
Reference<XInterface > OFactoryProxyHelper::createInstanceWithArguments
(
    const Sequence<Any>& Arguments
)
{
    return xFactory->createInstanceWithArguments( Arguments );
}

// XServiceInfo
OUString OFactoryProxyHelper::getImplementationName()
{
    Reference<XServiceInfo > xInfo( xFactory, UNO_QUERY  );
    if( xInfo.is() )
        return xInfo->getImplementationName();
    return OUString();
}

// XServiceInfo
sal_Bool OFactoryProxyHelper::supportsService(const OUString& ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

// XServiceInfo
Sequence< OUString > OFactoryProxyHelper::getSupportedServiceNames()
{
    Reference<XServiceInfo > xInfo( xFactory, UNO_QUERY  );
    if( xInfo.is() )
        return xInfo->getSupportedServiceNames();
    return Sequence< OUString >();
}

sal_Bool SAL_CALL OFactoryProxyHelper::releaseOnNotification()
{

    Reference<XUnloadingPreference> pref( xFactory, UNO_QUERY);
    if( pref.is())
        return pref->releaseOnNotification();
    return true;
}

// global function
Reference<XSingleServiceFactory > SAL_CALL createSingleFactory(
    const Reference<XMultiServiceFactory > & rServiceManager,
    const OUString & rImplementationName,
    ComponentInstantiation pCreateFunction,
    const Sequence< OUString > & rServiceNames,
    rtl_ModuleCount * )
{
    return new OFactoryComponentHelper(
        rServiceManager, rImplementationName, pCreateFunction, nullptr, &rServiceNames, false );
}

// global function
Reference<XSingleServiceFactory > SAL_CALL createFactoryProxy(
    SAL_UNUSED_PARAMETER const Reference<XMultiServiceFactory > &,
    const Reference<XSingleServiceFactory > & rFactory )
{
    return new OFactoryProxyHelper( rFactory );
}

// global function
Reference<XSingleServiceFactory > SAL_CALL createOneInstanceFactory(
    const Reference<XMultiServiceFactory > & rServiceManager,
    const OUString & rImplementationName,
    ComponentInstantiation pCreateFunction,
    const Sequence< OUString > & rServiceNames,
    rtl_ModuleCount * )
{
    return new OFactoryComponentHelper(
        rServiceManager, rImplementationName, pCreateFunction, nullptr, &rServiceNames, true );
}

// global function
Reference<XSingleServiceFactory > SAL_CALL createSingleRegistryFactory(
    const Reference<XMultiServiceFactory > & rServiceManager,
    const OUString & rImplementationName,
    const Reference<XRegistryKey > & rImplementationKey )
{
    return new ORegistryFactoryHelper(
        rServiceManager, rImplementationName, rImplementationKey, false );
}

// global function
Reference<XSingleServiceFactory > SAL_CALL createOneInstanceRegistryFactory(
    const Reference<XMultiServiceFactory > & rServiceManager,
    const OUString & rImplementationName,
    const Reference<XRegistryKey > & rImplementationKey )
{
    return new ORegistryFactoryHelper(
        rServiceManager, rImplementationName, rImplementationKey, true );
}


Reference< lang::XSingleComponentFactory > SAL_CALL createSingleComponentFactory(
    ComponentFactoryFunc fptr,
    OUString const & rImplementationName,
    Sequence< OUString > const & rServiceNames,
    rtl_ModuleCount *)
{
    return new OFactoryComponentHelper(
        Reference< XMultiServiceFactory >(), rImplementationName, nullptr, fptr, &rServiceNames, false );
}

Reference< lang::XSingleComponentFactory > SAL_CALL createOneInstanceComponentFactory(
    ComponentFactoryFunc fptr,
    OUString const & rImplementationName,
    Sequence< OUString > const & rServiceNames,
    rtl_ModuleCount *)
{
    return new OFactoryComponentHelper(
        Reference< XMultiServiceFactory >(), rImplementationName, nullptr, fptr, &rServiceNames, true );
}

}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
