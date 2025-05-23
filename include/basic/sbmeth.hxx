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

#pragma once

#include <comphelper/errcode.hxx>
#include <basic/sbxmeth.hxx>
#include <basic/sbdef.hxx>
#include <basic/basicdllapi.h>

class SbModule;

class BASIC_DLLPUBLIC SbMethod : public SbxMethod
{
    friend class SbiRuntime;
    friend class SbiFactory;
    friend class SbModule;
    friend class SbClassModuleObject;
    friend class SbiCodeGen;
    friend class SbJScriptMethod;
    friend class SbIfaceMapperMethod;

    SbxVariable*  mCaller;                   // caller
    SbModule*     pMod;
    BasicDebugFlags nDebugFlags;
    sal_uInt16    nLine1, nLine2;
    sal_uInt32    nStart;
    bool          bInvalid;
    SbxArrayRef   refStatics;
    BASIC_DLLPRIVATE SbMethod( const OUString&, SbxDataType, SbModule* );
    BASIC_DLLPRIVATE SbMethod( const SbMethod& );
    virtual bool LoadData( SvStream&, sal_uInt16 ) override;
    virtual std::pair<bool, sal_uInt32> StoreData( SvStream& ) const override;
    virtual ~SbMethod() override;

public:
    SBX_DECL_PERSIST_NODATA(SBXID_BASICMETHOD,2);
    virtual SbxInfo* GetInfo() override;
    SbxArray*  GetStatics();
    void       ClearStatics();
    SbModule*  GetModule()                         { return pMod;        }
    BasicDebugFlags GetDebugFlags() const          { return nDebugFlags; }
    void       SetDebugFlags( BasicDebugFlags n )  { nDebugFlags = n;    }
    void       GetLineRange( sal_uInt16&, sal_uInt16& );

    // Interface to execute a method from the applications
    ErrCode         Call( SbxValue* pRet,  SbxVariable* pCaller = nullptr );
    virtual void    Broadcast( SfxHintId nHintId ) override;
};

typedef tools::SvRef<SbMethod> SbMethodRef;

class SbIfaceMapperMethod final : public SbMethod
{
    friend class SbiRuntime;

    SbMethodRef mxImplMeth;

public:
    SbIfaceMapperMethod( const OUString& rName, SbMethod* pImplMeth )
        : SbMethod( rName, pImplMeth->GetType(), nullptr )
        , mxImplMeth( pImplMeth )
    {}
    virtual ~SbIfaceMapperMethod() override;
    SbMethod* getImplMethod()
        { return mxImplMeth.get(); }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
