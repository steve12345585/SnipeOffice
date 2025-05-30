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

#include <sal/types.h>

#include <embservconst.h>
#include "inprocembobj.h"

namespace inprocserv
{

namespace {

class ULONGGuard
{
    ULONG& m_rValue;

public:
    explicit ULONGGuard( ULONG& rValue )
    : m_rValue( ++rValue )
    {
    }

    ~ULONGGuard() { --m_rValue; }
};

void SetName( LPCOLESTR pszNameFromOutside, wchar_t*& pOwnName )
{
    if ( !pszNameFromOutside )
        return;

    // copy the string
    size_t nLen = 0;
    while( pszNameFromOutside[nLen] != 0 )
        nLen++;

    if ( pOwnName )
    {
        delete[] pOwnName;
        pOwnName = nullptr;
    }

    pOwnName = new wchar_t[nLen+1];
    for ( size_t nInd = 0; nInd < nLen; nInd++ )
        pOwnName[nInd] = pszNameFromOutside[nInd];
    pOwnName[nLen] = 0;
}

DWORD InsertAdviseLinkToList( const sal::systools::COMReference<OleWrapperAdviseSink>& pOwnAdvise, sal::systools::COMReference< OleWrapperAdviseSink > pAdvises[] )
{
    // the result should start from 1 in case of success, the element 0 can be used for own needs
    DWORD nResult = 0;

    if ( pOwnAdvise )
    {
        for ( DWORD nInd = 1; nInd < DEFAULT_ARRAY_LEN && nResult == 0; nInd++ )
        {
            if ( pAdvises[nInd].get() == pOwnAdvise.get() )
            {
                nResult = nInd;
            }
            else if ( pAdvises[nInd] == nullptr )
            {
                pAdvises[nInd] = pOwnAdvise;
                nResult = nInd;
            }
        }
    }

    return nResult;
}

}

BOOL InprocEmbedDocument_Impl::CheckDefHandler()
{
    // set the own listener
    if ( m_pOleAdvises[0] == nullptr )
    {
        m_pOleAdvises[0] = new OleWrapperAdviseSink();
    }
    else
    {
        if ( m_pOleAdvises[0]->IsClosed() )
        {
            if ( m_pDefHandler )
            {
                // deregister all the listeners

                sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);
                if ( pOleObject )
                {
                    for ( DWORD nInd = 0; nInd < DEFAULT_ARRAY_LEN; nInd++ )
                        if ( m_pOleAdvises[nInd] )
                        {
                            DWORD nID = m_pOleAdvises[nInd]->GetRegID();
                            pOleObject->Unadvise( nID );
                            m_pOleAdvises[nInd]->SetRegID( 0 );
                        }

                    pOleObject->SetClientSite( nullptr );
                }

                sal::systools::COMReference< IDataObject > pIDataObject(m_pDefHandler, sal::systools::COM_QUERY);
                if ( pIDataObject )
                {
                    for ( DWORD nInd = 0; nInd < DEFAULT_ARRAY_LEN; nInd++ )
                        if ( m_pDataAdvises[nInd] )
                        {
                            DWORD nID = m_pDataAdvises[nInd]->GetRegID();
                            pIDataObject->DUnadvise( nID );
                            m_pDataAdvises[nInd]->SetRegID( 0 );
                        }
                }

                sal::systools::COMReference< IViewObject > pIViewObject(m_pDefHandler, sal::systools::COM_QUERY);
                if ( pIViewObject )
                {
                    if ( m_pViewAdvise )
                        pIViewObject->SetAdvise( m_pViewAdvise->GetAspect(), m_pViewAdvise->GetViewAdviseFlag(), nullptr );
                }

                sal::systools::COMReference< IPersistStorage > pPersist(m_pDefHandler, sal::systools::COM_QUERY);
                if ( pPersist )
                {
                    // disconnect the old wrapper from the storage
                    pPersist->HandsOffStorage();
                }

                m_pDefHandler = nullptr;
            }

            m_pOleAdvises[0]->UnsetClosed();
        }
    }

    if ( m_nCallsOnStack )
        return FALSE;

    if ( !m_pDefHandler )
    {
        // create a new default inprocess handler
        HRESULT hr = OleCreateDefaultHandler( m_guid, nullptr, IID_IUnknown, reinterpret_cast<void**>(&m_pDefHandler) );
        if ( SUCCEEDED( hr ) )
        {
            if ( m_nInitMode == INIT_FROM_STORAGE )
            {
                sal::systools::COMReference< IPersistStorage > pPersist(m_pDefHandler, sal::systools::COM_QUERY);

                ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
                if ( pPersist && m_pStorage )
                    hr = pPersist->InitNew( m_pStorage.get() );
            }
            else if ( m_nInitMode == LOAD_FROM_STORAGE )
            {
                sal::systools::COMReference< IPersistStorage > pPersist(m_pDefHandler, sal::systools::COM_QUERY);

                ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
                if ( pPersist && m_pStorage )
                    hr = pPersist->Load( m_pStorage.get() );
            }
            else if ( m_nInitMode == LOAD_FROM_FILE )
            {
                sal::systools::COMReference< IPersistFile > pPersistFile(m_pDefHandler, sal::systools::COM_QUERY);

                ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
                if ( pPersistFile && m_pFileName )
                    hr = pPersistFile->Load( m_pFileName, m_nFileOpenMode );
            }
        }

        if ( !SUCCEEDED( hr ) || !m_pDefHandler )
        {
            m_pDefHandler = nullptr;
            return FALSE;
        }

        // register all the listeners new

        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);
        if ( pOleObject )
        {
            if ( m_pClientSite )
                pOleObject->SetClientSite( m_pClientSite.get() );

            for ( DWORD nInd = 0; nInd < DEFAULT_ARRAY_LEN; nInd++ )
                if ( m_pOleAdvises[nInd] )
                {
                    DWORD nRegID = 0;
                    if ( SUCCEEDED( pOleObject->Advise( m_pOleAdvises[nInd].get(), &nRegID ) ) && nRegID > 0 )
                        m_pOleAdvises[nInd]->SetRegID( nRegID );
                }
        }

        sal::systools::COMReference< IDataObject > pIDataObject(m_pDefHandler, sal::systools::COM_QUERY);
        if ( pIDataObject )
        {
            for ( DWORD nInd = 0; nInd < DEFAULT_ARRAY_LEN; nInd++ )
                if ( m_pDataAdvises[nInd] )
                {
                    DWORD nRegID = 0;
                    if ( SUCCEEDED( pIDataObject->DAdvise( m_pDataAdvises[nInd]->GetFormatEtc(), m_pDataAdvises[nInd]->GetDataAdviseFlag(), m_pDataAdvises[nInd].get(), &nRegID ) ) && nRegID > 0 )
                        m_pDataAdvises[nInd]->SetRegID( nRegID );
                }
        }

        sal::systools::COMReference< IViewObject > pIViewObject(m_pDefHandler, sal::systools::COM_QUERY);
        if ( pIViewObject )
        {
            if ( m_pViewAdvise )
                pIViewObject->SetAdvise( m_pViewAdvise->GetAspect(), m_pViewAdvise->GetViewAdviseFlag(), m_pViewAdvise.get() );
        }
    }


    return TRUE;
}

void InprocEmbedDocument_Impl::Clean()
{
    m_pDefHandler = nullptr;

    // no DisconnectOrigAdvise() call here, since it is no explicit disconnection
    for ( DWORD nInd = 0; nInd < DEFAULT_ARRAY_LEN; nInd++ )
    {
        m_pOleAdvises[nInd].clear();
        m_pDataAdvises[nInd].clear();
    }

    m_pViewAdvise = nullptr;

    m_nInitMode = NOINIT;
    m_pStorage = nullptr;

    if ( m_pOleContainer )
    {
        m_pOleContainer->LockContainer( FALSE );
        m_pOleContainer = nullptr;
    }

    m_pClientSite = nullptr;

    m_nFileOpenMode = 0;
    if ( m_pFileName )
    {
        delete m_pFileName;
        m_pFileName = nullptr;
    }
}

// IUnknown

COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::QueryInterface(REFIID riid, void** ppv)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppv = static_cast<IUnknown*>(static_cast<IPersistStorage*>(this));
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IPersist))
    {
        AddRef();
        *ppv = static_cast<IPersist*>(static_cast<IPersistStorage*>(this));
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IPersistStorage))
    {
        AddRef();
        *ppv = static_cast<IPersistStorage*>(this);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IDataObject))
    {
        AddRef();
        *ppv = static_cast<IDataObject*>(this);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IOleObject))
    {
        AddRef();
        *ppv = static_cast<IOleObject*>(this);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IPersistFile))
    {
        AddRef();
        *ppv = static_cast<IPersistFile*>(this);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IRunnableObject))
    {
        AddRef();
        *ppv = static_cast<IRunnableObject*>(this);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IViewObject))
    {
        AddRef();
        *ppv = static_cast<IViewObject*>(this);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IViewObject2))
    {
        AddRef();
        *ppv = static_cast<IViewObject2*>(this);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IOleCache))
    {
        AddRef();
        *ppv = static_cast<IOleCache*>(&m_aInternalCache);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IOleCache2))
    {
        AddRef();
        *ppv = static_cast<IOleCache2*>(&m_aInternalCache);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IOleWindow))
    {
        AddRef();
        *ppv = static_cast<IOleWindow*>(this);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IOleInPlaceObject))
    {
        AddRef();
        *ppv = static_cast<IOleInPlaceObject*>(this);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IDispatch))
    {
        AddRef();
        *ppv = static_cast<IDispatch*>(this);
        return S_OK;
    }

    *ppv = nullptr;
    return ResultFromScode(E_NOINTERFACE);
}


COM_DECLSPEC_NOTHROW STDMETHODIMP_(ULONG) InprocEmbedDocument_Impl::AddRef()
{
    return ++m_refCount;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP_(ULONG) InprocEmbedDocument_Impl::Release()
{
    // unfortunately there are reentrance problems in mfc that have to be workarounded
    sal_Int32 nCount = m_refCount > 0 ? --m_refCount : 0;
    if ( nCount == 0 && !m_bDeleted )
    {
        // deleting of this object can trigger deleting of mfc objects that will try to delete this object one more time
        m_bDeleted = TRUE;

        Clean();
        delete this;
    }
    return nCount;
}

// IPersist

COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetClassID( CLSID* pClassId )
{
    *pClassId = m_guid;
    return S_OK;
}

// IPersistStorage

COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::IsDirty()
{
    if ( m_pDefHandler == nullptr || m_pOleAdvises[0] == nullptr || m_pOleAdvises[0]->IsClosed() )
        return S_FALSE;

    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IPersistStorage > pPersist(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pPersist )
            return pPersist->IsDirty();
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::InitNew( IStorage *pStg )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IPersistStorage > pPersist(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pPersist )
        {
            HRESULT hr = pPersist->InitNew( pStg );
            if ( SUCCEEDED( hr ) )
            {
                m_nInitMode = INIT_FROM_STORAGE;
                m_pStorage = pStg;

                m_nFileOpenMode = 0;
                if ( m_pFileName )
                {
                    delete[] m_pFileName;
                    m_pFileName = nullptr;
                }
            }

            return hr;
        }
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::Load( IStorage *pStg )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IPersistStorage > pPersist(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pPersist )
        {
            HRESULT hr = pPersist->Load( pStg );
            if ( SUCCEEDED( hr ) )
            {
                m_nInitMode = LOAD_FROM_STORAGE;
                m_pStorage = pStg;

                m_nFileOpenMode = 0;
                if ( m_pFileName )
                {
                    delete[] m_pFileName;
                    m_pFileName = nullptr;
                }
            }

            return hr;
        }
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::Save( IStorage *pStgSave, BOOL fSameAsLoad )
{
    if ( fSameAsLoad && ( m_pDefHandler == nullptr || m_pOleAdvises[0] == nullptr || m_pOleAdvises[0]->IsClosed() ) )
        return S_OK;

    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IPersistStorage > pPersist(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pPersist )
            return pPersist->Save( pStgSave, fSameAsLoad );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::SaveCompleted( IStorage *pStgNew )
{
    if ( m_pDefHandler == nullptr || m_pOleAdvises[0] == nullptr || m_pOleAdvises[0]->IsClosed() )
    {
        if ( pStgNew )
            m_pStorage = pStgNew;

        return S_OK;
    }

    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IPersistStorage > pPersist(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pPersist )
        {
            HRESULT hr = pPersist->SaveCompleted( pStgNew );
            if ( SUCCEEDED( hr ) )
            {
                m_nInitMode = LOAD_FROM_STORAGE;
                if ( pStgNew )
                {
                    m_pStorage = pStgNew;
                }

                m_nFileOpenMode = 0;
                if ( m_pFileName )
                {
                    delete[] m_pFileName;
                    m_pFileName = nullptr;
                }
            }

            return hr;
        }
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::HandsOffStorage()
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IPersistStorage > pPersist(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pPersist )
        {
            HRESULT hr = pPersist->HandsOffStorage();
            if ( SUCCEEDED( hr ) )
            {
                m_pStorage = nullptr;
            }

            return hr;
        }
    }

    return E_FAIL;
}

// IPersistFile

COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::Load( LPCOLESTR pszFileName, DWORD dwMode )
{
    if ( CheckDefHandler() && pszFileName )
    {
        sal::systools::COMReference< IPersistFile > pPersist(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pPersist )
        {
            HRESULT hr = pPersist->Load( pszFileName, dwMode );
            if ( SUCCEEDED( hr ) )
            {
                m_nInitMode = LOAD_FROM_FILE;
                if ( m_pStorage )
                    m_pStorage = nullptr;

                m_nFileOpenMode = dwMode;
                // copy the string
                SetName( pszFileName, m_pFileName );
            }

            return hr;
        }
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::Save( LPCOLESTR pszFileName, BOOL fRemember )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IPersistFile > pPersist(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pPersist )
            return pPersist->Save( pszFileName, fRemember );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::SaveCompleted( LPCOLESTR pszFileName )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IPersistFile > pPersist(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pPersist )
        {
            HRESULT hr = pPersist->SaveCompleted( pszFileName );
            if ( SUCCEEDED( hr ) )
            {
                m_nInitMode = LOAD_FROM_STORAGE;
                if ( m_pStorage )
                    m_pStorage = nullptr;

                m_nFileOpenMode = STGM_READWRITE; // was just written
                // copy the string
                SetName( pszFileName, m_pFileName );
            }
        }

    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetCurFile( LPOLESTR *ppszFileName )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IPersistFile > pPersist(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pPersist )
            return pPersist->GetCurFile( ppszFileName );
    }

    return E_FAIL;
}

// IOleObject

COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::SetClientSite( IOleClientSite* pSite )
{
    if ( pSite == m_pClientSite.get() )
        return S_OK;

    if ( !pSite )
    {
        m_pClientSite = nullptr;
        if ( m_pOleContainer )
        {
            m_pOleContainer->LockContainer( FALSE );
            m_pOleContainer = nullptr;
        }
    }

    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
        {
            HRESULT hr2 = pOleObject->SetClientSite( pSite );
            if ( SUCCEEDED( hr2 ) )
            {
                m_pClientSite = pSite;

                if ( m_pOleContainer )
                {
                    m_pOleContainer->LockContainer( FALSE );
                    m_pOleContainer = nullptr;
                }

                m_pClientSite->GetContainer( &m_pOleContainer );
                if ( m_pOleContainer )
                    m_pOleContainer->LockContainer( TRUE );
            }

            return hr2;
        }
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetClientSite( IOleClientSite** pSite )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
            return pOleObject->GetClientSite( pSite );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::SetHostNames( LPCOLESTR szContainerApp, LPCOLESTR szContainerObj )
{

    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
        {
            pOleObject->SetHostNames( szContainerApp, szContainerObj );
        }
    }

    return S_OK;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::Close( DWORD dwSaveOption )
{
    HRESULT ret = S_OK;
    if ( m_pDefHandler && CheckDefHandler() )
    {
        // no need to close if there is no default handler.
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
        {
            HRESULT hr = pOleObject->Close( dwSaveOption );
            if (!SUCCEEDED(hr))
               ret = hr;
            hr = CoDisconnectObject( static_cast<IUnknown*>(static_cast<IPersistStorage*>(this)), 0 );
            if (!SUCCEEDED(hr) && SUCCEEDED(ret))
               ret = hr;
        }
    }

    // if the object is closed from outside that means that it should go to uninitialized state
    Clean();

    return ret;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::SetMoniker( DWORD dwWhichMoniker, IMoniker * pmk )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
            return pOleObject->SetMoniker( dwWhichMoniker, pmk );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetMoniker( DWORD dwAssign, DWORD dwWhichMoniker, IMoniker ** ppmk )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
            return pOleObject->GetMoniker( dwAssign, dwWhichMoniker, ppmk );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::InitFromData( IDataObject * pDataObject, BOOL fCreation, DWORD dwReserved )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
            return pOleObject->InitFromData( pDataObject, fCreation, dwReserved );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetClipboardData( DWORD dwReserved, IDataObject ** ppDataObject )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
            return pOleObject->GetClipboardData( dwReserved, ppDataObject );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::DoVerb(
    LONG iVerb,
    LPMSG pMsg,
    IOleClientSite *pActiveSite,
    LONG nLong,
    HWND hWin,
    LPCRECT pRect )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
        {
            return pOleObject->DoVerb( iVerb, pMsg, pActiveSite, nLong, hWin, pRect );
        }

    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::EnumVerbs( IEnumOLEVERB ** ppEnumOleVerb )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
            return pOleObject->EnumVerbs( ppEnumOleVerb );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::Update()
{

    if ( m_pDefHandler && CheckDefHandler() )
    {
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
            return pOleObject->Update();
    }

    return S_OK;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::IsUpToDate()
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
            return pOleObject->IsUpToDate();
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetUserClassID( CLSID *pClsid )
{
    if ( pClsid )
        *pClsid = m_guid;

    return S_OK;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetUserType( DWORD dwFormOfType, LPOLESTR * pszUserType )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
            return pOleObject->GetUserType( dwFormOfType, pszUserType );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::SetExtent( DWORD dwDrawAspect, SIZEL *psizel )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
            return pOleObject->SetExtent( dwDrawAspect, psizel );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetExtent( DWORD dwDrawAspect, SIZEL * psizel )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
            return pOleObject->GetExtent( dwDrawAspect, psizel );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::Advise( IAdviseSink *pAdvSink, DWORD *pdwConnection )
{

    if ( !pdwConnection )
        return E_FAIL;

    // CheckDefHandler will set the listener, avoid reusing of old listener
    if ( DEFAULT_ARRAY_LEN > *pdwConnection && *pdwConnection > 0 && m_pOleAdvises[*pdwConnection] )
    {
        m_pOleAdvises[*pdwConnection]->DisconnectOrigAdvise();
        m_pOleAdvises[*pdwConnection] = nullptr;
    }

    if ( pAdvSink && CheckDefHandler() )
    {
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
        {
            sal::systools::COMReference pOwnAdvise(new OleWrapperAdviseSink(pAdvSink));
            DWORD nRegID = 0;

            if ( SUCCEEDED( pOleObject->Advise( pOwnAdvise.get(), &nRegID ) ) && nRegID > 0 )
            {
                pOwnAdvise->SetRegID( nRegID );
                *pdwConnection = InsertAdviseLinkToList( pOwnAdvise, m_pOleAdvises );
                if ( *pdwConnection )
                    return S_OK;
                else
                    pOleObject->Unadvise( nRegID );
            }
        }
    }

    // return success always for now
    return S_OK;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::Unadvise( DWORD dwConnection )
{
    if ( DEFAULT_ARRAY_LEN > dwConnection && dwConnection > 0 && m_pOleAdvises[dwConnection] )
    {
        if ( m_pDefHandler )
        {
            sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

            ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
            if ( pOleObject )
            {
                DWORD nID = m_pOleAdvises[dwConnection]->GetRegID();
                pOleObject->Unadvise( nID );
            }
        }

        m_pOleAdvises[dwConnection]->DisconnectOrigAdvise();
        m_pOleAdvises[dwConnection] = nullptr;

        return S_OK;
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::EnumAdvise( IEnumSTATDATA ** /*ppenumAdvise*/ )
{
    return E_NOTIMPL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetMiscStatus( DWORD dwAspect, DWORD * pdwStatus )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
            return pOleObject->GetMiscStatus( dwAspect, pdwStatus );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::SetColorScheme( LOGPALETTE * pLogpal )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleObject > pOleObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pOleObject )
            return pOleObject->SetColorScheme( pLogpal );
    }

    return E_FAIL;
}

//IDataObject

COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetData( FORMATETC * pFormatetc, STGMEDIUM * pMedium )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IDataObject > pIDataObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIDataObject )
            return pIDataObject->GetData( pFormatetc, pMedium );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetDataHere( FORMATETC * pFormatetc, STGMEDIUM * pMedium )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IDataObject > pIDataObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIDataObject )
            return pIDataObject->GetDataHere( pFormatetc, pMedium );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::QueryGetData( FORMATETC * pFormatetc )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IDataObject > pIDataObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIDataObject )
            return pIDataObject->QueryGetData( pFormatetc );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetCanonicalFormatEtc( FORMATETC * pFormatetcIn, FORMATETC * pFormatetcOut )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IDataObject > pIDataObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIDataObject )
            return pIDataObject->GetCanonicalFormatEtc( pFormatetcIn, pFormatetcOut );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::SetData( FORMATETC * pFormatetc, STGMEDIUM * pMedium, BOOL fRelease )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IDataObject > pIDataObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIDataObject )
            return pIDataObject->SetData( pFormatetc, pMedium, fRelease );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::EnumFormatEtc( DWORD dwDirection, IEnumFORMATETC ** ppFormatetc )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IDataObject > pIDataObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIDataObject )
            return pIDataObject->EnumFormatEtc( dwDirection, ppFormatetc );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::DAdvise( FORMATETC * pFormatetc, DWORD advf, IAdviseSink * pAdvSink, DWORD * pdwConnection )
{

    if ( !pdwConnection )
        return E_FAIL;

    // avoid reusing of the old listener
    if ( m_pDefHandler && DEFAULT_ARRAY_LEN > *pdwConnection && *pdwConnection > 0 && m_pDataAdvises[*pdwConnection] )
    {
        m_pDataAdvises[*pdwConnection]->DisconnectOrigAdvise();
        m_pDataAdvises[*pdwConnection] = nullptr;
    }

    if ( pAdvSink && CheckDefHandler() )
    {
        sal::systools::COMReference< IDataObject > pIDataObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIDataObject )
        {
            sal::systools::COMReference pOwnAdvise( new OleWrapperAdviseSink( pAdvSink, pFormatetc, advf ) );
            DWORD nRegID = 0;

            if ( SUCCEEDED( pIDataObject->DAdvise( pFormatetc, advf, pOwnAdvise.get(), &nRegID ) ) && nRegID > 0 )
            {
                pOwnAdvise->SetRegID( nRegID );
                *pdwConnection = InsertAdviseLinkToList( pOwnAdvise, m_pDataAdvises );
                if ( *pdwConnection )
                    return S_OK;
                else
                    pIDataObject->DUnadvise( nRegID );
            }
        }
    }

    // return success always for now
    return S_OK;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::DUnadvise( DWORD dwConnection )
{
    if ( m_pDefHandler && DEFAULT_ARRAY_LEN > dwConnection && dwConnection > 0 && m_pDataAdvises[dwConnection] )
    {
        if ( CheckDefHandler() )
        {
            sal::systools::COMReference< IDataObject > pIDataObject(m_pDefHandler, sal::systools::COM_QUERY);

            ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
            if ( pIDataObject )
            {
                DWORD nID = m_pDataAdvises[dwConnection]->GetRegID();
                pIDataObject->DUnadvise( nID );
            }
        }

        m_pDataAdvises[dwConnection]->DisconnectOrigAdvise();
        m_pDataAdvises[dwConnection] = nullptr;

        return S_OK;
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::EnumDAdvise( IEnumSTATDATA ** ppenumAdvise )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IDataObject > pIDataObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIDataObject )
            return pIDataObject->EnumDAdvise( ppenumAdvise );
    }

    return E_FAIL;
}

// IRunnableObject

COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetRunningClass( LPCLSID lpClsid )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IRunnableObject > pIRunObj(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIRunObj )
            return pIRunObj->GetRunningClass( lpClsid );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::Run( LPBINDCTX pbc )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IRunnableObject > pIRunObj(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIRunObj )
            return pIRunObj->Run( pbc );
    }

    return E_FAIL;
}

BOOL STDMETHODCALLTYPE InprocEmbedDocument_Impl::IsRunning()
{
    if (CheckDefHandler())
    {
        sal::systools::COMReference< IRunnableObject > pIRunObj(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIRunObj )
            return pIRunObj->IsRunning();
    }

    return FALSE;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::LockRunning( BOOL fLock, BOOL fLastUnlockCloses )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IRunnableObject > pIRunObj(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIRunObj )
            return pIRunObj->LockRunning( fLock, fLastUnlockCloses );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::SetContainedObject( BOOL fContained)
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IRunnableObject > pIRunObj(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIRunObj )
            return pIRunObj->SetContainedObject( fContained );
    }

    return E_FAIL;
}


// IViewObject methods

COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::Draw( DWORD dwDrawAspect, LONG lindex, void *pvAspect, DVTARGETDEVICE *ptd, HDC hdcTargetDev, HDC hdcDraw, LPCRECTL lprcBounds, LPCRECTL lprcWBounds, BOOL ( STDMETHODCALLTYPE *pfnContinue )( ULONG_PTR dwContinue ), ULONG_PTR dwContinue )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IViewObject > pIViewObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIViewObject )
            return pIViewObject->Draw( dwDrawAspect, lindex, pvAspect, ptd, hdcTargetDev, hdcDraw, lprcBounds, lprcWBounds, pfnContinue, dwContinue );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetColorSet( DWORD dwDrawAspect, LONG lindex, void *pvAspect, DVTARGETDEVICE *ptd, HDC hicTargetDev, LOGPALETTE **ppColorSet )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IViewObject > pIViewObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIViewObject )
            return pIViewObject->GetColorSet( dwDrawAspect, lindex, pvAspect, ptd, hicTargetDev, ppColorSet );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::Freeze( DWORD dwDrawAspect, LONG lindex, void *pvAspect, DWORD *pdwFreeze )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IViewObject > pIViewObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIViewObject )
            return pIViewObject->Freeze( dwDrawAspect, lindex, pvAspect, pdwFreeze );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::Unfreeze( DWORD dwFreeze )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IViewObject > pIViewObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIViewObject )
            return pIViewObject->Unfreeze( dwFreeze );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::SetAdvise( DWORD aspects, DWORD advf, IAdviseSink *pAdvSink )
{

    // CheckDefHandler will set the listener, avoid reusing of old listener
    if ( m_pViewAdvise )
    {
        m_pViewAdvise->DisconnectOrigAdvise();
        m_pViewAdvise = nullptr;
    }

    if ( pAdvSink && CheckDefHandler() )
    {
        sal::systools::COMReference< IViewObject > pIViewObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIViewObject )
        {
            sal::systools::COMReference pOwnAdvise(new OleWrapperAdviseSink(pAdvSink, aspects, advf));

            if ( SUCCEEDED( pIViewObject->SetAdvise( aspects, advf, pOwnAdvise.get() ) ) )
            {
                m_pViewAdvise = pOwnAdvise;
                return S_OK;
            }
        }
    }

    return S_OK;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetAdvise( DWORD *pAspects, DWORD *pAdvf, IAdviseSink **ppAdvSink )
{
    if ( !ppAdvSink )
        return E_INVALIDARG;

    if ( m_pViewAdvise )
    {
        if ( pAspects )
            *pAspects = m_pViewAdvise->GetAspect();

        if ( pAdvf )
            *pAdvf = m_pViewAdvise->GetViewAdviseFlag();

        *ppAdvSink = m_pViewAdvise->GetOrigAdvise().get();
        if ( *ppAdvSink )
            (*ppAdvSink)->AddRef();
    }
    else
        *ppAdvSink = nullptr;

    return S_OK;
}

// IViewObject2 methods

COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetExtent( DWORD dwDrawAspect, LONG lindex, DVTARGETDEVICE *ptd, LPSIZEL lpsizel )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IViewObject2 > pIViewObject2(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIViewObject2 )
            return pIViewObject2->GetExtent( dwDrawAspect, lindex, ptd, lpsizel );
    }

    return E_FAIL;
}


// IOleWindow methods

COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetWindow( HWND *phwnd )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleWindow > pIOleWindow(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIOleWindow )
            return pIOleWindow->GetWindow( phwnd );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::ContextSensitiveHelp( BOOL fEnterMode )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleWindow > pIOleWindow(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIOleWindow )
            return pIOleWindow->ContextSensitiveHelp( fEnterMode );
    }

    return E_FAIL;
}


// IOleInPlaceObject methods

COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::InPlaceDeactivate()
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleInPlaceObject > pIOleInPlaceObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIOleInPlaceObject )
            return pIOleInPlaceObject->InPlaceDeactivate();
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::UIDeactivate()
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleInPlaceObject > pIOleInPlaceObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIOleInPlaceObject )
            return pIOleInPlaceObject->UIDeactivate();
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::SetObjectRects( LPCRECT lprcPosRect, LPCRECT lprcClipRect )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleInPlaceObject > pIOleInPlaceObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIOleInPlaceObject )
            return pIOleInPlaceObject->SetObjectRects( lprcPosRect, lprcClipRect );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::ReactivateAndUndo()
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IOleInPlaceObject > pIOleInPlaceObject(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIOleInPlaceObject )
            return pIOleInPlaceObject->ReactivateAndUndo();
    }

    return E_FAIL;
}


// IDispatch methods

COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetTypeInfoCount( UINT *pctinfo )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IDispatch > pIDispatch(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIDispatch )
            return pIDispatch->GetTypeInfoCount( pctinfo );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetTypeInfo( UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IDispatch > pIDispatch(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIDispatch )
            return pIDispatch->GetTypeInfo( iTInfo, lcid, ppTInfo );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::GetIDsOfNames( REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IDispatch > pIDispatch(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIDispatch )
            return pIDispatch->GetIDsOfNames( riid, rgszNames, cNames, lcid, rgDispId );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::Invoke( DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr )
{
    if ( CheckDefHandler() )
    {
        sal::systools::COMReference< IDispatch > pIDispatch(m_pDefHandler, sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_nCallsOnStack ); // avoid reentrance problem
        if ( pIDispatch )
            return pIDispatch->Invoke( dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
    }

    return E_FAIL;
}


// InternalCacheWrapper


// IUnknown

COM_DECLSPEC_NOTHROW STDMETHODIMP
InprocEmbedDocument_Impl::InternalCacheWrapper::QueryInterface(REFIID riid, void** ppv)
{
    return m_rOwnDocument.QueryInterface( riid, ppv );
}


COM_DECLSPEC_NOTHROW STDMETHODIMP_(ULONG) InprocEmbedDocument_Impl::InternalCacheWrapper::AddRef()
{
    return m_rOwnDocument.AddRef();
}


COM_DECLSPEC_NOTHROW STDMETHODIMP_(ULONG) InprocEmbedDocument_Impl::InternalCacheWrapper::Release()
{
    return m_rOwnDocument.Release();
}

// IOleCache methods

COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::InternalCacheWrapper::Cache( FORMATETC *pformatetc, DWORD advf, DWORD *pdwConnection )
{
    if ( m_rOwnDocument.CheckDefHandler() )
    {
        sal::systools::COMReference< IOleCache > pIOleCache(m_rOwnDocument.GetDefHandler(), sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_rOwnDocument.m_nCallsOnStack ); // avoid reentrance problem
        if ( pIOleCache )
            return pIOleCache->Cache( pformatetc, advf, pdwConnection );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::InternalCacheWrapper::Uncache( DWORD dwConnection )
{
    if ( m_rOwnDocument.CheckDefHandler() )
    {
        sal::systools::COMReference< IOleCache > pIOleCache(m_rOwnDocument.GetDefHandler(), sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_rOwnDocument.m_nCallsOnStack ); // avoid reentrance problem
        if ( pIOleCache )
            return pIOleCache->Uncache( dwConnection );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::InternalCacheWrapper::EnumCache( IEnumSTATDATA **ppenumSTATDATA )
{
    if ( m_rOwnDocument.CheckDefHandler() )
    {
        sal::systools::COMReference< IOleCache > pIOleCache(m_rOwnDocument.GetDefHandler(), sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_rOwnDocument.m_nCallsOnStack ); // avoid reentrance problem
        if ( pIOleCache )
            return pIOleCache->EnumCache( ppenumSTATDATA );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::InternalCacheWrapper::InitCache( IDataObject *pDataObject )
{
    if ( m_rOwnDocument.CheckDefHandler() )
    {
        sal::systools::COMReference< IOleCache > pIOleCache(m_rOwnDocument.GetDefHandler(), sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_rOwnDocument.m_nCallsOnStack ); // avoid reentrance problem
        if ( pIOleCache )
            return pIOleCache->InitCache( pDataObject );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::InternalCacheWrapper::SetData( FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease )
{
    if ( m_rOwnDocument.CheckDefHandler() )
    {
        sal::systools::COMReference< IOleCache > pIOleCache(m_rOwnDocument.GetDefHandler(), sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_rOwnDocument.m_nCallsOnStack ); // avoid reentrance problem
        if ( pIOleCache )
            return pIOleCache->SetData( pformatetc, pmedium, fRelease );
    }

    return E_FAIL;
}

// IOleCache2 methods

COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::InternalCacheWrapper::UpdateCache( LPDATAOBJECT pDataObject, DWORD grfUpdf, LPVOID pReserved )
{
    if ( m_rOwnDocument.CheckDefHandler() )
    {
        sal::systools::COMReference< IOleCache2 > pIOleCache2(m_rOwnDocument.GetDefHandler(), sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_rOwnDocument.m_nCallsOnStack ); // avoid reentrance problem
        if ( pIOleCache2 )
            return pIOleCache2->UpdateCache( pDataObject, grfUpdf, pReserved );
    }

    return E_FAIL;
}


COM_DECLSPEC_NOTHROW STDMETHODIMP InprocEmbedDocument_Impl::InternalCacheWrapper::DiscardCache( DWORD dwDiscardOptions )
{
    if ( m_rOwnDocument.CheckDefHandler() )
    {
        sal::systools::COMReference< IOleCache2 > pIOleCache2(m_rOwnDocument.GetDefHandler(), sal::systools::COM_QUERY);

        ULONGGuard aGuard( m_rOwnDocument.m_nCallsOnStack ); // avoid reentrance problem
        if ( pIOleCache2 )
            return pIOleCache2->DiscardCache( dwDiscardOptions );
    }

    return E_FAIL;
}

}; // namespace inprocserv

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
