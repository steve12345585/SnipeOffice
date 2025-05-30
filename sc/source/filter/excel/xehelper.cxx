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

#include <sal/config.h>

#include <string_view>

#include <com/sun/star/i18n/XBreakIterator.hpp>
#include <com/sun/star/i18n/ScriptType.hpp>
#include <com/sun/star/uri/XUriReference.hpp>
#include <com/sun/star/uri/XUriReferenceFactory.hpp>
#include <com/sun/star/uri/UriReferenceFactory.hpp>
#include <comphelper/processfactory.hxx>
#include <o3tl/string_view.hxx>
#include <sfx2/objsh.hxx>
#include <vcl/font.hxx>
#include <tools/urlobj.hxx>
#include <svl/itemset.hxx>
#include <svtools/ctrltool.hxx>
#include <svx/svdotext.hxx>
#include <editeng/outlobj.hxx>
#include <scitems.hxx>
#include <editeng/fhgtitem.hxx>
#include <editeng/flstitem.hxx>
#include <editeng/colritem.hxx>
#include <editeng/eeitem.hxx>
#include <editeng/flditem.hxx>
#include <editeng/escapementitem.hxx>
#include <editeng/svxfont.hxx>
#include <editeng/editids.hrc>
#include <osl/file.hxx>

#include <document.hxx>
#include <docpool.hxx>
#include <docsh.hxx>
#include <editutil.hxx>
#include <patattr.hxx>
#include <scmatrix.hxx>
#include <xestyle.hxx>
#include <fprogressbar.hxx>
#include <globstr.hrc>
#include <xltracer.hxx>
#include <xltools.hxx>
#include <xecontent.hxx>
#include <xelink.hxx>
#include <xehelper.hxx>

using ::com::sun::star::uno::Reference;
using ::com::sun::star::i18n::XBreakIterator;

// Export progress bar ========================================================

XclExpProgressBar::XclExpProgressBar( const XclExpRoot& rRoot ) :
    XclExpRoot( rRoot ),
    mxProgress( new ScfProgressBar( rRoot.GetDocShell(), STR_SAVE_DOC ) ),
    mpSubProgress( nullptr ),
    mpSubRowCreate( nullptr ),
    mpSubRowFinal( nullptr ),
    mnSegRowFinal( SCF_INV_SEGMENT ),
    mnRowCount( 0 )
{
}

XclExpProgressBar::~XclExpProgressBar()
{
}

void XclExpProgressBar::Initialize()
{
    const ScDocument& rDoc = GetDoc();
    const XclExpTabInfo& rTabInfo = GetTabInfo();
    SCTAB nScTabCount = rTabInfo.GetScTabCount();

    // *** segment: creation of ROW records *** -------------------------------

    sal_Int32 nSegRowCreate = mxProgress->AddSegment( 2000 );
    mpSubRowCreate = &mxProgress->GetSegmentProgressBar( nSegRowCreate );
    maSubSegRowCreate.resize( nScTabCount, SCF_INV_SEGMENT );

    for( SCTAB nScTab = 0; nScTab < nScTabCount; ++nScTab )
    {
        if( rTabInfo.IsExportTab( nScTab ) )
        {
            SCCOL nLastUsedScCol;
            SCROW nLastUsedScRow;
            rDoc.GetTableArea( nScTab, nLastUsedScCol, nLastUsedScRow );
            std::size_t nSegSize = static_cast< std::size_t >( nLastUsedScRow + 1 );
            maSubSegRowCreate[ nScTab ] = mpSubRowCreate->AddSegment( nSegSize );
        }
    }

    // *** segment: writing all ROW records *** -------------------------------

    mnSegRowFinal = mxProgress->AddSegment( 1000 );
    // sub progress bar and segment are created later in ActivateFinalRowsSegment()
}

void XclExpProgressBar::IncRowRecordCount()
{
    ++mnRowCount;
}

void XclExpProgressBar::ActivateCreateRowsSegment()
{
    OSL_ENSURE( (0 <= GetCurrScTab()) && (GetCurrScTab() < GetTabInfo().GetScTabCount()),
        "XclExpProgressBar::ActivateCreateRowsSegment - invalid sheet" );
    sal_Int32 nSeg = maSubSegRowCreate[ GetCurrScTab() ];
    OSL_ENSURE( nSeg != SCF_INV_SEGMENT, "XclExpProgressBar::ActivateCreateRowsSegment - invalid segment" );
    if( nSeg != SCF_INV_SEGMENT )
    {
        mpSubProgress = mpSubRowCreate;
        mpSubProgress->ActivateSegment( nSeg );
    }
    else
        mpSubProgress = nullptr;
}

void XclExpProgressBar::ActivateFinalRowsSegment()
{
    if( !mpSubRowFinal && (mnRowCount > 0) )
    {
        mpSubRowFinal = &mxProgress->GetSegmentProgressBar( mnSegRowFinal );
        mpSubRowFinal->AddSegment( mnRowCount );
    }
    mpSubProgress = mpSubRowFinal;
    if( mpSubProgress )
        mpSubProgress->Activate();
}

void XclExpProgressBar::Progress()
{
    if( mpSubProgress && !mpSubProgress->IsFull() )
        mpSubProgress->Progress();
}

// Calc->Excel cell address/range conversion ==================================

namespace {

/** Fills the passed Excel address with the passed Calc cell coordinates without checking any limits. */
void lclFillAddress( XclAddress& rXclPos, SCCOL nScCol, SCROW nScRow )
{
    rXclPos.mnCol = static_cast< sal_uInt16 >( nScCol );
    rXclPos.mnRow = static_cast< sal_uInt32 >( nScRow );
}

} // namespace

XclExpAddressConverter::XclExpAddressConverter( const XclExpRoot& rRoot ) :
    XclAddressConverterBase( rRoot.GetTracer(), rRoot.GetXclMaxPos() )
{
}

// cell address ---------------------------------------------------------------

bool XclExpAddressConverter::CheckAddress( const ScAddress& rScPos, bool bWarn )
{
    // ScAddress::operator<=() doesn't do what we want here
    bool bValidCol = (0 <= rScPos.Col()) && (rScPos.Col() <= maMaxPos.Col());
    bool bValidRow = (0 <= rScPos.Row()) && (rScPos.Row() <= maMaxPos.Row());
    bool bValidTab = (0 <= rScPos.Tab()) && (rScPos.Tab() <= maMaxPos.Tab());

    bool bValid = bValidCol && bValidRow && bValidTab;
    if( !bValid )
    {
        mbColTrunc |= !bValidCol;
        mbRowTrunc |= !bValidRow;
    }
    if( !bValid && bWarn )
    {
        mbTabTrunc |= (rScPos.Tab() > maMaxPos.Tab());  // do not warn for deleted refs
        mrTracer.TraceInvalidAddress( rScPos, maMaxPos );
    }
    return bValid;
}

bool XclExpAddressConverter::ConvertAddress( XclAddress& rXclPos,
        const ScAddress& rScPos, bool bWarn )
{
    bool bValid = CheckAddress( rScPos, bWarn );
    if( bValid )
        lclFillAddress( rXclPos, rScPos.Col(), rScPos.Row() );
    return bValid;
}

XclAddress XclExpAddressConverter::CreateValidAddress( const ScAddress& rScPos, bool bWarn )
{
    XclAddress aXclPos( ScAddress::UNINITIALIZED );
    if( !ConvertAddress( aXclPos, rScPos, bWarn ) )
        lclFillAddress( aXclPos, ::std::min( rScPos.Col(), maMaxPos.Col() ), ::std::min( rScPos.Row(), maMaxPos.Row() ) );
    return aXclPos;
}

// cell range -----------------------------------------------------------------

bool XclExpAddressConverter::CheckRange( const ScRange& rScRange, bool bWarn )
{
    return CheckAddress( rScRange.aStart, bWarn ) && CheckAddress( rScRange.aEnd, bWarn );
}

bool XclExpAddressConverter::ValidateRange( ScRange& rScRange, bool bWarn )
{
    rScRange.PutInOrder();

    // check start position
    bool bValidStart = CheckAddress( rScRange.aStart, bWarn );
    if( bValidStart )
    {
        // check & correct end position
        ScAddress& rScEnd = rScRange.aEnd;
        if( !CheckAddress( rScEnd, bWarn ) )
        {
            rScEnd.SetCol( ::std::min( rScEnd.Col(), maMaxPos.Col() ) );
            rScEnd.SetRow( ::std::min( rScEnd.Row(), maMaxPos.Row() ) );
            rScEnd.SetTab( ::std::min( rScEnd.Tab(), maMaxPos.Tab() ) );
        }
    }

    return bValidStart;
}

bool XclExpAddressConverter::ConvertRange( XclRange& rXclRange,
        const ScRange& rScRange, bool bWarn )
{
    // check start position
    bool bValidStart = CheckAddress( rScRange.aStart, bWarn );
    if( bValidStart )
    {
        lclFillAddress( rXclRange.maFirst, rScRange.aStart.Col(), rScRange.aStart.Row() );

        // check & correct end position
        SCCOL nScCol2 = rScRange.aEnd.Col();
        SCROW nScRow2 = rScRange.aEnd.Row();
        if( !CheckAddress( rScRange.aEnd, bWarn ) )
        {
            nScCol2 = ::std::min( nScCol2, maMaxPos.Col() );
            nScRow2 = ::std::min( nScRow2, maMaxPos.Row() );
        }
        lclFillAddress( rXclRange.maLast, nScCol2, nScRow2 );
    }
    return bValidStart;
}

// cell range list ------------------------------------------------------------

void XclExpAddressConverter::ValidateRangeList( ScRangeList& rScRanges, bool bWarn )
{
    for ( size_t nRange = rScRanges.size(); nRange > 0; )
    {
        ScRange & rScRange = rScRanges[ --nRange ];
        if( !CheckRange( rScRange, bWarn ) )
            rScRanges.Remove(nRange);
    }
}

void XclExpAddressConverter::ConvertRangeList( XclRangeList& rXclRanges,
        const ScRangeList& rScRanges, bool bWarn )
{
    rXclRanges.clear();
    for( size_t nPos = 0, nCount = rScRanges.size(); nPos < nCount; ++nPos )
    {
        const ScRange & rScRange = rScRanges[ nPos ];
        XclRange aXclRange( ScAddress::UNINITIALIZED );
        if( ConvertRange( aXclRange, rScRange, bWarn ) )
            rXclRanges.push_back( aXclRange );
    }
}

// EditEngine->String conversion ==============================================

namespace {

OUString lclGetUrlRepresentation( const SvxURLField& rUrlField )
{
    const OUString& aRepr = rUrlField.GetRepresentation();
    // no representation -> use URL
    return aRepr.isEmpty() ? rUrlField.GetURL() : aRepr;
}

} // namespace

XclExpHyperlinkHelper::XclExpHyperlinkHelper( const XclExpRoot& rRoot, const ScAddress& rScPos ) :
    XclExpRoot( rRoot ),
    maScPos( rScPos ),
    mbMultipleUrls( false )
{
}

XclExpHyperlinkHelper::~XclExpHyperlinkHelper()
{
}

OUString XclExpHyperlinkHelper::ProcessUrlField( const SvxURLField& rUrlField )
{
    OUString aUrlRepr;

    if( GetBiff() == EXC_BIFF8 )    // no HLINK records in BIFF2-BIFF7
    {
        // there was/is already a HLINK record
        mbMultipleUrls = static_cast< bool >(mxLinkRec);

        mxLinkRec = new XclExpHyperlink( GetRoot(), rUrlField, maScPos );

        if( const OUString* pRepr = mxLinkRec->GetRepr() )
            aUrlRepr = *pRepr;

        // add URL to note text
        maUrlList = ScGlobal::addToken( maUrlList, rUrlField.GetURL(), '\n' );
    }

    // no hyperlink representation from Excel HLINK record -> use it from text field
    return aUrlRepr.isEmpty() ? lclGetUrlRepresentation(rUrlField) : aUrlRepr;
}

bool XclExpHyperlinkHelper::HasLinkRecord() const
{
    return !mbMultipleUrls && mxLinkRec;
}

XclExpHyperlinkHelper::XclExpHyperlinkRef XclExpHyperlinkHelper::GetLinkRecord() const
{
    if( HasLinkRecord() )
        return mxLinkRec;
    return XclExpHyperlinkRef();
}

namespace {

/** Creates a new formatted string from the passed unformatted string.

    Creates a Unicode string or a byte string, depending on the current BIFF
    version contained in the passed XclExpRoot object. May create a formatted
    string object, if the text contains different script types.

    @param pCellAttr
        Cell attributes used for font formatting.
    @param nFlags
        Modifiers for string export.
    @param nMaxLen
        The maximum number of characters to store in this string.
    @return
        The new string object.
 */
XclExpStringRef lclCreateFormattedString(
        const XclExpRoot& rRoot, const OUString& rText, const ScPatternAttr* pCellAttr,
        XclStrFlags nFlags, sal_uInt16 nMaxLen )
{
    /*  Create an empty Excel string object with correctly initialized BIFF mode,
        because this function only uses Append() functions that require this. */
    XclExpStringRef xString = XclExpStringHelper::CreateString( rRoot, OUString(), nFlags, nMaxLen );

    // script type handling
    Reference< XBreakIterator > xBreakIt = rRoot.GetDoc().GetBreakIterator();
    namespace ApiScriptType = css::i18n::ScriptType;
    // #i63255# get script type for leading weak characters
    sal_Int16 nLastScript = XclExpStringHelper::GetLeadingScriptType( rRoot, rText );

    // font buffer and cell item set
    XclExpFontBuffer& rFontBuffer = rRoot.GetFontBuffer();
    const SfxItemSet& rItemSet = pCellAttr ?
        pCellAttr->GetItemSet() :
        rRoot.GetDoc().getCellAttributeHelper().getDefaultCellAttribute().GetItemSet();

    // process all script portions
    sal_Int32 nPortionPos = 0;
    sal_Int32 nTextLen = rText.getLength();
    while( nPortionPos < nTextLen )
    {
        // get script type and end position of next script portion
        sal_Int16 nScript = xBreakIt->getScriptType( rText, nPortionPos );
        sal_Int32 nPortionEnd = xBreakIt->endOfScript( rText, nPortionPos, nScript );

        // reuse previous script for following weak portions
        if( nScript == ApiScriptType::WEAK )
            nScript = nLastScript;

        // construct font from current text portion
        SvxFont aFont(XclExpFontHelper::GetFontFromItemSet(rRoot, rItemSet, nScript));
        model::ComplexColor aComplexColor;
        ScPatternAttr::fillColor(aComplexColor, rItemSet, ScAutoFontColorMode::Raw);

        // Excel start position of this portion
        sal_Int32 nXclPortionStart = xString->Len();
        // add portion text to Excel string
        XclExpStringHelper::AppendString( *xString, rRoot, rText.subView( nPortionPos, nPortionEnd - nPortionPos ) );
        if( nXclPortionStart < xString->Len() )
        {
            // insert font into buffer
            sal_uInt16 nFontIdx = rFontBuffer.Insert(aFont, aComplexColor, EXC_COLOR_CELLTEXT);
            // insert font index into format run vector
            xString->AppendFormat( nXclPortionStart, nFontIdx );
        }

        // go to next script portion
        nLastScript = nScript;
        nPortionPos = nPortionEnd;
    }

    return xString;
}

/** Creates a new formatted string from an edit engine text object.

    Creates a Unicode string or a byte string, depending on the current BIFF
    version contained in the passed XclExpRoot object.

    @param rEE
        The edit engine in use. The text object must already be set.
    @param nFlags
        Modifiers for string export.
    @param nMaxLen
        The maximum number of characters to store in this string.
    @return
        The new string object.
 */
XclExpStringRef lclCreateFormattedString(
        const XclExpRoot& rRoot, EditEngine& rEE, XclExpHyperlinkHelper* pLinkHelper,
        XclStrFlags nFlags, sal_uInt16 nMaxLen )
{
    /*  Create an empty Excel string object with correctly initialized BIFF mode,
        because this function only uses Append() functions that require this. */
    XclExpStringRef xString = XclExpStringHelper::CreateString( rRoot, OUString(), nFlags, nMaxLen );

    // font buffer and helper item set for edit engine -> Calc item conversion
    XclExpFontBuffer& rFontBuffer = rRoot.GetFontBuffer();
    SfxItemSetFixed<ATTR_PATTERN_START, ATTR_PATTERN_END> aItemSet( *rRoot.GetDoc().GetPool() );

    // script type handling
    Reference< XBreakIterator > xBreakIt = rRoot.GetDoc().GetBreakIterator();
    namespace ApiScriptType = css::i18n::ScriptType;
    // #i63255# get script type for leading weak characters
    sal_Int16 nLastScript = XclExpStringHelper::GetLeadingScriptType( rRoot, rEE.GetText() );

    // process all paragraphs
    sal_Int32 nParaCount = rEE.GetParagraphCount();
    for( sal_Int32 nPara = 0; nPara < nParaCount; ++nPara )
    {
        ESelection aSel( nPara, 0 );
        OUString aParaText( rEE.GetText( nPara ) );

        std::vector<sal_Int32> aPosList;
        rEE.GetPortions( nPara, aPosList );

        // process all portions in the paragraph
        for( const auto& rPos : aPosList )
        {
            aSel.end.nIndex = rPos;
            OUString aXclPortionText = aParaText.copy( aSel.start.nIndex, aSel.end.nIndex - aSel.start.nIndex );

            aItemSet.ClearItem();
            SfxItemSet aEditSet( rEE.GetAttribs( aSel ) );
            ScPatternAttr::GetFromEditItemSet( aItemSet, aEditSet );

            // get escapement value
            short nEsc = aEditSet.Get( EE_CHAR_ESCAPEMENT ).GetEsc();

            // process text fields
            bool bIsHyperlink = false;
            if (aSel.start.nIndex + 1 == aSel.end.nIndex)
            {
                // test if the character is a text field
                if( const SvxFieldItem* pItem = aEditSet.GetItemIfSet( EE_FEATURE_FIELD, false ) )
                {
                    const SvxFieldData* pField = pItem->GetField();
                    if( const SvxURLField* pUrlField = dynamic_cast<const SvxURLField*>( pField )  )
                    {
                        // convert URL field to string representation
                        aXclPortionText = pLinkHelper ?
                            pLinkHelper->ProcessUrlField( *pUrlField ) :
                            lclGetUrlRepresentation( *pUrlField );
                        bIsHyperlink = true;
                    }
                    else
                    {
                        OSL_FAIL( "lclCreateFormattedString - unknown text field" );
                        aXclPortionText.clear();
                    }
                }
            }

            // Excel start position of this portion
            sal_Int32 nXclPortionStart = xString->Len();
            // add portion text to Excel string
            XclExpStringHelper::AppendString( *xString, rRoot, aXclPortionText );
            if( (nXclPortionStart < xString->Len()) || (aParaText.isEmpty()) )
            {
                /*  Construct font from current edit engine text portion. Edit engine
                    creates different portions for different script types, no need to loop. */
                sal_Int16 nScript = xBreakIt->getScriptType( aXclPortionText, 0 );
                if( nScript == ApiScriptType::WEAK )
                    nScript = nLastScript;
                SvxFont aFont( XclExpFontHelper::GetFontFromItemSet(rRoot, aItemSet, nScript));
                model::ComplexColor aComplexColor;
                ScPatternAttr::fillColor(aComplexColor, aItemSet, ScAutoFontColorMode::Raw);

                nLastScript = nScript;

                // add escapement
                aFont.SetEscapement( nEsc );
                // modify automatic font color for hyperlinks
                if (bIsHyperlink && aItemSet.Get(ATTR_FONT_COLOR).GetValue() == COL_AUTO)
                    aComplexColor.setFinalColor(COL_LIGHTBLUE);

                // insert font into buffer
                sal_uInt16 nFontIdx = rFontBuffer.Insert(aFont, aComplexColor, EXC_COLOR_CELLTEXT);
                // insert font index into format run vector
                xString->AppendFormat( nXclPortionStart, nFontIdx );
            }

            aSel.start.nIndex = aSel.end.nIndex;
        }

        // add trailing newline (important for correct character index calculation)
        if( nPara + 1 < nParaCount )
            XclExpStringHelper::AppendChar( *xString, rRoot, '\n' );
    }

    if (xString->HasNewline() && nParaCount == 1)
    {
        // Found buggy Excel behaviour: although the content has newlines, it has not been wrapped.
        xString->SetSingleLineForMultipleParagraphs(true);
    }
    return xString;
}

} // namespace

XclExpStringRef XclExpStringHelper::CreateString(
        const XclExpRoot& rRoot, const OUString& rString, XclStrFlags nFlags, sal_uInt16 nMaxLen )
{
    XclExpStringRef xString = std::make_shared<XclExpString>();
    if( rRoot.GetBiff() == EXC_BIFF8 )
        xString->Assign( rString, nFlags, nMaxLen );
    else
        xString->AssignByte( rString, rRoot.GetTextEncoding(), nFlags, nMaxLen );
    return xString;
}

XclExpStringRef XclExpStringHelper::CreateString(
        const XclExpRoot& rRoot, sal_Unicode cChar, XclStrFlags nFlags, sal_uInt16 nMaxLen )
{
    XclExpStringRef xString = CreateString( rRoot, OUString(), nFlags, nMaxLen );
    AppendChar( *xString, rRoot, cChar );
    return xString;
}

void XclExpStringHelper::AppendString( XclExpString& rXclString, const XclExpRoot& rRoot, std::u16string_view rString )
{
    if( rRoot.GetBiff() == EXC_BIFF8 )
        rXclString.Append( rString );
    else
        rXclString.AppendByte( rString, rRoot.GetTextEncoding() );
}

void XclExpStringHelper::AppendChar( XclExpString& rXclString, const XclExpRoot& rRoot, sal_Unicode cChar )
{
    if( rRoot.GetBiff() == EXC_BIFF8 )
        rXclString.Append( rtl::OUStringChar(cChar) );
    else
        rXclString.AppendByte( cChar, rRoot.GetTextEncoding() );
}

XclExpStringRef XclExpStringHelper::CreateCellString(
        const XclExpRoot& rRoot, const OUString& rString, const ScPatternAttr* pCellAttr,
        XclStrFlags nFlags, sal_uInt16 nMaxLen )
{
    return lclCreateFormattedString(rRoot, rString, pCellAttr, nFlags, nMaxLen);
}

XclExpStringRef XclExpStringHelper::CreateCellString(
        const XclExpRoot& rRoot, const EditTextObject& rEditText, const ScPatternAttr* pCellAttr,
        XclExpHyperlinkHelper& rLinkHelper, XclStrFlags nFlags, sal_uInt16 nMaxLen )
{
    XclExpStringRef xString;

    // formatted cell
    ScEditEngineDefaulter& rEE = rRoot.GetEditEngine();
    bool bOldUpdateMode = rEE.SetUpdateLayout( true );

    // default items
    const SfxItemSet& rItemSet = pCellAttr ?
        pCellAttr->GetItemSet() :
        rRoot.GetDoc().getCellAttributeHelper().getDefaultCellAttribute().GetItemSet();
    SfxItemSet aEEItemSet( rEE.GetEmptyItemSet() );
    ScPatternAttr::FillToEditItemSet( aEEItemSet, rItemSet );
    rEE.SetDefaults( std::move(aEEItemSet) );      // edit engine takes ownership

    // create the string
    rEE.SetTextCurrentDefaults(rEditText);
    xString = lclCreateFormattedString( rRoot, rEE, &rLinkHelper, nFlags, nMaxLen );
    rEE.SetUpdateLayout( bOldUpdateMode );

    return xString;
}

XclExpStringRef XclExpStringHelper::CreateString(
        const XclExpRoot& rRoot, const SdrTextObj& rTextObj,
        XclStrFlags nFlags )
{
    XclExpStringRef xString;
    if( const OutlinerParaObject* pParaObj = rTextObj.GetOutlinerParaObject() )
    {
        EditEngine& rEE = rRoot.GetDrawEditEngine();
        bool bOldUpdateMode = rEE.SetUpdateLayout( true );
        // create the string
        rEE.SetText( pParaObj->GetTextObject() );
        xString = lclCreateFormattedString( rRoot, rEE, nullptr, nFlags, EXC_STR_MAXLEN );
        rEE.SetUpdateLayout( bOldUpdateMode );
        // limit formats - TODO: BIFF dependent
        if( !xString->IsEmpty() )
        {
            xString->LimitFormatCount( EXC_MAXRECSIZE_BIFF8 / 8 - 1 );
            xString->AppendTrailingFormat( EXC_FONT_APP );
        }
    }
    else
    {
        OSL_FAIL( "XclExpStringHelper::CreateString - textbox without para object" );
        // create BIFF dependent empty Excel string
        xString = CreateString( rRoot, OUString(), nFlags );
    }
    return xString;
}

XclExpStringRef XclExpStringHelper::CreateString(
        const XclExpRoot& rRoot, const EditTextObject& rEditObj,
        XclStrFlags nFlags )
{
    XclExpStringRef xString;
    EditEngine& rEE = rRoot.GetDrawEditEngine();
    bool bOldUpdateMode = rEE.SetUpdateLayout( true );
    rEE.SetText( rEditObj );
    xString = lclCreateFormattedString( rRoot, rEE, nullptr, nFlags, EXC_STR_MAXLEN );
    rEE.SetUpdateLayout( bOldUpdateMode );
    // limit formats - TODO: BIFF dependent
    if( !xString->IsEmpty() )
    {
        xString->LimitFormatCount( EXC_MAXRECSIZE_BIFF8 / 8 - 1 );
        xString->AppendTrailingFormat( EXC_FONT_APP );
    }
    return xString;
}

sal_Int16 XclExpStringHelper::GetLeadingScriptType( const XclExpRoot& rRoot, const OUString& rString )
{
    namespace ApiScriptType = css::i18n::ScriptType;
    Reference< XBreakIterator > xBreakIt = rRoot.GetDoc().GetBreakIterator();
    sal_Int32 nStrPos = 0;
    sal_Int32 nStrLen = rString.getLength();
    sal_Int16 nScript = ApiScriptType::WEAK;
    while( (nStrPos < nStrLen) && (nScript == ApiScriptType::WEAK) )
    {
        nScript = xBreakIt->getScriptType( rString, nStrPos );
        nStrPos = xBreakIt->endOfScript( rString, nStrPos, nScript );
    }
    return (nScript == ApiScriptType::WEAK) ? rRoot.GetDefApiScript() : nScript;
}

// Header/footer conversion ===================================================

XclExpHFConverter::XclExpHFConverter( const XclExpRoot& rRoot ) :
    XclExpRoot( rRoot ),
    mrEE( rRoot.GetHFEditEngine() ),
    mnTotalHeight( 0 )
{
}

void XclExpHFConverter::GenerateString(
        const EditTextObject* pLeftObj,
        const EditTextObject* pCenterObj,
        const EditTextObject* pRightObj )
{
    maHFString.clear();
    mnTotalHeight = 0;
    AppendPortion( pLeftObj, 'L' );
    AppendPortion( pCenterObj, 'C' );
    AppendPortion( pRightObj, 'R' );
}

void XclExpHFConverter::AppendPortion( const EditTextObject* pTextObj, sal_Unicode cPortionCode )
{
    if( !pTextObj ) return;

    OUString aText;
    sal_Int32 nHeight = 0;
    SfxItemSetFixed<ATTR_PATTERN_START, ATTR_PATTERN_END> aItemSet( *GetDoc().GetPool() );

    // edit engine
    bool bOldUpdateMode = mrEE.SetUpdateLayout( true );
    mrEE.SetText( *pTextObj );

    // font information
    XclFontData aFontData, aNewData;
    if( const XclExpFont* pFirstFont = GetFontBuffer().GetFont( EXC_FONT_APP ) )
    {
        aFontData = pFirstFont->GetFontData();
        aFontData.mnHeight = (aFontData.mnHeight + 10) / 20;   // using pt here, not twips
    }
    else
        aFontData.mnHeight = 10;

    const FontList* pFontList = nullptr;
    if( SfxObjectShell* pDocShell = GetDocShell() )
    {
        if( const SvxFontListItem* pInfoItem = static_cast< const SvxFontListItem* >(
                pDocShell->GetItem( SID_ATTR_CHAR_FONTLIST ) ) )
            pFontList = pInfoItem->GetFontList();
    }

    sal_Int32 nParaCount = mrEE.GetParagraphCount();
    for( sal_Int32 nPara = 0; nPara < nParaCount; ++nPara )
    {
        ESelection aSel( nPara, 0 );
        OUStringBuffer aParaText;
        sal_Int32 nParaHeight = 0;
        std::vector<sal_Int32> aPosList;
        mrEE.GetPortions( nPara, aPosList );

        for( const auto& rPos : aPosList )
        {
            aSel.end.nIndex = rPos;
            if (aSel.start.nIndex < aSel.end.nIndex)
            {

// --- font attributes ---

                vcl::Font aFont;
                model::ComplexColor aComplexColor;
                aItemSet.ClearItem();
                SfxItemSet aEditSet( mrEE.GetAttribs( aSel ) );
                ScPatternAttr::GetFromEditItemSet( aItemSet, aEditSet );
                ScPatternAttr::fillFontOnly(aFont, aItemSet);
                ScPatternAttr::fillColor(aComplexColor, aItemSet, ScAutoFontColorMode::Raw);

                // font name and style
                aNewData.maName = XclTools::GetXclFontName( aFont.GetFamilyName() );
                aNewData.mnWeight = (aFont.GetWeightMaybeAskConfig() > WEIGHT_NORMAL) ? EXC_FONTWGHT_BOLD : EXC_FONTWGHT_NORMAL;
                aNewData.mbItalic = (aFont.GetItalicMaybeAskConfig() != ITALIC_NONE);
                bool bNewFont = (aFontData.maName != aNewData.maName);
                bool bNewStyle = (aFontData.mnWeight != aNewData.mnWeight) ||
                                 (aFontData.mbItalic != aNewData.mbItalic);
                if( bNewFont || (bNewStyle && pFontList) )
                {
                    aParaText.append("&\"" + aNewData.maName);
                    if( pFontList )
                    {
                        FontMetric aFontMetric( pFontList->Get(
                            aNewData.maName,
                            (aNewData.mnWeight > EXC_FONTWGHT_NORMAL) ? WEIGHT_BOLD : WEIGHT_NORMAL,
                            aNewData.mbItalic ? ITALIC_NORMAL : ITALIC_NONE ) );
                        aNewData.maStyle = pFontList->GetStyleName( aFontMetric );
                        if( !aNewData.maStyle.isEmpty() )
                            aParaText.append("," + aNewData.maStyle);
                    }
                    aParaText.append("\"");
                }

                // height
                // is calculated wrong in ScPatternAttr::GetFromEditItemSet, because already in twips and not 100thmm
                // -> get it directly from edit engine item set
                aNewData.mnHeight = ulimit_cast< sal_uInt16 >( aEditSet.Get( EE_CHAR_FONTHEIGHT ).GetHeight() );
                aNewData.mnHeight = (aNewData.mnHeight + 10) / 20;
                bool bFontHtChanged = (aFontData.mnHeight != aNewData.mnHeight);
                if( bFontHtChanged )
                    aParaText.append("&" + OUString::number(aNewData.mnHeight));
                // update maximum paragraph height, convert to twips
                nParaHeight = ::std::max< sal_Int32 >( nParaHeight, aNewData.mnHeight * 20 );

                // underline
                aNewData.mnUnderline = EXC_FONTUNDERL_NONE;
                switch( aFont.GetUnderline() )
                {
                    case LINESTYLE_NONE:    aNewData.mnUnderline = EXC_FONTUNDERL_NONE;    break;
                    case LINESTYLE_SINGLE:  aNewData.mnUnderline = EXC_FONTUNDERL_SINGLE;  break;
                    case LINESTYLE_DOUBLE:  aNewData.mnUnderline = EXC_FONTUNDERL_DOUBLE;  break;
                    default:                aNewData.mnUnderline = EXC_FONTUNDERL_SINGLE;
                }
                if( aFontData.mnUnderline != aNewData.mnUnderline )
                {
                    sal_uInt8 nTmpUnderl = (aNewData.mnUnderline == EXC_FONTUNDERL_NONE) ?
                        aFontData.mnUnderline : aNewData.mnUnderline;
                    (nTmpUnderl == EXC_FONTUNDERL_SINGLE)? aParaText.append("&U") : aParaText.append("&E");
                }

                // font color
                aNewData.maComplexColor = std::move(aComplexColor);
                Color aNewColor = aNewData.maComplexColor.getFinalColor();

                if (!aFontData.maComplexColor.getFinalColor().IsRGBEqual(aNewColor))
                {
                    aParaText.append("&K" + aNewColor.AsRGBHexString());
                }

                // strikeout
                aNewData.mbStrikeout = (aFont.GetStrikeout() != STRIKEOUT_NONE);
                if( aFontData.mbStrikeout != aNewData.mbStrikeout )
                    aParaText.append("&S");

                // super/sub script
                const SvxEscapementItem& rEscapeItem = aEditSet.Get( EE_CHAR_ESCAPEMENT );
                aNewData.SetScEscapement( rEscapeItem.GetEsc() );
                if( aFontData.mnEscapem != aNewData.mnEscapem )
                {
                    switch(aNewData.mnEscapem)
                    {
                        // close the previous super/sub script.
                        case EXC_FONTESC_NONE:  (aFontData.mnEscapem == EXC_FONTESC_SUPER) ? aParaText.append("&X") : aParaText.append("&Y"); break;
                        case EXC_FONTESC_SUPER: aParaText.append("&X");  break;
                        case EXC_FONTESC_SUB:   aParaText.append("&Y");  break;
                        default: break;
                    }
                }

                aFontData = aNewData;

// --- text content or text fields ---

                const SvxFieldItem* pItem;
                if( (aSel.start.nIndex + 1 == aSel.end.nIndex) &&     // fields are single characters
                    (pItem = aEditSet.GetItemIfSet( EE_FEATURE_FIELD, false )) )
                {
                    if( const SvxFieldData* pFieldData = pItem->GetField() )
                    {
                        if( dynamic_cast<const SvxPageField*>( pFieldData) !=  nullptr )
                            aParaText.append("&P");
                        else if( dynamic_cast<const SvxPagesField*>( pFieldData) !=  nullptr )
                            aParaText.append("&N");
                        else if( dynamic_cast<const SvxDateField*>( pFieldData) !=  nullptr )
                            aParaText.append("&D");
                        else if( dynamic_cast<const SvxTimeField*>( pFieldData) != nullptr || dynamic_cast<const SvxExtTimeField*>( pFieldData) !=  nullptr )
                            aParaText.append("&T");
                        else if( dynamic_cast<const SvxTableField*>( pFieldData) !=  nullptr )
                            aParaText.append("&A");
                        else if( dynamic_cast<const SvxFileField*>( pFieldData) !=  nullptr )  // title -> file name
                            aParaText.append("&F");
                        else if( const SvxExtFileField* pFileField = dynamic_cast<const SvxExtFileField*>( pFieldData )  )
                        {
                            switch( pFileField->GetFormat() )
                            {
                                case SvxFileFormat::NameAndExt:
                                case SvxFileFormat::NameOnly:
                                    aParaText.append("&F");
                                break;
                                case SvxFileFormat::PathOnly:
                                    aParaText.append("&Z");
                                break;
                                case SvxFileFormat::PathFull:
                                    aParaText.append("&Z&F");
                                break;
                                default:
                                    OSL_FAIL( "XclExpHFConverter::AppendPortion - unknown file field" );
                            }
                        }
                    }
                }
                else
                {
                    OUString aPortionText( mrEE.GetText( aSel ) );
                    aPortionText = aPortionText.replaceAll( "&", "&&" );
                    // #i17440# space between font height and numbers in text
                    if( bFontHtChanged && aParaText.getLength() && !aPortionText.isEmpty() )
                    {
                        sal_Unicode cLast = aParaText[ aParaText.getLength() - 1 ];
                        sal_Unicode cFirst = aPortionText[0];
                        if( ('0' <= cLast) && (cLast <= '9') && ('0' <= cFirst) && (cFirst <= '9') )
                            aParaText.append(" ");
                    }
                    aParaText.append(aPortionText);
                }
            }

            aSel.start.nIndex = aSel.end.nIndex;
        }

        aText = ScGlobal::addToken( aText, aParaText, '\n' );
        aParaText.setLength(0);
        if( nParaHeight == 0 )
            nParaHeight = aFontData.mnHeight * 20;  // points -> twips
        nHeight += nParaHeight;
    }

    mrEE.SetUpdateLayout( bOldUpdateMode );

    if( !aText.isEmpty() )
    {
        maHFString += "&" + OUStringChar(cPortionCode) + aText;
        mnTotalHeight = ::std::max( mnTotalHeight, nHeight );
    }
}

// URL conversion =============================================================

namespace {

/** Encodes special parts of the path, i.e. directory separators and volume names.
    @param pTableName  Pointer to a table name to be encoded in this path, or 0. */
OUString lclEncodeDosPath(
    XclBiff eBiff, std::u16string_view path, bool bIsRel, const OUString* pTableName)
{
    OUStringBuffer aBuf;

    if (!path.empty())
    {
        aBuf.append(EXC_URLSTART_ENCODED);

        if ( path.length() > 2 && o3tl::starts_with(path, u"\\\\") )
        {
            // UNC
            aBuf.append(OUStringChar(EXC_URL_DOSDRIVE) + "@");
            path = path.substr(2);
        }
        else if ( path.length() > 2 && o3tl::starts_with(path.substr(1), u":\\") )
        {
            aBuf.append(OUStringChar(EXC_URL_DOSDRIVE) + OUStringChar(path[0]));
            path = path.substr(3);
        }
        else if ( !bIsRel )
        {
            // URL probably points to a document on a Unix-like file system
            aBuf.append(EXC_URL_DRIVEROOT);
        }

        // directories
        auto nPos = std::u16string_view::npos;
        while((nPos = path.find('\\')) != std::u16string_view::npos)
        {
            if ( o3tl::starts_with(path, u"..") )
                // parent dir (NOTE: the MS-XLS spec doesn't mention this, and
                // Excel seems confused by this token).
                aBuf.append(EXC_URL_PARENTDIR);
            else
                aBuf.append(path.substr(0,nPos) + OUStringChar(EXC_URL_SUBDIR));

            path = path.substr(nPos + 1);
        }

        // file name
        if (pTableName)    // enclose file name in brackets if table name follows
            aBuf.append(OUString::Concat("[") + path + "]");
        else
            aBuf.append(path);
    }
    else    // empty URL -> self reference
    {
        switch( eBiff )
        {
            case EXC_BIFF5:
                aBuf.append(pTableName ? EXC_URLSTART_SELFENCODED : EXC_URLSTART_SELF);
            break;
            case EXC_BIFF8:
                DBG_ASSERT( pTableName, "lclEncodeDosUrl - sheet name required for BIFF8" );
                aBuf.append(EXC_URLSTART_SELF);
            break;
            default:
                DBG_ERROR_BIFF();
        }
    }

    // table name
    if (pTableName)
        aBuf.append(*pTableName);

    // VirtualPath must be shorter than 255 chars ([MS-XLS].pdf 2.5.277)
    // It's better to truncate, than generate invalid file that Excel cannot open.
    if (aBuf.getLength() > 255)
        aBuf.setLength(255);

    return aBuf.makeStringAndClear();
}

bool isUrlRelative(const OUString& aUrl)
{
    css::uno::Reference<css::uri::XUriReferenceFactory> xUriFactory(
        css::uri::UriReferenceFactory::create(
            comphelper::getProcessComponentContext()));
    css::uno::Reference<css::uri::XUriReference> xUri(xUriFactory->parse(aUrl));

    return !xUri->isAbsolute();
}

} // namespace

OUString XclExpUrlHelper::EncodeUrl( const XclExpRoot& rRoot, std::u16string_view rAbsUrl, const OUString* pTableName )
{
    OUString aDosPath;
    bool bIsRel = false;

    if (rRoot.IsRelUrl())
    {
        OUString aUrlPath = INetURLObject::GetRelURL(
            rRoot.GetBasePath(), OUString(rAbsUrl),
            INetURLObject::EncodeMechanism::All,
            INetURLObject::DecodeMechanism::NONE,
            RTL_TEXTENCODING_UTF8, FSysStyle::Detect
        );

        if (isUrlRelative(aUrlPath))
        {
            bIsRel = true;
            osl::FileBase::getSystemPathFromFileURL(aUrlPath, aDosPath);
            aDosPath = aDosPath.replaceAll(u"/", u"\\");
        }
    }

    if (!bIsRel)
        aDosPath = INetURLObject(rAbsUrl).getFSysPath(FSysStyle::Dos);

    return lclEncodeDosPath(rRoot.GetBiff(), aDosPath, bIsRel, pTableName);
}

OUString XclExpUrlHelper::EncodeDde( std::u16string_view rApplic, std::u16string_view rTopic )
{
    return rApplic + OUStringChar(EXC_DDE_DELIM) + rTopic;
}

// Cached Value Lists =========================================================

XclExpCachedMatrix::XclExpCachedMatrix( const ScMatrix& rMatrix )
    : mrMatrix( rMatrix )
{
    mrMatrix.IncRef();
}
XclExpCachedMatrix::~XclExpCachedMatrix()
{
    mrMatrix.DecRef();
}

void XclExpCachedMatrix::GetDimensions( SCSIZE & nCols, SCSIZE & nRows ) const
{
    mrMatrix.GetDimensions( nCols, nRows );

    OSL_ENSURE( nCols && nRows, "XclExpCachedMatrix::GetDimensions - empty matrix" );
    OSL_ENSURE( nCols <= 256, "XclExpCachedMatrix::GetDimensions - too many columns" );
}

std::size_t XclExpCachedMatrix::GetSize() const
{
    SCSIZE nCols, nRows;

    GetDimensions( nCols, nRows );

    /*  The returned size may be wrong if the matrix contains strings. The only
        effect is that the export stream has to update a wrong record size which is
        faster than to iterate through all cached values and calculate their sizes. */
    return 3 + 9 * (nCols * nRows);
}

void XclExpCachedMatrix::Save( XclExpStream& rStrm ) const
{
    SCSIZE nCols, nRows;

    GetDimensions( nCols, nRows );

    if( rStrm.GetRoot().GetBiff() <= EXC_BIFF5 )
        // in BIFF2-BIFF7: 256 columns represented by 0 columns
        rStrm << static_cast< sal_uInt8 >( nCols ) << static_cast< sal_uInt16 >( nRows );
    else
        // in BIFF8: columns and rows decreased by 1
        rStrm << static_cast< sal_uInt8 >( nCols - 1 ) << static_cast< sal_uInt16 >( nRows - 1 );

    for( SCSIZE nRow = 0; nRow < nRows; ++nRow )
    {
        for( SCSIZE nCol = 0; nCol < nCols; ++nCol )
        {
            ScMatrixValue nMatVal = mrMatrix.Get( nCol, nRow );

            FormulaError nScError;
            if( ScMatValType::Empty == nMatVal.nType )
            {
                rStrm.SetSliceSize( 9 );
                rStrm << EXC_CACHEDVAL_EMPTY;
                rStrm.WriteZeroBytes( 8 );
            }
            else if( ScMatrix::IsNonValueType( nMatVal.nType ) )
            {
                XclExpString aStr( nMatVal.GetString().getString(), XclStrFlags::NONE );
                rStrm.SetSliceSize( 6 );
                rStrm << EXC_CACHEDVAL_STRING << aStr;
            }
            else if( ScMatValType::Boolean == nMatVal.nType )
            {
                sal_Int8 nBool = sal_Int8(nMatVal.GetBoolean());
                rStrm.SetSliceSize( 9 );
                rStrm << EXC_CACHEDVAL_BOOL << nBool;
                rStrm.WriteZeroBytes( 7 );
            }
            else if( (nScError = nMatVal.GetError()) != FormulaError::NONE )
            {
                sal_Int8 nError ( XclTools::GetXclErrorCode( nScError ) );
                rStrm.SetSliceSize( 9 );
                rStrm << EXC_CACHEDVAL_ERROR << nError;
                rStrm.WriteZeroBytes( 7 );
            }
            else
            {
                rStrm.SetSliceSize( 9 );
                rStrm << EXC_CACHEDVAL_DOUBLE << nMatVal.fVal;
            }
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
