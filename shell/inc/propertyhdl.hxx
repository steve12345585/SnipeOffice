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

#ifndef INCLUDED_SHELL_INC_INTERNAL_PROPERTYHDL_HXX
#define INCLUDED_SHELL_INC_INTERNAL_PROPERTYHDL_HXX

#include <shlobj.h>

// {AE424E85-F6DF-4910-A6A9-438797986431}
const CLSID CLSID_PROPERTY_HANDLER =
{ 0xae424e85, 0xf6df, 0x4910, { 0xa6, 0xa9, 0x43, 0x87, 0x97, 0x98, 0x64, 0x31 } };

class CMetaInfoReader;

class CPropertyHdl : public IPropertyStore,
                     public IPropertyStoreCapabilities,
                     public IInitializeWithStream
{
public:
             CPropertyHdl(LONG RefCnt = 1);
    virtual ~CPropertyHdl();


    // IUnknown methods

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID riid,
            void __RPC_FAR *__RPC_FAR *ppvObject ) override;
    virtual ULONG STDMETHODCALLTYPE AddRef() override;
    virtual ULONG STDMETHODCALLTYPE Release() override;


    // IPropertyStore

    virtual HRESULT STDMETHODCALLTYPE GetCount( DWORD *pcProps ) override;
    virtual HRESULT STDMETHODCALLTYPE GetAt( DWORD iProp, PROPERTYKEY *pkey ) override;
    virtual HRESULT STDMETHODCALLTYPE GetValue( REFPROPERTYKEY key, PROPVARIANT *pPropVar ) override;
    virtual HRESULT STDMETHODCALLTYPE SetValue( REFPROPERTYKEY key, REFPROPVARIANT propVar ) override;
    virtual HRESULT STDMETHODCALLTYPE Commit() override;


    // IPropertyStoreCapabilities

    virtual HRESULT STDMETHODCALLTYPE IsPropertyWritable( REFPROPERTYKEY key ) override;


    // IInitializeWithStream

    virtual HRESULT STDMETHODCALLTYPE Initialize(IStream *pStream, DWORD grfMode) override;

private:
    void        LoadProperties( CMetaInfoReader *pMetaInfoReader );

private:
    LONG m_RefCnt;
    IPropertyStoreCache* m_pCache;
};

class CClassFactory : public IClassFactory
{
public:
    CClassFactory( const CLSID& clsid );
    virtual ~CClassFactory();


    // IUnknown methods

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID riid,
            void __RPC_FAR *__RPC_FAR *ppvObject) override;
    virtual ULONG STDMETHODCALLTYPE AddRef() override;
    virtual ULONG STDMETHODCALLTYPE Release() override;


    // IClassFactory methods

    virtual HRESULT STDMETHODCALLTYPE CreateInstance(
            IUnknown __RPC_FAR *pUnkOuter,
            REFIID riid,
            void __RPC_FAR *__RPC_FAR *ppvObject) override;

    virtual HRESULT STDMETHODCALLTYPE LockServer( BOOL fLock ) override;
    static bool IsLocked();

private:
    LONG  m_RefCnt;
    CLSID m_Clsid;
    static LONG  s_ServerLocks;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
