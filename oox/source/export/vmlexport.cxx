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

#include <config_folders.h>
#include <rtl/bootstrap.hxx>
#include <svl/itemset.hxx>
#include <oox/export/drawingml.hxx>
#include <oox/export/vmlexport.hxx>
#include <sax/fastattribs.hxx>

#include <oox/token/tokens.hxx>

#include <rtl/strbuf.hxx>
#include <rtl/ustring.hxx>
#include <sal/log.hxx>

#include <tools/stream.hxx>
#include <comphelper/sequenceashashmap.hxx>
#include <svx/msdffdef.hxx>
#include <svx/svdotext.hxx>
#include <svx/svdograf.hxx>
#include <svx/sdmetitm.hxx>
#include <utility>
#include <vcl/cvtgrf.hxx>
#include <filter/msfilter/msdffimp.hxx>
#include <filter/msfilter/util.hxx>
#include <filter/msfilter/escherex.hxx>
#include <o3tl/string_view.hxx>
#include <drawingml/fontworkhelpers.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XPropertySetInfo.hpp>
#include <com/sun/star/drawing/XShape.hpp>
#include <com/sun/star/text/HoriOrientation.hpp>
#include <com/sun/star/text/VertOrientation.hpp>
#include <com/sun/star/text/RelOrientation.hpp>
#include <com/sun/star/text/WritingMode2.hpp>
#include <com/sun/star/text/XTextFrame.hpp>

#include <cstdio>

using namespace sax_fastparser;
using namespace oox::vml;
using namespace com::sun::star;

const sal_Int32 Tag_Container = 44444;
const sal_Int32 Tag_Commit = 44445;

VMLExport::VMLExport( ::sax_fastparser::FSHelperPtr pSerializer, VMLTextExport* pTextExport )
    : EscherEx( std::make_shared<EscherExGlobal>(), nullptr, /*bOOXML=*/true )
    , m_pSerializer(std::move( pSerializer ))
    , m_pTextExport( pTextExport )
    , m_eHOri( 0 )
    , m_eVOri( 0 )
    , m_eHRel( 0 )
    , m_eVRel( 0 )
    , m_bInline( false )
    , m_pSdrObject( nullptr )
    , m_nShapeType( ESCHER_ShpInst_Nil )
    , m_nShapeFlags(ShapeFlag::NONE)
    , m_ShapeStyle( 200 )
    , m_aShapeTypeWritten( ESCHER_ShpInst_COUNT )
    , m_bSkipwzName( false )
    , m_bUseHashMarkForType( false )
    , m_bOverrideShapeIdGeneration( false )
    , m_nShapeIDCounter( 0 )
{
    mnGroupLevel = 1;
}

void VMLExport::SetFS( const ::sax_fastparser::FSHelperPtr& pSerializer )
{
    m_pSerializer = pSerializer;
}

VMLExport::~VMLExport()
{
}

void VMLExport::OpenContainer( sal_uInt16 nEscherContainer, int nRecInstance )
{
    EscherEx::OpenContainer( nEscherContainer, nRecInstance );

    if ( nEscherContainer != ESCHER_SpContainer )
        return;

    // opening a shape container
    SAL_WARN_IF(m_nShapeType != ESCHER_ShpInst_Nil, "oox.vml", "opening shape inside of a shape!");
    m_nShapeType = ESCHER_ShpInst_Nil;
    m_pShapeAttrList = FastSerializerHelper::createAttrList();

    m_ShapeStyle.setLength(0);
    m_ShapeStyle.ensureCapacity(200);

    // postpone the output so that we are able to write even the elements
    // that we learn inside Commit()
    m_pSerializer->mark(Tag_Container);
}

void VMLExport::CloseContainer()
{
    if ( mRecTypes.back() == ESCHER_SpContainer )
    {
        // write the shape now when we have all the info
        sal_Int32 nShapeElement = StartShape();

        m_pSerializer->mergeTopMarks(Tag_Container);

        EndShape( nShapeElement );

        // cleanup
        m_nShapeType = ESCHER_ShpInst_Nil;
        m_pShapeAttrList = nullptr;
    }

    EscherEx::CloseContainer();
}

sal_uInt32 VMLExport::EnterGroup( const OUString& rShapeName, const tools::Rectangle* pRect )
{
    sal_uInt32 nShapeId = GenerateShapeId();

    OStringBuffer aStyle( 200 );
    rtl::Reference<FastAttributeList> pAttrList = FastSerializerHelper::createAttrList();

    pAttrList->add( XML_id, ShapeIdString( nShapeId ) );

    if ( rShapeName.getLength() )
        pAttrList->add( XML_alt, rShapeName );

    bool rbAbsolutePos = true;
    //editAs
    OUString rEditAs = EscherEx::GetEditAs();
    if (!rEditAs.isEmpty())
    {
        pAttrList->add(XML_editas, rEditAs);
        rbAbsolutePos = false;
    }

    // style
    if ( pRect )
        AddRectangleDimensions( aStyle, *pRect, rbAbsolutePos );

    if ( !aStyle.isEmpty() )
        pAttrList->add( XML_style, aStyle );

    // coordorigin/coordsize
    if ( pRect && ( mnGroupLevel == 1 ) )
    {
        pAttrList->add( XML_coordorigin,
                OString::number( pRect->Left() ) + "," + OString::number( pRect->Top() ) );

        pAttrList->add( XML_coordsize,
                OString::number( pRect->Right() - pRect->Left() ) + "," +
                OString::number( pRect->Bottom() - pRect->Top() ) );
    }

    m_pSerializer->startElementNS( XML_v, XML_group, pAttrList );

    mnGroupLevel++;
    return nShapeId;
}

void VMLExport::LeaveGroup()
{
    --mnGroupLevel;
    m_pSerializer->endElementNS( XML_v, XML_group );
}

void VMLExport::AddShape( sal_uInt32 nShapeType, ShapeFlag nShapeFlags, sal_uInt32 nShapeId )
{
    m_nShapeType = nShapeType;
    m_nShapeFlags = nShapeFlags;

    m_sShapeId = ShapeIdString( nShapeId );
    if (m_sShapeId.startsWith("_x0000_"))
    {
        // xml_id must be set elsewhere. The id is critical for matching VBA macros etc,
        // and the spid is critical to link to the shape number elsewhere.
        m_pShapeAttrList->addNS( XML_o, XML_spid, m_sShapeId );
    }
    else if (IsWaterMarkShape(m_pSdrObject->GetName()))
    {
        // Shape is a watermark object - keep the original shape's name
        // because Microsoft detects if it is a watermark by the actual name
        m_pShapeAttrList->add( XML_id, m_pSdrObject->GetName() );
        // also ('o:spid')
        m_pShapeAttrList->addNS( XML_o, XML_spid, m_sShapeId );
    }
    else
    {
        m_pShapeAttrList->add(XML_id, m_sShapeId);
    }
}

bool VMLExport::IsWaterMarkShape(std::u16string_view rStr)
{
     if (rStr.empty() )  return false;

     return o3tl::starts_with(rStr, u"PowerPlusWaterMarkObject") || o3tl::starts_with(rStr, u"WordPictureWatermark");
}

void VMLExport::OverrideShapeIDGen(bool bOverrideShapeIdGen, const OString& sShapeIDPrefix)
{
    m_bOverrideShapeIdGeneration = bOverrideShapeIdGen;
    if(bOverrideShapeIdGen)
    {
        assert(!sShapeIDPrefix.isEmpty());
        m_sShapeIDPrefix = sShapeIDPrefix;
    }
    else
        m_sShapeIDPrefix.clear();
}

static void impl_AddArrowHead( sax_fastparser::FastAttributeList *pAttrList, sal_Int32 nElement, sal_uInt32 nValue )
{
    if ( !pAttrList )
        return;

    const char *pArrowHead = nullptr;
    switch ( nValue )
    {
        case ESCHER_LineNoEnd:           pArrowHead = "none"; break;
        case ESCHER_LineArrowEnd:        pArrowHead = "block"; break;
        case ESCHER_LineArrowStealthEnd: pArrowHead = "classic"; break;
        case ESCHER_LineArrowDiamondEnd: pArrowHead = "diamond"; break;
        case ESCHER_LineArrowOvalEnd:    pArrowHead = "oval"; break;
        case ESCHER_LineArrowOpenEnd:    pArrowHead = "open"; break;
    }

    if ( pArrowHead )
        pAttrList->add( nElement, pArrowHead );
}

static void impl_AddArrowLength( sax_fastparser::FastAttributeList *pAttrList, sal_Int32 nElement, sal_uInt32 nValue )
{
    if ( !pAttrList )
        return;

    const char *pArrowLength = nullptr;
    switch ( nValue )
    {
        case ESCHER_LineShortArrow:     pArrowLength = "short"; break;
        case ESCHER_LineMediumLenArrow: pArrowLength = "medium"; break;
        case ESCHER_LineLongArrow:      pArrowLength = "long"; break;
    }

    if ( pArrowLength )
        pAttrList->add( nElement, pArrowLength );
}

static void impl_AddArrowWidth( sax_fastparser::FastAttributeList *pAttrList, sal_Int32 nElement, sal_uInt32 nValue )
{
    if ( !pAttrList )
        return;

    const char *pArrowWidth = nullptr;
    switch ( nValue )
    {
        case ESCHER_LineNarrowArrow:      pArrowWidth = "narrow"; break;
        case ESCHER_LineMediumWidthArrow: pArrowWidth = "medium"; break;
        case ESCHER_LineWideArrow:        pArrowWidth = "wide"; break;
    }

    if ( pArrowWidth )
        pAttrList->add( nElement, pArrowWidth );
}

static void impl_AddBool( sax_fastparser::FastAttributeList *pAttrList, sal_Int32 nElement, bool bValue )
{
    if ( !pAttrList )
        return;

    pAttrList->add( nElement, bValue? "t": "f" );
}

static void impl_AddColor( sax_fastparser::FastAttributeList *pAttrList, sal_Int32 nElement, sal_uInt32 nColor )
{
    SAL_WARN_IF( nColor & 0xFF000000 , "oox.vml" , "TODO: this is not a RGB value!");

    if ( !pAttrList || ( nColor & 0xFF000000 ) )
        return;

    nColor = ( ( nColor & 0xFF ) << 16 ) + ( nColor & 0xFF00 ) + ( ( nColor & 0xFF0000 ) >> 16 );

    const char *pColor = nullptr;
    char pRgbColor[10];
    switch ( nColor )
    {
        case 0x000000: pColor = "black"; break;
        case 0xC0C0C0: pColor = "silver"; break;
        case 0x808080: pColor = "gray"; break;
        case 0xFFFFFF: pColor = "white"; break;
        case 0x800000: pColor = "maroon"; break;
        case 0xFF0000: pColor = "red"; break;
        case 0x800080: pColor = "purple"; break;
        case 0xFF00FF: pColor = "fuchsia"; break;
        case 0x008000: pColor = "green"; break;
        case 0x00FF00: pColor = "lime"; break;
        case 0x808000: pColor = "olive"; break;
        case 0xFFFF00: pColor = "yellow"; break;
        case 0x000080: pColor = "navy"; break;
        case 0x0000FF: pColor = "blue"; break;
        case 0x008080: pColor = "teal"; break;
        case 0x00FFFF: pColor = "aqua"; break;
        default:
            {
                snprintf( pRgbColor, sizeof( pRgbColor ), "#%06x", static_cast< unsigned int >( nColor ) ); // not too handy to use OString::valueOf() here :-(
                pColor = pRgbColor;
            }
            break;
    }

    pAttrList->add( nElement, pColor );
}

static void impl_AddInt( sax_fastparser::FastAttributeList *pAttrList, sal_Int32 nElement, sal_uInt32 nValue )
{
    if ( !pAttrList )
        return;

    pAttrList->add( nElement, OString::number( nValue ) );
}

static sal_uInt16 impl_GetUInt16( const sal_uInt8* &pVal )
{
    sal_uInt16 nRet = *pVal++;
    nRet += ( *pVal++ ) << 8;
    return nRet;
}

static sal_Int32 impl_GetPointComponent( const sal_uInt8* &pVal, sal_uInt16 nPointSize )
{
    sal_Int32 nRet = 0;
    if ( ( nPointSize == 0xfff0 ) || ( nPointSize == 4 ) )
    {
        sal_uInt16 nUnsigned = *pVal++;
        nUnsigned += ( *pVal++ ) << 8;

        nRet = sal_Int16( nUnsigned );
    }
    else if ( nPointSize == 8 )
    {
        sal_uInt32 nUnsigned = *pVal++;
        nUnsigned += ( *pVal++ ) << 8;
        nUnsigned += ( *pVal++ ) << 16;
        nUnsigned += ( *pVal++ ) << 24;

        nRet = nUnsigned;
    }

    return nRet;
}

void  VMLExport::AddSdrObjectVMLObject( const SdrObject& rObj)
{
   m_pSdrObject = &rObj;
}
void VMLExport::Commit( EscherPropertyContainer& rProps, const tools::Rectangle& rRect )
{
    if ( m_nShapeType == ESCHER_ShpInst_Nil )
        return;

    // postpone the output of the embedded elements so that they are written
    // inside the shapes
    m_pSerializer->mark(Tag_Commit);

    // dimensions
    if ( m_nShapeType == ESCHER_ShpInst_Line )
        AddLineDimensions( rRect );
    else
    {
        if ( IsWaterMarkShape( m_pSdrObject->GetName() ) )
        {
            // Watermark need some padding to be compatible with MSO
            tools::Long nPaddingY = 0;
            const SfxItemSet& rSet = m_pSdrObject->GetMergedItemSet();
            if ( const SdrMetricItem* pItem = rSet.GetItem( SDRATTR_TEXT_UPPERDIST ) )
                nPaddingY += pItem->GetValue();

            tools::Rectangle aRect( rRect );
            aRect.setHeight( aRect.getOpenHeight() + nPaddingY );
            AddRectangleDimensions( m_ShapeStyle, aRect );
        }
        else
            AddRectangleDimensions( m_ShapeStyle, rRect );
    }

    // properties
    // The numbers of defines ESCHER_Prop_foo and DFF_Prop_foo correspond to the PIDs in
    // 'Microsoft Office Drawing 97-2007 Binary Format Specification'.
    // The property values are set by EscherPropertyContainer::CreateCustomShapeProperties() method.
    bool bAlreadyWritten[ 0xFFF ] = {};
    const EscherProperties &rOpts = rProps.GetOpts();
    for (auto const& opt : rOpts)
    {
        sal_uInt16 nId = ( opt.nPropId & 0x0FFF );

        if ( bAlreadyWritten[ nId ] )
            continue;

        switch ( nId )
        {
            case ESCHER_Prop_WrapText: // 133
                {
                    const char *pWrapType = nullptr;
                    switch ( opt.nPropValue )
                    {
                        case ESCHER_WrapSquare:
                        case ESCHER_WrapByPoints:  pWrapType = "square"; break; // these two are equivalent according to the docu
                        case ESCHER_WrapNone:      pWrapType = "none"; break;
                        case ESCHER_WrapTopBottom:
                        case ESCHER_WrapThrough:
                            break; // last two are *undefined* in MS-ODRAW, don't exist in VML
                    }
                    if ( pWrapType )
                    {
                        m_ShapeStyle.append(";mso-wrap-style:");
                        m_ShapeStyle.append(pWrapType);
                    }
                }
                bAlreadyWritten[ ESCHER_Prop_WrapText ] = true;
                break;

            case ESCHER_Prop_AnchorText: // 135
                {
                    char const* pValue(nullptr);
                    switch (opt.nPropValue)
                    {
                        case ESCHER_AnchorTop:
                            pValue = "top";
                        break;
                        case ESCHER_AnchorMiddle:
                            pValue = "middle";
                        break;
                        case ESCHER_AnchorBottom:
                            pValue = "bottom";
                        break;
                        case ESCHER_AnchorTopCentered:
                            pValue = "top-center";
                        break;
                        case ESCHER_AnchorMiddleCentered:
                            pValue = "middle-center";
                        break;
                        case ESCHER_AnchorBottomCentered:
                            pValue = "bottom-center";
                        break;
                        case ESCHER_AnchorTopBaseline:
                            pValue = "top-baseline";
                        break;
                        case ESCHER_AnchorBottomBaseline:
                            pValue = "bottom-baseline";
                        break;
                        case ESCHER_AnchorTopCenteredBaseline:
                            pValue = "top-center-baseline";
                        break;
                        case ESCHER_AnchorBottomCenteredBaseline:
                            pValue = "bottom-center-baseline";
                        break;
                    }
                    m_ShapeStyle.append(";v-text-anchor:");
                    m_ShapeStyle.append(pValue);
                }
                break;

            case ESCHER_Prop_txflTextFlow: // 136
                {
                    // at least "bottom-to-top" only has an effect when it's on the v:textbox element, not on v:shape
                    assert(m_TextboxStyle.isEmpty());
                    switch (opt.nPropValue)
                    {
                        case ESCHER_txflHorzN:
                            m_TextboxStyle.append("layout-flow:horizontal");
                        break;
                        case ESCHER_txflTtoBA:
                            m_TextboxStyle.append("layout-flow:vertical");
                        break;
                        case ESCHER_txflBtoT:
                            m_TextboxStyle.append("mso-layout-flow-alt:bottom-to-top");
                        break;
                        default:
                            assert(false); // unimplemented in escher export
                        break;
                    }
                }
                break;

            // coordorigin
            case ESCHER_Prop_geoLeft: // 320
            case ESCHER_Prop_geoTop: // 321
                {
                    sal_uInt32 nLeft = 0, nTop = 0;

                    if ( nId == ESCHER_Prop_geoLeft )
                    {
                        nLeft = opt.nPropValue;
                        rProps.GetOpt( ESCHER_Prop_geoTop, nTop );
                    }
                    else
                    {
                        nTop = opt.nPropValue;
                        rProps.GetOpt( ESCHER_Prop_geoLeft, nLeft );
                    }
                    if(nTop!=0 && nLeft!=0)
                        m_pShapeAttrList->add( XML_coordorigin,
                                OString::number( nLeft ) + "," + OString::number( nTop ) );
                }
                bAlreadyWritten[ ESCHER_Prop_geoLeft ] = true;
                bAlreadyWritten[ ESCHER_Prop_geoTop ] = true;
                break;

            // coordsize
            case ESCHER_Prop_geoRight: // 322
            case ESCHER_Prop_geoBottom: // 323
                {
                    sal_uInt32 nLeft = 0, nRight = 0, nTop = 0, nBottom = 0;
                    rProps.GetOpt( ESCHER_Prop_geoLeft, nLeft );
                    rProps.GetOpt( ESCHER_Prop_geoTop, nTop );

                    if ( nId == ESCHER_Prop_geoRight )
                    {
                        nRight = opt.nPropValue;
                        rProps.GetOpt( ESCHER_Prop_geoBottom, nBottom );
                    }
                    else
                    {
                        nBottom = opt.nPropValue;
                        rProps.GetOpt( ESCHER_Prop_geoRight, nRight );
                    }

                    if(nBottom!=0 &&  nRight!=0 )
                        m_pShapeAttrList->add( XML_coordsize,
                                OString::number( nRight - nLeft ) + "," + OString::number( nBottom - nTop ) );
                }
                bAlreadyWritten[ ESCHER_Prop_geoRight ] = true;
                bAlreadyWritten[ ESCHER_Prop_geoBottom ] = true;
                break;

            case ESCHER_Prop_pVertices: // 325
            case ESCHER_Prop_pSegmentInfo: // 326
                {
                    EscherPropSortStruct aVertices;
                    EscherPropSortStruct aSegments;

                    if ( rProps.GetOpt( ESCHER_Prop_pVertices, aVertices ) &&
                         rProps.GetOpt( ESCHER_Prop_pSegmentInfo, aSegments ) )
                    {
                        const sal_uInt8 *pVerticesIt = aVertices.nProp.data() + 6;
                        const sal_uInt8 *pSegmentIt = aSegments.nProp.data();
                        OStringBuffer aPath( 512 );

                        sal_uInt16 nPointSize = aVertices.nProp[4] + ( aVertices.nProp[5] << 8 );

                        // number of segments
                        sal_uInt16 nSegments = impl_GetUInt16( pSegmentIt );
                        pSegmentIt += 4;

                        for ( ; nSegments; --nSegments )
                        {
                            sal_uInt16 nSeg = impl_GetUInt16( pSegmentIt );

                            // The segment type is stored in the upper 3 bits
                            // and segment count is stored in the lower 13
                            // bits.
                            unsigned char nSegmentType = (nSeg & 0xE000) >> 13;
                            unsigned short nSegmentCount = nSeg & 0x03FF;

                            switch (nSegmentType)
                            {
                                case msopathMoveTo:
                                {
                                    sal_Int32 nX = impl_GetPointComponent( pVerticesIt, nPointSize );
                                    sal_Int32 nY = impl_GetPointComponent( pVerticesIt, nPointSize );
                                    if (nX >= 0 && nY >= 0 )
                                        aPath.append( "m" + OString::number( nX ) + "," + OString::number( nY ) );
                                    break;
                                }
                                case msopathClientEscape:
                                    break;
                                case msopathEscape:
                                {
                                    // If the segment type is msopathEscape, the lower 13 bits are
                                    // divided in a 5 bit escape code and 8 bit
                                    // vertex count (not segment count!)
                                    unsigned char nEscapeCode = (nSegmentCount & 0x1F00) >> 8;
                                    unsigned char nVertexCount = nSegmentCount & 0x00FF;
                                    pVerticesIt += nVertexCount;

                                    switch (nEscapeCode)
                                    {
                                        case 0xa: // nofill
                                            aPath.append( "nf" );
                                            break;
                                        case 0xb: // nostroke
                                            aPath.append( "ns" );
                                            break;
                                    }

                                    break;
                                }
                                case msopathLineTo:
                                    for (unsigned short i = 0; i < nSegmentCount; ++i)
                                    {
                                        sal_Int32 nX = impl_GetPointComponent( pVerticesIt, nPointSize );
                                        sal_Int32 nY = impl_GetPointComponent( pVerticesIt, nPointSize );
                                        aPath.append( "l" + OString::number( nX ) + "," + OString::number( nY ) );
                                    }
                                    break;
                                case msopathCurveTo:
                                    for (unsigned short i = 0; i < nSegmentCount; ++i)
                                    {
                                        sal_Int32 nX1 = impl_GetPointComponent( pVerticesIt, nPointSize );
                                        sal_Int32 nY1 = impl_GetPointComponent( pVerticesIt, nPointSize );
                                        sal_Int32 nX2 = impl_GetPointComponent( pVerticesIt, nPointSize );
                                        sal_Int32 nY2 = impl_GetPointComponent( pVerticesIt, nPointSize );
                                        sal_Int32 nX3 = impl_GetPointComponent( pVerticesIt, nPointSize );
                                        sal_Int32 nY3 = impl_GetPointComponent( pVerticesIt, nPointSize );
                                        aPath.append( "c" + OString::number( nX1 ) + "," + OString::number( nY1 ) + "," +
                                            OString::number( nX2 ) + "," + OString::number( nY2 ) + "," +
                                            OString::number( nX3 ) + "," + OString::number( nY3 ) );
                                    }
                                    break;
                                case msopathClose:
                                    aPath.append( "x" );
                                    break;
                                case msopathEnd:
                                    aPath.append( "e" );
                                    break;
                                default:
                                    SAL_WARN("oox", "Totally b0rked");
                                    break;
                                case msopathInvalid:
                                    SAL_WARN("oox", "Invalid - should never be found");
                                    break;
                            }
                        }
                        OString pathString = aPath.makeStringAndClear();
                        if ( !pathString.isEmpty() && pathString != "xe" )
                            m_pShapeAttrList->add( XML_path, pathString );
                    }
                    else
                        SAL_WARN("oox.vml", "unhandled shape path, missing either pVertices or pSegmentInfo.");
                }
                bAlreadyWritten[ ESCHER_Prop_pVertices ] = true;
                bAlreadyWritten[ ESCHER_Prop_pSegmentInfo ] = true;
                break;

            case ESCHER_Prop_fillType: // 384
            case ESCHER_Prop_fillColor: // 385
            case ESCHER_Prop_fillBackColor: // 387
            case ESCHER_Prop_fillBlip: // 390
            case ESCHER_Prop_fNoFillHitTest: // 447
            case ESCHER_Prop_fillOpacity: // 386
                {
                    sal_uInt32 nValue;
                    rtl::Reference<sax_fastparser::FastAttributeList> pAttrList
                        = FastSerializerHelper::createAttrList();

                    bool imageData = false;
                    EscherPropSortStruct aStruct;
                    const SdrGrafObj* pSdrGrafObj = dynamic_cast<const SdrGrafObj*>(m_pSdrObject);

                    if (pSdrGrafObj && pSdrGrafObj->isSignatureLine() && m_pTextExport)
                    {
                        rtl::Reference<sax_fastparser::FastAttributeList> pAttrListSignatureLine
                            = FastSerializerHelper::createAttrList();
                        pAttrListSignatureLine->add(XML_issignatureline, "t");
                        if (!pSdrGrafObj->getSignatureLineId().isEmpty())
                        {
                            pAttrListSignatureLine->add(
                                XML_id, pSdrGrafObj->getSignatureLineId());
                        }
                        if (!pSdrGrafObj->getSignatureLineSuggestedSignerName().isEmpty())
                        {
                            pAttrListSignatureLine->add(
                                FSNS(XML_o, XML_suggestedsigner),
                                pSdrGrafObj->getSignatureLineSuggestedSignerName());
                        }
                        if (!pSdrGrafObj->getSignatureLineSuggestedSignerTitle().isEmpty())
                        {
                            pAttrListSignatureLine->add(
                                FSNS(XML_o, XML_suggestedsigner2),
                                pSdrGrafObj->getSignatureLineSuggestedSignerTitle());
                        }
                        if (!pSdrGrafObj->getSignatureLineSuggestedSignerEmail().isEmpty())
                        {
                            pAttrListSignatureLine->add(
                                FSNS(XML_o, XML_suggestedsigneremail),
                                pSdrGrafObj->getSignatureLineSuggestedSignerEmail());
                        }
                        if (!pSdrGrafObj->getSignatureLineSigningInstructions().isEmpty())
                        {
                            pAttrListSignatureLine->add(XML_signinginstructionsset, "t");
                            pAttrListSignatureLine->add(
                                FSNS(XML_o, XML_signinginstructions),
                                pSdrGrafObj->getSignatureLineSigningInstructions());
                        }
                        pAttrListSignatureLine->add(
                            XML_showsigndate,
                            pSdrGrafObj->isSignatureLineShowSignDate() ? "t" : "f");
                        pAttrListSignatureLine->add(
                            XML_allowcomments,
                            pSdrGrafObj->isSignatureLineCanAddComment() ? "t" : "f");

                        m_pSerializer->singleElementNS(
                            XML_o, XML_signatureline,
                            pAttrListSignatureLine);

                        // Get signature line graphic
                        const uno::Reference<graphic::XGraphic>& xGraphic
                            = pSdrGrafObj->getSignatureLineUnsignedGraphic();
                        Graphic aGraphic(xGraphic);
                        OUString aImageId = m_pTextExport->GetDrawingML().writeGraphicToStorage(aGraphic, false);
                        pAttrList->add(FSNS(XML_r, XML_id), aImageId);
                        imageData = true;
                    }
                    else if (rProps.GetOpt(ESCHER_Prop_fillBlip, aStruct) && m_pTextExport)
                    {
                        SvMemoryStream aStream;
                        // The first bytes are WW8-specific, we're only interested in the PNG
                        int nHeaderSize = 25;
                        aStream.WriteBytes(aStruct.nProp.data() + nHeaderSize,
                                           aStruct.nProp.size() - nHeaderSize);
                        aStream.Seek(0);
                        Graphic aGraphic;
                        GraphicConverter::Import(aStream, aGraphic);
                        OUString aImageId = m_pTextExport->GetDrawingML().writeGraphicToStorage(aGraphic, false);
                        if (!aImageId.isEmpty())
                        {
                            pAttrList->add(FSNS(XML_r, XML_id), aImageId);
                            imageData = true;
                        }
                    }

                    if (rProps.GetOpt(ESCHER_Prop_fNoFillHitTest, nValue))
                        impl_AddBool(pAttrList.get(), FSNS(XML_o, XML_detectmouseclick), nValue != 0);

                    if (imageData && ((pSdrGrafObj && pSdrGrafObj->isSignatureLine())
                        || m_nShapeType == ESCHER_ShpInst_PictureFrame))
                        m_pSerializer->singleElementNS( XML_v, XML_imagedata, pAttrList );
                    else
                    {
                        if ( rProps.GetOpt( ESCHER_Prop_fillType, nValue ) )
                        {
                            const char *pFillType = nullptr;
                            switch ( nValue )
                            {
                                case ESCHER_FillSolid:       pFillType = "solid"; break;
                                // TODO case ESCHER_FillPattern:     pFillType = ""; break;
                                case ESCHER_FillTexture:     pFillType = "tile"; break;
                                case ESCHER_FillPicture:     pFillType = "frame"; break;
                                // TODO case ESCHER_FillShade:       pFillType = ""; break;
                                // TODO case ESCHER_FillShadeCenter: pFillType = ""; break;
                                // TODO case ESCHER_FillShadeShape:  pFillType = ""; break;
                                // TODO case ESCHER_FillShadeScale:  pFillType = ""; break;
                                // TODO case ESCHER_FillShadeTitle:  pFillType = ""; break;
                                // TODO case ESCHER_FillBackground:  pFillType = ""; break;
                                default:
                                    SAL_INFO("oox.vml", "Unhandled fill type: " << nValue);
                                    break;
                            }
                            if ( pFillType )
                                pAttrList->add( XML_type, pFillType );
                        }
                        else if (!rProps.GetOpt(ESCHER_Prop_fillColor, nValue))
                            pAttrList->add( XML_on, "false" );

                        if ( rProps.GetOpt( ESCHER_Prop_fillColor, nValue ) )
                            impl_AddColor( m_pShapeAttrList.get(), XML_fillcolor, nValue );

                        if ( rProps.GetOpt( ESCHER_Prop_fillBackColor, nValue ) )
                            impl_AddColor( pAttrList.get(), XML_color2, nValue );

                        if (rProps.GetOpt(ESCHER_Prop_fillOpacity, nValue))
                            // Partly undo the transformation at the end of EscherPropertyContainer::CreateFillProperties(): VML opacity is 0..1.
                            pAttrList->add(XML_opacity, OString::number(double((nValue * 100) >> 16) / 100));
                        m_pSerializer->singleElementNS( XML_v, XML_fill, pAttrList );

                    }
                }
                bAlreadyWritten[ ESCHER_Prop_fillType ] = true;
                bAlreadyWritten[ ESCHER_Prop_fillColor ] = true;
                bAlreadyWritten[ ESCHER_Prop_fillBackColor ] = true;
                bAlreadyWritten[ ESCHER_Prop_fillBlip ] = true;
                bAlreadyWritten[ ESCHER_Prop_fNoFillHitTest ] = true;
                bAlreadyWritten[ ESCHER_Prop_fillOpacity ] = true;
                break;

            case ESCHER_Prop_lineColor: // 448
            case ESCHER_Prop_lineWidth: // 459
            case ESCHER_Prop_lineDashing: // 462
            case ESCHER_Prop_lineStartArrowhead: // 464
            case ESCHER_Prop_lineEndArrowhead: // 465
            case ESCHER_Prop_lineStartArrowWidth: // 466
            case ESCHER_Prop_lineStartArrowLength: // 467
            case ESCHER_Prop_lineEndArrowWidth: // 468
            case ESCHER_Prop_lineEndArrowLength: // 469
            case ESCHER_Prop_lineJoinStyle: // 470
            case ESCHER_Prop_lineEndCapStyle: // 471
                {
                    sal_uInt32 nValue;
                    rtl::Reference<sax_fastparser::FastAttributeList> pAttrList = FastSerializerHelper::createAttrList();

                    if ( rProps.GetOpt( ESCHER_Prop_lineColor, nValue ) )
                        impl_AddColor( pAttrList.get(), XML_color, nValue );

                    if ( rProps.GetOpt( ESCHER_Prop_lineWidth, nValue ) )
                        impl_AddInt( pAttrList.get(), XML_weight, nValue );

                    if ( rProps.GetOpt( ESCHER_Prop_lineDashing, nValue ) )
                    {
                        const char *pDashStyle = nullptr;
                        switch ( nValue )
                        {
                            case ESCHER_LineSolid:             pDashStyle = "solid"; break;
                            case ESCHER_LineDashSys:           pDashStyle = "shortdash"; break;
                            case ESCHER_LineDotSys:            pDashStyle = "shortdot"; break;
                            case ESCHER_LineDashDotSys:        pDashStyle = "shortdashdot"; break;
                            case ESCHER_LineDashDotDotSys:     pDashStyle = "shortdashdotdot"; break;
                            case ESCHER_LineDotGEL:            pDashStyle = "dot"; break;
                            case ESCHER_LineDashGEL:           pDashStyle = "dash"; break;
                            case ESCHER_LineLongDashGEL:       pDashStyle = "longdash"; break;
                            case ESCHER_LineDashDotGEL:        pDashStyle = "dashdot"; break;
                            case ESCHER_LineLongDashDotGEL:    pDashStyle = "longdashdot"; break;
                            case ESCHER_LineLongDashDotDotGEL: pDashStyle = "longdashdotdot"; break;
                        }
                        if ( pDashStyle )
                            pAttrList->add( XML_dashstyle, pDashStyle );
                    }

                    if ( rProps.GetOpt( ESCHER_Prop_lineStartArrowhead, nValue ) )
                        impl_AddArrowHead( pAttrList.get(), XML_startarrow, nValue );

                    if ( rProps.GetOpt( ESCHER_Prop_lineEndArrowhead, nValue ) )
                        impl_AddArrowHead( pAttrList.get(), XML_endarrow, nValue );

                    if ( rProps.GetOpt( ESCHER_Prop_lineStartArrowWidth, nValue ) )
                        impl_AddArrowWidth( pAttrList.get(), XML_startarrowwidth, nValue );

                    if ( rProps.GetOpt( ESCHER_Prop_lineStartArrowLength, nValue ) )
                        impl_AddArrowLength( pAttrList.get(), XML_startarrowlength, nValue );

                    if ( rProps.GetOpt( ESCHER_Prop_lineEndArrowWidth, nValue ) )
                        impl_AddArrowWidth( pAttrList.get(), XML_endarrowwidth, nValue );

                    if ( rProps.GetOpt( ESCHER_Prop_lineEndArrowLength, nValue ) )
                        impl_AddArrowLength( pAttrList.get(), XML_endarrowlength, nValue );

                    if ( rProps.GetOpt( ESCHER_Prop_lineJoinStyle, nValue ) )
                    {
                        const char *pJoinStyle = nullptr;
                        switch ( nValue )
                        {
                            case ESCHER_LineJoinBevel: pJoinStyle = "bevel"; break;
                            case ESCHER_LineJoinMiter: pJoinStyle = "miter"; break;
                            case ESCHER_LineJoinRound: pJoinStyle = "round"; break;
                        }
                        if ( pJoinStyle )
                            pAttrList->add( XML_joinstyle, pJoinStyle );
                    }

                    if ( rProps.GetOpt( ESCHER_Prop_lineEndCapStyle, nValue ) )
                    {
                        const char *pEndCap = nullptr;
                        switch ( nValue )
                        {
                            case ESCHER_LineEndCapRound:  pEndCap = "round"; break;
                            case ESCHER_LineEndCapSquare: pEndCap = "square"; break;
                            case ESCHER_LineEndCapFlat:   pEndCap = "flat"; break;
                        }
                        if ( pEndCap )
                            pAttrList->add( XML_endcap, pEndCap );
                    }

                    m_pSerializer->singleElementNS( XML_v, XML_stroke, pAttrList );
                }
                bAlreadyWritten[ ESCHER_Prop_lineColor ] = true;
                bAlreadyWritten[ ESCHER_Prop_lineWidth ] = true;
                bAlreadyWritten[ ESCHER_Prop_lineDashing ] = true;
                bAlreadyWritten[ ESCHER_Prop_lineStartArrowhead ] = true;
                bAlreadyWritten[ ESCHER_Prop_lineEndArrowhead ] = true;
                bAlreadyWritten[ ESCHER_Prop_lineStartArrowWidth ] = true;
                bAlreadyWritten[ ESCHER_Prop_lineStartArrowLength ] = true;
                bAlreadyWritten[ ESCHER_Prop_lineEndArrowWidth ] = true;
                bAlreadyWritten[ ESCHER_Prop_lineEndArrowLength ] = true;
                bAlreadyWritten[ ESCHER_Prop_lineJoinStyle ] = true;
                bAlreadyWritten[ ESCHER_Prop_lineEndCapStyle ] = true;
                break;

            case ESCHER_Prop_fHidden:
                if ( !opt.nPropValue )
                    m_ShapeStyle.append( ";visibility:hidden" );
                break;
            case ESCHER_Prop_shadowColor:
            case ESCHER_Prop_fshadowObscured:
                {
                    sal_uInt32 nValue = 0;
                    bool bShadow = false;
                    bool bObscured = false;
                    if ( rProps.GetOpt( ESCHER_Prop_fshadowObscured, nValue ) )
                    {
                        bShadow = (( nValue & 0x20002 ) == 0x20002 );
                        bObscured = (( nValue & 0x10001 ) == 0x10001 );
                    }
                    if ( bShadow )
                    {
                        rtl::Reference<sax_fastparser::FastAttributeList> pAttrList = FastSerializerHelper::createAttrList();
                        impl_AddBool( pAttrList.get(), XML_on, bShadow );
                        impl_AddBool( pAttrList.get(), XML_obscured, bObscured );

                        if ( rProps.GetOpt( ESCHER_Prop_shadowColor, nValue ) )
                            impl_AddColor( pAttrList.get(), XML_color, nValue );

                        m_pSerializer->singleElementNS( XML_v, XML_shadow, pAttrList );
                        bAlreadyWritten[ ESCHER_Prop_fshadowObscured ] = true;
                        bAlreadyWritten[ ESCHER_Prop_shadowColor ] = true;
                    }
                }
                break;
            case ESCHER_Prop_gtextUNICODE:
            case ESCHER_Prop_gtextFont:
                {
                    EscherPropSortStruct aUnicode;
                    if (rProps.GetOpt(ESCHER_Prop_gtextUNICODE, aUnicode))
                    {
                        SvMemoryStream aStream;

                        if(!opt.nProp.empty())
                        {
                            aStream.WriteBytes(opt.nProp.data(), opt.nProp.size());
                        }

                        aStream.Seek(0);
                        OUString aTextPathString = SvxMSDffManager::MSDFFReadZString(aStream, opt.nProp.size(), true);
                        aStream.Seek(0);

                        m_pSerializer->singleElementNS(XML_v, XML_path, XML_textpathok, "t");

                        rtl::Reference<sax_fastparser::FastAttributeList> pAttrList = FastSerializerHelper::createAttrList();
                        pAttrList->add(XML_on, "t");
                        pAttrList->add(XML_fitshape, "t");
                        pAttrList->add(XML_string, aTextPathString);
                        EscherPropSortStruct aFont;
                        OUString aStyle;
                        if (rProps.GetOpt(ESCHER_Prop_gtextFont, aFont))
                        {
                            aStream.WriteBytes(aFont.nProp.data(), aFont.nProp.size());
                            aStream.Seek(0);
                            OUString aTextPathFont = SvxMSDffManager::MSDFFReadZString(aStream, aFont.nProp.size(), true);
                            aStyle += "font-family:\"" + aTextPathFont + "\"";
                        }
                        sal_uInt32 nSize;
                        if (rProps.GetOpt(ESCHER_Prop_gtextSize, nSize))
                        {
                            float nSizeF = static_cast<sal_Int32>(nSize) / 65536.0;
                            OUString aSize = OUString::number(nSizeF);
                            aStyle += ";font-size:" + aSize + "pt";
                        }

                        sal_uInt32 nGtextFlags;
                        if (rProps.GetOpt(DFF_Prop_gtextFStrikethrough /*255*/, nGtextFlags))
                        {
                            // The property is in fact a collection of flags. Two bytes contain the
                            // fUsegtextF* flags and the other two bytes at same place the associated
                            // On/Off flags. See '2.3.22.10 Geometry Text Boolean Properties' section
                            // in [MS-ODRAW].
                            if ((nGtextFlags & 0x00200020) == 0x00200020) // DFF_Prop_gtextFBold = 250
                                aStyle += ";font-weight:bold";
                            if ((nGtextFlags & 0x00100010) == 0x00100010) // DFF_Prop_gtextFItalic = 251
                                aStyle += ";font-style:italic";
                            if ((nGtextFlags & 0x00800080) == 0x00800080) // no DFF, PID gtextFNormalize = 248
                                aStyle += ";v-same-letter-heights:t";

                            // The value 'Fontwork character spacing' in LO is bound to field 'Scaling'
                            // not to 'Spacing' in character properties. In fact the characters are
                            // rendered with changed distance and width. The method in escherex.cxx has
                            // put a rounded value of 'CharScaleWidth' API property to
                            // DFF_Prop_gtextSpacing (=196) as integer part of 16.16 fixed point format.
                            // fUsegtextFTight and gtextFTight (244) of MS binary format are not used.
                            sal_uInt32 nGtextSpacing;
                            if (rProps.GetOpt(DFF_Prop_gtextSpacing, nGtextSpacing))
                                aStyle += ";v-text-spacing:" + OUString::number(nGtextSpacing) + "f";
                        }

                        if (!aStyle.isEmpty())
                            pAttrList->add(XML_style, aStyle);

                        // tdf#153260. LO renders all Fontwork shapes as if trim="t" is set. Default
                        // value is "f". So always write out "t", otherwise import will reduce the
                        // shape height as workaround for "f".
                        pAttrList->add(XML_trim, "t");

                        m_pSerializer->singleElementNS(XML_v, XML_textpath, pAttrList);
                    }

                    bAlreadyWritten[ESCHER_Prop_gtextUNICODE] = true;
                    bAlreadyWritten[ESCHER_Prop_gtextFont] = true;
                }
                break;
            case DFF_Prop_adjustValue:
            case DFF_Prop_adjust2Value:
                {
                    // FIXME: tdf#153296: The currently exported markup for <v:shapetype> is based on
                    // OOXML presets and unusable in regard to handles. Fontwork shapes use dedicated
                    // own markup, see FontworkHelpers::GetVMLFontworkShapetypeMarkup.
                    // Thus this is restricted to preset Fontwork shapes. Such have maximal two
                    // adjustment values.
                    if ((mso_sptTextSimple <= m_nShapeType && m_nShapeType <= mso_sptTextOnRing)
                        || (mso_sptTextPlainText <= m_nShapeType && m_nShapeType <= mso_sptTextCanDown))
                    {
                        sal_uInt32 nValue;
                        OString sAdj;
                        if (rProps.GetOpt(DFF_Prop_adjustValue, nValue))
                        {
                            sAdj = OString::number(static_cast<sal_Int32>(nValue));
                            if (rProps.GetOpt(DFF_Prop_adjust2Value, nValue))
                                sAdj += "," + OString::number(static_cast<sal_Int32>(nValue));
                        }
                        if (!sAdj.isEmpty())
                            m_pShapeAttrList->add(XML_adj, sAdj);
                        bAlreadyWritten[DFF_Prop_adjustValue] = true;
                        bAlreadyWritten[DFF_Prop_adjust2Value] = true;
                    }
                }
                break;
            case ESCHER_Prop_Rotation:
                {
                    // The higher half of the variable contains the angle.
                    m_ShapeStyle.append(";rotation:" + OString::number(double(opt.nPropValue >> 16)));
                    bAlreadyWritten[ESCHER_Prop_Rotation] = true;
                }
                break;
            case ESCHER_Prop_fNoLineDrawDash:
                {
                    // See DffPropertyReader::ApplyLineAttributes().
                    impl_AddBool( m_pShapeAttrList.get(), XML_stroked, (opt.nPropValue & 8) != 0 );
                    bAlreadyWritten[ESCHER_Prop_fNoLineDrawDash] = true;
                }
                break;
            case ESCHER_Prop_wzName:
                {
                    SvMemoryStream aStream;

                    if(!opt.nProp.empty())
                    {
                        aStream.WriteBytes(opt.nProp.data(), opt.nProp.size());
                    }

                    aStream.Seek(0);
                    OUString idStr = SvxMSDffManager::MSDFFReadZString(aStream, opt.nProp.size(), true);
                    aStream.Seek(0);
                    if (!IsWaterMarkShape(m_pSdrObject->GetName()) && !m_bSkipwzName)
                         m_pShapeAttrList->add(XML_ID, idStr);

                    // note that XML_ID is different from XML_id (although it looks like a LO
                    // implementation distinction without valid justification to me).
                    // FIXME: XML_ID produces invalid file, see tdf#153183
                    bAlreadyWritten[ESCHER_Prop_wzName] = true;
                }
                break;
            default:
#if OSL_DEBUG_LEVEL > 0
                const size_t opt_nProp_size(opt.nProp.size());
                SAL_WARN( "oox.vml", "TODO VMLExport::Commit(), unimplemented id: " << nId
                        << ", value: " << opt.nPropValue
                        << ", data: [" << opt_nProp_size << "]");
                if ( opt.nProp.size() )
                {
                    const sal_uInt8 *pIt = opt.nProp.data();
                    OStringBuffer buf( "    ( " );
                    for ( int nCount = opt.nProp.size(); nCount; --nCount )
                    {
                        buf.append( OString::number(static_cast<sal_Int32>(*pIt), 16) + " ");
                        ++pIt;
                    }
                    buf.append( ")" );
                    SAL_WARN("oox.vml", std::string_view(buf));
                }
#endif
                break;
        }
    }

    m_pSerializer->mergeTopMarks(Tag_Commit, sax_fastparser::MergeMarks::POSTPONE );
}

OString VMLExport::ShapeIdString( sal_uInt32 nId )
{
    if(m_bOverrideShapeIdGeneration)
        return m_sShapeIDPrefix + OString::number( nId );
    else
        return "shape_" + OString::number( nId );
}

void VMLExport::AddFlipXY( )
{
    if (m_nShapeFlags & (ShapeFlag::FlipH | ShapeFlag::FlipV))
    {
        m_ShapeStyle.append( ";flip:" );

        if (m_nShapeFlags & ShapeFlag::FlipH)
            m_ShapeStyle.append( "x" );

        if (m_nShapeFlags & ShapeFlag::FlipV)
            m_ShapeStyle.append( "y" );
    }
}

void VMLExport::AddLineDimensions( const tools::Rectangle& rRectangle )
{
    // style
    if (!m_ShapeStyle.isEmpty())
        m_ShapeStyle.append( ";" );

    m_ShapeStyle.append( "position:absolute" );

    AddFlipXY();

    // the actual dimensions
    OString aLeft, aTop, aRight, aBottom;

    if ( mnGroupLevel == 1 )
    {
        static constexpr OString aPt( "pt"_ostr );
        aLeft = OString::number( double( rRectangle.Left() ) / 20 ) + aPt;
        aTop = OString::number( double( rRectangle.Top() ) / 20 ) + aPt;
        aRight = OString::number( double( rRectangle.Right() ) / 20 ) + aPt;
        aBottom = OString::number( double( rRectangle.Bottom() ) / 20 ) + aPt;
    }
    else
    {
        aLeft = OString::number( rRectangle.Left() );
        aTop = OString::number( rRectangle.Top() );
        aRight = OString::number( rRectangle.Right() );
        aBottom = OString::number( rRectangle.Bottom() );
    }

    m_pShapeAttrList->add( XML_from, aLeft + "," + aTop );

    m_pShapeAttrList->add( XML_to, aRight + "," + aBottom );
}

void VMLExport::AddRectangleDimensions( OStringBuffer& rBuffer, const tools::Rectangle& rRectangle, bool rbAbsolutePos)
{
    if ( !rBuffer.isEmpty() )
        rBuffer.append( ";" );

    if (rbAbsolutePos && !m_bInline)
    {
        rBuffer.append( "position:absolute;" );
    }

    if(m_bInline)
    {
        rBuffer.append( "width:" + OString::number( double( rRectangle.Right() - rRectangle.Left() ) / 20 ) +
            "pt;height:" + OString::number( double( rRectangle.Bottom() - rRectangle.Top() ) / 20 ) +
            "pt" );
    }
    else if ( mnGroupLevel == 1 )
    {
        rBuffer.append( "margin-left:" + OString::number( double( rRectangle.Left() ) / 20 ) +
            "pt;margin-top:" + OString::number( double( rRectangle.Top() ) / 20 ) +
            "pt;width:" + OString::number( double( rRectangle.Right() - rRectangle.Left() ) / 20 ) +
            "pt;height:" + OString::number( double( rRectangle.Bottom() - rRectangle.Top() ) / 20 ) +
            "pt" );
    }
    else
    {
        rBuffer.append( "left:" + OString::number( rRectangle.Left() ) +
            ";top:" + OString::number( rRectangle.Top() ) +
            ";width:" + OString::number( rRectangle.Right() - rRectangle.Left() ) +
            ";height:" + OString::number( rRectangle.Bottom() - rRectangle.Top() ) );
    }

    AddFlipXY();
}

void VMLExport::AddShapeAttribute( sal_Int32 nAttribute, std::string_view rValue )
{
    m_pShapeAttrList->add( nAttribute, rValue );
}

static std::vector<OString> lcl_getShapeTypes()
{
    std::vector<OString> aRet;

    OUString aPath(u"$BRAND_BASE_DIR/" LIBO_SHARE_FOLDER "/filter/vml-shape-types"_ustr);
    rtl::Bootstrap::expandMacros(aPath);
    SvFileStream aStream(aPath, StreamMode::READ);
    if (aStream.GetError() != ERRCODE_NONE)
        SAL_WARN("oox", "failed to open vml-shape-types");
    OStringBuffer aLine;
    bool bNotDone = aStream.ReadLine(aLine);
    while (bNotDone)
    {
        // Filter out comments.
        if (!o3tl::starts_with(aLine, "/"))
            aRet.push_back(OString(aLine));
        bNotDone = aStream.ReadLine(aLine);
    }
    return aRet;
}

static bool lcl_isTextBox(const SdrObject* pSdrObject)
{
    uno::Reference<beans::XPropertySet> xPropertySet(const_cast<SdrObject*>(pSdrObject)->getUnoShape(), uno::UNO_QUERY);
    if (!xPropertySet.is())
        return false;
    uno::Reference<beans::XPropertySetInfo> xPropertySetInfo = xPropertySet->getPropertySetInfo();
    if (!xPropertySetInfo->hasPropertyByName(u"TextBox"_ustr))
       return false;
    css::uno::Any aTextBox(xPropertySet->getPropertyValue(u"TextBox"_ustr));
    if (!aTextBox.hasValue())
       return false;
    return aTextBox.get<bool>();
}

static OUString lcl_getAnchorIdFromGrabBag(const SdrObject* pSdrObject)
{
    OUString aResult;

    uno::Reference<beans::XPropertySet> xShape(const_cast<SdrObject*>(pSdrObject)->getUnoShape(), uno::UNO_QUERY);
    if (xShape->getPropertySetInfo()->hasPropertyByName(u"InteropGrabBag"_ustr))
    {
        comphelper::SequenceAsHashMap aInteropGrabBag(xShape->getPropertyValue(u"InteropGrabBag"_ustr));
        auto it = aInteropGrabBag.find(u"AnchorId"_ustr);
        if (it != aInteropGrabBag.end())
            it->second >>= aResult;
    }

    return aResult;
}

sal_uInt32 VMLExport::GenerateShapeId()
{
    if(!m_bOverrideShapeIdGeneration)
        return EscherEx::GenerateShapeId();
    else
        return m_nShapeIDCounter++;
}

OString VMLExport::GetVMLShapeTypeDefinition(
    std::string_view sShapeID, const bool bIsPictureFrame )
{
    OString sShapeType;
    if ( !bIsPictureFrame )
        // We don't have a shape definition for host control in presetShapeDefinitions.xml
        // So use a definition copied from DOCX file created with MSO
        sShapeType = OString::Concat("<v:shapetype id=\"_x0000_t") + sShapeID +
                        "\" coordsize=\"21600,21600\" o:spt=\"" + sShapeID +
                        "\" path=\"m,l,21600l21600,21600l21600,xe\">\n"
                        "<v:stroke joinstyle=\"miter\"/>\n"
                        "<v:path shadowok=\"f\" o:extrusionok=\"f\" strokeok=\"f\" fillok=\"f\" o:connecttype=\"rect\"/>\n"
                        "<o:lock v:ext=\"edit\" shapetype=\"t\"/>\n"
                    "</v:shapetype>";
    else
        // We don't have a shape definition for picture frame in presetShapeDefinitions.xml
        // So use a definition copied from DOCX file created with MSO
        sShapeType = OString::Concat("<v:shapetype id=\"_x0000_t") + sShapeID +
                        "\" coordsize=\"21600,21600\" o:spt=\"" + sShapeID +
                        "\" o:preferrelative=\"t\" path=\"m@4@5l@4@11@9@11@9@5xe\" filled=\"f\" stroked=\"f\">\n"
                        "<v:stroke joinstyle=\"miter\"/>\n"
                        "<v:formulas>\n"
                            "<v:f eqn=\"if lineDrawn pixelLineWidth 0\"/>\n"
                            "<v:f eqn=\"sum @0 1 0\"/>\n"
                            "<v:f eqn=\"sum 0 0 @1\"/>\n"
                            "<v:f eqn=\"prod @2 1 2\"/>\n"
                            "<v:f eqn=\"prod @3 21600 pixelWidth\"/>\n"
                            "<v:f eqn=\"prod @3 21600 pixelHeight\"/>\n"
                            "<v:f eqn=\"sum @0 0 1\"/>\n"
                            "<v:f eqn=\"prod @6 1 2\"/>\n"
                            "<v:f eqn=\"prod @7 21600 pixelWidth\"/>\n"
                            "<v:f eqn=\"sum @8 21600 0\"/>\n"
                            "<v:f eqn=\"prod @7 21600 pixelHeight\"/>\n"
                            "<v:f eqn=\"sum @10 21600 0\"/>\n"
                        "</v:formulas>\n"
                        "<v:path o:extrusionok=\"f\" gradientshapeok=\"t\" o:connecttype=\"rect\"/>\n"
                        "<o:lock v:ext=\"edit\" aspectratio=\"t\"/>\n"
                        "</v:shapetype>";
    return sShapeType;
}

sal_Int32 VMLExport::StartShape()
{
    if ( m_nShapeType == ESCHER_ShpInst_Nil )
        return -1;

    // some of the shapes have their own name ;-)
    sal_Int32 nShapeElement = -1;
    bool bReferToShapeType = false;
    switch ( m_nShapeType )
    {
        case ESCHER_ShpInst_NotPrimitive:   nShapeElement = XML_shape;     break;
        case ESCHER_ShpInst_Rectangle:      nShapeElement = XML_rect;      break;
        case ESCHER_ShpInst_RoundRectangle: nShapeElement = XML_roundrect; break;
        case ESCHER_ShpInst_Ellipse:        nShapeElement = XML_oval;      break;
        case ESCHER_ShpInst_Arc:            nShapeElement = XML_arc;       break;
        case ESCHER_ShpInst_Line:           nShapeElement = XML_line;      break;
        case ESCHER_ShpInst_HostControl:
        {
            bReferToShapeType = true;
            nShapeElement = XML_shape;
            if ( !m_aShapeTypeWritten[ m_nShapeType ] )
            {
                m_pSerializer->write(GetVMLShapeTypeDefinition(OString::number(m_nShapeType), false));
                m_aShapeTypeWritten[ m_nShapeType ] = true;
            }
            break;
        }
        case ESCHER_ShpInst_PictureFrame:
        {
            bReferToShapeType = true;
            nShapeElement = XML_shape;
            if ( !m_aShapeTypeWritten[ m_nShapeType ] )
            {
                m_pSerializer->write(GetVMLShapeTypeDefinition(OString::number(m_nShapeType), true));
                m_aShapeTypeWritten[ m_nShapeType ] = true;
            }
            break;
        }
        default:
            nShapeElement = XML_shape;
            if (m_pSdrObject->IsTextPath())
            {
                bReferToShapeType = m_aShapeTypeWritten[m_nShapeType];
                if (!bReferToShapeType)
                {
                    // Does a predefined markup exist at all?
                    OString sMarkup = FontworkHelpers::GetVMLFontworkShapetypeMarkup(
                        static_cast<MSO_SPT>(m_nShapeType));
                    if (!sMarkup.isEmpty())
                    {
                        m_pSerializer->write(sMarkup);
                        m_aShapeTypeWritten[m_nShapeType] = true;
                        bReferToShapeType = true;
                    }
                }
                // ToDo: The case bReferToShapeType==false happens for 'non-primitive' shapes for
                // example. We need to get the geometry from CustomShapeGeometry in these cases.
            }
            else if ( m_nShapeType < ESCHER_ShpInst_COUNT )
            {
                // a predefined shape?
                static std::vector<OString> aShapeTypes = lcl_getShapeTypes();
                SAL_WARN_IF(m_nShapeType >= aShapeTypes.size(), "oox.vml", "Unknown shape type!");
                if (m_nShapeType < aShapeTypes.size() && aShapeTypes[m_nShapeType] != "NULL")
                {
                    bReferToShapeType = true;
                    if ( !m_aShapeTypeWritten[ m_nShapeType ] )
                    {
                        m_pSerializer->write(aShapeTypes[m_nShapeType]);
                        m_aShapeTypeWritten[ m_nShapeType ] = true;
                    }
                }
                else
                {
                    // rectangle is probably the best fallback...
                    nShapeElement = XML_rect;
                }
            }
            break;
    }

    // anchoring
    switch (m_eHOri)
    {
        case text::HoriOrientation::LEFT:
            m_ShapeStyle.append(";mso-position-horizontal:left");
            break;
        case text::HoriOrientation::CENTER:
            m_ShapeStyle.append(";mso-position-horizontal:center");
            break;
        case text::HoriOrientation::RIGHT:
            m_ShapeStyle.append(";mso-position-horizontal:right");
            break;
        case text::HoriOrientation::INSIDE:
            m_ShapeStyle.append(";mso-position-horizontal:inside");
            break;
        case text::HoriOrientation::OUTSIDE:
            m_ShapeStyle.append(";mso-position-horizontal:outside");
            break;
        default:
        case text::HoriOrientation::NONE:
            break;
    }
    switch (m_eHRel)
    {
        case text::RelOrientation::PAGE_PRINT_AREA:
            m_ShapeStyle.append(";mso-position-horizontal-relative:margin");
            break;
        case text::RelOrientation::PAGE_FRAME:
        case text::RelOrientation::PAGE_LEFT:
        case text::RelOrientation::PAGE_RIGHT:
            m_ShapeStyle.append(";mso-position-horizontal-relative:page");
            break;
        case text::RelOrientation::CHAR:
            m_ShapeStyle.append(";mso-position-horizontal-relative:char");
            break;
        default:
            break;
    }

    switch (m_eVOri)
    {
        case text::VertOrientation::TOP:
        case text::VertOrientation::LINE_TOP:
        case text::VertOrientation::CHAR_TOP:
            m_ShapeStyle.append(";mso-position-vertical:top");
            break;
        case text::VertOrientation::CENTER:
        case text::VertOrientation::LINE_CENTER:
            m_ShapeStyle.append(";mso-position-vertical:center");
            break;
        case text::VertOrientation::BOTTOM:
        case text::VertOrientation::LINE_BOTTOM:
        case text::VertOrientation::CHAR_BOTTOM:
            m_ShapeStyle.append(";mso-position-vertical:bottom");
            break;
        default:
        case text::VertOrientation::NONE:
            break;
    }
    switch (m_eVRel)
    {
        case text::RelOrientation::PAGE_PRINT_AREA:
            m_ShapeStyle.append(";mso-position-vertical-relative:margin");
            break;
        case text::RelOrientation::PAGE_FRAME:
            m_ShapeStyle.append(";mso-position-vertical-relative:page");
            break;
        default:
            break;
    }

    if (!m_pSdrObject->getHyperlink().isEmpty())
        m_pShapeAttrList->add(
            XML_href, m_pSdrObject->getHyperlink());

    m_pShapeAttrList->addNS(XML_o, XML_allowincell, m_IsFollowingTextFlow ? "t" : "f");

    // add style
    m_pShapeAttrList->add( XML_style, m_ShapeStyle.makeStringAndClear() );

    OUString sAnchorId = lcl_getAnchorIdFromGrabBag(m_pSdrObject);
    if (!sAnchorId.isEmpty())
        m_pShapeAttrList->addNS(XML_wp14, XML_anchorId, sAnchorId);

    if ( nShapeElement >= 0 && !m_pShapeAttrList->hasAttribute( XML_type ) && bReferToShapeType )
    {
        OString sType;
        if (m_bUseHashMarkForType)
            sType = "#"_ostr;
        m_pShapeAttrList->add( XML_type, sType +
                "_x0000_t" + OString::number( m_nShapeType ) );
    }

    // allow legacy id (which in form controls and textboxes
    // by definition seems to have this otherwise illegal name).
    m_pSerializer->setAllowXEscape(!m_sShapeIDPrefix.startsWith("_x0000_"));

    // start of the shape
    m_pSerializer->startElementNS( XML_v, nShapeElement, m_pShapeAttrList );
    m_pSerializer->setAllowXEscape(true);

    OString const textboxStyle(m_TextboxStyle.makeStringAndClear());

    // now check if we have some editeng text (not associated textbox) and we have a text exporter registered
    const SdrTextObj* pTxtObj = DynCastSdrTextObj( m_pSdrObject );
    if (pTxtObj && m_pTextExport && !m_pSdrObject->IsTextPath()
        && !IsWaterMarkShape(m_pSdrObject->GetName()) && !lcl_isTextBox(m_pSdrObject))
    {
        std::optional<OutlinerParaObject> pParaObj;

        /*
        #i13885#
        When the object is actively being edited, that text is not set into
        the objects normal text object, but lives in a separate object.
        */
        if (pTxtObj->IsTextEditActive())
        {
            pParaObj = pTxtObj->CreateEditOutlinerParaObject();
        }
        else if (pTxtObj->GetOutlinerParaObject())
        {
            pParaObj = *pTxtObj->GetOutlinerParaObject();
        }

        if( pParaObj )
        {
            rtl::Reference<sax_fastparser::FastAttributeList> pTextboxAttrList = FastSerializerHelper::createAttrList();
            if (!textboxStyle.isEmpty())
            {
                pTextboxAttrList->add(XML_style, textboxStyle);
            }

            // this is reached only in case some text is attached to the shape
            m_pSerializer->startElementNS(XML_v, XML_textbox, pTextboxAttrList);
            m_pTextExport->WriteOutliner(*pParaObj);
            m_pSerializer->endElementNS(XML_v, XML_textbox);
        }
    }

    return nShapeElement;
}

void VMLExport::EndShape( sal_Int32 nShapeElement )
{
    if ( nShapeElement < 0 )
        return;

    if (m_pTextExport && lcl_isTextBox(m_pSdrObject))
    {
        uno::Reference<drawing::XShape> xShape {const_cast<SdrObject*>(m_pSdrObject)->getUnoShape(), uno::UNO_QUERY};
        uno::Reference<beans::XPropertySet> xPropertySet(xShape, uno::UNO_QUERY);
        uno::Reference<beans::XPropertySetInfo> xPropertySetInfo = xPropertySet->getPropertySetInfo();
        bool bBottomToTop = false;
        if (xPropertySetInfo->hasPropertyByName(u"CustomShapeGeometry"_ustr))
        {
            // In this case a DrawingML DOCX was imported.
            auto aAny = xPropertySet->getPropertyValue(u"WritingMode"_ustr);
            sal_Int16 nWritingMode;
            if ((aAny >>= nWritingMode) && nWritingMode == text::WritingMode2::BT_LR)
                bBottomToTop = true;
        }
        else
        {
            // In this case a pure VML DOCX was imported, so there is no CustomShapeGeometry.
            auto pTextExport = m_pTextExport->GetDrawingML().GetTextExport();
            // FIXME: somewhy pTextExport is always nullptr, we should find its reason
            if (pTextExport)
            {
                auto xTextFrame = pTextExport->GetUnoTextFrame(xShape);
                uno::Reference<beans::XPropertySet> xPropSet(xTextFrame, uno::UNO_QUERY);
                auto aAny = xPropSet->getPropertyValue(u"WritingMode"_ustr);
                sal_Int16 nWritingMode;
                if (aAny >>= nWritingMode)
                {
                    switch (nWritingMode)
                    {
                        case text::WritingMode2::BT_LR:
                            bBottomToTop = true;
                            break;
                        default:
                            break;
                    }
                }
            }
        }
        rtl::Reference<sax_fastparser::FastAttributeList> pTextboxAttrList = FastSerializerHelper::createAttrList();
        if (bBottomToTop)
            pTextboxAttrList->add(XML_style, "mso-layout-flow-alt:bottom-to-top");
        m_pSerializer->startElementNS(XML_v, XML_textbox, pTextboxAttrList);

        m_pTextExport->WriteVMLTextBox(uno::Reference<drawing::XShape>(xPropertySet, uno::UNO_QUERY_THROW));

        m_pSerializer->endElementNS(XML_v, XML_textbox);
    }

    if (m_pWrapAttrList)
    {
        m_pSerializer->singleElementNS(XML_w10, XML_wrap, m_pWrapAttrList);
    }

    // end of the shape
    m_pSerializer->endElementNS( XML_v, nShapeElement );
}

OString const & VMLExport::AddSdrObject( const SdrObject& rObj,
        bool const bIsFollowingTextFlow,
        sal_Int16 eHOri, sal_Int16 eVOri, sal_Int16 eHRel, sal_Int16 eVRel,
        FastAttributeList* pWrapAttrList,
        const bool bOOxmlExport, sal_uInt32 nId)
{
    m_pSdrObject = &rObj;
    m_eHOri = eHOri;
    m_eVOri = eVOri;
    m_eHRel = eHRel;
    m_eVRel = eVRel;
    m_pWrapAttrList = pWrapAttrList;
    m_bInline = false;
    m_IsFollowingTextFlow = bIsFollowingTextFlow;
    EscherEx::AddSdrObject(rObj, bOOxmlExport, nId);
    return m_sShapeId;
}

OString const & VMLExport::AddInlineSdrObject( const SdrObject& rObj, const bool bOOxmlExport )
{
    m_pSdrObject = &rObj;
    m_eHOri = -1;
    m_eVOri = -1;
    m_eHRel = -1;
    m_eVRel = -1;
    m_pWrapAttrList.clear();
    m_bInline = true;
    m_IsFollowingTextFlow = true;
    EscherEx::AddSdrObject(rObj, bOOxmlExport);
    return m_sShapeId;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
