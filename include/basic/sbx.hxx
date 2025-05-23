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

#include <config_options.h>
#include <tools/ref.hxx>
#include <svl/hint.hxx>

#include <basic/sbxdef.hxx>
#include <basic/sbxobj.hxx>
#include <basic/basicdllapi.h>

#include <utility>
#include <vector>
#include <memory>

class SvStream;

// Parameter information
struct SbxParamInfo
{
    const OUString aName;          // Name of the parameter
    SbxDataType    eType;          // Data type
    SbxFlagBits    nFlags;         // Flag-Bits
    sal_uInt32     nUserData;      // IDs etc.
    SbxParamInfo( OUString s, SbxDataType t, SbxFlagBits n )
        : aName(std::move( s )), eType( t ), nFlags( n ), nUserData( 0 ) {}
};

typedef std::vector<std::unique_ptr<SbxParamInfo>> SbxParams;

class UNLESS_MERGELIBS(BASIC_DLLPUBLIC) SbxInfo final : public SvRefBase
{
    friend class SbxVariable;
    friend class SbMethod;

    OUString        aComment;
    OUString        aHelpFile;
    sal_uInt32      nHelpId;
    SbxParams       m_Params;

    SbxInfo(SbxInfo const&) = delete;
    void operator=(SbxInfo const&) = delete;

    void LoadData( SvStream&, sal_uInt16 );
    void StoreData( SvStream& ) const;
    virtual ~SbxInfo() override;
public:
    SbxInfo();
    SbxInfo( OUString , sal_uInt32 );

    void                AddParam( const OUString&, SbxDataType, SbxFlagBits=SbxFlagBits::Read );
    const SbxParamInfo* GetParam( sal_uInt16 n ) const; // index starts with 1!
    const OUString&     GetComment() const              { return aComment; }
    const OUString&     GetHelpFile() const             { return aHelpFile; }
    sal_uInt32          GetHelpId() const               { return nHelpId;   }

    void                SetComment( const OUString& r )   { aComment = r; }
};

class BASIC_DLLPUBLIC SbxHint final : public SfxHint
{
    SbxVariable* pVar;
public:
    SbxHint( SfxHintId n, SbxVariable* v ) : SfxHint( n ), pVar( v ) {}
    ~SbxHint() override;
    SbxVariable* GetVar() const { return pVar; }
};

// SbxArray is an unidimensional, dynamic Array
// The variables convert from SbxVariablen. Put()/Insert() into the
// declared datatype, if they are not SbxVARIANT.

struct SbxVarEntry;

class BASIC_DLLPUBLIC SbxArray : public SbxBase
{
// #100883 Method to set method directly to parameter array
    friend class SbMethod;
    friend class SbClassModuleObject;
    friend SbxObjectRef cloneTypeObjectImpl( const SbxObject& rTypeObj );
    BASIC_DLLPRIVATE void PutDirect( SbxVariable* pVar, sal_uInt32 nIdx );

    std::vector<SbxVarEntry> mVarEntries;          // The variables
    SbxDataType eType;            // Data type of the array

protected:
    virtual ~SbxArray() override;
    virtual bool LoadData( SvStream&, sal_uInt16 ) override;
    virtual std::pair<bool, sal_uInt32> StoreData( SvStream& ) const override;

public:
    SBX_DECL_PERSIST_NODATA(SBXID_ARRAY,1);
    SbxArray( SbxDataType=SbxVARIANT );
    SbxArray( const SbxArray& ) = delete;
    SbxArray& operator=( const SbxArray& );
    virtual void Clear() override;
    sal_uInt32 Count() const;
    virtual SbxDataType  GetType() const override;
    SbxVariableRef& GetRef(sal_uInt32);
    SbxVariable* Get(sal_uInt32);
    void Put(SbxVariable*, sal_uInt32);
    void Insert(SbxVariable*, sal_uInt32);
    void                 Remove( sal_uInt32 );
    void                 Remove( SbxVariable const * );
    void                 Merge( SbxArray* );
    const OUString & GetAlias(sal_uInt32);
    void PutAlias(const OUString&, sal_uInt32);
    SbxVariable* Find( const OUString&, SbxClassType );
};

// SbxDimArray is an array that can dimensioned using BASIC conventions.
struct SbxDim {                 // an array-dimension:
    sal_Int32 nLbound, nUbound; // Limitations
    sal_Int32 nSize;            // Number of elements
};

class BASIC_DLLPUBLIC SbxDimArray final : public SbxArray
{
    std::vector<SbxDim> m_vDimensions;     // Dimension table
    BASIC_DLLPRIVATE void AddDimImpl(sal_Int32, sal_Int32, bool bAllowSize0);
    bool mbHasFixedSize;

    sal_uInt32 Offset(const sal_Int32*);
    sal_uInt32 Offset(SbxArray*);
    virtual bool LoadData( SvStream&, sal_uInt16 ) override;
    virtual std::pair<bool, sal_uInt32> StoreData( SvStream& ) const override;
    virtual ~SbxDimArray() override;
public:
    SBX_DECL_PERSIST_NODATA(SBXID_DIMARRAY,1);
    SbxDimArray( SbxDataType=SbxVARIANT );
    SbxDimArray( const SbxDimArray& ) = delete;
    SbxDimArray& operator=( const SbxDimArray& );
    virtual void Clear() override;
    SbxVariable* Get( SbxArray* );

    using SbxArray::GetRef;
    using SbxArray::Get;
    SbxVariable* Get(const sal_Int32*);
    using SbxArray::Put;
    void Put(SbxVariable*, const sal_Int32*);
    sal_Int32 GetDims() const { return m_vDimensions.size(); }
    void AddDim(sal_Int32, sal_Int32);
    void unoAddDim(sal_Int32, sal_Int32);
    bool GetDim(sal_Int32, sal_Int32&, sal_Int32&) const;
    bool hasFixedSize() const { return mbHasFixedSize; };
    void setHasFixedSize( bool bHasFixedSize ) {mbHasFixedSize = bHasFixedSize; };
};

class SbxCollection : public SbxObject
{
    void Initialize();
protected:
    virtual ~SbxCollection() override;
    virtual bool LoadData( SvStream&, sal_uInt16 ) override;
    virtual void Notify( SfxBroadcaster& rBC, const SfxHint& rHint ) override;
    // Overridable methods (why not pure virtual?):
    virtual void CollAdd( SbxArray* pPar );
    void CollItem( SbxArray* pPar );
    virtual void CollRemove( SbxArray* pPar );

public:
    SBX_DECL_PERSIST_NODATA(SBXID_COLLECTION,1);
    SbxCollection();
    SbxCollection( const SbxCollection& );
    SbxCollection& operator=( const SbxCollection& );
    virtual SbxVariable* Find( const OUString&, SbxClassType ) override;
    virtual void Clear() override;
};

class SbxStdCollection final : public SbxCollection
{
    OUString aElemClass;
    bool bAddRemoveOk;
    virtual ~SbxStdCollection() override;
    virtual bool LoadData( SvStream&, sal_uInt16 ) override;
    virtual std::pair<bool, sal_uInt32> StoreData( SvStream& ) const override;
    virtual void CollAdd( SbxArray* pPar ) override;
    virtual void CollRemove( SbxArray* pPar ) override;
public:
    SBX_DECL_PERSIST_NODATA(SBXID_FIXCOLLECTION,1);
    SbxStdCollection();
    SbxStdCollection( const SbxStdCollection& );
    SbxStdCollection& operator=( const SbxStdCollection& );
    virtual void Insert( SbxVariable* ) override;
};

typedef tools::SvRef<SbxArray> SbxArrayRef;
typedef tools::SvRef<SbxInfo> SbxInfoRef;
typedef tools::SvRef<SbxDimArray> SbxDimArrayRef;

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
