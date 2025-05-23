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


#include <comphelper/MasterPropertySet.hxx>
#include <comphelper/MasterPropertySetInfo.hxx>
#include <comphelper/ChainablePropertySet.hxx>
#include <comphelper/ChainablePropertySetInfo.hxx>
#include <comphelper/solarmutex.hxx>


#include <memory>
#include <vector>
#include <optional>

namespace {

class AutoOGuardArray
{
    std::vector<std::optional< osl::Guard< comphelper::SolarMutex > >>  maGuardArray;

public:
    explicit AutoOGuardArray( sal_Int32 nNumElements );

    std::optional< osl::Guard< comphelper::SolarMutex > > &  operator[] ( sal_Int32 i ) { return maGuardArray[i]; }
};

}

AutoOGuardArray::AutoOGuardArray( sal_Int32 nNumElements ) : maGuardArray(nNumElements)
{
}


using namespace ::comphelper;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::beans;


SlaveData::SlaveData ( ChainablePropertySet *pSlave)
: mxSlave ( pSlave )
, mbInit ( false )
{
}

MasterPropertySet::MasterPropertySet( comphelper::MasterPropertySetInfo* pInfo, comphelper::SolarMutex* pMutex )
    noexcept
: mpMutex ( pMutex )
, mnLastId ( 0 )
, mxInfo ( pInfo )
{
}

MasterPropertySet::~MasterPropertySet()
    noexcept
{
    for( const auto& rSlave : maSlaveMap )
        delete rSlave.second;
}

// XPropertySet
Reference< XPropertySetInfo > SAL_CALL MasterPropertySet::getPropertySetInfo(  )
{
    return mxInfo;
}

void MasterPropertySet::registerSlave ( ChainablePropertySet *pNewSet )
    noexcept
{
    maSlaveMap [ ++mnLastId ] = new SlaveData ( pNewSet );
    mxInfo->add ( pNewSet->mxInfo->maMap, mnLastId );
}

void SAL_CALL MasterPropertySet::setPropertyValue( const OUString& rPropertyName, const Any& rValue )
{
    // acquire mutex in c-tor and releases it in the d-tor (exception safe!).
    std::optional< osl::Guard< comphelper::SolarMutex > > xMutexGuard;
    if (mpMutex)
        xMutexGuard.emplace( mpMutex );

    PropertyDataHash::const_iterator aIter = mxInfo->maMap.find ( rPropertyName );

    if( aIter == mxInfo->maMap.end())
        throw UnknownPropertyException( rPropertyName, static_cast< XPropertySet* >( this ) );

    if ( (*aIter).second->mnMapId == 0 ) // 0 means it's one of ours !
    {
        _preSetValues();
        _setSingleValue( *((*aIter).second->mpInfo), rValue );
        _postSetValues();
    }
    else
    {
        ChainablePropertySet * pSlave = maSlaveMap [ (*aIter).second->mnMapId ]->mxSlave.get();

        // acquire mutex in c-tor and releases it in the d-tor (exception safe!).
        std::optional< osl::Guard< comphelper::SolarMutex > > xMutexGuard2;
        if (pSlave->mpMutex)
            xMutexGuard2.emplace( pSlave->mpMutex );

        pSlave->_preSetValues();
        pSlave->_setSingleValue( *((*aIter).second->mpInfo), rValue );
        pSlave->_postSetValues();
    }
}

Any SAL_CALL MasterPropertySet::getPropertyValue( const OUString& rPropertyName )
{
    // acquire mutex in c-tor and releases it in the d-tor (exception safe!).
    std::optional< osl::Guard< comphelper::SolarMutex > > xMutexGuard;
    if (mpMutex)
        xMutexGuard.emplace( mpMutex );

    PropertyDataHash::const_iterator aIter = mxInfo->maMap.find ( rPropertyName );

    if( aIter == mxInfo->maMap.end())
        throw UnknownPropertyException( rPropertyName, static_cast< XPropertySet* >( this ) );

    Any aAny;
    if ( (*aIter).second->mnMapId == 0 ) // 0 means it's one of ours !
    {
        _preGetValues();
        _getSingleValue( *((*aIter).second->mpInfo), aAny );
        _postGetValues();
    }
    else
    {
        ChainablePropertySet * pSlave = maSlaveMap [ (*aIter).second->mnMapId ]->mxSlave.get();

        // acquire mutex in c-tor and releases it in the d-tor (exception safe!).
        std::optional< osl::Guard< comphelper::SolarMutex > > xMutexGuard2;
        if (pSlave->mpMutex)
            xMutexGuard2.emplace( pSlave->mpMutex );

        pSlave->_preGetValues();
        pSlave->_getSingleValue( *((*aIter).second->mpInfo), aAny );
        pSlave->_postGetValues();
    }
    return aAny;
}

void SAL_CALL MasterPropertySet::addPropertyChangeListener( const OUString&, const Reference< XPropertyChangeListener >& )
{
    // todo
}

void SAL_CALL MasterPropertySet::removePropertyChangeListener( const OUString&, const Reference< XPropertyChangeListener >& )
{
    // todo
}

void SAL_CALL MasterPropertySet::addVetoableChangeListener( const OUString&, const Reference< XVetoableChangeListener >& )
{
    // todo
}

void SAL_CALL MasterPropertySet::removeVetoableChangeListener( const OUString&, const Reference< XVetoableChangeListener >& )
{
    // todo
}

// XMultiPropertySet
void SAL_CALL MasterPropertySet::setPropertyValues( const Sequence< OUString >& aPropertyNames, const Sequence< Any >& aValues )
{
    // acquire mutex in c-tor and releases it in the d-tor (exception safe!).
    std::optional< osl::Guard< comphelper::SolarMutex > > xMutexGuard;
    if (mpMutex)
        xMutexGuard.emplace( mpMutex );

    const sal_Int32 nCount = aPropertyNames.getLength();

    if( nCount != aValues.getLength() )
        throw IllegalArgumentException();

    if( !nCount )
        return;

    _preSetValues();

    PropertyDataHash::const_iterator aEnd = mxInfo->maMap.end(), aIter;

    //!! have a unique_ptr to an array of OGuards in order to have the
    //!! allocated memory properly freed (exception safe!).
    //!! Since the array itself has unique_ptrs as members we have to use a
    //!! helper class 'AutoOGuardArray' in order to have
    //!! the acquired locks properly released.
    AutoOGuardArray aOGuardArray( nCount );

    for (sal_Int32 i = 0; i < nCount; ++i)
    {
        aIter = mxInfo->maMap.find(aPropertyNames[i]);
        if ( aIter == aEnd )
            throw RuntimeException(aPropertyNames[i], static_cast<XPropertySet*>(this));

        if ( (*aIter).second->mnMapId == 0 ) // 0 means it's one of ours !
            _setSingleValue(*((*aIter).second->mpInfo), aValues[i]);
        else
        {
            SlaveData * pSlave = maSlaveMap [ (*aIter).second->mnMapId ];
            if (!pSlave->IsInit())
            {
                // acquire mutex in c-tor and releases it in the d-tor (exception safe!).
                if (pSlave->mxSlave->mpMutex)
                    aOGuardArray[i].emplace( pSlave->mxSlave->mpMutex );

                pSlave->mxSlave->_preSetValues();
                pSlave->SetInit ( true );
            }
            pSlave->mxSlave->_setSingleValue(*((*aIter).second->mpInfo), aValues[i]);
        }
    }

    _postSetValues();
    for( const auto& rSlave : maSlaveMap )
    {
        if( rSlave.second->IsInit() )
        {
            rSlave.second->mxSlave->_postSetValues();
            rSlave.second->SetInit( false );
        }
    }
}

Sequence< Any > SAL_CALL MasterPropertySet::getPropertyValues( const Sequence< OUString >& aPropertyNames )
{
    // acquire mutex in c-tor and releases it in the d-tor (exception safe!).
    std::optional< osl::Guard< comphelper::SolarMutex > > xMutexGuard;
    if (mpMutex)
        xMutexGuard.emplace( mpMutex );

    const sal_Int32 nCount = aPropertyNames.getLength();

    Sequence < Any > aValues ( nCount );

    if( nCount )
    {
        _preGetValues();

        Any * pAny = aValues.getArray();
        PropertyDataHash::const_iterator aEnd = mxInfo->maMap.end(), aIter;

        //!! have a unique_ptr to an array of OGuards in order to have the
        //!! allocated memory properly freed (exception safe!).
        //!! Since the array itself has unique_ptrs as members we have to use a
        //!! helper class 'AutoOGuardArray' in order to have
        //!! the acquired locks properly released.
        AutoOGuardArray aOGuardArray( nCount );

        for (sal_Int32 i = 0; i < nCount; ++i)
        {
            aIter = mxInfo->maMap.find(aPropertyNames[i]);
            if ( aIter == aEnd )
                throw RuntimeException(aPropertyNames[i], static_cast<XPropertySet*>(this));

            if ( (*aIter).second->mnMapId == 0 ) // 0 means it's one of ours !
                _getSingleValue(*((*aIter).second->mpInfo), pAny[i]);
            else
            {
                SlaveData * pSlave = maSlaveMap [ (*aIter).second->mnMapId ];
                if (!pSlave->IsInit())
                {
                    // acquire mutex in c-tor and releases it in the d-tor (exception safe!).
                    if (pSlave->mxSlave->mpMutex)
                        aOGuardArray[i].emplace( pSlave->mxSlave->mpMutex );

                    pSlave->mxSlave->_preGetValues();
                    pSlave->SetInit ( true );
                }
                pSlave->mxSlave->_getSingleValue(*((*aIter).second->mpInfo), pAny[i]);
            }
        }

        _postSetValues();
        for( const auto& rSlave : maSlaveMap )
        {
            if( rSlave.second->IsInit() )
            {
                rSlave.second->mxSlave->_postSetValues();
                rSlave.second->SetInit( false );
            }
        }
    }
    return aValues;
}

void SAL_CALL MasterPropertySet::addPropertiesChangeListener( const Sequence< OUString >&, const Reference< XPropertiesChangeListener >& )
{
    // todo
}

void SAL_CALL MasterPropertySet::removePropertiesChangeListener( const Reference< XPropertiesChangeListener >& )
{
    // todo
}

void SAL_CALL MasterPropertySet::firePropertiesChangeEvent( const Sequence< OUString >&, const Reference< XPropertiesChangeListener >& )
{
    // todo
}

// XPropertyState
PropertyState SAL_CALL MasterPropertySet::getPropertyState( const OUString& PropertyName )
{
    PropertyDataHash::const_iterator aIter =  mxInfo->maMap.find( PropertyName );
    if( aIter == mxInfo->maMap.end())
        throw UnknownPropertyException( PropertyName, static_cast< XPropertySet* >( this ) );

    // 0 means it's one of ours !
    if ( (*aIter).second->mnMapId != 0 )
    {
        ChainablePropertySet * pSlave = maSlaveMap [ (*aIter).second->mnMapId ]->mxSlave.get();

        // acquire mutex in c-tor and releases it in the d-tor (exception safe!).
        std::optional< osl::Guard< comphelper::SolarMutex > > xMutexGuard;
        if (pSlave->mpMutex)
            xMutexGuard.emplace( pSlave->mpMutex );
    }

    return PropertyState_AMBIGUOUS_VALUE;
}

Sequence< PropertyState > SAL_CALL MasterPropertySet::getPropertyStates( const Sequence< OUString >& rPropertyNames )
{
    const sal_Int32 nCount = rPropertyNames.getLength();

    Sequence< PropertyState > aStates( nCount );
    if( nCount )
    {
        PropertyState * pState = aStates.getArray();
        PropertyDataHash::const_iterator aEnd = mxInfo->maMap.end(), aIter;

        for (sal_Int32 i = 0; i < nCount; ++i)
        {
            aIter = mxInfo->maMap.find(rPropertyNames[i]);
            if ( aIter == aEnd )
                throw UnknownPropertyException(rPropertyNames[i], static_cast<XPropertySet*>(this));

            // 0 means it's one of ours !
            if ( (*aIter).second->mnMapId != 0 )
            {
                SlaveData * pSlave = maSlaveMap [ (*aIter).second->mnMapId ];
                if (!pSlave->IsInit())
                {
                    pSlave->SetInit ( true );
                }
            }

            pState[i] = PropertyState_AMBIGUOUS_VALUE;
        }
        for( const auto& rSlave : maSlaveMap )
        {
            if( rSlave.second->IsInit() )
            {
                rSlave.second->SetInit( false );
            }
        }
    }
    return aStates;
}

void SAL_CALL MasterPropertySet::setPropertyToDefault( const OUString& rPropertyName )
{
    PropertyDataHash::const_iterator aIter = mxInfo->maMap.find ( rPropertyName );

    if( aIter == mxInfo->maMap.end())
        throw UnknownPropertyException( rPropertyName, static_cast< XPropertySet* >( this ) );
}

Any SAL_CALL MasterPropertySet::getPropertyDefault( const OUString& rPropertyName )
{
    PropertyDataHash::const_iterator aIter = mxInfo->maMap.find ( rPropertyName );

    if( aIter == mxInfo->maMap.end())
        throw UnknownPropertyException( rPropertyName, static_cast< XPropertySet* >( this ) );
    return Any();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
