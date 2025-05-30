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


#include <algorithm>
#include <unordered_map>
#include <cassert>
#include <list>
#include <set>
#include <utility>
#include <vector>

#include <stdlib.h>
#include <string.h>
#include <sal/log.hxx>
#include <osl/interlck.h>
#include <osl/mutex.hxx>
#include <rtl/ustring.hxx>
#include <osl/diagnose.h>
#include <typelib/typedescription.h>
#include <uno/any2.h>
#include <o3tl/string_view.hxx>
#include "typelib.hxx"

using namespace osl;

#ifdef _WIN32
#pragma pack(push, 8)
#endif

namespace {

/**
 * The double member determines the alignment.
 * Under OS2 and MS-Windows the Alignment is min( 8, sizeof( type ) ).
 * The alignment of a structure is min( 8, sizeof( max basic type ) ), the greatest basic type
 * determines the alignment.
 */
struct AlignSize_Impl
{
    sal_Int16 nInt16;
    double dDouble;
};

}

#ifdef _WIN32
#pragma pack(pop)
#endif

// the value of the maximal alignment
const sal_Int32 nMaxAlignment = static_cast<sal_Int32>( reinterpret_cast<sal_Size>(&reinterpret_cast<AlignSize_Impl *>(16)->dDouble) - 16);

static sal_Int32 adjustAlignment( sal_Int32 nRequestedAlignment )
{
    if( nRequestedAlignment > nMaxAlignment )
        nRequestedAlignment = nMaxAlignment;
    return nRequestedAlignment;
}

/**
 * Calculate the new size of the structure.
 */
static sal_Int32 newAlignedSize(
    sal_Int32 OldSize, sal_Int32 ElementSize, sal_Int32 NeededAlignment )
{
    NeededAlignment = adjustAlignment( NeededAlignment );
    return (OldSize + NeededAlignment -1) / NeededAlignment * NeededAlignment + ElementSize;
}

static sal_Int32 getDescriptionSize( typelib_TypeClass eTypeClass )
{
    OSL_ASSERT( typelib_TypeClass_TYPEDEF != eTypeClass );

    sal_Int32 nSize;
    // The reference is the description
    // if the description is empty, then it must be filled with
    // the new description
    switch( eTypeClass )
    {
        case typelib_TypeClass_SEQUENCE:
            nSize = sal_Int32(sizeof( typelib_IndirectTypeDescription ));
        break;

        case typelib_TypeClass_STRUCT:
            nSize = sal_Int32(sizeof( typelib_StructTypeDescription ));
        break;

        case typelib_TypeClass_EXCEPTION:
            nSize = sal_Int32(sizeof( typelib_CompoundTypeDescription ));
        break;

        case typelib_TypeClass_ENUM:
            nSize = sal_Int32(sizeof( typelib_EnumTypeDescription ));
        break;

        case typelib_TypeClass_INTERFACE:
            nSize = sal_Int32(sizeof( typelib_InterfaceTypeDescription ));
        break;

        case typelib_TypeClass_INTERFACE_METHOD:
            nSize = sal_Int32(sizeof( typelib_InterfaceMethodTypeDescription ));
        break;

        case typelib_TypeClass_INTERFACE_ATTRIBUTE:
            nSize = sal_Int32(sizeof( typelib_InterfaceAttributeTypeDescription ));
        break;

        default:
            nSize = sal_Int32(sizeof( typelib_TypeDescription ));
    }
    return nSize;
}

namespace {

struct equalStr_Impl
{
    bool operator()(const sal_Unicode * const & s1, const sal_Unicode * const & s2) const
        { return 0 == rtl_ustr_compare( s1, s2 ); }
};


struct hashStr_Impl
{
    size_t operator()(const sal_Unicode * const & s) const
        { return rtl_ustr_hashCode( s ); }
};

}

// Heavy hack, the const sal_Unicode * is hold by the typedescription reference
typedef std::unordered_map< const sal_Unicode *, typelib_TypeDescriptionReference *,
                  hashStr_Impl, equalStr_Impl > WeakMap_Impl;

typedef std::pair< void *, typelib_typedescription_Callback > CallbackEntry;
typedef std::list< CallbackEntry > CallbackSet_Impl;
typedef std::list< typelib_TypeDescription * > TypeDescriptionList_Impl;

// # of cached elements
constexpr auto nCacheSize = 256;

namespace {

struct TypeDescriptor_Init_Impl
{
    // all type description references
    WeakMap_Impl maWeakMap;
    // all type description callbacks
    CallbackSet_Impl maCallbacks;
    // A cache to hold descriptions
    TypeDescriptionList_Impl maCache;
    // The mutex to guard all type library accesses
    Mutex      maMutex;

    inline void callChain( typelib_TypeDescription ** ppRet, rtl_uString * pName );

#if OSL_DEBUG_LEVEL > 0
    // only for debugging
    sal_Int32 nTypeDescriptionCount = 0;
    sal_Int32 nCompoundTypeDescriptionCount = 0;
    sal_Int32 nIndirectTypeDescriptionCount = 0;
    sal_Int32 nEnumTypeDescriptionCount = 0;
    sal_Int32 nInterfaceMethodTypeDescriptionCount = 0;
    sal_Int32 nInterfaceAttributeTypeDescriptionCount = 0;
    sal_Int32 nInterfaceTypeDescriptionCount = 0;
    sal_Int32 nTypeDescriptionReferenceCount = 0;
#endif

    TypeDescriptor_Init_Impl() = default;

    ~TypeDescriptor_Init_Impl();
};

}

inline void TypeDescriptor_Init_Impl::callChain(
    typelib_TypeDescription ** ppRet, rtl_uString * pName )
{
    assert(ppRet != nullptr);
    assert(*ppRet == nullptr);
    for( const CallbackEntry & rEntry : maCallbacks )
    {
        (*rEntry.second)( rEntry.first, ppRet, pName );
        if( *ppRet )
            return;
    }
}


TypeDescriptor_Init_Impl::~TypeDescriptor_Init_Impl()
{
    for( typelib_TypeDescription* pItem : maCache )
    {
        typelib_typedescription_release( pItem );
    }

    {
        std::vector< typelib_TypeDescriptionReference * > ppTDR;
        ppTDR.reserve( maWeakMap.size() );

        // save all weak references
        for( const auto& rEntry : maWeakMap )
        {
            ppTDR.push_back( rEntry.second );
            typelib_typedescriptionreference_acquire( ppTDR.back() );
        }

        for( typelib_TypeDescriptionReference * pTDR : ppTDR )
        {
            OSL_ASSERT( pTDR->nRefCount > pTDR->nStaticRefCount );
            pTDR->nRefCount -= pTDR->nStaticRefCount;

            if( pTDR->pType && !pTDR->pType->bOnDemand )
            {
                pTDR->pType->bOnDemand = true;
                typelib_typedescription_release( pTDR->pType );
            }
            typelib_typedescriptionreference_release( pTDR );
        }

#if defined SAL_LOG_INFO
        for( const auto& rEntry : maWeakMap )
        {
            typelib_TypeDescriptionReference * pTDR = rEntry.second;
            if (pTDR)
            {
                OString aTypeName( OUStringToOString( OUString::unacquired(&pTDR->pTypeName), RTL_TEXTENCODING_ASCII_US ) );
                SAL_INFO("cppu.typelib", "remaining type: " << aTypeName << "; ref count = " << pTDR->nRefCount);
            }
            else
            {
                SAL_INFO("cppu.typelib", "remaining null type entry!?");
            }
        }
#endif
    }
#if OSL_DEBUG_LEVEL > 0
    SAL_INFO_IF( nTypeDescriptionCount, "cppu.typelib", "nTypeDescriptionCount is not zero" );
    SAL_INFO_IF( nCompoundTypeDescriptionCount, "cppu.typelib", "nCompoundTypeDescriptionCount is not zero" );
    SAL_INFO_IF( nIndirectTypeDescriptionCount, "cppu.typelib", "nIndirectTypeDescriptionCount is not zero" );
    SAL_INFO_IF( nEnumTypeDescriptionCount, "cppu.typelib", "nEnumTypeDescriptionCount is not zero" );
    SAL_INFO_IF( nInterfaceMethodTypeDescriptionCount, "cppu.typelib", "nInterfaceMethodTypeDescriptionCount is not zero" );
    SAL_INFO_IF( nInterfaceAttributeTypeDescriptionCount, "cppu.typelib", "nInterfaceAttributeTypeDescriptionCount is not zero" );
    SAL_INFO_IF( nInterfaceTypeDescriptionCount, "cppu.typelib", "nInterfaceTypeDescriptionCount is not zero" );
    SAL_INFO_IF( nTypeDescriptionReferenceCount, "cppu.typelib", "nTypeDescriptionReferenceCount is not zero" );
#endif

    SAL_INFO_IF( !maCallbacks.empty(), "cppu.typelib", "pCallbacks is not NULL or empty" );
};

namespace {
TypeDescriptor_Init_Impl& Init()
{
    static TypeDescriptor_Init_Impl SINGLETON;
    return SINGLETON;
}
}

extern "C" void SAL_CALL typelib_typedescription_registerCallback(
    void * pContext, typelib_typedescription_Callback pCallback ) noexcept
{
    // todo mt safe: guard is no solution, can not acquire while calling callback!
    TypeDescriptor_Init_Impl &rInit = Init();
//      OslGuard aGuard( rInit.getMutex() );
    rInit.maCallbacks.push_back( CallbackEntry( pContext, pCallback ) );
}


extern "C" void SAL_CALL typelib_typedescription_revokeCallback(
    void * pContext, typelib_typedescription_Callback pCallback ) noexcept
{
    TypeDescriptor_Init_Impl &rInit = Init();
    {
        // todo mt safe: guard is no solution, can not acquire while calling callback!
//          OslGuard aGuard( rInit.getMutex() );
        CallbackEntry aEntry( pContext, pCallback );
        std::erase(rInit.maCallbacks, aEntry);
    }
}

static void typelib_typedescription_initTables(
    typelib_TypeDescription * pTD )
{
    typelib_InterfaceTypeDescription * pITD = reinterpret_cast<typelib_InterfaceTypeDescription *>(pTD);

    std::vector<bool> aReadWriteAttributes(pITD->nAllMembers);
    for ( sal_Int32 i = pITD->nAllMembers; i--; )
    {
        aReadWriteAttributes[i] = false;
        if( typelib_TypeClass_INTERFACE_ATTRIBUTE == pITD->ppAllMembers[i]->eTypeClass )
        {
            typelib_TypeDescription * pM = nullptr;
            TYPELIB_DANGER_GET( &pM, pITD->ppAllMembers[i] );
            OSL_ASSERT( pM );
            if (pM)
            {
                aReadWriteAttributes[i] = !reinterpret_cast<typelib_InterfaceAttributeTypeDescription *>(pM)->bReadOnly;
                TYPELIB_DANGER_RELEASE( pM );
            }
            else
            {
                SAL_INFO( "cppu.typelib", "cannot get attribute type description: " <<  pITD->ppAllMembers[i]->pTypeName );
            }
        }
    }

    MutexGuard aGuard( Init().maMutex );
    if( pTD->bComplete )
        return;

    // create the index table from member to function table
    pITD->pMapMemberIndexToFunctionIndex = new sal_Int32[ pITD->nAllMembers ];
    sal_Int32 nAdditionalOffset = 0; // +1 for read/write attributes
    sal_Int32 i;
    for( i = 0; i < pITD->nAllMembers; i++ )
    {
        // index to the get method of the attribute
        pITD->pMapMemberIndexToFunctionIndex[i] = i + nAdditionalOffset;
        // extra offset if it is a read/write attribute?
        if (aReadWriteAttributes[i])
        {
            // a read/write attribute
            nAdditionalOffset++;
        }
    }

    // create the index table from function to member table
    pITD->pMapFunctionIndexToMemberIndex = new sal_Int32[ pITD->nAllMembers + nAdditionalOffset ];
    nAdditionalOffset = 0; // +1 for read/write attributes
    for( i = 0; i < pITD->nAllMembers; i++ )
    {
        // index to the get method of the attribute
        pITD->pMapFunctionIndexToMemberIndex[i + nAdditionalOffset] = i;
        // extra offset if it is a read/write attribute?
        if (aReadWriteAttributes[i])
        {
            // a read/write attribute
            pITD->pMapFunctionIndexToMemberIndex[i + ++nAdditionalOffset] = i;
        }
    }
    // must be the last action after all initialization is done
    pITD->nMapFunctionIndexToMemberIndex = pITD->nAllMembers + nAdditionalOffset;
    pTD->bComplete = true;
}

namespace {

template<typename T> T * allocTypeDescription() {
    return reinterpret_cast<T *>(new char[sizeof (T)]);
}

void freeTypeDescription(typelib_TypeDescription const * desc) {
    delete[] reinterpret_cast<char const *>(desc);
}

// In some situations (notably typelib_typedescription_newInterfaceMethod and
// typelib_typedescription_newInterfaceAttribute), only the members nMembers,
// ppMembers, nAllMembers, and ppAllMembers of an incomplete interface type
// description are necessary, but not the additional
// pMapMemberIndexToFunctionIndex, nMapFunctionIndexToMemberIndex, and
// pMapFunctionIndexToMemberIndex (which are computed by
// typelib_typedescription_initTables).  Furthermore, in those situations, it
// might be illegal to compute those tables, as the creation of the interface
// member type descriptions would recursively require a complete interface type
// description.  The parameter initTables controls whether or not to call
// typelib_typedescription_initTables in those situations.
bool complete(typelib_TypeDescription ** ppTypeDescr, bool initTables) {
    if ((*ppTypeDescr)->bComplete)
        return true;

    OSL_ASSERT( (typelib_TypeClass_STRUCT == (*ppTypeDescr)->eTypeClass ||
                 typelib_TypeClass_EXCEPTION == (*ppTypeDescr)->eTypeClass ||
                 typelib_TypeClass_ENUM == (*ppTypeDescr)->eTypeClass ||
                 typelib_TypeClass_INTERFACE == (*ppTypeDescr)->eTypeClass) &&
                !TYPELIB_TYPEDESCRIPTIONREFERENCE_ISREALLYWEAK( (*ppTypeDescr)->eTypeClass ) );

    if (typelib_TypeClass_INTERFACE == (*ppTypeDescr)->eTypeClass &&
        reinterpret_cast<typelib_InterfaceTypeDescription *>(*ppTypeDescr)->ppAllMembers)
    {
        if (initTables) {
            typelib_typedescription_initTables( *ppTypeDescr );
        }
        return true;
    }

    typelib_TypeDescription * pTD = nullptr;
    // on demand access of complete td
    TypeDescriptor_Init_Impl &rInit = Init();
    rInit.callChain( &pTD, (*ppTypeDescr)->pTypeName );
    if (pTD)
    {
        if (typelib_TypeClass_TYPEDEF == pTD->eTypeClass)
        {
            typelib_typedescriptionreference_getDescription(
                &pTD, reinterpret_cast<typelib_IndirectTypeDescription *>(pTD)->pType );
            OSL_ASSERT( pTD );
            if (! pTD)
                return false;
        }

        OSL_ASSERT( typelib_TypeClass_TYPEDEF != pTD->eTypeClass );
        // typedescription found
        // set to on demand
        pTD->bOnDemand = true;

        if (pTD->eTypeClass == typelib_TypeClass_INTERFACE
            && !pTD->bComplete && initTables)
        {
            // mandatory info from callback chain
            OSL_ASSERT( reinterpret_cast<typelib_InterfaceTypeDescription *>(pTD)->ppAllMembers );
            // complete except of tables init
            typelib_typedescription_initTables( pTD );
            pTD->bComplete = true;
        }

        // The type description is hold by the reference until
        // on demand is activated.
        ::typelib_typedescription_register( &pTD ); // replaces incomplete one
        OSL_ASSERT( pTD == *ppTypeDescr ); // has to merge into existing one

        // insert into the cache
        MutexGuard aGuard( rInit.maMutex );
        if( rInit.maCache.size() >= nCacheSize )
        {
            typelib_typedescription_release( rInit.maCache.front() );
            rInit.maCache.pop_front();
        }
        // descriptions in the cache must be acquired!
        typelib_typedescription_acquire( pTD );
        rInit.maCache.push_back( pTD );

        OSL_ASSERT(
            pTD->bComplete
            || (pTD->eTypeClass == typelib_TypeClass_INTERFACE
                && !initTables));

        ::typelib_typedescription_release( *ppTypeDescr );
        *ppTypeDescr = pTD;
    }
    else
    {
        SAL_INFO(
            "cppu.typelib",
            "type cannot be completed: " << OUString::unacquired(&(*ppTypeDescr)->pTypeName));
        return false;
    }
    return true;
}

}


extern "C" void typelib_typedescription_newEmpty(
    typelib_TypeDescription ** ppRet,
    typelib_TypeClass eTypeClass, rtl_uString * pTypeName ) noexcept
{
    if( *ppRet )
    {
        typelib_typedescription_release( *ppRet );
        *ppRet = nullptr;
    }

    OSL_ASSERT( typelib_TypeClass_TYPEDEF != eTypeClass );

    typelib_TypeDescription * pRet;
    switch( eTypeClass )
    {
        case typelib_TypeClass_SEQUENCE:
        {
            auto pTmp = allocTypeDescription<typelib_IndirectTypeDescription>();
            pRet = &pTmp->aBase;
#if OSL_DEBUG_LEVEL > 0
            osl_atomic_increment( &Init().nIndirectTypeDescriptionCount );
#endif
            pTmp->pType = nullptr;
            // coverity[leaked_storage] - this is on purpose
        }
        break;

        case typelib_TypeClass_STRUCT:
        {
            // FEATURE_EMPTYCLASS
            auto pTmp = allocTypeDescription<typelib_StructTypeDescription>();
            pRet = &pTmp->aBase.aBase;
#if OSL_DEBUG_LEVEL > 0
            osl_atomic_increment( &Init().nCompoundTypeDescriptionCount );
#endif
            pTmp->aBase.pBaseTypeDescription = nullptr;
            pTmp->aBase.nMembers = 0;
            pTmp->aBase.pMemberOffsets = nullptr;
            pTmp->aBase.ppTypeRefs = nullptr;
            pTmp->aBase.ppMemberNames = nullptr;
            pTmp->pParameterizedTypes = nullptr;
            // coverity[leaked_storage] - this is on purpose
        }
        break;

        case typelib_TypeClass_EXCEPTION:
        {
            // FEATURE_EMPTYCLASS
            auto pTmp = allocTypeDescription<typelib_CompoundTypeDescription>();
            pRet = &pTmp->aBase;
#if OSL_DEBUG_LEVEL > 0
            osl_atomic_increment( &Init().nCompoundTypeDescriptionCount );
#endif
            pTmp->pBaseTypeDescription = nullptr;
            pTmp->nMembers = 0;
            pTmp->pMemberOffsets = nullptr;
            pTmp->ppTypeRefs = nullptr;
            pTmp->ppMemberNames = nullptr;
            // coverity[leaked_storage] - this is on purpose
        }
        break;

        case typelib_TypeClass_ENUM:
        {
            auto pTmp = allocTypeDescription<typelib_EnumTypeDescription>();
            pRet = &pTmp->aBase;
#if OSL_DEBUG_LEVEL > 0
            osl_atomic_increment( &Init().nEnumTypeDescriptionCount );
#endif
            pTmp->nDefaultEnumValue = 0;
            pTmp->nEnumValues       = 0;
            pTmp->ppEnumNames       = nullptr;
            pTmp->pEnumValues       = nullptr;
            // coverity[leaked_storage] - this is on purpose
        }
        break;

        case typelib_TypeClass_INTERFACE:
        {
            auto pTmp = allocTypeDescription<
                typelib_InterfaceTypeDescription>();
            pRet = &pTmp->aBase;
#if OSL_DEBUG_LEVEL > 0
            osl_atomic_increment( &Init().nInterfaceTypeDescriptionCount );
#endif
            pTmp->pBaseTypeDescription = nullptr;
            pTmp->nMembers = 0;
            pTmp->ppMembers = nullptr;
            pTmp->nAllMembers = 0;
            pTmp->ppAllMembers = nullptr;
            pTmp->nMapFunctionIndexToMemberIndex = 0;
            pTmp->pMapFunctionIndexToMemberIndex = nullptr;
            pTmp->pMapMemberIndexToFunctionIndex= nullptr;
            pTmp->nBaseTypes = 0;
            pTmp->ppBaseTypes = nullptr;
            // coverity[leaked_storage] - this is on purpose
        }
        break;

        case typelib_TypeClass_INTERFACE_METHOD:
        {
            auto pTmp = allocTypeDescription<
                typelib_InterfaceMethodTypeDescription>();
            pRet = &pTmp->aBase.aBase;
#if OSL_DEBUG_LEVEL > 0
            osl_atomic_increment( &Init().nInterfaceMethodTypeDescriptionCount );
#endif
            pTmp->aBase.pMemberName = nullptr;
            pTmp->pReturnTypeRef = nullptr;
            pTmp->nParams = 0;
            pTmp->pParams = nullptr;
            pTmp->nExceptions = 0;
            pTmp->ppExceptions = nullptr;
            pTmp->pInterface = nullptr;
            pTmp->pBaseRef = nullptr;
            pTmp->nIndex = 0;
            // coverity[leaked_storage] - this is on purpose
        }
        break;

        case typelib_TypeClass_INTERFACE_ATTRIBUTE:
        {
            auto * pTmp = allocTypeDescription<
                typelib_InterfaceAttributeTypeDescription>();
            pRet = &pTmp->aBase.aBase;
#if OSL_DEBUG_LEVEL > 0
            osl_atomic_increment( &Init().nInterfaceAttributeTypeDescriptionCount );
#endif
            pTmp->aBase.pMemberName = nullptr;
            pTmp->pAttributeTypeRef = nullptr;
            pTmp->pInterface = nullptr;
            pTmp->pBaseRef = nullptr;
            pTmp->nIndex = 0;
            pTmp->nGetExceptions = 0;
            pTmp->ppGetExceptions = nullptr;
            pTmp->nSetExceptions = 0;
            pTmp->ppSetExceptions = nullptr;
            // coverity[leaked_storage] - this is on purpose
        }
        break;

        default:
        {
            pRet = allocTypeDescription<typelib_TypeDescription>();
#if OSL_DEBUG_LEVEL > 0
            osl_atomic_increment( &Init().nTypeDescriptionCount );
#endif
        }
    }

    pRet->nRefCount = 1; // reference count is initially 1
    pRet->nStaticRefCount = 0;
    pRet->eTypeClass = eTypeClass;
    pRet->pUniqueIdentifier = nullptr;
    pRet->pReserved = nullptr;
    pRet->pTypeName = pTypeName;
    rtl_uString_acquire( pRet->pTypeName );
    pRet->pSelf = pRet;
    pRet->bComplete = true;
    pRet->nSize = 0;
    pRet->nAlignment = 0;
    pRet->pWeakRef = nullptr;
    pRet->bOnDemand = false;
    *ppRet = pRet;
}


namespace {

void newTypeDescription(
    typelib_TypeDescription ** ppRet, typelib_TypeClass eTypeClass,
    rtl_uString * pTypeName, typelib_TypeDescriptionReference * pType,
    sal_Int32 nMembers, typelib_CompoundMember_Init * pCompoundMembers,
    typelib_StructMember_Init * pStructMembers)
{
    OSL_ASSERT(
        (pCompoundMembers == nullptr || pStructMembers == nullptr)
        && (pStructMembers == nullptr || eTypeClass == typelib_TypeClass_STRUCT));
    if (typelib_TypeClass_TYPEDEF == eTypeClass)
    {
        SAL_WARN("cppu.typelib", "unexpected typedef!" );
        typelib_typedescriptionreference_getDescription( ppRet, pType );
        return;
    }

    typelib_typedescription_newEmpty( ppRet, eTypeClass, pTypeName );

    switch( eTypeClass )
    {
        case typelib_TypeClass_SEQUENCE:
        {
            OSL_ASSERT( nMembers == 0 );
            typelib_typedescriptionreference_acquire( pType );
            reinterpret_cast<typelib_IndirectTypeDescription *>(*ppRet)->pType = pType;
        }
        break;

        case typelib_TypeClass_EXCEPTION:
        case typelib_TypeClass_STRUCT:
        {
            // FEATURE_EMPTYCLASS
            typelib_CompoundTypeDescription * pTmp = reinterpret_cast<typelib_CompoundTypeDescription*>(*ppRet);

            sal_Int32 nOffset = 0;
            if( pType )
            {
                typelib_typedescriptionreference_getDescription(
                    reinterpret_cast<typelib_TypeDescription **>(&pTmp->pBaseTypeDescription), pType );
                nOffset = pTmp->pBaseTypeDescription->aBase.nSize;
                OSL_ENSURE( newAlignedSize( 0, pTmp->pBaseTypeDescription->aBase.nSize, pTmp->pBaseTypeDescription->aBase.nAlignment ) == pTmp->pBaseTypeDescription->aBase.nSize, "### unexpected offset!" );
            }
            if( nMembers )
            {
                pTmp->nMembers = nMembers;
                pTmp->pMemberOffsets = new sal_Int32[ nMembers ];
                pTmp->ppTypeRefs = new typelib_TypeDescriptionReference *[ nMembers ];
                pTmp->ppMemberNames = new rtl_uString *[ nMembers ];
                bool polymorphic = eTypeClass == typelib_TypeClass_STRUCT
                    && OUString::unacquired(&pTypeName).indexOf('<') >= 0;
                assert(!polymorphic || pStructMembers != nullptr);
                if (polymorphic) {
                    reinterpret_cast< typelib_StructTypeDescription * >(pTmp)->
                        pParameterizedTypes = new sal_Bool[nMembers];
                }
                for( sal_Int32 i = 0 ; i < nMembers; i++ )
                {
                    // read the type and member names
                    pTmp->ppTypeRefs[i] = nullptr;
                    if (pCompoundMembers != nullptr) {
                        typelib_typedescriptionreference_new(
                            pTmp->ppTypeRefs +i, pCompoundMembers[i].eTypeClass,
                            pCompoundMembers[i].pTypeName );
                        pTmp->ppMemberNames[i]
                            = pCompoundMembers[i].pMemberName;
                        rtl_uString_acquire( pTmp->ppMemberNames[i] );
                    } else {
                        typelib_typedescriptionreference_new(
                            pTmp->ppTypeRefs +i,
                            pStructMembers[i].aBase.eTypeClass,
                            pStructMembers[i].aBase.pTypeName );
                        pTmp->ppMemberNames[i]
                            = pStructMembers[i].aBase.pMemberName;
                        rtl_uString_acquire(pTmp->ppMemberNames[i]);
                    }
                    assert(pTmp->ppTypeRefs[i]);
                    // write offset
                    sal_Int32 size;
                    sal_Int32 alignment;
                    if (pTmp->ppTypeRefs[i]->eTypeClass ==
                        typelib_TypeClass_SEQUENCE)
                    {
                        // Take care of recursion like
                        // struct S { sequence<S> x; };
                        size = sizeof(void *);
                        alignment = adjustAlignment(size);
                    } else {
                        typelib_TypeDescription * pTD = nullptr;
                        TYPELIB_DANGER_GET( &pTD, pTmp->ppTypeRefs[i] );
                        OSL_ENSURE( pTD->nSize, "### void member?" );
                        size = pTD->nSize;
                        alignment = pTD->nAlignment;
                        TYPELIB_DANGER_RELEASE( pTD );
                    }
                    nOffset = newAlignedSize( nOffset, size, alignment );
                    pTmp->pMemberOffsets[i] = nOffset - size;

                    if (polymorphic) {
                        reinterpret_cast< typelib_StructTypeDescription * >(
                            pTmp)->pParameterizedTypes[i]
                            = pStructMembers[i].bParameterizedType;
                    }
                }
            }
        }
        break;

        default:
        break;
    }

    if( !TYPELIB_TYPEDESCRIPTIONREFERENCE_ISREALLYWEAK( eTypeClass ) )
        (*ppRet)->pWeakRef = reinterpret_cast<typelib_TypeDescriptionReference *>(*ppRet);
    if( eTypeClass != typelib_TypeClass_VOID )
    {
        // sizeof(void) not allowed
        (*ppRet)->nSize = typelib_typedescription_getAlignedUnoSize( (*ppRet), 0, (*ppRet)->nAlignment );
        (*ppRet)->nAlignment = adjustAlignment( (*ppRet)->nAlignment );
    }
}

}

extern "C" void SAL_CALL typelib_typedescription_new(
    typelib_TypeDescription ** ppRet,
    typelib_TypeClass eTypeClass,
    rtl_uString * pTypeName,
    typelib_TypeDescriptionReference * pType,
    sal_Int32 nMembers,
    typelib_CompoundMember_Init * pMembers ) noexcept
{
    newTypeDescription(
        ppRet, eTypeClass, pTypeName, pType, nMembers, pMembers, nullptr);
}

extern "C" void SAL_CALL typelib_typedescription_newStruct(
    typelib_TypeDescription ** ppRet,
    rtl_uString * pTypeName,
    typelib_TypeDescriptionReference * pType,
    sal_Int32 nMembers,
    typelib_StructMember_Init * pMembers ) noexcept
{
    newTypeDescription(
        ppRet, typelib_TypeClass_STRUCT, pTypeName, pType, nMembers, nullptr,
        pMembers);
}


extern "C" void SAL_CALL typelib_typedescription_newEnum(
    typelib_TypeDescription ** ppRet,
    rtl_uString * pTypeName,
    sal_Int32 nDefaultValue,
    sal_Int32 nEnumValues,
    rtl_uString ** ppEnumNames,
    sal_Int32 * pEnumValues ) noexcept
{
    typelib_typedescription_newEmpty( ppRet, typelib_TypeClass_ENUM, pTypeName );
    typelib_EnumTypeDescription * pEnum = reinterpret_cast<typelib_EnumTypeDescription *>(*ppRet);

    pEnum->nDefaultEnumValue = nDefaultValue;
    pEnum->nEnumValues       = nEnumValues;
    pEnum->ppEnumNames       = new rtl_uString * [ nEnumValues ];
    for ( sal_Int32 nPos = nEnumValues; nPos--; )
    {
        pEnum->ppEnumNames[nPos] = ppEnumNames[nPos];
        rtl_uString_acquire( pEnum->ppEnumNames[nPos] );
    }
    pEnum->pEnumValues      = new sal_Int32[ nEnumValues ];
    ::memcpy( pEnum->pEnumValues, pEnumValues, nEnumValues * sizeof(sal_Int32) );

    static_assert(!TYPELIB_TYPEDESCRIPTIONREFERENCE_ISREALLYWEAK(typelib_TypeClass_ENUM));
    (*ppRet)->pWeakRef = reinterpret_cast<typelib_TypeDescriptionReference *>(*ppRet);
    // sizeof(void) not allowed
    (*ppRet)->nSize = typelib_typedescription_getAlignedUnoSize( (*ppRet), 0, (*ppRet)->nAlignment );
    (*ppRet)->nAlignment = adjustAlignment( (*ppRet)->nAlignment );
}


extern "C" void SAL_CALL typelib_typedescription_newInterface(
    typelib_InterfaceTypeDescription ** ppRet,
    rtl_uString * pTypeName,
    SAL_UNUSED_PARAMETER sal_uInt32, SAL_UNUSED_PARAMETER sal_uInt16,
    SAL_UNUSED_PARAMETER sal_uInt16, SAL_UNUSED_PARAMETER sal_uInt32,
    SAL_UNUSED_PARAMETER sal_uInt32,
    typelib_TypeDescriptionReference * pBaseInterface,
    sal_Int32 nMembers,
    typelib_TypeDescriptionReference ** ppMembers ) noexcept
{
    // coverity[callee_ptr_arith] - not a bug
    typelib_typedescription_newMIInterface(
        ppRet, pTypeName, 0, 0, 0, 0, 0, pBaseInterface == nullptr ? 0 : 1,
        &pBaseInterface, nMembers, ppMembers);
}

namespace {

class BaseList {
public:
    struct Entry {
        sal_Int32 memberOffset;
        sal_Int32 directBaseIndex;
        sal_Int32 directBaseMemberOffset;
        typelib_InterfaceTypeDescription const * base;
    };

    typedef std::vector< Entry > List;

    explicit BaseList(typelib_InterfaceTypeDescription const * desc);

    List const & getList() const { return list; }

    sal_Int32 getBaseMembers() const { return members; }

private:
    typedef std::set< OUString > Set;

    void calculate(
        Set& allSet,
        sal_Int32 directBaseIndex, Set & directBaseSet,
        sal_Int32 * directBaseMembers,
        typelib_InterfaceTypeDescription const * desc);

    List list;
    sal_Int32 members;
};

BaseList::BaseList(typelib_InterfaceTypeDescription const * desc)
   : members(0)
{
    Set allSet;
    for (sal_Int32 i = 0; i < desc->nBaseTypes; ++i) {
        Set directBaseSet;
        sal_Int32 directBaseMembers = 0;
        calculate(allSet, i, directBaseSet, &directBaseMembers, desc->ppBaseTypes[i]);
    }
}

void BaseList::calculate(
    Set& allSet,
    sal_Int32 directBaseIndex, Set & directBaseSet,
    sal_Int32 * directBaseMembers,
    typelib_InterfaceTypeDescription const * desc)
{
    for (sal_Int32 i = 0; i < desc->nBaseTypes; ++i) {
        calculate(allSet,
            directBaseIndex, directBaseSet, directBaseMembers,
            desc->ppBaseTypes[i]);
    }
    if (allSet.insert(desc->aBase.pTypeName).second) {
        Entry e;
        e.memberOffset = members;
        e.directBaseIndex = directBaseIndex;
        e.directBaseMemberOffset = *directBaseMembers;
        e.base = desc;
        list.push_back(e);
        OSL_ASSERT(desc->ppAllMembers != nullptr);
        members += desc->nMembers;
    }
    if (directBaseSet.insert(desc->aBase.pTypeName).second) {
        OSL_ASSERT(desc->ppAllMembers != nullptr);
        *directBaseMembers += desc->nMembers;
    }
}

}

extern "C" void SAL_CALL typelib_typedescription_newMIInterface(
    typelib_InterfaceTypeDescription ** ppRet,
    rtl_uString * pTypeName,
    SAL_UNUSED_PARAMETER sal_uInt32, SAL_UNUSED_PARAMETER sal_uInt16,
    SAL_UNUSED_PARAMETER sal_uInt16, SAL_UNUSED_PARAMETER sal_uInt32,
    SAL_UNUSED_PARAMETER sal_uInt32,
    sal_Int32 nBaseInterfaces,
    typelib_TypeDescriptionReference ** ppBaseInterfaces,
    sal_Int32 nMembers,
    typelib_TypeDescriptionReference ** ppMembers ) noexcept
{
    if (*ppRet != nullptr) {
        typelib_typedescription_release(&(*ppRet)->aBase);
        *ppRet = nullptr;
    }

    typelib_InterfaceTypeDescription * pITD = nullptr;
    typelib_typedescription_newEmpty(
        reinterpret_cast<typelib_TypeDescription **>(&pITD), typelib_TypeClass_INTERFACE, pTypeName );

    pITD->nBaseTypes = nBaseInterfaces;
    pITD->ppBaseTypes = new typelib_InterfaceTypeDescription *[nBaseInterfaces];
    for (sal_Int32 i = 0; i < nBaseInterfaces; ++i) {
        pITD->ppBaseTypes[i] = nullptr;
        typelib_typedescriptionreference_getDescription(
            reinterpret_cast< typelib_TypeDescription ** >(
                &pITD->ppBaseTypes[i]),
            ppBaseInterfaces[i]);
        if (pITD->ppBaseTypes[i] == nullptr
            || !complete(
                reinterpret_cast< typelib_TypeDescription ** >(
                    &pITD->ppBaseTypes[i]),
                false))
        {
            OSL_ASSERT(false);
            return;
        }
        OSL_ASSERT(pITD->ppBaseTypes[i] != nullptr);
    }
    if (nBaseInterfaces > 0) {
        pITD->pBaseTypeDescription = pITD->ppBaseTypes[0];
    }
    // set the
    pITD->aUik.m_Data1 = 0;
    pITD->aUik.m_Data2 = 0;
    pITD->aUik.m_Data3 = 0;
    pITD->aUik.m_Data4 = 0;
    pITD->aUik.m_Data5 = 0;

    BaseList aBaseList(pITD);
    pITD->nAllMembers = nMembers + aBaseList.getBaseMembers();
    pITD->nMembers = nMembers;

    if( pITD->nAllMembers )
    {
        // at minimum one member exist, allocate the memory
        pITD->ppAllMembers = new typelib_TypeDescriptionReference *[ pITD->nAllMembers ];
        sal_Int32 n = 0;

        BaseList::List const & rList = aBaseList.getList();
        for (const auto& rEntry : rList)
        {
            typelib_InterfaceTypeDescription const * pBase = rEntry.base;
            typelib_InterfaceTypeDescription const * pDirectBase
                = pITD->ppBaseTypes[rEntry.directBaseIndex];
            OSL_ASSERT(pBase->ppAllMembers != nullptr);
            for (sal_Int32 j = 0; j < pBase->nMembers; ++j) {
                typelib_TypeDescriptionReference const * pDirectBaseMember
                    = pDirectBase->ppAllMembers[rEntry.directBaseMemberOffset + j];
                OUString aName = OUString::unacquired(&pDirectBaseMember->pTypeName) +
                        ":@" +
                        OUString::number(rEntry.directBaseIndex) +
                        "," +
                        OUString::number(rEntry.memberOffset + j) +
                        ":" +
                        OUString::unacquired(&pITD->aBase.pTypeName);
                typelib_TypeDescriptionReference * pDerivedMember = nullptr;
                typelib_typedescriptionreference_new(
                    &pDerivedMember, pDirectBaseMember->eTypeClass,
                    aName.pData);
                pITD->ppAllMembers[n++] = pDerivedMember;
            }
        }

        if( nMembers )
        {
            pITD->ppMembers = pITD->ppAllMembers + aBaseList.getBaseMembers();
        }

        // add own members
        for( sal_Int32 i = 0; i < nMembers; i++ )
        {
            typelib_typedescriptionreference_acquire( ppMembers[i] );
            pITD->ppAllMembers[n++] = ppMembers[i];
        }
    }

    typelib_TypeDescription * pTmp = &pITD->aBase;
    static_assert( !TYPELIB_TYPEDESCRIPTIONREFERENCE_ISREALLYWEAK( typelib_TypeClass_INTERFACE ) );
    pTmp->pWeakRef = reinterpret_cast<typelib_TypeDescriptionReference *>(pTmp);
    pTmp->nSize = typelib_typedescription_getAlignedUnoSize( pTmp, 0, pTmp->nAlignment );
    pTmp->nAlignment = adjustAlignment( pTmp->nAlignment );
    pTmp->bComplete = false;

    *ppRet = pITD;
}


namespace {

typelib_TypeDescriptionReference ** copyExceptions(
    sal_Int32 count, rtl_uString ** typeNames)
{
    OSL_ASSERT(count >= 0);
    if (count == 0) {
        return nullptr;
    }
    typelib_TypeDescriptionReference ** p
        = new typelib_TypeDescriptionReference *[count];
    for (sal_Int32 i = 0; i < count; ++i) {
        p[i] = nullptr;
        typelib_typedescriptionreference_new(
            p + i, typelib_TypeClass_EXCEPTION, typeNames[i]);
    }
    return p;
}

}

extern "C" void SAL_CALL typelib_typedescription_newInterfaceMethod(
    typelib_InterfaceMethodTypeDescription ** ppRet,
    sal_Int32 nAbsolutePosition,
    sal_Bool bOneWay,
    rtl_uString * pTypeName,
    typelib_TypeClass eReturnTypeClass,
    rtl_uString * pReturnTypeName,
    sal_Int32 nParams,
    typelib_Parameter_Init * pParams,
    sal_Int32 nExceptions,
    rtl_uString ** ppExceptionNames ) noexcept
{
    if (*ppRet != nullptr) {
        typelib_typedescription_release(&(*ppRet)->aBase.aBase);
        *ppRet = nullptr;
    }
    sal_Int32 nOffset = rtl_ustr_lastIndexOfChar_WithLength(
        pTypeName->buffer, pTypeName->length, ':');
    if (nOffset <= 0 || pTypeName->buffer[nOffset - 1] != ':') {
        OSL_FAIL("Bad interface method type name");
        return;
    }
    OUString aInterfaceTypeName(pTypeName->buffer, nOffset - 1);
    typelib_InterfaceTypeDescription * pInterface = nullptr;
    typelib_typedescription_getByName(
        reinterpret_cast< typelib_TypeDescription ** >(&pInterface),
        aInterfaceTypeName.pData);
    if (pInterface == nullptr
        || pInterface->aBase.eTypeClass != typelib_TypeClass_INTERFACE
        || !complete(
            reinterpret_cast< typelib_TypeDescription ** >(&pInterface), false))
    {
        OSL_FAIL("No interface corresponding to interface method");
        return;
    }

    typelib_typedescription_newEmpty(
        reinterpret_cast<typelib_TypeDescription **>(ppRet), typelib_TypeClass_INTERFACE_METHOD, pTypeName );

    rtl_uString_newFromStr_WithLength( &(*ppRet)->aBase.pMemberName,
                                       pTypeName->buffer + nOffset +1,
                                       pTypeName->length - nOffset -1 );
    (*ppRet)->aBase.nPosition = nAbsolutePosition;
    (*ppRet)->bOneWay = bOneWay;
    typelib_typedescriptionreference_new( &(*ppRet)->pReturnTypeRef, eReturnTypeClass, pReturnTypeName );
    (*ppRet)->nParams = nParams;
    if( nParams )
    {
        (*ppRet)->pParams = new typelib_MethodParameter[ nParams ];

        for( sal_Int32 i = 0; i < nParams; i++ )
        {
            // get the name of the parameter
            (*ppRet)->pParams[ i ].pName = pParams[i].pParamName;
            rtl_uString_acquire( (*ppRet)->pParams[ i ].pName );
            (*ppRet)->pParams[ i ].pTypeRef = nullptr;
            // get the type name of the parameter and create the weak reference
            typelib_typedescriptionreference_new(
                &(*ppRet)->pParams[ i ].pTypeRef, pParams[i].eTypeClass, pParams[i].pTypeName );
            (*ppRet)->pParams[ i ].bIn = pParams[i].bIn;
            (*ppRet)->pParams[ i ].bOut = pParams[i].bOut;
        }
    }
    (*ppRet)->nExceptions = nExceptions;
    (*ppRet)->ppExceptions = copyExceptions(nExceptions, ppExceptionNames);
    (*ppRet)->pInterface = pInterface;
    (*ppRet)->pBaseRef = nullptr;
    OSL_ASSERT(
        (nAbsolutePosition >= pInterface->nAllMembers - pInterface->nMembers)
        && nAbsolutePosition < pInterface->nAllMembers);
    (*ppRet)->nIndex = nAbsolutePosition
        - (pInterface->nAllMembers - pInterface->nMembers);
    static_assert( TYPELIB_TYPEDESCRIPTIONREFERENCE_ISREALLYWEAK( typelib_TypeClass_INTERFACE_METHOD ) );
    assert(reinterpret_cast<typelib_TypeDescription *>(*ppRet)->pWeakRef == nullptr);
}


extern "C" void SAL_CALL typelib_typedescription_newInterfaceAttribute(
    typelib_InterfaceAttributeTypeDescription ** ppRet,
    sal_Int32 nAbsolutePosition,
    rtl_uString * pTypeName,
    typelib_TypeClass eAttributeTypeClass,
    rtl_uString * pAttributeTypeName,
    sal_Bool bReadOnly ) noexcept
{
    typelib_typedescription_newExtendedInterfaceAttribute(
        ppRet, nAbsolutePosition, pTypeName, eAttributeTypeClass,
        pAttributeTypeName, bReadOnly, 0, nullptr, 0, nullptr);
}


extern "C" void SAL_CALL typelib_typedescription_newExtendedInterfaceAttribute(
    typelib_InterfaceAttributeTypeDescription ** ppRet,
    sal_Int32 nAbsolutePosition,
    rtl_uString * pTypeName,
    typelib_TypeClass eAttributeTypeClass,
    rtl_uString * pAttributeTypeName,
    sal_Bool bReadOnly,
    sal_Int32 nGetExceptions, rtl_uString ** ppGetExceptionNames,
    sal_Int32 nSetExceptions, rtl_uString ** ppSetExceptionNames ) noexcept
{
    if (*ppRet != nullptr) {
        typelib_typedescription_release(&(*ppRet)->aBase.aBase);
        *ppRet = nullptr;
    }
    sal_Int32 nOffset = rtl_ustr_lastIndexOfChar_WithLength(
        pTypeName->buffer, pTypeName->length, ':');
    if (nOffset <= 0 || pTypeName->buffer[nOffset - 1] != ':') {
        OSL_FAIL("Bad interface attribute type name");
        return;
    }
    OUString aInterfaceTypeName(pTypeName->buffer, nOffset - 1);
    typelib_InterfaceTypeDescription * pInterface = nullptr;
    typelib_typedescription_getByName(
        reinterpret_cast< typelib_TypeDescription ** >(&pInterface),
        aInterfaceTypeName.pData);
    if (pInterface == nullptr
        || pInterface->aBase.eTypeClass != typelib_TypeClass_INTERFACE
        || !complete(
            reinterpret_cast< typelib_TypeDescription ** >(&pInterface), false))
    {
        OSL_FAIL("No interface corresponding to interface attribute");
        return;
    }

    typelib_typedescription_newEmpty(
        reinterpret_cast<typelib_TypeDescription **>(ppRet), typelib_TypeClass_INTERFACE_ATTRIBUTE, pTypeName );

    rtl_uString_newFromStr_WithLength( &(*ppRet)->aBase.pMemberName,
                                       pTypeName->buffer + nOffset +1,
                                       pTypeName->length - nOffset -1 );
    (*ppRet)->aBase.nPosition = nAbsolutePosition;
    typelib_typedescriptionreference_new( &(*ppRet)->pAttributeTypeRef, eAttributeTypeClass, pAttributeTypeName );
    (*ppRet)->bReadOnly = bReadOnly;
    (*ppRet)->pInterface = pInterface;
    (*ppRet)->pBaseRef = nullptr;
    OSL_ASSERT(
        (nAbsolutePosition >= pInterface->nAllMembers - pInterface->nMembers)
        && nAbsolutePosition < pInterface->nAllMembers);
    (*ppRet)->nIndex = nAbsolutePosition
        - (pInterface->nAllMembers - pInterface->nMembers);
    (*ppRet)->nGetExceptions = nGetExceptions;
    (*ppRet)->ppGetExceptions = copyExceptions(
        nGetExceptions, ppGetExceptionNames);
    (*ppRet)->nSetExceptions = nSetExceptions;
    (*ppRet)->ppSetExceptions = copyExceptions(
        nSetExceptions, ppSetExceptionNames);
    static_assert( TYPELIB_TYPEDESCRIPTIONREFERENCE_ISREALLYWEAK( typelib_TypeClass_INTERFACE_ATTRIBUTE ) );
    assert(reinterpret_cast<typelib_TypeDescription *>(*ppRet)->pWeakRef == nullptr);
}


extern "C" void SAL_CALL typelib_typedescription_acquire(
    typelib_TypeDescription * pTypeDescription ) noexcept
{
    osl_atomic_increment( &pTypeDescription->nRefCount );
}


namespace {

void deleteExceptions(
    sal_Int32 count, typelib_TypeDescriptionReference ** exceptions)
{
    for (sal_Int32 i = 0; i < count; ++i) {
        typelib_typedescriptionreference_release(exceptions[i]);
    }
    delete[] exceptions;
}

}

// frees anything except typelib_TypeDescription base!
static void typelib_typedescription_destructExtendedMembers(
    typelib_TypeDescription * pTD )
{
    OSL_ASSERT( typelib_TypeClass_TYPEDEF != pTD->eTypeClass );

    switch( pTD->eTypeClass )
    {
    case typelib_TypeClass_SEQUENCE:
        if( reinterpret_cast<typelib_IndirectTypeDescription*>(pTD)->pType )
            typelib_typedescriptionreference_release( reinterpret_cast<typelib_IndirectTypeDescription*>(pTD)->pType );
        break;
    case typelib_TypeClass_STRUCT:
        delete[] reinterpret_cast< typelib_StructTypeDescription * >(pTD)->
            pParameterizedTypes;
        [[fallthrough]];
    case typelib_TypeClass_EXCEPTION:
    {
        typelib_CompoundTypeDescription * pCTD = reinterpret_cast<typelib_CompoundTypeDescription*>(pTD);
        if( pCTD->pBaseTypeDescription )
            typelib_typedescription_release( &pCTD->pBaseTypeDescription->aBase );
        sal_Int32 i;
        for( i = 0; i < pCTD->nMembers; i++ )
        {
            typelib_typedescriptionreference_release( pCTD->ppTypeRefs[i] );
        }
        if (pCTD->ppMemberNames)
        {
            for ( i = 0; i < pCTD->nMembers; i++ )
            {
                rtl_uString_release( pCTD->ppMemberNames[i] );
            }
            delete [] pCTD->ppMemberNames;
        }
        delete [] pCTD->ppTypeRefs;
        delete [] pCTD->pMemberOffsets;
    }
    break;
    case typelib_TypeClass_INTERFACE:
    {
        typelib_InterfaceTypeDescription * pITD = reinterpret_cast<typelib_InterfaceTypeDescription*>(pTD);
        for( sal_Int32 i = 0; i < pITD->nAllMembers; i++ )
        {
            typelib_typedescriptionreference_release( pITD->ppAllMembers[i] );
        }
        delete [] pITD->ppAllMembers;
        delete [] pITD->pMapMemberIndexToFunctionIndex;
        delete [] pITD->pMapFunctionIndexToMemberIndex;
        for (sal_Int32 i = 0; i < pITD->nBaseTypes; ++i) {
            typelib_typedescription_release(
                reinterpret_cast< typelib_TypeDescription * >(
                    pITD->ppBaseTypes[i]));
        }
        delete[] pITD->ppBaseTypes;
        break;
    }
    case typelib_TypeClass_INTERFACE_METHOD:
    {
        typelib_InterfaceMethodTypeDescription * pIMTD = reinterpret_cast<typelib_InterfaceMethodTypeDescription*>(pTD);
        if( pIMTD->pReturnTypeRef )
            typelib_typedescriptionreference_release( pIMTD->pReturnTypeRef );
        for( sal_Int32 i = 0; i < pIMTD->nParams; i++ )
        {
            rtl_uString_release( pIMTD->pParams[ i ].pName );
            typelib_typedescriptionreference_release( pIMTD->pParams[ i ].pTypeRef );
        }
        delete [] pIMTD->pParams;
        deleteExceptions(pIMTD->nExceptions, pIMTD->ppExceptions);
        rtl_uString_release( pIMTD->aBase.pMemberName );
        typelib_typedescription_release(&pIMTD->pInterface->aBase);
        if (pIMTD->pBaseRef != nullptr) {
            typelib_typedescriptionreference_release(pIMTD->pBaseRef);
        }
    }
    break;
    case typelib_TypeClass_INTERFACE_ATTRIBUTE:
    {
        typelib_InterfaceAttributeTypeDescription * pIATD = reinterpret_cast<typelib_InterfaceAttributeTypeDescription*>(pTD);
        deleteExceptions(pIATD->nGetExceptions, pIATD->ppGetExceptions);
        deleteExceptions(pIATD->nSetExceptions, pIATD->ppSetExceptions);
        if( pIATD->pAttributeTypeRef )
            typelib_typedescriptionreference_release( pIATD->pAttributeTypeRef );
        if( pIATD->aBase.pMemberName )
            rtl_uString_release( pIATD->aBase.pMemberName );
        typelib_typedescription_release(&pIATD->pInterface->aBase);
        if (pIATD->pBaseRef != nullptr) {
            typelib_typedescriptionreference_release(pIATD->pBaseRef);
        }
    }
    break;
    case typelib_TypeClass_ENUM:
    {
        typelib_EnumTypeDescription * pEnum = reinterpret_cast<typelib_EnumTypeDescription *>(pTD);
        for ( sal_Int32 nPos = pEnum->nEnumValues; nPos--; )
        {
            rtl_uString_release( pEnum->ppEnumNames[nPos] );
        }
        delete [] pEnum->ppEnumNames;
        delete [] pEnum->pEnumValues;
    }
    break;
    default:
    break;
    }
}


extern "C" void SAL_CALL typelib_typedescription_release(
    typelib_TypeDescription * pTD ) noexcept
{
    sal_Int32 ref = osl_atomic_decrement( &pTD->nRefCount );
    OSL_ASSERT(ref >= 0);
    if (0 != ref)
        return;

    TypeDescriptor_Init_Impl &rInit = Init();
    if( TYPELIB_TYPEDESCRIPTIONREFERENCE_ISREALLYWEAK( pTD->eTypeClass ) )
    {
        if( pTD->pWeakRef )
        {
            {
                MutexGuard aGuard( rInit.maMutex );
                // remove this description from the weak reference
                pTD->pWeakRef->pType = nullptr;
            }
            typelib_typedescriptionreference_release( pTD->pWeakRef );
        }
    }
    else
    {
        // this description is a reference too, so remove it from the hash table
        MutexGuard aGuard( rInit.maMutex );
        WeakMap_Impl::iterator aIt = rInit.maWeakMap.find( pTD->pTypeName->buffer );
        if( aIt != rInit.maWeakMap.end() && static_cast<void *>((*aIt).second) == static_cast<void *>(pTD) )
        {
            // remove only if it contains the same object
            rInit.maWeakMap.erase( aIt );
        }
    }

    typelib_typedescription_destructExtendedMembers( pTD );
    rtl_uString_release( pTD->pTypeName );

#if OSL_DEBUG_LEVEL > 0
    switch( pTD->eTypeClass )
    {
    case typelib_TypeClass_SEQUENCE:
        osl_atomic_decrement( &rInit.nIndirectTypeDescriptionCount );
        break;
    case typelib_TypeClass_STRUCT:
    case typelib_TypeClass_EXCEPTION:
        osl_atomic_decrement( &rInit.nCompoundTypeDescriptionCount );
        break;
    case typelib_TypeClass_INTERFACE:
        osl_atomic_decrement( &rInit.nInterfaceTypeDescriptionCount );
        break;
    case typelib_TypeClass_INTERFACE_METHOD:
        osl_atomic_decrement( &rInit.nInterfaceMethodTypeDescriptionCount );
        break;
    case typelib_TypeClass_INTERFACE_ATTRIBUTE:
        osl_atomic_decrement( &rInit.nInterfaceAttributeTypeDescriptionCount );
        break;
    case typelib_TypeClass_ENUM:
        osl_atomic_decrement( &rInit.nEnumTypeDescriptionCount );
        break;
    default:
        osl_atomic_decrement( &rInit.nTypeDescriptionCount );
    }
#endif

    freeTypeDescription(pTD);
}


extern "C" void SAL_CALL typelib_typedescription_register(
    typelib_TypeDescription ** ppNewDescription ) noexcept
{
    // connect the description with the weak reference
    TypeDescriptor_Init_Impl &rInit = Init();
    ClearableMutexGuard aGuard( rInit.maMutex );

    typelib_TypeDescriptionReference * pTDR = nullptr;
    typelib_typedescriptionreference_getByName( &pTDR, (*ppNewDescription)->pTypeName );

    OSL_ASSERT( (*ppNewDescription)->pWeakRef || TYPELIB_TYPEDESCRIPTIONREFERENCE_ISREALLYWEAK( (*ppNewDescription)->eTypeClass ) );
    if( pTDR )
    {
        OSL_ASSERT( (*ppNewDescription)->eTypeClass == pTDR->eTypeClass );
        if( pTDR->pType )
        {
            if (TYPELIB_TYPEDESCRIPTIONREFERENCE_ISREALLYWEAK( pTDR->eTypeClass ))
            {
                // pRef->pType->pWeakRef == 0 means that the description is empty
                if (pTDR->pType->pWeakRef)
                {
                    if (osl_atomic_increment( &pTDR->pType->nRefCount ) > 1)
                    {
                        // The reference is incremented. The object cannot be destroyed.
                        // Release the guard at the earliest point.
                        aGuard.clear();
                        ::typelib_typedescription_release( *ppNewDescription );
                        *ppNewDescription = pTDR->pType;
                        ::typelib_typedescriptionreference_release( pTDR );
                        return;
                    }
                    // destruction of this type in progress (another thread!)
                    (void)osl_atomic_decrement( &pTDR->pType->nRefCount );
                }
                // take new descr
                pTDR->pType = *ppNewDescription;
                OSL_ASSERT( ! (*ppNewDescription)->pWeakRef );
                (*ppNewDescription)->pWeakRef = pTDR;
                return;
            }
            // !reallyWeak

            if ((static_cast<void *>(pTDR) != static_cast<void *>(*ppNewDescription)) && // if different
                (!pTDR->pType->pWeakRef || // uninit: ref data only set
                 // new one is complete:
                 (!pTDR->pType->bComplete && (*ppNewDescription)->bComplete) ||
                 // new one may be partly initialized interface (except of tables):
                 (typelib_TypeClass_INTERFACE == pTDR->pType->eTypeClass &&
                  !reinterpret_cast<typelib_InterfaceTypeDescription *>(pTDR->pType)->ppAllMembers &&
                  (*reinterpret_cast<typelib_InterfaceTypeDescription **>(ppNewDescription))->ppAllMembers)))
            {
                // uninitialized or incomplete

                if (pTDR->pType->pWeakRef) // if init
                {
                    switch (pTDR->pType->eTypeClass) {
                    case typelib_TypeClass_ENUM:
                        {
                            auto const src = reinterpret_cast<typelib_EnumTypeDescription *>(
                                *ppNewDescription);
                            auto const dst = reinterpret_cast<typelib_EnumTypeDescription *>(
                                pTDR->pType);
                            assert(dst->nEnumValues == 0);
                            assert(dst->ppEnumNames == nullptr);
                            assert(dst->pEnumValues == nullptr);
                            std::swap(src->nEnumValues, dst->nEnumValues);
                            std::swap(src->ppEnumNames, dst->ppEnumNames);
                            std::swap(src->pEnumValues, dst->pEnumValues);
                            break;
                        }
                    case typelib_TypeClass_STRUCT:
                    case typelib_TypeClass_EXCEPTION:
                        {
                            auto const src = reinterpret_cast<typelib_CompoundTypeDescription *>(
                                *ppNewDescription);
                            auto const dst = reinterpret_cast<typelib_CompoundTypeDescription *>(
                                pTDR->pType);
                            assert(
                                (dst->pBaseTypeDescription == nullptr)
                                == (src->pBaseTypeDescription == nullptr));
                            assert(dst->nMembers == src->nMembers);
                            assert((dst->pMemberOffsets == nullptr) == (dst->nMembers == 0));
                            assert((dst->ppTypeRefs == nullptr) == (dst->nMembers == 0));
                            assert(dst->ppMemberNames == nullptr);
                            assert(
                                pTDR->pType->eTypeClass != typelib_TypeClass_STRUCT
                                || ((reinterpret_cast<typelib_StructTypeDescription *>(
                                         dst)->pParameterizedTypes
                                     == nullptr)
                                    == (reinterpret_cast<typelib_StructTypeDescription *>(
                                            src)->pParameterizedTypes
                                        == nullptr)));
                            std::swap(src->ppMemberNames, dst->ppMemberNames);
                            break;
                        }
                    case typelib_TypeClass_INTERFACE:
                        {
                            auto const src = reinterpret_cast<typelib_InterfaceTypeDescription *>(
                                *ppNewDescription);
                            auto const dst = reinterpret_cast<typelib_InterfaceTypeDescription *>(
                                pTDR->pType);
                            assert(
                                (dst->pBaseTypeDescription == nullptr)
                                == (src->pBaseTypeDescription == nullptr));
                            assert(dst->nMembers == 0);
                            assert(dst->ppMembers == nullptr);
                            assert(dst->nAllMembers == 0);
                            assert(dst->ppAllMembers == nullptr);
                            assert(dst->pMapMemberIndexToFunctionIndex == nullptr);
                            assert(dst->nMapFunctionIndexToMemberIndex == 0);
                            assert(dst->pMapFunctionIndexToMemberIndex == nullptr);
                            assert(dst->nBaseTypes == src->nBaseTypes);
                            assert((dst->ppBaseTypes == nullptr) == (src->ppBaseTypes == nullptr));
                            std::swap(src->nMembers, dst->nMembers);
                            std::swap(src->ppMembers, dst->ppMembers);
                            std::swap(src->nAllMembers, dst->nAllMembers);
                            std::swap(src->ppAllMembers, dst->ppAllMembers);
                            std::swap(
                                src->pMapMemberIndexToFunctionIndex,
                                dst->pMapMemberIndexToFunctionIndex);
                            std::swap(
                                src->nMapFunctionIndexToMemberIndex,
                                dst->nMapFunctionIndexToMemberIndex);
                            std::swap(
                                src->pMapFunctionIndexToMemberIndex,
                                dst->pMapFunctionIndexToMemberIndex);
                            break;
                        }
                    default:
                        assert(false); // this cannot happen
                    }
                }
                else
                {
                    // pTDR->pType->pWeakRef == 0 means that the description is empty
                    // description is not weak and the not the same
                    sal_Int32 nSize = getDescriptionSize( (*ppNewDescription)->eTypeClass );

                    // copy all specific data for the descriptions
                    memcpy(
                        pTDR->pType +1,
                        *ppNewDescription +1,
                        nSize - sizeof(typelib_TypeDescription) );

                    memset(
                        *ppNewDescription +1,
                        0,
                        nSize - sizeof( typelib_TypeDescription ) );
                }

                pTDR->pType->bComplete = (*ppNewDescription)->bComplete;
                pTDR->pType->nSize = (*ppNewDescription)->nSize;
                pTDR->pType->nAlignment = (*ppNewDescription)->nAlignment;

                if( pTDR->pType->bOnDemand && !(*ppNewDescription)->bOnDemand )
                {
                    // switch from OnDemand to !OnDemand, so the description must be acquired
                    typelib_typedescription_acquire( pTDR->pType );
                }
                else if( !pTDR->pType->bOnDemand && (*ppNewDescription)->bOnDemand )
                {
                    // switch from !OnDemand to OnDemand, so the description must be released
                    assert(pTDR->pType->nRefCount > 1);
                    // coverity[freed_arg] - pType's nRefCount is > 1 here
                    typelib_typedescription_release( pTDR->pType );
                }

                pTDR->pType->bOnDemand = (*ppNewDescription)->bOnDemand;
                // initialized
                pTDR->pType->pWeakRef = pTDR;
            }

            typelib_typedescription_release( *ppNewDescription );
            // pTDR was acquired by getByName(), so it must not be acquired again
            *ppNewDescription = pTDR->pType;
            return;
        }
    }
    else if( TYPELIB_TYPEDESCRIPTIONREFERENCE_ISREALLYWEAK( (*ppNewDescription)->eTypeClass) )
    {
        typelib_typedescriptionreference_new(
            &pTDR, (*ppNewDescription)->eTypeClass, (*ppNewDescription)->pTypeName );
    }
    else
    {
        pTDR = reinterpret_cast<typelib_TypeDescriptionReference *>(*ppNewDescription);

        // description is the weak itself, so register it
        rInit.maWeakMap[pTDR->pTypeName->buffer] = pTDR;
        OSL_ASSERT( static_cast<void *>(*ppNewDescription) == static_cast<void *>(pTDR) );
    }

    // By default this reference is not really weak. The reference hold the description
    // and the description hold the reference.
    if( !(*ppNewDescription)->bOnDemand )
    {
        // nor OnDemand so the description must be acquired if registered
        typelib_typedescription_acquire( *ppNewDescription );
    }

    pTDR->pType = *ppNewDescription;
    (*ppNewDescription)->pWeakRef = pTDR;
    OSL_ASSERT( rtl_ustr_compare( pTDR->pTypeName->buffer, (*ppNewDescription)->pTypeName->buffer ) == 0 );
    OSL_ASSERT( pTDR->eTypeClass == (*ppNewDescription)->eTypeClass );
}


static bool type_equals(
    typelib_TypeDescriptionReference const * p1, typelib_TypeDescriptionReference const * p2 )
{
    return (p1 == p2 ||
            (p1->eTypeClass == p2->eTypeClass &&
             p1->pTypeName->length == p2->pTypeName->length &&
             rtl_ustr_compare( p1->pTypeName->buffer, p2->pTypeName->buffer ) == 0));
}
extern "C" sal_Bool SAL_CALL typelib_typedescription_equals(
    const typelib_TypeDescription * p1, const typelib_TypeDescription * p2 ) noexcept
{
    return type_equals(
        reinterpret_cast<typelib_TypeDescriptionReference const *>(p1), reinterpret_cast<typelib_TypeDescriptionReference const *>(p2) );
}


extern "C" sal_Int32 typelib_typedescription_getAlignedUnoSize(
    const typelib_TypeDescription * pTypeDescription,
    sal_Int32 nOffset, sal_Int32 & rMaxIntegralTypeSize ) noexcept
{
    sal_Int32 nSize;
    if( pTypeDescription->nSize )
    {
        // size and alignment are set
        rMaxIntegralTypeSize = pTypeDescription->nAlignment;
        nSize = pTypeDescription->nSize;
    }
    else
    {
        nSize = 0;
        rMaxIntegralTypeSize = 1;

        OSL_ASSERT( typelib_TypeClass_TYPEDEF != pTypeDescription->eTypeClass );

        switch( pTypeDescription->eTypeClass )
        {
            case typelib_TypeClass_INTERFACE:
                // FEATURE_INTERFACE
                nSize = rMaxIntegralTypeSize = sal_Int32(sizeof( void * ));
                break;
            case typelib_TypeClass_ENUM:
                nSize = rMaxIntegralTypeSize = sal_Int32(sizeof( typelib_TypeClass ));
                break;
            case typelib_TypeClass_STRUCT:
            case typelib_TypeClass_EXCEPTION:
                // FEATURE_EMPTYCLASS
                {
                typelib_CompoundTypeDescription const * pTmp = reinterpret_cast<typelib_CompoundTypeDescription const *>(pTypeDescription);
                sal_Int32 nStructSize = 0;
                if( pTmp->pBaseTypeDescription )
                {
                    // inherit structs extends the base struct.
                    nStructSize = pTmp->pBaseTypeDescription->aBase.nSize;
                    rMaxIntegralTypeSize = pTmp->pBaseTypeDescription->aBase.nAlignment;
                }
                for( sal_Int32 i = 0; i < pTmp->nMembers; i++ )
                {
                    typelib_TypeDescription * pMemberType = nullptr;
                    typelib_TypeDescriptionReference * pMemberRef = pTmp->ppTypeRefs[i];

                    sal_Int32 nMaxIntegral;
                    if (pMemberRef->eTypeClass == typelib_TypeClass_INTERFACE
                        || pMemberRef->eTypeClass == typelib_TypeClass_SEQUENCE)
                    {
                        nMaxIntegral = sal_Int32(sizeof(void *));
                        nStructSize = newAlignedSize( nStructSize, nMaxIntegral, nMaxIntegral );
                    }
                    else
                    {
                        TYPELIB_DANGER_GET( &pMemberType, pMemberRef );
                        nStructSize = typelib_typedescription_getAlignedUnoSize(
                            pMemberType, nStructSize, nMaxIntegral );
                        TYPELIB_DANGER_RELEASE( pMemberType );
                    }
                    if( nMaxIntegral > rMaxIntegralTypeSize )
                        rMaxIntegralTypeSize = nMaxIntegral;
                }
#ifdef __m68k__
                // Anything that is at least 16 bits wide is aligned on a 16-bit
                // boundary on the m68k default abi
                sal_Int32 nMaxAlign = std::min(rMaxIntegralTypeSize, sal_Int32( 2 ));
                nStructSize = (nStructSize + nMaxAlign -1) / nMaxAlign * nMaxAlign;
#else
                // Example: A { double; int; } structure has a size of 16 instead of 10. The
                // compiler must follow this rule if it is possible to access members in arrays through:
                // (Element *)((char *)pArray + sizeof( Element ) * ElementPos)
                nStructSize = (nStructSize + rMaxIntegralTypeSize -1)
                                / rMaxIntegralTypeSize * rMaxIntegralTypeSize;
#endif
                nSize += nStructSize;
                }
                break;
            case typelib_TypeClass_SEQUENCE:
                nSize = rMaxIntegralTypeSize = sal_Int32(sizeof( void * ));
                break;
            case typelib_TypeClass_ANY:
                // FEATURE_ANY
                nSize = sal_Int32(sizeof( uno_Any ));
                rMaxIntegralTypeSize = sal_Int32(sizeof( void * ));
                break;
            case typelib_TypeClass_TYPE:
                nSize = rMaxIntegralTypeSize = sal_Int32(sizeof( typelib_TypeDescriptionReference * ));
                break;
            case typelib_TypeClass_BOOLEAN:
                nSize = rMaxIntegralTypeSize = sal_Int32(sizeof( sal_Bool ));
                break;
            case typelib_TypeClass_CHAR:
                nSize = rMaxIntegralTypeSize = sal_Int32(sizeof( sal_Unicode ));
                break;
            case typelib_TypeClass_STRING:
                // FEATURE_STRING
                nSize = rMaxIntegralTypeSize = sal_Int32(sizeof( rtl_uString * ));
                break;
            case typelib_TypeClass_FLOAT:
                nSize = rMaxIntegralTypeSize = sal_Int32(sizeof( float ));
                break;
            case typelib_TypeClass_DOUBLE:
                nSize = rMaxIntegralTypeSize = sal_Int32(sizeof( double ));
                break;
            case typelib_TypeClass_BYTE:
                nSize = rMaxIntegralTypeSize = sal_Int32(sizeof( sal_Int8 ));
                break;
            case typelib_TypeClass_SHORT:
            case typelib_TypeClass_UNSIGNED_SHORT:
                nSize = rMaxIntegralTypeSize = sal_Int32(sizeof( sal_Int16 ));
                break;
            case typelib_TypeClass_LONG:
            case typelib_TypeClass_UNSIGNED_LONG:
                nSize = rMaxIntegralTypeSize = sal_Int32(sizeof( sal_Int32 ));
                break;
            case typelib_TypeClass_HYPER:
            case typelib_TypeClass_UNSIGNED_HYPER:
                nSize = rMaxIntegralTypeSize = sal_Int32(sizeof( sal_Int64 ));
                break;
            case typelib_TypeClass_UNKNOWN:
            case typelib_TypeClass_SERVICE:
            case typelib_TypeClass_MODULE:
            default:
                OSL_FAIL( "not convertible type" );
        };
    }

    return newAlignedSize( nOffset, nSize, rMaxIntegralTypeSize );
}


namespace {

typelib_TypeDescriptionReference ** copyExceptions(
    sal_Int32 count, typelib_TypeDescriptionReference ** source)
{
    typelib_TypeDescriptionReference ** p
        = new typelib_TypeDescriptionReference *[count];
    for (sal_Int32 i = 0; i < count; ++i) {
        p[i] = source[i];
        typelib_typedescriptionreference_acquire(p[i]);
    }
    return p;
}

bool createDerivedInterfaceMemberDescription(
    typelib_TypeDescription ** result, OUString const & name,
    typelib_TypeDescriptionReference * baseRef,
    typelib_TypeDescription const * base, typelib_TypeDescription * interface,
    sal_Int32 index, sal_Int32 position)
{
    if (!baseRef || !base || !interface)
        return false;

    switch (base->eTypeClass) {
    case typelib_TypeClass_INTERFACE_METHOD:
        {
            typelib_typedescription_newEmpty(
                result, typelib_TypeClass_INTERFACE_METHOD, name.pData);
            typelib_InterfaceMethodTypeDescription const * baseMethod
                = reinterpret_cast<
                typelib_InterfaceMethodTypeDescription const * >(base);
            typelib_InterfaceMethodTypeDescription * newMethod
                = reinterpret_cast<
                typelib_InterfaceMethodTypeDescription * >(*result);
            newMethod->aBase.nPosition = position;
            newMethod->aBase.pMemberName
                = baseMethod->aBase.pMemberName;
            rtl_uString_acquire(
                newMethod->aBase.pMemberName);
            newMethod->pReturnTypeRef = baseMethod->pReturnTypeRef;
            typelib_typedescriptionreference_acquire(
                newMethod->pReturnTypeRef);
            newMethod->nParams = baseMethod->nParams;
            newMethod->pParams = new typelib_MethodParameter[
                newMethod->nParams];
            for (sal_Int32 i = 0; i < newMethod->nParams; ++i) {
                newMethod->pParams[i].pName
                    = baseMethod->pParams[i].pName;
                rtl_uString_acquire(
                    newMethod->pParams[i].pName);
                newMethod->pParams[i].pTypeRef
                    = baseMethod->pParams[i].pTypeRef;
                typelib_typedescriptionreference_acquire(
                    newMethod->pParams[i].pTypeRef);
                newMethod->pParams[i].bIn = baseMethod->pParams[i].bIn;
                newMethod->pParams[i].bOut = baseMethod->pParams[i].bOut;
            }
            newMethod->nExceptions = baseMethod->nExceptions;
            newMethod->ppExceptions = copyExceptions(
                baseMethod->nExceptions, baseMethod->ppExceptions);
            newMethod->bOneWay = baseMethod->bOneWay;
            newMethod->pInterface
                = reinterpret_cast< typelib_InterfaceTypeDescription * >(
                    interface);
            newMethod->pBaseRef = baseRef;
            newMethod->nIndex = index;
            return true;
        }

    case typelib_TypeClass_INTERFACE_ATTRIBUTE:
        {
            typelib_typedescription_newEmpty(
                result, typelib_TypeClass_INTERFACE_ATTRIBUTE, name.pData);
            typelib_InterfaceAttributeTypeDescription const * baseAttribute
                = reinterpret_cast<
                typelib_InterfaceAttributeTypeDescription const * >(base);
            typelib_InterfaceAttributeTypeDescription * newAttribute
                = reinterpret_cast<
                typelib_InterfaceAttributeTypeDescription * >(*result);
            newAttribute->aBase.nPosition = position;
            newAttribute->aBase.pMemberName
                = baseAttribute->aBase.pMemberName;
            rtl_uString_acquire(newAttribute->aBase.pMemberName);
            newAttribute->bReadOnly = baseAttribute->bReadOnly;
            newAttribute->pAttributeTypeRef
                = baseAttribute->pAttributeTypeRef;
            typelib_typedescriptionreference_acquire(newAttribute->pAttributeTypeRef);
            newAttribute->pInterface
                = reinterpret_cast< typelib_InterfaceTypeDescription * >(
                    interface);
            newAttribute->pBaseRef = baseRef;
            newAttribute->nIndex = index;
            newAttribute->nGetExceptions = baseAttribute->nGetExceptions;
            newAttribute->ppGetExceptions = copyExceptions(
                baseAttribute->nGetExceptions,
                baseAttribute->ppGetExceptions);
            newAttribute->nSetExceptions = baseAttribute->nSetExceptions;
            newAttribute->ppSetExceptions = copyExceptions(
                baseAttribute->nSetExceptions,
                baseAttribute->ppSetExceptions);
            return true;
        }

    default:
        break;
    }
    return false;
}

}

extern "C" void SAL_CALL typelib_typedescription_getByName(
    typelib_TypeDescription ** ppRet, rtl_uString * pName ) noexcept
{
    if( *ppRet )
    {
        typelib_typedescription_release( *ppRet );
        *ppRet = nullptr;
    }

    static bool bInited = false;
    TypeDescriptor_Init_Impl &rInit = Init();

    if( !bInited )
    {
        // guard against multi thread access
        MutexGuard aGuard( rInit.maMutex );
        if( !bInited )
        {
            // avoid recursion during the next ...new calls
            bInited = true;

            typelib_TypeDescription * pType = nullptr;
            typelib_typedescription_new( &pType, typelib_TypeClass_TYPE, u"type"_ustr.pData, nullptr, 0, nullptr );
            typelib_typedescription_register( &pType );
            typelib_typedescription_new( &pType, typelib_TypeClass_VOID, u"void"_ustr.pData, nullptr, 0, nullptr );
            typelib_typedescription_register( &pType );
            typelib_typedescription_new( &pType, typelib_TypeClass_BOOLEAN, u"boolean"_ustr.pData, nullptr, 0, nullptr );
            typelib_typedescription_register( &pType );
            typelib_typedescription_new( &pType, typelib_TypeClass_CHAR, u"char"_ustr.pData, nullptr, 0, nullptr );
            typelib_typedescription_register( &pType );
            typelib_typedescription_new( &pType, typelib_TypeClass_BYTE, u"byte"_ustr.pData, nullptr, 0, nullptr );
            typelib_typedescription_register( &pType );
            typelib_typedescription_new( &pType, typelib_TypeClass_STRING, u"string"_ustr.pData, nullptr, 0, nullptr );
            typelib_typedescription_register( &pType );
            typelib_typedescription_new( &pType, typelib_TypeClass_SHORT, u"short"_ustr.pData, nullptr, 0, nullptr );
            typelib_typedescription_register( &pType );
            typelib_typedescription_new( &pType, typelib_TypeClass_UNSIGNED_SHORT, u"unsigned short"_ustr.pData, nullptr, 0, nullptr );
            typelib_typedescription_register( &pType );
            typelib_typedescription_new( &pType, typelib_TypeClass_LONG, u"long"_ustr.pData, nullptr, 0, nullptr );
            typelib_typedescription_register( &pType );
            typelib_typedescription_new( &pType, typelib_TypeClass_UNSIGNED_LONG, u"unsigned long"_ustr.pData, nullptr, 0, nullptr );
            typelib_typedescription_register( &pType );
            typelib_typedescription_new( &pType, typelib_TypeClass_HYPER, u"hyper"_ustr.pData, nullptr, 0, nullptr );
            typelib_typedescription_register( &pType );
            typelib_typedescription_new( &pType, typelib_TypeClass_UNSIGNED_HYPER, u"unsigned hyper"_ustr.pData, nullptr, 0, nullptr );
            typelib_typedescription_register( &pType );
            typelib_typedescription_new( &pType, typelib_TypeClass_FLOAT, u"float"_ustr.pData, nullptr, 0, nullptr );
            typelib_typedescription_register( &pType );
            typelib_typedescription_new( &pType, typelib_TypeClass_DOUBLE, u"double"_ustr.pData, nullptr, 0, nullptr );
            typelib_typedescription_register( &pType );
            typelib_typedescription_new( &pType, typelib_TypeClass_ANY, u"any"_ustr.pData, nullptr, 0, nullptr );
            typelib_typedescription_register( &pType );
            typelib_typedescription_release( pType );
        }
    }

    typelib_TypeDescriptionReference * pTDR = nullptr;
    typelib_typedescriptionreference_getByName( &pTDR, pName );
    if( pTDR )
    {
        {
        // guard against multi thread access
        MutexGuard aGuard( rInit.maMutex );
        // pTDR->pType->pWeakRef == 0 means that the description is empty
        if( pTDR->pType && pTDR->pType->pWeakRef )
        {
            typelib_typedescription_acquire( pTDR->pType );
            *ppRet = pTDR->pType;
        }
        }
        typelib_typedescriptionreference_release( pTDR );
    }

    if (nullptr != *ppRet)
        return;

    // check for sequence
    OUString const & name = OUString::unacquired( &pName );
    if (2 < name.getLength() && '[' == name[ 0 ])
    {
        OUString element_name( name.copy( 2 ) );
        typelib_TypeDescription * element_td = nullptr;
        typelib_typedescription_getByName( &element_td, element_name.pData );
        if (nullptr != element_td)
        {
            typelib_typedescription_new(
                ppRet, typelib_TypeClass_SEQUENCE, pName, element_td->pWeakRef, 0, nullptr );
            // register?
            typelib_typedescription_release( element_td );
        }
    }
    if (nullptr == *ppRet)
    {
        // Check for derived interface member type:
        sal_Int32 i1 = name.lastIndexOf(":@");
        if (i1 >= 0) {
            sal_Int32 i2 = i1 + RTL_CONSTASCII_LENGTH(":@");
            sal_Int32 i3 = name.indexOf(',', i2);
            if (i3 >= 0) {
                sal_Int32 i4 = name.indexOf(':', i3);
                if (i4 >= 0) {
                    typelib_TypeDescriptionReference * pBaseRef = nullptr;
                    typelib_TypeDescription * pBase = nullptr;
                    typelib_TypeDescription * pInterface = nullptr;
                    typelib_typedescriptionreference_getByName(
                        &pBaseRef, name.copy(0, i1).pData);
                    if (pBaseRef != nullptr) {
                        typelib_typedescriptionreference_getDescription(
                            &pBase, pBaseRef);
                    }
                    typelib_typedescription_getByName(
                        &pInterface, name.copy(i4 + 1).pData);
                    if (!createDerivedInterfaceMemberDescription(
                            ppRet, name, pBaseRef, pBase, pInterface,
                            o3tl::toInt32(name.subView(i2, i3 - i2)),
                            o3tl::toInt32(name.subView(i3 + 1, i4 - i3 - 1))))
                    {
                        if (pInterface != nullptr) {
                            typelib_typedescription_release(pInterface);
                        }
                        if (pBase != nullptr) {
                            typelib_typedescription_release(pBase);
                        }
                        if (pBaseRef != nullptr) {
                            typelib_typedescriptionreference_release(
                                pBaseRef);
                        }
                    }
                }
            }
        }
    }
    if (nullptr == *ppRet)
    {
        // on demand access
        rInit.callChain( ppRet, pName );
    }

    if( !(*ppRet) )
        return;

    // typedescription found
    if (typelib_TypeClass_TYPEDEF == (*ppRet)->eTypeClass)
    {
        typelib_TypeDescription * pTD = nullptr;
        typelib_typedescriptionreference_getDescription(
            &pTD, reinterpret_cast<typelib_IndirectTypeDescription *>(*ppRet)->pType );
        typelib_typedescription_release( *ppRet );
        *ppRet = pTD;
    }
    else
    {
        // set to on demand
        (*ppRet)->bOnDemand = true;
        // The type description is hold by the reference until
        // on demand is activated.
        typelib_typedescription_register( ppRet );

        // insert into the cache
        MutexGuard aGuard( rInit.maMutex );
        if( rInit.maCache.size() >= nCacheSize )
        {
            typelib_typedescription_release( rInit.maCache.front() );
            rInit.maCache.pop_front();
        }
        // descriptions in the cache must be acquired!
        typelib_typedescription_acquire( *ppRet );
        rInit.maCache.push_back( *ppRet );
    }
}

extern "C" void SAL_CALL typelib_typedescriptionreference_newByAsciiName(
    typelib_TypeDescriptionReference ** ppTDR,
    typelib_TypeClass eTypeClass,
    const char * pTypeName ) noexcept
{
    OUString aTypeName( OUString::createFromAscii( pTypeName ) );
    typelib_typedescriptionreference_new( ppTDR, eTypeClass, aTypeName.pData );
}

extern "C" void SAL_CALL typelib_typedescriptionreference_new(
    typelib_TypeDescriptionReference ** ppTDR,
    typelib_TypeClass eTypeClass, rtl_uString * pTypeName ) noexcept
{
    TypeDescriptor_Init_Impl &rInit = Init();
    if( eTypeClass == typelib_TypeClass_TYPEDEF )
    {
        // on demand access
        typelib_TypeDescription * pRet = nullptr;
        rInit.callChain( &pRet, pTypeName );
        if( pRet )
        {
            // typedescription found
            if (typelib_TypeClass_TYPEDEF == pRet->eTypeClass)
            {
                typelib_typedescriptionreference_acquire(
                    reinterpret_cast<typelib_IndirectTypeDescription *>(pRet)->pType );
                if (*ppTDR)
                    typelib_typedescriptionreference_release( *ppTDR );
                *ppTDR = reinterpret_cast<typelib_IndirectTypeDescription *>(pRet)->pType;
                typelib_typedescription_release( pRet );
            }
            else
            {
                // set to on demand
                pRet->bOnDemand = true;
                // The type description is hold by the reference until
                // on demand is activated.
                typelib_typedescription_register( &pRet );

                // insert into the cache
                MutexGuard aGuard( rInit.maMutex );
                if( rInit.maCache.size() >= nCacheSize )
                {
                    typelib_typedescription_release( rInit.maCache.front() );
                    rInit.maCache.pop_front();
                }
                rInit.maCache.push_back( pRet );
                // pRet kept acquired for cache

                typelib_typedescriptionreference_acquire( pRet->pWeakRef );
                if (*ppTDR)
                    typelib_typedescriptionreference_release( *ppTDR );
                *ppTDR = pRet->pWeakRef;
            }
        }
        else if (*ppTDR)
        {
            SAL_INFO("cppu.typelib", "typedef not found : " << pTypeName);
            typelib_typedescriptionreference_release( *ppTDR );
            *ppTDR = nullptr;
        }
        return;
    }

    MutexGuard aGuard( rInit.maMutex );
    typelib_typedescriptionreference_getByName( ppTDR, pTypeName );
    if( *ppTDR )
        return;

    if( TYPELIB_TYPEDESCRIPTIONREFERENCE_ISREALLYWEAK( eTypeClass ) )
    {
        typelib_TypeDescriptionReference * pTDR = new typelib_TypeDescriptionReference;
#if OSL_DEBUG_LEVEL > 0
        osl_atomic_increment( &rInit.nTypeDescriptionReferenceCount );
#endif
        pTDR->nRefCount = 1;
        pTDR->nStaticRefCount = 0;
        pTDR->eTypeClass = eTypeClass;
        pTDR->pUniqueIdentifier = nullptr;
        pTDR->pReserved = nullptr;
        pTDR->pTypeName = pTypeName;
        rtl_uString_acquire( pTDR->pTypeName );
        pTDR->pType = nullptr;
        *ppTDR = pTDR;
    }
    else
    {
        typelib_typedescription_newEmpty( reinterpret_cast<typelib_TypeDescription ** >(ppTDR), eTypeClass, pTypeName );
        // description will be registered but not acquired
        (*reinterpret_cast<typelib_TypeDescription **>(ppTDR))->bOnDemand = true;
        (*reinterpret_cast<typelib_TypeDescription **>(ppTDR))->bComplete = false;
    }

    // Heavy hack, the const sal_Unicode * is hold by the typedescription reference
    // not registered
    rInit.maWeakMap[ (*ppTDR)->pTypeName->buffer ] = *ppTDR;
}


extern "C" void SAL_CALL typelib_typedescriptionreference_acquire(
    typelib_TypeDescriptionReference * pRef ) noexcept
{
    osl_atomic_increment( &pRef->nRefCount );
}


extern "C" void SAL_CALL typelib_typedescriptionreference_release(
    typelib_TypeDescriptionReference * pRef ) noexcept
{
    // Is it a type description?
    if( TYPELIB_TYPEDESCRIPTIONREFERENCE_ISREALLYWEAK( pRef->eTypeClass ) )
    {
        if( ! osl_atomic_decrement( &pRef->nRefCount ) )
        {
            TypeDescriptor_Init_Impl &rInit = Init();
            MutexGuard aGuard( rInit.maMutex );
            WeakMap_Impl::iterator aIt = rInit.maWeakMap.find( pRef->pTypeName->buffer );
            if( aIt != rInit.maWeakMap.end() && (*aIt).second == pRef )
            {
                // remove only if it contains the same object
                rInit.maWeakMap.erase( aIt );
            }

            rtl_uString_release( pRef->pTypeName );
            OSL_ASSERT( pRef->pType == nullptr );
#if OSL_DEBUG_LEVEL > 0
            osl_atomic_decrement( &rInit.nTypeDescriptionReferenceCount );
#endif
            delete pRef;
        }
    }
    else
    {
        typelib_typedescription_release( reinterpret_cast<typelib_TypeDescription *>(pRef) );
    }
}


extern "C" void SAL_CALL typelib_typedescriptionreference_getDescription(
    typelib_TypeDescription ** ppRet, typelib_TypeDescriptionReference * pRef ) noexcept
{
    if( *ppRet )
    {
        typelib_typedescription_release( *ppRet );
        *ppRet = nullptr;
    }

    if( !TYPELIB_TYPEDESCRIPTIONREFERENCE_ISREALLYWEAK( pRef->eTypeClass ) && pRef->pType && pRef->pType->pWeakRef )
    {
        // reference is a description and initialized
        osl_atomic_increment( &reinterpret_cast<typelib_TypeDescription *>(pRef)->nRefCount );
        *ppRet = reinterpret_cast<typelib_TypeDescription *>(pRef);
        return;
    }

    {
    MutexGuard aGuard( Init().maMutex );
    // pRef->pType->pWeakRef == 0 means that the description is empty
    if( pRef->pType && pRef->pType->pWeakRef )
    {
        sal_Int32 n = osl_atomic_increment( &pRef->pType->nRefCount );
        if( n > 1 )
        {
            // The reference is incremented. The object cannot be destroyed.
            // Release the guard at the earliest point.
            *ppRet = pRef->pType;
            return;
        }
        (void)osl_atomic_decrement( &pRef->pType->nRefCount );
        // destruction of this type in progress (another thread!)
        // no access through this weak reference
        pRef->pType = nullptr;
    }
    }

    typelib_typedescription_getByName( ppRet, pRef->pTypeName );
    OSL_ASSERT( !*ppRet || rtl_ustr_compare( pRef->pTypeName->buffer, (*ppRet)->pTypeName->buffer ) == 0 );
    OSL_ASSERT( !*ppRet || pRef->eTypeClass == (*ppRet)->eTypeClass );
    OSL_ASSERT( !*ppRet || pRef == (*ppRet)->pWeakRef );
    pRef->pType = *ppRet;
}


extern "C" void typelib_typedescriptionreference_getByName(
    typelib_TypeDescriptionReference ** ppRet, rtl_uString const * pName ) noexcept
{
    if( *ppRet )
    {
        typelib_typedescriptionreference_release( *ppRet );
        *ppRet = nullptr;
    }
    TypeDescriptor_Init_Impl &rInit = Init();

    MutexGuard aGuard( rInit.maMutex );
    WeakMap_Impl::const_iterator aIt = rInit.maWeakMap.find( pName->buffer );
    if( aIt == rInit.maWeakMap.end() )
        return;

    sal_Int32 n = osl_atomic_increment( &(*aIt).second->nRefCount );
    if( n > 1 )
    {
        // The reference is incremented. The object cannot be destroyed.
        // Release the guard at the earliest point.
        *ppRet = (*aIt).second;
    }
    else
    {
        // destruction of this type in progress (another thread!)
        // no access through this weak reference
        (void)osl_atomic_decrement( &(*aIt).second->nRefCount );
    }
}


extern "C" sal_Bool SAL_CALL typelib_typedescriptionreference_equals(
    const typelib_TypeDescriptionReference * p1,
    const typelib_TypeDescriptionReference * p2 ) noexcept
{
    return (p1 == p2 ||
            (p1->eTypeClass == p2->eTypeClass &&
             p1->pTypeName->length == p2->pTypeName->length &&
             rtl_ustr_compare( p1->pTypeName->buffer, p2->pTypeName->buffer ) == 0));
}


extern "C" void SAL_CALL typelib_typedescriptionreference_assign(
    typelib_TypeDescriptionReference ** ppDest,
    typelib_TypeDescriptionReference * pSource ) noexcept
{
    if (*ppDest != pSource)
    {
        ::typelib_typedescriptionreference_acquire( pSource );
        ::typelib_typedescriptionreference_release( *ppDest );
        *ppDest = pSource;
    }
}


extern "C" void SAL_CALL typelib_setCacheSize( sal_Int32 ) noexcept
{
}


const bool s_aAssignableFromTab[11][11] =
{
                          /* from CH,    BO,    BY,    SH,    US,    LO,    UL,    HY,    UH,    FL,    DO */
/* TypeClass_CHAR */            { true,  false, false, false, false, false, false, false, false, false, false },
/* TypeClass_BOOLEAN */         { false, true,  false, false, false, false, false, false, false, false, false },
/* TypeClass_BYTE */            { false, false, true,  false, false, false, false, false, false, false, false },
/* TypeClass_SHORT */           { false, false, true,  true,  true,  false, false, false, false, false, false },
/* TypeClass_UNSIGNED_SHORT */  { false, false, true,  true,  true,  false, false, false, false, false, false },
/* TypeClass_LONG */            { false, false, true,  true,  true,  true,  true,  false, false, false, false },
/* TypeClass_UNSIGNED_LONG */   { false, false, true,  true,  true,  true,  true,  false, false, false, false },
/* TypeClass_HYPER */           { false, false, true,  true,  true,  true,  true,  true,  true,  false, false },
/* TypeClass_UNSIGNED_HYPER */  { false, false, true,  true,  true,  true,  true,  true,  true,  false, false },
/* TypeClass_FLOAT */           { false, false, true,  true,  true,  false, false, false, false, true,  false },
/* TypeClass_DOUBLE */          { false, false, true,  true,  true,  true,  true,  false, false, true,  true  }
};


extern "C" sal_Bool SAL_CALL typelib_typedescriptionreference_isAssignableFrom(
    typelib_TypeDescriptionReference * pAssignable,
    typelib_TypeDescriptionReference * pFrom ) noexcept
{
    if (!pAssignable || !pFrom)
        return false;

    typelib_TypeClass eAssignable = pAssignable->eTypeClass;
    typelib_TypeClass eFrom       = pFrom->eTypeClass;

    if (eAssignable == typelib_TypeClass_ANY) // anything can be assigned to an any .)
        return true;
    if (eAssignable == eFrom)
    {
        if (type_equals( pAssignable, pFrom )) // first shot
        {
            return true;
        }
        switch (eAssignable)
        {
        case typelib_TypeClass_STRUCT:
        case typelib_TypeClass_EXCEPTION:
        {
            typelib_TypeDescription * pFromDescr = nullptr;
            TYPELIB_DANGER_GET( &pFromDescr, pFrom );
            if (!reinterpret_cast<typelib_CompoundTypeDescription *>(pFromDescr)->pBaseTypeDescription)
            {
                TYPELIB_DANGER_RELEASE( pFromDescr );
                return false;
            }
            bool bRet = typelib_typedescriptionreference_isAssignableFrom(
                pAssignable,
                reinterpret_cast<typelib_CompoundTypeDescription *>(pFromDescr)->pBaseTypeDescription->aBase.pWeakRef );
            TYPELIB_DANGER_RELEASE( pFromDescr );
            return bRet;
        }
        case typelib_TypeClass_INTERFACE:
        {
            typelib_TypeDescription * pFromDescr = nullptr;
            TYPELIB_DANGER_GET( &pFromDescr, pFrom );
            typelib_InterfaceTypeDescription * pFromIfc
                = reinterpret_cast<
                    typelib_InterfaceTypeDescription * >(pFromDescr);
            bool bRet = false;
            for (sal_Int32 i = 0; i < pFromIfc->nBaseTypes; ++i) {
                if (typelib_typedescriptionreference_isAssignableFrom(
                        pAssignable,
                        pFromIfc->ppBaseTypes[i]->aBase.pWeakRef))
                {
                    bRet = true;
                    break;
                }
            }
            TYPELIB_DANGER_RELEASE( pFromDescr );
            return bRet;
        }
        default:
        {
            return false;
        }
        }
    }
    return (eAssignable >= typelib_TypeClass_CHAR && eAssignable <= typelib_TypeClass_DOUBLE &&
            eFrom >= typelib_TypeClass_CHAR && eFrom <= typelib_TypeClass_DOUBLE &&
            s_aAssignableFromTab[eAssignable-1][eFrom-1]);
}

extern "C" sal_Bool SAL_CALL typelib_typedescription_isAssignableFrom(
    typelib_TypeDescription * pAssignable,
    typelib_TypeDescription * pFrom ) noexcept
{
    return typelib_typedescriptionreference_isAssignableFrom(
        pAssignable->pWeakRef, pFrom->pWeakRef );
}


extern "C" sal_Bool SAL_CALL typelib_typedescription_complete(
    typelib_TypeDescription ** ppTypeDescr ) noexcept
{
    return complete(ppTypeDescr, true);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
