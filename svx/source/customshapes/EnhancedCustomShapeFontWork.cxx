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

#include "EnhancedCustomShapeFontWork.hxx"
#include <svl/itemset.hxx>
#include <svx/compatflags.hxx>
#include <svx/svddef.hxx>
#include <svx/svdopath.hxx>
#include <vcl/kernarray.hxx>
#include <vcl/metric.hxx>
#include <svx/sdasitm.hxx>
#include <svx/sdtfsitm.hxx>
#include <vcl/virdev.hxx>
#include <svx/svditer.hxx>
#include <editeng/eeitem.hxx>
#include <editeng/frmdiritem.hxx>
#include <editeng/fontitem.hxx>
#include <editeng/postitem.hxx>
#include <editeng/wghtitem.hxx>
#include <editeng/fhgtitem.hxx>
#include <editeng/charscaleitem.hxx>
#include <svx/svdoashp.hxx>
#include <svx/sdshitm.hxx>
#include <svx/svdmodel.hxx>
#include <editeng/outlobj.hxx>
#include <editeng/editobj.hxx>
#include <o3tl/numeric.hxx>
#include <vector>
#include <numeric>
#include <algorithm>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/i18n/BreakIterator.hpp>
#include <com/sun/star/i18n/ScriptType.hpp>
#include <basegfx/polygon/b2dpolypolygontools.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <sal/log.hxx>
#include <rtl/math.hxx>
#include <comphelper/configuration.hxx>

using namespace com::sun::star;
using namespace com::sun::star::uno;

namespace {

struct FWCharacterData                  // representing a single character
{
    std::vector< tools::PolyPolygon >   vOutlines;
    tools::Rectangle                           aBoundRect;
};
struct FWParagraphData                  // representing a single paragraph
{
    OUString                            aString;
    std::vector< FWCharacterData >      vCharacters;
    tools::Rectangle                           aBoundRect;
    SvxFrameDirection                   nFrameDirection;
};
struct FWTextArea                       // representing multiple concluding paragraphs
{
    std::vector< FWParagraphData >      vParagraphs;
    tools::Rectangle                           aBoundRect;
    sal_Int32                           nHAlignMove = 0;
};
struct FWData                           // representing the whole text
{
    std::vector< FWTextArea >           vTextAreas;
    double                              fHorizontalTextScaling;
    double                              fVerticalTextScaling;
    sal_uInt32                          nMaxParagraphsPerTextArea;
    sal_Int32                           nSingleLineHeight;
    bool                                bSingleLineMode;
    bool                                bScaleX;
};

}

static bool InitializeFontWorkData(
    const SdrObjCustomShape& rSdrObjCustomShape,
    const sal_uInt16 nOutlinesCount2d,
    FWData& rFWData)
{
    bool bNoErr = false;
    bool bSingleLineMode = false;
    sal_uInt16 nTextAreaCount = nOutlinesCount2d;
    if ( nOutlinesCount2d & 1 )
        bSingleLineMode = true;
    else
        nTextAreaCount >>= 1;

    const SdrCustomShapeGeometryItem& rGeometryItem( rSdrObjCustomShape.GetMergedItem( SDRATTR_CUSTOMSHAPE_GEOMETRY ) );
    const css::uno::Any* pAny = rGeometryItem.GetPropertyValueByName( u"TextPath"_ustr, u"ScaleX"_ustr );
    if (pAny)
        *pAny >>= rFWData.bScaleX;
    else
        rFWData.bScaleX = false;

    if ( nTextAreaCount )
    {
        rFWData.bSingleLineMode = bSingleLineMode;

        // setting the strings
        OutlinerParaObject* pParaObj(rSdrObjCustomShape.GetOutlinerParaObject());

        if ( pParaObj )
        {
            const EditTextObject& rTextObj = pParaObj->GetTextObject();
            sal_Int32 nParagraphsCount = rTextObj.GetParagraphCount();

            // Collect all the lines from all paragraphs
            std::vector<int> aLineParaID;      // which para this line is in
            std::vector<int> aLineStart;       // where this line start in that para
            std::vector<int> aLineLength;
            std::vector<OUString> aParaText;
            for (sal_Int32 nPara = 0; nPara < nParagraphsCount; ++nPara)
            {
                aParaText.push_back(rTextObj.GetText(nPara));
                sal_Int32 nPos = 0;
                sal_Int32 nPrevPos = 0;
                do
                {
                    // search line break.
                    if (!rSdrObjCustomShape.getSdrModelFromSdrObject().GetCompatibilityFlag(
                            SdrCompatibilityFlag::LegacyFontwork))
                        nPos = aParaText[nPara].indexOf(sal_Unicode(u'\1'), nPrevPos);
                    else
                        nPos = -1; // tdf#148000: ignore line breaks in legacy fontworks

                    aLineParaID.push_back(nPara);
                    aLineStart.push_back(nPrevPos);
                    aLineLength.push_back((nPos >= 0 ? nPos : aParaText[nPara].getLength())
                                          - nPrevPos);
                    nPrevPos = nPos + 1;
                } while (nPos >= 0);
            }

            sal_Int32 nLinesLeft = aLineParaID.size();

            rFWData.nMaxParagraphsPerTextArea = ((nLinesLeft - 1) / nTextAreaCount) + 1;
            sal_Int32 nLine = 0;
            while (nLinesLeft && nTextAreaCount)
            {
                FWTextArea aTextArea;
                sal_Int32 nLinesInPara = ((nLinesLeft - 1) / nTextAreaCount) + 1;
                for (sal_Int32 i = 0; i < nLinesInPara; ++i, ++nLine)
                {
                    FWParagraphData aParagraphData;
                    aParagraphData.aString = aParaText[aLineParaID[nLine]].subView(
                        aLineStart[nLine], aLineLength[nLine]);

                    // retrieving some paragraph attributes
                    const SfxItemSet& rParaSet = rTextObj.GetParaAttribs(aLineParaID[nLine]);
                    aParagraphData.nFrameDirection = rParaSet.Get(EE_PARA_WRITINGDIR).GetValue();
                    aTextArea.vParagraphs.push_back(aParagraphData);
                }
                rFWData.vTextAreas.push_back(aTextArea);
                nLinesLeft -= nLinesInPara;
                nTextAreaCount--;
            }

            bNoErr = true;
        }
    }
    return bNoErr;
}

static double GetLength( const tools::Polygon& rPolygon )
{
    double fLength = 0;
    if ( rPolygon.GetSize() > 1 )
    {
        sal_uInt16 nCount = rPolygon.GetSize();
        while( --nCount )
            fLength += rPolygon.CalcDistance( nCount, nCount - 1 );
    }
    return fLength;
}


/* CalculateHorizontalScalingFactor returns the horizontal scaling factor for
the whole text object, so that each text will match its corresponding 2d Outline */
static void CalculateHorizontalScalingFactor(
    const SdrObjCustomShape& rSdrObjCustomShape,
    FWData& rFWData,
    const tools::PolyPolygon& rOutline2d)
{
    double fScalingFactor = 1.0;
    rFWData.fVerticalTextScaling = 1.0;

    sal_uInt16 i = 0;
    bool bSingleLineMode = false;
    sal_uInt16 nOutlinesCount2d = rOutline2d.Count();

    vcl::Font aFont;
    const SvxFontItem& rFontItem( rSdrObjCustomShape.GetMergedItem( EE_CHAR_FONTINFO ) );
    const SvxFontHeightItem& rFontHeight( rSdrObjCustomShape.GetMergedItem( EE_CHAR_FONTHEIGHT ) );
    sal_Int32 nFontSize = rFontHeight.GetHeight();

    if (rFWData.bScaleX)
        aFont.SetFontHeight( nFontSize );
    else
        aFont.SetFontHeight( rSdrObjCustomShape.GetLogicRect().GetHeight() / rFWData.nMaxParagraphsPerTextArea );

    aFont.SetAlignment( ALIGN_TOP );
    aFont.SetFamilyName( rFontItem.GetFamilyName() );
    aFont.SetFamily( rFontItem.GetFamily() );
    aFont.SetStyleName( rFontItem.GetStyleName() );
    const SvxPostureItem& rPostureItem = rSdrObjCustomShape.GetMergedItem( EE_CHAR_ITALIC );
    aFont.SetItalic( rPostureItem.GetPosture() );

    const SvxWeightItem& rWeightItem = rSdrObjCustomShape.GetMergedItem( EE_CHAR_WEIGHT );
    aFont.SetWeight( rWeightItem.GetWeight() );
    aFont.SetOrientation( 0_deg10 );
    // initializing virtual device

    ScopedVclPtrInstance< VirtualDevice > pVirDev(DeviceFormat::WITHOUT_ALPHA);
    pVirDev->SetMapMode(MapMode(MapUnit::Map100thMM));
    pVirDev->SetFont( aFont );
    pVirDev->SetAntialiasing( AntialiasingFlags::DisableText );

    if ( nOutlinesCount2d & 1 )
        bSingleLineMode = true;

    // In case of rFWData.bScaleX == true it loops with reduced font size until the current run
    // results in a fScalingFactor >=1.0. The fact, that case rFWData.bScaleX == true keeps font
    // size if possible, is not done here with scaling factor 1 but is done in method
    // FitTextOutlinesToShapeOutlines()
    do
    {
        i = 0;
        bool bScalingFactorDefined = false; // New calculation for each font size
        for( const auto& rTextArea : rFWData.vTextAreas )
        {
            // calculating the width of the corresponding 2d text area
            double fWidth = GetLength( rOutline2d.GetObject( i++ ) );
            if ( !bSingleLineMode )
            {
                fWidth += GetLength( rOutline2d.GetObject( i++ ) );
                fWidth /= 2.0;
            }

            for( const auto& rParagraph : rTextArea.vParagraphs )
            {
                double fTextWidth = pVirDev->GetTextWidth( rParagraph.aString );
                if ( fTextWidth > 0.0 )
                {
                    double fScale = fWidth / fTextWidth;
                    if ( !bScalingFactorDefined )
                    {
                        fScalingFactor = fScale;
                        bScalingFactorDefined = true;
                    }
                    else if (fScale < fScalingFactor)
                    {
                        fScalingFactor = fScale;
                    }
                }
            }
        }

        if (fScalingFactor < 1.0)
        {
            nFontSize--;
            aFont.SetFontHeight( nFontSize );
            pVirDev->SetFont( aFont );
        }
    }
    while (rFWData.bScaleX && fScalingFactor < 1.0 && nFontSize > 1 );

    if (nFontSize > 1)
        rFWData.fVerticalTextScaling = static_cast<double>(nFontSize) / rFontHeight.GetHeight();

    rFWData.fHorizontalTextScaling = fScalingFactor;
}

static void GetTextAreaOutline(
    const FWData& rFWData,
    const SdrObjCustomShape& rSdrObjCustomShape,
    FWTextArea& rTextArea,
    bool bSameLetterHeights)
{
    bool bIsVertical(rSdrObjCustomShape.IsVerticalWriting());
    sal_Int32 nVerticalOffset = rFWData.nMaxParagraphsPerTextArea > rTextArea.vParagraphs.size()
                                    ? rFWData.nSingleLineHeight / 2 : 0;

    for( auto& rParagraph : rTextArea.vParagraphs )
    {
        const OUString& rText = rParagraph.aString;
        if ( !rText.isEmpty() )
        {
            // generating vcl/font
            sal_uInt16 nScriptType = i18n::ScriptType::LATIN;
            Reference< i18n::XBreakIterator > xBI( EnhancedCustomShapeFontWork::GetBreakIterator() );
            if ( xBI.is() )
            {
                nScriptType = xBI->getScriptType( rText, 0 );
                if( i18n::ScriptType::WEAK == nScriptType )
                {
                    sal_Int32 nChg = xBI->endOfScript( rText, 0, nScriptType );
                    if (nChg < rText.getLength() && nChg >= 0)
                        nScriptType = xBI->getScriptType( rText, nChg );
                    else
                        nScriptType = i18n::ScriptType::LATIN;
                }
            }
            sal_uInt16 nFntItm = EE_CHAR_FONTINFO;
            if ( nScriptType == i18n::ScriptType::COMPLEX )
                nFntItm = EE_CHAR_FONTINFO_CTL;
            else if ( nScriptType == i18n::ScriptType::ASIAN )
                nFntItm = EE_CHAR_FONTINFO_CJK;
            const SvxFontItem& rFontItem = static_cast<const SvxFontItem&>(rSdrObjCustomShape.GetMergedItem( nFntItm ));
            vcl::Font aFont;

            aFont.SetFontHeight( rFWData.nSingleLineHeight );

            aFont.SetAlignment( ALIGN_TOP );

            aFont.SetFamilyName( rFontItem.GetFamilyName() );
            aFont.SetFamily( rFontItem.GetFamily() );
            aFont.SetStyleName( rFontItem.GetStyleName() );
            aFont.SetOrientation( 0_deg10 );

            const SvxPostureItem& rPostureItem = rSdrObjCustomShape.GetMergedItem( EE_CHAR_ITALIC );
            aFont.SetItalic( rPostureItem.GetPosture() );

            const SvxWeightItem& rWeightItem = rSdrObjCustomShape.GetMergedItem( EE_CHAR_WEIGHT );
            aFont.SetWeight( rWeightItem.GetWeight() );

            // initializing virtual device
            ScopedVclPtrInstance< VirtualDevice > pVirDev(DeviceFormat::WITHOUT_ALPHA);
            pVirDev->SetMapMode(MapMode(MapUnit::Map100thMM));
            pVirDev->SetFont( aFont );
            pVirDev->SetAntialiasing( AntialiasingFlags::DisableText );

            pVirDev->EnableRTL();
            if ( rParagraph.nFrameDirection == SvxFrameDirection::Horizontal_RL_TB )
                pVirDev->SetLayoutMode( vcl::text::ComplexTextLayoutFlags::BiDiRtl );

            const SvxCharScaleWidthItem& rCharScaleWidthItem = rSdrObjCustomShape.GetMergedItem( EE_CHAR_FONTWIDTH );
            sal_uInt16 nCharScaleWidth = rCharScaleWidthItem.GetValue();
            sal_Int32 nWidth = 0;

            // VERTICAL
            if ( bIsVertical )
            {
                // vertical _> each single character needs to be rotated by 90
                sal_Int32 i;
                sal_Int32 nHeight = 0;
                tools::Rectangle aSingleCharacterUnion;
                for ( i = 0; i < rText.getLength(); i++ )
                {
                    FWCharacterData aCharacterData;
                    OUString aCharText( rText[ i ] );
                    if ( pVirDev->GetTextOutlines( aCharacterData.vOutlines, aCharText, 0, 0, -1, nWidth, {} ) )
                    {
                        sal_Int32 nTextWidth = pVirDev->GetTextWidth( aCharText);
                        if ( aCharacterData.vOutlines.empty() )
                        {
                            nHeight += rFWData.nSingleLineHeight;
                        }
                        else
                        {
                            for ( auto& rOutline : aCharacterData.vOutlines )
                            {
                                // rotating
                                rOutline.Rotate( Point( nTextWidth / 2, rFWData.nSingleLineHeight / 2 ), 900_deg10 );
                                aCharacterData.aBoundRect.Union( rOutline.GetBoundRect() );
                            }
                            for ( auto& rOutline : aCharacterData.vOutlines )
                            {
                                sal_Int32 nM = - aCharacterData.aBoundRect.Left() + nHeight;
                                rOutline.Move( nM, 0 );
                                aCharacterData.aBoundRect.Move( nM, 0 );
                            }
                            nHeight += aCharacterData.aBoundRect.GetWidth() + ( rFWData.nSingleLineHeight / 5 );
                            aSingleCharacterUnion.Union( aCharacterData.aBoundRect );
                        }
                    }
                    rParagraph.vCharacters.push_back( aCharacterData );
                }
                for ( auto& rCharacter : rParagraph.vCharacters )
                {
                    for ( auto& rOutline : rCharacter.vOutlines )
                    {
                        rOutline.Move( ( aSingleCharacterUnion.GetWidth() - rCharacter.aBoundRect.GetWidth() ) / 2, 0 );
                    }
                }
            }
            else
            {
                KernArray aDXArray;
                if ( ( nCharScaleWidth != 100 ) && nCharScaleWidth )
                {   // applying character spacing
                    pVirDev->GetTextArray( rText, &aDXArray);
                    FontMetric aFontMetric( pVirDev->GetFontMetric() );
                    aFont.SetAverageFontWidth( static_cast<sal_Int32>( static_cast<double>(aFontMetric.GetAverageFontWidth()) * ( double(100) / static_cast<double>(nCharScaleWidth) ) ) );
                    pVirDev->SetFont( aFont );
                }
                FWCharacterData aCharacterData;
                if ( pVirDev->GetTextOutlines( aCharacterData.vOutlines, rText, 0, 0, -1, nWidth, aDXArray ) )
                {
                    rParagraph.vCharacters.push_back( aCharacterData );
                }
                else
                {
                    // GetTextOutlines failed what usually means that it is
                    // not implemented. To make FontWork not fail (it is
                    // dependent of graphic content to get a Range) create
                    // a rectangle substitution for now
                    pVirDev->GetTextArray( rText, &aDXArray);
                    aCharacterData.vOutlines.clear();

                    if(!aDXArray.empty())
                    {
                        for(size_t a(0); a < aDXArray.size(); a++)
                        {
                            const basegfx::B2DPolygon aPolygon(
                                basegfx::utils::createPolygonFromRect(
                                basegfx::B2DRange(
                                    0 == a ? 0 : aDXArray[a - 1],
                                    0,
                                    aDXArray[a],
                                    aFont.GetFontHeight()
                                )));
                            aCharacterData.vOutlines.emplace_back(tools::Polygon(aPolygon));
                        }
                    }
                    else
                    {
                        const basegfx::B2DPolygon aPolygon(
                            basegfx::utils::createPolygonFromRect(
                            basegfx::B2DRange(
                                0,
                                0,
                                10,
                                aFont.GetFontHeight()
                            )));
                        aCharacterData.vOutlines.emplace_back(tools::Polygon(aPolygon));
                    }


                    rParagraph.vCharacters.push_back( aCharacterData );
                }
            }

            // vertical alignment
            for ( auto& rCharacter : rParagraph.vCharacters )
            {
                for( tools::PolyPolygon& rPolyPoly : rCharacter.vOutlines )
                {
                    if ( nVerticalOffset )
                        rPolyPoly.Move( 0, nVerticalOffset );

                    // retrieving the boundrect for the paragraph
                    tools::Rectangle aBoundRect( rPolyPoly.GetBoundRect() );
                    rParagraph.aBoundRect.Union( aBoundRect );
                }
            }
        }
        // updating the boundrect for the text area by merging the current paragraph boundrect
        if ( rParagraph.aBoundRect.IsEmpty() )
        {
            if ( rTextArea.aBoundRect.IsEmpty() )
                rTextArea.aBoundRect = tools::Rectangle( Point( 0, 0 ), Size( 1, rFWData.nSingleLineHeight ) );
            else
                rTextArea.aBoundRect.AdjustBottom(rFWData.nSingleLineHeight );
        }
        else
        {
            tools::Rectangle& rParagraphBoundRect = rParagraph.aBoundRect;
            rTextArea.aBoundRect.Union( rParagraphBoundRect );

            if ( bSameLetterHeights )
            {
                for ( auto& rCharacter : rParagraph.vCharacters )
                {
                    for( auto& rOutline : rCharacter.vOutlines )
                    {
                        tools::Rectangle aPolyPolyBoundRect( rOutline.GetBoundRect() );
                        if (aPolyPolyBoundRect.GetHeight() != rParagraphBoundRect.GetHeight() && aPolyPolyBoundRect.GetHeight())
                            rOutline.Scale( 1.0, static_cast<double>(rParagraphBoundRect.GetHeight()) / aPolyPolyBoundRect.GetHeight() );
                        aPolyPolyBoundRect = rOutline.GetBoundRect();
                        sal_Int32 nMove = aPolyPolyBoundRect.Top() - rParagraphBoundRect.Top();
                        if ( nMove )
                            rOutline.Move( 0, -nMove );
                    }
                }
            }
        }
        if ( bIsVertical )
            nVerticalOffset -= rFWData.nSingleLineHeight;
        else
            nVerticalOffset += rFWData.nSingleLineHeight;
    }
}

static bool GetFontWorkOutline(
    FWData& rFWData,
    const SdrObjCustomShape& rSdrObjCustomShape)
{
    SdrTextHorzAdjust eHorzAdjust(rSdrObjCustomShape.GetMergedItem( SDRATTR_TEXT_HORZADJUST ).GetValue());
    drawing::TextFitToSizeType const eFTS(rSdrObjCustomShape.GetMergedItem( SDRATTR_TEXT_FITTOSIZE ).GetValue());

    bool bSameLetterHeights = false;
    const SdrCustomShapeGeometryItem& rGeometryItem(rSdrObjCustomShape.GetMergedItem( SDRATTR_CUSTOMSHAPE_GEOMETRY ));
    const css::uno::Any* pAny = rGeometryItem.GetPropertyValueByName( u"TextPath"_ustr, u"SameLetterHeights"_ustr );
    if ( pAny )
        *pAny >>= bSameLetterHeights;

    const SvxFontHeightItem& rFontHeight( rSdrObjCustomShape.GetMergedItem( EE_CHAR_FONTHEIGHT ) );
    if (rFWData.bScaleX)
        rFWData.nSingleLineHeight = rFWData.fVerticalTextScaling * rFontHeight.GetHeight();
    else
        rFWData.nSingleLineHeight = static_cast<sal_Int32>( ( static_cast<double>( rSdrObjCustomShape.GetLogicRect().GetHeight() )
                                                    / rFWData.nMaxParagraphsPerTextArea ) * rFWData.fHorizontalTextScaling );

    if (rFWData.nSingleLineHeight == SAL_MIN_INT32)
        return false;

    for ( auto& rTextArea : rFWData.vTextAreas )
    {
        GetTextAreaOutline(
            rFWData,
            rSdrObjCustomShape,
            rTextArea,
            bSameLetterHeights);

        if (eFTS == drawing::TextFitToSizeType_ALLLINES ||
            // tdf#97630 interpret PROPORTIONAL same as ALLLINES so we don't
            // need another ODF attribute!
            eFTS == drawing::TextFitToSizeType_PROPORTIONAL)
        {
            for ( auto& rParagraph : rTextArea.vParagraphs )
            {
                sal_Int32 nParaWidth = rParagraph.aBoundRect.GetWidth();
                if ( nParaWidth )
                {
                    double fScale = static_cast<double>(rTextArea.aBoundRect.GetWidth()) / nParaWidth;

                    for ( auto& rCharacter : rParagraph.vCharacters )
                    {
                        for( auto& rOutline : rCharacter.vOutlines )
                        {
                            rOutline.Scale( fScale, 1.0 );
                        }
                    }
                }
            }
        }
        else if (rFWData.bScaleX)
        {
            const SdrTextVertAdjust nVertJustify = rSdrObjCustomShape.GetMergedItem( SDRATTR_TEXT_VERTADJUST ).GetValue();
            double fFactor = nVertJustify == SdrTextVertAdjust::SDRTEXTVERTADJUST_BOTTOM ? -0.5 : ( nVertJustify == SdrTextVertAdjust::SDRTEXTVERTADJUST_TOP ? 0.5 : 0 );

            for ( auto& rParagraph : rTextArea.vParagraphs )
            {
                sal_Int32 nHorzDiff = 0;
                sal_Int32 nVertDiff = static_cast<double>( rFWData.nSingleLineHeight ) * fFactor * ( rTextArea.vParagraphs.size() - 1 );
                rTextArea.nHAlignMove = nVertDiff;

                if ( eHorzAdjust == SDRTEXTHORZADJUST_CENTER )
                    nHorzDiff = ( rFWData.fHorizontalTextScaling * rTextArea.aBoundRect.GetWidth() - rParagraph.aBoundRect.GetWidth() ) / 2;
                else if ( eHorzAdjust == SDRTEXTHORZADJUST_RIGHT )
                    nHorzDiff = ( rFWData.fHorizontalTextScaling * rTextArea.aBoundRect.GetWidth() - rParagraph.aBoundRect.GetWidth() );

                if (nHorzDiff || nVertDiff)
                {
                    for ( auto& rCharacter : rParagraph.vCharacters )
                    {
                        for( auto& rOutline : rCharacter.vOutlines )
                        {
                            rOutline.Move( nHorzDiff, nVertDiff );
                        }
                    }
                }
            }
        }
        else
        {
            switch( eHorzAdjust )
            {
                case SDRTEXTHORZADJUST_RIGHT :
                case SDRTEXTHORZADJUST_CENTER:
                {
                    for ( auto& rParagraph : rTextArea.vParagraphs )
                    {
                        sal_Int32 nHorzDiff = 0;
                        if ( eHorzAdjust == SDRTEXTHORZADJUST_CENTER )
                            nHorzDiff = ( rTextArea.aBoundRect.GetWidth() - rParagraph.aBoundRect.GetWidth() ) / 2;
                        else if ( eHorzAdjust == SDRTEXTHORZADJUST_RIGHT )
                            nHorzDiff = ( rTextArea.aBoundRect.GetWidth() - rParagraph.aBoundRect.GetWidth() );
                        if ( nHorzDiff )
                        {
                            for ( auto& rCharacter : rParagraph.vCharacters )
                            {
                                for( auto& rOutline : rCharacter.vOutlines )
                                {
                                    rOutline.Move( nHorzDiff, 0 );
                                }
                            }
                        }
                    }
                }
                break;
                default:
                case SDRTEXTHORZADJUST_BLOCK : break;   // don't know
                case SDRTEXTHORZADJUST_LEFT : break;    // already left aligned -> nothing to do
            }
        }
    }

    return true;
}

static basegfx::B2DPolyPolygon GetOutlinesFromShape2d( const SdrObject* pShape2d )
{
    basegfx::B2DPolyPolygon aOutlines2d;

    SdrObjListIter aObjListIter( *pShape2d, SdrIterMode::DeepWithGroups );
    while( aObjListIter.IsMore() )
    {
        SdrObject* pPartObj = aObjListIter.Next();
        if ( auto pPathObj = dynamic_cast<const SdrPathObj*>( pPartObj))
        {
            basegfx::B2DPolyPolygon aCandidate(pPathObj->GetPathPoly());
            if(aCandidate.areControlPointsUsed())
            {
                aCandidate = basegfx::utils::adaptiveSubdivideByAngle(aCandidate);
            }
            aOutlines2d.append(aCandidate);
        }
    }

    return aOutlines2d;
}

static void CalcDistances( const tools::Polygon& rPoly, std::vector< double >& rDistances )
{
    sal_uInt16 i, nCount = rPoly.GetSize();
    if ( nCount <= 1 )
        return;

    for ( i = 0; i < nCount; i++ )
    {
        double fDistance = i ? rPoly.CalcDistance( i, i - 1 ) : 0.0;
        rDistances.push_back( fDistance );
    }
    std::partial_sum( rDistances.begin(), rDistances.end(), rDistances.begin() );
    double fLength = rDistances[ rDistances.size() - 1 ];
    if ( fLength > 0.0 )
    {
        for ( auto& rDistance : rDistances )
            rDistance /= fLength;
    }
}

static void InsertMissingOutlinePoints( const std::vector< double >& rDistances,
                                 const tools::Rectangle& rTextAreaBoundRect, tools::Polygon& rPoly )
{
    sal_uInt16 nSize = rPoly.GetSize();
    if (nSize == 0)
        return;

    tools::Long nTextWidth = rTextAreaBoundRect.GetWidth();

    if (nTextWidth == 0)
        throw o3tl::divide_by_zero();

    double fLastDistance = 0.0;
    for (sal_uInt16 i = 0; i < nSize; ++i)
    {
        Point& rPoint = rPoly[ i ];
        double fDistance = static_cast<double>( rPoint.X() - rTextAreaBoundRect.Left() ) / static_cast<double>(nTextWidth);
        if ( i )
        {
            if ( fDistance > fLastDistance )
            {
                std::vector< double >::const_iterator aIter = std::upper_bound( rDistances.begin(), rDistances.end(), fLastDistance );
                if  ( aIter != rDistances.end() && ( *aIter > fLastDistance ) && ( *aIter < fDistance ) )
                {
                    Point& rPt0 = rPoly[ i - 1 ];
                    sal_Int32 fX = rPoint.X() - rPt0.X();
                    sal_Int32 fY = rPoint.Y() - rPt0.Y();
                    double fd = ( 1.0 / ( fDistance - fLastDistance ) ) * ( *aIter - fLastDistance );
                    rPoly.Insert( i, Point( static_cast<sal_Int32>( rPt0.X() + fX * fd ), static_cast<sal_Int32>( rPt0.Y() + fY * fd ) ) );
                    fDistance = *aIter;
                }
            }
            else if ( fDistance < fLastDistance )
            {
                std::vector< double >::const_iterator aIter = std::lower_bound( rDistances.begin(), rDistances.end(), fLastDistance );
                if  ( aIter != rDistances.begin() )
                {
                    --aIter;
                    if ( ( *aIter > fDistance ) && ( *aIter < fLastDistance ) )
                    {
                        Point& rPt0 = rPoly[ i - 1 ];
                        sal_Int32 fX = rPoint.X() - rPt0.X();
                        sal_Int32 fY = rPoint.Y() - rPt0.Y();
                        double fd = ( 1.0 / ( fDistance - fLastDistance ) ) * ( *aIter - fLastDistance );
                        rPoly.Insert( i, Point( static_cast<sal_Int32>( rPt0.X() + fX * fd ), static_cast<sal_Int32>( rPt0.Y() + fY * fd ) ) );
                        fDistance = *aIter;
                    }
                }
            }
        }
        fLastDistance = fDistance;
    }
}

//only 2 types used: 'const tools::Polygon&' and 'const std::vector<Point>&'
template <class T>
static void GetPoint( T rPoly, const std::vector< double >& rDistances, const double& fX, double& fx1, double& fy1 )
{
    fy1 = fx1 = 0.0;
    if (rPoly.size() <= 1)
        return;

    std::vector< double >::const_iterator aIter = std::lower_bound( rDistances.begin(), rDistances.end(), fX );
    sal_uInt16 nIdx = sal::static_int_cast<sal_uInt16>( std::distance( rDistances.begin(), aIter ) );
    if ( aIter == rDistances.end() )
        nIdx--;
    const Point& rPt = rPoly[ nIdx ];
    fx1 = rPt.X();
    fy1 = rPt.Y();
    if ( !nIdx || ( aIter == rDistances.end() ) || rtl::math::approxEqual( *aIter, fX ) )
        return;

    nIdx = sal::static_int_cast<sal_uInt16>( std::distance( rDistances.begin(), aIter ) );
    double fDist0 = *( aIter - 1 );
    double fd = ( 1.0 / ( *aIter - fDist0 ) ) * ( fX - fDist0 );
    const Point& rPt2 = rPoly[ nIdx - 1 ];
    double fWidth = rPt.X() - rPt2.X();
    double fHeight= rPt.Y() - rPt2.Y();
    fWidth *= fd;
    fHeight*= fd;
    fx1 = rPt2.X() + fWidth;
    fy1 = rPt2.Y() + fHeight;
}

static void FitTextOutlinesToShapeOutlines(const tools::PolyPolygon& aOutlines2d, FWData& rFWData,
                                           SdrTextHorzAdjust eHorzAdjust, bool bPPFontwork)
{
    sal_uInt16 nOutline2dIdx = 0;
    for( auto& rTextArea : rFWData.vTextAreas )
    {
        tools::Rectangle rTextAreaBoundRect = rTextArea.aBoundRect;
        sal_Int32 nLeft = rTextAreaBoundRect.Left();
        sal_Int32 nTop = rTextAreaBoundRect.Top();
        sal_Int32 nWidth = rTextAreaBoundRect.GetWidth();
        sal_Int32 nHeight= rTextAreaBoundRect.GetHeight();

        if (rFWData.bScaleX)
        {
            nWidth *= rFWData.fHorizontalTextScaling;
        }

        if ( rFWData.bSingleLineMode && nHeight && nWidth )
        {
            if ( nOutline2dIdx >= aOutlines2d.Count() )
                break;
            const tools::Polygon& rOutlinePoly( aOutlines2d[ nOutline2dIdx++ ] );
            const sal_uInt16 nPointCount = rOutlinePoly.GetSize();
            if ( nPointCount > 1 )
            {
                std::vector< double > vDistances;
                vDistances.reserve( nPointCount );
                CalcDistances( rOutlinePoly, vDistances );

                if ( !vDistances.empty() )
                {
                    // horizontal alignment: how much we have to move text to the right.
                    int nAdjust = -1;
                    switch (eHorzAdjust)
                    {
                        case SDRTEXTHORZADJUST_RIGHT:
                            nAdjust = 2; // 2 half of the possible
                            break;
                        case SDRTEXTHORZADJUST_CENTER:
                            nAdjust = 1; // 1 half of the possible
                            break;
                        case SDRTEXTHORZADJUST_BLOCK:
                            nAdjust = -1; // don't know what it is, so don't even align
                            break;
                        case SDRTEXTHORZADJUST_LEFT:
                            nAdjust = 0; // no need to move
                            break;
                    }

                    if (bPPFontwork && rTextArea.vParagraphs.size() > 1 && nAdjust >= 0)
                    {
                        // If we have multiple lines of text to fit to the outline (curve)
                        // then we have to be able to calculate outer versions of the outline
                        // where we can fit the next lines of texts
                        // those outer lines will be wider (or shorter) as the original outline
                        // and probably will looks different as the original outline.
                        //
                        // for example if we have an outline like this:
                        // <____>
                        // then the middle part will have the same normals, so distances there,
                        //  will not change for an outer outline
                        // while the points near the edge will have different normals,
                        //  distances around there will increase for an outer (wider) outline

                        //Normal vectors for every rOutlinePoly point. 1024 long
                        std::vector<Point> vNorm;
                        //wider curve path points, for current paragraph (rOutlinePoly + vNorm*line)
                        std::vector<Point> vCurOutline;
                        //distances between points of this wider curve
                        std::vector<double> vCurDistances;

                        vCurDistances.reserve(nPointCount);
                        vCurOutline.reserve(nPointCount);
                        vNorm.reserve(nPointCount);

                        // Calculate Normal vectors, and allocate curve data
                        sal_uInt16 i;
                        for (i = 0; i < nPointCount; i++)
                        {
                            //Normal vector for a point will be calculated from its neighbour points
                            //except if it is in the start/end of the vector
                            sal_uInt16 nPointIdx1 = i == 0 ? i : i - 1;
                            sal_uInt16 nPointIdx2 = i == nPointCount - 1 ? i : i + 1;

                            Point aPoint = rOutlinePoly.GetPoint(nPointIdx2)
                                           - rOutlinePoly.GetPoint(nPointIdx1);

                            double fLen = hypot(aPoint.X(), aPoint.Y());

                            if (fLen > 0)
                            {
                                //Rotate by 90 degree, and divide by length, to get normal vector
                                vNorm.emplace_back(aPoint.getY() * 1024 / fLen,
                                                   -aPoint.getX() * 1024 / fLen);
                            }
                            else
                            {
                                vNorm.emplace_back(0, 0);
                            }
                            vCurOutline.emplace_back(Point());
                            vCurDistances.push_back(0);

                        }

                        for( auto& rParagraph : rTextArea.vParagraphs )
                        {
                            //calculate the actual outline length, and its align adjustments
                            double fAdjust;
                            double fCurWidth;

                            // distance between the original and the current curve
                            double fCurvesDist = rTextArea.aBoundRect.GetHeight() / 2.0
                                                 + rTextArea.aBoundRect.Top()
                                                 - rParagraph.aBoundRect.Center().Y();
                            // vertical alignment adjust
                            fCurvesDist -= rTextArea.nHAlignMove;

                            for (i = 0; i < nPointCount; i++)
                            {
                                vCurOutline[i]
                                    = rOutlinePoly.GetPoint(i) + vNorm[i] * fCurvesDist / 1024.0;
                                if (i > 0)
                                {
                                    //calculate distances between points on the outer outline
                                    const double fDx = vCurOutline[i].X() - vCurOutline[i - 1].X();
                                    const double fDy = vCurOutline[i].Y() - vCurOutline[i - 1].Y();
                                    vCurDistances[i] = hypot(fDx, fDy);
                                }
                                else
                                    vCurDistances[i] = 0;
                            }
                            std::partial_sum(vCurDistances.begin(), vCurDistances.end(),
                                             vCurDistances.begin());
                            fCurWidth = vCurDistances[vCurDistances.size() - 1];
                            if (fCurWidth > 0.0)
                            {
                                for (auto& rDistance : vCurDistances)
                                    rDistance /= fCurWidth;
                            }

                            // if the current outline is longer then the text to fit in,
                            // then we have to divide the bonus space between the
                            // before-/after- text area.
                            // fAdjust means how much space we put before the text.
                            if (fCurWidth > rParagraph.aBoundRect.GetWidth())
                            {
                                fAdjust
                                    = nAdjust * (fCurWidth - rParagraph.aBoundRect.GetWidth()) / 2;
                            }
                            else
                                fAdjust = -1;   // we need to shrink the text to fit the curve

                            for ( auto& rCharacter : rParagraph.vCharacters )
                            {
                                for (tools::PolyPolygon& rPolyPoly : rCharacter.vOutlines)
                                {
                                    tools::Rectangle aBoundRect(rPolyPoly.GetBoundRect());
                                    double fx1 = aBoundRect.Left() - nLeft;
                                    double fx2 = aBoundRect.Right() - nLeft;

                                    double fParaRectWidth = rParagraph.aBoundRect.GetWidth();
                                    // Undo Horizontal alignment, hacked into poly coords,
                                    // so we can calculate it the right way
                                    double fHA = (rFWData.fHorizontalTextScaling
                                                      * rTextArea.aBoundRect.GetWidth()
                                                  - rParagraph.aBoundRect.GetWidth())
                                                 * nAdjust / 2;

                                    fx1 -= fHA;
                                    fx2 -= fHA;

                                    double fy1, fy2;
                                    double fM1 = fx1 / fParaRectWidth;
                                    double fM2 = fx2 / fParaRectWidth;

                                    // if fAdjust<0, then it means, the text was longer, as
                                    // the current outline, so we will skip the text scaling, and
                                    // the text horizontal alignment adjustment
                                    // so the text will be rendered just as long as the curve is.
                                    if (fAdjust >= 0)
                                    {
                                        fM1 = (fM1 * fParaRectWidth + fAdjust) / fCurWidth;
                                        fM2 = (fM2 * fParaRectWidth + fAdjust) / fCurWidth;
                                    }
                                    // 0 <= fM1,fM2 <= 1 should be true, but rounding errors can
                                    // make a small mistake.
                                    // make sure they are >0 because GetPoint() need that
                                    if (fM1 < 0) fM1 = 0;
                                    if (fM2 < 0) fM2 = 0;

                                    GetPoint(vCurOutline, vCurDistances, fM1, fx1, fy1);
                                    GetPoint(vCurOutline, vCurDistances, fM2, fx2, fy2);

                                    double fvx = fy2 - fy1;
                                    double fvy = - ( fx2 - fx1 );
                                    fx1 = fx1 + ( ( fx2 - fx1 ) * 0.5 );
                                    fy1 = fy1 + ( ( fy2 - fy1 ) * 0.5 );

                                    double fAngle = atan2( -fvx, -fvy );
                                    double fL = hypot( fvx, fvy );
                                    if (fL == 0.0)
                                    {
                                        SAL_WARN("svx", "FitTextOutlinesToShapeOutlines div-by-zero, abandon fit");
                                        break;
                                    }
                                    fvx = fvx / fL;
                                    fvy = fvy / fL;
                                    // Undo Vertical alignment hacked into poly coords
                                    // We already calculated the right alignment into the curve
                                    fL = rTextArea.nHAlignMove;
                                    fvx *= fL;
                                    fvy *= fL;
                                    rPolyPoly.Rotate( Point( aBoundRect.Center().X(), rParagraph.aBoundRect.Center().Y() ), sin( fAngle ), cos( fAngle ) );
                                    rPolyPoly.Move( static_cast<sal_Int32>( ( fx1 + fvx )- aBoundRect.Center().X() ), static_cast<sal_Int32>( ( fy1 + fvy ) - rParagraph.aBoundRect.Center().Y() ) );
                                }
                            }
                        }
                    }
                    else
                    {
                        // Fallback / old way to handle multiple lines:
                        // Every text lines use the same original outline (curve),
                        // it just scale character coordinates to fit to the right text line
                        // (curve), resulting wider/thinner space between characters
                        for (auto& rParagraph : rTextArea.vParagraphs)
                        {
                            for (auto& rCharacter : rParagraph.vCharacters)
                            {
                                for (tools::PolyPolygon& rPolyPoly : rCharacter.vOutlines)
                                {
                                    tools::Rectangle aBoundRect(rPolyPoly.GetBoundRect());
                                    double fx1 = aBoundRect.Left() - nLeft;
                                    double fx2 = aBoundRect.Right() - nLeft;
                                    double fy1, fy2;
                                    double fM1 = fx1 / static_cast<double>(nWidth);
                                    double fM2 = fx2 / static_cast<double>(nWidth);

                                    GetPoint(rOutlinePoly, vDistances, fM1, fx1, fy1);
                                    GetPoint(rOutlinePoly, vDistances, fM2, fx2, fy2);

                                    double fvx = fy2 - fy1;
                                    double fvy = -(fx2 - fx1);
                                    fx1 = fx1 + ((fx2 - fx1) * 0.5);
                                    fy1 = fy1 + ((fy2 - fy1) * 0.5);

                                    double fAngle = atan2(-fvx, -fvy);
                                    double fL = hypot(fvx, fvy);
                                    if (fL == 0.0)
                                    {
                                        SAL_WARN("svx", "FitTextOutlinesToShapeOutlines div-by-zero, abandon fit");
                                        break;
                                    }
                                    fvx = fvx / fL;
                                    fvy = fvy / fL;
                                    fL = rTextArea.aBoundRect.GetHeight() / 2.0 + rTextArea.aBoundRect.Top() - rParagraph.aBoundRect.Center().Y();
                                    fvx *= fL;
                                    fvy *= fL;
                                    rPolyPoly.Rotate( Point( aBoundRect.Center().X(), rParagraph.aBoundRect.Center().Y() ), sin( fAngle ), cos( fAngle ) );
                                    rPolyPoly.Move( static_cast<sal_Int32>( ( fx1 + fvx )- aBoundRect.Center().X() ), static_cast<sal_Int32>( ( fy1 + fvy ) - rParagraph.aBoundRect.Center().Y() ) );
                                }
                            }
                        }
                    }

                }
            }
        }
        else
        {
            if ( ( nOutline2dIdx + 1 ) >= aOutlines2d.Count() )
                break;
            const tools::Polygon& rOutlinePoly( aOutlines2d[ nOutline2dIdx++ ] );
            const tools::Polygon& rOutlinePoly2( aOutlines2d[ nOutline2dIdx++ ] );
            const sal_uInt16 nPointCount = rOutlinePoly.GetSize();
            const sal_uInt16 nPointCount2 = rOutlinePoly2.GetSize();
            if ( ( nPointCount > 1 ) && ( nPointCount2 > 1 ) )
            {
                std::vector< double > vDistances;
                vDistances.reserve( nPointCount );
                std::vector< double > vDistances2;
                vDistances2.reserve( nPointCount2 );
                CalcDistances( rOutlinePoly, vDistances );
                CalcDistances( rOutlinePoly2, vDistances2 );
                for( auto& rParagraph : rTextArea.vParagraphs )
                {
                    for ( auto& rCharacter : rParagraph.vCharacters )
                    {
                        for( tools::PolyPolygon& rPolyPoly : rCharacter.vOutlines )
                        {
                            sal_uInt16 i, nPolyCount = rPolyPoly.Count();
                            for ( i = 0; i < nPolyCount; i++ )
                            {
                                // #i35928#
                                basegfx::B2DPolygon aCandidate(rPolyPoly[ i ].getB2DPolygon());

                                if(aCandidate.areControlPointsUsed())
                                {
                                    aCandidate = basegfx::utils::adaptiveSubdivideByAngle(aCandidate);
                                }

                                // create local polygon copy to work on
                                tools::Polygon aLocalPoly(aCandidate);

                                InsertMissingOutlinePoints( vDistances, rTextAreaBoundRect, aLocalPoly );
                                InsertMissingOutlinePoints( vDistances2, rTextAreaBoundRect, aLocalPoly );

                                sal_uInt16 _nPointCount = aLocalPoly.GetSize();
                                if (_nPointCount)
                                {
                                    if (!nWidth || !nHeight)
                                        throw o3tl::divide_by_zero();
                                    for (sal_uInt16 j = 0; j < _nPointCount; ++j)
                                    {
                                        Point& rPoint = aLocalPoly[ j ];
                                        rPoint.AdjustX( -nLeft );
                                        rPoint.AdjustY( -nTop );
                                        double fX = static_cast<double>(rPoint.X()) / static_cast<double>(nWidth);
                                        double fY = static_cast<double>(rPoint.Y()) / static_cast<double>(nHeight);

                                        double fx1, fy1, fx2, fy2;
                                        GetPoint( rOutlinePoly, vDistances, fX, fx1, fy1 );
                                        GetPoint( rOutlinePoly2, vDistances2, fX, fx2, fy2 );
                                        double fWidth = fx2 - fx1;
                                        double fHeight= fy2 - fy1;
                                        rPoint.setX( static_cast<sal_Int32>( fx1 + fWidth * fY ) );
                                        rPoint.setY( static_cast<sal_Int32>( fy1 + fHeight* fY ) );
                                    }
                                }

                                // write back polygon
                                rPolyPoly[i] = std::move(aLocalPoly);
                            }
                        }
                    }
                }
            }
        }
    }
}

static rtl::Reference<SdrObject> CreateSdrObjectFromParagraphOutlines(
    const FWData& rFWData,
    const SdrObjCustomShape& rSdrObjCustomShape)
{
    rtl::Reference<SdrObject> pRet;
    basegfx::B2DPolyPolygon aPolyPoly;
    if ( !rFWData.vTextAreas.empty() )
    {
        for ( const auto& rTextArea : rFWData.vTextAreas )
        {
            for ( const auto& rParagraph : rTextArea.vParagraphs )
            {
                for ( const auto& rCharacter : rParagraph.vCharacters )
                {
                    for( const auto& rOutline : rCharacter.vOutlines )
                    {
                        aPolyPoly.append( rOutline.getB2DPolyPolygon() );
                    }
                }
            }
        }

        pRet = new SdrPathObj(
            rSdrObjCustomShape.getSdrModelFromSdrObject(),
            SdrObjKind::Polygon,
            std::move(aPolyPoly));

        SfxItemSet aSet(rSdrObjCustomShape.GetMergedItemSet());
        aSet.ClearItem( SDRATTR_TEXTDIRECTION );    //SJ: vertical writing is not required, by removing this item no outliner is created
        aSet.Put(makeSdrShadowItem(false)); // #i37011# NO shadow for FontWork geometry
        pRet->SetMergedItemSet( aSet );             // * otherwise we would crash, because the outliner tries to create a Paraobject, but there is no model
    }

    return pRet;
}

Reference < i18n::XBreakIterator > EnhancedCustomShapeFontWork::mxBreakIterator;

Reference < i18n::XBreakIterator > const & EnhancedCustomShapeFontWork::GetBreakIterator()
{
    if ( !mxBreakIterator.is() )
    {
        const Reference< uno::XComponentContext >& xContext = ::comphelper::getProcessComponentContext();
        mxBreakIterator = i18n::BreakIterator::create(xContext);
    }
    return mxBreakIterator;
}

rtl::Reference<SdrObject> EnhancedCustomShapeFontWork::CreateFontWork(
    const SdrObject* pShape2d,
    const SdrObjCustomShape& rSdrObjCustomShape)
{
    rtl::Reference<SdrObject> pRet;

    // calculating scaling factor is too slow
    if (comphelper::IsFuzzing())
        return pRet;

    tools::PolyPolygon aOutlines2d( GetOutlinesFromShape2d( pShape2d ) );
    sal_uInt16 nOutlinesCount2d = aOutlines2d.Count();
    if ( nOutlinesCount2d )
    {
        FWData aFWData;

        if(InitializeFontWorkData(rSdrObjCustomShape, nOutlinesCount2d, aFWData))
        {
            /* retrieves the horizontal scaling factor that has to be used
            to fit each paragraph text into its corresponding 2d outline */
            CalculateHorizontalScalingFactor(
                rSdrObjCustomShape,
                aFWData,
                aOutlines2d);

            /* retrieving the Outlines for the each Paragraph. */
            if(!GetFontWorkOutline(
                aFWData,
                rSdrObjCustomShape))
            {
                return nullptr;
            }

            SdrTextHorzAdjust eHorzAdjust(
                rSdrObjCustomShape.GetMergedItem(SDRATTR_TEXT_HORZADJUST).GetValue());
            bool bPPFontwork = !rSdrObjCustomShape.getSdrModelFromSdrObject().GetCompatibilityFlag(
                              SdrCompatibilityFlag::LegacyFontwork);
            FitTextOutlinesToShapeOutlines( aOutlines2d, aFWData, eHorzAdjust, bPPFontwork );

            pRet = CreateSdrObjectFromParagraphOutlines(
                aFWData,
                rSdrObjCustomShape);
        }
    }
    return pRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
