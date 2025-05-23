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

#include <com/sun/star/util/MeasureUnit.hpp>

#include <hintids.hxx>
#include <rtl/ustring.hxx>
#include <rtl/ustrbuf.hxx>
#include <utility>
#include <xmloff/xmluconv.hxx>
#include "xmlexpit.hxx"
#include <xmloff/namespacemap.hxx>
#include "xmlbrshe.hxx"
#include <editeng/brushitem.hxx>
#include <fmtornt.hxx>
#include <unomid.h>
#include <frmfmt.hxx>
#include "xmlexp.hxx"
#include <editeng/memberids.h>
#include <editeng/prntitem.hxx>

using namespace ::com::sun::star;
using namespace ::xmloff::token;

namespace {

class SwXMLTableItemMapper_Impl: public SvXMLExportItemMapper
{
    SwXMLBrushItemExport m_aBrushItemExport;

protected:

    sal_uInt32 m_nAbsWidth;

    static void AddAttribute( sal_uInt16 nPrefix, enum XMLTokenEnum eLName,
                       const OUString& rValue,
                       const SvXMLNamespaceMap& rNamespaceMap,
                       comphelper::AttributeList& rAttrList );

public:

    SwXMLTableItemMapper_Impl(
            SvXMLItemMapEntriesRef rMapEntries,
            SwXMLExport& rExp );

    virtual void handleSpecialItem( comphelper::AttributeList& rAttrList,
                                    const SvXMLItemMapEntry& rEntry,
                                    const SfxPoolItem& rItem,
                                    const SvXMLUnitConverter& rUnitConverter,
                                    const SvXMLNamespaceMap& rNamespaceMap,
                                    const SfxItemSet *pSet ) const override;

    virtual void handleElementItem(
            const SvXMLItemMapEntry& rEntry,
            const SfxPoolItem& rItem ) const override;

    inline void SetAbsWidth( sal_uInt32 nAbs );
};

}

SwXMLTableItemMapper_Impl::SwXMLTableItemMapper_Impl(
        SvXMLItemMapEntriesRef rMapEntries,
        SwXMLExport& rExp ) :
    SvXMLExportItemMapper( std::move(rMapEntries) ),
    m_aBrushItemExport( rExp ),
    m_nAbsWidth( USHRT_MAX )
{
}

void SwXMLTableItemMapper_Impl::AddAttribute( sal_uInt16 nPrefix,
        enum XMLTokenEnum eLName,
        const OUString& rValue,
        const SvXMLNamespaceMap& rNamespaceMap,
        comphelper::AttributeList& rAttrList )
{
    OUString sName( rNamespaceMap.GetQNameByKey( nPrefix,
                                                 GetXMLToken(eLName) ) );
    rAttrList.AddAttribute( sName, rValue );
}

void SwXMLTableItemMapper_Impl::handleSpecialItem(
        comphelper::AttributeList& rAttrList,
        const SvXMLItemMapEntry& rEntry,
        const SfxPoolItem& rItem,
        const SvXMLUnitConverter& rUnitConverter,
        const SvXMLNamespaceMap& rNamespaceMap,
        const SfxItemSet *pSet ) const
{
    switch( rEntry.nWhichId )
    {

    case RES_PRINT:
        {
            const SvxPrintItem *pItem;
            if( pSet &&
                (pItem = pSet->GetItemIfSet( RES_PRINT )) )
            {
                bool bHasTextChangesOnly = pItem->GetValue();
                if ( !bHasTextChangesOnly )
                {
                    OUString sValue;
                    sal_uInt16 nMemberId =
                        static_cast<sal_uInt16>( rEntry.nMemberId & MID_SW_FLAG_MASK );

                    if( SvXMLExportItemMapper::QueryXMLValue(
                        rItem, sValue, nMemberId, rUnitConverter ) )
                    {
                        AddAttribute( rEntry.nNameSpace, rEntry.eLocalName,
                                      sValue, rNamespaceMap, rAttrList );
                    }
                }
            }
        }
        break;

    case RES_LR_SPACE:
        {
            const SwFormatHoriOrient *pItem;
            if( pSet &&
                (pItem = pSet->GetItemIfSet( RES_HORI_ORIENT )) )
            {
                sal_Int16 eHoriOrient = pItem->GetHoriOrient();
                bool bExport = false;
                sal_uInt16 nMemberId =
                    o3tl::narrowing<sal_uInt16>( rEntry.nMemberId & MID_SW_FLAG_MASK );
                switch( nMemberId )
                {
                case MID_L_MARGIN:
                    bExport = text::HoriOrientation::NONE == eHoriOrient ||
                              text::HoriOrientation::LEFT_AND_WIDTH == eHoriOrient;
                    break;
                case MID_R_MARGIN:
                    bExport = text::HoriOrientation::NONE == eHoriOrient;
                    break;
                }
                OUString sValue;
                if( bExport && SvXMLExportItemMapper::QueryXMLValue(
                    rItem, sValue, nMemberId, rUnitConverter ) )
                {
                    AddAttribute( rEntry.nNameSpace, rEntry.eLocalName, sValue,
                                  rNamespaceMap, rAttrList );
                }
            }
        }
        break;

    case RES_FRM_SIZE:
        {
            sal_uInt16 nMemberId =
                o3tl::narrowing<sal_uInt16>( rEntry.nMemberId & MID_SW_FLAG_MASK );
            switch( nMemberId )
            {
            case MID_FRMSIZE_WIDTH:
                if( m_nAbsWidth )
                {
                    OUStringBuffer sBuffer;
                    rUnitConverter.convertMeasureToXML( sBuffer, m_nAbsWidth );
                    AddAttribute( rEntry.nNameSpace, rEntry.eLocalName,
                                  sBuffer.makeStringAndClear(),
                                  rNamespaceMap, rAttrList );
                }
                break;
            case MID_FRMSIZE_REL_WIDTH:
                {
                    OUString sValue;
                    if( SvXMLExportItemMapper::QueryXMLValue(
                        rItem, sValue, nMemberId, rUnitConverter ) )
                    {
                        AddAttribute( rEntry.nNameSpace, rEntry.eLocalName,
                                      sValue, rNamespaceMap, rAttrList );
                    }
                }
                break;
            }
        }
        break;
    }
}

/** this method is called for every item that has the
    MID_SW_FLAG_ELEMENT_EXPORT flag set */
void SwXMLTableItemMapper_Impl::handleElementItem(
        const SvXMLItemMapEntry& rEntry,
        const SfxPoolItem& rItem ) const
{
    switch( rEntry.nWhichId )
    {
    case RES_BACKGROUND:
        {
            const_cast<SwXMLTableItemMapper_Impl *>(this)->m_aBrushItemExport.exportXML(
                                                static_cast<const SvxBrushItem&>(rItem) );
        }
        break;
    }
}

inline void SwXMLTableItemMapper_Impl::SetAbsWidth( sal_uInt32 nAbs )
{
    m_nAbsWidth = nAbs;
}

void SwXMLExport::InitItemExport()
{
    m_pTwipUnitConverter.reset(new SvXMLUnitConverter(getComponentContext(),
        util::MeasureUnit::TWIP, GetMM100UnitConverter().GetXMLMeasureUnit(),
        getSaneDefaultVersion()));

    m_xTableItemMap = new SvXMLItemMapEntries( aXMLTableItemMap );
    m_xTableRowItemMap = new SvXMLItemMapEntries( aXMLTableRowItemMap );
    m_xTableCellItemMap = new SvXMLItemMapEntries( aXMLTableCellItemMap );

    m_pTableItemMapper.reset(new SwXMLTableItemMapper_Impl( m_xTableItemMap, *this ));
}

void SwXMLExport::FinitItemExport()
{
    m_pTableItemMapper.reset();
    m_pTwipUnitConverter.reset();
}

void SwXMLExport::ExportTableFormat( const SwFrameFormat& rFormat, sal_uInt32 nAbsWidth )
{
    static_cast<SwXMLTableItemMapper_Impl *>(m_pTableItemMapper.get())
        ->SetAbsWidth( nAbsWidth );
    ExportFormat(rFormat, XML_TABLE, {});
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
