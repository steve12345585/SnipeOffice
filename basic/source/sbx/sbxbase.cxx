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

#include <memory>
#include <tools/debug.hxx>
#include <tools/stream.hxx>
#include <vcl/svapp.hxx>

#include <basic/sbx.hxx>
#include <sbxfac.hxx>
#include <sbxform.hxx>
#include <basic/sbxmeth.hxx>
#include <sbxprop.hxx>
#include <sbxbase.hxx>

#include <rtl/ustring.hxx>
#include <sal/log.hxx>
#include <filefmt.hxx>

// AppData-Structure for SBX:


SbxAppData::SbxAppData()
    : eErrCode(ERRCODE_NONE)
    , aErrorMsg(OUString())
    , eBasicFormaterLangType(LANGUAGE_DONTKNOW)
{
}

SbxAppData::~SbxAppData()
{
    SolarMutexGuard g;

    pBasicFormater.reset();
    // basic manager repository must be destroyed before factories
    mrImplRepository.clear();
}

SbxBase::SbxBase()
{
    nFlags  = SbxFlagBits::ReadWrite;
}

SbxBase::SbxBase(const SbxBase& r) = default;

SbxBase::~SbxBase()
{
}

SbxBase& SbxBase::operator=( const SbxBase& r )
{
    nFlags = r.nFlags;
    return *this;
}

SbxDataType SbxBase::GetType() const
{
    return SbxEMPTY;
}

bool SbxBase::IsFixed() const
{
    return IsSet( SbxFlagBits::Fixed );
}

void SbxBase::SetModified( bool b )
{
    if( IsSet( SbxFlagBits::NoModify ) )
        return;
    if( b )
        SetFlag( SbxFlagBits::Modified );
    else
        ResetFlag( SbxFlagBits::Modified );
}

ErrCode const & SbxBase::GetError()
{
    return GetSbxData_Impl().eErrCode;
}

OUString const & SbxBase::GetErrorMsg()
{
    return GetSbxData_Impl().aErrorMsg;
}

void SbxBase::SetError(ErrCode e, const OUString& rMsg)
{
    SbxAppData& r = GetSbxData_Impl();
    if (e && r.eErrCode == ERRCODE_NONE)
    {
        r.eErrCode = e;
        r.aErrorMsg = rMsg;
    }
}

void SbxBase::SetError( ErrCode e )
{
    SbxAppData& r = GetSbxData_Impl();
    if( e && r.eErrCode == ERRCODE_NONE )
        r.eErrCode = e;
}

bool SbxBase::IsError()
{
    return GetSbxData_Impl().eErrCode != ERRCODE_NONE;
}

void SbxBase::ResetError()
{
    GetSbxData_Impl().eErrCode = ERRCODE_NONE;
    GetSbxData_Impl().aErrorMsg = OUString();
}

void SbxBase::AddFactory( SbxFactory* pFac )
{
    GetSbxData_Impl().m_Factories.emplace_back(pFac);
}

void SbxBase::RemoveFactory( SbxFactory const * pFac )
{
    if (!IsSbxData_Impl())
        return;
    SbxAppData& r = GetSbxData_Impl();
    auto it = std::find(r.m_Factories.begin(), r.m_Factories.end(), pFac);
    if (it != r.m_Factories.end())
        r.m_Factories.erase( it );
}


SbxBaseRef SbxBase::Create( sal_uInt16 nSbxId, sal_uInt32 nCreator )
{
    // #91626: Hack to skip old Basic dialogs
    // Problem: There does not exist a factory any more,
    // so we have to create a dummy SbxVariable instead
    if( nSbxId == 0x65 )    // Dialog Id
        return new SbxVariable;

    if( nCreator == SBXCR_SBX )
      switch( nSbxId )
    {
        case SBXID_VALUE:       return new SbxValue;
        case SBXID_VARIABLE:    return new SbxVariable;
        case SBXID_ARRAY:       return new SbxArray;
        case SBXID_DIMARRAY:    return new SbxDimArray;
        case SBXID_OBJECT:      return new SbxObject( u""_ustr );
        case SBXID_COLLECTION:  return new SbxCollection;
        case SBXID_FIXCOLLECTION:
                                return new SbxStdCollection;
        case SBXID_METHOD:      return new SbxMethod( u""_ustr, SbxEMPTY );
        case SBXID_PROPERTY:    return new SbxProperty( u""_ustr, SbxEMPTY );
    }
    // Unknown type: go over the factories!
    SbxAppData& r = GetSbxData_Impl();
    SbxBaseRef pNew;
    for (auto const& rpFac : r.m_Factories)
    {
        pNew = rpFac->Create( nSbxId, nCreator );
        if( pNew )
            break;
    }
    SAL_WARN_IF(!pNew, "basic", "No factory for SBX ID " << nSbxId);
    return pNew;
}

SbxObjectRef SbxBase::CreateObject( const OUString& rClass )
{
    SbxAppData& r = GetSbxData_Impl();
    SbxObjectRef pNew;
    for (auto const& rpFac : r.m_Factories)
    {
        pNew = rpFac->CreateObject( rClass );
        if( pNew )
            break;
    }
    SAL_WARN_IF(!pNew, "basic", "No factory for object class " << rClass);
    return pNew;
}

namespace {

// coverity[ -taint_source ]
[[nodiscard]] SbxFlagBits CorrectFlags(SbxFlagBits nFlags)
{
    // Correcting a foolishness of mine:
    if (nFlags & SbxFlagBits::Reserved)
        nFlags |= SbxFlagBits::GlobalSearch;
    return nFlags & ~SbxFlagBits::Reserved;
}

}

SbxBaseRef SbxBase::Load( SvStream& rStrm )
{
    sal_uInt16 nSbxId(0), nFlagsTmp(0), nVer(0);
    sal_uInt32 nCreator(0), nSize(0);
    rStrm.ReadUInt32( nCreator ).ReadUInt16( nSbxId ).ReadUInt16( nFlagsTmp ).ReadUInt16( nVer );
    SbxFlagBits nFlags = CorrectFlags(static_cast<SbxFlagBits>(nFlagsTmp));

    sal_uInt64 nOldPos = rStrm.Tell();
    rStrm.ReadUInt32( nSize );
    SbxBaseRef p = Create( nSbxId, nCreator );
    if( p )
    {
        p->nFlags = nFlags;
        if( p->LoadData( rStrm, nVer ) )
        {
            sal_uInt64 const nNewPos = rStrm.Tell();
            nOldPos += nSize;
            DBG_ASSERT( nOldPos >= nNewPos, "SBX: Too much data loaded" );
            if( nOldPos != nNewPos )
                rStrm.Seek( nOldPos );
            if( !p->LoadCompleted() )
            {
                // Deleting of the object
                SbxBaseRef xDeleteRef( p );
                p = nullptr;
            }
        }
        else
        {
            rStrm.SetError( SVSTREAM_FILEFORMAT_ERROR );
            // Deleting of the object
            SbxBaseRef xDeleteRef( p );
            p = nullptr;
        }
    }
    else
        rStrm.SetError( SVSTREAM_FILEFORMAT_ERROR );
    return p;
}

std::pair<bool, sal_uInt32> SbxBase::Store( SvStream& rStrm )
{
    if( ( nFlags & SbxFlagBits::DontStore ) == SbxFlagBits::NONE )
    {
        rStrm.WriteUInt32( SBXCR_SBX )
             .WriteUInt16( GetSbxId() )
             .WriteUInt16( static_cast<sal_uInt16>(GetFlags()) )
             .WriteUInt16( GetVersion() );
        sal_uInt64 const nOldPos = rStrm.Tell();
        rStrm.WriteUInt32( 0 );
        auto [bRes, nVersion] = StoreData(rStrm);
        sal_uInt64 const nNewPos = rStrm.Tell();
        rStrm.Seek( nOldPos );
        rStrm.WriteUInt32( nNewPos - nOldPos );
        rStrm.Seek( nNewPos );
        if( rStrm.GetError() != ERRCODE_NONE )
            bRes = false;
        if( bRes )
            bRes = true;
        return { bRes, nVersion };
    }
    else
        return { true, B_IMG_VERSION_12 };
}

bool SbxBase::LoadCompleted()
{
    return true;
}

//////////////////////////////// SbxFactory

SbxFactory::~SbxFactory()
{
}

SbxBaseRef SbxFactory::Create( sal_uInt16, sal_uInt32 )
{
    return nullptr;
}

SbxObjectRef SbxFactory::CreateObject( const OUString& )
{
    return nullptr;
}

///////////////////////////////// SbxInfo

SbxInfo::~SbxInfo()
{}

void SbxInfo::AddParam(const OUString& rName, SbxDataType eType, SbxFlagBits nFlags)
{
    m_Params.push_back(std::make_unique<SbxParamInfo>(rName, eType, nFlags));
}

const SbxParamInfo* SbxInfo::GetParam( sal_uInt16 n ) const
{
    if (n < 1 || n > m_Params.size())
        return nullptr;
    else
        return m_Params[n - 1].get();
}

void SbxInfo::LoadData( SvStream& rStrm, sal_uInt16 nVer )
{
    m_Params.clear();
    sal_uInt16 nParam;
    aComment = read_uInt16_lenPrefixed_uInt8s_ToOUString(rStrm,
        RTL_TEXTENCODING_ASCII_US);
    aHelpFile = read_uInt16_lenPrefixed_uInt8s_ToOUString(rStrm,
        RTL_TEXTENCODING_ASCII_US);
    rStrm.ReadUInt32( nHelpId ).ReadUInt16( nParam );
    while( nParam-- )
    {
        sal_uInt16 nType(0), nFlagsTmp(0);
        sal_uInt32 nUserData = 0;
        OUString aName = read_uInt16_lenPrefixed_uInt8s_ToOUString(rStrm,
            RTL_TEXTENCODING_ASCII_US);
        rStrm.ReadUInt16( nType ).ReadUInt16( nFlagsTmp );
        SbxFlagBits nFlags = static_cast<SbxFlagBits>(nFlagsTmp);
        if( nVer > 1 )
            rStrm.ReadUInt32( nUserData );
        AddParam( aName, static_cast<SbxDataType>(nType), nFlags );
        SbxParamInfo& p(*m_Params.back());
        p.nUserData = nUserData;
    }
}

void SbxInfo::StoreData( SvStream& rStrm ) const
{
    write_uInt16_lenPrefixed_uInt8s_FromOUString(rStrm, aComment,
        RTL_TEXTENCODING_ASCII_US );
    write_uInt16_lenPrefixed_uInt8s_FromOUString(rStrm, aHelpFile,
        RTL_TEXTENCODING_ASCII_US);
    rStrm.WriteUInt32( nHelpId ).WriteUInt16( m_Params.size() );
    for (auto const& i : m_Params)
    {
        write_uInt16_lenPrefixed_uInt8s_FromOUString(rStrm, i->aName,
            RTL_TEXTENCODING_ASCII_US);
        rStrm.WriteUInt16( i->eType )
             .WriteUInt16( static_cast<sal_uInt16>(i->nFlags) )
             .WriteUInt32( i->nUserData );
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
