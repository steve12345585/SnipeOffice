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


#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <rtl/strbuf.hxx>
#include <rtl/ustrbuf.hxx>
#include <osl/mutex.hxx>

#include <com/sun/star/uno/genfunc.hxx>
#include <typelib/typedescription.hxx>
#include <uno/any2.h>
#include <unordered_map>
#include "share.hxx"


using namespace ::std;
using namespace ::osl;
using namespace ::com::sun::star::uno;
using namespace ::__cxxabiv1;


namespace CPPU_CURRENT_NAMESPACE
{

void dummy_can_throw_anything( char const * )
{
}

static OUString toUNOname( char const * p )
{
#if OSL_DEBUG_LEVEL > 1
    char const * start = p;
#endif

    // example: N3com3sun4star4lang24IllegalArgumentExceptionE

    OUStringBuffer buf( 64 );
    assert( 'N' == *p );
    ++p; // skip N

    while ('E' != *p)
    {
        // read chars count
        long n = (*p++ - '0');
        while ('0' <= *p && '9' >= *p)
        {
            n *= 10;
            n += (*p++ - '0');
        }
        buf.appendAscii( p, n );
        p += n;
        if ('E' != *p)
            buf.append( '.' );
    }

#if OSL_DEBUG_LEVEL > 1
    OUString ret( buf.makeStringAndClear() );
    OString c_ret( OUStringToOString( ret, RTL_TEXTENCODING_ASCII_US ) );
    fprintf( stderr, "> toUNOname(): %s => %s\n", start, c_ret.getStr() );
    return ret;
#else
    return buf.makeStringAndClear();
#endif
}

class RTTI
{
    typedef std::unordered_map< OUString, type_info * > t_rtti_map;

    Mutex m_mutex;
    t_rtti_map m_rttis;
    t_rtti_map m_generatedRttis;

    void * m_hApp;

public:
    RTTI();
    ~RTTI();

    type_info * getRTTI( typelib_CompoundTypeDescription * );
};

RTTI::RTTI()
    : m_hApp( dlopen( 0, RTLD_LAZY ) )
{
}

RTTI::~RTTI()
{
    dlclose( m_hApp );
}


type_info * RTTI::getRTTI( typelib_CompoundTypeDescription *pTypeDescr )
{
    type_info * rtti;

    OUString const & unoName = *(OUString const *)&pTypeDescr->aBase.pTypeName;

    MutexGuard guard( m_mutex );
    t_rtti_map::const_iterator iRttiFind( m_rttis.find( unoName ) );
    if (iRttiFind == m_rttis.end())
    {
        // RTTI symbol
        OStringBuffer buf( 64 );
        buf.append( "_ZTIN" );
        sal_Int32 index = 0;
        do
        {
            OUString token( unoName.getToken( 0, '.', index ) );
            buf.append( token.getLength() );
            OString c_token( OUStringToOString( token, RTL_TEXTENCODING_ASCII_US ) );
            buf.append( c_token );
        }
        while (index >= 0);
        buf.append( 'E' );

        OString symName( buf.makeStringAndClear() );
        rtti = (type_info *)dlsym( m_hApp, symName.getStr() );

        if (rtti)
        {
            pair< t_rtti_map::iterator, bool > insertion(
                m_rttis.insert( t_rtti_map::value_type( unoName, rtti ) ) );
            assert( insertion.second && "### inserting new rtti failed?!");
        }
        else
        {
            // try to lookup the symbol in the generated rtti map
            t_rtti_map::const_iterator iFind( m_generatedRttis.find( unoName ) );
            if (iFind == m_generatedRttis.end())
            {
                // we must generate it !
                // symbol and rtti-name is nearly identical,
                // the symbol is prefixed with _ZTI
                char const * rttiName = symName.getStr() +4;
#if OSL_DEBUG_LEVEL > 1
                fprintf( stderr,"generated rtti for %s\n", rttiName );
#endif
                if (pTypeDescr->pBaseTypeDescription)
                {
                    // ensure availability of base
                    type_info * base_rtti = getRTTI(
                        (typelib_CompoundTypeDescription *)pTypeDescr->pBaseTypeDescription );
                    rtti = new __si_class_type_info(
                        strdup( rttiName ), (__class_type_info *)base_rtti );
                }
                else
                {
                    // this class has no base class
                    rtti = new __class_type_info( strdup( rttiName ) );
                }

                pair< t_rtti_map::iterator, bool > insertion(
                    m_generatedRttis.insert( t_rtti_map::value_type( unoName, rtti ) ) );
                assert(insertion.second && "### inserting new generated rtti failed?!");
            }
            else // taking already generated rtti
            {
                rtti = iFind->second;
            }
        }
    }
    else
    {
        rtti = iRttiFind->second;
    }

    return rtti;
}


static void deleteException( void * pExc )
{
    __cxa_exception const * header = ((__cxa_exception const *)pExc - 1);
    typelib_TypeDescription * pTD = 0;
    OUString unoName( toUNOname( header->exceptionType->name() ) );
    ::typelib_typedescription_getByName( &pTD, unoName.pData );
    assert(pTD && "### unknown exception type! leaving out destruction => leaking!!!");
    if (pTD)
    {
        ::uno_destructData( pExc, pTD, cpp_release );
        ::typelib_typedescription_release( pTD );
    }
}

void raiseException( uno_Any * pUnoExc, uno_Mapping * pUno2Cpp )
{
    void * pCppExc;
    type_info * rtti;

    {
    // construct cpp exception object
    typelib_TypeDescription * pTypeDescr = 0;
    TYPELIB_DANGER_GET( &pTypeDescr, pUnoExc->pType );
    assert(pTypeDescr);
    if (! pTypeDescr)
        terminate();

    pCppExc = __cxa_allocate_exception( pTypeDescr->nSize );
    ::uno_copyAndConvertData( pCppExc, pUnoExc->pData, pTypeDescr, pUno2Cpp );

    // destruct uno exception
    ::uno_any_destruct( pUnoExc, 0 );
    // avoiding locked counts
    static RTTI rtti_data;
    rtti = (type_info*)rtti_data.getRTTI((typelib_CompoundTypeDescription*)pTypeDescr);
    TYPELIB_DANGER_RELEASE( pTypeDescr );
    if (! rtti)
        terminate();
    }

    __cxa_throw( pCppExc, rtti, deleteException );
}

void fillUnoException(uno_Any * pExc, uno_Mapping * pCpp2Uno)
{
    __cxa_exception * header = __cxa_get_globals()->caughtExceptions;
    if (! header)
        terminate();

    std::type_info *exceptionType = __cxa_current_exception_type();

    typelib_TypeDescription * pExcTypeDescr = 0;
    OUString unoName( toUNOname( exceptionType->name() ) );
    ::typelib_typedescription_getByName( &pExcTypeDescr, unoName.pData );
    if (! pExcTypeDescr)
        terminate();

    // construct uno exception any
    ::uno_any_constructAndConvert( pExc, header->adjustedPtr, pExcTypeDescr, pCpp2Uno );
    ::typelib_typedescription_release( pExcTypeDescr );
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
