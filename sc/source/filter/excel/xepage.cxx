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

#include <utility>
#include <xepage.hxx>
#include <svl/itemset.hxx>
#include <scitems.hxx>
#include <svl/eitem.hxx>
#include <svl/intitem.hxx>
#include <svx/pageitem.hxx>
#include <editeng/sizeitem.hxx>
#include <editeng/lrspitem.hxx>
#include <editeng/ulspitem.hxx>
#include <editeng/brushitem.hxx>
#include <oox/export/utils.hxx>
#include <oox/token/tokens.hxx>
#include <sax/fastattribs.hxx>
#include <document.hxx>
#include <stlpool.hxx>
#include <attrib.hxx>
#include <xehelper.hxx>
#include <xeescher.hxx>
#include <xltools.hxx>

#include <set>
#include <limits>

using namespace ::oox;

using ::std::set;
using ::std::numeric_limits;

// Page settings records ======================================================

// Header/footer --------------------------------------------------------------

XclExpHeaderFooter::XclExpHeaderFooter( sal_uInt16 nRecId, OUString aHdrString ) :
    XclExpRecord( nRecId ),
    maHdrString(std::move( aHdrString ))
{
}

void XclExpHeaderFooter::SaveXml( XclExpXmlStream& rStrm )
{
    sax_fastparser::FSHelperPtr& rWorksheet = rStrm.GetCurrentStream();
    sal_Int32 nElement;
    switch(GetRecId()) {
        case EXC_ID_HEADER_FIRST: nElement = XML_firstHeader; break;
        case EXC_ID_FOOTER_FIRST: nElement = XML_firstFooter; break;
        case EXC_ID_HEADER_EVEN:  nElement = XML_evenHeader; break;
        case EXC_ID_FOOTER_EVEN:  nElement = XML_evenFooter; break;
        case EXC_ID_HEADER:       nElement = XML_oddHeader; break;
        case EXC_ID_FOOTER:
        default:                  nElement = XML_oddFooter;
    }
    rWorksheet->startElement(nElement);
    rWorksheet->writeEscaped( maHdrString );
    rWorksheet->endElement( nElement );
}

void XclExpHeaderFooter::WriteBody( XclExpStream& rStrm )
{
    if( !maHdrString.isEmpty() )
    {
        XclExpString aExString;
        if( rStrm.GetRoot().GetBiff() <= EXC_BIFF5 )
            aExString.AssignByte( maHdrString, rStrm.GetRoot().GetTextEncoding(), XclStrFlags::EightBitLength );
        else
            aExString.Assign( maHdrString, XclStrFlags::NONE, 255 );  // 16-bit length, but max 255 chars
        rStrm << aExString;
    }
}

// General page settings ------------------------------------------------------

XclExpSetup::XclExpSetup( const XclPageData& rPageData ) :
    XclExpRecord( EXC_ID_SETUP, 34 ),
    mrData( rPageData )
{
}

void XclExpSetup::SaveXml( XclExpXmlStream& rStrm )
{
    rtl::Reference<sax_fastparser::FastAttributeList> pAttrList = sax_fastparser::FastSerializerHelper::createAttrList();
    if( rStrm.getVersion() != oox::core::ISOIEC_29500_2008 ||
        mrData.mnStrictPaperSize != EXC_PAPERSIZE_USER )
    {
        pAttrList->add( XML_paperSize,           OString::number(  mrData.mnPaperSize ) );
    }
    else
    {
        pAttrList->add( XML_paperWidth,     OString::number( mrData.mnPaperWidth ) + "mm" );
        pAttrList->add( XML_paperHeight,    OString::number( mrData.mnPaperHeight ) + "mm" );
        // pAttrList->add( XML_paperUnits,          "mm" );
    }
    pAttrList->add( XML_scale,              OString::number(  mrData.mnScaling ) );
    pAttrList->add( XML_fitToWidth,         OString::number(  mrData.mnFitToWidth ) );
    pAttrList->add( XML_fitToHeight,        OString::number(  mrData.mnFitToHeight ) );
    pAttrList->add( XML_pageOrder,          mrData.mbPrintInRows ? "overThenDown" : "downThenOver" );
    pAttrList->add( XML_orientation,        mrData.mbPortrait ? "portrait" : "landscape" );   // OOXTODO: "default"?
    // tdf#48767 if XML_usePrinterDefaults field is exist, then XML_orientation is always "portrait" in MS Excel
    // To resolve that import issue, if XML_usePrinterDefaults has default value (false) then XML_usePrinterDefaults is not added.
    if ( !mrData.mbValid )
        pAttrList->add( XML_usePrinterDefaults, ToPsz( !mrData.mbValid ) );
    pAttrList->add( XML_blackAndWhite,      ToPsz( mrData.mbBlackWhite ) );
    pAttrList->add( XML_draft,              ToPsz( mrData.mbDraftQuality ) );
    pAttrList->add( XML_cellComments,       mrData.mbPrintNotes ? "atEnd" : "none" );         // OOXTODO: "asDisplayed"?

    if ( mrData.mbManualStart )
    {
        pAttrList->add( XML_firstPageNumber,    OString::number(  mrData.mnStartPage ) );
        pAttrList->add( XML_useFirstPageNumber, ToPsz( mrData.mbManualStart ) );
    }
    // OOXTODO: XML_errors, // == displayed|blank|dash|NA
    pAttrList->add( XML_horizontalDpi,      OString::number(  mrData.mnHorPrintRes ) );
    pAttrList->add( XML_verticalDpi,        OString::number(  mrData.mnVerPrintRes ) );
    pAttrList->add( XML_copies,             OString::number(  mrData.mnCopies ) );
    // OOXTODO: devMode settings part RelationshipId: FSNS( XML_r, XML_id ),

    rStrm.GetCurrentStream()->singleElement( XML_pageSetup, pAttrList );
}

void XclExpSetup::WriteBody( XclExpStream& rStrm )
{
    XclBiff eBiff = rStrm.GetRoot().GetBiff();

    sal_uInt16 nFlags = 0;
    ::set_flag( nFlags, EXC_SETUP_INROWS,       mrData.mbPrintInRows );
    ::set_flag( nFlags, EXC_SETUP_PORTRAIT,     mrData.mbPortrait );
    ::set_flag( nFlags, EXC_SETUP_INVALID,      !mrData.mbValid );
    ::set_flag( nFlags, EXC_SETUP_BLACKWHITE,   mrData.mbBlackWhite );
    if( eBiff >= EXC_BIFF5 )
    {
        ::set_flag( nFlags, EXC_SETUP_DRAFT,        mrData.mbDraftQuality );
        /*  Set the Comments/Notes to "At end of sheet" if Print Notes is true.
            We don't currently support "as displayed on sheet". Thus this value
            will be re-interpreted to "At end of sheet". */
        const sal_uInt16 nNotes = EXC_SETUP_PRINTNOTES | EXC_SETUP_NOTES_END;
        ::set_flag( nFlags, nNotes,                 mrData.mbPrintNotes );
        ::set_flag( nFlags, EXC_SETUP_STARTPAGE,    mrData.mbManualStart );
    }

    rStrm   << mrData.mnPaperSize << mrData.mnScaling << mrData.mnStartPage
            << mrData.mnFitToWidth << mrData.mnFitToHeight << nFlags;
    if( eBiff >= EXC_BIFF5 )
    {
        rStrm   << mrData.mnHorPrintRes << mrData.mnVerPrintRes
                << mrData.mfHeaderMargin << mrData.mfFooterMargin << mrData.mnCopies;
    }
}

// Manual page breaks ---------------------------------------------------------

XclExpPageBreaks::XclExpPageBreaks( sal_uInt16 nRecId, const ScfUInt16Vec& rPageBreaks, sal_uInt16 nMaxPos ) :
    XclExpRecord( nRecId ),
    mrPageBreaks( rPageBreaks ),
    mnMaxPos( nMaxPos )
{
}

void XclExpPageBreaks::Save( XclExpStream& rStrm )
{
    if( !mrPageBreaks.empty() )
    {
        SetRecSize( 2 + ((rStrm.GetRoot().GetBiff() <= EXC_BIFF5) ? 2 : 6) * mrPageBreaks.size() );
        XclExpRecord::Save( rStrm );
    }
}

void XclExpPageBreaks::WriteBody( XclExpStream& rStrm )
{
    bool bWriteRange = (rStrm.GetRoot().GetBiff() == EXC_BIFF8);

    rStrm << static_cast< sal_uInt16 >( mrPageBreaks.size() );
    for( const auto& rPageBreak : mrPageBreaks )
    {
        rStrm << rPageBreak;
        if( bWriteRange )
            rStrm << sal_uInt16( 0 ) << mnMaxPos;
    }
}

void XclExpPageBreaks::SaveXml( XclExpXmlStream& rStrm )
{
    if( mrPageBreaks.empty() )
        return;

    sal_Int32 nElement = GetRecId() == EXC_ID_HORPAGEBREAKS ? XML_rowBreaks : XML_colBreaks;
    sax_fastparser::FSHelperPtr& pWorksheet = rStrm.GetCurrentStream();
    OString sNumPageBreaks = OString::number(  mrPageBreaks.size() );
    pWorksheet->startElement( nElement,
            XML_count,              sNumPageBreaks,
            XML_manualBreakCount,   sNumPageBreaks );
    for( const auto& rPageBreak : mrPageBreaks )
    {
        pWorksheet->singleElement( XML_brk,
                XML_id,     OString::number(rPageBreak),
                XML_man,    "true",
                XML_max,    OString::number(mnMaxPos),
                XML_min,    "0"
                // OOXTODO: XML_pt, ""
        );
    }
    pWorksheet->endElement( nElement );
}

// Page settings ==============================================================

XclExpPageSettings::XclExpPageSettings( const XclExpRoot& rRoot ) :
    XclExpRoot( rRoot )
{
    ScDocument& rDoc = GetDoc();
    SCTAB nScTab = GetCurrScTab();

    if( SfxStyleSheetBase* pStyleSheet = GetStyleSheetPool().Find( rDoc.GetPageStyle( nScTab ), SfxStyleFamily::Page ) )
    {
        const SfxItemSet& rItemSet = pStyleSheet->GetItemSet();
        maData.mbValid = true;

        // *** page settings ***

        maData.mbPrintInRows   = ! rItemSet.Get( ATTR_PAGE_TOPDOWN ).GetValue();
        maData.mbHorCenter     =  rItemSet.Get( ATTR_PAGE_HORCENTER ).GetValue();
        maData.mbVerCenter     =  rItemSet.Get( ATTR_PAGE_VERCENTER ).GetValue();
        maData.mbPrintHeadings =  rItemSet.Get( ATTR_PAGE_HEADERS   ).GetValue();
        maData.mbPrintGrid     =  rItemSet.Get( ATTR_PAGE_GRID      ).GetValue();
        maData.mbPrintNotes    =  rItemSet.Get( ATTR_PAGE_NOTES     ).GetValue();

        maData.mnStartPage     = rItemSet.Get( ATTR_PAGE_FIRSTPAGENO ).GetValue();
        maData.mbManualStart   = maData.mnStartPage && (!nScTab || rDoc.NeedPageResetAfterTab( nScTab - 1 ));

        const SvxLRSpaceItem& rLRItem = rItemSet.Get( ATTR_LRSPACE );
        maData.mfLeftMargin = XclTools::GetInchFromTwips(rLRItem.ResolveLeft({}));
        maData.mfRightMargin = XclTools::GetInchFromTwips(rLRItem.ResolveRight({}));
        const SvxULSpaceItem& rULItem = rItemSet.Get( ATTR_ULSPACE );
        maData.mfTopMargin     = XclTools::GetInchFromTwips( rULItem.GetUpper() );
        maData.mfBottomMargin  = XclTools::GetInchFromTwips( rULItem.GetLower() );

        const SvxPageItem& rPageItem = rItemSet.Get( ATTR_PAGE );
        const SvxSizeItem& rSizeItem = rItemSet.Get( ATTR_PAGE_SIZE );
        maData.SetScPaperSize( rSizeItem.GetSize(), !rPageItem.IsLandscape() );

        const ScPageScaleToItem& rScaleToItem = rItemSet.Get( ATTR_PAGE_SCALETO );
        sal_uInt16 nPages = rItemSet.Get( ATTR_PAGE_SCALETOPAGES ).GetValue();
        sal_uInt16 nScale = rItemSet.Get( ATTR_PAGE_SCALE ).GetValue();

        if( ScfTools::CheckItem( rItemSet, ATTR_PAGE_SCALETO, false ) && rScaleToItem.IsValid() )
        {
            maData.mnFitToWidth = rScaleToItem.GetWidth();
            maData.mnFitToHeight = rScaleToItem.GetHeight();
            maData.mbFitToPages = true;
        }
        else if( ScfTools::CheckItem( rItemSet, ATTR_PAGE_SCALETOPAGES, false ) && nPages )
        {
            maData.mnFitToWidth = 1;
            maData.mnFitToHeight = nPages;
            maData.mbFitToPages = true;
        }
        else if( nScale )
        {
            maData.mnScaling = nScale;
            maData.mbFitToPages = false;
        }

        maData.mxBrushItem.reset( new SvxBrushItem( rItemSet.Get( ATTR_BACKGROUND ) ) );
        maData.mbUseEvenHF = false;
        maData.mbUseFirstHF = false;

        // *** header and footer ***

        XclExpHFConverter aHFConv( GetRoot() );

        // header
        const SfxItemSet& rHdrItemSet = rItemSet.Get( ATTR_PAGE_HEADERSET ).GetItemSet();
        if( rHdrItemSet.Get( ATTR_PAGE_ON ).GetValue() )
        {
            const ScPageHFItem& rHFItem = rItemSet.Get( ATTR_PAGE_HEADERRIGHT );
            aHFConv.GenerateString( rHFItem.GetLeftArea(), rHFItem.GetCenterArea(), rHFItem.GetRightArea() );
            maData.maHeader = aHFConv.GetHFString();
            if ( rHdrItemSet.HasItem(ATTR_PAGE_SHARED) && !rHdrItemSet.Get(ATTR_PAGE_SHARED).GetValue())
            {
                const ScPageHFItem& rHFItemLeft = rItemSet.Get( ATTR_PAGE_HEADERLEFT );
                aHFConv.GenerateString( rHFItemLeft.GetLeftArea(), rHFItemLeft.GetCenterArea(), rHFItemLeft.GetRightArea() );
                maData.maHeaderEven = aHFConv.GetHFString();
                maData.mbUseEvenHF = true;
            }
            else
            {
                // If maData.mbUseEvenHF become true, then we will need a copy of maHeader in maHeaderEven.
                maData.maHeaderEven = maData.maHeader;
            }
            if (rHdrItemSet.HasItem(ATTR_PAGE_SHARED_FIRST) && !rHdrItemSet.Get(ATTR_PAGE_SHARED_FIRST).GetValue())
            {
                const ScPageHFItem& rHFItemFirst = rItemSet.Get( ATTR_PAGE_HEADERFIRST );
                aHFConv.GenerateString( rHFItemFirst.GetLeftArea(), rHFItemFirst.GetCenterArea(), rHFItemFirst.GetRightArea() );
                maData.maHeaderFirst = aHFConv.GetHFString();
                maData.mbUseFirstHF = true;
            }
            else
            {
                maData.maHeaderFirst = maData.maHeader;
            }
            // header height (Excel excludes header from top margin)
            sal_Int32 nHdrHeight = rHdrItemSet.Get( ATTR_PAGE_DYNAMIC ).GetValue() ?
                // dynamic height: calculate header height, add header <-> sheet area distance
                (aHFConv.GetTotalHeight() + rHdrItemSet.Get( ATTR_ULSPACE ).GetLower()) :
                // static height: ATTR_PAGE_SIZE already includes header <-> sheet area distance
                static_cast< sal_Int32 >( rHdrItemSet.Get( ATTR_PAGE_SIZE ).GetSize().Height() );
            maData.mfHeaderMargin = maData.mfTopMargin;
            maData.mfTopMargin += XclTools::GetInchFromTwips( nHdrHeight );
        }

        // footer
        const SfxItemSet& rFtrItemSet = rItemSet.Get( ATTR_PAGE_FOOTERSET ).GetItemSet();
        if( rFtrItemSet.Get( ATTR_PAGE_ON ).GetValue() )
        {
            const ScPageHFItem& rHFItem = rItemSet.Get( ATTR_PAGE_FOOTERRIGHT );
            aHFConv.GenerateString( rHFItem.GetLeftArea(), rHFItem.GetCenterArea(), rHFItem.GetRightArea() );
            maData.maFooter = aHFConv.GetHFString();
            if (rFtrItemSet.HasItem(ATTR_PAGE_SHARED) && !rFtrItemSet.Get(ATTR_PAGE_SHARED).GetValue())
            {
                const ScPageHFItem& rHFItemLeft = rItemSet.Get( ATTR_PAGE_FOOTERLEFT );
                aHFConv.GenerateString( rHFItemLeft.GetLeftArea(), rHFItemLeft.GetCenterArea(), rHFItemLeft.GetRightArea() );
                maData.maFooterEven = aHFConv.GetHFString();
                maData.mbUseEvenHF = true;
            }
            else
            {
                maData.maFooterEven = maData.maFooter;
            }
            if (rFtrItemSet.HasItem(ATTR_PAGE_SHARED_FIRST) && !rFtrItemSet.Get(ATTR_PAGE_SHARED_FIRST).GetValue())
            {
                const ScPageHFItem& rHFItemFirst = rItemSet.Get( ATTR_PAGE_FOOTERFIRST );
                aHFConv.GenerateString( rHFItemFirst.GetLeftArea(), rHFItemFirst.GetCenterArea(), rHFItemFirst.GetRightArea() );
                maData.maFooterFirst = aHFConv.GetHFString();
                maData.mbUseFirstHF = true;
            }
            else
            {
                maData.maFooterFirst = maData.maFooter;
            }
            // footer height (Excel excludes footer from bottom margin)
            sal_Int32 nFtrHeight = rFtrItemSet.Get( ATTR_PAGE_DYNAMIC ).GetValue() ?
                // dynamic height: calculate footer height, add sheet area <-> footer distance
                (aHFConv.GetTotalHeight() + rFtrItemSet.Get( ATTR_ULSPACE ).GetUpper()) :
                // static height: ATTR_PAGE_SIZE already includes sheet area <-> footer distance
                static_cast< sal_Int32 >( rFtrItemSet.Get( ATTR_PAGE_SIZE ).GetSize().Height() );
            maData.mfFooterMargin = maData.mfBottomMargin;
            maData.mfBottomMargin += XclTools::GetInchFromTwips( nFtrHeight );
        }
    }

    // *** page breaks ***

    set<SCROW> aRowBreaks;
    rDoc.GetAllRowBreaks(aRowBreaks, nScTab, false, true);

    SCROW const nMaxRow = numeric_limits<sal_uInt16>::max();
    for (const SCROW nRow : aRowBreaks)
    {
        if (nRow > nMaxRow)
            break;

        maData.maHorPageBreaks.push_back(nRow);
    }

    if (maData.maHorPageBreaks.size() > 1026)
    {
        // Excel allows only up to 1026 page breaks.  Trim any excess page breaks.
        ScfUInt16Vec::iterator itr = maData.maHorPageBreaks.begin();
        ::std::advance(itr, 1026);
        maData.maHorPageBreaks.erase(itr, maData.maHorPageBreaks.end());
    }

    set<SCCOL> aColBreaks;
    rDoc.GetAllColBreaks(aColBreaks, nScTab, false, true);
    for (const auto& rColBreak : aColBreaks)
        maData.maVerPageBreaks.push_back(rColBreak);
}

namespace {

class XclExpXmlStartHeaderFooterElementRecord : public XclExpXmlElementRecord
{
public:
    explicit XclExpXmlStartHeaderFooterElementRecord(sal_Int32 const nElement, bool const bDifferentOddEven = false, bool const bDifferentFirst = false)
         : XclExpXmlElementRecord(nElement), mbDifferentOddEven(bDifferentOddEven), mbDifferentFirst(bDifferentFirst) {}

    virtual void        SaveXml( XclExpXmlStream& rStrm ) override;
private:
    bool            mbDifferentOddEven;
    bool            mbDifferentFirst;
};

}

void XclExpXmlStartHeaderFooterElementRecord::SaveXml(XclExpXmlStream& rStrm)
{
    // OOXTODO: we currently only emit oddHeader/oddFooter elements, and
    //          do not support the first/even/odd page distinction.
    sax_fastparser::FSHelperPtr& rStream = rStrm.GetCurrentStream();
    rStream->startElement( mnElement,
            // OOXTODO: XML_alignWithMargins,
            XML_differentFirst,     mbDifferentFirst   ? "true" : "false",
            XML_differentOddEven,   mbDifferentOddEven ? "true" : "false"
            // OOXTODO: XML_scaleWithDoc
    );
}

void XclExpPageSettings::Save( XclExpStream& rStrm )
{
    XclExpBoolRecord( EXC_ID_PRINTHEADERS, maData.mbPrintHeadings ).Save( rStrm );
    XclExpBoolRecord( EXC_ID_PRINTGRIDLINES, maData.mbPrintGrid ).Save( rStrm );
    XclExpBoolRecord( EXC_ID_GRIDSET, true ).Save( rStrm );
    XclExpPageBreaks( EXC_ID_HORPAGEBREAKS, maData.maHorPageBreaks, static_cast< sal_uInt16 >( GetXclMaxPos().Col() ) ).Save( rStrm );
    XclExpPageBreaks( EXC_ID_VERPAGEBREAKS, maData.maVerPageBreaks, static_cast< sal_uInt16 >( GetXclMaxPos().Row() ) ).Save( rStrm );
    XclExpHeaderFooter( EXC_ID_HEADER, maData.maHeader ).Save( rStrm );
    XclExpHeaderFooter( EXC_ID_FOOTER, maData.maFooter ).Save( rStrm );
    XclExpBoolRecord( EXC_ID_HCENTER, maData.mbHorCenter ).Save( rStrm );
    XclExpBoolRecord( EXC_ID_VCENTER, maData.mbVerCenter ).Save( rStrm );
    XclExpDoubleRecord( EXC_ID_LEFTMARGIN, maData.mfLeftMargin ).Save( rStrm );
    XclExpDoubleRecord( EXC_ID_RIGHTMARGIN, maData.mfRightMargin ).Save( rStrm );
    XclExpDoubleRecord( EXC_ID_TOPMARGIN, maData.mfTopMargin ).Save( rStrm );
    XclExpDoubleRecord( EXC_ID_BOTTOMMARGIN, maData.mfBottomMargin ).Save( rStrm );
    XclExpSetup( maData ).Save( rStrm );

    if( (GetBiff() == EXC_BIFF8) && maData.mxBrushItem )
        if( const Graphic* pGraphic = maData.mxBrushItem->GetGraphic() )
            XclExpImgData( *pGraphic, EXC_ID8_IMGDATA ).Save( rStrm );
}

void XclExpPageSettings::SaveXml( XclExpXmlStream& rStrm )
{
    XclExpXmlStartSingleElementRecord( XML_printOptions ).SaveXml( rStrm );
    XclExpBoolRecord( EXC_ID_PRINTHEADERS, maData.mbPrintHeadings, XML_headings ).SaveXml( rStrm );
    XclExpBoolRecord( EXC_ID_PRINTGRIDLINES, maData.mbPrintGrid, XML_gridLines ).SaveXml( rStrm );
    XclExpBoolRecord( EXC_ID_GRIDSET, true, XML_gridLinesSet ).SaveXml( rStrm );
    XclExpBoolRecord( EXC_ID_HCENTER, maData.mbHorCenter, XML_horizontalCentered ).SaveXml( rStrm );
    XclExpBoolRecord( EXC_ID_VCENTER, maData.mbVerCenter, XML_verticalCentered ).SaveXml( rStrm );
    XclExpXmlEndSingleElementRecord().SaveXml( rStrm );    // XML_printOptions

    XclExpXmlStartSingleElementRecord( XML_pageMargins ).SaveXml( rStrm );
    XclExpDoubleRecord( EXC_ID_LEFTMARGIN, maData.mfLeftMargin ).SetAttribute( XML_left )->SaveXml( rStrm );
    XclExpDoubleRecord( EXC_ID_RIGHTMARGIN, maData.mfRightMargin ).SetAttribute( XML_right )->SaveXml( rStrm );
    XclExpDoubleRecord( EXC_ID_TOPMARGIN, maData.mfTopMargin ).SetAttribute( XML_top )->SaveXml( rStrm );
    XclExpDoubleRecord( EXC_ID_BOTTOMMARGIN, maData.mfBottomMargin ).SetAttribute( XML_bottom )->SaveXml( rStrm );
    XclExpDoubleRecord( 0, maData.mfHeaderMargin).SetAttribute( XML_header )->SaveXml( rStrm );
    XclExpDoubleRecord( 0, maData.mfFooterMargin).SetAttribute( XML_footer )->SaveXml( rStrm );
    XclExpXmlEndSingleElementRecord().SaveXml( rStrm );    // XML_pageMargins

    XclExpSetup( maData ).SaveXml( rStrm );

    XclExpXmlStartHeaderFooterElementRecord(XML_headerFooter, maData.mbUseEvenHF, maData.mbUseFirstHF).SaveXml(rStrm);
    XclExpHeaderFooter( EXC_ID_HEADER, maData.maHeader ).SaveXml( rStrm );
    XclExpHeaderFooter( EXC_ID_FOOTER, maData.maFooter ).SaveXml( rStrm );
    if (maData.mbUseEvenHF)
    {
        XclExpHeaderFooter( EXC_ID_HEADER_EVEN, maData.maHeaderEven ).SaveXml( rStrm );
        XclExpHeaderFooter( EXC_ID_FOOTER_EVEN, maData.maFooterEven ).SaveXml( rStrm );
    }
    if (maData.mbUseFirstHF)
    {
        XclExpHeaderFooter( EXC_ID_HEADER_FIRST, maData.maHeaderFirst ).SaveXml( rStrm );
        XclExpHeaderFooter( EXC_ID_FOOTER_FIRST, maData.maFooterFirst ).SaveXml( rStrm );
    }
    XclExpXmlEndElementRecord( XML_headerFooter ).SaveXml( rStrm );

    XclExpPageBreaks( EXC_ID_HORPAGEBREAKS, maData.maHorPageBreaks,
                    static_cast< sal_uInt16 >( GetXclMaxPos().Col() ) ).SaveXml( rStrm );
    XclExpPageBreaks( EXC_ID_VERPAGEBREAKS, maData.maVerPageBreaks,
                    static_cast< sal_uInt16 >( GetXclMaxPos().Row() ) ).SaveXml( rStrm );
}

XclExpImgData* XclExpPageSettings::getGraphicExport()
{
    if( maData.mxBrushItem )
        if( const Graphic* pGraphic = maData.mxBrushItem->GetGraphic() )
            return new XclExpImgData( *pGraphic, EXC_ID8_IMGDATA );

    return nullptr;
}

XclExpChartPageSettings::XclExpChartPageSettings( const XclExpRoot& rRoot ) :
    XclExpRoot( rRoot )
{
}

void XclExpChartPageSettings::Save( XclExpStream& rStrm )
{
    XclExpHeaderFooter( EXC_ID_HEADER, maData.maHeader ).Save( rStrm );
    XclExpHeaderFooter( EXC_ID_FOOTER, maData.maFooter ).Save( rStrm );
    XclExpBoolRecord( EXC_ID_HCENTER, maData.mbHorCenter ).Save( rStrm );
    XclExpBoolRecord( EXC_ID_VCENTER, maData.mbVerCenter ).Save( rStrm );
    XclExpSetup( maData ).Save( rStrm );
    XclExpUInt16Record( EXC_ID_PRINTSIZE, EXC_PRINTSIZE_FULL ).Save( rStrm );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
