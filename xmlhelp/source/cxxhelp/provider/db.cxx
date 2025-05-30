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


#include "db.hxx"

#include <algorithm>
#include <charconv>
#include <cstring>
#include <system_error>
#include <utility>

#include <com/sun/star/io/XSeekable.hpp>

using namespace com::sun::star::uno;
using namespace com::sun::star::io;

namespace {

std::pair<sal_Int32, char const *> readInt32(char const * begin, char const * end) {
    sal_Int32 n = 0;
    auto const [ptr, ec] = std::from_chars(begin, end, n, 16);
    return {std::max(n, sal_Int32(0)), ec == std::errc{} && n >= 0 ? ptr : begin};
}

}

namespace helpdatafileproxy {

void HDFData::copyToBuffer( const char* pSrcData, int nSize )
{
    m_nSize = nSize;
    m_pBuffer.reset( new char[m_nSize+1] );
    memcpy( m_pBuffer.get(), pSrcData, m_nSize );
    m_pBuffer[m_nSize] = 0;
}


// Hdf

bool Hdf::implReadLenAndData( const char* pData, char const * end, int& riPos, HDFData& rValue )
{
    bool bSuccess = false;

    // Read key len
    const char* pStartPtr = pData + riPos;
    auto [nKeyLen, pEndPtr] = readInt32(pStartPtr, end);
    if( pEndPtr == pStartPtr )
        return bSuccess;
    riPos += (pEndPtr - pStartPtr) + 1;

    const char* pKeySrc = pData + riPos;
    rValue.copyToBuffer( pKeySrc, nKeyLen );
    riPos += nKeyLen + 1;

    bSuccess = true;
    return bSuccess;
}

void Hdf::createHashMap( bool bOptimizeForPerformance )
{
    releaseHashMap();
    if( bOptimizeForPerformance )
    {
        if( m_pStringToDataMap != nullptr )
            return;
        m_pStringToDataMap.reset(new StringToDataMap);
    }
    else
    {
        if( m_pStringToValPosMap != nullptr )
            return;
        m_pStringToValPosMap.reset(new StringToValPosMap);
    }

    Reference< XInputStream > xIn = m_xSFA->openFileRead( m_aFileURL );
    if( !xIn.is() )
        return;

    Sequence< sal_Int8 > aData;
    sal_Int32 nSize = m_xSFA->getSize( m_aFileURL );
    sal_Int32 nRead = xIn->readBytes( aData, nSize );

    const char* pData = reinterpret_cast<const char*>(aData.getConstArray());
    auto const end = pData + nRead;
    int iPos = 0;
    while( iPos < nRead )
    {
        HDFData aDBKey;
        if( !implReadLenAndData( pData, end, iPos, aDBKey ) )
            break;

        OString aOKeyStr = aDBKey.getData();

        // Read val len
        const char* pStartPtr = pData + iPos;
        auto [nValLen, pEndPtr] = readInt32(pStartPtr, end);
        if( pEndPtr == pStartPtr )
            break;

        iPos += (pEndPtr - pStartPtr) + 1;

        if( bOptimizeForPerformance )
        {
            const char* pValSrc = pData + iPos;
            (*m_pStringToDataMap)[aOKeyStr] = OString(pValSrc, nValLen);
        }
        else
        {
            // store value start position
            (*m_pStringToValPosMap)[aOKeyStr] = std::pair<int,int>( iPos, nValLen );
        }
        iPos += nValLen + 1;
    }

    xIn->closeInput();
}

void Hdf::releaseHashMap()
{
    m_pStringToDataMap.reset();
    m_pStringToValPosMap.reset();
}


Hdf::~Hdf()
{
}

bool Hdf::getValueForKey( const OString& rKey, HDFData& rValue )
{
    bool bSuccess = false;
    if( !m_xSFA.is() )
        return bSuccess;

    try
    {

    if( m_pStringToDataMap == nullptr && m_pStringToValPosMap == nullptr )
    {
        createHashMap( false/*bOptimizeForPerformance*/ );
    }

    if( m_pStringToValPosMap != nullptr )
    {
        StringToValPosMap::const_iterator it = m_pStringToValPosMap->find( rKey );
        if( it != m_pStringToValPosMap->end() )
        {
            const std::pair<int,int>& rValPair = it->second;
            int iValuePos = rValPair.first;
            int nValueLen = rValPair.second;

            Reference< XInputStream > xIn = m_xSFA->openFileRead( m_aFileURL );
            if( xIn.is() )
            {
                Reference< XSeekable > xXSeekable( xIn, UNO_QUERY );
                if( xXSeekable.is() )
                {
                    xXSeekable->seek( iValuePos );

                    Sequence< sal_Int8 > aData;
                    sal_Int32 nRead = xIn->readBytes( aData, nValueLen );
                    if( nRead == nValueLen )
                    {
                        const char* pData = reinterpret_cast<const char*>(aData.getConstArray());
                        rValue.copyToBuffer( pData, nValueLen );
                        bSuccess = true;
                    }
                }
                xIn->closeInput();
            }
        }
    }

    else if( m_pStringToDataMap != nullptr )
    {
        StringToDataMap::const_iterator it = m_pStringToDataMap->find( rKey );
        if( it != m_pStringToDataMap->end() )
        {
            const OString& rValueStr = it->second;
            int nValueLen = rValueStr.getLength();
            const char* pData = rValueStr.getStr();
            rValue.copyToBuffer( pData, nValueLen );
            bSuccess = true;
        }
    }

    }
    catch( Exception & )
    {
        bSuccess = false;
    }

    return bSuccess;
}

bool Hdf::startIteration()
{
    bool bSuccess = false;

    sal_Int32 nSize = m_xSFA->getSize( m_aFileURL );

    Reference< XInputStream > xIn = m_xSFA->openFileRead( m_aFileURL );
    if( xIn.is() )
    {
        m_nItRead = xIn->readBytes( m_aItData, nSize );
        if( m_nItRead == nSize )
        {
            bSuccess = true;
            m_iItPos = 0;
        }
        else
        {
            stopIteration();
        }
    }

    return bSuccess;
}

bool Hdf::getNextKeyAndValue( HDFData& rKey, HDFData& rValue )
{
    bool bSuccess = false;

    if( m_iItPos < m_nItRead )
    {
        auto const p = reinterpret_cast<const char*>(m_aItData.getConstArray());
        if( implReadLenAndData( p, p + m_aItData.size(), m_iItPos, rKey ) )
        {
            if( implReadLenAndData( p, p + m_aItData.size(), m_iItPos, rValue ) )
                bSuccess = true;
        }
    }

    return bSuccess;
}

void Hdf::stopIteration()
{
    m_aItData = Sequence<sal_Int8>();
    m_nItRead = -1;
    m_iItPos = -1;
}

} // end of namespace helpdatafileproxy

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
