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

#include <xetable.hxx>

#include <map>
#include <numeric>
#include <com/sun/star/i18n/ScriptType.hpp>
#include <scitems.hxx>
#include <svl/intitem.hxx>
#include <svl/numformat.hxx>
#include <svl/stritem.hxx>
#include <tools/UnitConversion.hxx>
#include <editeng/flditem.hxx>
#include <document.hxx>
#include <dociter.hxx>
#include <olinetab.hxx>
#include <formulacell.hxx>
#include <patattr.hxx>
#include <attrib.hxx>
#include <xehelper.hxx>
#include <xecontent.hxx>
#include <xeescher.hxx>
#include <xeextlst.hxx>
#include <xeformula.hxx>
#include <xlcontent.hxx>
#include <xltools.hxx>
#include <tokenarray.hxx>
#include <formula/errorcodes.hxx>
#include <comphelper/threadpool.hxx>
#include <oox/token/tokens.hxx>
#include <oox/export/utils.hxx>

using namespace ::oox;

namespace ApiScriptType = ::com::sun::star::i18n::ScriptType;

// Helper records for cell records

XclExpStringRec::XclExpStringRec( const XclExpRoot& rRoot, const OUString& rResult ) :
    XclExpRecord( EXC_ID3_STRING ),
    mxResult( XclExpStringHelper::CreateString( rRoot, rResult ) )
{
    OSL_ENSURE( (rRoot.GetBiff() <= EXC_BIFF5) || (mxResult->Len() > 0),
        "XclExpStringRec::XclExpStringRec - empty result not allowed in BIFF8+" );
    SetRecSize( mxResult->GetSize() );
}

void XclExpStringRec::WriteBody( XclExpStream& rStrm )
{
    rStrm << *mxResult;
}

// Additional records for special formula ranges ==============================

XclExpRangeFmlaBase::XclExpRangeFmlaBase(
        sal_uInt16 nRecId, sal_uInt32 nRecSize, const ScAddress& rScPos ) :
    XclExpRecord( nRecId, nRecSize ),
    maXclRange( ScAddress::UNINITIALIZED ),
    maBaseXclPos( ScAddress::UNINITIALIZED )
{
    maBaseXclPos.Set( static_cast< sal_uInt16 >( rScPos.Col() ), static_cast< sal_uInt16 >( rScPos.Row() ) );
    maXclRange.maFirst = maXclRange.maLast = maBaseXclPos;
}

XclExpRangeFmlaBase::XclExpRangeFmlaBase(
        sal_uInt16 nRecId, sal_uInt32 nRecSize, const ScRange& rScRange ) :
    XclExpRecord( nRecId, nRecSize ),
    maXclRange( ScAddress::UNINITIALIZED ),
    maBaseXclPos( ScAddress::UNINITIALIZED )
{
    maXclRange.Set(
        static_cast< sal_uInt16 >( rScRange.aStart.Col() ),
        static_cast< sal_uInt16 >( rScRange.aStart.Row() ),
        static_cast< sal_uInt16 >( rScRange.aEnd.Col() ),
        static_cast< sal_uInt16 >( rScRange.aEnd.Row() ) );
    maBaseXclPos = maXclRange.maFirst;
}

bool XclExpRangeFmlaBase::IsBasePos( sal_uInt16 nXclCol, sal_uInt32 nXclRow ) const
{
    return (maBaseXclPos.mnCol == nXclCol) && (maBaseXclPos.mnRow == nXclRow);
}

void XclExpRangeFmlaBase::Extend( const ScAddress& rScPos )
{
    sal_uInt16 nXclCol = static_cast< sal_uInt16 >( rScPos.Col() );
    sal_uInt32 nXclRow = static_cast< sal_uInt32 >( rScPos.Row() );
    maXclRange.maFirst.mnCol = ::std::min( maXclRange.maFirst.mnCol, nXclCol );
    maXclRange.maFirst.mnRow = ::std::min( maXclRange.maFirst.mnRow, nXclRow );
    maXclRange.maLast.mnCol  = ::std::max( maXclRange.maLast.mnCol,  nXclCol );
    maXclRange.maLast.mnRow  = ::std::max( maXclRange.maLast.mnRow,  nXclRow );
}

void XclExpRangeFmlaBase::WriteRangeAddress( XclExpStream& rStrm ) const
{
    maXclRange.Write( rStrm, false );
}

// Array formulas =============================================================

XclExpArray::XclExpArray( const XclTokenArrayRef& xTokArr, const ScRange& rScRange ) :
    XclExpRangeFmlaBase( EXC_ID3_ARRAY, 14 + xTokArr->GetSize(), rScRange ),
    mxTokArr( xTokArr )
{
}

XclTokenArrayRef XclExpArray::CreateCellTokenArray( const XclExpRoot& rRoot ) const
{
    return rRoot.GetFormulaCompiler().CreateSpecialRefFormula( EXC_TOKID_EXP, maBaseXclPos );
}

bool XclExpArray::IsVolatile() const
{
    return mxTokArr->IsVolatile();
}

void XclExpArray::WriteBody( XclExpStream& rStrm )
{
    WriteRangeAddress( rStrm );
    sal_uInt16 nFlags = EXC_ARRAY_DEFAULTFLAGS;
    ::set_flag( nFlags, EXC_ARRAY_RECALC_ALWAYS, IsVolatile() );
    rStrm << nFlags << sal_uInt32( 0 ) << *mxTokArr;
}

XclExpArrayBuffer::XclExpArrayBuffer( const XclExpRoot& rRoot ) :
    XclExpRoot( rRoot )
{
}

XclExpArrayRef XclExpArrayBuffer::CreateArray( const ScTokenArray& rScTokArr, const ScRange& rScRange )
{
    const ScAddress& rScPos = rScRange.aStart;
    XclTokenArrayRef xTokArr = GetFormulaCompiler().CreateFormula( EXC_FMLATYPE_MATRIX, rScTokArr, &rScPos );

    OSL_ENSURE( maRecMap.find( rScPos ) == maRecMap.end(), "XclExpArrayBuffer::CreateArray - array exists already" );
    XclExpArrayRef& rxRec = maRecMap[ rScPos ];
    rxRec = new XclExpArray( xTokArr, rScRange );
    return rxRec;
}

XclExpArrayRef XclExpArrayBuffer::FindArray( const ScTokenArray& rScTokArr, const ScAddress& rBasePos ) const
{
    XclExpArrayRef xRec;
    // try to extract a matrix reference token
    if (rScTokArr.GetLen() != 1)
        // Must consist of a single reference token.
        return xRec;

    const formula::FormulaToken* pToken = rScTokArr.GetArray()[0];
    if (!pToken || pToken->GetOpCode() != ocMatRef)
        // not a matrix reference token.
        return xRec;

    const ScSingleRefData& rRef = *pToken->GetSingleRef();
    ScAddress aAbsPos = rRef.toAbs(GetRoot().GetDoc(), rBasePos);
    XclExpArrayMap::const_iterator it = maRecMap.find(aAbsPos);

    if (it != maRecMap.end())
        xRec = it->second;
    return xRec;
}

// Shared formulas ============================================================

XclExpShrfmla::XclExpShrfmla( const XclTokenArrayRef& xTokArr, const ScAddress& rScPos ) :
    XclExpRangeFmlaBase( EXC_ID_SHRFMLA, 10 + xTokArr->GetSize(), rScPos ),
    mxTokArr( xTokArr ),
    mnUsedCount( 1 )
{
}

void XclExpShrfmla::ExtendRange( const ScAddress& rScPos )
{
    Extend( rScPos );
    ++mnUsedCount;
}

XclTokenArrayRef XclExpShrfmla::CreateCellTokenArray( const XclExpRoot& rRoot ) const
{
    return rRoot.GetFormulaCompiler().CreateSpecialRefFormula( EXC_TOKID_EXP, maBaseXclPos );
}

bool XclExpShrfmla::IsVolatile() const
{
    return mxTokArr->IsVolatile();
}

void XclExpShrfmla::WriteBody( XclExpStream& rStrm )
{
    WriteRangeAddress( rStrm );
    rStrm << sal_uInt8( 0 ) << mnUsedCount << *mxTokArr;
}

XclExpShrfmlaBuffer::XclExpShrfmlaBuffer( const XclExpRoot& rRoot ) :
    XclExpRoot( rRoot )
{
}

bool XclExpShrfmlaBuffer::IsValidTokenArray( const ScTokenArray& rArray ) const
{
    using namespace formula;

    FormulaToken** pTokens = rArray.GetArray();
    sal_uInt16 nLen = rArray.GetLen();
    for (sal_uInt16 i = 0; i < nLen; ++i)
    {
        const FormulaToken* p = pTokens[i];
        switch (p->GetType())
        {
            case svSingleRef:
            {
                const ScSingleRefData& rRefData = *p->GetSingleRef();
                if (!GetFormulaCompiler().IsRef2D(rRefData))
                    // Excel's shared formula cannot include 3D reference.
                    return false;
            }
            break;
            case svDoubleRef:
            {
                const ScComplexRefData& rRefData = *p->GetDoubleRef();
                if (!GetFormulaCompiler().IsRef2D(rRefData))
                    // Excel's shared formula cannot include 3D reference.
                    return false;
            }
            break;
            case svExternalSingleRef:
            case svExternalDoubleRef:
            case svExternalName:
                // External references aren't allowed.
                return false;
            default:
                ;
        }
    }
    return true;
}

XclExpShrfmlaRef XclExpShrfmlaBuffer::CreateOrExtendShrfmla(
    const ScFormulaCell& rScCell, const ScAddress& rScPos )
{
    XclExpShrfmlaRef xRec;
    const ScTokenArray* pShrdScTokArr = rScCell.GetSharedCode();
    if (!pShrdScTokArr)
        // This formula cell is not shared formula cell.
        return xRec;

    // Check to see if this shared formula contains any tokens that Excel's shared formula cannot handle.
    if (maBadTokens.count(pShrdScTokArr) > 0)
        // Already on the black list. Skip it.
        return xRec;

    if (!IsValidTokenArray(*pShrdScTokArr))
    {
        // We can't export this as shared formula.
        maBadTokens.insert(pShrdScTokArr);
        return xRec;
    }

    TokensType::iterator aIt = maRecMap.find(pShrdScTokArr);
    if( aIt == maRecMap.end() )
    {
        // create a new record
        XclTokenArrayRef xTokArr = GetFormulaCompiler().CreateFormula( EXC_FMLATYPE_SHARED, *pShrdScTokArr, &rScPos );
        xRec = new XclExpShrfmla( xTokArr, rScPos );
        maRecMap[ pShrdScTokArr ] = xRec;
    }
    else
    {
        // extend existing record
        OSL_ENSURE( aIt->second, "XclExpShrfmlaBuffer::CreateOrExtendShrfmla - missing record" );
        xRec = aIt->second;
        xRec->ExtendRange( rScPos );
    }

    return xRec;
}

// Multiple operations ========================================================

XclExpTableop::XclExpTableop( const ScAddress& rScPos,
        const XclMultipleOpRefs& rRefs, sal_uInt8 nScMode ) :
    XclExpRangeFmlaBase( EXC_ID3_TABLEOP, 16, rScPos ),
    mnLastAppXclCol( static_cast< sal_uInt16 >( rScPos.Col() ) ),
    mnColInpXclCol( static_cast< sal_uInt16 >( rRefs.maColFirstScPos.Col() ) ),
    mnColInpXclRow( static_cast< sal_uInt16 >( rRefs.maColFirstScPos.Row() ) ),
    mnRowInpXclCol( static_cast< sal_uInt16 >( rRefs.maRowFirstScPos.Col() ) ),
    mnRowInpXclRow( static_cast< sal_uInt16 >( rRefs.maRowFirstScPos.Row() ) ),
    mnScMode( nScMode ),
    mbValid( false )
{
}

bool XclExpTableop::TryExtend( const ScAddress& rScPos, const XclMultipleOpRefs& rRefs )
{
    sal_uInt16 nXclCol = static_cast< sal_uInt16 >( rScPos.Col() );
    sal_uInt16 nXclRow = static_cast< sal_uInt16 >( rScPos.Row() );

    bool bOk = IsAppendable( nXclCol, nXclRow );
    if( bOk )
    {
        SCCOL nFirstScCol  = static_cast< SCCOL >( maXclRange.maFirst.mnCol );
        SCROW nFirstScRow  = static_cast< SCROW >( maXclRange.maFirst.mnRow );
        SCCOL nColInpScCol = static_cast< SCCOL >( mnColInpXclCol );
        SCROW nColInpScRow = static_cast< SCROW >( mnColInpXclRow );
        SCCOL nRowInpScCol = static_cast< SCCOL >( mnRowInpXclCol );
        SCROW nRowInpScRow = static_cast< SCROW >( mnRowInpXclRow );

        bOk =   ((mnScMode == 2) == rRefs.mbDblRefMode) &&
                (rScPos.Tab() == rRefs.maFmlaScPos.Tab()) &&
                (nColInpScCol == rRefs.maColFirstScPos.Col()) &&
                (nColInpScRow == rRefs.maColFirstScPos.Row()) &&
                (rScPos.Tab() == rRefs.maColFirstScPos.Tab()) &&
                (rScPos.Tab() == rRefs.maColRelScPos.Tab());

        if( bOk ) switch( mnScMode )
        {
            case 0:
                bOk =   (rScPos.Col() == rRefs.maFmlaScPos.Col()) &&
                        (nFirstScRow  == rRefs.maFmlaScPos.Row() + 1) &&
                        (nFirstScCol  == rRefs.maColRelScPos.Col() + 1) &&
                        (rScPos.Row() == rRefs.maColRelScPos.Row());
            break;
            case 1:
                bOk =   (nFirstScCol  == rRefs.maFmlaScPos.Col() + 1) &&
                        (rScPos.Row() == rRefs.maFmlaScPos.Row()) &&
                        (rScPos.Col() == rRefs.maColRelScPos.Col()) &&
                        (nFirstScRow  == rRefs.maColRelScPos.Row() + 1);
            break;
            case 2:
                bOk =   (nFirstScCol  == rRefs.maFmlaScPos.Col() + 1) &&
                        (nFirstScRow  == rRefs.maFmlaScPos.Row() + 1) &&
                        (nFirstScCol  == rRefs.maColRelScPos.Col() + 1) &&
                        (rScPos.Row() == rRefs.maColRelScPos.Row()) &&
                        (nRowInpScCol == rRefs.maRowFirstScPos.Col()) &&
                        (nRowInpScRow == rRefs.maRowFirstScPos.Row()) &&
                        (rScPos.Tab() == rRefs.maRowFirstScPos.Tab()) &&
                        (rScPos.Col() == rRefs.maRowRelScPos.Col()) &&
                        (nFirstScRow  == rRefs.maRowRelScPos.Row() + 1) &&
                        (rScPos.Tab() == rRefs.maRowRelScPos.Tab());
            break;
            default:
                bOk = false;
        }

        if( bOk )
        {
            // extend the cell range
            OSL_ENSURE( IsAppendable( nXclCol, nXclRow ), "XclExpTableop::TryExtend - wrong cell address" );
            Extend( rScPos );
            mnLastAppXclCol = nXclCol;
        }
    }

    return bOk;
}

void XclExpTableop::Finalize()
{
    // is the range complete? (last appended cell is in last column)
    mbValid = maXclRange.maLast.mnCol == mnLastAppXclCol;
    // if last row is incomplete, try to shorten the used range
    if( !mbValid && (maXclRange.maFirst.mnRow < maXclRange.maLast.mnRow) )
    {
        --maXclRange.maLast.mnRow;
        mbValid = true;
    }

    // check if referred cells are outside of own range
    if( !mbValid )
        return;

    switch( mnScMode )
    {
    case 0:
        mbValid =   (mnColInpXclCol + 1 < maXclRange.maFirst.mnCol) || (mnColInpXclCol > maXclRange.maLast.mnCol) ||
                    (mnColInpXclRow     < maXclRange.maFirst.mnRow) || (mnColInpXclRow > maXclRange.maLast.mnRow);
    break;
    case 1:
        mbValid =   (mnColInpXclCol     < maXclRange.maFirst.mnCol) || (mnColInpXclCol > maXclRange.maLast.mnCol) ||
                    (mnColInpXclRow + 1 < maXclRange.maFirst.mnRow) || (mnColInpXclRow > maXclRange.maLast.mnRow);
    break;
    case 2:
        mbValid =   ((mnColInpXclCol + 1 < maXclRange.maFirst.mnCol) || (mnColInpXclCol > maXclRange.maLast.mnCol) ||
                     (mnColInpXclRow + 1 < maXclRange.maFirst.mnRow) || (mnColInpXclRow > maXclRange.maLast.mnRow)) &&
                    ((mnRowInpXclCol + 1 < maXclRange.maFirst.mnCol) || (mnRowInpXclCol > maXclRange.maLast.mnCol) ||
                     (mnRowInpXclRow + 1 < maXclRange.maFirst.mnRow) || (mnRowInpXclRow > maXclRange.maLast.mnRow));
    break;
    }
}

XclTokenArrayRef XclExpTableop::CreateCellTokenArray( const XclExpRoot& rRoot ) const
{
    XclExpFormulaCompiler& rFmlaComp = rRoot.GetFormulaCompiler();
    return mbValid ?
        rFmlaComp.CreateSpecialRefFormula( EXC_TOKID_TBL, maBaseXclPos ) :
        rFmlaComp.CreateErrorFormula( EXC_ERR_NA );
}

bool XclExpTableop::IsVolatile() const
{
    return true;
}

void XclExpTableop::Save( XclExpStream& rStrm )
{
    if( mbValid )
        XclExpRangeFmlaBase::Save( rStrm );
}

bool XclExpTableop::IsAppendable( sal_uInt16 nXclCol, sal_uInt16 nXclRow ) const
{
    return  ((nXclCol == mnLastAppXclCol + 1) && (nXclRow == maXclRange.maFirst.mnRow)) ||
            ((nXclCol == mnLastAppXclCol + 1) && (nXclCol <= maXclRange.maLast.mnCol) && (nXclRow == maXclRange.maLast.mnRow)) ||
            ((mnLastAppXclCol == maXclRange.maLast.mnCol) && (nXclCol == maXclRange.maFirst.mnCol) && (nXclRow == maXclRange.maLast.mnRow + 1));
}

void XclExpTableop::WriteBody( XclExpStream& rStrm )
{
    sal_uInt16 nFlags = EXC_TABLEOP_DEFAULTFLAGS;
    ::set_flag( nFlags, EXC_TABLEOP_RECALC_ALWAYS, IsVolatile() );
    switch( mnScMode )
    {
        case 1: ::set_flag( nFlags, EXC_TABLEOP_ROW );  break;
        case 2: ::set_flag( nFlags, EXC_TABLEOP_BOTH ); break;
    }

    WriteRangeAddress( rStrm );
    rStrm << nFlags;
    if( mnScMode == 2 )
        rStrm << mnRowInpXclRow << mnRowInpXclCol << mnColInpXclRow << mnColInpXclCol;
    else
        rStrm << mnColInpXclRow << mnColInpXclCol << sal_uInt32( 0 );
}

XclExpTableopBuffer::XclExpTableopBuffer( const XclExpRoot& rRoot ) :
    XclExpRoot( rRoot )
{
}

XclExpTableopRef XclExpTableopBuffer::CreateOrExtendTableop(
        const ScTokenArray& rScTokArr, const ScAddress& rScPos )
{
    XclExpTableopRef xRec;

    // try to extract cell references of a multiple operations formula
    XclMultipleOpRefs aRefs;
    if (XclTokenArrayHelper::GetMultipleOpRefs(GetDoc(), aRefs, rScTokArr, rScPos))
    {
        // try to find an existing TABLEOP record for this cell position
        for( size_t nPos = 0, nSize = maTableopList.GetSize(); !xRec && (nPos < nSize); ++nPos )
        {
            XclExpTableop* xTempRec = maTableopList.GetRecord( nPos );
            if( xTempRec->TryExtend( rScPos, aRefs ) )
                xRec = xTempRec;
        }

        // no record found, or found record not extensible
        if( !xRec )
            xRec = TryCreate( rScPos, aRefs );
    }

    return xRec;
}

void XclExpTableopBuffer::Finalize()
{
    for( size_t nPos = 0, nSize = maTableopList.GetSize(); nPos < nSize; ++nPos )
        maTableopList.GetRecord( nPos )->Finalize();
}

XclExpTableopRef XclExpTableopBuffer::TryCreate( const ScAddress& rScPos, const XclMultipleOpRefs& rRefs )
{
    sal_uInt8 nScMode = 0;
    bool bOk =  (rScPos.Tab() == rRefs.maFmlaScPos.Tab()) &&
                (rScPos.Tab() == rRefs.maColFirstScPos.Tab()) &&
                (rScPos.Tab() == rRefs.maColRelScPos.Tab());

    if( bOk )
    {
        if( rRefs.mbDblRefMode )
        {
            nScMode = 2;
            bOk =   (rScPos.Col() == rRefs.maFmlaScPos.Col() + 1) &&
                    (rScPos.Row() == rRefs.maFmlaScPos.Row() + 1) &&
                    (rScPos.Col() == rRefs.maColRelScPos.Col() + 1) &&
                    (rScPos.Row() == rRefs.maColRelScPos.Row()) &&
                    (rScPos.Tab() == rRefs.maRowFirstScPos.Tab()) &&
                    (rScPos.Col() == rRefs.maRowRelScPos.Col()) &&
                    (rScPos.Row() == rRefs.maRowRelScPos.Row() + 1) &&
                    (rScPos.Tab() == rRefs.maRowRelScPos.Tab());
        }
        else if( (rScPos.Col() == rRefs.maFmlaScPos.Col()) &&
                 (rScPos.Row() == rRefs.maFmlaScPos.Row() + 1) &&
                 (rScPos.Col() == rRefs.maColRelScPos.Col() + 1) &&
                 (rScPos.Row() == rRefs.maColRelScPos.Row()) )
        {
            nScMode = 0;
        }
        else if( (rScPos.Col() == rRefs.maFmlaScPos.Col() + 1) &&
                 (rScPos.Row() == rRefs.maFmlaScPos.Row()) &&
                 (rScPos.Col() == rRefs.maColRelScPos.Col()) &&
                 (rScPos.Row() == rRefs.maColRelScPos.Row() + 1) )
        {
            nScMode = 1;
        }
        else
        {
            bOk = false;
        }
    }

    XclExpTableopRef xRec;
    if( bOk )
    {
        xRec = new XclExpTableop( rScPos, rRefs, nScMode );
        maTableopList.AppendRecord( xRec );
    }

    return xRec;
}

// Cell records

XclExpCellBase::XclExpCellBase(
        sal_uInt16 nRecId, std::size_t nContSize, const XclAddress& rXclPos ) :
    XclExpRecord( nRecId, nContSize + 4 ),
    maXclPos( rXclPos )
{
}

bool XclExpCellBase::IsMultiLineText() const
{
    return false;
}

bool XclExpCellBase::TryMerge( const XclExpCellBase& /*rCell*/ )
{
    return false;
}

void XclExpCellBase::GetBlankXFIndexes( ScfUInt16Vec& /*rXFIndexes*/ ) const
{
    // default: do nothing
}

void XclExpCellBase::RemoveUnusedBlankCells( const ScfUInt16Vec& /*rXFIndexes*/, size_t /*nStartAllNotFound*/ )
{
    // default: do nothing
}

// Single cell records ========================================================

XclExpSingleCellBase::XclExpSingleCellBase(
        sal_uInt16 nRecId, std::size_t nContSize, const XclAddress& rXclPos, sal_uInt32 nXFId ) :
    XclExpCellBase( nRecId, 2, rXclPos ),
    maXFId( nXFId ),
    mnContSize( nContSize )
{
}

XclExpSingleCellBase::XclExpSingleCellBase( const XclExpRoot& rRoot,
        sal_uInt16 nRecId, std::size_t nContSize, const XclAddress& rXclPos,
        const ScPatternAttr* pPattern, sal_Int16 nScript, sal_uInt32 nForcedXFId ) :
    XclExpCellBase( nRecId, 2, rXclPos ),
    maXFId( nForcedXFId ),
    mnContSize( nContSize )
{
    if( GetXFId() == EXC_XFID_NOTFOUND )
        SetXFId( rRoot.GetXFBuffer().Insert( pPattern, nScript ) );
}

sal_uInt16 XclExpSingleCellBase::GetLastXclCol() const
{
    return GetXclCol();
}

sal_uInt32 XclExpSingleCellBase::GetFirstXFId() const
{
    return GetXFId();
}

bool XclExpSingleCellBase::IsEmpty() const
{
    return false;
}

void XclExpSingleCellBase::ConvertXFIndexes( const XclExpRoot& rRoot )
{
    maXFId.ConvertXFIndex( rRoot );
}

void XclExpSingleCellBase::Save( XclExpStream& rStrm )
{
    OSL_ENSURE_BIFF( rStrm.GetRoot().GetBiff() >= EXC_BIFF3 );
    AddRecSize( mnContSize );
    XclExpCellBase::Save( rStrm );
}

void XclExpSingleCellBase::WriteBody( XclExpStream& rStrm )
{
    rStrm << static_cast<sal_uInt16> (GetXclRow()) << GetXclCol() << maXFId.mnXFIndex;
    WriteContents( rStrm );
}

XclExpNumberCell::XclExpNumberCell(
        const XclExpRoot& rRoot, const XclAddress& rXclPos,
        const ScPatternAttr* pPattern, sal_uInt32 nForcedXFId, double fValue ) :
    // #i41210# always use latin script for number cells - may look wrong for special number formats...
    XclExpSingleCellBase( rRoot, EXC_ID3_NUMBER, 8, rXclPos, pPattern, ApiScriptType::LATIN, nForcedXFId ),
    mfValue( fValue )
{
}

static OString lcl_GetStyleId( const XclExpXmlStream& rStrm, sal_uInt32 nXFIndex )
{
    return OString::number( rStrm.GetRoot().GetXFBuffer()
            .GetXmlCellIndex( nXFIndex ) );
}

static OString lcl_GetStyleId( const XclExpXmlStream& rStrm, const XclExpCellBase& rCell )
{
    sal_uInt32 nXFId    = rCell.GetFirstXFId();
    sal_uInt16 nXFIndex = rStrm.GetRoot().GetXFBuffer().GetXFIndex( nXFId );
    return lcl_GetStyleId( rStrm, nXFIndex );
}

void XclExpNumberCell::SaveXml( XclExpXmlStream& rStrm )
{
    sax_fastparser::FSHelperPtr& rWorksheet = rStrm.GetCurrentStream();
    rWorksheet->startElement( XML_c,
            XML_r, XclXmlUtils::ToOString(rStrm.GetRoot().GetStringBuf(), GetXclPos()).getStr(),
            XML_s, lcl_GetStyleId(rStrm, *this),
            XML_t, "n"
            // OOXTODO: XML_cm, XML_vm, XML_ph
    );
    rWorksheet->startElement(XML_v);
    rWorksheet->write( mfValue );
    rWorksheet->endElement( XML_v );
    rWorksheet->endElement( XML_c );
}

void XclExpNumberCell::WriteContents( XclExpStream& rStrm )
{
    rStrm << mfValue;
}

XclExpBooleanCell::XclExpBooleanCell(
        const XclExpRoot& rRoot, const XclAddress& rXclPos,
        const ScPatternAttr* pPattern, sal_uInt32 nForcedXFId, bool bValue ) :
    // #i41210# always use latin script for boolean cells
    XclExpSingleCellBase( rRoot, EXC_ID3_BOOLERR, 2, rXclPos, pPattern, ApiScriptType::LATIN, nForcedXFId ),
    mbValue( bValue )
{
}

void XclExpBooleanCell::SaveXml( XclExpXmlStream& rStrm )
{
    sax_fastparser::FSHelperPtr& rWorksheet = rStrm.GetCurrentStream();
    rWorksheet->startElement( XML_c,
            XML_r, XclXmlUtils::ToOString(rStrm.GetRoot().GetStringBuf(), GetXclPos()).getStr(),
            XML_s, lcl_GetStyleId(rStrm, *this),
            XML_t, "b"
            // OOXTODO: XML_cm, XML_vm, XML_ph
    );
    rWorksheet->startElement( XML_v );
    rWorksheet->write( mbValue ? "1" : "0" );
    rWorksheet->endElement( XML_v );
    rWorksheet->endElement( XML_c );
}

void XclExpBooleanCell::WriteContents( XclExpStream& rStrm )
{
    rStrm << sal_uInt16( mbValue ? 1 : 0 ) << EXC_BOOLERR_BOOL;
}

XclExpLabelCell::XclExpLabelCell(
        const XclExpRoot& rRoot, const XclAddress& rXclPos,
        const ScPatternAttr* pPattern, sal_uInt32 nForcedXFId, const OUString& rStr ) :
    XclExpSingleCellBase( EXC_ID3_LABEL, 0, rXclPos, nForcedXFId )
{
    sal_uInt16 nMaxLen = (rRoot.GetBiff() == EXC_BIFF8) ? EXC_STR_MAXLEN : EXC_LABEL_MAXLEN;
    XclExpStringRef xText = XclExpStringHelper::CreateCellString(
        rRoot, rStr, pPattern, XclStrFlags::NONE, nMaxLen);
    Init( rRoot, pPattern, xText );
}

XclExpLabelCell::XclExpLabelCell(
        const XclExpRoot& rRoot, const XclAddress& rXclPos,
        const ScPatternAttr* pPattern, sal_uInt32 nForcedXFId,
        const EditTextObject* pEditText, XclExpHyperlinkHelper& rLinkHelper ) :
    XclExpSingleCellBase( EXC_ID3_LABEL, 0, rXclPos, nForcedXFId )
{
    sal_uInt16 nMaxLen = (rRoot.GetBiff() == EXC_BIFF8) ? EXC_STR_MAXLEN : EXC_LABEL_MAXLEN;

    XclExpStringRef xText;
    if (pEditText)
        xText = XclExpStringHelper::CreateCellString(
            rRoot, *pEditText, pPattern, rLinkHelper, XclStrFlags::NONE, nMaxLen);
    else
        xText = XclExpStringHelper::CreateCellString(
            rRoot, OUString(), pPattern, XclStrFlags::NONE, nMaxLen);

    Init( rRoot, pPattern, xText );
}

bool XclExpLabelCell::IsMultiLineText() const
{
    return mbLineBreak || mxText->HasNewline();
}

void XclExpLabelCell::Init( const XclExpRoot& rRoot,
        const ScPatternAttr* pPattern, XclExpStringRef const & xText )
{
    OSL_ENSURE( xText && xText->Len(), "XclExpLabelCell::XclExpLabelCell - empty string passed" );
    mxText = xText;
    mnSstIndex = 0;

    const XclFormatRunVec& rFormats = mxText->GetFormats();
    // remove formatting of the leading run if the entire string
    // is equally formatted
    sal_uInt16 nXclFont = EXC_FONT_NOTFOUND;
    if( rFormats.size() == 1 )
        nXclFont = mxText->RemoveLeadingFont();
    else
        nXclFont = mxText->GetLeadingFont();

    // create cell format
    if( GetXFId() == EXC_XFID_NOTFOUND )
    {
        OSL_ENSURE(nXclFont != EXC_FONT_NOTFOUND, "XclExpLabelCell::Init - leading font not found");

        // Buggy Excel behaviour - newlines are ignored unless wrap-text is enabled,
        // so always force text-wrapping (unless it was imported that way and not modified).
        bool bForceLineBreak = mxText->HasNewline() && !mxText->IsSingleLineForMultipleParagraphs();
        SetXFId(rRoot.GetXFBuffer().InsertWithFont(
            pPattern, ApiScriptType::WEAK, nXclFont, bForceLineBreak));
    }

    // get auto-wrap attribute from cell format
    const XclExpXF* pXF = rRoot.GetXFBuffer().GetXFById( GetXFId() );
    mbLineBreak = pXF && pXF->GetAlignmentData().mbLineBreak;

    // initialize the record contents
    switch( rRoot.GetBiff() )
    {
        case EXC_BIFF5:
            // BIFF5-BIFF7: create a LABEL or RSTRING record
            OSL_ENSURE( mxText->Len() <= EXC_LABEL_MAXLEN, "XclExpLabelCell::XclExpLabelCell - string too long" );
            SetContSize( mxText->GetSize() );
            // formatted string is exported in an RSTRING record
            if( mxText->IsRich() )
            {
                OSL_ENSURE( mxText->GetFormatsCount() <= EXC_LABEL_MAXLEN, "XclExpLabelCell::WriteContents - too many formats" );
                mxText->LimitFormatCount( EXC_LABEL_MAXLEN );
                SetRecId( EXC_ID_RSTRING );
                SetContSize( GetContSize() + 1 + 2 * mxText->GetFormatsCount() );
            }
        break;
        case EXC_BIFF8:
            // BIFF8+: create a LABELSST record
            mnSstIndex = rRoot.GetSst().Insert( xText );
            SetRecId( EXC_ID_LABELSST );
            SetContSize( 4 );
        break;
        default:    DBG_ERROR_BIFF();
    }
}

void XclExpLabelCell::SaveXml( XclExpXmlStream& rStrm )
{
    sax_fastparser::FSHelperPtr& rWorksheet = rStrm.GetCurrentStream();
    rWorksheet->startElement( XML_c,
            XML_r, XclXmlUtils::ToOString(rStrm.GetRoot().GetStringBuf(), GetXclPos()).getStr(),
            XML_s, lcl_GetStyleId(rStrm, *this),
            XML_t, "s"
            // OOXTODO: XML_cm, XML_vm, XML_ph
    );
    rWorksheet->startElement( XML_v );
    rWorksheet->write( static_cast<sal_Int32>(mnSstIndex) );
    rWorksheet->endElement( XML_v );
    rWorksheet->endElement( XML_c );
}

void XclExpLabelCell::WriteContents( XclExpStream& rStrm )
{
    switch( rStrm.GetRoot().GetBiff() )
    {
        case EXC_BIFF5:
            rStrm << *mxText;
            if( mxText->IsRich() )
            {
                rStrm << static_cast< sal_uInt8 >( mxText->GetFormatsCount() );
                mxText->WriteFormats( rStrm );
            }
        break;
        case EXC_BIFF8:
            rStrm << mnSstIndex;
        break;
        default:    DBG_ERROR_BIFF();
    }
}

XclExpFormulaCell::XclExpFormulaCell(
        const XclExpRoot& rRoot, const XclAddress& rXclPos,
        const ScPatternAttr* pPattern, sal_uInt32 nForcedXFId,
        const ScFormulaCell& rScFmlaCell,
        XclExpArrayBuffer& rArrayBfr,
        XclExpShrfmlaBuffer& rShrfmlaBfr,
        XclExpTableopBuffer& rTableopBfr ) :
    XclExpSingleCellBase( EXC_ID2_FORMULA, 0, rXclPos, nForcedXFId ),
    mrScFmlaCell( const_cast< ScFormulaCell& >( rScFmlaCell ) )
{
    // *** Find result number format overwriting cell number format *** -------

    if( GetXFId() == EXC_XFID_NOTFOUND )
    {
        SvNumberFormatter& rFormatter = rRoot.GetFormatter();
        XclExpNumFmtBuffer& rNumFmtBfr = rRoot.GetNumFmtBuffer();

        // current cell number format
        sal_uInt32 nScNumFmt = pPattern ?
            pPattern->GetItem( ATTR_VALUE_FORMAT ).GetValue() :
            rNumFmtBfr.GetStandardFormat();

        // alternative number format passed to XF buffer
        sal_uInt32 nAltScNumFmt = NUMBERFORMAT_ENTRY_NOT_FOUND;
        /*  Xcl doesn't know Boolean number formats, we write
            "TRUE";"FALSE" (language dependent). Don't do it for automatic
            formula formats, because Excel gets them right. */
        /*  #i8640# Don't set text format, if we have string results. */
        SvNumFormatType nFormatType = mrScFmlaCell.GetFormatType();
        if( ((nScNumFmt % SV_COUNTRY_LANGUAGE_OFFSET) == 0) &&
                (nFormatType != SvNumFormatType::LOGICAL) &&
                (nFormatType != SvNumFormatType::TEXT) )
            nAltScNumFmt = nScNumFmt;
        /*  If cell number format is Boolean and automatic formula
            format is Boolean don't write that ugly special format. */
        else if( (nFormatType == SvNumFormatType::LOGICAL) &&
                (rFormatter.GetType( nScNumFmt ) == SvNumFormatType::LOGICAL) )
            nAltScNumFmt = rNumFmtBfr.GetStandardFormat();

        // #i41420# find script type according to result type (always latin for numeric results)
        sal_Int16 nScript = ApiScriptType::LATIN;
        bool bForceLineBreak = false;
        if( nFormatType == SvNumFormatType::TEXT )
        {
            OUString aResult = mrScFmlaCell.GetString().getString();
            bForceLineBreak = mrScFmlaCell.IsMultilineResult();
            nScript = XclExpStringHelper::GetLeadingScriptType( rRoot, aResult );
        }
        SetXFId( rRoot.GetXFBuffer().InsertWithNumFmt( pPattern, nScript, nAltScNumFmt, bForceLineBreak ) );
    }

    // *** Convert the formula token array *** --------------------------------

    ScAddress aScPos( static_cast< SCCOL >( rXclPos.mnCol ), static_cast< SCROW >( rXclPos.mnRow ), rRoot.GetCurrScTab() );
    const ScTokenArray& rScTokArr = *mrScFmlaCell.GetCode();

    // first try to create multiple operations
    mxAddRec = rTableopBfr.CreateOrExtendTableop( rScTokArr, aScPos );

    // no multiple operation found - try to create matrix formula
    if( !mxAddRec )
        switch( mrScFmlaCell.GetMatrixFlag() )
        {
            case ScMatrixMode::Formula:
            {
                // origin of the matrix - find the used matrix range
                SCCOL nMatWidth;
                SCROW nMatHeight;
                mrScFmlaCell.GetMatColsRows( nMatWidth, nMatHeight );
                OSL_ENSURE( nMatWidth && nMatHeight, "XclExpFormulaCell::XclExpFormulaCell - empty matrix" );
                ScRange aMatScRange( aScPos );
                ScAddress& rMatEnd = aMatScRange.aEnd;
                rMatEnd.IncCol( static_cast< SCCOL >( nMatWidth - 1 ) );
                rMatEnd.IncRow( static_cast< SCROW >( nMatHeight - 1 ) );
                // reduce to valid range (range keeps valid, because start position IS valid)
                rRoot.GetAddressConverter().ValidateRange( aMatScRange, true );
                // create the ARRAY record
                mxAddRec = rArrayBfr.CreateArray( rScTokArr, aMatScRange );
            }
            break;
            case ScMatrixMode::Reference:
            {
                // other formula cell covered by a matrix - find the ARRAY record
                mxAddRec = rArrayBfr.FindArray(rScTokArr, aScPos);
                // should always be found, if Calc document is not broken
                OSL_ENSURE( mxAddRec, "XclExpFormulaCell::XclExpFormulaCell - no matrix found" );
            }
            break;
            default:;
        }

    // no matrix found - try to create shared formula
    if( !mxAddRec )
        mxAddRec = rShrfmlaBfr.CreateOrExtendShrfmla(mrScFmlaCell, aScPos);

    // no shared formula found - create a simple cell formula
    if( !mxAddRec )
        mxTokArr = rRoot.GetFormulaCompiler().CreateFormula( EXC_FMLATYPE_CELL, rScTokArr, &aScPos );
}

void XclExpFormulaCell::Save( XclExpStream& rStrm )
{
    // create token array for FORMULA cells with additional record
    if( mxAddRec )
        mxTokArr = mxAddRec->CreateCellTokenArray( rStrm.GetRoot() );

    // FORMULA record itself
    OSL_ENSURE( mxTokArr, "XclExpFormulaCell::Save - missing token array" );
    if( !mxTokArr )
        mxTokArr = rStrm.GetRoot().GetFormulaCompiler().CreateErrorFormula( EXC_ERR_NA );
    SetContSize( 16 + mxTokArr->GetSize() );
    XclExpSingleCellBase::Save( rStrm );

    // additional record (ARRAY, SHRFMLA, or TABLEOP), only for first FORMULA record
    if( mxAddRec && mxAddRec->IsBasePos( GetXclCol(), GetXclRow() ) )
        mxAddRec->Save( rStrm );

    // STRING record for string result
    if( mxStringRec )
        mxStringRec->Save( rStrm );
}

void XclExpFormulaCell::SaveXml( XclExpXmlStream& rStrm )
{
    const char* sType = nullptr;
    OUString    sValue;
    XclXmlUtils::GetFormulaTypeAndValue( mrScFmlaCell, sType, sValue );
    sax_fastparser::FSHelperPtr& rWorksheet = rStrm.GetCurrentStream();
    rWorksheet->startElement( XML_c,
            XML_r, XclXmlUtils::ToOString(rStrm.GetRoot().GetStringBuf(), GetXclPos()).getStr(),
            XML_s, lcl_GetStyleId(rStrm, *this),
            XML_t, sType
            // OOXTODO: XML_cm, XML_vm, XML_ph
    );

    bool bWriteFormula = true;
    bool bTagStarted = false;
    ScAddress aScPos( static_cast< SCCOL >( GetXclPos().mnCol ),
            static_cast< SCROW >( GetXclPos().mnRow ), rStrm.GetRoot().GetCurrScTab() );

    switch (mrScFmlaCell.GetMatrixFlag())
    {
        case ScMatrixMode::NONE:
            break;
        case ScMatrixMode::Reference:
            bWriteFormula = false;
            break;
        case ScMatrixMode::Formula:
            {
                // origin of the matrix - find the used matrix range
                SCCOL nMatWidth;
                SCROW nMatHeight;
                mrScFmlaCell.GetMatColsRows( nMatWidth, nMatHeight );
                OSL_ENSURE( nMatWidth && nMatHeight, "XclExpFormulaCell::XclExpFormulaCell - empty matrix" );
                ScRange aMatScRange( aScPos );
                ScAddress& rMatEnd = aMatScRange.aEnd;
                rMatEnd.IncCol( static_cast< SCCOL >( nMatWidth - 1 ) );
                rMatEnd.IncRow( static_cast< SCROW >( nMatHeight - 1 ) );
                // reduce to valid range (range keeps valid, because start position IS valid
                rStrm.GetRoot().GetAddressConverter().ValidateRange( aMatScRange, true );

                OStringBuffer sFmlaCellRange;
                if (rStrm.GetRoot().GetDoc().ValidRange(aMatScRange))
                {
                    // calculate the cell range.
                    sFmlaCellRange.append( XclXmlUtils::ToOString(
                                rStrm.GetRoot().GetStringBuf(), aMatScRange.aStart )
                                + OString::Concat(":"));
                    sFmlaCellRange.append( XclXmlUtils::ToOString(
                                    rStrm.GetRoot().GetStringBuf(), aMatScRange.aEnd ));
                }

                if (    aMatScRange.aStart.Col() == GetXclPos().mnCol &&
                        aMatScRange.aStart.Row() == static_cast<SCROW>(GetXclPos().mnRow))
                {
                    rWorksheet->startElement( XML_f,
                            XML_aca, ToPsz( (mxTokArr && mxTokArr->IsVolatile()) ||
                                (mxAddRec && mxAddRec->IsVolatile())),
                            XML_t, mxAddRec ? "array" : nullptr,
                            XML_ref, !sFmlaCellRange.isEmpty()? sFmlaCellRange.getStr() : nullptr
                            // OOXTODO: XML_dt2D,   bool
                            // OOXTODO: XML_dtr,    bool
                            // OOXTODO: XML_del1,   bool
                            // OOXTODO: XML_del2,   bool
                            // OOXTODO: XML_r1,     ST_CellRef
                            // OOXTODO: XML_r2,     ST_CellRef
                            // OOXTODO: XML_ca,     bool
                            // OOXTODO: XML_si,     uint
                            // OOXTODO: XML_bx      bool
                    );
                    bTagStarted = true;
                }
            }
            break;
    }

    if (bWriteFormula)
    {
        if (!bTagStarted)
        {
            rWorksheet->startElement( XML_f,
                    XML_aca, ToPsz( (mxTokArr && mxTokArr->IsVolatile()) ||
                        (mxAddRec && mxAddRec->IsVolatile()) ) );
        }
        rWorksheet->writeEscaped( XclXmlUtils::ToOUString(
                    rStrm.GetRoot().GetCompileFormulaContext(), mrScFmlaCell.aPos, mrScFmlaCell.GetCode(),
                    mrScFmlaCell.GetErrCode()));
        rWorksheet->endElement( XML_f );
    }

    if( strcmp( sType, "inlineStr" ) == 0 )
    {
        rWorksheet->startElement(XML_is);
        rWorksheet->startElement(XML_t);
        rWorksheet->writeEscaped( sValue );
        rWorksheet->endElement( XML_t );
        rWorksheet->endElement( XML_is );
    }
    else
    {
        rWorksheet->startElement(XML_v);
        rWorksheet->writeEscaped( sValue );
        rWorksheet->endElement( XML_v );
    }
    rWorksheet->endElement( XML_c );
}

void XclExpFormulaCell::WriteContents( XclExpStream& rStrm )
{
    FormulaError nScErrCode = mrScFmlaCell.GetErrCode();
    if( nScErrCode != FormulaError::NONE )
    {
        rStrm << EXC_FORMULA_RES_ERROR << sal_uInt8( 0 )
            << XclTools::GetXclErrorCode( nScErrCode )
            << sal_uInt8( 0 ) << sal_uInt16( 0 )
            << sal_uInt16( 0xFFFF );
    }
    else
    {
        // result of the formula
        switch( mrScFmlaCell.GetFormatType() )
        {
            case SvNumFormatType::NUMBER:
                {
                    // either value or error code
                    rStrm << mrScFmlaCell.GetValue();
                }
                break;

            case SvNumFormatType::TEXT:
                {
                    OUString aResult = mrScFmlaCell.GetString().getString();
                    if( !aResult.isEmpty() || (rStrm.GetRoot().GetBiff() <= EXC_BIFF5) )
                    {
                        rStrm << EXC_FORMULA_RES_STRING;
                        mxStringRec = new XclExpStringRec( rStrm.GetRoot(), aResult );
                    }
                    else
                        rStrm << EXC_FORMULA_RES_EMPTY;     // BIFF8 only
                    rStrm << sal_uInt8( 0 ) << sal_uInt32( 0 ) << sal_uInt16( 0xFFFF );
                }
                break;

            case SvNumFormatType::LOGICAL:
                {
                    sal_uInt8 nXclValue = (mrScFmlaCell.GetValue() == 0.0) ? 0 : 1;
                    rStrm << EXC_FORMULA_RES_BOOL << sal_uInt8( 0 )
                        << nXclValue << sal_uInt8( 0 ) << sal_uInt16( 0 )
                        << sal_uInt16( 0xFFFF );
                }
                break;

            default:
                rStrm << mrScFmlaCell.GetValue();
        }
    }

    // flags and formula token array
    sal_uInt16 nFlags = EXC_FORMULA_DEFAULTFLAGS;
    ::set_flag( nFlags, EXC_FORMULA_RECALC_ALWAYS, mxTokArr->IsVolatile() || (mxAddRec && mxAddRec->IsVolatile()) );
    ::set_flag( nFlags, EXC_FORMULA_SHARED, mxAddRec && (mxAddRec->GetRecId() == EXC_ID_SHRFMLA) );
    rStrm << nFlags << sal_uInt32( 0 ) << *mxTokArr;
}

// Multiple cell records ======================================================

XclExpMultiCellBase::XclExpMultiCellBase(
        sal_uInt16 nRecId, sal_uInt16 nMulRecId, std::size_t nContSize, const XclAddress& rXclPos ) :
    XclExpCellBase( nRecId, 0, rXclPos ),
    mnMulRecId( nMulRecId ),
    mnContSize( nContSize )
{
}

sal_uInt16 XclExpMultiCellBase::GetLastXclCol() const
{
    return GetXclCol() + GetCellCount() - 1;
}

sal_uInt32 XclExpMultiCellBase::GetFirstXFId() const
{
    return maXFIds.empty() ? XclExpXFBuffer::GetDefCellXFId() : maXFIds.front().mnXFId;
}

bool XclExpMultiCellBase::IsEmpty() const
{
    return maXFIds.empty();
}

void XclExpMultiCellBase::ConvertXFIndexes( const XclExpRoot& rRoot )
{
    for( auto& rXFId : maXFIds )
        rXFId.ConvertXFIndex( rRoot );
}

void XclExpMultiCellBase::Save( XclExpStream& rStrm )
{
    OSL_ENSURE_BIFF( rStrm.GetRoot().GetBiff() >= EXC_BIFF3 );

    XclExpMultiXFIdDeq::const_iterator aEnd = maXFIds.end();
    XclExpMultiXFIdDeq::const_iterator aRangeBeg = maXFIds.begin();
    XclExpMultiXFIdDeq::const_iterator aRangeEnd = aRangeBeg;
    sal_uInt16 nBegXclCol = GetXclCol();
    sal_uInt16 nEndXclCol = nBegXclCol;

    while( aRangeEnd != aEnd )
    {
        // find begin of next used XF range
        aRangeBeg = aRangeEnd;
        nBegXclCol = nEndXclCol;
        while( (aRangeBeg != aEnd) && (aRangeBeg->mnXFIndex == EXC_XF_NOTFOUND) )
        {
            nBegXclCol = nBegXclCol + aRangeBeg->mnCount;
            ++aRangeBeg;
        }
        // find end of next used XF range
        aRangeEnd = aRangeBeg;
        nEndXclCol = nBegXclCol;
        while( (aRangeEnd != aEnd) && (aRangeEnd->mnXFIndex != EXC_XF_NOTFOUND) )
        {
            nEndXclCol = nEndXclCol + aRangeEnd->mnCount;
            ++aRangeEnd;
        }

        // export this range as a record
        if( aRangeBeg != aRangeEnd )
        {
            sal_uInt16 nCount = nEndXclCol - nBegXclCol;
            bool bIsMulti = nCount > 1;
            std::size_t nTotalSize = GetRecSize() + (2 + mnContSize) * nCount;
            if( bIsMulti ) nTotalSize += 2;

            rStrm.StartRecord( bIsMulti ? mnMulRecId : GetRecId(), nTotalSize );
            rStrm << static_cast<sal_uInt16> (GetXclRow()) << nBegXclCol;

            sal_uInt16 nRelCol = nBegXclCol - GetXclCol();
            for( XclExpMultiXFIdDeq::const_iterator aIt = aRangeBeg; aIt != aRangeEnd; ++aIt )
            {
                for( sal_uInt16 nIdx = 0; nIdx < aIt->mnCount; ++nIdx )
                {
                    rStrm << aIt->mnXFIndex;
                    WriteContents( rStrm, nRelCol );
                    ++nRelCol;
                }
            }
            if( bIsMulti )
                rStrm << static_cast< sal_uInt16 >( nEndXclCol - 1 );
            rStrm.EndRecord();
        }
    }
}

void XclExpMultiCellBase::SaveXml( XclExpXmlStream& rStrm )
{
    XclExpMultiXFIdDeq::const_iterator aEnd = maXFIds.end();
    XclExpMultiXFIdDeq::const_iterator aRangeBeg = maXFIds.begin();
    XclExpMultiXFIdDeq::const_iterator aRangeEnd = aRangeBeg;
    sal_uInt16 nBegXclCol = GetXclCol();
    sal_uInt16 nEndXclCol = nBegXclCol;

    while( aRangeEnd != aEnd )
    {
        // find begin of next used XF range
        aRangeBeg = aRangeEnd;
        nBegXclCol = nEndXclCol;
        while( (aRangeBeg != aEnd) && (aRangeBeg->mnXFIndex == EXC_XF_NOTFOUND) )
        {
            nBegXclCol = nBegXclCol + aRangeBeg->mnCount;
            ++aRangeBeg;
        }
        // find end of next used XF range
        aRangeEnd = aRangeBeg;
        nEndXclCol = nBegXclCol;
        while( (aRangeEnd != aEnd) && (aRangeEnd->mnXFIndex != EXC_XF_NOTFOUND) )
        {
            nEndXclCol = nEndXclCol + aRangeEnd->mnCount;
            ++aRangeEnd;
        }

        // export this range as a record
        if( aRangeBeg != aRangeEnd )
        {
            sal_uInt16 nRelColIdx = nBegXclCol - GetXclCol();
            sal_Int32  nRelCol    = 0;
            for( XclExpMultiXFIdDeq::const_iterator aIt = aRangeBeg; aIt != aRangeEnd; ++aIt )
            {
                for( sal_uInt16 nIdx = 0; nIdx < aIt->mnCount; ++nIdx )
                {
                    WriteXmlContents(
                            rStrm,
                            XclAddress( static_cast<sal_uInt16>(nBegXclCol + nRelCol), GetXclRow() ),
                            aIt->mnXFIndex,
                            nRelColIdx );
                    ++nRelCol;
                    ++nRelColIdx;
                }
            }
        }
    }
}

sal_uInt16 XclExpMultiCellBase::GetCellCount() const
{
    return std::accumulate(maXFIds.begin(), maXFIds.end(), sal_uInt16(0),
        [](const sal_uInt16& rSum, const XclExpMultiXFId& rXFId) { return rSum + rXFId.mnCount; });
}

void XclExpMultiCellBase::AppendXFId( const XclExpMultiXFId& rXFId )
{
    if( maXFIds.empty() || (maXFIds.back().mnXFId != rXFId.mnXFId) )
        maXFIds.push_back( rXFId );
    else
        maXFIds.back().mnCount += rXFId.mnCount;
}

void XclExpMultiCellBase::AppendXFId( const XclExpRoot& rRoot,
        const ScPatternAttr* pPattern, sal_uInt16 nScript, sal_uInt32 nForcedXFId, sal_uInt16 nCount )
{
    sal_uInt32 nXFId = (nForcedXFId == EXC_XFID_NOTFOUND) ?
        rRoot.GetXFBuffer().Insert( pPattern, nScript ) : nForcedXFId;
    AppendXFId( XclExpMultiXFId( nXFId, nCount ) );
}

bool XclExpMultiCellBase::TryMergeXFIds( const XclExpMultiCellBase& rCell )
{
    if( GetLastXclCol() + 1 == rCell.GetXclCol() )
    {
        maXFIds.insert( maXFIds.end(), rCell.maXFIds.begin(), rCell.maXFIds.end() );
        return true;
    }
    return false;
}

void XclExpMultiCellBase::GetXFIndexes( ScfUInt16Vec& rXFIndexes ) const
{
    OSL_ENSURE( GetLastXclCol() < rXFIndexes.size(), "XclExpMultiCellBase::GetXFIndexes - vector too small" );
    ScfUInt16Vec::iterator aDestIt = rXFIndexes.begin() + GetXclCol();
    for( const auto& rXFId : maXFIds )
    {
        ::std::fill( aDestIt, aDestIt + rXFId.mnCount, rXFId.mnXFIndex );
        aDestIt += rXFId.mnCount;
    }
}

void XclExpMultiCellBase::RemoveUnusedXFIndexes( const ScfUInt16Vec& rXFIndexes, size_t nStartAllNotFound )
{
    // save last column before calling maXFIds.clear()
    sal_uInt16 nLastXclCol = GetLastXclCol();
    OSL_ENSURE( nLastXclCol < rXFIndexes.size(), "XclExpMultiCellBase::RemoveUnusedXFIndexes - XF index vector too small" );

    // build new XF index vector, containing passed XF indexes
    maXFIds.clear();
    // Process only all that possibly are not EXC_XF_NOTFOUND.
    size_t nEnd = std::min<size_t>(nLastXclCol + 1, nStartAllNotFound);
    for( size_t i = GetXclCol(); i < nEnd; ++i )
    {
        XclExpMultiXFId aXFId( 0 );
        // AppendXFId() tests XclExpXFIndex::mnXFId, set it too
        aXFId.mnXFId = aXFId.mnXFIndex = rXFIndexes[ i ];
        AppendXFId( aXFId );
    }

    // remove leading and trailing unused XF indexes
    if( !maXFIds.empty() && (maXFIds.front().mnXFIndex == EXC_XF_NOTFOUND) )
    {
        SetXclCol( GetXclCol() + maXFIds.front().mnCount );
        maXFIds.erase(maXFIds.begin(), maXFIds.begin() + 1);
    }
    if( !maXFIds.empty() && (maXFIds.back().mnXFIndex == EXC_XF_NOTFOUND) )
        maXFIds.pop_back();

    // The Save() function will skip all XF indexes equal to EXC_XF_NOTFOUND.
}

sal_uInt16 XclExpMultiCellBase::GetStartColAllDefaultCell() const
{
    sal_uInt16 col = GetXclCol();
    sal_uInt16 nMaxNonDefCol = col;
    for( const auto& rXFId : maXFIds )
    {
        col += rXFId.mnCount;
        if (rXFId.mnXFIndex != EXC_XF_DEFAULTCELL)
            nMaxNonDefCol = col;
    }
    return nMaxNonDefCol;
}

XclExpBlankCell::XclExpBlankCell( const XclAddress& rXclPos, const XclExpMultiXFId& rXFId ) :
    XclExpMultiCellBase( EXC_ID3_BLANK, EXC_ID_MULBLANK, 0, rXclPos )
{
    OSL_ENSURE( rXFId.mnCount > 0, "XclExpBlankCell::XclExpBlankCell - invalid count" );
    AppendXFId( rXFId );
}

XclExpBlankCell::XclExpBlankCell(
        const XclExpRoot& rRoot, const XclAddress& rXclPos, sal_uInt16 nLastXclCol,
        const ScPatternAttr* pPattern, sal_uInt32 nForcedXFId ) :
    XclExpMultiCellBase( EXC_ID3_BLANK, EXC_ID_MULBLANK, 0, rXclPos )
{
    OSL_ENSURE( rXclPos.mnCol <= nLastXclCol, "XclExpBlankCell::XclExpBlankCell - invalid column range" );
    // #i46627# use default script type instead of ApiScriptType::WEAK
    AppendXFId( rRoot, pPattern, rRoot.GetDefApiScript(), nForcedXFId, nLastXclCol - rXclPos.mnCol + 1 );
}

bool XclExpBlankCell::TryMerge( const XclExpCellBase& rCell )
{
    const XclExpBlankCell* pBlankCell = dynamic_cast< const XclExpBlankCell* >( &rCell );
    return pBlankCell && TryMergeXFIds( *pBlankCell );
}

void XclExpBlankCell::GetBlankXFIndexes( ScfUInt16Vec& rXFIndexes ) const
{
    GetXFIndexes( rXFIndexes );
}

void XclExpBlankCell::RemoveUnusedBlankCells( const ScfUInt16Vec& rXFIndexes, size_t nStartAllNotFound )
{
    RemoveUnusedXFIndexes( rXFIndexes, nStartAllNotFound );
}

void XclExpBlankCell::WriteContents( XclExpStream& /*rStrm*/, sal_uInt16 /*nRelCol*/ )
{
}

void XclExpBlankCell::WriteXmlContents( XclExpXmlStream& rStrm, const XclAddress& rAddress, sal_uInt32 nXFId, sal_uInt16 /* nRelCol */ )
{
    sax_fastparser::FSHelperPtr& rWorksheet = rStrm.GetCurrentStream();
    rWorksheet->singleElement( XML_c,
            XML_r, XclXmlUtils::ToOString(rStrm.GetRoot().GetStringBuf(), rAddress).getStr(),
            XML_s, lcl_GetStyleId(rStrm, nXFId) );
}

XclExpRkCell::XclExpRkCell(
        const XclExpRoot& rRoot, const XclAddress& rXclPos,
        const ScPatternAttr* pPattern, sal_uInt32 nForcedXFId, sal_Int32 nRkValue ) :
    XclExpMultiCellBase( EXC_ID_RK, EXC_ID_MULRK, 4, rXclPos )
{
    // #i41210# always use latin script for number cells - may look wrong for special number formats...
    AppendXFId( rRoot, pPattern, ApiScriptType::LATIN, nForcedXFId );
    maRkValues.push_back( nRkValue );
}

bool XclExpRkCell::TryMerge( const XclExpCellBase& rCell )
{
    const XclExpRkCell* pRkCell = dynamic_cast< const XclExpRkCell* >( &rCell );
    if( pRkCell && TryMergeXFIds( *pRkCell ) )
    {
        maRkValues.insert( maRkValues.end(), pRkCell->maRkValues.begin(), pRkCell->maRkValues.end() );
        return true;
    }
    return false;
}

void XclExpRkCell::WriteXmlContents( XclExpXmlStream& rStrm, const XclAddress& rAddress, sal_uInt32 nXFId, sal_uInt16 nRelCol )
{
    sax_fastparser::FSHelperPtr& rWorksheet = rStrm.GetCurrentStream();
    rWorksheet->startElement( XML_c,
            XML_r, XclXmlUtils::ToOString(rStrm.GetRoot().GetStringBuf(), rAddress).getStr(),
            XML_s, lcl_GetStyleId(rStrm, nXFId),
            XML_t, "n"
            // OOXTODO: XML_cm, XML_vm, XML_ph
    );
    rWorksheet->startElement( XML_v );
    rWorksheet->write( XclTools::GetDoubleFromRK( maRkValues[ nRelCol ] ) );
    rWorksheet->endElement( XML_v );
    rWorksheet->endElement( XML_c );
}

void XclExpRkCell::WriteContents( XclExpStream& rStrm, sal_uInt16 nRelCol )
{
    OSL_ENSURE( nRelCol < maRkValues.size(), "XclExpRkCell::WriteContents - overflow error" );
    rStrm << maRkValues[ nRelCol ];
}

// Rows and Columns

XclExpOutlineBuffer::XclExpOutlineBuffer( const XclExpRoot& rRoot, bool bRows ) :
        mpScOLArray( nullptr ),
        maLevelInfos( SC_OL_MAXDEPTH ),
        mnCurrLevel( 0 ),
        mbCurrCollapse( false )
{
    if( const ScOutlineTable* pOutlineTable = rRoot.GetDoc().GetOutlineTable( rRoot.GetCurrScTab() ) )
        mpScOLArray = &(bRows ? pOutlineTable->GetRowArray() : pOutlineTable->GetColArray());

    if( mpScOLArray )
        for( size_t nLevel = 0; nLevel < SC_OL_MAXDEPTH; ++nLevel )
            if( const ScOutlineEntry* pEntry = mpScOLArray->GetEntryByPos( nLevel, 0 ) )
                maLevelInfos[ nLevel ].mnScEndPos = pEntry->GetEnd();
}

void XclExpOutlineBuffer::UpdateColRow( SCCOLROW nScPos )
{
    if( !mpScOLArray )
        return;

    // find open level index for passed position
    size_t nNewOpenScLevel = 0; // new open level (0-based Calc index)
    sal_uInt8 nNewLevel = 0;    // new open level (1-based Excel index)

    if( mpScOLArray->FindTouchedLevel( nScPos, nScPos, nNewOpenScLevel ) )
        nNewLevel = static_cast< sal_uInt8 >( nNewOpenScLevel + 1 );
    // else nNewLevel keeps 0 to show that there are no groups

    mbCurrCollapse = false;
    if( nNewLevel >= mnCurrLevel )
    {
        // new level(s) opened, or no level closed - update all level infos
        for( size_t nScLevel = 0; nScLevel <= nNewOpenScLevel; ++nScLevel )
        {
            /*  In each level: check if a new group is started (there may be
                neighbored groups without gap - therefore check ALL levels). */
            if( maLevelInfos[ nScLevel ].mnScEndPos < nScPos )
            {
                if( const ScOutlineEntry* pEntry = mpScOLArray->GetEntryByPos( nScLevel, nScPos ) )
                {
                    maLevelInfos[ nScLevel ].mnScEndPos = pEntry->GetEnd();
                    maLevelInfos[ nScLevel ].mbHidden = pEntry->IsHidden();
                }
            }
        }
    }
    else
    {
        // level(s) closed - check if any of the closed levels are collapsed
        // Calc uses 0-based level indexes
        sal_uInt16 nOldOpenScLevel = mnCurrLevel - 1;
        for( sal_uInt16 nScLevel = nNewOpenScLevel + 1; !mbCurrCollapse && (nScLevel <= nOldOpenScLevel); ++nScLevel )
            mbCurrCollapse = maLevelInfos[ nScLevel ].mbHidden;
    }

    // cache new opened level
    mnCurrLevel = nNewLevel;
}

XclExpGuts::XclExpGuts( const XclExpRoot& rRoot ) :
    XclExpRecord( EXC_ID_GUTS, 8 ),
    mnColLevels( 0 ),
    mnColWidth( 0 ),
    mnRowLevels( 0 ),
    mnRowWidth( 0 )
{
    const ScOutlineTable* pOutlineTable = rRoot.GetDoc().GetOutlineTable( rRoot.GetCurrScTab() );
    if(!pOutlineTable)
        return;

    // column outline groups
    const ScOutlineArray& rColArray = pOutlineTable->GetColArray();
    mnColLevels = ulimit_cast< sal_uInt16 >( rColArray.GetDepth(), EXC_OUTLINE_MAX );
    if( mnColLevels )
    {
        ++mnColLevels;
        mnColWidth = 12 * mnColLevels + 5;
    }

    // row outline groups
    const ScOutlineArray& rRowArray = pOutlineTable->GetRowArray();
    mnRowLevels = ulimit_cast< sal_uInt16 >( rRowArray.GetDepth(), EXC_OUTLINE_MAX );
    if( mnRowLevels )
    {
        ++mnRowLevels;
        mnRowWidth = 12 * mnRowLevels + 5;
    }
}

void XclExpGuts::WriteBody( XclExpStream& rStrm )
{
    rStrm << mnRowWidth << mnColWidth << mnRowLevels << mnColLevels;
}

XclExpDimensions::XclExpDimensions( const XclExpRoot& rRoot ) :
    mrRoot(rRoot),
    mnFirstUsedXclRow( 0 ),
    mnFirstFreeXclRow( 0 ),
    mnFirstUsedXclCol( 0 ),
    mnFirstFreeXclCol( 0 )
{
    switch( rRoot.GetBiff() )
    {
        case EXC_BIFF2: SetRecHeader( EXC_ID2_DIMENSIONS, 8 );  break;
        case EXC_BIFF3:
        case EXC_BIFF4:
        case EXC_BIFF5: SetRecHeader( EXC_ID3_DIMENSIONS, 10 ); break;
        case EXC_BIFF8: SetRecHeader( EXC_ID3_DIMENSIONS, 14 ); break;
        default:        DBG_ERROR_BIFF();
    }
}

void XclExpDimensions::SetDimensions(
        sal_uInt16 nFirstUsedXclCol, sal_uInt32 nFirstUsedXclRow,
        sal_uInt16 nFirstFreeXclCol, sal_uInt32 nFirstFreeXclRow )
{
    mnFirstUsedXclRow = nFirstUsedXclRow;
    mnFirstFreeXclRow = nFirstFreeXclRow;
    mnFirstUsedXclCol = nFirstUsedXclCol;
    mnFirstFreeXclCol = nFirstFreeXclCol;
}

void XclExpDimensions::SaveXml( XclExpXmlStream& rStrm )
{
    ScRange aRange;
    aRange.aStart.SetRow( static_cast<SCROW>(mnFirstUsedXclRow) );
    aRange.aStart.SetCol( static_cast<SCCOL>(mnFirstUsedXclCol) );

    if( mnFirstFreeXclRow != mnFirstUsedXclRow && mnFirstFreeXclCol != mnFirstUsedXclCol )
    {
        aRange.aEnd.SetRow( static_cast<SCROW>(mnFirstFreeXclRow-1) );
        aRange.aEnd.SetCol( static_cast<SCCOL>(mnFirstFreeXclCol-1) );
    }

    aRange.PutInOrder();
    rStrm.GetCurrentStream()->singleElement( XML_dimension,
            // To be compatible with MS Office 2007,
            // we need full address notation format
            // e.g. "A1:AMJ177" and not partial like: "1:177".
            XML_ref, XclXmlUtils::ToOString(mrRoot.GetDoc(), aRange, true) );
}

void XclExpDimensions::WriteBody( XclExpStream& rStrm )
{
    XclBiff eBiff = rStrm.GetRoot().GetBiff();
    if( eBiff == EXC_BIFF8 )
        rStrm << mnFirstUsedXclRow << mnFirstFreeXclRow;
    else
        rStrm << static_cast< sal_uInt16 >( mnFirstUsedXclRow ) << static_cast< sal_uInt16 >( mnFirstFreeXclRow );
    rStrm << mnFirstUsedXclCol << mnFirstFreeXclCol;
    if( eBiff >= EXC_BIFF3 )
        rStrm << sal_uInt16( 0 );
}

namespace {

double lclGetCChCorrection(const XclExpRoot& rRoot)
{
    // Convert the correction from 1/256ths of a character size to count of chars
    // TODO: make to fit ECMA-376-1:2016 18.3.1.81 sheetFormatPr (Sheet Format Properties):
    // 5 pixels are added to the base width: 2 for margin padding on each side, plus 1 for gridline
    // So this should depend on rRoot.GetCharWidth(), not on font height

    tools::Long nFontHt = rRoot.GetFontBuffer().GetAppFontData().mnHeight;
    return XclTools::GetXclDefColWidthCorrection(nFontHt) / 256.0;
}

} // namespace

XclExpDefcolwidth::XclExpDefcolwidth( const XclExpRoot& rRoot ) :
    XclExpDoubleRecord(EXC_ID_DEFCOLWIDTH, EXC_DEFCOLWIDTH_DEF + lclGetCChCorrection(rRoot)),
    XclExpRoot( rRoot )
{
}

bool XclExpDefcolwidth::IsDefWidth( sal_uInt16 nXclColWidth ) const
{
    // This formula is taking number of characters with GetValue()
    // and it is translating it into default column width.
    // https://msdn.microsoft.com/en-us/library/documentformat.openxml.spreadsheet.column.aspx
    double defaultColumnWidth = 256.0 * GetValue();

    // exactly matched, if difference is less than 1/16 of a character to the left or to the right
    return std::abs(defaultColumnWidth - nXclColWidth) < 256.0 * 1.0 / 16.0;
}

void XclExpDefcolwidth::SetDefWidth( sal_uInt16 nXclColWidth, bool bXLS )
{
    double fCCh = nXclColWidth / 256.0;
    if (bXLS)
    {
        const double fCorrection = lclGetCChCorrection(GetRoot());
        const double fCorrectedCCh = fCCh - fCorrection;
        // Now get the value which would be stored in XLS DefColWidth struct
        double fCChRound = std::round(fCorrectedCCh);
        // If default width was set to a value that is not representable as integral CCh between 0
        // and 255, then just ignore that value, and use an arbitrary default. That way, the stored
        // default might not represent the most used column width (or any used column width), but
        // that's OK, and it just means that those columns will explicitly store their width in
        // 1/256ths of char, and have fUserSet in their ColInfo records.
        if (fCChRound < 0.0 || fCChRound > 255.0 || std::abs(fCChRound - fCorrectedCCh) > 1.0 / 512)
            fCChRound = 8.0;
        fCCh = fCChRound + fCorrection;
    }

    SetValue(fCCh);
}

void XclExpDefcolwidth::Save(XclExpStream& rStrm)
{
    double fCorrectedCCh = GetValue() - lclGetCChCorrection(GetRoot());
    // Convert double to sal_uInt16
    XclExpUInt16Record aUInt16Rec(GetRecId(),
                                  static_cast<sal_uInt16>(std::round(fCorrectedCCh)));
    aUInt16Rec.Save(rStrm);
}

XclExpColinfo::XclExpColinfo( const XclExpRoot& rRoot,
        SCCOL nScCol, SCROW nLastScRow, XclExpColOutlineBuffer& rOutlineBfr ) :
    XclExpRecord( EXC_ID_COLINFO, 12 ),
    XclExpRoot( rRoot ),
    mbCustomWidth( false ),
    mnWidth( 0 ),
    mnScWidth( 0 ),
    mnFlags( 0 ),
    mnOutlineLevel( 0 ),
    mnFirstXclCol( static_cast< sal_uInt16 >( nScCol ) ),
    mnLastXclCol( static_cast< sal_uInt16 >( nScCol ) )
{
    ScDocument& rDoc = GetDoc();
    SCTAB nScTab = GetCurrScTab();

    // column default format
    maXFId.mnXFId = GetXFBuffer().Insert(
        rDoc.GetMostUsedPattern( nScCol, 0, nLastScRow, nScTab ), GetDefApiScript() );

    // column width. If column is hidden then we should return real value (not zero)
    sal_uInt16 nScWidth = rDoc.GetColWidth( nScCol, nScTab, false );
    mnWidth = XclTools::GetXclColumnWidth( nScWidth, GetCharWidth() );
    mnScWidth = convertTwipToMm100(nScWidth);

    // column flags
    ::set_flag( mnFlags, EXC_COLINFO_HIDDEN, rDoc.ColHidden(nScCol, nScTab) );

    // outline data
    rOutlineBfr.Update( nScCol );
    ::set_flag( mnFlags, EXC_COLINFO_COLLAPSED, rOutlineBfr.IsCollapsed() );
    ::insert_value( mnFlags, rOutlineBfr.GetLevel(), 8, 3 );
    mnOutlineLevel = rOutlineBfr.GetLevel();
}

void XclExpColinfo::ConvertXFIndexes()
{
    maXFId.ConvertXFIndex( GetRoot() );
}

bool XclExpColinfo::IsDefault( const XclExpDefcolwidth& rDefColWidth )
{
    mbCustomWidth = !rDefColWidth.IsDefWidth(mnWidth);
    return (maXFId.mnXFIndex == EXC_XF_DEFAULTCELL) &&
           (mnFlags == 0) &&
           (mnOutlineLevel == 0) &&
           !mbCustomWidth;
}

bool XclExpColinfo::TryMerge( const XclExpColinfo& rColInfo )
{
    if( (maXFId.mnXFIndex == rColInfo.maXFId.mnXFIndex) &&
        (mnWidth == rColInfo.mnWidth) &&
        (mnFlags == rColInfo.mnFlags) &&
        (mnOutlineLevel == rColInfo.mnOutlineLevel) &&
        (mnLastXclCol + 1 == rColInfo.mnFirstXclCol) )
    {
        mnLastXclCol = rColInfo.mnLastXclCol;
        return true;
    }
    return false;
}

void XclExpColinfo::WriteBody( XclExpStream& rStrm )
{
    // if last column is equal to last possible column, Excel adds one more
    sal_uInt16 nLastXclCol = mnLastXclCol;
    if( nLastXclCol == static_cast< sal_uInt16 >( rStrm.GetRoot().GetMaxPos().Col() ) )
        ++nLastXclCol;

    rStrm   << mnFirstXclCol
            << nLastXclCol
            << mnWidth
            << maXFId.mnXFIndex
            << mnFlags
            << sal_uInt16( 0 );
}

void XclExpColinfo::SaveXml( XclExpXmlStream& rStrm )
{
    const double nExcelColumnWidth = mnScWidth / convertTwipToMm100<double>(GetCharWidth());

    // tdf#101363 In MS specification the output value is set with double precision after delimiter:
    // =Truncate(({width in pixels} - 5)/{Maximum Digit Width} * 100 + 0.5)/100
    // Explanation of magic numbers:
    // 5 number - are 4 pixels of margin padding (two on each side), plus 1 pixel padding for the gridlines.
    //            It is unknown if it should be applied during LibreOffice export
    // 100 number - used to limit precision to 0.01 with formula =Truncate( {value}*100+0.5 ) / 100
    // 0.5 number (0.005 to output value) - used to increase value before truncating,
    //            to avoid situation when 2.997 will be truncated to 2.99 and not to 3.00
    const double nTruncatedExcelColumnWidth = std::trunc( nExcelColumnWidth * 100.0 + 0.5 ) / 100.0;
    rStrm.GetCurrentStream()->singleElement( XML_col,
            // OOXTODO: XML_bestFit,
            XML_collapsed,      ToPsz( ::get_flag( mnFlags, EXC_COLINFO_COLLAPSED ) ),
            XML_customWidth,    ToPsz( mbCustomWidth ),
            XML_hidden,         ToPsz( ::get_flag( mnFlags, EXC_COLINFO_HIDDEN ) ),
            XML_outlineLevel,   OString::number(mnOutlineLevel),
            XML_max,            OString::number(mnLastXclCol + 1),
            XML_min,            OString::number(mnFirstXclCol + 1),
            // OOXTODO: XML_phonetic,
            XML_style,          lcl_GetStyleId(rStrm, maXFId.mnXFIndex),
            XML_width,          OString::number(nTruncatedExcelColumnWidth) );
}

XclExpColinfoBuffer::XclExpColinfoBuffer( const XclExpRoot& rRoot ) :
    XclExpRoot( rRoot ),
    maDefcolwidth( rRoot ),
    maOutlineBfr( rRoot ),
    mnHighestOutlineLevel( 0 )
{
}

void XclExpColinfoBuffer::Initialize( SCROW nLastScRow )
{

    for( sal_uInt16 nScCol = 0, nLastScCol = GetMaxPos().Col(); nScCol <= nLastScCol; ++nScCol )
    {
        maColInfos.AppendNewRecord( new XclExpColinfo( GetRoot(), nScCol, nLastScRow, maOutlineBfr ) );
        if( maOutlineBfr.GetLevel() > mnHighestOutlineLevel )
        {
           mnHighestOutlineLevel = maOutlineBfr.GetLevel();
        }
    }
}

void XclExpColinfoBuffer::Finalize( ScfUInt16Vec& rXFIndexes, bool bXLS )
{
    rXFIndexes.clear();
    rXFIndexes.reserve( maColInfos.GetSize() );

    if( !maColInfos.IsEmpty())
    {
        XclExpColinfo* xPrevRec = maColInfos.GetRecord( 0 );
        xPrevRec->ConvertXFIndexes();
        for( size_t nPos = 1; nPos < maColInfos.GetSize(); ++nPos )
        {
            XclExpColinfo* xRec = maColInfos.GetRecord( nPos );
            xRec->ConvertXFIndexes();

            // try to merge with previous record
            if( xPrevRec->TryMerge( *xRec ) )
                maColInfos.InvalidateRecord( nPos );
            else
                xPrevRec = xRec;
        }
        maColInfos.RemoveInvalidatedRecords();
    }

    // put XF indexes into passed vector, collect use count of all different widths
    std::unordered_map< sal_uInt16, sal_uInt16 > aWidthMap;
    sal_uInt16 nMaxColCount = 0;
    sal_uInt16 nMaxUsedWidth = 0;
    for( size_t nPos = 0; nPos < maColInfos.GetSize(); ++nPos )
    {
        const XclExpColinfo* xRec = maColInfos.GetRecord( nPos );
        sal_uInt16 nColCount = xRec->GetColCount();

        // add XF index to passed vector
        rXFIndexes.resize( rXFIndexes.size() + nColCount, xRec->GetXFIndex() );

        // collect use count of column width
        sal_uInt16 nWidth = xRec->GetColWidth();
        sal_uInt16& rnMapCount = aWidthMap[ nWidth ];
        rnMapCount = rnMapCount + nColCount;
        if( rnMapCount > nMaxColCount )
        {
            nMaxColCount = rnMapCount;
            nMaxUsedWidth = nWidth;
        }
    }
    maDefcolwidth.SetDefWidth( nMaxUsedWidth, bXLS );

    // remove all default COLINFO records
    for( size_t nPos = 0; nPos < maColInfos.GetSize(); ++nPos )
    {
        XclExpColinfo* xRec = maColInfos.GetRecord( nPos );
        if( xRec->IsDefault( maDefcolwidth ) )
            maColInfos.InvalidateRecord( nPos );
    }
    maColInfos.RemoveInvalidatedRecords();
}

void XclExpColinfoBuffer::Save( XclExpStream& rStrm )
{
    // DEFCOLWIDTH
    maDefcolwidth.Save( rStrm );
    // COLINFO records
    maColInfos.Save( rStrm );
}

void XclExpColinfoBuffer::SaveXml( XclExpXmlStream& rStrm )
{
    if( maColInfos.IsEmpty() )
        return;

    sax_fastparser::FSHelperPtr& rWorksheet = rStrm.GetCurrentStream();
    rWorksheet->startElement(XML_cols);
    maColInfos.SaveXml( rStrm );
    rWorksheet->endElement( XML_cols );
}

XclExpDefaultRowData::XclExpDefaultRowData() :
    mnFlags( EXC_DEFROW_DEFAULTFLAGS ),
    mnHeight( EXC_DEFROW_DEFAULTHEIGHT )
{
}

XclExpDefaultRowData::XclExpDefaultRowData( const XclExpRow& rRow ) :
    mnFlags( EXC_DEFROW_DEFAULTFLAGS ),
    mnHeight( rRow.GetHeight() )
{
    ::set_flag( mnFlags, EXC_DEFROW_HIDDEN, rRow.IsHidden() );
    ::set_flag( mnFlags, EXC_DEFROW_UNSYNCED, rRow.IsUnsynced() );
}

static bool operator<( const XclExpDefaultRowData& rLeft, const XclExpDefaultRowData& rRight )
{
    return (rLeft.mnHeight < rRight.mnHeight) ||
        ((rLeft.mnHeight == rRight.mnHeight) && (rLeft.mnFlags < rRight.mnFlags));
}

XclExpDefrowheight::XclExpDefrowheight() :
    XclExpRecord( EXC_ID3_DEFROWHEIGHT, 4 )
{
}

void XclExpDefrowheight::SetDefaultData( const XclExpDefaultRowData& rDefData )
{
    maDefData = rDefData;
}

void XclExpDefrowheight::WriteBody( XclExpStream& rStrm )
{
    OSL_ENSURE_BIFF( rStrm.GetRoot().GetBiff() >= EXC_BIFF3 );
    rStrm << maDefData.mnFlags << maDefData.mnHeight;
}

XclExpRow::XclExpRow( const XclExpRoot& rRoot, sal_uInt32 nXclRow,
        XclExpRowOutlineBuffer& rOutlineBfr, bool bAlwaysEmpty, bool bHidden, sal_uInt16 nHeight ) :
    XclExpRecord( EXC_ID3_ROW, 16 ),
    XclExpRoot( rRoot ),
    mnXclRow( nXclRow ),
    mnHeight( nHeight ),
    mnFlags( EXC_ROW_DEFAULTFLAGS ),
    mnXFIndex( EXC_XF_DEFAULTCELL ),
    mnOutlineLevel( 0 ),
    mnXclRowRpt( 1 ),
    mnCurrentRow( nXclRow ),
    mbAlwaysEmpty( bAlwaysEmpty ),
    mbEnabled( true )
{
    SCTAB nScTab = GetCurrScTab();
    SCROW nScRow = static_cast< SCROW >( mnXclRow );

    // *** Row flags *** ------------------------------------------------------

    CRFlags nRowFlags = GetDoc().GetRowFlags( nScRow, nScTab );
    bool bUserHeight( nRowFlags & CRFlags::ManualSize );
    ::set_flag( mnFlags, EXC_ROW_UNSYNCED, bUserHeight );
    ::set_flag( mnFlags, EXC_ROW_HIDDEN, bHidden );

    // *** Outline data *** ---------------------------------------------------

    rOutlineBfr.Update( nScRow );
    ::set_flag( mnFlags, EXC_ROW_COLLAPSED, rOutlineBfr.IsCollapsed() );
    ::insert_value( mnFlags, rOutlineBfr.GetLevel(), 0, 3 );
    mnOutlineLevel = rOutlineBfr.GetLevel();

    // *** Progress bar *** ---------------------------------------------------

    XclExpProgressBar& rProgress = GetProgressBar();
    rProgress.IncRowRecordCount();
    rProgress.Progress();
}

static size_t findFirstAllSameUntilEnd( const ScfUInt16Vec& rIndexes, sal_uInt16 value,
    size_t searchStart = std::numeric_limits<size_t>::max())
{
    for( size_t i = std::min(rIndexes.size(), searchStart); i >= 1; --i )
    {
        if( rIndexes[ i - 1 ] != value )
            return i;
    }
    return 0;
}

void XclExpRow::AppendCell( XclExpCellRef const & xCell, bool bIsMergedBase )
{
    OSL_ENSURE( !mbAlwaysEmpty, "XclExpRow::AppendCell - row is marked to be always empty" );
    // try to merge with last existing cell
    InsertCell( xCell, maCellList.GetSize(), bIsMergedBase );
}

void XclExpRow::Finalize( const ScfUInt16Vec& rColXFIndexes, ScfUInt16Vec& aXFIndexes, size_t nStartColAllDefault, bool bProgress )
{
    size_t nPos, nSize;

    // *** Convert XF identifiers *** -----------------------------------------

    // additionally collect the blank XF indexes
    size_t nColCount = GetMaxPos().Col() + 1;
    OSL_ENSURE( rColXFIndexes.size() == nColCount, "XclExpRow::Finalize - wrong column XF index count" );

    // The vector should be preset to all items being EXC_XF_NOTFOUND, to avoid repeated allocations
    // and clearing.
    assert( aXFIndexes.size() == nColCount );
    assert( aXFIndexes.front() == EXC_XF_NOTFOUND );
    assert( aXFIndexes.back() == EXC_XF_NOTFOUND );
    for( nPos = 0, nSize = maCellList.GetSize(); nPos < nSize; ++nPos )
    {
        XclExpCellBase* pCell = maCellList.GetRecord( nPos );
        pCell->ConvertXFIndexes( GetRoot() );
        pCell->GetBlankXFIndexes( aXFIndexes );
    }

    // *** Fill gaps with BLANK/MULBLANK cell records *** ---------------------

    /*  This is needed because nonexistent cells in Calc are not formatted at all,
        but in Excel they would have the column default format. Blank cells that
        are equal to the respective column default are removed later in this function. */
    if( !mbAlwaysEmpty )
    {
        // XF identifier representing default cell XF
        XclExpMultiXFId aXFId( XclExpXFBuffer::GetDefCellXFId() );
        aXFId.ConvertXFIndex( GetRoot() );

        nPos = 0;
        while( nPos <= maCellList.GetSize() )  // don't cache list size, may change in the loop
        {
            // get column index that follows previous cell
            sal_uInt16 nFirstFreeXclCol = (nPos > 0) ? (maCellList.GetRecord( nPos - 1 )->GetLastXclCol() + 1) : 0;
            // get own column index
            sal_uInt16 nNextUsedXclCol = (nPos < maCellList.GetSize()) ? maCellList.GetRecord( nPos )->GetXclCol() : (GetMaxPos().Col() + 1);

            // is there a gap?
            if( nFirstFreeXclCol < nNextUsedXclCol )
            {
                aXFId.mnCount = nNextUsedXclCol - nFirstFreeXclCol;
                XclExpCellRef xNewCell = new XclExpBlankCell( XclAddress( nFirstFreeXclCol, mnXclRow ), aXFId );
                // insert the cell, InsertCell() may merge it with existing BLANK records
                InsertCell( xNewCell, nPos, false );
                // insert default XF indexes into aXFIndexes
                for( size_t i = nFirstFreeXclCol; i < nNextUsedXclCol; ++i )
                    aXFIndexes[ i ] = aXFId.mnXFIndex;
                // don't step forward with nPos, InsertCell() may remove records
            }
            else
                ++nPos;
        }
    }

    // *** Find default row format *** ----------------------------------------

    // Often there will be many EXC_XF_DEFAULTCELL at the end, optimize by ignoring them.
    size_t nStartSearchAllDefault = aXFIndexes.size();
    if( !maCellList.IsEmpty() && dynamic_cast< const XclExpBlankCell* >( maCellList.GetLastRecord()))
    {
        const XclExpBlankCell* pLastBlank = static_cast< const XclExpBlankCell* >( maCellList.GetLastRecord());
        assert(pLastBlank->GetLastXclCol() == aXFIndexes.size() - 1);
        nStartSearchAllDefault = pLastBlank->GetStartColAllDefaultCell();
    }
    size_t nStartAllDefault = findFirstAllSameUntilEnd( aXFIndexes, EXC_XF_DEFAULTCELL, nStartSearchAllDefault);

    // find most used XF index in the row
    sal_uInt16 nRowXFIndex = EXC_XF_DEFAULTCELL;
    const size_t nHalfIndexes = aXFIndexes.size() / 2;
    if( nStartAllDefault > nHalfIndexes ) // Otherwise most are EXC_XF_DEFAULTCELL.
    {
        // Very likely the most common one is going to be the last one.
        nRowXFIndex = aXFIndexes.back();
        size_t nStartLastSame = findFirstAllSameUntilEnd( aXFIndexes, nRowXFIndex );
        if( nStartLastSame > nHalfIndexes ) // No, find out the most used one by counting.
        {
            std::unordered_map< sal_uInt16, size_t > aIndexMap;
            size_t nMaxXFCount = 0;
            for( const auto& rXFIndex : aXFIndexes )
            {
                if( rXFIndex != EXC_XF_NOTFOUND )
                {
                    size_t& rnCount = aIndexMap[ rXFIndex ];
                    ++rnCount;
                    if( rnCount > nMaxXFCount )
                    {
                        nRowXFIndex = rXFIndex;
                        nMaxXFCount = rnCount;
                        if (nMaxXFCount > nHalfIndexes)
                        {
                            // No other XF index can have a greater usage count, we
                            // don't need to loop through the remaining cells.
                            // Specifically for the tail of unused default
                            // cells/columns this makes a difference.
                            break;  // for
                        }
                    }
                }
            }
        }
    }

    // decide whether to use the row default XF index or column default XF indexes
    bool bUseColDefXFs = nRowXFIndex == EXC_XF_DEFAULTCELL;
    if( !bUseColDefXFs )
    {
        // count needed XF indexes for blank cells with and without row default XF index
        size_t nXFCountWithRowDefXF = 0;
        size_t nXFCountWithoutRowDefXF = 0;
        ScfUInt16Vec::const_iterator aColIt = rColXFIndexes.begin();
        for( const auto& rXFIndex : aXFIndexes )
        {
            sal_uInt16 nXFIndex = rXFIndex;
            if( nXFIndex != EXC_XF_NOTFOUND )
            {
                if( nXFIndex != nRowXFIndex )
                    ++nXFCountWithRowDefXF;     // with row default XF index
                if( nXFIndex != *aColIt )
                    ++nXFCountWithoutRowDefXF;  // without row default XF index
            }
            ++aColIt;
        }

        // use column XF indexes if this would cause less or equal number of BLANK records
        bUseColDefXFs = nXFCountWithoutRowDefXF <= nXFCountWithRowDefXF;
    }

    // *** Remove unused BLANK cell records *** -------------------------------

    size_t maxStartAllNotFound;
    if( bUseColDefXFs )
    {
        size_t maxStartAllDefault = std::max( nStartAllDefault, nStartColAllDefault );
        // use column default XF indexes
        // #i194#: remove cell XF indexes equal to column default XF indexes
        for( size_t i = 0; i < maxStartAllDefault; ++i )
        {
            if( aXFIndexes[ i ] == rColXFIndexes[ i ] )
                aXFIndexes[ i ] = EXC_XF_NOTFOUND;
        }
        // They can differ only up to maxNonDefault, in the rest they are the same.
        for( size_t i = maxStartAllDefault; i < aXFIndexes.size(); ++i )
            aXFIndexes[ i ] = EXC_XF_NOTFOUND;
        maxStartAllNotFound = maxStartAllDefault;
    }
    else
    {
        // use row default XF index
        mnXFIndex = nRowXFIndex;
        ::set_flag( mnFlags, EXC_ROW_USEDEFXF );
        // #98133#, #i194#, #i27407#: remove cell XF indexes equal to row default XF index
        for( auto& rXFIndex : aXFIndexes )
            if( rXFIndex == nRowXFIndex )
                rXFIndex = EXC_XF_NOTFOUND;
        maxStartAllNotFound = aXFIndexes.size();
    }

    // remove unused parts of BLANK/MULBLANK cell records
    size_t nStartAllNotFound = findFirstAllSameUntilEnd( aXFIndexes, EXC_XF_NOTFOUND, maxStartAllNotFound );
    nPos = 0;
    while( nPos < maCellList.GetSize() )   // do not cache list size, may change in the loop
    {
        XclExpCellBase* xCell = maCellList.GetRecord( nPos );
        xCell->RemoveUnusedBlankCells( aXFIndexes, nStartAllNotFound );
        if( xCell->IsEmpty() )
            maCellList.RemoveRecord( nPos );
        else
            ++nPos;
    }
    // Ensure it's all EXC_XF_NOTFOUND again for next reuse.
    for( size_t i = 0; i < nStartAllNotFound; ++i )
        aXFIndexes[ i ] = EXC_XF_NOTFOUND;

    // progress bar includes disabled rows; only update it in the lead thread.
    if (bProgress)
        GetProgressBar().Progress();
}
sal_uInt16 XclExpRow::GetFirstUsedXclCol() const
{
    return maCellList.IsEmpty() ? 0 : maCellList.GetFirstRecord()->GetXclCol();
}

sal_uInt16 XclExpRow::GetFirstFreeXclCol() const
{
    return maCellList.IsEmpty() ? 0 : (maCellList.GetLastRecord()->GetLastXclCol() + 1);
}

bool XclExpRow::IsDefaultable() const
{
    const sal_uInt16 nFlagsAlwaysMarkedAsDefault = EXC_ROW_DEFAULTFLAGS | EXC_ROW_HIDDEN | EXC_ROW_UNSYNCED;
    return !::get_flag( mnFlags, static_cast< sal_uInt16 >( ~nFlagsAlwaysMarkedAsDefault ) ) &&
           IsEmpty();
}

void XclExpRow::DisableIfDefault( const XclExpDefaultRowData& rDefRowData )
{
    mbEnabled = !IsDefaultable() ||
        (mnHeight != rDefRowData.mnHeight) ||
        (IsHidden() != rDefRowData.IsHidden()) ||
        (IsUnsynced() != rDefRowData.IsUnsynced());
}

void XclExpRow::WriteCellList( XclExpStream& rStrm )
{
    OSL_ENSURE( mbEnabled || maCellList.IsEmpty(), "XclExpRow::WriteCellList - cells in disabled row" );
    maCellList.Save( rStrm );
}

void XclExpRow::Save( XclExpStream& rStrm )
{
    if( mbEnabled )
    {
        mnCurrentRow = mnXclRow;
        for ( sal_uInt32 i = 0; i < mnXclRowRpt; ++i, ++mnCurrentRow )
            XclExpRecord::Save( rStrm );
    }
}

void XclExpRow::InsertCell( XclExpCellRef xCell, size_t nPos, bool bIsMergedBase )
{
    OSL_ENSURE( xCell, "XclExpRow::InsertCell - missing cell" );

    /*  If we have a multi-line text in a merged cell, and the resulting
        row height has not been confirmed, we need to force the EXC_ROW_UNSYNCED
        flag to be true to ensure Excel works correctly. */
    if( bIsMergedBase && xCell->IsMultiLineText() )
        ::set_flag( mnFlags, EXC_ROW_UNSYNCED );

    // try to merge with previous cell, insert the new cell if not successful
    XclExpCellBase* xPrevCell = maCellList.GetRecord( nPos - 1 );
    if( xPrevCell && xPrevCell->TryMerge( *xCell ) )
        xCell = xPrevCell;
    else
        maCellList.InsertRecord( xCell, nPos++ );
    // nPos points now to following cell

    // try to merge with following cell, remove it if successful
    XclExpCellRef xNextCell = maCellList.GetRecord( nPos );
    if( xNextCell && xCell->TryMerge( *xNextCell ) )
        maCellList.RemoveRecord( nPos );
}

void XclExpRow::WriteBody( XclExpStream& rStrm )
{
    rStrm   << static_cast< sal_uInt16 >(mnCurrentRow)
            << GetFirstUsedXclCol()
            << GetFirstFreeXclCol()
            << mnHeight
            << sal_uInt32( 0 )
            << mnFlags
            << mnXFIndex;
}

void XclExpRow::SaveXml( XclExpXmlStream& rStrm )
{
    if( !mbEnabled )
        return;
    sax_fastparser::FSHelperPtr& rWorksheet = rStrm.GetCurrentStream();
    bool haveFormat = ::get_flag( mnFlags, EXC_ROW_USEDEFXF );
    mnCurrentRow = mnXclRow + 1;
    for ( sal_uInt32 i=0; i<mnXclRowRpt; ++i )
    {
        rWorksheet->startElement( XML_row,
                XML_r,              OString::number(mnCurrentRow++),
                // OOXTODO: XML_spans,          optional
                XML_s,              haveFormat ? lcl_GetStyleId( rStrm, mnXFIndex ).getStr() : nullptr,
                XML_customFormat,   ToPsz( haveFormat ),
                XML_ht,             OString::number(static_cast<double>(mnHeight) / 20.0),
                XML_hidden,         ToPsz( ::get_flag( mnFlags, EXC_ROW_HIDDEN ) ),
                XML_customHeight,   ToPsz( ::get_flag( mnFlags, EXC_ROW_UNSYNCED ) ),
                XML_outlineLevel,   OString::number(mnOutlineLevel),
                XML_collapsed,      ToPsz( ::get_flag( mnFlags, EXC_ROW_COLLAPSED ) )
                // OOXTODO: XML_thickTop,       bool
                // OOXTODO: XML_thickBot,       bool
                // OOXTODO: XML_ph,             bool
        );
        // OOXTODO: XML_extLst
        maCellList.SaveXml( rStrm );
        rWorksheet->endElement( XML_row );
    }
}

XclExpRowBuffer::XclExpRowBuffer( const XclExpRoot& rRoot ) :
    XclExpRoot( rRoot ),
    maOutlineBfr( rRoot ),
    maDimensions( rRoot ),
    mnHighestOutlineLevel( 0 )
{
}

void XclExpRowBuffer::AppendCell( XclExpCellRef const & xCell, bool bIsMergedBase )
{
    OSL_ENSURE( xCell, "XclExpRowBuffer::AppendCell - missing cell" );
    GetOrCreateRow( xCell->GetXclRow(), false ).AppendCell( xCell, bIsMergedBase );
}

void XclExpRowBuffer::CreateRows( SCROW nFirstFreeScRow )
{
    if( nFirstFreeScRow > 0 )
        GetOrCreateRow(  ::std::max ( nFirstFreeScRow - 1, GetMaxPos().Row() ), true );
}

namespace {

class RowFinalizeTask : public comphelper::ThreadTask
{
    bool mbProgress;
    const ScfUInt16Vec& mrColXFIndexes;
    size_t mnStartColAllDefault;
    std::vector< XclExpRow * > maRows;
public:
             RowFinalizeTask( const std::shared_ptr<comphelper::ThreadTaskTag> & pTag,
                              const ScfUInt16Vec& rColXFIndexes,
                              size_t nStartColAllDefault,
                              bool bProgress ) :
                 comphelper::ThreadTask( pTag ),
                 mbProgress( bProgress ),
                 mrColXFIndexes( rColXFIndexes ),
                 mnStartColAllDefault( nStartColAllDefault )
                 {}

    void     push_back( XclExpRow *pRow ) { maRows.push_back( pRow ); }
    virtual void doWork() override
    {
        ScfUInt16Vec aXFIndexes( mrColXFIndexes.size(), EXC_XF_NOTFOUND );
        for (XclExpRow* p : maRows)
            p->Finalize( mrColXFIndexes, aXFIndexes, mnStartColAllDefault, mbProgress );
    }
};

}

void XclExpRowBuffer::Finalize( XclExpDefaultRowData& rDefRowData,
                                const ScfUInt16Vec& rColXFIndexes,
                                size_t nStartColAllDefault )
{
    // *** Finalize all rows *** ----------------------------------------------

    GetProgressBar().ActivateFinalRowsSegment();

#if 1
    // This is staggeringly slow, and each element operates only
    // on its own data.
    const size_t nRows = maRowMap.size();
    const size_t nThreads = nRows < 128 ? 1 : comphelper::ThreadPool::getPreferredConcurrency();
#else
    const size_t nThreads = 1; // globally disable multi-threading for now.
#endif
    if (nThreads == 1)
    {
        ScfUInt16Vec aXFIndexes( rColXFIndexes.size(), EXC_XF_NOTFOUND );
        for (auto& rEntry : maRowMap)
            rEntry.second->Finalize( rColXFIndexes, aXFIndexes, nStartColAllDefault, true );
    }
    else
    {
        comphelper::ThreadPool &rPool = comphelper::ThreadPool::getSharedOptimalPool();
        std::shared_ptr<comphelper::ThreadTaskTag> pTag = comphelper::ThreadPool::createThreadTaskTag();
        std::vector<std::unique_ptr<RowFinalizeTask>> aTasks(nThreads);
        for ( size_t i = 0; i < nThreads; i++ )
            aTasks[ i ].reset( new RowFinalizeTask( pTag, rColXFIndexes, nStartColAllDefault, i == 0 ) );

        size_t nIdx = 0;
        for ( const auto& rEntry : maRowMap )
        {
            aTasks[ nIdx % nThreads ]->push_back( rEntry.second.get() );
            ++nIdx;
        }

        for ( size_t i = 1; i < nThreads; i++ )
            rPool.pushTask( std::move(aTasks[ i ]) );

        // Progress bar updates must be synchronous to avoid deadlock
        aTasks[0]->doWork();

        rPool.waitUntilDone(pTag);
    }

    // *** Default row format *** ---------------------------------------------

    std::map< XclExpDefaultRowData, size_t > aDefRowMap;

    XclExpDefaultRowData aMaxDefData;
    size_t nMaxDefCount = 0;
    // only look for default format in existing rows, if there are more than unused
    // if the row is hidden, then row xml must be created even if it not contain cells
    XclExpRow* pPrev = nullptr;
    std::vector< XclExpRow* > aRepeated;
    for (const auto& rEntry : maRowMap)
    {
        const RowRef& rRow = rEntry.second;
        if ( rRow->IsDefaultable() )
        {
            XclExpDefaultRowData aDefData( *rRow );
            size_t& rnDefCount = aDefRowMap[ aDefData ];
            ++rnDefCount;
            if( rnDefCount > nMaxDefCount )
            {
                nMaxDefCount = rnDefCount;
                aMaxDefData = aDefData;
            }
        }
        if ( pPrev )
        {
            if ( pPrev->IsDefaultable() )
            {
                // if the previous row we processed is not
                // defaultable then afaict the rows in between are
                // not used ( and not repeatable )
                sal_uInt32 nRpt =  rRow->GetXclRow() - pPrev->GetXclRow();
                if ( nRpt > 1 )
                    aRepeated.push_back( pPrev );
                pPrev->SetXclRowRpt( nRpt );
                XclExpDefaultRowData aDefData( *pPrev );
                size_t& rnDefCount = aDefRowMap[ aDefData ];
                rnDefCount += ( pPrev->GetXclRowRpt() - 1 );
                if( rnDefCount > nMaxDefCount )
                {
                    nMaxDefCount = rnDefCount;
                    aMaxDefData = aDefData;
                }
            }
        }
        pPrev = rRow.get();
    }
    // return the default row format to caller
    rDefRowData = aMaxDefData;

    // now disable repeating extra (empty) rows that are equal to the default row
    for (auto& rpRow : aRepeated)
    {
        if ( rpRow->GetXclRowRpt() > 1
             && rpRow->GetHeight() == rDefRowData.mnHeight
             && rpRow->IsHidden() == rDefRowData.IsHidden() )
        {
            rpRow->SetXclRowRpt( 1 );
        }
    }

    // *** Disable unused ROW records, find used area *** ---------------------

    sal_uInt16 nFirstUsedXclCol = SAL_MAX_UINT16;
    sal_uInt16 nFirstFreeXclCol = 0;
    sal_uInt32 nFirstUsedXclRow = SAL_MAX_UINT32;
    sal_uInt32 nFirstFreeXclRow = 0;

    for (const auto& rEntry : maRowMap)
    {
        const RowRef& rRow = rEntry.second;
        // disable unused rows
        rRow->DisableIfDefault( aMaxDefData );

        // find used column range
        if( !rRow->IsEmpty() )      // empty rows return (0...0) as used range
        {
            nFirstUsedXclCol = ::std::min( nFirstUsedXclCol, rRow->GetFirstUsedXclCol() );
            nFirstFreeXclCol = ::std::max( nFirstFreeXclCol, rRow->GetFirstFreeXclCol() );
        }

        // find used row range
        if( rRow->IsEnabled() )
        {
            sal_uInt32 nXclRow = rRow->GetXclRow();
            nFirstUsedXclRow = ::std::min< sal_uInt32 >( nFirstUsedXclRow, nXclRow );
            nFirstFreeXclRow = ::std::max< sal_uInt32 >( nFirstFreeXclRow, nXclRow + 1 );
        }
    }

    // adjust start position, if there are no or only empty/disabled ROW records
    nFirstUsedXclCol = ::std::min( nFirstUsedXclCol, nFirstFreeXclCol );
    nFirstUsedXclRow = ::std::min( nFirstUsedXclRow, nFirstFreeXclRow );

    // initialize the DIMENSIONS record
    maDimensions.SetDimensions(
        nFirstUsedXclCol, nFirstUsedXclRow, nFirstFreeXclCol, nFirstFreeXclRow );
}

void XclExpRowBuffer::Save( XclExpStream& rStrm )
{
    // DIMENSIONS record
    maDimensions.Save( rStrm );

    // save in blocks of 32 rows, each block contains first all ROWs, then all cells
    size_t nSize = maRowMap.size();
    RowMap::iterator itr = maRowMap.begin(), itrEnd = maRowMap.end();
    RowMap::iterator itrBlkStart = maRowMap.begin(), itrBlkEnd = maRowMap.begin();
    sal_uInt16 nStartXclRow = (nSize == 0) ? 0 : itr->second->GetXclRow();

    for (; itr != itrEnd; ++itr)
    {
        // find end of row block
        itrBlkEnd = std::find_if_not(itrBlkEnd, itrEnd,
            [&nStartXclRow](const RowMap::value_type& rRow) { return rRow.second->GetXclRow() - nStartXclRow < EXC_ROW_ROWBLOCKSIZE; });

        // write the ROW records
        std::for_each(itrBlkStart, itrBlkEnd, [&rStrm](const RowMap::value_type& rRow) { rRow.second->Save( rStrm ); });

        // write the cell records
        std::for_each(itrBlkStart, itrBlkEnd, [&rStrm](const RowMap::value_type& rRow) { rRow.second->WriteCellList( rStrm ); });

        itrBlkStart = (itrBlkEnd == itrEnd) ? itrBlkEnd : itrBlkEnd++;
        nStartXclRow += EXC_ROW_ROWBLOCKSIZE;
    }
}

void XclExpRowBuffer::SaveXml( XclExpXmlStream& rStrm )
{
    if (std::none_of(maRowMap.begin(), maRowMap.end(), [](const RowMap::value_type& rRow) { return rRow.second->IsEnabled(); }))
    {
        rStrm.GetCurrentStream()->singleElement(XML_sheetData);
        return;
    }

    sax_fastparser::FSHelperPtr& rWorksheet = rStrm.GetCurrentStream();
    rWorksheet->startElement(XML_sheetData);
    for (const auto& rEntry : maRowMap)
        rEntry.second->SaveXml(rStrm);
    rWorksheet->endElement( XML_sheetData );
}

XclExpRow& XclExpRowBuffer::GetOrCreateRow( sal_uInt32 nXclRow, bool bRowAlwaysEmpty )
{
    // This is called rather often, so optimize for the most common case of saving row by row
    // (so the requested row is often the last one in the map or belongs after the last one).
    RowMap::iterator itr;
    if(maRowMap.empty())
        itr = maRowMap.end();
    else
    {
        RowMap::reverse_iterator last = maRowMap.rbegin();
        if( last->first == nXclRow )
            return *last->second;
        if( nXclRow > last->first )
            itr = maRowMap.end();
        else
            itr = maRowMap.lower_bound( nXclRow );
    }
    const bool bFound = itr != maRowMap.end();
    // bFoundHigher: nXclRow was identical to the previous entry, so not explicitly created earlier
    // coverity[deref_iterator : FALSE] - clearly itr if only derefed if bFound which checks for valid itr
    const bool bFoundHigher = bFound && itr->first != nXclRow;
    if( bFound && !bFoundHigher )
        return *itr->second;

    size_t nFrom = 0;
    RowRef pPrevEntry;
    if( itr != maRowMap.begin() )
    {
        --itr;
        pPrevEntry = itr->second;
        if( bFoundHigher )
            nFrom = nXclRow;
        else
            nFrom = itr->first + 1;
    }

    const ScDocument& rDoc = GetRoot().GetDoc();
    const SCTAB nScTab = GetRoot().GetCurrScTab();
    // Do not repeatedly call RowHidden() / GetRowHeight() for same values.
    bool bHidden = false;
    SCROW lastSameHiddenRow = -1;
    sal_uInt16 nHeight = 0;
    SCROW lastSameHeightRow = -1;
    // create the missing rows first
    while( nFrom <= nXclRow )
    {
        // only create RowMap entries if it is first row in spreadsheet,
        // if it is the desired row, or for rows that differ from previous.
        if( static_cast<SCROW>(nFrom) > lastSameHiddenRow )
            bHidden = rDoc.RowHidden(nFrom, nScTab, nullptr, &lastSameHiddenRow);
        // Always get the actual row height even if the manual size flag is
        // not set, to correctly export the heights of rows with wrapped
        // texts.
        if( static_cast<SCROW>(nFrom) > lastSameHeightRow )
            nHeight = rDoc.GetRowHeight(nFrom, nScTab, nullptr, &lastSameHeightRow, false);
        if ( !pPrevEntry || ( nFrom == nXclRow ) ||
             ( maOutlineBfr.IsCollapsed() ) ||
             ( maOutlineBfr.GetLevel() != 0 ) ||
             ( bRowAlwaysEmpty && !pPrevEntry->IsEmpty() ) ||
             ( bHidden != pPrevEntry->IsHidden() ) ||
             ( nHeight != pPrevEntry->GetHeight() ) )
        {
            if( maOutlineBfr.GetLevel() > mnHighestOutlineLevel )
            {
                mnHighestOutlineLevel = maOutlineBfr.GetLevel();
            }
            RowRef p = std::make_shared<XclExpRow>(GetRoot(), nFrom, maOutlineBfr, bRowAlwaysEmpty, bHidden, nHeight);
            maRowMap.emplace(nFrom, p);
            pPrevEntry = std::move(p);
        }
        ++nFrom;
    }
    itr = maRowMap.find(nXclRow);
    return *itr->second;
}

// Cell Table

XclExpCellTable::XclExpCellTable( const XclExpRoot& rRoot ) :
    XclExpRoot( rRoot ),
    maColInfoBfr( rRoot ),
    maRowBfr( rRoot ),
    maArrayBfr( rRoot ),
    maShrfmlaBfr( rRoot ),
    maTableopBfr( rRoot ),
    mxDefrowheight( new XclExpDefrowheight() ),
    mxGuts( new XclExpGuts( rRoot ) ),
    mxNoteList( new XclExpNoteList ),
    mxMergedcells( new XclExpMergedcells( rRoot ) ),
    mxHyperlinkList( new XclExpHyperlinkList ),
    mxDval( new XclExpDval( rRoot ) ),
    mxExtLst( new XclExtLst( rRoot ) )
{
    ScDocument& rDoc = GetDoc();
    SCTAB nScTab = GetCurrScTab();
    SvNumberFormatter& rFormatter = GetFormatter();

    // maximum sheet limits
    SCCOL nMaxScCol = GetMaxPos().Col();
    SCROW nMaxScRow = GetMaxPos().Row();

    // find used area (non-empty cells)
    SCCOL nLastUsedScCol;
    SCROW nLastUsedScRow;
    rDoc.GetTableArea( nScTab, nLastUsedScCol, nLastUsedScRow );

    if(nLastUsedScCol > nMaxScCol)
        nLastUsedScCol = nMaxScCol;

    // check extra blank rows to avoid of losing their not default settings (workaround for tdf#41425)
    nLastUsedScRow += 1000;

    if(nLastUsedScRow > nMaxScRow)
        nLastUsedScRow = nMaxScRow;

    ScRange aUsedRange( 0, 0, nScTab, nLastUsedScCol, nLastUsedScRow, nScTab );
    GetAddressConverter().ValidateRange( aUsedRange, true );
    nLastUsedScRow = aUsedRange.aEnd.Row();

    // first row without any set attributes (height/hidden/...)
    SCROW nFirstUnflaggedScRow = rDoc.GetLastFlaggedRow( nScTab ) + 1;

    // find range of outlines
    SCROW nFirstUngroupedScRow = 0;
    if( const ScOutlineTable* pOutlineTable = rDoc.GetOutlineTable( nScTab ) )
    {
        SCCOLROW nScStartPos, nScEndPos;
        const ScOutlineArray& rRowArray = pOutlineTable->GetRowArray();
        rRowArray.GetRange( nScStartPos, nScEndPos );
        // +1 because open/close button is in next row in Excel, +1 for "end->first unused"
        nFirstUngroupedScRow = static_cast< SCROW >( nScEndPos + 2 );
    }

    // column settings
    /*  #i30411# Files saved with SO7/OOo1.x with nonstandard default column
        formatting cause big Excel files, because all rows from row 1 to row
        32000 are exported. Now, if the used area goes exactly to row 32000,
        use this row as default and ignore all rows >32000.
        #i59220# Tolerance of +-128 rows for inserted/removed rows. */
    if( (31871 <= nLastUsedScRow) && (nLastUsedScRow <= 32127) && (nFirstUnflaggedScRow < nLastUsedScRow) && (nFirstUngroupedScRow <= nLastUsedScRow) )
        nMaxScRow = nLastUsedScRow;
    maColInfoBfr.Initialize( nMaxScRow );

    // range for cell iterator
    SCCOL nLastIterScCol = nMaxScCol;
    SCROW nLastIterScRow = ulimit_cast< SCROW >( nLastUsedScRow, nMaxScRow );
    ScUsedAreaIterator aIt( rDoc, nScTab, 0, 0, nLastIterScCol, nLastIterScRow );

    // activate the correct segment and sub segment at the progress bar
    GetProgressBar().ActivateCreateRowsSegment();

    for( bool bIt = aIt.GetNext(); bIt; bIt = aIt.GetNext() )
    {
        SCCOL nScCol = aIt.GetStartCol();
        SCROW nScRow = aIt.GetRow();
        SCCOL nLastScCol = aIt.GetEndCol();
        ScAddress aScPos( nScCol, nScRow, nScTab );

        XclAddress aXclPos( static_cast< sal_uInt16 >( nScCol ), static_cast< sal_uInt32 >( nScRow ) );
        sal_uInt16 nLastXclCol = static_cast< sal_uInt16 >( nLastScCol );

        const ScRefCellValue& rScCell = aIt.GetCell();
        XclExpCellRef xCell;

        const ScPatternAttr* pPattern = aIt.GetPattern();

        // handle overlapped merged cells before creating the cell record
        sal_uInt32 nMergeBaseXFId = EXC_XFID_NOTFOUND;
        bool bIsMergedBase = false;
        if( pPattern )
        {
            const SfxItemSet& rItemSet = pPattern->GetItemSet();
            // base cell in a merged range
            const ScMergeAttr& rMergeItem = rItemSet.Get( ATTR_MERGE );
            bIsMergedBase = rMergeItem.IsMerged();
            /*  overlapped cell in a merged range; in Excel all merged cells
                must contain same XF index, for correct border */
            const ScMergeFlagAttr& rMergeFlagItem = rItemSet.Get( ATTR_MERGE_FLAG );
            if( rMergeFlagItem.IsOverlapped() )
                nMergeBaseXFId = mxMergedcells->GetBaseXFId( aScPos );
        }

        OUString aAddNoteText;    // additional text to be appended to a note

        switch (rScCell.getType())
        {
            case CELLTYPE_VALUE:
            {
                double fValue = rScCell.getDouble();

                if (pPattern)
                {
                    OUString aUrl = pPattern->GetItem(ATTR_HYPERLINK).GetValue();
                    if (!aUrl.isEmpty())
                    {
                        rtl::Reference<XclExpHyperlink> aLink =
                            new XclExpHyperlink(GetRoot(), SvxURLField(aUrl, aUrl), aScPos);
                        mxHyperlinkList->AppendRecord(aLink);
                    }
                }

                // try to create a Boolean cell
                if( pPattern && ((fValue == 0.0) || (fValue == 1.0)) )
                {
                    sal_uInt32 nScNumFmt = pPattern->GetItem( ATTR_VALUE_FORMAT ).GetValue();
                    if( rFormatter.GetType( nScNumFmt ) == SvNumFormatType::LOGICAL )
                        xCell = new XclExpBooleanCell(
                            GetRoot(), aXclPos, pPattern, nMergeBaseXFId, fValue != 0.0 );
                }

                // try to create an RK value (compressed floating-point number)
                sal_Int32 nRkValue;
                if( !xCell && XclTools::GetRKFromDouble( nRkValue, fValue ) )
                    xCell = new XclExpRkCell(
                        GetRoot(), aXclPos, pPattern, nMergeBaseXFId, nRkValue );

                // else: simple floating-point number cell
                if( !xCell )
                    xCell = new XclExpNumberCell(
                        GetRoot(), aXclPos, pPattern, nMergeBaseXFId, fValue );
            }
            break;

            case CELLTYPE_STRING:
            {
                xCell = new XclExpLabelCell(
                    GetRoot(), aXclPos, pPattern, nMergeBaseXFId, rScCell.getSharedString()->getString());
            }
            break;

            case CELLTYPE_EDIT:
            {
                XclExpHyperlinkHelper aLinkHelper( GetRoot(), aScPos );
                xCell = new XclExpLabelCell(
                    GetRoot(), aXclPos, pPattern, nMergeBaseXFId, rScCell.getEditText(), aLinkHelper);

                // add a single created HLINK record to the record list
                if( aLinkHelper.HasLinkRecord() )
                    mxHyperlinkList->AppendRecord( aLinkHelper.GetLinkRecord() );
                // add list of multiple URLs to the additional cell note text
                if( aLinkHelper.HasMultipleUrls() )
                    aAddNoteText = ScGlobal::addToken( aAddNoteText, aLinkHelper.GetUrlList(), '\n', 2 );
            }
            break;

            case CELLTYPE_FORMULA:
            {
                if (pPattern)
                {
                    OUString aUrl = pPattern->GetItem(ATTR_HYPERLINK).GetValue();
                    if (!aUrl.isEmpty())
                    {
                        rtl::Reference<XclExpHyperlink> aLink =
                            new XclExpHyperlink(GetRoot(), SvxURLField(aUrl, aUrl), aScPos);
                        mxHyperlinkList->AppendRecord(aLink);
                    }
                }

                xCell = new XclExpFormulaCell(
                    GetRoot(), aXclPos, pPattern, nMergeBaseXFId,
                    *rScCell.getFormula(), maArrayBfr, maShrfmlaBfr, maTableopBfr);
            }
            break;

            default:
                OSL_FAIL( "XclExpCellTable::XclExpCellTable - unknown cell type" );
                [[fallthrough]];
            case CELLTYPE_NONE:
            {
                xCell = new XclExpBlankCell(
                    GetRoot(), aXclPos, nLastXclCol, pPattern, nMergeBaseXFId );
            }
            break;
        }

        assert(xCell && "can only reach here with xCell set");

        // insert the cell into the current row
        maRowBfr.AppendCell( xCell, bIsMergedBase );

        if ( !aAddNoteText.isEmpty()  )
            mxNoteList->AppendNewRecord( new XclExpNote( GetRoot(), aScPos, nullptr, aAddNoteText ) );

        // other sheet contents
        if( pPattern )
        {
            const SfxItemSet& rItemSet = pPattern->GetItemSet();

            // base cell in a merged range
            if( bIsMergedBase )
            {
                const ScMergeAttr& rMergeItem = rItemSet.Get( ATTR_MERGE );
                ScRange aScRange( aScPos );
                aScRange.aEnd.IncCol( rMergeItem.GetColMerge() - 1 );
                aScRange.aEnd.IncRow( rMergeItem.GetRowMerge() - 1 );
                sal_uInt32 nXFId = xCell->GetFirstXFId();
                // blank cells merged vertically may occur repeatedly
                OSL_ENSURE( (aScRange.aStart.Col() == aScRange.aEnd.Col()) || (nScCol == nLastScCol),
                    "XclExpCellTable::XclExpCellTable - invalid repeated blank merged cell" );
                for( SCCOL nIndex = nScCol; nIndex <= nLastScCol; ++nIndex )
                {
                    mxMergedcells->AppendRange( aScRange, nXFId );
                    aScRange.aStart.IncCol();
                    aScRange.aEnd.IncCol();
                }
            }

            // data validation
            if( ScfTools::CheckItem( rItemSet, ATTR_VALIDDATA, false ) )
            {
                sal_uInt32 nScHandle = rItemSet.Get( ATTR_VALIDDATA ).GetValue();
                ScRange aScRange( aScPos );
                aScRange.aEnd.SetCol( nLastScCol );
                mxDval->InsertCellRange( aScRange, nScHandle );
            }
        }
    }

    // create missing row settings for rows anyhow flagged or with outlines
    maRowBfr.CreateRows( ::std::max( nFirstUnflaggedScRow, nFirstUngroupedScRow ) );
}

void XclExpCellTable::Finalize(bool bXLS)
{
    // Finalize multiple operations.
    maTableopBfr.Finalize();

    /*  Finalize column buffer. This calculates column default XF indexes from
        the XF identifiers and fills a vector with these XF indexes. */
    ScfUInt16Vec aColXFIndexes;
    maColInfoBfr.Finalize( aColXFIndexes, bXLS );

    // Usually many indexes towards the end will be EXC_XF_DEFAULTCELL, find
    // the index that starts all EXC_XF_DEFAULTCELL until the end.
    size_t nStartColAllDefault = findFirstAllSameUntilEnd( aColXFIndexes, EXC_XF_DEFAULTCELL );

    /*  Finalize row buffer. This calculates all cell XF indexes from the XF
        identifiers. Then the XF index vector aColXFIndexes (filled above) is
        used to calculate the row default formats. With this, all unneeded blank
        cell records (equal to row default or column default) will be removed.
        The function returns the (most used) default row format in aDefRowData. */
    XclExpDefaultRowData aDefRowData;
    maRowBfr.Finalize( aDefRowData, aColXFIndexes, nStartColAllDefault );

    // Initialize the DEFROWHEIGHT record.
    mxDefrowheight->SetDefaultData( aDefRowData );
}

XclExpRecordRef XclExpCellTable::CreateRecord( sal_uInt16 nRecId ) const
{
    XclExpRecordRef xRec;
    switch( nRecId )
    {
        case EXC_ID3_DIMENSIONS:    xRec = new XclExpDelegatingRecord( &const_cast<XclExpRowBuffer*>(&maRowBfr)->GetDimensions() );   break;
        case EXC_ID2_DEFROWHEIGHT:  xRec = mxDefrowheight;  break;
        case EXC_ID_GUTS:           xRec = mxGuts;          break;
        case EXC_ID_NOTE:           xRec = mxNoteList;      break;
        case EXC_ID_MERGEDCELLS:    xRec = mxMergedcells;   break;
        case EXC_ID_HLINK:          xRec = mxHyperlinkList; break;
        case EXC_ID_DVAL:           xRec = mxDval;          break;
        case EXC_ID_EXTLST:         xRec = mxExtLst;        break;
        default:    OSL_FAIL( "XclExpCellTable::CreateRecord - unknown record id" );
    }
    return xRec;
}

void XclExpCellTable::Save( XclExpStream& rStrm )
{
    // DEFCOLWIDTH and COLINFOs
    maColInfoBfr.Save( rStrm );
    // ROWs and cell records
    maRowBfr.Save( rStrm );
}

void XclExpCellTable::SaveXml( XclExpXmlStream& rStrm )
{
    // DEFAULT row height
    XclExpDefaultRowData& rDefData = mxDefrowheight->GetDefaultData();
    sax_fastparser::FSHelperPtr& rWorksheet = rStrm.GetCurrentStream();
    rWorksheet->startElement( XML_sheetFormatPr,
        // OOXTODO: XML_baseColWidth
        XML_defaultColWidth, OString::number(maColInfoBfr.GetDefColWidth()),
        // OOXTODO: XML_thickTop
        // OOXTODO: XML_thickBottom
        XML_defaultRowHeight, OString::number(static_cast<double> (rDefData.mnHeight) / 20.0),
        XML_customHeight, ToPsz(true),
        XML_zeroHeight, ToPsz( rDefData.IsHidden() ),
        XML_outlineLevelRow, OString::number(maRowBfr.GetHighestOutlineLevel()),
        XML_outlineLevelCol, OString::number(maColInfoBfr.GetHighestOutlineLevel()) );
    rWorksheet->endElement( XML_sheetFormatPr );

    maColInfoBfr.SaveXml( rStrm );
    maRowBfr.SaveXml( rStrm );
    mxExtLst->SaveXml( rStrm );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
