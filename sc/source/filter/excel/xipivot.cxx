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

#include <xipivot.hxx>

#include <com/sun/star/sheet/DataPilotFieldSortInfo.hpp>
#include <com/sun/star/sheet/DataPilotFieldAutoShowInfo.hpp>
#include <com/sun/star/sheet/DataPilotFieldLayoutInfo.hpp>
#include <com/sun/star/sheet/DataPilotFieldReference.hpp>
#include <com/sun/star/sheet/DataPilotFieldReferenceItemType.hpp>

#include <tools/datetime.hxx>
#include <svl/intitem.hxx>
#include <svl/numformat.hxx>
#include <sal/log.hxx>
#include <sot/storage.hxx>
#include <comphelper/configuration.hxx>

#include <document.hxx>
#include <formulacell.hxx>
#include <dpsave.hxx>
#include <dpdimsave.hxx>
#include <dpobject.hxx>
#include <dpshttab.hxx>
#include <dpoutputgeometry.hxx>
#include <scitems.hxx>
#include <attrib.hxx>

#include <xltracer.hxx>
#include <xistream.hxx>
#include <xihelper.hxx>
#include <xilink.hxx>
#include <xiescher.hxx>

//TODO ExcelToSc usage
#include <excform.hxx>
#include <documentimport.hxx>

#include <vector>

using namespace com::sun::star;

using ::com::sun::star::sheet::DataPilotFieldOrientation_DATA;
using ::com::sun::star::sheet::DataPilotFieldSortInfo;
using ::com::sun::star::sheet::DataPilotFieldAutoShowInfo;
using ::com::sun::star::sheet::DataPilotFieldLayoutInfo;
using ::com::sun::star::sheet::DataPilotFieldReference;
using ::std::vector;

// Pivot cache

XclImpPCItem::XclImpPCItem( XclImpStream& rStrm )
{
    switch( rStrm.GetRecId() )
    {
        case EXC_ID_SXDOUBLE:   ReadSxdouble( rStrm );      break;
        case EXC_ID_SXBOOLEAN:  ReadSxboolean( rStrm );     break;
        case EXC_ID_SXERROR:    ReadSxerror( rStrm );       break;
        case EXC_ID_SXINTEGER:  ReadSxinteger( rStrm );     break;
        case EXC_ID_SXSTRING:   ReadSxstring( rStrm );      break;
        case EXC_ID_SXDATETIME: ReadSxdatetime( rStrm );    break;
        case EXC_ID_SXEMPTY:    ReadSxempty( rStrm );       break;
        default:    OSL_FAIL( "XclImpPCItem::XclImpPCItem - unknown record id" );
    }
}

namespace {

void lclSetValue( XclImpRoot& rRoot, const ScAddress& rScPos, double fValue, SvNumFormatType nFormatType )
{
    ScDocumentImport& rDoc = rRoot.GetDocImport();
    rDoc.setNumericCell(rScPos, fValue);
    sal_uInt32 nScNumFmt = rRoot.GetFormatter().GetStandardFormat( nFormatType, rRoot.GetDocLanguage() );
    rDoc.getDoc().ApplyAttr(
        rScPos.Col(), rScPos.Row(), rScPos.Tab(), SfxUInt32Item(ATTR_VALUE_FORMAT, nScNumFmt));
}

} // namespace

void XclImpPCItem::WriteToSource( XclImpRoot& rRoot, const ScAddress& rScPos ) const
{
    ScDocumentImport& rDoc = rRoot.GetDocImport();
    if( const OUString* pText = GetText() )
        rDoc.setStringCell(rScPos, *pText);
    else if( const double* pfValue = GetDouble() )
        rDoc.setNumericCell(rScPos, *pfValue);
    else if( const sal_Int16* pnValue = GetInteger() )
        rDoc.setNumericCell(rScPos, *pnValue);
    else if( const bool* pbValue = GetBool() )
        lclSetValue( rRoot, rScPos, *pbValue ? 1.0 : 0.0, SvNumFormatType::LOGICAL );
    else if( const DateTime* pDateTime = GetDateTime() )
    {
        // set number format date, time, or date/time, depending on the value
        double fValue = rRoot.GetDoubleFromDateTime( *pDateTime );
        double fInt = 0.0;
        double fFrac = modf( fValue, &fInt );
        SvNumFormatType nFormatType = ((fFrac == 0.0) && (fInt != 0.0)) ? SvNumFormatType::DATE :
            ((fInt == 0.0) ? SvNumFormatType::TIME : SvNumFormatType::DATETIME);
        lclSetValue( rRoot, rScPos, fValue, nFormatType );
    }
    else if( const sal_uInt16* pnError = GetError() )
    {
        double fValue;
        sal_uInt8 nErrCode = static_cast< sal_uInt8 >( *pnError );
        std::unique_ptr<ScTokenArray> pScTokArr = rRoot.GetOldFmlaConverter().GetBoolErr(
            XclTools::ErrorToEnum( fValue, true, nErrCode ) );
        ScFormulaCell* pCell = pScTokArr
            ? new ScFormulaCell(rDoc.getDoc(), rScPos, std::move(pScTokArr))
            : new ScFormulaCell(rDoc.getDoc(), rScPos);
        pCell->SetHybridDouble( fValue );
        rDoc.setFormulaCell(rScPos, pCell);
    }
}

void XclImpPCItem::ReadSxdouble( XclImpStream& rStrm )
{
    OSL_ENSURE( rStrm.GetRecSize() == 8, "XclImpPCItem::ReadSxdouble - wrong record size" );
    SetDouble( rStrm.ReadDouble() );
}

void XclImpPCItem::ReadSxboolean( XclImpStream& rStrm )
{
    OSL_ENSURE( rStrm.GetRecSize() == 2, "XclImpPCItem::ReadSxboolean - wrong record size" );
    SetBool( rStrm.ReaduInt16() != 0 );
}

void XclImpPCItem::ReadSxerror( XclImpStream& rStrm )
{
    OSL_ENSURE( rStrm.GetRecSize() == 2, "XclImpPCItem::ReadSxerror - wrong record size" );
    SetError( rStrm.ReaduInt16() );
}

void XclImpPCItem::ReadSxinteger( XclImpStream& rStrm )
{
    OSL_ENSURE( rStrm.GetRecSize() == 2, "XclImpPCItem::ReadSxinteger - wrong record size" );
    SetInteger( rStrm.ReadInt16() );
}

void XclImpPCItem::ReadSxstring( XclImpStream& rStrm )
{
    OSL_ENSURE( rStrm.GetRecSize() >= 3, "XclImpPCItem::ReadSxstring - wrong record size" );
    SetText( rStrm.ReadUniString() );
}

void XclImpPCItem::ReadSxdatetime( XclImpStream& rStrm )
{
    OSL_ENSURE( rStrm.GetRecSize() == 8, "XclImpPCItem::ReadSxdatetime - wrong record size" );
    sal_uInt16 nYear, nMonth;
    sal_uInt8 nDay, nHour, nMin, nSec;
    nYear = rStrm.ReaduInt16();
    nMonth = rStrm.ReaduInt16();
    nDay = rStrm.ReaduInt8();
    nHour = rStrm.ReaduInt8();
    nMin = rStrm.ReaduInt8();
    nSec = rStrm.ReaduInt8();
    SetDateTime( DateTime( Date( nDay, nMonth, nYear ), tools::Time( nHour, nMin, nSec ) ) );
}

void XclImpPCItem::ReadSxempty( XclImpStream& rStrm )
{
    OSL_ENSURE( rStrm.GetRecSize() == 0, "XclImpPCItem::ReadSxempty - wrong record size" );
    SetEmpty();
}

XclImpPCField::XclImpPCField( const XclImpRoot& rRoot, XclImpPivotCache& rPCache, sal_uInt16 nFieldIdx ) :
    XclPCField( EXC_PCFIELD_UNKNOWN, nFieldIdx ),
    XclImpRoot( rRoot ),
    mrPCache( rPCache ),
    mnSourceScCol( -1 ),
    mbNumGroupInfoRead( false )
{
}

XclImpPCField::~XclImpPCField()
{
}

// general field/item access --------------------------------------------------

const OUString& XclImpPCField::GetFieldName( const ScfStringVec& rVisNames ) const
{
    if( IsGroupChildField() && (mnFieldIdx < rVisNames.size()) )
    {
        const OUString& rVisName = rVisNames[ mnFieldIdx ];
        if (!rVisName.isEmpty())
            return rVisName;
    }
    return maFieldInfo.maName;
}

const XclImpPCField* XclImpPCField::GetGroupBaseField() const
{
    OSL_ENSURE( IsGroupChildField(), "XclImpPCField::GetGroupBaseField - this field type does not have a base field" );
    return IsGroupChildField() ? mrPCache.GetField( maFieldInfo.mnGroupBase ) : nullptr;
}

const XclImpPCItem* XclImpPCField::GetItem( sal_uInt16 nItemIdx ) const
{
    return (nItemIdx < maItems.size()) ? maItems[ nItemIdx ].get() : nullptr;
}

const XclImpPCItem* XclImpPCField::GetLimitItem( sal_uInt16 nItemIdx ) const
{
    OSL_ENSURE( nItemIdx < 3, "XclImpPCField::GetLimitItem - invalid item index" );
    OSL_ENSURE( nItemIdx < maNumGroupItems.size(), "XclImpPCField::GetLimitItem - no item found" );
    return (nItemIdx < maNumGroupItems.size()) ? maNumGroupItems[ nItemIdx ].get() : nullptr;
}

void XclImpPCField::WriteFieldNameToSource( SCCOL nScCol, SCTAB nScTab )
{
    OSL_ENSURE( HasOrigItems(), "XclImpPCField::WriteFieldNameToSource - only for standard fields" );
    GetDocImport().setStringCell(ScAddress(nScCol, 0, nScTab), maFieldInfo.maName);
    mnSourceScCol = nScCol;
}

void XclImpPCField::WriteOrigItemToSource( SCROW nScRow, SCTAB nScTab, sal_uInt16 nItemIdx )
{
    if( nItemIdx < maOrigItems.size() )
        maOrigItems[ nItemIdx ]->WriteToSource( GetRoot(), ScAddress( mnSourceScCol, nScRow, nScTab ) );
}

void XclImpPCField::WriteLastOrigItemToSource( SCROW nScRow, SCTAB nScTab )
{
    if( !maOrigItems.empty() )
        maOrigItems.back()->WriteToSource( GetRoot(), ScAddress( mnSourceScCol, nScRow, nScTab ) );
}

// records --------------------------------------------------------------------

void XclImpPCField::ReadSxfield( XclImpStream& rStrm )
{
    rStrm >> maFieldInfo;

    /*  Detect the type of this field. This is done very restrictive to detect
        any unexpected state. */
    meFieldType = EXC_PCFIELD_UNKNOWN;

    bool bItems  = ::get_flag( maFieldInfo.mnFlags, EXC_SXFIELD_HASITEMS );
    bool bPostp  = ::get_flag( maFieldInfo.mnFlags, EXC_SXFIELD_POSTPONE );
    bool bCalced = ::get_flag( maFieldInfo.mnFlags, EXC_SXFIELD_CALCED );
    bool bChild  = ::get_flag( maFieldInfo.mnFlags, EXC_SXFIELD_HASCHILD );
    bool bNum    = ::get_flag( maFieldInfo.mnFlags, EXC_SXFIELD_NUMGROUP );

    sal_uInt16 nVisC   = maFieldInfo.mnVisItems;
    sal_uInt16 nGroupC = maFieldInfo.mnGroupItems;
    sal_uInt16 nBaseC  = maFieldInfo.mnBaseItems;
    sal_uInt16 nOrigC  = maFieldInfo.mnOrigItems;
    OSL_ENSURE( nVisC > 0, "XclImpPCField::ReadSxfield - field without visible items" );

    sal_uInt16 nType = maFieldInfo.mnFlags & EXC_SXFIELD_DATA_MASK;
    bool bType =
        (nType == EXC_SXFIELD_DATA_STR) ||
        (nType == EXC_SXFIELD_DATA_INT) ||
        (nType == EXC_SXFIELD_DATA_DBL) ||
        (nType == EXC_SXFIELD_DATA_STR_INT) ||
        (nType == EXC_SXFIELD_DATA_STR_DBL) ||
        (nType == EXC_SXFIELD_DATA_DATE) ||
        (nType == EXC_SXFIELD_DATA_DATE_EMP) ||
        (nType == EXC_SXFIELD_DATA_DATE_NUM) ||
        (nType == EXC_SXFIELD_DATA_DATE_STR);
    bool bTypeNone =
        (nType == EXC_SXFIELD_DATA_NONE);
    // for now, ignore data type of calculated fields
    OSL_ENSURE( bCalced || bType || bTypeNone, "XclImpPCField::ReadSxfield - unknown item data type" );

    if( !(nVisC > 0 || bPostp) )
        return;

    if( bItems && !bPostp )
    {
        if( !bCalced )
        {
            // 1) standard fields and standard grouping fields
            if( !bNum )
            {
                // 1a) standard field without grouping
                if( bType && (nGroupC == 0) && (nBaseC == 0) && (nOrigC == nVisC) )
                    meFieldType = EXC_PCFIELD_STANDARD;

                // 1b) standard grouping field
                else if( bTypeNone && (nGroupC == nVisC) && (nBaseC > 0) && (nOrigC == 0) )
                    meFieldType = EXC_PCFIELD_STDGROUP;
            }
            // 2) numerical grouping fields
            else if( (nGroupC == nVisC) && (nBaseC == 0) )
            {
                // 2a) single num/date grouping field without child grouping field
                if( !bChild && bType && (nOrigC > 0) )
                {
                    switch( nType )
                    {
                        case EXC_SXFIELD_DATA_INT:
                        case EXC_SXFIELD_DATA_DBL:  meFieldType = EXC_PCFIELD_NUMGROUP;     break;
                        case EXC_SXFIELD_DATA_DATE: meFieldType = EXC_PCFIELD_DATEGROUP;    break;
                        default:    OSL_FAIL( "XclImpPCField::ReadSxfield - numeric group with wrong data type" );
                    }
                }

                // 2b) first date grouping field with child grouping field
                else if( bChild && (nType == EXC_SXFIELD_DATA_DATE) && (nOrigC > 0) )
                    meFieldType = EXC_PCFIELD_DATEGROUP;

                // 2c) additional date grouping field
                else if( bTypeNone && (nOrigC == 0) )
                    meFieldType = EXC_PCFIELD_DATECHILD;
            }
            OSL_ENSURE( meFieldType != EXC_PCFIELD_UNKNOWN, "XclImpPCField::ReadSxfield - invalid standard or grouped field" );
        }

        // 3) calculated field
        else
        {
            if( !bChild && !bNum && (nGroupC == 0) && (nBaseC == 0) && (nOrigC == 0) )
                meFieldType = EXC_PCFIELD_CALCED;
            OSL_ENSURE( meFieldType == EXC_PCFIELD_CALCED, "XclImpPCField::ReadSxfield - invalid calculated field" );
        }
    }

    else if( !bItems && bPostp )
    {
        // 4) standard field with postponed items
        if( !bCalced && !bChild && !bNum && bType && (nGroupC == 0) && (nBaseC == 0) && (nOrigC == 0) )
            meFieldType = EXC_PCFIELD_STANDARD;
        OSL_ENSURE( meFieldType == EXC_PCFIELD_STANDARD, "XclImpPCField::ReadSxfield - invalid postponed field" );
    }
}

void XclImpPCField::ReadItem( XclImpStream& rStrm )
{
    OSL_ENSURE( HasInlineItems() || HasPostponedItems(), "XclImpPCField::ReadItem - field does not expect items" );

    // read the item
    XclImpPCItemRef xItem = std::make_shared<XclImpPCItem>( rStrm );

    // try to insert into an item list
    if( mbNumGroupInfoRead )
    {
        // there are 3 items after SXNUMGROUP that contain grouping limits and step count
        if( maNumGroupItems.size() < 3 )
            maNumGroupItems.push_back( xItem );
        else
            maOrigItems.push_back( xItem );
    }
    else if( HasInlineItems() || HasPostponedItems() )
    {
        maItems.push_back( xItem );
        // visible item is original item in standard fields
        if( IsStandardField() )
            maOrigItems.push_back( xItem );
    }
}

void XclImpPCField::ReadSxnumgroup( XclImpStream& rStrm )
{
    OSL_ENSURE( IsNumGroupField() || IsDateGroupField(), "XclImpPCField::ReadSxnumgroup - SXNUMGROUP outside numeric grouping field" );
    OSL_ENSURE( !mbNumGroupInfoRead, "XclImpPCField::ReadSxnumgroup - multiple SXNUMGROUP records" );
    OSL_ENSURE( maItems.size() == maFieldInfo.mnGroupItems, "XclImpPCField::ReadSxnumgroup - SXNUMGROUP out of record order" );
    rStrm >> maNumGroupInfo;
    mbNumGroupInfoRead = IsNumGroupField() || IsDateGroupField();
}

void XclImpPCField::ReadSxgroupinfo( XclImpStream& rStrm )
{
    OSL_ENSURE( IsStdGroupField(), "XclImpPCField::ReadSxgroupinfo - SXGROUPINFO outside grouping field" );
    OSL_ENSURE( maGroupOrder.empty(), "XclImpPCField::ReadSxgroupinfo - multiple SXGROUPINFO records" );
    OSL_ENSURE( maItems.size() == maFieldInfo.mnGroupItems, "XclImpPCField::ReadSxgroupinfo - SXGROUPINFO out of record order" );
    OSL_ENSURE( (rStrm.GetRecLeft() / 2) == maFieldInfo.mnBaseItems, "XclImpPCField::ReadSxgroupinfo - wrong SXGROUPINFO size" );
    maGroupOrder.clear();
    size_t nSize = rStrm.GetRecLeft() / 2;
    maGroupOrder.resize( nSize, 0 );
    for( size_t nIdx = 0; nIdx < nSize; ++nIdx )
        maGroupOrder[ nIdx ] = rStrm.ReaduInt16();
}

// grouping -------------------------------------------------------------------

void XclImpPCField::ConvertGroupField( ScDPSaveData& rSaveData, const ScfStringVec& rVisNames ) const
{
    if (!GetFieldName(rVisNames).isEmpty())
    {
        if( IsStdGroupField() )
            ConvertStdGroupField( rSaveData, rVisNames );
        else if( IsNumGroupField() )
            ConvertNumGroupField( rSaveData, rVisNames );
        else if( IsDateGroupField() )
            ConvertDateGroupField( rSaveData, rVisNames );
    }
}

// private --------------------------------------------------------------------

void XclImpPCField::ConvertStdGroupField( ScDPSaveData& rSaveData, const ScfStringVec& rVisNames ) const
{
    const XclImpPCField* pBaseField = GetGroupBaseField();
    if(!pBaseField)
        return;

    const OUString& rBaseFieldName = pBaseField->GetFieldName( rVisNames );
    if( rBaseFieldName.isEmpty() )
        return;

    // *** create a ScDPSaveGroupItem for each own item, they collect base item names ***
    ScDPSaveGroupItemVec aGroupItems;
    aGroupItems.reserve( maItems.size() );
    // initialize with own item names
    for( const auto& rxItem : maItems )
        aGroupItems.emplace_back( rxItem->ConvertToText() );

    // *** iterate over all base items, set their names at corresponding own items ***
    for( sal_uInt16 nItemIdx = 0, nItemCount = static_cast< sal_uInt16 >( maGroupOrder.size() ); nItemIdx < nItemCount; ++nItemIdx )
        if( maGroupOrder[ nItemIdx ] < aGroupItems.size() )
            if( const XclImpPCItem* pBaseItem = pBaseField->GetItem( nItemIdx ) )
                if( const XclImpPCItem* pGroupItem = GetItem( maGroupOrder[ nItemIdx ] ) )
                    if( *pBaseItem != *pGroupItem )
                        aGroupItems[ maGroupOrder[ nItemIdx ] ].AddElement( pBaseItem->ConvertToText() );

    // *** create the ScDPSaveGroupDimension object, fill with grouping info ***
    ScDPSaveGroupDimension aGroupDim( rBaseFieldName, GetFieldName( rVisNames ) );
    for( const auto& rGroupItem : aGroupItems )
        if( !rGroupItem.IsEmpty() )
            aGroupDim.AddGroupItem( rGroupItem );
    rSaveData.GetDimensionData()->AddGroupDimension( aGroupDim );
}

void XclImpPCField::ConvertNumGroupField( ScDPSaveData& rSaveData, const ScfStringVec& rVisNames ) const
{
    ScDPNumGroupInfo aNumInfo( GetScNumGroupInfo() );
    ScDPSaveNumGroupDimension aNumGroupDim( GetFieldName( rVisNames ), aNumInfo );
    rSaveData.GetDimensionData()->AddNumGroupDimension( aNumGroupDim );
}

void XclImpPCField::ConvertDateGroupField( ScDPSaveData& rSaveData, const ScfStringVec& rVisNames ) const
{
    ScDPNumGroupInfo aDateInfo( GetScDateGroupInfo() );
    sal_Int32 nScDateType = maNumGroupInfo.GetScDateType();

    switch( meFieldType )
    {
        case EXC_PCFIELD_DATEGROUP:
        {
            if( aDateInfo.mbDateValues )
            {
                // special case for days only with step value - create numeric grouping
                ScDPSaveNumGroupDimension aNumGroupDim( GetFieldName( rVisNames ), aDateInfo );
                rSaveData.GetDimensionData()->AddNumGroupDimension( aNumGroupDim );
            }
            else
            {
                ScDPSaveNumGroupDimension aNumGroupDim( GetFieldName( rVisNames ), ScDPNumGroupInfo() );
                aNumGroupDim.SetDateInfo( aDateInfo, nScDateType );
                rSaveData.GetDimensionData()->AddNumGroupDimension( aNumGroupDim );
            }
        }
        break;

        case EXC_PCFIELD_DATECHILD:
        {
            if( const XclImpPCField* pBaseField = GetGroupBaseField() )
            {
                const OUString& rBaseFieldName = pBaseField->GetFieldName( rVisNames );
                if( !rBaseFieldName.isEmpty() )
                {
                    ScDPSaveGroupDimension aGroupDim( rBaseFieldName, GetFieldName( rVisNames ) );
                    aGroupDim.SetDateInfo( aDateInfo, nScDateType );
                    rSaveData.GetDimensionData()->AddGroupDimension( aGroupDim );
                }
            }
        }
        break;

        default:
            OSL_FAIL( "XclImpPCField::ConvertDateGroupField - unknown date field type" );
    }
}

ScDPNumGroupInfo XclImpPCField::GetScNumGroupInfo() const
{
    ScDPNumGroupInfo aNumInfo;
    aNumInfo.mbEnable = true;
    aNumInfo.mbDateValues = false;
    aNumInfo.mbAutoStart = true;
    aNumInfo.mbAutoEnd = true;

    if( const double* pfMinValue = GetNumGroupLimit( EXC_SXFIELD_INDEX_MIN ) )
    {
        aNumInfo.mfStart = *pfMinValue;
        aNumInfo.mbAutoStart = ::get_flag( maNumGroupInfo.mnFlags, EXC_SXNUMGROUP_AUTOMIN );
    }
    if( const double* pfMaxValue = GetNumGroupLimit( EXC_SXFIELD_INDEX_MAX ) )
    {
        aNumInfo.mfEnd = *pfMaxValue;
        aNumInfo.mbAutoEnd = ::get_flag( maNumGroupInfo.mnFlags, EXC_SXNUMGROUP_AUTOMAX );
    }
    if( const double* pfStepValue = GetNumGroupLimit( EXC_SXFIELD_INDEX_STEP ) )
        aNumInfo.mfStep = *pfStepValue;

    return aNumInfo;
}

ScDPNumGroupInfo XclImpPCField::GetScDateGroupInfo() const
{
    ScDPNumGroupInfo aDateInfo;
    aDateInfo.mbEnable = true;
    aDateInfo.mbDateValues = false;
    aDateInfo.mbAutoStart = true;
    aDateInfo.mbAutoEnd = true;

    if( const DateTime* pMinDate = GetDateGroupLimit( EXC_SXFIELD_INDEX_MIN ) )
    {
        aDateInfo.mfStart = GetDoubleFromDateTime( *pMinDate );
        aDateInfo.mbAutoStart = ::get_flag( maNumGroupInfo.mnFlags, EXC_SXNUMGROUP_AUTOMIN );
    }
    if( const DateTime* pMaxDate = GetDateGroupLimit( EXC_SXFIELD_INDEX_MAX ) )
    {
        aDateInfo.mfEnd = GetDoubleFromDateTime( *pMaxDate );
        aDateInfo.mbAutoEnd = ::get_flag( maNumGroupInfo.mnFlags, EXC_SXNUMGROUP_AUTOMAX );
    }
    // GetDateGroupStep() returns a value for date type "day" in single date groups only
    if( const sal_Int16* pnStepValue = GetDateGroupStep() )
    {
        aDateInfo.mfStep = *pnStepValue;
        aDateInfo.mbDateValues = true;
    }

    return aDateInfo;
}

const double* XclImpPCField::GetNumGroupLimit( sal_uInt16 nLimitIdx ) const
{
    OSL_ENSURE( IsNumGroupField(), "XclImpPCField::GetNumGroupLimit - only for numeric grouping fields" );
    if( const XclImpPCItem* pItem = GetLimitItem( nLimitIdx ) )
    {
        OSL_ENSURE( pItem->GetDouble(), "XclImpPCField::GetNumGroupLimit - SXDOUBLE item expected" );
        return pItem->GetDouble();
    }
    return nullptr;
}

const DateTime* XclImpPCField::GetDateGroupLimit( sal_uInt16 nLimitIdx ) const
{
    OSL_ENSURE( IsDateGroupField(), "XclImpPCField::GetDateGroupLimit - only for date grouping fields" );
    if( const XclImpPCItem* pItem = GetLimitItem( nLimitIdx ) )
    {
        OSL_ENSURE( pItem->GetDateTime(), "XclImpPCField::GetDateGroupLimit - SXDATETIME item expected" );
        return pItem->GetDateTime();
    }
    return nullptr;
}

const sal_Int16* XclImpPCField::GetDateGroupStep() const
{
    // only for single date grouping fields, not for grouping chains
    if( !IsGroupBaseField() && !IsGroupChildField() )
    {
        // only days may have a step value, return 0 for all other date types
        if( maNumGroupInfo.GetXclDataType() == EXC_SXNUMGROUP_TYPE_DAY )
        {
            if( const XclImpPCItem* pItem = GetLimitItem( EXC_SXFIELD_INDEX_STEP ) )
            {
                OSL_ENSURE( pItem->GetInteger(), "XclImpPCField::GetDateGroupStep - SXINTEGER item expected" );
                if( const sal_Int16* pnStep = pItem->GetInteger() )
                {
                    OSL_ENSURE( *pnStep > 0, "XclImpPCField::GetDateGroupStep - invalid step count" );
                    // return nothing for step count 1 - this is also a standard date group in Excel
                    return (*pnStep > 1) ? pnStep : nullptr;
                }
            }
        }
    }
    return nullptr;
}

XclImpPivotCache::XclImpPivotCache( const XclImpRoot& rRoot ) :
    XclImpRoot( rRoot ),
    maSrcRange( ScAddress::INITIALIZE_INVALID ),
    mnStrmId( 0 ),
    mnSrcType( EXC_SXVS_UNKNOWN ),
    mbSelfRef( false )
{
}

XclImpPivotCache::~XclImpPivotCache()
{
}

// data access ----------------------------------------------------------------

const XclImpPCField* XclImpPivotCache::GetField( sal_uInt16 nFieldIdx ) const
{
    return (nFieldIdx < maFields.size()) ? maFields[ nFieldIdx ].get() : nullptr;
}

// records --------------------------------------------------------------------

void XclImpPivotCache::ReadSxidstm( XclImpStream& rStrm )
{
    mnStrmId = rStrm.ReaduInt16();
}

void XclImpPivotCache::ReadSxvs( XclImpStream& rStrm )
{
    mnSrcType = rStrm.ReaduInt16();
    GetTracer().TracePivotDataSource( mnSrcType != EXC_SXVS_SHEET );
}

void XclImpPivotCache::ReadDconref( XclImpStream& rStrm )
{
    /*  Read DCONREF only once (by checking maTabName), there may be other
        DCONREF records in another context. Read reference only if a leading
        SXVS record is present (by checking mnSrcType). */
    if( !maTabName.isEmpty() || (mnSrcType != EXC_SXVS_SHEET) )
        return;

    XclRange aXclRange( ScAddress::UNINITIALIZED );
    aXclRange.Read( rStrm, false );
    OUString aEncUrl = rStrm.ReadUniString();

    XclImpUrlHelper::DecodeUrl( maUrl, maTabName, mbSelfRef, GetRoot(), aEncUrl );

    /*  Do not convert maTabName to Calc sheet name -> original name is used to
        find the sheet in the document. Sheet index of source range will be
        found later in XclImpPivotCache::ReadPivotCacheStream(), because sheet
        may not exist yet. */
    if( mbSelfRef )
        GetAddressConverter().ConvertRange( maSrcRange, aXclRange, 0, 0, true );
}

void XclImpPivotCache::ReadDConName( XclImpStream& rStrm )
{
    maSrcRangeName = rStrm.ReadUniString();

    // This 2-byte value equals the length of string that follows, or if 0 it
    // indicates that the name has a workbook scope.  For now, we only support
    // internal defined name with a workbook scope.
    sal_uInt16 nFlag;
    nFlag = rStrm.ReaduInt16();
    mbSelfRef = (nFlag == 0);

    if (!mbSelfRef)
        // External name is not supported yet.
        maSrcRangeName.clear();
}

void XclImpPivotCache::ReadPivotCacheStream( const XclImpStream& rStrm )
{
    if( (mnSrcType != EXC_SXVS_SHEET) && (mnSrcType != EXC_SXVS_EXTERN) )
        return;

    ScDocument& rDoc = GetDoc();
    SCCOL nFieldScCol = 0;              // column index of source data for next field
    SCROW nItemScRow = 0;               // row index of source data for current items
    SCTAB nScTab = 0;                   // sheet index of source data
    bool bGenerateSource = false;       // true = write source data from cache to dummy table

    if( mbSelfRef )
    {
        if (maSrcRangeName.isEmpty())
        {
            // try to find internal sheet containing the source data
            nScTab = GetTabInfo().GetScTabFromXclName( maTabName );
            if( rDoc.HasTable( nScTab ) )
            {
                // set sheet index to source range
                maSrcRange.aStart.SetTab( nScTab );
                maSrcRange.aEnd.SetTab( nScTab );
            }
            else
            {
                // create dummy sheet for deleted internal sheet
                bGenerateSource = true;
            }
        }
    }
    else
    {
        // create dummy sheet for external sheet
        bGenerateSource = true;
    }

    // create dummy sheet for source data from external or deleted sheet
    if( bGenerateSource )
    {
        if( rDoc.GetTableCount() >= MAXTABCOUNT )
            // cannot create more sheets -> exit
            return;

        nScTab = rDoc.GetTableCount();
        rDoc.MakeTable( nScTab );
        OUStringBuffer aDummyName("DPCache");
        if( maTabName.getLength() > 0 )
            aDummyName.append( "_" + maTabName );
        OUString aName = aDummyName.makeStringAndClear();
        rDoc.CreateValidTabName( aName );
        rDoc.RenameTab( nScTab, aName );
        // set sheet index to source range
        maSrcRange.aStart.SetTab( nScTab );
        maSrcRange.aEnd.SetTab( nScTab );
    }

    // open pivot cache storage stream
    rtl::Reference<SotStorage> xSvStrg = OpenStorage(EXC_STORAGE_PTCACHE);
    rtl::Reference<SotStorageStream> xSvStrm = OpenStream(xSvStrg, ScfTools::GetHexStr(mnStrmId));
    if( !xSvStrm.is() )
        return;

    // create Excel record stream object
    XclImpStream aPCStrm( *xSvStrm, GetRoot() );
    aPCStrm.CopyDecrypterFrom( rStrm );     // pivot cache streams are encrypted

    XclImpPCFieldRef xCurrField;    // current field for new items
    XclImpPCFieldVec aOrigFields;   // all standard fields with inline original  items
    XclImpPCFieldVec aPostpFields;  // all standard fields with postponed original items
    size_t nPostpIdx = 0;           // index to current field with postponed items
    bool bLoop = true;              // true = continue loop

    while( bLoop && aPCStrm.StartNextRecord() )
    {
        switch( aPCStrm.GetRecId() )
        {
            case EXC_ID_EOF:
                bLoop = false;
            break;

            case EXC_ID_SXDB:
                aPCStrm >> maPCInfo;
            break;

            case EXC_ID_SXFIELD:
            {
                xCurrField.reset();
                sal_uInt16 nNewFieldIdx = static_cast< sal_uInt16 >( maFields.size() );
                if( nNewFieldIdx < EXC_PC_MAXFIELDCOUNT )
                {
                    xCurrField = std::make_shared<XclImpPCField>( GetRoot(), *this, nNewFieldIdx );
                    maFields.push_back( xCurrField );
                    xCurrField->ReadSxfield( aPCStrm );
                    if( xCurrField->HasOrigItems() )
                    {
                        if( xCurrField->HasPostponedItems() )
                            aPostpFields.push_back( xCurrField );
                        else
                            aOrigFields.push_back( xCurrField );
                        // insert field name into generated source data, field remembers its column index
                        if( bGenerateSource && (nFieldScCol <= rDoc.MaxCol()) )
                            xCurrField->WriteFieldNameToSource( nFieldScCol++, nScTab );
                    }
                    // do not read items into invalid/postponed fields
                    if( !xCurrField->HasInlineItems() )
                        xCurrField.reset();
                }
            }
            break;

            case EXC_ID_SXINDEXLIST:
                // read index list and insert all items into generated source data
                if( bGenerateSource && (nItemScRow <= rDoc.MaxRow()) && (++nItemScRow <= rDoc.MaxRow()) )
                {
                    for( const auto& rxOrigField : aOrigFields )
                    {
                        sal_uInt16 nItemIdx = rxOrigField->Has16BitIndexes() ? aPCStrm.ReaduInt16() : aPCStrm.ReaduInt8();
                        rxOrigField->WriteOrigItemToSource( nItemScRow, nScTab, nItemIdx );
                    }
                }
                xCurrField.reset();
            break;

            case EXC_ID_SXDOUBLE:
            case EXC_ID_SXBOOLEAN:
            case EXC_ID_SXERROR:
            case EXC_ID_SXINTEGER:
            case EXC_ID_SXSTRING:
            case EXC_ID_SXDATETIME:
            case EXC_ID_SXEMPTY:
                if( xCurrField )                   // inline items
                {
                    xCurrField->ReadItem( aPCStrm );
                }
                else if( !aPostpFields.empty() )        // postponed items
                {
                    // read postponed item
                    aPostpFields[ nPostpIdx ]->ReadItem( aPCStrm );
                    // write item to source
                    if( bGenerateSource && (nItemScRow <= rDoc.MaxRow()) )
                    {
                        // start new row, if there are only postponed fields
                        if( aOrigFields.empty() && (nPostpIdx == 0) )
                            ++nItemScRow;
                        if( nItemScRow <= rDoc.MaxRow() )
                            aPostpFields[ nPostpIdx ]->WriteLastOrigItemToSource( nItemScRow, nScTab );
                    }
                    // get index of next postponed field
                    ++nPostpIdx;
                    if( nPostpIdx >= aPostpFields.size() )
                        nPostpIdx = 0;
                }
            break;

            case EXC_ID_SXNUMGROUP:
                if( xCurrField )
                    xCurrField->ReadSxnumgroup( aPCStrm );
            break;

            case EXC_ID_SXGROUPINFO:
                if( xCurrField )
                    xCurrField->ReadSxgroupinfo( aPCStrm );
            break;

            // known but ignored records
            case EXC_ID_SXRULE:
            case EXC_ID_SXFILT:
            case EXC_ID_00F5:
            case EXC_ID_SXNAME:
            case EXC_ID_SXPAIR:
            case EXC_ID_SXFMLA:
            case EXC_ID_SXFORMULA:
            case EXC_ID_SXDBEX:
            case EXC_ID_SXFDBTYPE:
            break;

            default:
                SAL_WARN("sc.filter",  "XclImpPivotCache::ReadPivotCacheStream - unknown record 0x" << std::hex << aPCStrm.GetRecId() );
        }
    }

    OSL_ENSURE( maPCInfo.mnTotalFields == maFields.size(),
        "XclImpPivotCache::ReadPivotCacheStream - field count mismatch" );

    if (static_cast<bool>(maPCInfo.mnFlags & EXC_SXDB_SAVEDATA))
    {
        SCROW nNewEnd = maSrcRange.aStart.Row() + maPCInfo.mnSrcRecs;
        maSrcRange.aEnd.SetRow(nNewEnd);
    }

    // set source range for external source data
    if( bGenerateSource && (nFieldScCol > 0) )
    {
        maSrcRange.aStart.SetCol( 0 );
        maSrcRange.aStart.SetRow( 0 );
        // nFieldScCol points to first unused column
        maSrcRange.aEnd.SetCol( nFieldScCol - 1 );
        // nItemScRow points to last used row
        maSrcRange.aEnd.SetRow( nItemScRow );
    }
}

bool XclImpPivotCache::IsRefreshOnLoad() const
{
    return static_cast<bool>(maPCInfo.mnFlags & EXC_SXDB_REFRESH_LOAD);
}

bool XclImpPivotCache::IsValid() const
{
    if (!maSrcRangeName.isEmpty())
        return true;

    return maSrcRange.IsValid();
}

// Pivot table

XclImpPTItem::XclImpPTItem( const XclImpPCField* pCacheField ) :
    mpCacheField( pCacheField )
{
}

const OUString* XclImpPTItem::GetItemName() const
{
    if( mpCacheField )
        if( const XclImpPCItem* pCacheItem = mpCacheField->GetItem( maItemInfo.mnCacheIdx ) )
            //TODO: use XclImpPCItem::ConvertToText(), if all conversions are available
            return pCacheItem->IsEmpty() ? nullptr : pCacheItem->GetText();
    return nullptr;
}

std::pair<bool, OUString> XclImpPTItem::GetItemName(const ScDPSaveDimension& rSaveDim, ScDPObject* pObj, const XclImpRoot& rRoot) const
{
    if(!mpCacheField)
        return std::pair<bool, OUString>(false, OUString());

    const XclImpPCItem* pCacheItem = mpCacheField->GetItem( maItemInfo.mnCacheIdx );
    if(!pCacheItem)
        return std::pair<bool, OUString>(false, OUString());

    OUString sItemName;
    if(pCacheItem->GetType() == EXC_PCITEM_TEXT || pCacheItem->GetType() == EXC_PCITEM_ERROR)
    {
        const OUString* pItemName = pCacheItem->GetText();
        if(!pItemName)
            return std::pair<bool, OUString>(false, OUString());
        sItemName = *pItemName;
    }
    else if (pCacheItem->GetType() == EXC_PCITEM_DOUBLE)
    {
        sItemName = pObj->GetFormattedString(rSaveDim.GetName(), *pCacheItem->GetDouble());
    }
    else if (pCacheItem->GetType() == EXC_PCITEM_INTEGER)
    {
        sItemName = pObj->GetFormattedString(rSaveDim.GetName(), static_cast<double>(*pCacheItem->GetInteger()));
    }
    else if (pCacheItem->GetType() == EXC_PCITEM_BOOL)
    {
        sItemName = pObj->GetFormattedString(rSaveDim.GetName(), static_cast<double>(*pCacheItem->GetBool()));
    }
    else if (pCacheItem->GetType() == EXC_PCITEM_DATETIME)
    {
        sItemName = pObj->GetFormattedString(rSaveDim.GetName(), rRoot.GetDoubleFromDateTime(*pCacheItem->GetDateTime()));
    }
    else if (pCacheItem->GetType() == EXC_PCITEM_EMPTY)
    {
        // sItemName is an empty string
    }
    else // EXC_PCITEM_INVALID
        return std::pair<bool, OUString>(false, OUString());

    return std::pair<bool, OUString>(true, sItemName);
}

void XclImpPTItem::ReadSxvi( XclImpStream& rStrm )
{
    rStrm >> maItemInfo;
}

void XclImpPTItem::ConvertItem( ScDPSaveDimension& rSaveDim, ScDPObject* pObj, const XclImpRoot& rRoot ) const
{
    // Find member and set properties
    std::pair<bool, OUString> aReturnedName = GetItemName(rSaveDim, pObj, rRoot);
    if(aReturnedName.first)
    {
        ScDPSaveMember* pMember = rSaveDim.GetExistingMemberByName(aReturnedName.second);
        if(pMember)
        {
            pMember->SetIsVisible( !::get_flag( maItemInfo.mnFlags, EXC_SXVI_HIDDEN ) );
            pMember->SetShowDetails( !::get_flag( maItemInfo.mnFlags, EXC_SXVI_HIDEDETAIL ) );
            if (maItemInfo.HasVisName())
                pMember->SetLayoutName(*maItemInfo.GetVisName());
        }
    }
}

XclImpPTField::XclImpPTField( const XclImpPivotTable& rPTable, sal_uInt16 nCacheIdx ) :
    mrPTable( rPTable )
{
    maFieldInfo.mnCacheIdx = nCacheIdx;
}

// general field/item access --------------------------------------------------

const XclImpPCField* XclImpPTField::GetCacheField() const
{
    XclImpPivotCacheRef xPCache = mrPTable.GetPivotCache();
    return xPCache ? xPCache->GetField( maFieldInfo.mnCacheIdx ) : nullptr;
}

OUString XclImpPTField::GetFieldName() const
{
    const XclImpPCField* pField = GetCacheField();
    return pField ? pField->GetFieldName( mrPTable.GetVisFieldNames() ) : OUString();
}

OUString XclImpPTField::GetVisFieldName() const
{
    const OUString* pVisName = maFieldInfo.GetVisName();
    return pVisName ? *pVisName : OUString();
}

const XclImpPTItem* XclImpPTField::GetItem( sal_uInt16 nItemIdx ) const
{
    return (nItemIdx < maItems.size()) ? maItems[ nItemIdx ].get() : nullptr;
}

const OUString* XclImpPTField::GetItemName( sal_uInt16 nItemIdx ) const
{
    const XclImpPTItem* pItem = GetItem( nItemIdx );
    return pItem ? pItem->GetItemName() : nullptr;
}

// records --------------------------------------------------------------------

void XclImpPTField::ReadSxvd( XclImpStream& rStrm )
{
    rStrm >> maFieldInfo;
}

void XclImpPTField::ReadSxvdex( XclImpStream& rStrm )
{
    rStrm >> maFieldExtInfo;
}

void XclImpPTField::ReadSxvi( XclImpStream& rStrm )
{
    XclImpPTItemRef xItem = std::make_shared<XclImpPTItem>( GetCacheField() );
    maItems.push_back( xItem );
    xItem->ReadSxvi( rStrm );
}

// row/column fields ----------------------------------------------------------

void XclImpPTField::ConvertRowColField( ScDPSaveData& rSaveData ) const
{
    OSL_ENSURE( maFieldInfo.mnAxes & EXC_SXVD_AXIS_ROWCOL, "XclImpPTField::ConvertRowColField - no row/column field" );
    // special data orientation field?
    if( maFieldInfo.mnCacheIdx == EXC_SXIVD_DATA )
        rSaveData.GetDataLayoutDimension()->SetOrientation( maFieldInfo.GetApiOrient( EXC_SXVD_AXIS_ROWCOL ) );
    else
        ConvertRCPField( rSaveData );
}

// page fields ----------------------------------------------------------------

void XclImpPTField::SetPageFieldInfo( const XclPTPageFieldInfo& rPageInfo )
{
    maPageInfo = rPageInfo;
}

void XclImpPTField::ConvertPageField( ScDPSaveData& rSaveData ) const
{
    OSL_ENSURE( maFieldInfo.mnAxes & EXC_SXVD_AXIS_PAGE, "XclImpPTField::ConvertPageField - no page field" );
    ConvertRCPField( rSaveData );
}

// hidden fields --------------------------------------------------------------

void XclImpPTField::ConvertHiddenField( ScDPSaveData& rSaveData ) const
{
    OSL_ENSURE( (maFieldInfo.mnAxes & EXC_SXVD_AXIS_ROWCOLPAGE) == 0, "XclImpPTField::ConvertHiddenField - field not hidden" );
    ConvertRCPField( rSaveData );
}

// data fields ----------------------------------------------------------------

bool XclImpPTField::HasDataFieldInfo() const
{
    return !maDataInfoVector.empty();
}

void XclImpPTField::AddDataFieldInfo( const XclPTDataFieldInfo& rDataInfo )
{
    OSL_ENSURE( maFieldInfo.mnAxes & EXC_SXVD_AXIS_DATA, "XclImpPTField::AddDataFieldInfo - no data field" );
    maDataInfoVector.push_back( rDataInfo );
}

void XclImpPTField::ConvertDataField( ScDPSaveData& rSaveData ) const
{
    OSL_ENSURE( maFieldInfo.mnAxes & EXC_SXVD_AXIS_DATA, "XclImpPTField::ConvertDataField - no data field" );
    OSL_ENSURE( !maDataInfoVector.empty(), "XclImpPTField::ConvertDataField - no data field info" );
    if (maDataInfoVector.empty())
        return;

    OUString aFieldName = GetFieldName();
    if (aFieldName.isEmpty())
        return;

    ScDPSaveDimension* pSaveDim = rSaveData.GetNewDimensionByName(aFieldName);
    if (!pSaveDim)
    {
        SAL_WARN("sc.filter","XclImpPTField::ConvertDataField - field name not found: " << aFieldName);
        return;
    }

    auto aIt = maDataInfoVector.begin(), aEnd = maDataInfoVector.end();

    ConvertDataField( *pSaveDim, *aIt );

    // multiple data fields -> clone dimension
    for( ++aIt; aIt != aEnd; ++aIt )
    {
        ScDPSaveDimension& rDupDim = rSaveData.DuplicateDimension( *pSaveDim );
        ConvertDataFieldInfo( rDupDim, *aIt );
    }
}

// private --------------------------------------------------------------------

/**
 * Convert Excel-encoded subtotal name to a Calc-encoded one.
 */
static OUString lcl_convertExcelSubtotalName(const OUString& rName)
{
    OUStringBuffer aBuf;
    const sal_Unicode* p = rName.getStr();
    sal_Int32 n = rName.getLength();
    for (sal_Int32 i = 0; i < n; ++i)
    {
        const sal_Unicode c = p[i];
        if (c == '\\')
        {
            aBuf.append(OUStringChar(c) + OUStringChar(c));
        }
        else
            aBuf.append(c);
    }
    return aBuf.makeStringAndClear();
}

void XclImpPTField::ConvertRCPField( ScDPSaveData& rSaveData ) const
{
    const OUString aFieldName = GetFieldName();
    if( aFieldName.isEmpty() )
        return;

    const XclImpPCField* pCacheField = GetCacheField();
    if( !pCacheField || !pCacheField->IsSupportedField() )
        return;

    ScDPSaveDimension* pTest = rSaveData.GetNewDimensionByName(aFieldName);
    if (!pTest)
        return;

    ScDPSaveDimension& rSaveDim = *pTest;

    // orientation
    rSaveDim.SetOrientation( maFieldInfo.GetApiOrient( EXC_SXVD_AXIS_ROWCOLPAGE ) );

    // visible name
    if (const OUString* pVisName = maFieldInfo.GetVisName())
        if (!pVisName->isEmpty())
            rSaveDim.SetLayoutName( *pVisName );

    // subtotal function(s)
    XclPTSubtotalVec aSubtotalVec;
    maFieldInfo.GetSubtotals( aSubtotalVec );
    if( !aSubtotalVec.empty() )
        rSaveDim.SetSubTotals( std::move(aSubtotalVec) );

    // sorting
    DataPilotFieldSortInfo aSortInfo;
    aSortInfo.Field = mrPTable.GetDataFieldName( maFieldExtInfo.mnSortField );
    aSortInfo.IsAscending = ::get_flag( maFieldExtInfo.mnFlags, EXC_SXVDEX_SORT_ASC );
    aSortInfo.Mode = maFieldExtInfo.GetApiSortMode();
    rSaveDim.SetSortInfo( &aSortInfo );

    // auto show
    DataPilotFieldAutoShowInfo aShowInfo;
    aShowInfo.IsEnabled = ::get_flag( maFieldExtInfo.mnFlags, EXC_SXVDEX_AUTOSHOW );
    aShowInfo.ShowItemsMode = maFieldExtInfo.GetApiAutoShowMode();
    aShowInfo.ItemCount = maFieldExtInfo.GetApiAutoShowCount();
    aShowInfo.DataField = mrPTable.GetDataFieldName( maFieldExtInfo.mnShowField );
    rSaveDim.SetAutoShowInfo( &aShowInfo );

    // layout
    DataPilotFieldLayoutInfo aLayoutInfo;
    aLayoutInfo.LayoutMode = maFieldExtInfo.GetApiLayoutMode();
    aLayoutInfo.AddEmptyLines = ::get_flag( maFieldExtInfo.mnFlags, EXC_SXVDEX_LAYOUT_BLANK );
    rSaveDim.SetLayoutInfo( &aLayoutInfo );

    // grouping info
    pCacheField->ConvertGroupField( rSaveData, mrPTable.GetVisFieldNames() );

    // custom subtotal name
    if (maFieldExtInfo.mpFieldTotalName)
    {
        OUString aSubName = lcl_convertExcelSubtotalName(*maFieldExtInfo.mpFieldTotalName);
        rSaveDim.SetSubtotalName(aSubName);
    }
}

void XclImpPTField::ConvertFieldInfo( const ScDPSaveData& rSaveData, ScDPObject* pObj, const XclImpRoot& rRoot, bool bPageField ) const
{
    const OUString aFieldName = GetFieldName();
    if( aFieldName.isEmpty() )
        return;

    const XclImpPCField* pCacheField = GetCacheField();
    if( !pCacheField || !pCacheField->IsSupportedField() )
        return;

    ScDPSaveDimension* pSaveDim = rSaveData.GetExistingDimensionByName(aFieldName);
    if (!pSaveDim)
        return;

    pSaveDim->SetShowEmpty( ::get_flag( maFieldExtInfo.mnFlags, EXC_SXVDEX_SHOWALL ) );
    for( const auto& rxItem : maItems )
        rxItem->ConvertItem( *pSaveDim, pObj, rRoot );

    if(bPageField && maPageInfo.mnSelItem != EXC_SXPI_ALLITEMS)
    {
        const XclImpPTItem* pItem = GetItem( maPageInfo.mnSelItem );
        if(pItem)
        {
            std::pair<bool, OUString> aReturnedName = pItem->GetItemName(*pSaveDim, pObj, rRoot);
            if(aReturnedName.first)
                pSaveDim->SetCurrentPage(&aReturnedName.second);
        }
    }
}

void XclImpPTField::ConvertDataField( ScDPSaveDimension& rSaveDim, const XclPTDataFieldInfo& rDataInfo ) const
{
    // orientation
    rSaveDim.SetOrientation( DataPilotFieldOrientation_DATA );
    // extended data field info
    ConvertDataFieldInfo( rSaveDim, rDataInfo );
}

void XclImpPTField::ConvertDataFieldInfo( ScDPSaveDimension& rSaveDim, const XclPTDataFieldInfo& rDataInfo ) const
{
    // visible name
    const OUString* pVisName = rDataInfo.GetVisName();
    if (pVisName && !pVisName->isEmpty())
        rSaveDim.SetLayoutName(*pVisName);

    // aggregation function
    rSaveDim.SetFunction( rDataInfo.GetApiAggFunc() );

    // result field reference
    sal_Int32 nRefType = rDataInfo.GetApiRefType();
    DataPilotFieldReference aFieldRef;
    aFieldRef.ReferenceType = nRefType;
    const XclImpPTField* pRefField = mrPTable.GetField(rDataInfo.mnRefField);
    if (pRefField)
    {
        aFieldRef.ReferenceField = pRefField->GetFieldName();
        aFieldRef.ReferenceItemType = rDataInfo.GetApiRefItemType();
        if (aFieldRef.ReferenceItemType == sheet::DataPilotFieldReferenceItemType::NAMED)
        {
            const OUString* pRefItemName = pRefField->GetItemName(rDataInfo.mnRefItem);
            if (pRefItemName)
                aFieldRef.ReferenceItemName = *pRefItemName;
        }
    }

    rSaveDim.SetReferenceValue(&aFieldRef);
}

XclImpPivotTable::XclImpPivotTable( const XclImpRoot& rRoot ) :
    XclImpRoot( rRoot ),
    maDataOrientField( *this, EXC_SXIVD_DATA ),
    mpDPObj(nullptr)
{
}

XclImpPivotTable::~XclImpPivotTable()
{
}

// cache/field access, misc. --------------------------------------------------

sal_uInt16 XclImpPivotTable::GetFieldCount() const
{
    return static_cast< sal_uInt16 >( maFields.size() );
}

const XclImpPTField* XclImpPivotTable::GetField( sal_uInt16 nFieldIdx ) const
{
    return (nFieldIdx == EXC_SXIVD_DATA) ? &maDataOrientField :
        ((nFieldIdx < maFields.size()) ? maFields[ nFieldIdx ].get() : nullptr);
}

XclImpPTField* XclImpPivotTable::GetFieldAcc( sal_uInt16 nFieldIdx )
{
    // do not return maDataOrientField
    return (nFieldIdx < maFields.size()) ? maFields[ nFieldIdx ].get() : nullptr;
}

const XclImpPTField* XclImpPivotTable::GetDataField( sal_uInt16 nDataFieldIdx ) const
{
    if( nDataFieldIdx < maOrigDataFields.size() )
        return GetField( maOrigDataFields[ nDataFieldIdx ] );
    return nullptr;
}

OUString XclImpPivotTable::GetDataFieldName( sal_uInt16 nDataFieldIdx ) const
{
    if( const XclImpPTField* pField = GetDataField( nDataFieldIdx ) )
        return pField->GetFieldName();
    return OUString();
}

// records --------------------------------------------------------------------

void XclImpPivotTable::ReadSxview( XclImpStream& rStrm )
{
    rStrm >> maPTInfo;

    GetAddressConverter().ConvertRange(
        maOutScRange, maPTInfo.maOutXclRange, GetCurrScTab(), GetCurrScTab(), true );

    mxPCache = GetPivotTableManager().GetPivotCache( maPTInfo.mnCacheIdx );
    mxCurrField.reset();
}

void XclImpPivotTable::ReadSxvd( XclImpStream& rStrm )
{
    sal_uInt16 nFieldCount = GetFieldCount();
    if( nFieldCount < EXC_PT_MAXFIELDCOUNT )
    {
        // cache index for the field is equal to the SXVD record index
        mxCurrField = std::make_shared<XclImpPTField>( *this, nFieldCount );
        maFields.push_back( mxCurrField );
        mxCurrField->ReadSxvd( rStrm );
        // add visible name of new field to list of visible names
        maVisFieldNames.push_back( mxCurrField->GetVisFieldName() );
        OSL_ENSURE( maFields.size() == maVisFieldNames.size(),
            "XclImpPivotTable::ReadSxvd - wrong size of visible name array" );
    }
    else
        mxCurrField.reset();
}

void XclImpPivotTable::ReadSxvi( XclImpStream& rStrm )
{
    if( mxCurrField )
        mxCurrField->ReadSxvi( rStrm );
}

void XclImpPivotTable::ReadSxvdex( XclImpStream& rStrm )
{
    if( mxCurrField )
        mxCurrField->ReadSxvdex( rStrm );
}

void XclImpPivotTable::ReadSxivd( XclImpStream& rStrm )
{
    mxCurrField.reset();

    // find the index vector to fill (row SXIVD doesn't exist without row fields)
    ScfUInt16Vec* pFieldVec = nullptr;
    if( maRowFields.empty() && (maPTInfo.mnRowFields > 0) )
        pFieldVec = &maRowFields;
    else if( maColFields.empty() && (maPTInfo.mnColFields > 0) )
        pFieldVec = &maColFields;

    // fill the vector from record data
    if( !pFieldVec )
        return;

    sal_uInt16 nSize = ulimit_cast< sal_uInt16 >( rStrm.GetRecSize() / 2, EXC_PT_MAXROWCOLCOUNT );
    pFieldVec->reserve( nSize );
    for( sal_uInt16 nIdx = 0; nIdx < nSize; ++nIdx )
    {
        sal_uInt16 nFieldIdx;
        nFieldIdx = rStrm.ReaduInt16();
        pFieldVec->push_back( nFieldIdx );

        // set orientation at special data orientation field
        if( nFieldIdx == EXC_SXIVD_DATA )
        {
            sal_uInt16 nAxis = (pFieldVec == &maRowFields) ? EXC_SXVD_AXIS_ROW : EXC_SXVD_AXIS_COL;
            maDataOrientField.SetAxes( nAxis );
        }
    }
}

void XclImpPivotTable::ReadSxpi( XclImpStream& rStrm )
{
    mxCurrField.reset();

    sal_uInt16 nSize = ulimit_cast< sal_uInt16 >( rStrm.GetRecSize() / 6 );
    for( sal_uInt16 nEntry = 0; nEntry < nSize; ++nEntry )
    {
        XclPTPageFieldInfo aPageInfo;
        rStrm >> aPageInfo;
        if( XclImpPTField* pField = GetFieldAcc( aPageInfo.mnField ) )
        {
            maPageFields.push_back( aPageInfo.mnField );
            pField->SetPageFieldInfo( aPageInfo );
        }
        GetCurrSheetDrawing().SetSkipObj( aPageInfo.mnObjId );
    }
}

void XclImpPivotTable::ReadSxdi( XclImpStream& rStrm )
{
    mxCurrField.reset();

    XclPTDataFieldInfo aDataInfo;
    rStrm >> aDataInfo;
    if( XclImpPTField* pField = GetFieldAcc( aDataInfo.mnField ) )
    {
        maOrigDataFields.push_back( aDataInfo.mnField );
        // DataPilot does not support double data fields -> add first appearance to index list only
        if( !pField->HasDataFieldInfo() )
            maFiltDataFields.push_back( aDataInfo.mnField );
        pField->AddDataFieldInfo( aDataInfo );
    }
}

void XclImpPivotTable::ReadSxex( XclImpStream& rStrm )
{
    rStrm >> maPTExtInfo;
}

void XclImpPivotTable::ReadSxViewEx9( XclImpStream& rStrm )
{
    rStrm >> maPTViewEx9Info;
}

void XclImpPivotTable::ReadSxAddl( XclImpStream& rStrm )
{
    rStrm >> maPTAddlInfo;
}

void XclImpPivotTable::Convert()
{
    if( !mxPCache || !mxPCache->IsValid() )
        return;

    if (comphelper::IsFuzzing()) //just too slow
        return;

    ScDPSaveData aSaveData;

    // *** global settings ***

    aSaveData.SetRowGrand( ::get_flag( maPTInfo.mnFlags, EXC_SXVIEW_ROWGRAND ) );
    aSaveData.SetColumnGrand( ::get_flag( maPTInfo.mnFlags, EXC_SXVIEW_COLGRAND ) );
    aSaveData.SetFilterButton( false );
    aSaveData.SetDrillDown( ::get_flag( maPTExtInfo.mnFlags, EXC_SXEX_DRILLDOWN ) );
    aSaveData.SetIgnoreEmptyRows( false );
    aSaveData.SetRepeatIfEmpty( false );

    // *** fields ***

    // row fields
    for( const auto& rRowField : maRowFields )
        if( const XclImpPTField* pField = GetField( rRowField ) )
            pField->ConvertRowColField( aSaveData );

    // column fields
    for( const auto& rColField : maColFields )
        if( const XclImpPTField* pField = GetField( rColField ) )
            pField->ConvertRowColField( aSaveData );

    // page fields
    for( const auto& rPageField : maPageFields )
        if( const XclImpPTField* pField = GetField( rPageField ) )
            pField->ConvertPageField( aSaveData );

    // We need to import hidden fields because hidden fields may contain
    // special settings for subtotals (aggregation function, filters, custom
    // name etc.) and members (hidden, custom name etc.).

    // hidden fields
    for( sal_uInt16 nField = 0, nCount = GetFieldCount(); nField < nCount; ++nField )
        if( const XclImpPTField* pField = GetField( nField ) )
            if (!pField->GetAxes())
                pField->ConvertHiddenField( aSaveData );

    // data fields
    for( const auto& rFiltDataField : maFiltDataFields )
        if( const XclImpPTField* pField = GetField( rFiltDataField ) )
            pField->ConvertDataField( aSaveData );

    // *** insert into Calc document ***

    // create source descriptor
    ScSheetSourceDesc aDesc(&GetDoc());
    const OUString& rSrcName = mxPCache->GetSourceRangeName();
    if (!rSrcName.isEmpty())
        // Range name is the data source.
        aDesc.SetRangeName(rSrcName);
    else
        // Normal cell range.
        aDesc.SetSourceRange(mxPCache->GetSourceRange());

    // adjust output range to include the page fields
    ScRange aOutRange( maOutScRange );
    if( !maPageFields.empty() )
    {
        SCROW nDecRows = ::std::min< SCROW >( aOutRange.aStart.Row(), maPageFields.size() + 1 );
        aOutRange.aStart.IncRow( -nDecRows );
    }

    // create the DataPilot
    std::unique_ptr<ScDPObject> pDPObj(new ScDPObject( &GetDoc() ));
    pDPObj->SetName( maPTInfo.maTableName );
    if (!maPTInfo.maDataName.isEmpty())
        aSaveData.GetDataLayoutDimension()->SetLayoutName(maPTInfo.maDataName);

    if (!maPTViewEx9Info.maGrandTotalName.isEmpty())
        aSaveData.SetGrandTotalName(maPTViewEx9Info.maGrandTotalName);

    pDPObj->SetSaveData( aSaveData );
    pDPObj->SetSheetDesc( aDesc );
    pDPObj->SetOutRange( aOutRange );
    pDPObj->SetHeaderLayout( maPTViewEx9Info.mnGridLayout == 0 );

    mpDPObj = GetDoc().GetDPCollection()->InsertNewTable(std::move(pDPObj));

    ApplyFieldInfo();
    ApplyMergeFlags(aOutRange, aSaveData);
}

void XclImpPivotTable::MaybeRefresh()
{
    if (mpDPObj && mxPCache->IsRefreshOnLoad())
    {
        // 'refresh table on load' flag is set.  Refresh the table now.  Some
        // Excel files contain partial table output when this flag is set.
        ScRange aOutRange = mpDPObj->GetOutRange();
        mpDPObj->Output(aOutRange.aStart);
    }
}

void XclImpPivotTable::ApplyMergeFlags(const ScRange& rOutRange, const ScDPSaveData& rSaveData)
{
    // Apply merge flags for various datapilot controls.

    ScDPOutputGeometry aGeometry(rOutRange, false);
    aGeometry.setColumnFieldCount(maPTInfo.mnColFields);
    aGeometry.setPageFieldCount(maPTInfo.mnPageFields);
    aGeometry.setDataFieldCount(maPTInfo.mnDataFields);
    aGeometry.setRowFieldCount(maPTInfo.mnRowFields);

    // Make sure we set headerlayout when input file has additional raw header
    if(maPTInfo.mnColFields == 0)
    {
        mpDPObj->SetHeaderLayout( maPTInfo.mnFirstHeadRow - 2 == static_cast<sal_uInt16>(aGeometry.getRowFieldHeaderRow()) );
    }
    aGeometry.setHeaderLayout(mpDPObj->GetHeaderLayout());
    aGeometry.setCompactMode(maPTAddlInfo.mbCompactMode);

    ScDocument& rDoc = GetDoc();

    vector<const ScDPSaveDimension*> aFieldDims;
    vector<ScAddress> aFieldBtns;

    aGeometry.getPageFieldPositions(aFieldBtns);
    for (const auto& rFieldBtn : aFieldBtns)
    {
        rDoc.ApplyFlagsTab(rFieldBtn.Col(), rFieldBtn.Row(), rFieldBtn.Col(), rFieldBtn.Row(), rFieldBtn.Tab(), ScMF::Button);

        ScMF nMFlag = ScMF::ButtonPopup;
        OUString aName = rDoc.GetString(rFieldBtn.Col(), rFieldBtn.Row(), rFieldBtn.Tab());
        if (rSaveData.HasInvisibleMember(aName))
            nMFlag |= ScMF::HiddenMember;

        rDoc.ApplyFlagsTab(rFieldBtn.Col()+1, rFieldBtn.Row(), rFieldBtn.Col()+1, rFieldBtn.Row(), rFieldBtn.Tab(), nMFlag);
    }

    aGeometry.getColumnFieldPositions(aFieldBtns);
    rSaveData.GetAllDimensionsByOrientation(sheet::DataPilotFieldOrientation_COLUMN, aFieldDims);
    if (aFieldBtns.size() == aFieldDims.size())
    {
        vector<const ScDPSaveDimension*>::const_iterator itDim = aFieldDims.begin();
        for (const auto& rFieldBtn : aFieldBtns)
        {
            ScMF nMFlag = ScMF::Button;
            const ScDPSaveDimension* pDim = *itDim;
            if (pDim->HasInvisibleMember())
                nMFlag |= ScMF::HiddenMember;
            if (!pDim->IsDataLayout())
                nMFlag |= ScMF::ButtonPopup;
            rDoc.ApplyFlagsTab(rFieldBtn.Col(), rFieldBtn.Row(), rFieldBtn.Col(), rFieldBtn.Row(), rFieldBtn.Tab(), nMFlag);
            ++itDim;
        }
    }

    aGeometry.getRowFieldPositions(aFieldBtns);
    rSaveData.GetAllDimensionsByOrientation(sheet::DataPilotFieldOrientation_ROW, aFieldDims);
    if (!((aFieldBtns.size() == aFieldDims.size()) || (maPTAddlInfo.mbCompactMode && aFieldBtns.size() == 1)))
        return;

    vector<const ScDPSaveDimension*>::const_iterator itDim = aFieldDims.begin();
    for (const auto& rFieldBtn : aFieldBtns)
    {
        ScMF nMFlag = ScMF::Button;
        const ScDPSaveDimension* pDim = itDim != aFieldDims.end() ? *itDim++ : nullptr;
        if (pDim && pDim->HasInvisibleMember())
            nMFlag |= ScMF::HiddenMember;
        if (!pDim || !pDim->IsDataLayout())
            nMFlag |= ScMF::ButtonPopup;
        rDoc.ApplyFlagsTab(rFieldBtn.Col(), rFieldBtn.Row(), rFieldBtn.Col(), rFieldBtn.Row(), rFieldBtn.Tab(), nMFlag);
    }
}


void XclImpPivotTable::ApplyFieldInfo()
{
    mpDPObj->BuildAllDimensionMembers();
    ScDPSaveData& rSaveData = *mpDPObj->GetSaveData();

    // row fields
    for( const auto& rRowField : maRowFields )
        if( const XclImpPTField* pField = GetField( rRowField ) )
            pField->ConvertFieldInfo( rSaveData, mpDPObj, *this );

    // column fields
    for( const auto& rColField : maColFields )
        if( const XclImpPTField* pField = GetField( rColField ) )
            pField->ConvertFieldInfo( rSaveData, mpDPObj, *this );

    // page fields
    for( const auto& rPageField : maPageFields )
        if( const XclImpPTField* pField = GetField( rPageField ) )
            pField->ConvertFieldInfo( rSaveData, mpDPObj, *this, true );

    // hidden fields
    for( sal_uInt16 nField = 0, nCount = GetFieldCount(); nField < nCount; ++nField )
        if( const XclImpPTField* pField = GetField( nField ) )
            if (!pField->GetAxes())
                pField->ConvertFieldInfo( rSaveData, mpDPObj, *this );
}

XclImpPivotTableManager::XclImpPivotTableManager( const XclImpRoot& rRoot ) :
    XclImpRoot( rRoot )
{
}

XclImpPivotTableManager::~XclImpPivotTableManager()
{
}

// pivot cache records --------------------------------------------------------

XclImpPivotCacheRef XclImpPivotTableManager::GetPivotCache( sal_uInt16 nCacheIdx )
{
    XclImpPivotCacheRef xPCache;
    if( nCacheIdx < maPCaches.size() )
        xPCache = maPCaches[ nCacheIdx ];
    return xPCache;
}

void XclImpPivotTableManager::ReadSxidstm( XclImpStream& rStrm )
{
    XclImpPivotCacheRef xPCache = std::make_shared<XclImpPivotCache>( GetRoot() );
    maPCaches.push_back( xPCache );
    xPCache->ReadSxidstm( rStrm );
}

void XclImpPivotTableManager::ReadSxvs( XclImpStream& rStrm )
{
    if( !maPCaches.empty() )
        maPCaches.back()->ReadSxvs( rStrm );
}

void XclImpPivotTableManager::ReadDconref( XclImpStream& rStrm )
{
    if( !maPCaches.empty() )
        maPCaches.back()->ReadDconref( rStrm );
}

void XclImpPivotTableManager::ReadDConName( XclImpStream& rStrm )
{
    if( !maPCaches.empty() )
        maPCaches.back()->ReadDConName( rStrm );
}

// pivot table records --------------------------------------------------------

void XclImpPivotTableManager::ReadSxview( XclImpStream& rStrm )
{
    XclImpPivotTableRef xPTable = std::make_shared<XclImpPivotTable>( GetRoot() );
    maPTables.push_back( xPTable );
    xPTable->ReadSxview( rStrm );
}

void XclImpPivotTableManager::ReadSxvd( XclImpStream& rStrm )
{
    if( !maPTables.empty() )
        maPTables.back()->ReadSxvd( rStrm );
}

void XclImpPivotTableManager::ReadSxvdex( XclImpStream& rStrm )
{
    if( !maPTables.empty() )
        maPTables.back()->ReadSxvdex( rStrm );
}

void XclImpPivotTableManager::ReadSxivd( XclImpStream& rStrm )
{
    if( !maPTables.empty() )
        maPTables.back()->ReadSxivd( rStrm );
}

void XclImpPivotTableManager::ReadSxpi( XclImpStream& rStrm )
{
    if( !maPTables.empty() )
        maPTables.back()->ReadSxpi( rStrm );
}

void XclImpPivotTableManager::ReadSxdi( XclImpStream& rStrm )
{
    if( !maPTables.empty() )
        maPTables.back()->ReadSxdi( rStrm );
}

void XclImpPivotTableManager::ReadSxvi( XclImpStream& rStrm )
{
    if( !maPTables.empty() )
        maPTables.back()->ReadSxvi( rStrm );
}

void XclImpPivotTableManager::ReadSxex( XclImpStream& rStrm )
{
    if( !maPTables.empty() )
        maPTables.back()->ReadSxex( rStrm );
}

void XclImpPivotTableManager::ReadSxViewEx9( XclImpStream& rStrm )
{
    if( !maPTables.empty() )
        maPTables.back()->ReadSxViewEx9( rStrm );
}

void XclImpPivotTableManager::ReadSxAddl( XclImpStream& rStrm )
{
    if( !maPTables.empty() )
        maPTables.back()->ReadSxAddl( rStrm );
}

void XclImpPivotTableManager::ReadPivotCaches( const XclImpStream& rStrm )
{
    for( auto& rxPCache : maPCaches )
        rxPCache->ReadPivotCacheStream( rStrm );
}

void XclImpPivotTableManager::ConvertPivotTables()
{
    for( auto& rxPTable : maPTables )
        rxPTable->Convert();
}

void XclImpPivotTableManager::MaybeRefreshPivotTables()
{
    for( auto& rxPTable : maPTables )
        rxPTable->MaybeRefresh();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
