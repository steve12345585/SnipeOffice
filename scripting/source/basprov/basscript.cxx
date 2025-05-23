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

#include "basscript.hxx"
#include <utility>
#include <vcl/svapp.hxx>
#include <basic/sbx.hxx>
#include <basic/sbmod.hxx>
#include <basic/sbmeth.hxx>
#include <basic/sbuno.hxx>
#include <basic/basmgr.hxx>
#include <com/sun/star/script/provider/ScriptFrameworkErrorException.hpp>
#include <com/sun/star/script/provider/ScriptFrameworkErrorType.hpp>
#include <comphelper/propertycontainer.hxx>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <map>


using namespace ::com::sun::star;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::script;
using namespace ::com::sun::star::document;
using namespace ::com::sun::star::beans;


static void ChangeTypeKeepingValue(SbxVariable& var, SbxDataType to)
{
    SbxValues val(to);
    var.Get(val);
    bool bSetFlag = var.IsSet(SbxFlagBits::Fixed);
    var.ResetFlag(SbxFlagBits::Fixed);
    var.Put(val);
    if (bSetFlag)
        var.SetFlag(SbxFlagBits::Fixed);
}

namespace basprov
{

#define BASSCRIPT_PROPERTY_ID_CALLER         1
constexpr OUString BASSCRIPT_PROPERTY_CALLER = u"Caller"_ustr;

#define BASSCRIPT_DEFAULT_ATTRIBS()       PropertyAttribute::BOUND | PropertyAttribute::TRANSIENT

    typedef ::std::map< sal_Int16, Any > OutParamMap;


    // BasicScriptImpl


    BasicScriptImpl::BasicScriptImpl( OUString funcName, SbMethodRef xMethod )
        :
        m_xMethod(std::move( xMethod ))
        ,m_funcName(std::move( funcName ))
        ,m_documentBasicManager( nullptr )
        ,m_xDocumentScriptContext()
    {
        registerProperty( BASSCRIPT_PROPERTY_CALLER, BASSCRIPT_PROPERTY_ID_CALLER, BASSCRIPT_DEFAULT_ATTRIBS(), &m_caller, cppu::UnoType<decltype(m_caller)>::get() );
    }


    BasicScriptImpl::BasicScriptImpl( OUString funcName, SbMethodRef xMethod,
        BasicManager& documentBasicManager, const Reference< XScriptInvocationContext >& documentScriptContext ) :
        m_xMethod(std::move( xMethod ))
        ,m_funcName(std::move( funcName ))
        ,m_documentBasicManager( &documentBasicManager )
        ,m_xDocumentScriptContext( documentScriptContext )
    {
        StartListening( *m_documentBasicManager );
        registerProperty( BASSCRIPT_PROPERTY_CALLER, BASSCRIPT_PROPERTY_ID_CALLER, BASSCRIPT_DEFAULT_ATTRIBS(), &m_caller, cppu::UnoType<decltype(m_caller)>::get() );
    }


    BasicScriptImpl::~BasicScriptImpl()
    {
        SolarMutexGuard g;

        if ( m_documentBasicManager )
            EndListening( *m_documentBasicManager );
    }


    // SfxListener

    void BasicScriptImpl::Notify( SfxBroadcaster& rBC, const SfxHint& rHint )
    {
        if ( &rBC != m_documentBasicManager )
        {
            OSL_ENSURE( false, "BasicScriptImpl::Notify: where does this come from?" );
            // not interested in
            return;
        }
        if ( rHint.GetId() == SfxHintId::Dying )
        {
            m_documentBasicManager = nullptr;
            EndListening( rBC );    // prevent multiple notifications
        }
    }


    // XInterface


    IMPLEMENT_FORWARD_XINTERFACE2( BasicScriptImpl, BasicScriptImpl_BASE, comphelper::OPropertyContainer2 )


    // XTypeProvider


    IMPLEMENT_FORWARD_XTYPEPROVIDER2( BasicScriptImpl, BasicScriptImpl_BASE, comphelper::OPropertyContainer2 )


    // OPropertySetHelper


    ::cppu::IPropertyArrayHelper& BasicScriptImpl::getInfoHelper(  )
    {
        return *getArrayHelper();
    }


    // OPropertyArrayUsageHelper


    ::cppu::IPropertyArrayHelper* BasicScriptImpl::createArrayHelper(  ) const
    {
        Sequence< Property > aProps;
        describeProperties( aProps );
        return new ::cppu::OPropertyArrayHelper( aProps );
    }


    // XPropertySet


    Reference< XPropertySetInfo > BasicScriptImpl::getPropertySetInfo(  )
    {
        Reference< XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
        return xInfo;
    }


    // XScript


    Any BasicScriptImpl::invoke( const Sequence< Any >& aParams, Sequence< sal_Int16 >& aOutParamIndex, Sequence< Any >& aOutParam )
    {
        // TODO: throw CannotConvertException
        // TODO: check length of aOutParamIndex, aOutParam

        SolarMutexGuard aGuard;

        Any aReturn;

        if ( m_xMethod.is() )
        {
            // check if compiled
            SbModule* pModule = static_cast< SbModule* >( m_xMethod->GetParent() );
            if ( pModule && !pModule->IsCompiled() )
                pModule->Compile();

            // check number of parameters
            sal_Int32 nParamsCount = aParams.getLength();
            SbxInfo* pInfo = m_xMethod->GetInfo();
            if ( pInfo )
            {
                sal_Int32 nSbxOptional = 0;
                sal_uInt16 n = 1;
                for ( const SbxParamInfo* pParamInfo = pInfo->GetParam( n ); pParamInfo; pParamInfo = pInfo->GetParam( ++n ) )
                {
                    if ( pParamInfo->nFlags & SbxFlagBits::Optional )
                        ++nSbxOptional;
                    else
                        nSbxOptional = 0;
                }
                sal_Int32 nSbxCount = n - 1;
                if ( nParamsCount < nSbxCount - nSbxOptional )
                {
                    throw provider::ScriptFrameworkErrorException(
                         u"wrong number of parameters!"_ustr,
                         Reference< XInterface >(),
                         m_funcName,
                         u"Basic"_ustr,
                         provider::ScriptFrameworkErrorType::NO_SUCH_SCRIPT  );
                }
            }

            // set parameters
            SbxArrayRef xSbxParams;
            if ( nParamsCount > 0 )
            {
                xSbxParams = new SbxArray;
                for ( sal_Int32 i = 0; i < nParamsCount; ++i )
                {
                    SbxVariableRef xSbxVar = new SbxVariable( SbxVARIANT );
                    unoToSbxValue(xSbxVar.get(), aParams[i]);
                    xSbxParams->Put(xSbxVar.get(), static_cast<sal_uInt32>(i) + 1);

                    if (pInfo)
                    {
                        if (auto* p = pInfo->GetParam(static_cast<sal_uInt16>(i) + 1))
                        {
                            SbxDataType t = static_cast<SbxDataType>(p->eType & 0x0FFF);
                            // tdf#133889 Revert the downcasting performed in sbxToUnoValueImpl
                            // to allow passing by reference.
                            SbxDataType a = xSbxVar->GetType();
                            if (t == SbxSINGLE && (a == SbxINTEGER || a == SbxLONG))
                            {
                                sal_Int32 val = xSbxVar->GetLong();
                                if (val >= -16777216 && val <= 16777215)
                                    ChangeTypeKeepingValue(*xSbxVar, t);
                            }
                            else if (t == SbxDOUBLE && (a == SbxINTEGER || a == SbxLONG))
                                ChangeTypeKeepingValue(*xSbxVar, t);
                            else if (t == SbxLONG && a == SbxINTEGER)
                                ChangeTypeKeepingValue(*xSbxVar, t);
                            else if (t == SbxULONG && a == SbxUSHORT)
                                ChangeTypeKeepingValue(*xSbxVar, t);
                            // Enable passing by ref
                            if (t != SbxVARIANT)
                                xSbxVar->SetFlag(SbxFlagBits::Fixed);
                        }
                    }
                }
            }
            if ( xSbxParams.is() )
                m_xMethod->SetParameters( xSbxParams.get() );

            // call method
            SbxVariableRef xReturn = new SbxVariable;
            ErrCode nErr = ERRCODE_NONE;

            // if it's a document-based script, temporarily reset ThisComponent to the script invocation context
            Any aOldThisComponent;
            if ( m_documentBasicManager && m_xDocumentScriptContext.is() )
                m_documentBasicManager->SetGlobalUNOConstant( u"ThisComponent"_ustr, Any( m_xDocumentScriptContext ), &aOldThisComponent );

            if ( m_caller.hasElements() && m_caller[ 0 ].hasValue()  )
            {
                SbxVariableRef xCallerVar = new SbxVariable( SbxVARIANT );
                unoToSbxValue( xCallerVar.get(), m_caller[ 0 ] );
                nErr = m_xMethod->Call( xReturn.get(), xCallerVar.get() );
            }
            else
                nErr = m_xMethod->Call( xReturn.get() );

            if ( m_documentBasicManager && m_xDocumentScriptContext.is() )
                m_documentBasicManager->SetGlobalUNOConstant( u"ThisComponent"_ustr, aOldThisComponent );

            if ( nErr != ERRCODE_NONE )
            {
                // TODO: throw InvocationTargetException ?
            }

            // get output parameters
            if ( xSbxParams.is() )
            {
                SbxInfo* pInfo_ = m_xMethod->GetInfo();
                if ( pInfo_ )
                {
                    OutParamMap aOutParamMap;
                    for (sal_uInt32 n = 1, nCount = xSbxParams->Count(); n < nCount; ++n)
                    {
                        assert(nCount <= std::numeric_limits<sal_uInt16>::max());
                        const SbxParamInfo* pParamInfo = pInfo_->GetParam( sal::static_int_cast<sal_uInt16>(n) );
                        if ( pParamInfo && ( pParamInfo->eType & SbxBYREF ) != 0 )
                        {
                            SbxVariable* pVar = xSbxParams->Get(n);
                            if ( pVar )
                            {
                                SbxVariableRef xVar = pVar;
                                aOutParamMap.emplace( n - 1, sbxToUnoValue( xVar.get() ) );
                            }
                        }
                    }
                    sal_Int32 nOutParamCount = aOutParamMap.size();
                    aOutParamIndex.realloc( nOutParamCount );
                    aOutParam.realloc( nOutParamCount );
                    sal_Int16* pOutParamIndex = aOutParamIndex.getArray();
                    Any* pOutParam = aOutParam.getArray();
                    for ( const auto& rEntry : aOutParamMap )
                    {
                        *pOutParamIndex = rEntry.first;
                        ++pOutParamIndex;
                        *pOutParam = rEntry.second;
                        ++pOutParam;
                    }
                }
            }

            // get return value
            aReturn = sbxToUnoValue( xReturn.get() );

            // reset parameters
            m_xMethod->SetParameters( nullptr );
        }

        return aReturn;
    }


}   // namespace basprov


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
