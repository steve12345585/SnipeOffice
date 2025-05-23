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
#include <com/sun/star/lang/ServiceNotRegisteredException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/beans/IntrospectionException.hpp>
#include <com/sun/star/beans/theIntrospection.hpp>
#include <com/sun/star/beans/MethodConcept.hpp>
#include <com/sun/star/script/CannotConvertException.hpp>
#include <com/sun/star/script/CannotCreateAdapterException.hpp>
#include <com/sun/star/script/XEventAttacher2.hpp>
#include <com/sun/star/script/Converter.hpp>
#include <com/sun/star/script/XAllListener.hpp>
#include <com/sun/star/script/InvocationAdapterFactory.hpp>
#include <com/sun/star/reflection/theCoreReflection.hpp>
#include <com/sun/star/reflection/XIdlReflection.hpp>

// InvocationToAllListenerMapper
#include <com/sun/star/script/XInvocation.hpp>
#include <cppuhelper/exc_hlp.hxx>
#include <cppuhelper/weak.hxx>
#include <cppuhelper/factory.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>

#include <mutex>
#include <utility>

namespace com::sun::star::lang { class XMultiServiceFactory; }

using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::script;
using namespace com::sun::star::reflection;
using namespace cppu;


namespace comp_EventAttacher {


//  class InvocationToAllListenerMapper
//  helper class to map XInvocation to XAllListener

namespace {

class InvocationToAllListenerMapper : public WeakImplHelper< XInvocation >
{
public:
    InvocationToAllListenerMapper( const Reference< XIdlClass >& ListenerType,
        const Reference< XAllListener >& AllListener, Any Helper );

    // XInvocation
    virtual Reference< XIntrospectionAccess > SAL_CALL getIntrospection() override;
    virtual Any SAL_CALL invoke(const OUString& FunctionName, const Sequence< Any >& Params, Sequence< sal_Int16 >& OutParamIndex, Sequence< Any >& OutParam) override;
    virtual void SAL_CALL setValue(const OUString& PropertyName, const Any& Value) override;
    virtual Any SAL_CALL getValue(const OUString& PropertyName) override;
    virtual sal_Bool SAL_CALL hasMethod(const OUString& Name) override;
    virtual sal_Bool SAL_CALL hasProperty(const OUString& Name) override;

private:
    Reference< XAllListener >    m_xAllListener;
    Reference< XIdlClass >       m_xListenerType;
    Any                          m_Helper;
};

}

// Function to replace AllListenerAdapterService::createAllListerAdapter
static Reference< XInterface > createAllListenerAdapter
(
    const Reference< XInvocationAdapterFactory2 >& xInvocationAdapterFactory,
    const Reference< XIdlClass >& xListenerType,
    const Reference< XAllListener >& xListener,
    const Any& Helper
)
{
    Reference< XInterface > xAdapter;
    if( xInvocationAdapterFactory.is() && xListenerType.is() && xListener.is() )
    {
        Reference< XInvocation > xInvocationToAllListenerMapper =
            new InvocationToAllListenerMapper(xListenerType, xListener, Helper);
        Type aListenerType( xListenerType->getTypeClass(), xListenerType->getName());
        Sequence<Type> arg2 { aListenerType };
        xAdapter = xInvocationAdapterFactory->createAdapter( xInvocationToAllListenerMapper, arg2 );
    }
    return xAdapter;
}


// InvocationToAllListenerMapper
InvocationToAllListenerMapper::InvocationToAllListenerMapper
    ( const Reference< XIdlClass >& ListenerType, const Reference< XAllListener >& AllListener, Any Helper )
        : m_xAllListener( AllListener )
        , m_xListenerType( ListenerType )
        , m_Helper(std::move( Helper ))
{
}


Reference< XIntrospectionAccess > SAL_CALL InvocationToAllListenerMapper::getIntrospection()
{
    return Reference< XIntrospectionAccess >();
}


Any SAL_CALL InvocationToAllListenerMapper::invoke(const OUString& FunctionName, const Sequence< Any >& Params,
    Sequence< sal_Int16 >& , Sequence< Any >& )
{
    Any aRet;

    // Check if to firing or approveFiring has to be called
    Reference< XIdlMethod > xMethod = m_xListenerType->getMethod( FunctionName );
    bool bApproveFiring = false;
    if( !xMethod.is() )
        return aRet;
    Reference< XIdlClass > xReturnType = xMethod->getReturnType();
    Sequence< Reference< XIdlClass > > aExceptionSeq = xMethod->getExceptionTypes();
    if( ( xReturnType.is() && xReturnType->getTypeClass() != TypeClass_VOID ) ||
        aExceptionSeq.hasElements() )
    {
        bApproveFiring = true;
    }
    else
    {
        Sequence< ParamInfo > aParamSeq = xMethod->getParameterInfos();
        sal_uInt32 nParamCount = aParamSeq.getLength();
        if( nParamCount > 1 )
        {
            const ParamInfo* pInfo = aParamSeq.getConstArray();
            for( sal_uInt32 i = 0 ; i < nParamCount ; i++ )
            {
                if( pInfo[ i ].aMode != ParamMode_IN )
                {
                    bApproveFiring = true;
                    break;
                }
            }
        }
    }

    AllEventObject aAllEvent;
    aAllEvent.Source = getXWeak();
    aAllEvent.Helper = m_Helper;
    aAllEvent.ListenerType = Type(m_xListenerType->getTypeClass(), m_xListenerType->getName());
    aAllEvent.MethodName = FunctionName;
    aAllEvent.Arguments = Params;
    if( bApproveFiring )
        aRet = m_xAllListener->approveFiring( aAllEvent );
    else
        m_xAllListener->firing( aAllEvent );
    return aRet;
}


void SAL_CALL InvocationToAllListenerMapper::setValue(const OUString& , const Any& )
{
}


Any SAL_CALL InvocationToAllListenerMapper::getValue(const OUString& )
{
    return Any();
}


sal_Bool SAL_CALL InvocationToAllListenerMapper::hasMethod(const OUString& Name)
{
    Reference< XIdlMethod > xMethod = m_xListenerType->getMethod( Name );
    return xMethod.is();
}


sal_Bool SAL_CALL InvocationToAllListenerMapper::hasProperty(const OUString& Name)
{
    Reference< XIdlField > xField = m_xListenerType->getField( Name );
    return xField.is();
}


//  class EventAttacherImpl
//  represents an implementation of the EventAttacher service

namespace {

class EventAttacherImpl : public WeakImplHelper < XEventAttacher2, XInitialization, XServiceInfo >
{
public:
    explicit EventAttacherImpl( const Reference< XComponentContext >& );

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName(  ) override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;

    // XInitialization
    virtual void SAL_CALL initialize( const Sequence< Any >& aArguments ) override;

    // methods of XEventAttacher
    virtual Reference< XEventListener > SAL_CALL attachListener(const Reference< XInterface >& xObject,
            const Reference< XAllListener >& AllListener, const Any& Helper,
            const OUString& ListenerType, const OUString& AddListenerParam) override;
    virtual Reference< XEventListener > SAL_CALL attachSingleEventListener(const Reference< XInterface >& xObject,
            const Reference< XAllListener >& AllListener, const Any& Helper,
            const OUString& ListenerType, const OUString& AddListenerParam,
            const OUString& EventMethod) override;
    virtual void SAL_CALL removeListener(const Reference< XInterface >& xObject,
            const OUString& ListenerType, const OUString& AddListenerParam,
            const Reference< XEventListener >& aToRemoveListener) override;

    // XEventAttacher2
    virtual Sequence< Reference<XEventListener> > SAL_CALL attachMultipleEventListeners(
        const Reference<XInterface>& xObject, const Sequence<css::script::EventListener>& aListeners ) override;

    // used by FilterAllListener_Impl
    /// @throws Exception
    Reference< XTypeConverter > getConverter();

    friend class FilterAllListenerImpl;

private:
    static Reference<XEventListener> attachListenerForTarget(
        const Reference<XIntrospectionAccess>& xAccess,
        const Reference<XInvocationAdapterFactory2>& xInvocationAdapterFactory,
        const Reference<XAllListener>& xAllListener,
        const Any& aObject,
        const Any& aHelper,
        const OUString& aListenerType,
        const OUString& aAddListenerParam );

    Sequence< Reference<XEventListener> > attachListeners(
        const Reference<XInterface>& xObject,
        const Sequence< Reference<XAllListener> >& AllListeners,
        const Sequence<css::script::EventListener>& aListeners );

private:
    std::mutex                               m_aMutex;
    Reference< XComponentContext >      m_xContext;

    // Save Services
    Reference< XIntrospection >             m_xIntrospection;
    Reference< XIdlReflection >             m_xReflection;
    Reference< XTypeConverter >             m_xConverter;
    Reference< XInvocationAdapterFactory2 >  m_xInvocationAdapterFactory;

    // needed services
    /// @throws Exception
    Reference< XIntrospection >             getIntrospection();
    /// @throws Exception
    Reference< XIdlReflection >             getReflection();
    /// @throws Exception
    Reference< XInvocationAdapterFactory2 >  getInvocationAdapterService();
};

}

EventAttacherImpl::EventAttacherImpl( const Reference< XComponentContext >& rxContext )
    : m_xContext( rxContext )
{
}

/// @throws Exception
OUString SAL_CALL EventAttacherImpl::getImplementationName(  )
{
    return u"com.sun.star.comp.EventAttacher"_ustr;
}

sal_Bool SAL_CALL EventAttacherImpl::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}

Sequence<OUString> SAL_CALL EventAttacherImpl::getSupportedServiceNames(  )
{
    return { u"com.sun.star.script.EventAttacher"_ustr };
}

void SAL_CALL EventAttacherImpl::initialize(const Sequence< Any >& Arguments)
{
    // get services from the argument list
    for( const Any& arg : Arguments )
    {
        if( arg.getValueTypeClass() != TypeClass_INTERFACE )
            throw IllegalArgumentException();

        // InvocationAdapter service ?
        Reference< XInvocationAdapterFactory2 > xALAS;
        arg >>= xALAS;
        if( xALAS.is() )
        {
            std::scoped_lock aGuard( m_aMutex );
            m_xInvocationAdapterFactory = xALAS;
        }
        // Introspection service ?
        Reference< XIntrospection > xI;
        arg >>= xI;
        if( xI.is() )
        {
            std::scoped_lock aGuard( m_aMutex );
            m_xIntrospection = xI;
        }
        // Reflection service ?
        Reference< XIdlReflection > xIdlR;
        arg >>= xIdlR;
        if( xIdlR.is() )
        {
            std::scoped_lock aGuard( m_aMutex );
            m_xReflection = xIdlR;
        }
        // Converter Service ?
        Reference< XTypeConverter > xC;
        arg >>= xC;
        if( xC.is() )
        {
            std::scoped_lock aGuard( m_aMutex );
            m_xConverter = xC;
        }

        // no right interface
        if( !xALAS.is() && !xI.is() && !xIdlR.is() && !xC.is() )
            throw IllegalArgumentException();
    }
}


//*** Private helper methods ***
Reference< XIntrospection > EventAttacherImpl::getIntrospection()
{
    std::scoped_lock aGuard( m_aMutex );
    if( !m_xIntrospection.is() )
    {
        m_xIntrospection = theIntrospection::get( m_xContext );
    }
    return m_xIntrospection;
}


//*** Private helper methods ***
Reference< XIdlReflection > EventAttacherImpl::getReflection()
{
    std::scoped_lock aGuard( m_aMutex );
    if( !m_xReflection.is() )
    {
        m_xReflection = theCoreReflection::get(m_xContext);
    }
    return m_xReflection;
}


//*** Private helper methods ***
Reference< XInvocationAdapterFactory2 > EventAttacherImpl::getInvocationAdapterService()
{
    std::scoped_lock aGuard( m_aMutex );
    if( !m_xInvocationAdapterFactory.is() )
    {
        m_xInvocationAdapterFactory = InvocationAdapterFactory::create(m_xContext);
    }
    return m_xInvocationAdapterFactory;
}


//*** Private helper methods ***
Reference< XTypeConverter > EventAttacherImpl::getConverter()
{
    std::scoped_lock aGuard( m_aMutex );
    if( !m_xConverter.is() )
    {
        m_xConverter = Converter::create(m_xContext);
    }
    return m_xConverter;
}

namespace {

// Implementation of an EventAttacher-related AllListeners, which brings
// a few Events to a general AllListener
class FilterAllListenerImpl : public WeakImplHelper< XAllListener  >
{
public:
    FilterAllListenerImpl( EventAttacherImpl * pEA_, OUString  EventMethod_,
                           const Reference< XAllListener >& AllListener_ );

    // XAllListener
    virtual void SAL_CALL firing(const AllEventObject& Event) override;
    virtual Any SAL_CALL approveFiring(const AllEventObject& Event) override;

    // XEventListener
    virtual void SAL_CALL disposing(const EventObject& Source) override;

private:
    // convert
    /// @throws CannotConvertException
    /// @throws RuntimeException
    void convertToEventReturn( Any & rRet, const Type& rRetType );

    EventAttacherImpl *         m_pEA;
    OUString                    m_EventMethod;
    Reference< XAllListener >   m_AllListener;
};

}

FilterAllListenerImpl::FilterAllListenerImpl( EventAttacherImpl * pEA_, OUString EventMethod_,
                                              const Reference< XAllListener >& AllListener_ )
        : m_pEA( pEA_ )
        , m_EventMethod(std::move( EventMethod_ ))
        , m_AllListener( AllListener_ )
{
}


void SAL_CALL FilterAllListenerImpl::firing(const AllEventObject& Event)
{
    if( Event.MethodName == m_EventMethod && m_AllListener.is() )
        m_AllListener->firing( Event );
}

// Convert to the standard event return
void FilterAllListenerImpl::convertToEventReturn( Any & rRet, const Type & rRetType )
{
    // no return value? Set to the specified values
    if( rRet.getValueTypeClass() == TypeClass_VOID )
    {
        switch( rRetType.getTypeClass()  )
        {
            case TypeClass_INTERFACE:
                {
                rRet <<= Reference< XInterface >();
                }
                break;

            case TypeClass_BOOLEAN:
                rRet <<= true;
                break;

            case TypeClass_STRING:
                rRet <<= OUString();
                break;

            case TypeClass_FLOAT:           rRet <<= float(0);  break;
            case TypeClass_DOUBLE:          rRet <<= 0.0;   break;
            case TypeClass_BYTE:            rRet <<= sal_uInt8( 0 );    break;
            case TypeClass_SHORT:           rRet <<= sal_Int16( 0 );    break;
            case TypeClass_LONG:            rRet <<= sal_Int32( 0 );    break;
            case TypeClass_UNSIGNED_SHORT:  rRet <<= sal_uInt16( 0 );   break;
            case TypeClass_UNSIGNED_LONG:   rRet <<= sal_uInt32( 0 );   break;
                     default:
            break;
        }
    }
    else if( !rRet.getValueType().equals( rRetType ) )
    {
        Reference< XTypeConverter > xConverter = m_pEA->getConverter();
        if( !xConverter.is() )
            throw CannotConvertException(); // TODO TypeConversionException
        rRet = xConverter->convertTo( rRet, rRetType );
    }
}


Any SAL_CALL FilterAllListenerImpl::approveFiring( const AllEventObject& Event )
{
    Any aRet;

    if( Event.MethodName == m_EventMethod && m_AllListener.is() )
        aRet = m_AllListener->approveFiring( Event );
    else
    {
        // Convert to the standard event return
        try
        {
            Reference< XIdlClass > xListenerType = m_pEA->getReflection()->
                        forName( Event.ListenerType.getTypeName() );
            Reference< XIdlMethod > xMeth = xListenerType->getMethod( Event.MethodName );
            if( xMeth.is() )
            {
                Reference< XIdlClass > xRetType = xMeth->getReturnType();
                Type aRetType( xRetType->getTypeClass(), xRetType->getName() );
                convertToEventReturn( aRet, aRetType );
            }
        }
        catch( const CannotConvertException& )
        {
            css::uno::Any anyEx = cppu::getCaughtException();
            throw InvocationTargetException( OUString(), Reference< XInterface >(), anyEx );
        }
    }
    return aRet;
}


void FilterAllListenerImpl::disposing(const EventObject& )
{
    // TODO: ???
}


Reference< XEventListener > EventAttacherImpl::attachListener
(
    const Reference< XInterface >& xObject,
    const Reference< XAllListener >& AllListener,
    const Any& Helper,
    const OUString& ListenerType,
    const OUString& AddListenerParam
)
{
    if( !xObject.is() || !AllListener.is() )
        throw IllegalArgumentException();

    Reference< XInvocationAdapterFactory2 > xInvocationAdapterFactory = getInvocationAdapterService();
    if( !xInvocationAdapterFactory.is() )
        throw ServiceNotRegisteredException();

    Reference< XIdlReflection > xReflection = getReflection();
    if( !xReflection.is() )
        throw ServiceNotRegisteredException();

    // Sign in, Call the fitting addListener method
    // First Introspection, as the Methods can be analyzed in the same way
    // For better performance it is implemented here again or make the Impl-Method
    // of the Introspection configurable for this purpose.
    Reference< XIntrospection > xIntrospection = getIntrospection();
    if( !xIntrospection.is() )
        return Reference<XEventListener>();

    // Inspect Introspection
    Any aObjAny( &xObject, cppu::UnoType<XInterface>::get());

    Reference< XIntrospectionAccess > xAccess = xIntrospection->inspect( aObjAny );
    if( !xAccess.is() )
        return Reference<XEventListener>();

    return attachListenerForTarget(
        xAccess, xInvocationAdapterFactory, AllListener, aObjAny, Helper,
        ListenerType, AddListenerParam);
}

Reference<XEventListener> EventAttacherImpl::attachListenerForTarget(
    const Reference<XIntrospectionAccess>& xAccess,
    const Reference<XInvocationAdapterFactory2>& xInvocationAdapterFactory,
    const Reference<XAllListener>& xAllListener,
    const Any& aObject,
    const Any& aHelper,
    const OUString& aListenerType,
    const OUString& aAddListenerParam)
{
    Reference< XEventListener > xRet;

    // Construct the name of the addListener-Method.
    sal_Int32 nIndex = aListenerType.lastIndexOf('.');
    // set index to the interface name without package name
    if( nIndex == -1 )
        // not found
        nIndex = 0;
    else
        nIndex++;

    OUString aListenerName = (!aListenerType.isEmpty() && aListenerType[nIndex] == 'X') ? aListenerType.copy(nIndex+1) : aListenerType;
    OUString aAddListenerName = "add" + aListenerName;

    // Send Methods to the correct addListener-Method
    const Sequence< Reference< XIdlMethod > > aMethodSeq = xAccess->getMethods( MethodConcept::LISTENER );
    for (const Reference< XIdlMethod >& rxMethod : aMethodSeq)
    {
        // Is it the correct method?
        OUString aMethName = rxMethod->getName();

        if (aAddListenerName != aMethName)
            continue;

        Sequence< Reference< XIdlClass > > params = rxMethod->getParameterTypes();
        sal_uInt32 nParamCount = params.getLength();

        Reference< XIdlClass > xListenerType;
        if( nParamCount == 1 )
            xListenerType = params.getConstArray()[0];
        else if( nParamCount == 2 )
            xListenerType = params.getConstArray()[1];

        // Request Adapter for the actual Listener type
        Reference< XInterface > xAdapter = createAllListenerAdapter(
            xInvocationAdapterFactory, xListenerType, xAllListener, aHelper );

        if( !xAdapter.is() )
            throw CannotCreateAdapterException();
        xRet.set( xAdapter, UNO_QUERY );

        // Just the Listener as parameter?
        if( nParamCount == 1 )
        {
            Sequence< Any > args( 1 );
            args.getArray()[0] <<= xAdapter;
            try
            {
                rxMethod->invoke( aObject, args );
            }
            catch( const InvocationTargetException& )
            {
                throw IntrospectionException();
            }
        }
        // Else, pass the other parameter now
        else if( nParamCount == 2 )
        {
            Sequence< Any > args( 2 );
            Any* pAnys = args.getArray();

            // Check the type of the 1st parameter
            Reference< XIdlClass > xParamClass = params.getConstArray()[0];
            if( xParamClass->getTypeClass() == TypeClass_STRING )
            {
                pAnys[0] <<= aAddListenerParam;
            }

            // 2nd Parameter == Listener? TODO: Test!
            pAnys[1] <<= xAdapter;

            // TODO: Convert String -> ?
            // else
            try
            {
                rxMethod->invoke( aObject, args );
            }
            catch( const InvocationTargetException& )
            {
                throw IntrospectionException();
            }
        }
        break;
        // else...
        // Anything else is not supported
    }

    return xRet;
}

Sequence< Reference<XEventListener> > EventAttacherImpl::attachListeners(
    const Reference<XInterface>& xObject,
    const Sequence< Reference<XAllListener> >& AllListeners,
    const Sequence<css::script::EventListener>& aListeners )
{
    sal_Int32 nCount = aListeners.getLength();
    if (nCount != AllListeners.getLength())
        // This is a prerequisite!
        throw RuntimeException();

    if (!xObject.is())
        throw IllegalArgumentException();

    Reference< XInvocationAdapterFactory2 > xInvocationAdapterFactory = getInvocationAdapterService();
    if( !xInvocationAdapterFactory.is() )
        throw ServiceNotRegisteredException();

    Reference< XIdlReflection > xReflection = getReflection();
    if( !xReflection.is() )
        throw ServiceNotRegisteredException();

    // Sign in, Call the fitting addListener method
    // First Introspection, as the Methods can be analyzed in the same way
    // For better performance it is implemented here again or make the Impl-Method
    // of the Introspection configurable for this purpose.
    Reference< XIntrospection > xIntrospection = getIntrospection();
    if( !xIntrospection.is() )
        return Sequence< Reference<XEventListener> >();

    // Inspect Introspection
    Any aObjAny( &xObject, cppu::UnoType<XInterface>::get() );

    Reference<XIntrospectionAccess> xAccess = xIntrospection->inspect(aObjAny);
    if (!xAccess.is())
        return Sequence< Reference<XEventListener> >();

    Sequence< Reference<XEventListener> > aRet(nCount);
    Reference<XEventListener>* pArray = aRet.getArray();

    for (sal_Int32 i = 0; i < nCount; ++i)
    {
        pArray[i] = attachListenerForTarget(
            xAccess, xInvocationAdapterFactory, AllListeners[ i ],
            aObjAny, aListeners[i].Helper, aListeners[i].ListenerType, aListeners[i].AddListenerParam);
    }

    return aRet;
}

// XEventAttacher
Reference< XEventListener > EventAttacherImpl::attachSingleEventListener
(
    const Reference< XInterface >& xObject,
    const Reference< XAllListener >& AllListener,
    const Any& Helper,
    const OUString& ListenerType,
    const OUString& AddListenerParam,
    const OUString& EventMethod
)
{
    // Subscribe FilterListener
    Reference<XAllListener> aFilterListener
        = new FilterAllListenerImpl(this, EventMethod, AllListener);
    return attachListener( xObject, aFilterListener, Helper, ListenerType, AddListenerParam);
}

// XEventAttacher
void EventAttacherImpl::removeListener
(
    const Reference< XInterface >& xObject,
    const OUString& ListenerType,
    const OUString& AddListenerParam,
    const Reference< XEventListener >& aToRemoveListener
)
{
    if( !xObject.is() || !aToRemoveListener.is() )
        throw IllegalArgumentException();

    Reference< XIdlReflection > xReflection = getReflection();
    if( !xReflection.is() )
        throw IntrospectionException();

    // Sign off, Call the fitting removeListener method
    // First Introspection, as the Methods can be analyzed in the same way
    // For better performance it is implemented here again or make the Impl-Method
    // of the Introspection configurable for this purpose.
    Reference< XIntrospection > xIntrospection = getIntrospection();
    if( !xIntrospection.is() )
        throw IntrospectionException();

    //Inspect Introspection
    Any aObjAny( &xObject, cppu::UnoType<XInterface>::get());
    Reference< XIntrospectionAccess > xAccess = xIntrospection->inspect( aObjAny );
    if( !xAccess.is() )
        throw IntrospectionException();

    // Create name of the removeListener-Method
    OUString aRemoveListenerName;
    OUString aListenerName( ListenerType );
    sal_Int32 nIndex = aListenerName.lastIndexOf( '.' );
    // set index to the interface name without package name
    if( nIndex == -1 )
        // not found
        nIndex = 0;
    else
        nIndex++;
    if( aListenerName[nIndex] == 'X' )
        // erase X from the interface name
        aListenerName = aListenerName.copy( nIndex +1 );
    aRemoveListenerName = "remove" + aListenerName;

    // Search methods for the correct removeListener method
    Sequence< Reference< XIdlMethod > > aMethodSeq = xAccess->getMethods( MethodConcept::LISTENER );
    sal_uInt32 i, nLen = aMethodSeq.getLength();
    const Reference< XIdlMethod >* pMethods = aMethodSeq.getConstArray();
    for( i = 0 ; i < nLen ; i++ )
    {
        // Call Method
        const Reference< XIdlMethod >& rxMethod = pMethods[i];

        // Is it the right method?
        if( aRemoveListenerName == rxMethod->getName() )
        {
            Sequence< Reference< XIdlClass > > params = rxMethod->getParameterTypes();
            sal_uInt32 nParamCount = params.getLength();

            // Just the Listener as parameter?
            if( nParamCount == 1 )
            {
                Sequence< Any > args( 1 );
                args.getArray()[0] <<= aToRemoveListener;
                try
                {
                    rxMethod->invoke( aObjAny, args );
                }
                catch( const InvocationTargetException& )
                {
                    throw IntrospectionException();
                }
            }
            // Else pass the other parameter
            else if( nParamCount == 2 )
            {
                Sequence< Any > args( 2 );
                Any* pAnys = args.getArray();

                // Check the type of the 1st parameter
                Reference< XIdlClass > xParamClass = params.getConstArray()[0];
                if( xParamClass->getTypeClass() == TypeClass_STRING )
                    pAnys[0] <<= AddListenerParam;

                // 2nd parameter == Listener? TODO: Test!
                pAnys[1] <<= aToRemoveListener;

                // TODO: Convert String -> ?
                // else
                try
                {
                    rxMethod->invoke( aObjAny, args );
                }
                catch( const InvocationTargetException& )
                {
                    throw IntrospectionException();
                }
            }
            break;
        }
    }
}

Sequence< Reference<XEventListener> > EventAttacherImpl::attachMultipleEventListeners(
    const Reference<XInterface>& xObject, const Sequence<css::script::EventListener>& aListeners )
{
    sal_Int32 nCount = aListeners.getLength();
    Sequence< Reference<XAllListener> > aFilterListeners(nCount);
    auto aFilterListenersRange = asNonConstRange(aFilterListeners);
    for (sal_Int32 i = 0; i < nCount; ++i)
    {
        aFilterListenersRange[i]
            = new FilterAllListenerImpl(this, aListeners[i].EventMethod, aListeners[i].AllListener);
    }

    return attachListeners(xObject, aFilterListeners, aListeners);
}

}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
eventattacher_EventAttacher(
    css::uno::XComponentContext* context, css::uno::Sequence<css::uno::Any> const& )
{
    return cppu::acquire(new comp_EventAttacher::EventAttacherImpl(context));
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
