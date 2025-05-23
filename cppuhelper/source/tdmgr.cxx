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


#include <sal/config.h>
#include <sal/log.hxx>

#include <vector>

#include <osl/diagnose.h>
#include <rtl/ustring.hxx>

#include <uno/lbnames.h>
#include <uno/mapping.hxx>

#include <cppuhelper/bootstrap.hxx>
#include <cppuhelper/implbase.hxx>
#include <typelib/typedescription.h>

#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/container/XHierarchicalNameAccess.hpp>
#include <com/sun/star/reflection/XTypeDescription.hpp>
#include <com/sun/star/reflection/XEnumTypeDescription.hpp>
#include <com/sun/star/reflection/XIndirectTypeDescription.hpp>
#include <com/sun/star/reflection/XInterfaceMemberTypeDescription.hpp>
#include <com/sun/star/reflection/XInterfaceAttributeTypeDescription2.hpp>
#include <com/sun/star/reflection/XMethodParameter.hpp>
#include <com/sun/star/reflection/XInterfaceMethodTypeDescription.hpp>
#include <com/sun/star/reflection/XInterfaceTypeDescription2.hpp>
#include <com/sun/star/reflection/XCompoundTypeDescription.hpp>
#include <com/sun/star/reflection/XStructTypeDescription.hpp>

#include <memory>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::reflection;

namespace cppu
{

static typelib_TypeDescription * createCTD(
    Reference< container::XHierarchicalNameAccess > const & access,
    const Reference< XTypeDescription > & xType );


static typelib_TypeDescription * createCTD(
    const Reference< XCompoundTypeDescription > & xType )
{
    typelib_TypeDescription * pRet = nullptr;
    if (xType.is())
    {
        typelib_TypeDescription * pBaseType = createCTD(
            Reference< XCompoundTypeDescription >::query( xType->getBaseType() ) );
        if (pBaseType)
            typelib_typedescription_register( &pBaseType );

        // construct member init array
        const Sequence<Reference< XTypeDescription > > aMemberTypes = xType->getMemberTypes();
        const Sequence< OUString > aMemberNames                     = xType->getMemberNames();

        sal_Int32 nMembers = aMemberTypes.getLength();
        OSL_ENSURE( nMembers == aMemberNames.getLength(), "### lens differ!" );

        OUString aTypeName( xType->getName() );

        typelib_CompoundMember_Init * pMemberInits = static_cast<typelib_CompoundMember_Init *>(alloca(
            sizeof(typelib_CompoundMember_Init) * nMembers ));

        sal_Int32 nPos;
        for ( nPos = nMembers; nPos--; )
        {
            typelib_CompoundMember_Init & rInit = pMemberInits[nPos];
            rInit.eTypeClass = static_cast<typelib_TypeClass>(aMemberTypes[nPos]->getTypeClass());

            OUString aMemberTypeName(aMemberTypes[nPos]->getName());
            rInit.pTypeName = aMemberTypeName.pData;
            rtl_uString_acquire( rInit.pTypeName );

            // string is held by rMemberNames
            rInit.pMemberName = aMemberNames[nPos].pData;
        }

        typelib_typedescription_new(
            &pRet,
            static_cast<typelib_TypeClass>(xType->getTypeClass()),
            aTypeName.pData,
            (pBaseType ? pBaseType->pWeakRef : nullptr),
            nMembers, pMemberInits );

        // cleanup
        for ( nPos = nMembers; nPos--; )
        {
            rtl_uString_release( pMemberInits[nPos].pTypeName );
        }
        if (pBaseType)
            typelib_typedescription_release( pBaseType );
    }
    return pRet;
}

static typelib_TypeDescription * createCTD(
    Reference< container::XHierarchicalNameAccess > const & access,
    const Reference< XStructTypeDescription > & xType )
{
    typelib_TypeDescription * pRet = nullptr;
    if (xType.is() && !xType->getTypeParameters().hasElements())
    {
        typelib_TypeDescription * pBaseType = createCTD(
            access, xType->getBaseType() );
        if (pBaseType)
            typelib_typedescription_register( &pBaseType );

        // construct member init array
        const Sequence<Reference< XTypeDescription > > aMemberTypes = xType->getMemberTypes();
        const Sequence< OUString > aMemberNames                     = xType->getMemberNames();

        sal_Int32 nMembers = aMemberTypes.getLength();
        OSL_ENSURE( nMembers == aMemberNames.getLength(), "### lens differ!" );

        OUString aTypeName( xType->getName() );

        typelib_StructMember_Init * pMemberInits = static_cast<typelib_StructMember_Init *>(alloca(
            sizeof(typelib_StructMember_Init) * nMembers ));

        Sequence< Reference< XTypeDescription > > templateMemberTypes;
        sal_Int32 i = aTypeName.indexOf('<');
        if (i >= 0) {
            Reference< XStructTypeDescription > templateDesc(
                access->getByHierarchicalName(aTypeName.copy(0, i)),
                UNO_QUERY_THROW);
            OSL_ASSERT(
                templateDesc->getTypeParameters().getLength()
                == xType->getTypeArguments().getLength());
            templateMemberTypes = templateDesc->getMemberTypes();
            OSL_ASSERT(templateMemberTypes.getLength() == nMembers);
        }

        sal_Int32 nPos;
        for ( nPos = nMembers; nPos--; )
        {
            typelib_StructMember_Init & rInit = pMemberInits[nPos];
            rInit.aBase.eTypeClass
                = static_cast<typelib_TypeClass>(aMemberTypes[nPos]->getTypeClass());

            OUString aMemberTypeName(aMemberTypes[nPos]->getName());
            rInit.aBase.pTypeName = aMemberTypeName.pData;
            rtl_uString_acquire( rInit.aBase.pTypeName );

            // string is held by rMemberNames
            rInit.aBase.pMemberName = aMemberNames[nPos].pData;

            rInit.bParameterizedType = templateMemberTypes.hasElements()
                && (templateMemberTypes[nPos]->getTypeClass()
                    == TypeClass_UNKNOWN);
        }

        typelib_typedescription_newStruct(
            &pRet,
            aTypeName.pData,
            (pBaseType ? pBaseType->pWeakRef : nullptr),
            nMembers, pMemberInits );

        // cleanup
        for ( nPos = nMembers; nPos--; )
        {
            rtl_uString_release( pMemberInits[nPos].aBase.pTypeName );
        }
        if (pBaseType)
            typelib_typedescription_release( pBaseType );
    }
    return pRet;
}

static typelib_TypeDescription * createCTD(
    const Reference< XInterfaceAttributeTypeDescription2 > & xAttribute )
{
    typelib_TypeDescription * pRet = nullptr;
    if (xAttribute.is())
    {
        OUString aMemberName( xAttribute->getName() );
        Reference< XTypeDescription > xType( xAttribute->getType() );
        OUString aMemberTypeName( xType->getName() );
        std::vector< rtl_uString * > getExc;
        const Sequence< Reference< XCompoundTypeDescription > > getExcs(
            xAttribute->getGetExceptions() );
        for (const auto & ctd : getExcs)
        {
            OSL_ASSERT( ctd.is() );
            getExc.push_back( ctd->getName().pData );
        }
        std::vector< rtl_uString * > setExc;
        const Sequence< Reference< XCompoundTypeDescription > > setExcs(
            xAttribute->getSetExceptions() );
        for (const auto & ctd : setExcs)
        {
            OSL_ASSERT( ctd.is() );
            setExc.push_back( ctd->getName().pData );
        }
        typelib_typedescription_newExtendedInterfaceAttribute(
            reinterpret_cast<typelib_InterfaceAttributeTypeDescription **>(&pRet),
            xAttribute->getPosition(),
            aMemberName.pData, // name
            static_cast<typelib_TypeClass>(xType->getTypeClass()),
            aMemberTypeName.pData, // type name
            xAttribute->isReadOnly(),
            getExc.size(), getExc.data(),
            setExc.size(), setExc.data() );
    }
    return pRet;
}

static typelib_TypeDescription * createCTD(
    const Reference< XInterfaceMethodTypeDescription > & xMethod )
{
    typelib_TypeDescription * pRet = nullptr;
    if (xMethod.is())
    {
        Reference< XTypeDescription > xReturnType( xMethod->getReturnType() );

        // init all params
        const Sequence<Reference< XMethodParameter > > aParams = xMethod->getParameters();
        sal_Int32 nParams = aParams.getLength();

        typelib_Parameter_Init * pParamInit = static_cast<typelib_Parameter_Init *>(alloca(
            sizeof(typelib_Parameter_Init) * nParams ));

        sal_Int32 nPos;
        for ( nPos = nParams; nPos--; )
        {
            const Reference<XMethodParameter>& xParam = aParams[nPos];
            const Reference< XTypeDescription > xType  = xParam->getType();
            typelib_Parameter_Init & rInit = pParamInit[xParam->getPosition()];

            rInit.eTypeClass = static_cast<typelib_TypeClass>(xType->getTypeClass());
            OUString aParamTypeName( xType->getName() );
            rInit.pTypeName = aParamTypeName.pData;
            rtl_uString_acquire( rInit.pTypeName );
            OUString aParamName( xParam->getName() );
            rInit.pParamName = aParamName.pData;
            rtl_uString_acquire( rInit.pParamName );
            rInit.bIn  = xParam->isIn();
            rInit.bOut = xParam->isOut();
        }

        // init all exception strings
        const Sequence<Reference< XTypeDescription > > aExceptions = xMethod->getExceptions();
        sal_Int32 nExceptions = aExceptions.getLength();
        rtl_uString ** ppExceptionNames = static_cast<rtl_uString **>(alloca(
            sizeof(rtl_uString *) * nExceptions ));

        for ( nPos = nExceptions; nPos--; )
        {
            OUString aExceptionTypeName(aExceptions[nPos]->getName());
            ppExceptionNames[nPos] = aExceptionTypeName.pData;
            rtl_uString_acquire( ppExceptionNames[nPos] );
        }

        OUString aTypeName( xMethod->getName() );
        OUString aReturnTypeName( xReturnType->getName() );

        typelib_typedescription_newInterfaceMethod(
            reinterpret_cast<typelib_InterfaceMethodTypeDescription **>(&pRet),
            xMethod->getPosition(),
            xMethod->isOneway(),
            aTypeName.pData,
            static_cast<typelib_TypeClass>(xReturnType->getTypeClass()),
            aReturnTypeName.pData,
            nParams, pParamInit,
            nExceptions, ppExceptionNames );

        for ( nPos = nParams; nPos--; )
        {
            rtl_uString_release( pParamInit[nPos].pTypeName );
            rtl_uString_release( pParamInit[nPos].pParamName );
        }
        for ( nPos = nExceptions; nPos--; )
        {
            rtl_uString_release( ppExceptionNames[nPos] );
        }
    }
    return pRet;
}

static typelib_TypeDescription * createCTD(
    Reference< container::XHierarchicalNameAccess > const & access,
    const Reference< XInterfaceTypeDescription2 > & xType )
{
    typelib_TypeDescription * pRet = nullptr;
    if (xType.is())
    {
        Sequence< Reference< XTypeDescription > > aBases(xType->getBaseTypes());
        sal_Int32 nBases = aBases.getLength();
        // Exploit the fact that a typelib_TypeDescription for an interface type
        // is also the typelib_TypeDescriptionReference for that type:
        std::unique_ptr< typelib_TypeDescription * []> aBaseTypes(
            new typelib_TypeDescription *[nBases]);
        for (sal_Int32 i = 0; i < nBases; ++i) {
            typelib_TypeDescription * p = createCTD(access, aBases[i]);
            OSL_ASSERT(
                !TYPELIB_TYPEDESCRIPTIONREFERENCE_ISREALLYWEAK(p->eTypeClass));
            typelib_typedescription_register(&p);
            aBaseTypes[i] = p;
        }
        typelib_TypeDescriptionReference ** pBaseTypeRefs
            = reinterpret_cast< typelib_TypeDescriptionReference ** >(
                aBaseTypes.get());

        // construct all member refs
        const Sequence<Reference< XInterfaceMemberTypeDescription > > aMembers = xType->getMembers();
        sal_Int32 nMembers = aMembers.getLength();

        typelib_TypeDescriptionReference ** ppMemberRefs = static_cast<typelib_TypeDescriptionReference **>(alloca(
            sizeof(typelib_TypeDescriptionReference *) * nMembers ));

        OUString aTypeName( xType->getName() );

        sal_Int32 nPos;
        for ( nPos = nMembers; nPos--; )
        {
            OUString aMemberTypeName(aMembers[nPos]->getName());
            ppMemberRefs[nPos] = nullptr;
            typelib_typedescriptionreference_new(
                ppMemberRefs + nPos,
                static_cast<typelib_TypeClass>(aMembers[nPos]->getTypeClass()),
                aMemberTypeName.pData );
        }

        typelib_typedescription_newMIInterface(
            reinterpret_cast<typelib_InterfaceTypeDescription **>(&pRet),
            aTypeName.pData,
            0, 0, 0, 0, 0,
            nBases, pBaseTypeRefs,
            nMembers, ppMemberRefs );

        // cleanup refs and base type
        for (int i = 0; i < nBases; ++i) {
            typelib_typedescription_release(aBaseTypes[i]);
        }

        for ( nPos = nMembers; nPos--; )
        {
            typelib_typedescriptionreference_release( ppMemberRefs[nPos] );
        }
    }
    return pRet;
}

static typelib_TypeDescription * createCTD( const Reference< XEnumTypeDescription > & xType )
{
    typelib_TypeDescription * pRet = nullptr;
    if (xType.is())
    {
        OUString aTypeName( xType->getName() );
        Sequence< OUString > aNames( xType->getEnumNames() );
        OSL_ASSERT( sizeof(OUString) == sizeof(rtl_uString *) ); // !!!
        Sequence< sal_Int32 > aValues( xType->getEnumValues() );

        typelib_typedescription_newEnum(
            &pRet, aTypeName.pData, xType->getDefaultEnumValue(),
            aNames.getLength(),
            const_cast<rtl_uString **>(reinterpret_cast<rtl_uString * const *>(aNames.getConstArray())),
            const_cast< sal_Int32 * >( aValues.getConstArray() ) );
    }
    return pRet;
}

static typelib_TypeDescription * createCTD(
    Reference< container::XHierarchicalNameAccess > const & access,
    const Reference< XIndirectTypeDescription > & xType )
{
    typelib_TypeDescription * pRet = nullptr;
    if (xType.is())
    {
        typelib_TypeDescription * pRefType = createCTD(
            access, xType->getReferencedType() );
        typelib_typedescription_register( &pRefType );

        OUString aTypeName( xType->getName() );

        typelib_typedescription_new(
            &pRet,
            static_cast<typelib_TypeClass>(xType->getTypeClass()),
            aTypeName.pData,
            pRefType->pWeakRef,
            0, nullptr );

        // cleanup
        typelib_typedescription_release( pRefType );
    }
    return pRet;
}


static typelib_TypeDescription * createCTD(
    Reference< container::XHierarchicalNameAccess > const & access,
    const Reference< XTypeDescription > & xType )
{
    typelib_TypeDescription * pRet = nullptr;

    if (xType.is())
    {
        switch (xType->getTypeClass())
        {
            // built in types
        case TypeClass_VOID:
        {
            OUString aTypeName(u"void"_ustr);
            typelib_typedescription_new( &pRet, typelib_TypeClass_VOID, aTypeName.pData, nullptr, 0, nullptr );
            break;
        }
        case TypeClass_CHAR:
        {
            OUString aTypeName(u"char"_ustr);
            typelib_typedescription_new( &pRet, typelib_TypeClass_CHAR, aTypeName.pData, nullptr, 0, nullptr );
            break;
        }
        case TypeClass_BOOLEAN:
        {
            OUString aTypeName(u"boolean"_ustr);
            typelib_typedescription_new( &pRet, typelib_TypeClass_BOOLEAN, aTypeName.pData, nullptr, 0, nullptr );
            break;
        }
        case TypeClass_BYTE:
        {
            OUString aTypeName(u"byte"_ustr);
            typelib_typedescription_new( &pRet, typelib_TypeClass_BYTE, aTypeName.pData, nullptr, 0, nullptr );
            break;
        }
        case TypeClass_SHORT:
        {
            OUString aTypeName(u"short"_ustr);
            typelib_typedescription_new( &pRet, typelib_TypeClass_SHORT, aTypeName.pData, nullptr, 0, nullptr );
            break;
        }
        case TypeClass_UNSIGNED_SHORT:
        {
            OUString aTypeName(u"unsigned short"_ustr);
            typelib_typedescription_new( &pRet, typelib_TypeClass_UNSIGNED_SHORT, aTypeName.pData, nullptr, 0, nullptr );
            break;
        }
        case TypeClass_LONG:
        {
            OUString aTypeName(u"long"_ustr);
            typelib_typedescription_new( &pRet, typelib_TypeClass_LONG, aTypeName.pData, nullptr, 0, nullptr );
            break;
        }
        case TypeClass_UNSIGNED_LONG:
        {
            OUString aTypeName(u"unsigned long"_ustr);
            typelib_typedescription_new( &pRet, typelib_TypeClass_UNSIGNED_LONG, aTypeName.pData, nullptr, 0, nullptr );
            break;
        }
        case TypeClass_HYPER:
        {
            OUString aTypeName(u"hyper"_ustr);
            typelib_typedescription_new( &pRet, typelib_TypeClass_HYPER, aTypeName.pData, nullptr, 0, nullptr );
            break;
        }
        case TypeClass_UNSIGNED_HYPER:
        {
            OUString aTypeName(u"unsigned hyper"_ustr);
            typelib_typedescription_new( &pRet, typelib_TypeClass_UNSIGNED_HYPER, aTypeName.pData, nullptr, 0, nullptr );
            break;
        }
        case TypeClass_FLOAT:
        {
            OUString aTypeName(u"float"_ustr);
            typelib_typedescription_new( &pRet, typelib_TypeClass_FLOAT, aTypeName.pData, nullptr, 0, nullptr );
            break;
        }
        case TypeClass_DOUBLE:
        {
            OUString aTypeName(u"double"_ustr);
            typelib_typedescription_new( &pRet, typelib_TypeClass_DOUBLE, aTypeName.pData, nullptr, 0, nullptr );
            break;
        }
        case TypeClass_STRING:
        {
            OUString aTypeName(u"string"_ustr);
            typelib_typedescription_new( &pRet, typelib_TypeClass_STRING, aTypeName.pData, nullptr, 0, nullptr );
            break;
        }
        case TypeClass_TYPE:
        {
            OUString aTypeName(u"type"_ustr);
            typelib_typedescription_new( &pRet, typelib_TypeClass_TYPE, aTypeName.pData, nullptr, 0, nullptr );
            break;
        }
        case TypeClass_ANY:
        {
            OUString aTypeName(u"any"_ustr);
            typelib_typedescription_new( &pRet, typelib_TypeClass_ANY, aTypeName.pData, nullptr, 0, nullptr );
            break;
        }

        case TypeClass_EXCEPTION:
            pRet = createCTD( Reference< XCompoundTypeDescription >::query( xType ) );
            break;
        case TypeClass_STRUCT:
            pRet = createCTD(
                access, Reference< XStructTypeDescription >::query( xType ) );
            break;
        case TypeClass_ENUM:
            pRet = createCTD( Reference< XEnumTypeDescription >::query( xType ) );
            break;
        case TypeClass_TYPEDEF:
        {
            Reference< XIndirectTypeDescription > xTypedef( xType, UNO_QUERY );
            if (xTypedef.is())
                pRet = createCTD( access, xTypedef->getReferencedType() );
            break;
        }
        case TypeClass_SEQUENCE:
            pRet = createCTD(
                access, Reference< XIndirectTypeDescription >::query( xType ) );
            break;
        case TypeClass_INTERFACE:
            pRet = createCTD(
                access,
                Reference< XInterfaceTypeDescription2 >::query( xType ) );
            break;
        case TypeClass_INTERFACE_METHOD:
            pRet = createCTD( Reference< XInterfaceMethodTypeDescription >::query( xType ) );
            break;
        case TypeClass_INTERFACE_ATTRIBUTE:
            pRet = createCTD( Reference< XInterfaceAttributeTypeDescription2 >::query( xType ) );
            break;
        default:
            break;
        }
    }

    return pRet;
}


extern "C"
{
static void typelib_callback(
    void * pContext, typelib_TypeDescription ** ppRet, rtl_uString * pTypeName )
{
    OSL_ENSURE( pContext && ppRet && pTypeName, "### null ptr!" );
    if (!ppRet)
        return;

    if (*ppRet)
    {
        ::typelib_typedescription_release( *ppRet );
        *ppRet = nullptr;
    }
    if (!(pContext && pTypeName))
        return;

    Reference< container::XHierarchicalNameAccess > access(
        static_cast< container::XHierarchicalNameAccess * >(
            pContext));
    try
    {
        OUString const & rTypeName = OUString::unacquired( &pTypeName );
        Reference< XTypeDescription > xTD;
        if (access->getByHierarchicalName(rTypeName ) >>= xTD)
        {
            *ppRet = createCTD( access, xTD );
        }
    }
    catch (const container::NoSuchElementException & exc)
    {
        SAL_INFO("cppuhelper", "typelibrary type not available: " << exc );
    }
    catch (const Exception & exc)
    {
        SAL_INFO("cppuhelper", exc );
    }
}
}

namespace {

class EventListenerImpl
    : public WeakImplHelper< lang::XEventListener >
{
    Reference< container::XHierarchicalNameAccess > m_xTDMgr;

public:
    explicit EventListenerImpl(
        Reference< container::XHierarchicalNameAccess > const & xTDMgr )
        : m_xTDMgr( xTDMgr )
        {}

    // XEventListener
    virtual void SAL_CALL disposing( lang::EventObject const & rEvt ) override;
};

}

void EventListenerImpl::disposing( lang::EventObject const & rEvt )
{
    if (rEvt.Source != m_xTDMgr) {
        OSL_ASSERT(false);
    }
    // deregister of c typelib callback
    ::typelib_typedescription_revokeCallback( m_xTDMgr.get(), typelib_callback );
}


sal_Bool SAL_CALL installTypeDescriptionManager(
    Reference< container::XHierarchicalNameAccess > const & xTDMgr_c )
{
    uno::Environment curr_env(Environment::getCurrent());
    uno::Environment target_env(CPPU_CURRENT_LANGUAGE_BINDING_NAME);

    uno::Mapping curr2target(curr_env, target_env);


    Reference<container::XHierarchicalNameAccess> xTDMgr(
        static_cast<container::XHierarchicalNameAccess *>(
            curr2target.mapInterface(xTDMgr_c.get(), cppu::UnoType<decltype(xTDMgr_c)>::get())),
        SAL_NO_ACQUIRE);

    Reference< lang::XComponent > xComp( xTDMgr, UNO_QUERY );
    if (xComp.is())
    {
        xComp->addEventListener( new EventListenerImpl( xTDMgr ) );
        // register c typelib callback
        ::typelib_typedescription_registerCallback( xTDMgr.get(), typelib_callback );
        return true;
    }
    return false;
}

} // end namespace cppu

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
