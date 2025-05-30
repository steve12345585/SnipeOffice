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

#include <comphelper/sequence.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <cppuhelper/exc_hlp.hxx>
#include <cppuhelper/weak.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/implbase.hxx>

#include <com/sun/star/script/CannotConvertException.hpp>
#include <com/sun/star/script/XTypeConverter.hpp>
#include <com/sun/star/script/XInvocation.hpp>
#include <com/sun/star/script/XInvocation2.hpp>
#include <com/sun/star/reflection/XIdlReflection.hpp>
#include <com/sun/star/reflection/theCoreReflection.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/container/XIndexContainer.hpp>
#include <com/sun/star/container/XEnumerationAccess.hpp>
#include <com/sun/star/beans/XExactName.hpp>
#include <com/sun/star/beans/XMaterialHolder.hpp>
#include <com/sun/star/beans/theIntrospection.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/beans/MethodConcept.hpp>
#include <com/sun/star/beans/PropertyConcept.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XTypeProvider.hpp>

#include <memory>
#include <vector>

using namespace css::uno;
using namespace css::lang;
using namespace css::script;
using namespace css::reflection;
using namespace css::beans;
using namespace css::container;
using namespace cppu;

namespace stoc_inv
{


// TODO: Implement centrally
static Reference<XIdlClass> TypeToIdlClass( const Type& rType, const Reference< XIdlReflection > & xRefl )
{
    return xRefl->forName( rType.getTypeName() );
}

namespace {

class Invocation_Impl
    : public OWeakObject
    , public XInvocation2
    , public XNameContainer
    , public XIndexContainer
    , public XEnumerationAccess
    , public XExactName
    , public XMaterialHolder
    , public XTypeProvider
{
public:
    Invocation_Impl( const Any & rAdapted, const Reference<XTypeConverter> &,
                                           const Reference<XIntrospection> &,
                                           const Reference<XIdlReflection> &,
                                           bool bFromOLE );

    // XInterface
    virtual Any         SAL_CALL queryInterface( const Type & aType) override;
    virtual void        SAL_CALL acquire() noexcept override { OWeakObject::acquire(); }
    virtual void        SAL_CALL release() noexcept override { OWeakObject::release(); }


    // XTypeProvider
    virtual Sequence< css::uno::Type > SAL_CALL getTypes(  ) override;
    virtual Sequence< sal_Int8 > SAL_CALL getImplementationId(  ) override;

    // XMaterialHolder
    virtual Any         SAL_CALL getMaterial() override;

    // XInvocation
    virtual Reference<XIntrospectionAccess> SAL_CALL getIntrospection() override;
    virtual Any SAL_CALL invoke(const OUString& FunctionName, const Sequence< Any >& Params, Sequence< sal_Int16 >& OutParamIndex, Sequence< Any >& OutParam) override;
    virtual void SAL_CALL setValue(const OUString& PropertyName, const Any& Value) override;
    virtual Any SAL_CALL getValue(const OUString& PropertyName) override;
    virtual sal_Bool SAL_CALL hasMethod(const OUString& Name) override;
    virtual sal_Bool SAL_CALL hasProperty(const OUString& Name) override;

    // XInvocation2
    virtual Sequence< OUString > SAL_CALL getMemberNames(  ) override;
    virtual Sequence< InvocationInfo > SAL_CALL getInfo(  ) override;
    virtual InvocationInfo SAL_CALL getInfoForName( const OUString& aName, sal_Bool bExact ) override;

    // All Access and Container methods are not thread safe
    // XElementAccess
    virtual Type SAL_CALL getElementType() override
        { return _xElementAccess->getElementType(); }

    virtual sal_Bool SAL_CALL hasElements() override
        { return _xElementAccess->hasElements(); }

    // XNameContainer
    virtual void SAL_CALL insertByName( const OUString& Name, const Any& Element ) override
        { _xNameContainer->insertByName( Name, Element ); }

    virtual void SAL_CALL removeByName( const OUString& Name ) override
        { _xNameContainer->removeByName( Name ); }

    // XNameReplace
    virtual void SAL_CALL replaceByName( const OUString& Name, const Any& Element ) override
        { _xNameReplace->replaceByName( Name, Element ); }

    // XNameAccess
    virtual Any SAL_CALL getByName( const OUString& Name ) override
        { return _xNameAccess->getByName( Name ); }

    virtual Sequence<OUString> SAL_CALL getElementNames() override
        { return _xNameAccess->getElementNames(); }

    virtual sal_Bool SAL_CALL hasByName( const OUString& Name ) override
        { return _xNameAccess->hasByName( Name ); }

    // XIndexContainer
    virtual void SAL_CALL insertByIndex( sal_Int32 Index, const Any& Element ) override
        { _xIndexContainer->insertByIndex( Index, Element ); }

    virtual void SAL_CALL removeByIndex( sal_Int32 Index ) override
        { _xIndexContainer->removeByIndex( Index ); }

    // XIndexReplace
    virtual void SAL_CALL replaceByIndex( sal_Int32 Index, const Any& Element ) override
        { _xIndexReplace->replaceByIndex( Index, Element ); }

    // XIndexAccess
    virtual sal_Int32 SAL_CALL getCount() override
        { return _xIndexAccess->getCount(); }

    virtual Any SAL_CALL getByIndex( sal_Int32 Index ) override
        { return _xIndexAccess->getByIndex( Index ); }

    // XEnumerationAccess
    virtual Reference<XEnumeration> SAL_CALL createEnumeration() override
        { return _xEnumerationAccess->createEnumeration(); }

    // XExactName
    virtual OUString SAL_CALL getExactName( const OUString& rApproximateName ) override;


private:
    void setMaterial( const Any& rMaterial );

    void getInfoSequenceImpl( Sequence< OUString >* pStringSeq, Sequence< InvocationInfo >* pInfoSeq );
    void fillInfoForNameAccess( InvocationInfo& rInfo, const OUString& aName );
    static void fillInfoForProperty( InvocationInfo& rInfo, const Property& rProp );
    static void fillInfoForMethod( InvocationInfo& rInfo, const Reference< XIdlMethod >& xMethod );

    Reference<XTypeConverter>           xTypeConverter;
    Reference<XIntrospection>           xIntrospection;
    Reference<XIdlReflection>           xCoreReflection;

    Any                             _aMaterial;
    // _xDirect and (_xIntrospectionAccess, xPropertySet) are exclusive
    Reference<XInvocation>              _xDirect;
    Reference<XInvocation2>             _xDirect2;
    Reference<XPropertySet>             _xPropertySet;
    Reference<XIntrospectionAccess>     _xIntrospectionAccess;

    // supplied Interfaces
    Reference<XNameContainer>           _xNameContainer;
    Reference<XNameReplace>             _xNameReplace;
    Reference<XNameAccess>              _xNameAccess;
    Reference<XIndexContainer>          _xIndexContainer;
    Reference<XIndexReplace>            _xIndexReplace;
    Reference<XIndexAccess>             _xIndexAccess;
    Reference<XEnumerationAccess>       _xEnumerationAccess;
    Reference<XElementAccess>           _xElementAccess;


    Reference<XExactName>               _xENDirect, _xENIntrospection;

    bool                                mbFromOLE;
};

}

Invocation_Impl::Invocation_Impl
(
    const Any & rAdapted,
    const Reference<XTypeConverter> & rTC,
    const Reference<XIntrospection> & rI,
    const Reference<XIdlReflection> & rCR,
    bool bFromOLE
)
    : xTypeConverter( rTC )
    , xIntrospection( rI )
    , xCoreReflection( rCR )
    , mbFromOLE( bFromOLE )
{
    setMaterial( rAdapted );
}

//### INTERFACE IMPLEMENTATIONS ####################################################################


Any SAL_CALL Invocation_Impl::queryInterface( const Type & aType )
{
    // PropertySet implementation
    Any a = ::cppu::queryInterface( aType,
                                   static_cast< XInvocation* >(this),
                                   static_cast< XMaterialHolder* >(this),
                                   static_cast< XTypeProvider * >(this) );
    if( a.hasValue() )
    {
        return a;
    }

    if( aType  == cppu::UnoType<XExactName>::get())
    {
        // Invocation does not support XExactName, if direct object supports
        // XInvocation, but not XExactName. Except when called from OLE Automation.
        if (mbFromOLE ||
            (_xDirect.is() && _xENDirect.is()) ||
            (!_xDirect.is() && _xENIntrospection.is()))
        {
            return Any( Reference< XExactName >( static_cast< XExactName* >(this) ) );
        }
    }
    else if ( aType == cppu::UnoType<XNameContainer>::get())
    {
        if( _xNameContainer.is() )
            return Any( Reference< XNameContainer >( static_cast< XNameContainer* >(this) ) );
    }
    else if ( aType == cppu::UnoType<XNameReplace>::get())
    {
        if( _xNameReplace.is() )
            return Any( Reference< XNameReplace >( static_cast< XNameReplace* >(this) ) );
    }
    else if ( aType == cppu::UnoType<XNameAccess>::get())
    {
        if( _xNameAccess.is() )
            return Any( Reference< XNameAccess >( static_cast< XNameAccess* >(this) ) );
    }
    else if ( aType == cppu::UnoType<XIndexContainer>::get())
    {
        if (_xIndexContainer.is())
            return Any( Reference< XIndexContainer >( static_cast< XIndexContainer* >(this) ) );
    }
    else if ( aType == cppu::UnoType<XIndexReplace>::get())
    {
        if (_xIndexReplace.is())
            return Any( Reference< XIndexReplace >( static_cast< XIndexReplace* >(this) ) );
    }
    else if ( aType == cppu::UnoType<XIndexAccess>::get())
    {
        if (_xIndexAccess.is())
            return Any( Reference< XIndexAccess >( static_cast< XIndexAccess* >(this) ) );
    }
    else if ( aType == cppu::UnoType<XEnumerationAccess>::get())
    {
        if (_xEnumerationAccess.is())
            return Any( Reference< XEnumerationAccess >( static_cast< XEnumerationAccess* >(this) ) );
    }
    else if ( aType == cppu::UnoType<XElementAccess>::get())
    {
        if (_xElementAccess.is())
        {
            return Any( Reference< XElementAccess >(
                static_cast< XElementAccess* >(static_cast< XNameContainer* >(this)) ) );
        }
    }
    else if ( aType == cppu::UnoType<XInvocation2>::get())
    {
        // Invocation does not support XInvocation2, if direct object supports
        // XInvocation, but not XInvocation2.
        if ( mbFromOLE ||
             ( _xDirect.is() && _xDirect2.is()) ||
             (!_xDirect.is() && _xIntrospectionAccess.is() ) )
        {
            return Any( Reference< XInvocation2 >( static_cast< XInvocation2* >(this) ) );
        }
    }

    return OWeakObject::queryInterface( aType );
}


Any Invocation_Impl::getMaterial()
{
    // AB, 12.2.1999  Make sure that the material is taken when possible
    // from the direct Invocation of the Introspection, otherwise structs
    // are not handled correctly
    Reference<XMaterialHolder> xMaterialHolder;
    if( _xDirect.is() )
    {
        xMaterialHolder.set( _xDirect, UNO_QUERY );
        //_xDirect->queryInterface( XMaterialHolder::getSmartUik(), xMaterialHolder );
    }
    else if( _xIntrospectionAccess.is() )
    {
        xMaterialHolder.set( _xIntrospectionAccess, UNO_QUERY );
        //_xIntrospectionAccess->queryInterface( XMaterialHolder::getSmartUik(), xMaterialHolder );
    }
    if( xMaterialHolder.is() )
    {
        return xMaterialHolder->getMaterial();
    }
    return _aMaterial;
}


void Invocation_Impl::setMaterial( const Any& rMaterial )
{
    // set the material first and only once
    _aMaterial = rMaterial;

    // First do this outside the guard
    _xDirect.set( rMaterial, UNO_QUERY );

    if( !mbFromOLE && _xDirect.is() )
    {
        // Consult object directly
        _xElementAccess.set( _xDirect, UNO_QUERY );
        _xEnumerationAccess.set( _xDirect, UNO_QUERY );
        _xIndexAccess.set( _xDirect, UNO_QUERY );
        _xIndexReplace.set( _xDirect, UNO_QUERY );
        _xIndexContainer.set( _xDirect, UNO_QUERY );
        _xNameAccess.set( _xDirect, UNO_QUERY );
        _xNameReplace.set( _xDirect, UNO_QUERY );
        _xNameContainer.set( _xDirect, UNO_QUERY );
        _xENDirect.set( _xDirect, UNO_QUERY );
        _xDirect2.set( _xDirect, UNO_QUERY );
    }
    else
    {
        // Make Invocation on the Introspection
        if (xIntrospection.is())
        {
            _xIntrospectionAccess = xIntrospection->inspect( _aMaterial );
            if( _xIntrospectionAccess.is() )
            {
                _xElementAccess.set(
                      _xIntrospectionAccess->queryAdapter(
                                 cppu::UnoType<XElementAccess>::get()), UNO_QUERY );

                if( _xElementAccess.is() )
                {
                    _xEnumerationAccess.set(
                               _xIntrospectionAccess->queryAdapter(
                                    cppu::UnoType<XEnumerationAccess>::get()), UNO_QUERY );

                    _xIndexAccess.set(
                           _xIntrospectionAccess->queryAdapter(
                                    cppu::UnoType<XIndexAccess>::get()), UNO_QUERY );

                    if( _xIndexAccess.is() )
                    {
                        _xIndexReplace.set(
                             _xIntrospectionAccess->queryAdapter(
                                        cppu::UnoType<XIndexReplace>::get()), UNO_QUERY );

                        _xIndexContainer.set(
                             _xIntrospectionAccess->queryAdapter(
                                        cppu::UnoType<XIndexContainer>::get()), UNO_QUERY );
                    }

                    _xNameAccess.set(
                         _xIntrospectionAccess->queryAdapter(
                                    cppu::UnoType<XNameAccess>::get()), UNO_QUERY );

                    if( _xNameAccess.is() )
                    {
                        _xNameReplace.set(
                                   _xIntrospectionAccess->queryAdapter(
                                       cppu::UnoType<XNameReplace>::get()), UNO_QUERY );

                        _xNameContainer.set(
                                   _xIntrospectionAccess->queryAdapter(
                                       cppu::UnoType<XNameContainer>::get()), UNO_QUERY );
                    }
                }

                _xPropertySet.set( _xIntrospectionAccess->queryAdapter( cppu::UnoType<XPropertySet>::get()),
                                   UNO_QUERY );

                _xENIntrospection.set( _xIntrospectionAccess, UNO_QUERY );
            }
        }
    }
}


OUString Invocation_Impl::getExactName( const OUString& rApproximateName )
{
    if (_xENDirect.is())
        return _xENDirect->getExactName( rApproximateName );

    OUString aRet;
    if (_xENIntrospection.is())
        aRet = _xENIntrospection->getExactName( rApproximateName );
    return aRet;
}


Reference<XIntrospectionAccess> Invocation_Impl::getIntrospection()
{
    if( _xDirect.is() )
        return _xDirect->getIntrospection();
    else
        return _xIntrospectionAccess;
}


sal_Bool Invocation_Impl::hasMethod( const OUString& Name )
{
    if (!mbFromOLE && _xDirect.is())
        return _xDirect->hasMethod( Name );
    if( _xIntrospectionAccess.is() )
        return _xIntrospectionAccess->hasMethod( Name, MethodConcept::ALL ^ MethodConcept::DANGEROUS );
    return false;
}


sal_Bool Invocation_Impl::hasProperty( const OUString& Name )
{
    if (_xDirect.is())
    {
        bool bRet = _xDirect->hasProperty( Name );
        if (bRet || !mbFromOLE)
            return bRet;
    }
    // PropertySet
    if( _xIntrospectionAccess.is()
        && _xIntrospectionAccess->hasProperty( Name, PropertyConcept::ALL ^ PropertyConcept::DANGEROUS ) )
        return true;
    // NameAccess
    if( _xNameAccess.is() )
        return _xNameAccess->hasByName( Name );
    return false;
}


Any Invocation_Impl::getValue( const OUString& PropertyName )
{
    try
    {
        if (_xDirect.is())
            return _xDirect->getValue( PropertyName );
    }
    catch (Exception &)
    {
        if (!mbFromOLE)
            throw;
    }
    try
    {
        // PropertySet
        if( _xIntrospectionAccess.is() && _xPropertySet.is()
            && _xIntrospectionAccess->hasProperty
            ( PropertyName, PropertyConcept::ALL ^ PropertyConcept::DANGEROUS ) )
        {
            return _xPropertySet->getPropertyValue( PropertyName );
        }
        // NameAccess
        if( _xNameAccess.is() && _xNameAccess->hasByName( PropertyName ) )
            return _xNameAccess->getByName( PropertyName );
    }
    catch (UnknownPropertyException &)
    {
        throw;
    }
    catch (RuntimeException &)
    {
        throw;
    }
    catch (Exception &)
    {
    }

    throw UnknownPropertyException( "cannot get value " + PropertyName );
}


void Invocation_Impl::setValue( const OUString& PropertyName, const Any& Value )
{
    try
    {
        if (_xDirect.is())
        {
            _xDirect->setValue( PropertyName, Value );
            return;
        }
    }
    catch (Exception &)
    {
        if (!mbFromOLE)
            throw;
    }
    try
    {
        // Properties
        if( _xIntrospectionAccess.is() && _xPropertySet.is()
            && _xIntrospectionAccess->hasProperty(
                PropertyName, PropertyConcept::ALL ^ PropertyConcept::DANGEROUS ) )
        {
            Property aProp = _xIntrospectionAccess->getProperty(
                PropertyName, PropertyConcept::ALL ^ PropertyConcept::DANGEROUS );
            Reference < XIdlClass > r = TypeToIdlClass( aProp.Type, xCoreReflection );
            if( r->isAssignableFrom( TypeToIdlClass( Value.getValueType(), xCoreReflection ) ) )
                _xPropertySet->setPropertyValue( PropertyName, Value );
            else if( xTypeConverter.is() )
                _xPropertySet->setPropertyValue(
                    PropertyName, xTypeConverter->convertTo( Value, aProp.Type ) );
            else
                throw RuntimeException( u"no type converter service!"_ustr );
        }
        // NameContainer
        else if( _xNameContainer.is() )
        {
            // Note: This misfeature deliberately not adapted to apply to objects which
            // have XNameReplace but not XNameContainer
            Any aConv;
            Reference < XIdlClass > r =
                TypeToIdlClass( _xNameContainer->getElementType(), xCoreReflection );
            if( r->isAssignableFrom(TypeToIdlClass( Value.getValueType(), xCoreReflection ) ) )
                aConv = Value;
            else if( xTypeConverter.is() )
                aConv = xTypeConverter->convertTo( Value, _xNameContainer->getElementType() );
            else
                throw RuntimeException( u"no type converter service!"_ustr );

            // Replace if present, otherwise insert
            if (_xNameContainer->hasByName( PropertyName ))
                _xNameContainer->replaceByName( PropertyName, aConv );
            else
                _xNameContainer->insertByName( PropertyName, aConv );
        }
        else
            throw UnknownPropertyException( u"no introspection nor name container!"_ustr );
    }
    catch (UnknownPropertyException &)
    {
        throw;
    }
    catch (CannotConvertException &)
    {
        throw;
    }
    catch (InvocationTargetException &)
    {
        throw;
    }
    catch (RuntimeException &)
    {
        throw;
    }
    catch (const Exception & exc)
    {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw InvocationTargetException(
            "exception occurred in setValue(): " + exc.Message,
            Reference< XInterface >(), anyEx );
    }
}


Any Invocation_Impl::invoke( const OUString& FunctionName, const Sequence<Any>& InParams,
                                Sequence<sal_Int16>& OutIndices, Sequence<Any>& OutParams )
{
    if (!mbFromOLE && _xDirect.is())
        return _xDirect->invoke( FunctionName, InParams, OutIndices, OutParams );

    if (_xIntrospectionAccess.is())
    {
        // throw NoSuchMethodException if not exist
        Reference<XIdlMethod> xMethod = _xIntrospectionAccess->getMethod(
            FunctionName, MethodConcept::ALL ^ MethodConcept::DANGEROUS );

        // ParameterInfos
        Sequence<ParamInfo> aFParams        = xMethod->getParameterInfos();
        const ParamInfo* pFParams           = aFParams.getConstArray();
        sal_Int32 nFParamsLen               = aFParams.getLength();
        if (nFParamsLen != InParams.getLength())
        {
            throw IllegalArgumentException(
                "incorrect number of parameters passed invoking function " + FunctionName +
                ": expected " + OUString::number(nFParamsLen) + ", got " + OUString::number(InParams.getLength()),
                getXWeak(), sal_Int16(1) );
        }

        // IN Parameter
        const Any* pInParams                = InParams.getConstArray();

        // Introspection Invoke Parameter
        Sequence<Any> aInvokeParams( nFParamsLen );
        Any* pInvokeParams                  = aInvokeParams.getArray();

        // OUT Indices
        OutIndices.realloc( nFParamsLen );
        sal_Int16* pOutIndices              = OutIndices.getArray();
        sal_uInt32 nOutIndex                = 0;

        for ( sal_Int32 nPos = 0; nPos < nFParamsLen; ++nPos )
        {
            try
            {
                const ParamInfo& rFParam = pFParams[nPos];
                const Reference<XIdlClass>& rDestType = rFParam.aType;

                // is IN/INOUT parameter?
                if (rFParam.aMode != ParamMode_OUT)
                {
                    if (rDestType->isAssignableFrom( TypeToIdlClass( pInParams[nPos].getValueType(), xCoreReflection ) ))
                    {
                        pInvokeParams[nPos] = pInParams[nPos];
                    }
                    else if (xTypeConverter.is())
                    {
                        Type aDestType( rDestType->getTypeClass(), rDestType->getName() );
                        pInvokeParams[nPos] = xTypeConverter->convertTo( pInParams[nPos], aDestType );
                    }
                    else
                    {
                        throw CannotConvertException(u"invocation type mismatch!"_ustr, *this, {}, 0, 0);
                    }
                }

                // is OUT/INOUT parameter?
                if (rFParam.aMode != ParamMode_IN)
                {
                    pOutIndices[nOutIndex] = static_cast<sal_Int16>(nPos);
                    if (rFParam.aMode == ParamMode_OUT)
                        rDestType->createObject( pInvokeParams[nPos] );     // default init
                    ++nOutIndex;
                }
            }
            catch( CannotConvertException& rExc )
            {
                rExc.ArgumentIndex = nPos;  // Add optional parameter index
                throw;
            }
        }

        // execute Method
        Any aRet = xMethod->invoke( _aMaterial, aInvokeParams );

        // OUT Params
        OutIndices.realloc( nOutIndex );
        OutParams.realloc( nOutIndex );

        std::transform(std::cbegin(OutIndices), std::cend(OutIndices), OutParams.getArray(),
            [&pInvokeParams](const sal_Int16 nIndex) -> Any { return pInvokeParams[nIndex]; });

        return aRet;
    }

    throw RuntimeException(u"invocation lacking of introspection access!"_ustr, *this);
}

namespace {

// Struct to optimize sorting
struct MemberItem
{
    OUString aName;

    // Defines where the member comes from
    enum class Mode { NameAccess, PropertySet, Method };
    Mode eMode;

    // Index to respective sequence
    // (Index to NameAccess sequence for eMode==Mode::NameAccess etc.)
    sal_Int32 nIndex;
};

}

// Implementation of getting name or info
// String sequence will be filled when pStringSeq != NULL
// Info sequence will be filled when pInfoSeq != NULL
void Invocation_Impl::getInfoSequenceImpl
(
    Sequence< OUString >* pStringSeq,
    Sequence< InvocationInfo >* pInfoSeq
)
{
    //Sequence< OUString > aStrSeq;
    //if( !pStringSeq )
        //pStringSeq = &aStrSeq;


    // Get all needed sequences
    Sequence<OUString> aNameAccessNames;
    Sequence<Property> aPropertySeq;
    Sequence< Reference< XIdlMethod > > aMethodSeq;

    if( _xNameAccess.is() )
    {
        aNameAccessNames = _xNameAccess->getElementNames();
    }

    if( _xIntrospectionAccess.is() )
    {
        aPropertySeq = _xIntrospectionAccess->getProperties
            ( PropertyConcept::ALL - PropertyConcept::DANGEROUS );

        aMethodSeq = _xIntrospectionAccess->getMethods
            ( MethodConcept::ALL - MethodConcept::DANGEROUS );
    }

    sal_Int32 nNameAccessCount = aNameAccessNames.getLength();
    sal_Int32 nPropertyCount = aPropertySeq.getLength();
    sal_Int32 nMethodCount = aMethodSeq.getLength();
    sal_Int32 nTotalCount = nNameAccessCount + nPropertyCount + nMethodCount;

    // Create and fill array of MemberItems
    std::unique_ptr< MemberItem []> pItems( new MemberItem[ nTotalCount ] );
    const OUString* pStrings = aNameAccessNames.getConstArray();
    const Property* pProps = aPropertySeq.getConstArray();
    const Reference< XIdlMethod >* pMethods = aMethodSeq.getConstArray();

    // Fill array of MemberItems
    sal_Int32 i, iTotal = 0;

    // Name Access
    for( i = 0 ; i < nNameAccessCount ; i++, iTotal++ )
    {
        MemberItem& rItem = pItems[ iTotal ];
        rItem.aName = pStrings[ i ];
        rItem.eMode = MemberItem::Mode::NameAccess;
        rItem.nIndex = i;
    }

    // Property set
    for( i = 0 ; i < nPropertyCount ; i++, iTotal++ )
    {
        MemberItem& rItem = pItems[ iTotal ];
        rItem.aName = pProps[ i ].Name;
        rItem.eMode = MemberItem::Mode::PropertySet;
        rItem.nIndex = i;
    }

    // Methods
    for( i = 0 ; i < nMethodCount ; i++, iTotal++ )
    {
        MemberItem& rItem = pItems[ iTotal ];
        Reference< XIdlMethod > xMethod = pMethods[ i ];
        rItem.aName = xMethod->getName();
        rItem.eMode = MemberItem::Mode::Method;
        rItem.nIndex = i;
    }

    // Setting up result sequences
    OUString* pRetStrings = nullptr;
    if( pStringSeq )
    {
        pStringSeq->realloc( nTotalCount );
        pRetStrings = pStringSeq->getArray();
    }

    InvocationInfo* pRetInfos = nullptr;
    if( pInfoSeq )
    {
        pInfoSeq->realloc( nTotalCount );
        pRetInfos = pInfoSeq->getArray();
    }

    // Fill result sequences in the correct order of members
    for( iTotal = 0 ; iTotal < nTotalCount ; iTotal++ )
    {
        MemberItem& rItem = pItems[ iTotal ];
        if( pRetStrings )
        {
            pRetStrings[ iTotal ] = rItem.aName;
        }

        if( pRetInfos )
        {
            if( rItem.eMode == MemberItem::Mode::NameAccess )
            {
                fillInfoForNameAccess( pRetInfos[ iTotal ], rItem.aName );
            }
            else if( rItem.eMode == MemberItem::Mode::PropertySet )
            {
                fillInfoForProperty( pRetInfos[ iTotal ], pProps[ rItem.nIndex ] );
            }
            else if( rItem.eMode == MemberItem::Mode::Method )
            {
                fillInfoForMethod( pRetInfos[ iTotal ], pMethods[ rItem.nIndex ] );
            }
        }
    }
}

// XInvocation2
Sequence< OUString > SAL_CALL Invocation_Impl::getMemberNames(  )
{
    if( _xDirect2.is() )
    {
        return _xDirect2->getMemberNames();
    }
    Sequence< OUString > aRetSeq;
    getInfoSequenceImpl( &aRetSeq, nullptr );
    return aRetSeq;
}

Sequence< InvocationInfo > SAL_CALL Invocation_Impl::getInfo(  )
{
    if( _xDirect2.is() )
    {
        return _xDirect2->getInfo();
    }
    Sequence< InvocationInfo > aRetSeq;
    getInfoSequenceImpl( nullptr, &aRetSeq );
    return aRetSeq;
}

InvocationInfo SAL_CALL Invocation_Impl::getInfoForName( const OUString& aName, sal_Bool bExact )
{
    if( _xDirect2.is() )
    {
        return _xDirect2->getInfoForName( aName, bExact );
    }

    bool bFound = false;
    OUString aExactName = aName;
    InvocationInfo aRetInfo;

    if( bExact )
        aExactName = getExactName( aName );
    if( !aExactName.isEmpty() )
    {
        if( _xIntrospectionAccess->hasMethod( aExactName, MethodConcept::ALL ^ MethodConcept::DANGEROUS ) )
        {
            Reference<XIdlMethod> xMethod = _xIntrospectionAccess->getMethod
                ( aExactName, MethodConcept::ALL ^ MethodConcept::DANGEROUS );
            fillInfoForMethod( aRetInfo, xMethod );
            bFound = true;
        }
        else
        {
            if( _xIntrospectionAccess.is() && _xIntrospectionAccess->hasProperty
                 ( aExactName, PropertyConcept::ALL ^ PropertyConcept::DANGEROUS ) )
            {
                Property aProp = _xIntrospectionAccess->getProperty
                    ( aExactName, PropertyConcept::ALL ^ PropertyConcept::DANGEROUS );
                fillInfoForProperty( aRetInfo, aProp );
                bFound = true;
            }
            // NameAccess
            else if( _xNameAccess.is() && _xNameAccess->hasByName( aExactName ) )
            {
                fillInfoForNameAccess( aRetInfo, aExactName );
                bFound = true;
            }
        }
    }
    if( !bFound )
    {
        throw IllegalArgumentException(
            "getExactName(), Unknown name " + aName,
            getXWeak(), 0 );
    }
    return aRetInfo;
}

// Helper functions to fill InvocationInfo for XNameAccess
void Invocation_Impl::fillInfoForNameAccess
(
    InvocationInfo& rInfo,
    const OUString& aName
)
{
    rInfo.aName = aName;
    rInfo.eMemberType = MemberType_PROPERTY;
    rInfo.PropertyAttribute = 0;
    if( !_xNameContainer.is() )
    {
        rInfo.PropertyAttribute = PropertyAttribute::READONLY;
    }
    rInfo.aType = _xNameAccess->getElementType();
}

void Invocation_Impl::fillInfoForProperty
(
    InvocationInfo& rInfo,
    const Property& rProp
)
{
    rInfo.aName = rProp.Name;
    rInfo.eMemberType = MemberType_PROPERTY;
    rInfo.PropertyAttribute = rProp.Attributes;
    rInfo.aType = rProp.Type;
}

void Invocation_Impl::fillInfoForMethod
(
    InvocationInfo& rInfo,
    const Reference< XIdlMethod >& xMethod
)
{
    rInfo.aName = xMethod->getName();
    rInfo.eMemberType = MemberType_METHOD;
    Reference< XIdlClass > xReturnClass = xMethod->getReturnType();
    rInfo.aType = Type(xReturnClass->getTypeClass(), xReturnClass->getName());
    Sequence<ParamInfo> aParamInfos = xMethod->getParameterInfos();
    sal_Int32 nParamCount = aParamInfos.getLength();
    if( nParamCount <= 0 )
        return;

    const ParamInfo* pInfo = aParamInfos.getConstArray();

    rInfo.aParamTypes.realloc( nParamCount );
    Type* pParamTypes = rInfo.aParamTypes.getArray();
    rInfo.aParamModes.realloc( nParamCount );
    ParamMode* pParamModes = rInfo.aParamModes.getArray();

    for( sal_Int32 i = 0 ; i < nParamCount ; i++ )
    {
        Reference< XIdlClass > xParamClass = pInfo[i].aType;
        pParamTypes[ i ] = Type(xParamClass->getTypeClass(), xParamClass->getName());
        pParamModes[ i ] = pInfo[i].aMode;
    }
}


// XTypeProvider
Sequence< Type > SAL_CALL Invocation_Impl::getTypes()
{
    static Sequence<Type> s_types = [this]() {
        std::vector<Type> tmp {
            cppu::UnoType<XTypeProvider>::get(),
            cppu::UnoType<XWeak>::get(),
            cppu::UnoType<XInvocation>::get(),
            cppu::UnoType<XMaterialHolder>::get() };

        // Invocation does not support XExactName if direct object supports
        // XInvocation, but not XExactName.
        if ((_xDirect.is() && _xENDirect.is()) || (!_xDirect.is() && _xENIntrospection.is()))
            tmp.push_back(cppu::UnoType<XExactName>::get());
        if (_xNameContainer.is())
            tmp.push_back(cppu::UnoType<XNameContainer>::get());
        if (_xNameReplace.is())
            tmp.push_back(cppu::UnoType<XNameReplace>::get());
        if (_xNameAccess.is())
            tmp.push_back(cppu::UnoType<XNameAccess>::get());
        if (_xIndexContainer.is())
            tmp.push_back(cppu::UnoType<XIndexContainer>::get());
        if (_xIndexReplace.is())
            tmp.push_back(cppu::UnoType<XIndexReplace>::get());
        if (_xIndexAccess.is())
            tmp.push_back(cppu::UnoType<XIndexAccess>::get());
        if (_xEnumerationAccess.is())
            tmp.push_back(cppu::UnoType<XEnumerationAccess>::get());
        if (_xElementAccess.is())
            tmp.push_back(cppu::UnoType<XElementAccess>::get());
        // Invocation does not support XInvocation2, if direct object supports
        // XInvocation, but not XInvocation2.
        if ((_xDirect.is() && _xDirect2.is()) || (!_xDirect.is() && _xIntrospectionAccess.is()))
            tmp.push_back(cppu::UnoType<XInvocation2>::get());

        return comphelper::containerToSequence(tmp);
    }();
    return s_types;
}

Sequence< sal_Int8 > SAL_CALL Invocation_Impl::getImplementationId(  )
{
    return css::uno::Sequence<sal_Int8>();
}

namespace {

class InvocationService
    : public WeakImplHelper< XSingleServiceFactory, XServiceInfo >
{
public:
    explicit InvocationService( const Reference<XComponentContext> & xCtx );

    // XServiceInfo
    OUString                    SAL_CALL getImplementationName() override;
    sal_Bool                        SAL_CALL supportsService(const OUString& ServiceName) override;
    Sequence< OUString >        SAL_CALL getSupportedServiceNames() override;

    // XSingleServiceFactory
    Reference<XInterface>       SAL_CALL createInstance() override;
    Reference<XInterface>       SAL_CALL createInstanceWithArguments(
        const Sequence<Any>& rArguments ) override;
private:
    Reference<XComponentContext> mxCtx;
    Reference<XMultiComponentFactory> mxSMgr;
    Reference<XTypeConverter> xTypeConverter;
    Reference<XIntrospection> xIntrospection;
    Reference<XIdlReflection> xCoreReflection;
};

}

InvocationService::InvocationService( const Reference<XComponentContext> & xCtx )
    : mxCtx( xCtx )
    , mxSMgr( xCtx->getServiceManager() )
    , xCoreReflection( css::reflection::theCoreReflection::get(mxCtx) )
{
    xTypeConverter.set(
        mxSMgr->createInstanceWithContext( u"com.sun.star.script.Converter"_ustr, xCtx ),
        UNO_QUERY );
    xIntrospection = theIntrospection::get(xCtx);
}

// XServiceInfo
OUString InvocationService::getImplementationName()
{
    return u"com.sun.star.comp.stoc.Invocation"_ustr;
}

// XServiceInfo
sal_Bool InvocationService::supportsService(const OUString& ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

// XServiceInfo
Sequence< OUString > InvocationService::getSupportedServiceNames()
{
    return { u"com.sun.star.script.Invocation"_ustr };
}


Reference<XInterface> InvocationService::createInstance()
{
    //TODO:throw( Exception("no default construction of invocation adapter possible!", *this) );
    return Reference<XInterface>(); // dummy
}


Reference<XInterface> InvocationService::createInstanceWithArguments(
    const Sequence<Any>& rArguments )
{
    if (rArguments.getLength() == 2)
    {
        OUString aArg1;
        if ((rArguments[1] >>= aArg1) &&
            aArg1 == "FromOLE")
        {
            return Reference< XInterface >
                ( *new Invocation_Impl( *rArguments.getConstArray(),
                                        xTypeConverter, xIntrospection, xCoreReflection, true ) );
        }
    }
    if (rArguments.getLength() == 1)
    {
        return Reference< XInterface >
            ( *new Invocation_Impl( *rArguments.getConstArray(),
                                    xTypeConverter, xIntrospection, xCoreReflection, false ) );
    }

    //TODO:throw( Exception("no default construction of invocation adapter possible!", *this) );
    return Reference<XInterface>();
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
stoc_InvocationService_get_implementation(
    css::uno::XComponentContext* context, css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new InvocationService(context));
}

}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
