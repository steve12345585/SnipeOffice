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

#include <osl/diagnose.h>

#include "olewrapclient.hxx"
#include "olecomponent.hxx"

// TODO: May be a mutex must be introduced

OleWrapperClientSite::OleWrapperClientSite( OleComponent* pOleComp )
: m_nRefCount( 0 )
, m_pOleComp( pOleComp )
{
    OSL_ENSURE( m_pOleComp, "No ole component is provided!" );
}

OleWrapperClientSite::~OleWrapperClientSite()
{
}

STDMETHODIMP OleWrapperClientSite::QueryInterface( REFIID riid , void** ppv )
{
    *ppv=nullptr;

    if ( riid == IID_IUnknown )
        *ppv = static_cast<IUnknown*>(this);

    if ( riid == IID_IOleClientSite )
        *ppv = static_cast<IOleClientSite*>(this);

    if ( *ppv != nullptr )
    {
        static_cast<IUnknown*>(*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) OleWrapperClientSite::AddRef()
{
    return osl_atomic_increment( &m_nRefCount);
}

STDMETHODIMP_(ULONG) OleWrapperClientSite::Release()
{
    ULONG nReturn = --m_nRefCount;
    if ( m_nRefCount == 0 )
        delete this;

    return nReturn;
}

void OleWrapperClientSite::disconnectOleComponent()
{
    // must not be called from the descructor of OleComponent!!!
    osl::MutexGuard aGuard( m_aMutex );
    m_pOleComp = nullptr;
}

STDMETHODIMP OleWrapperClientSite::SaveObject()
{
    OleComponent* pLockComponent = nullptr;
    HRESULT hResult = E_FAIL;

    {
        osl::MutexGuard aGuard( m_aMutex );
        if ( m_pOleComp )
        {
            pLockComponent = m_pOleComp;
            pLockComponent->acquire();
        }
    }

    if ( pLockComponent )
    {
        if ( pLockComponent->SaveObject_Impl() )
            hResult = S_OK;

        pLockComponent->release();
    }

    return hResult;
}

STDMETHODIMP OleWrapperClientSite::GetMoniker( DWORD, DWORD, IMoniker **ppmk )
{
    *ppmk = nullptr;
    return E_NOTIMPL;
}

STDMETHODIMP OleWrapperClientSite::GetContainer( IOleContainer** ppContainer )
{
    *ppContainer = nullptr;
    return E_NOTIMPL;
}

STDMETHODIMP OleWrapperClientSite::ShowObject()
{
    return S_OK;
}

STDMETHODIMP OleWrapperClientSite::OnShowWindow( BOOL bShow )
{
    OleComponent* pLockComponent = nullptr;

    // TODO/LATER: redirect the notification to the main thread so that SolarMutex can be locked
    {
        osl::MutexGuard aGuard( m_aMutex );
        if ( m_pOleComp )
        {
            pLockComponent = m_pOleComp;
            pLockComponent->acquire();
        }
    }

    if ( pLockComponent )
    {
        pLockComponent->OnShowWindow_Impl( bShow ); // the result is not interesting
        pLockComponent->release();
    }

    return S_OK;
}

STDMETHODIMP OleWrapperClientSite::RequestNewObjectLayout()
{
    return E_NOTIMPL;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
