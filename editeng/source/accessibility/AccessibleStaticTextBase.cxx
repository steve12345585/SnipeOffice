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


// Global header


#include <utility>
#include <memory>
#include <vector>
#include <algorithm>
#include <rtl/ustrbuf.hxx>
#include <tools/debug.hxx>
#include <vcl/svapp.hxx>
#include <vcl/unohelp.hxx>
#include <comphelper/sequence.hxx>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/awt/Point.hpp>
#include <com/sun/star/awt/Rectangle.hpp>
#include <com/sun/star/accessibility/AccessibleTextType.hpp>


// Project-local header


#include <editeng/editdata.hxx>
#include <editeng/AccessibleStaticTextBase.hxx>


using namespace ::com::sun::star;
using namespace ::com::sun::star::accessibility;

/* TODO:
   =====

   - separate adapter functionality from AccessibleStaticText class

   - refactor common loops into templates, using mem_fun

 */

namespace accessibility
{

typedef std::vector< beans::PropertyValue > PropertyValueVector;

namespace {

class PropertyValueEqualFunctor
{
    const beans::PropertyValue& m_rPValue;

public:
    explicit PropertyValueEqualFunctor(const beans::PropertyValue& rPValue)
        : m_rPValue(rPValue)
    {}
    bool operator() ( const beans::PropertyValue& rhs ) const
    {
        return ( m_rPValue.Name == rhs.Name && m_rPValue.Value == rhs.Value );
    }
};

}

sal_Unicode const cNewLine(0x0a);


// Static Helper


static ESelection MakeSelection( sal_Int32 nStartPara, sal_Int32 nStartIndex,
                          sal_Int32 nEndPara, sal_Int32 nEndIndex )
{
    DBG_ASSERT(nStartPara >= 0 &&
               nStartIndex >= 0 &&
               nEndPara >= 0 &&
               nEndIndex >= 0,
               "AccessibleStaticTextBase_Impl::MakeSelection: index value overflow");

    return ESelection(nStartPara, nStartIndex, nEndPara, nEndIndex);
}


AccessibleEditableTextPara& AccessibleStaticTextBase::GetParagraph( sal_Int32 nPara ) const
{

    if( !mxTextParagraph.is() )
        throw lang::DisposedException (u"object has been already disposed"_ustr);

    // TODO: Have a different method on AccessibleEditableTextPara
    // that does not care about state changes
    mxTextParagraph->SetParagraphIndex( nPara );

    return *mxTextParagraph;
}

sal_Int32 AccessibleStaticTextBase::GetParagraphCount() const
{

    if( !mxTextParagraph.is() )
        return 0;
    else
        return mxTextParagraph->GetTextForwarder().GetParagraphCount();
}

sal_Int32 AccessibleStaticTextBase::Internal2Index(EPaM nEEIndex) const
{
    // XXX checks for overflow and returns maximum if so
    sal_Int32 aRes(0);
    for(sal_Int32 i=0; i<nEEIndex.nPara; ++i)
    {
        sal_Int32 nCount = GetParagraph(i).getCharacterCount();
        if (SAL_MAX_INT32 - aRes > nCount)
            return SAL_MAX_INT32;
        aRes += nCount;
    }

    if (SAL_MAX_INT32 - aRes > nEEIndex.nIndex)
        return SAL_MAX_INT32;
    return aRes + nEEIndex.nIndex;
}

void AccessibleStaticTextBase::CorrectTextSegment(TextSegment& aTextSegment,
                                                  int nPara) const
{
    // Keep 'invalid' values at the TextSegment
    if( aTextSegment.SegmentStart != -1 &&
        aTextSegment.SegmentEnd != -1 )
    {
        // #112814# Correct TextSegment by paragraph offset
        sal_Int32 nOffset(0);
        int i;
        for(i=0; i<nPara; ++i)
            nOffset += GetParagraph(i).getCharacterCount();

        aTextSegment.SegmentStart += nOffset;
        aTextSegment.SegmentEnd += nOffset;
    }
}

EPaM AccessibleStaticTextBase::ImpCalcInternal(sal_Int32 nFlatIndex, bool bExclusive) const
{

    if( nFlatIndex < 0 )
        throw lang::IndexOutOfBoundsException(u"AccessibleStaticTextBase_Impl::Index2Internal: character index out of bounds"_ustr);
    // gratuitously accepting larger indices here, AccessibleEditableTextPara will throw eventually

    sal_Int32 nCurrPara, nCurrIndex, nParas, nCurrCount;
    for( nCurrPara=0, nParas=GetParagraphCount(), nCurrCount=0, nCurrIndex=0; nCurrPara<nParas; ++nCurrPara )
    {
        nCurrCount = GetParagraph( nCurrPara ).getCharacterCount();
        nCurrIndex += nCurrCount;
        if( nCurrIndex >= nFlatIndex )
        {
            // check overflow
            DBG_ASSERT(nCurrPara >= 0 &&
                       nFlatIndex - nCurrIndex + nCurrCount >= 0,
                       "AccessibleStaticTextBase::Index2Internal: index value overflow");

            return EPaM(nCurrPara, nFlatIndex - nCurrIndex + nCurrCount);
        }
    }

    // #102170# Allow one-past the end for ranges
    if( bExclusive && nCurrIndex == nFlatIndex )
    {
        // check overflow
        DBG_ASSERT(nCurrPara > 0 &&
                   nFlatIndex - nCurrIndex + nCurrCount >= 0,
                   "AccessibleStaticTextBase::Index2Internal: index value overflow");

        return EPaM(nCurrPara - 1, nFlatIndex - nCurrIndex + nCurrCount);
    }

    // not found? Out of bounds
    throw lang::IndexOutOfBoundsException(u"AccessibleStaticTextBase::Index2Internal: character index out of bounds"_ustr);
}

bool AccessibleStaticTextBase::SetSelection( sal_Int32 nStartPara, sal_Int32 nStartIndex,
                                                      sal_Int32 nEndPara, sal_Int32 nEndIndex )
{

    if( !mxTextParagraph.is() )
        return false;

    try
    {
        SvxEditViewForwarder& rCacheVF = mxTextParagraph->GetEditViewForwarder( true );
        return rCacheVF.SetSelection( MakeSelection(nStartPara, nStartIndex, nEndPara, nEndIndex) );
    }
    catch( const uno::RuntimeException& )
    {
        return false;
    }
}

bool AccessibleStaticTextBase::CopyText( sal_Int32 nStartPara, sal_Int32 nStartIndex,
                                                  sal_Int32 nEndPara, sal_Int32 nEndIndex )
{

    if( !mxTextParagraph.is() )
        return false;

    try
    {
        SvxEditViewForwarder& rCacheVF = mxTextParagraph->GetEditViewForwarder( true );
        mxTextParagraph->GetTextForwarder();    // MUST be after GetEditViewForwarder(), see method docs
        bool aRetVal;

        // save current selection
        ESelection aOldSelection;

        rCacheVF.GetSelection( aOldSelection );
        rCacheVF.SetSelection( MakeSelection(nStartPara, nStartIndex, nEndPara, nEndIndex) );
        aRetVal = rCacheVF.Copy();
        rCacheVF.SetSelection( aOldSelection ); // restore

        return aRetVal;
    }
    catch( const uno::RuntimeException& )
    {
        return false;
    }
}

//the input argument is the index(including "\n" ) in the string.
//the function will calculate the actual index(not including "\n") in the string.
//and return true if the index is just at a "\n"
bool AccessibleStaticTextBase::RemoveLineBreakCount( sal_Int32& rIndex )
{
    // get the total char number inside the cell.
    sal_Int32 i, nCount, nParas;
    for( i=0, nCount=0, nParas=GetParagraphCount(); i<nParas; ++i )
        nCount += GetParagraph(i).getCharacterCount();
    nCount = nCount + (nParas-1);
    if( nCount == 0 &&  rIndex == 0) return false;


    sal_Int32 nCurrPara, nCurrCount;
    sal_Int32 nLineBreakPos = 0, nLineBreakCount = 0;
    sal_Int32 nParaCount = GetParagraphCount();
    for ( nCurrCount = 0, nCurrPara = 0; nCurrPara < nParaCount; nCurrPara++ )
    {
        nCurrCount += GetParagraph( nCurrPara ).getCharacterCount();
        nLineBreakPos = nCurrCount++;
        if ( rIndex == nLineBreakPos )
        {
            rIndex -= (++nLineBreakCount);//(++nLineBreakCount);
            if ( rIndex < 0)
            {
                rIndex = 0;
            }
            //if the index is at the last position of the last paragraph
            //there is no "\n" , so we should increase rIndex by 1 and return false.
            if ( (nCurrPara+1) == nParaCount )
            {
                rIndex++;
                return false;
            }
            else
            {
                return true;
            }
        }
        else if ( rIndex < nLineBreakPos )
        {
            rIndex -= nLineBreakCount;
            return false;
        }
        else
        {
            nLineBreakCount++;
        }
    }
    return false;
}

AccessibleStaticTextBase::AccessibleStaticTextBase(std::unique_ptr<SvxEditSource>&& pEditSource)
    // TODO: this is still somewhat of a hack, all the more since
    // now the maTextParagraph has an empty parent reference set
    : mxTextParagraph(new AccessibleEditableTextPara(nullptr))
{
    SolarMutexGuard aGuard;

    SetEditSource( std::move(pEditSource) );
}

AccessibleStaticTextBase::~AccessibleStaticTextBase()
{
}

void AccessibleStaticTextBase::SetEditSource( std::unique_ptr< SvxEditSource > && pEditSource )
{
    // precondition: solar mutex locked
    DBG_TESTSOLARMUTEX();

    maEditSource.SetEditSource(std::move(pEditSource));
    if (mxTextParagraph.is())
        mxTextParagraph->SetEditSource(&maEditSource);
}

void AccessibleStaticTextBase::SetOffset( const Point& rPoint )
{
    // precondition: solar mutex locked
    DBG_TESTSOLARMUTEX();

    if (mxTextParagraph.is())
        mxTextParagraph->SetEEOffset(rPoint);
}

void AccessibleStaticTextBase::Dispose()
{
    // we're the owner of the paragraph, so destroy it, too
    if (mxTextParagraph.is())
        mxTextParagraph->dispose();

    // drop reference
    mxTextParagraph.clear();
}

// XAccessibleContext
sal_Int64 AccessibleStaticTextBase::getAccessibleChildCount()
{
    // no children at all
    return 0;
}

uno::Reference< XAccessible > AccessibleStaticTextBase::getAccessibleChild( sal_Int64 /*i*/ )
{
    // no children at all
    return uno::Reference< XAccessible >();
}

uno::Reference< XAccessible > AccessibleStaticTextBase::getAccessibleAtPoint( const awt::Point& /*_aPoint*/ )
{
    // no children at all
    return uno::Reference< XAccessible >();
}

// XAccessibleText
sal_Int32 SAL_CALL AccessibleStaticTextBase::getCaretPosition()
{
    SolarMutexGuard aGuard;

    sal_Int32 i, nPos, nParas;
    for (i = 0, nPos = -1, nParas = GetParagraphCount(); i<nParas; ++i )
    {
        if ((nPos = GetParagraph(i).getCaretPosition()) != -1)
            return nPos;
    }

    return nPos;
}

sal_Bool SAL_CALL AccessibleStaticTextBase::setCaretPosition( sal_Int32 nIndex )
{
    return setSelection(nIndex, nIndex);
}

sal_Unicode SAL_CALL AccessibleStaticTextBase::getCharacter( sal_Int32 nIndex )
{
    SolarMutexGuard aGuard;

    EPaM aPos(Index2Internal(nIndex));

    return GetParagraph(aPos.nPara).getCharacter(aPos.nIndex);
}

uno::Sequence< beans::PropertyValue > SAL_CALL AccessibleStaticTextBase::getCharacterAttributes( sal_Int32 nIndex, const css::uno::Sequence< OUString >& aRequestedAttributes )
{
    SolarMutexGuard aGuard;

    //get the actual index without "\n"
    RemoveLineBreakCount(nIndex);

    EPaM aPos(Index2Internal(nIndex));

    return GetParagraph( aPos.nPara ).getCharacterAttributes( aPos.nIndex, aRequestedAttributes );
}

awt::Rectangle SAL_CALL AccessibleStaticTextBase::getCharacterBounds( sal_Int32 nIndex )
{
    SolarMutexGuard aGuard;

    // #108900# Allow ranges for nIndex, as one-past-the-end
    // values are now legal, too.
    EPaM aPos(Range2Internal(nIndex));

    // #i70916# Text in spread sheet cells return the wrong extents
    AccessibleEditableTextPara& rPara = GetParagraph( aPos.nPara );
    awt::Rectangle aParaBounds( rPara.getBounds() );
    awt::Rectangle aBounds( rPara.getCharacterBounds( aPos.nIndex ) );
    aBounds.X += aParaBounds.X;
    aBounds.Y += aParaBounds.Y;

    return aBounds;
}

sal_Int32 SAL_CALL AccessibleStaticTextBase::getCharacterCount()
{
    SolarMutexGuard aGuard;

    sal_Int32 i, nCount, nParas;
    for (i = 0, nCount = 0, nParas = GetParagraphCount(); i < nParas; ++i)
        nCount += GetParagraph(i).getCharacterCount();
    //count on the number of "\n" which equals number of paragraphs decrease 1.
    nCount = nCount + (nParas-1);
    return nCount;
}

sal_Int32 SAL_CALL AccessibleStaticTextBase::getIndexAtPoint( const awt::Point& rPoint )
{
    SolarMutexGuard aGuard;

    const sal_Int32 nParas(GetParagraphCount());
    sal_Int32 nIndex;
    int i;
    for( i=0; i<nParas; ++i )
    {
        // TODO: maybe exploit the fact that paragraphs are
        // ordered vertically for early exit

        // #i70916# Text in spread sheet cells return the wrong extents
        AccessibleEditableTextPara& rPara = GetParagraph(i);
        awt::Rectangle aParaBounds( rPara.getBounds() );
        awt::Point aPoint( rPoint );
        aPoint.X -= aParaBounds.X;
        aPoint.Y -= aParaBounds.Y;

        // #112814# Use correct index offset
        if ( ( nIndex = rPara.getIndexAtPoint( aPoint ) ) != -1 )
            return Internal2Index(EPaM(i, nIndex));
    }

    return -1;
}

OUString SAL_CALL AccessibleStaticTextBase::getSelectedText()
{
    SolarMutexGuard aGuard;

    sal_Int32 nStart( getSelectionStart() );
    sal_Int32 nEnd( getSelectionEnd() );

    // #104481# Return the empty string for 'no selection'
    if( nStart < 0 || nEnd < 0 )
        return OUString();

    return getTextRange( nStart, nEnd );
}

sal_Int32 SAL_CALL AccessibleStaticTextBase::getSelectionStart()
{
    SolarMutexGuard aGuard;

    sal_Int32 i, nPos, nParas;
    for (i = 0, nPos = -1, nParas = GetParagraphCount(); i < nParas; ++i)
    {
        if ((nPos=GetParagraph(i).getSelectionStart()) != -1)
            return nPos;
    }

    return nPos;
}

sal_Int32 SAL_CALL AccessibleStaticTextBase::getSelectionEnd()
{
    SolarMutexGuard aGuard;

    sal_Int32 i, nPos, nParas;
    for (i = 0, nPos = -1, nParas = GetParagraphCount(); i < nParas; ++i)
    {
        if ((nPos = GetParagraph(i).getSelectionEnd()) != -1)
            return nPos;
    }

    return nPos;
}

sal_Bool SAL_CALL AccessibleStaticTextBase::setSelection( sal_Int32 nStartIndex, sal_Int32 nEndIndex )
{
    SolarMutexGuard aGuard;

    EPaM aStartIndex(Range2Internal(nStartIndex));
    EPaM aEndIndex(Range2Internal(nEndIndex));

    return SetSelection(aStartIndex.nPara, aStartIndex.nIndex,
                        aEndIndex.nPara, aEndIndex.nIndex);
}

OUString SAL_CALL AccessibleStaticTextBase::getText()
{
    SolarMutexGuard aGuard;

    sal_Int32 i, nParas;
    OUStringBuffer aRes;
    for (i = 0, nParas = GetParagraphCount(); i < nParas; ++i)
        aRes.append(GetParagraph(i).getText());

    return aRes.makeStringAndClear();
}

OUString SAL_CALL AccessibleStaticTextBase::getTextRange( sal_Int32 nStartIndex, sal_Int32 nEndIndex )
{
    SolarMutexGuard aGuard;

    if( nStartIndex > nEndIndex )
        std::swap(nStartIndex, nEndIndex);
    //if startindex equals endindex we will get nothing. So return an empty string directly.
    if ( nStartIndex == nEndIndex )
    {
        return OUString();
    }
    bool bStart = RemoveLineBreakCount(nStartIndex);
    //if the start index is just at a "\n", we need to begin from the next char
    if ( bStart )
    {
        nStartIndex++;
    }
    //we need to find out whether the previous position of the current endindex is at "\n" or not
    //if yes we need to mark it and add "\n" at the end of the result
    sal_Int32 nTemp = nEndIndex - 1;
    bool bEnd = RemoveLineBreakCount(nTemp);
    bool bTemp = RemoveLineBreakCount(nEndIndex);
    //if the below condition is true it indicates an empty paragraph with just a "\n"
    //so we need to set one "\n" flag to avoid duplication.
    if ( bStart && bEnd && ( nStartIndex == nEndIndex) )
    {
        bEnd = false;
    }
    //if the current endindex is at a "\n", we need to increase endindex by 1 to make sure
    //the char before "\n" is included. Because string returned by this function will not include
    //the char at the endindex.
    if ( bTemp )
    {
        nEndIndex++;
    }
    OUStringBuffer aRes;
    EPaM aStartIndex(Range2Internal(nStartIndex));
    EPaM aEndIndex(Range2Internal(nEndIndex));

    // #102170# Special case: start and end paragraph are identical
    if( aStartIndex.nPara == aEndIndex.nPara )
    {
        //we don't return the string directly now for that we have to do some further process for "\n"
        aRes = GetParagraph( aStartIndex.nPara ).getTextRange( aStartIndex.nIndex, aEndIndex.nIndex );
    }
    else
    {
        sal_Int32 i( aStartIndex.nPara );
        aRes = GetParagraph(i).getTextRange(aStartIndex.nIndex,
                                            GetParagraph(i).getCharacterCount()/*-1*/);
        ++i;

        // paragraphs inbetween are fully included
        for( ; i<aEndIndex.nPara; ++i )
        {
            aRes.append(OUStringChar(cNewLine) + GetParagraph(i).getText());
        }

        if( i<=aEndIndex.nPara )
        {
            //if the below condition is matched it means that endindex is at mid of the last paragraph
            //we need to add a "\n" before we add the last part of the string.
            if ( !bEnd && aEndIndex.nIndex )
            {
                aRes.append(cNewLine);
            }
            aRes.append(GetParagraph(i).getTextRange( 0, aEndIndex.nIndex ));
        }
    }
    //According to the flag we marked before, we have to add "\n" at the beginning
    //or at the end of the result string.
    if ( bStart )
    {
        aRes.insert(0, OUStringChar(cNewLine));
    }
    if ( bEnd )
    {
        aRes.append(OUStringChar(cNewLine));
    }
    return aRes.makeStringAndClear();
}

css::accessibility::TextSegment SAL_CALL AccessibleStaticTextBase::getTextAtIndex( sal_Int32 nIndex, sal_Int16 aTextType )
{
    SolarMutexGuard aGuard;

    bool bLineBreak = RemoveLineBreakCount(nIndex);
    EPaM aPos(Range2Internal(nIndex));

    css::accessibility::TextSegment aResult;

    if( AccessibleTextType::PARAGRAPH == aTextType )
    {
        // #106393# Special casing one behind last paragraph is
        // not necessary, since then, we return the content and
        // boundary of that last paragraph. Range2Internal is
        // tolerant against that, and returns the last paragraph
        // in aPos.nPara.

        // retrieve full text of the paragraph
        aResult.SegmentText = GetParagraph( aPos.nPara ).getText();

        // #112814# Adapt the start index with the paragraph offset
        aResult.SegmentStart = Internal2Index(EPaM(aPos.nPara, 0));
        aResult.SegmentEnd = aResult.SegmentStart + aResult.SegmentText.getLength();
    }
    else if ( AccessibleTextType::ATTRIBUTE_RUN == aTextType )
    {
        SvxAccessibleTextAdapter& rTextForwarder = GetParagraph( aPos.nIndex ).GetTextForwarder();
        sal_Int32 nStartIndex, nEndIndex;
        if ( rTextForwarder.GetAttributeRun( nStartIndex, nEndIndex, aPos.nPara, aPos.nIndex, true ) )
        {
            aResult.SegmentText = getTextRange( nStartIndex, nEndIndex );
            aResult.SegmentStart = nStartIndex;
            aResult.SegmentEnd = nEndIndex;
        }
    }
    else
    {
        // No special handling required, forward to wrapped class
        aResult = GetParagraph( aPos.nPara ).getTextAtIndex( aPos.nIndex, aTextType );

        // #112814# Adapt the start index with the paragraph offset
        CorrectTextSegment( aResult, aPos.nPara );
        if ( bLineBreak )
        {
            aResult.SegmentText = OUString(cNewLine);
        }
    }

    return aResult;
}

css::accessibility::TextSegment SAL_CALL AccessibleStaticTextBase::getTextBeforeIndex( sal_Int32 nIndex, sal_Int16 aTextType )
{
    SolarMutexGuard aGuard;

    sal_Int32 nOldIdx = nIndex;
    bool bLineBreak =  RemoveLineBreakCount(nIndex);
    EPaM aPos(Range2Internal(nIndex));

    css::accessibility::TextSegment aResult;

    if( AccessibleTextType::PARAGRAPH == aTextType )
    {
        if( aPos.nIndex == GetParagraph( aPos.nPara ).getCharacterCount() )
        {
            // #103589# Special casing one behind the last paragraph
            aResult.SegmentText = GetParagraph( aPos.nPara ).getText();

            // #112814# Adapt the start index with the paragraph offset
            aResult.SegmentStart = Internal2Index(EPaM(aPos.nPara, 0));
        }
        else if( aPos.nPara > 0 )
        {
            aResult.SegmentText = GetParagraph( aPos.nPara - 1 ).getText();

            // #112814# Adapt the start index with the paragraph offset
            aResult.SegmentStart = Internal2Index(EPaM(aPos.nPara - 1, 0));
        }

        aResult.SegmentEnd = aResult.SegmentStart + aResult.SegmentText.getLength();
    }
    else
    {
        // No special handling required, forward to wrapped class
        aResult = GetParagraph( aPos.nPara ).getTextBeforeIndex( aPos.nIndex, aTextType );

        // #112814# Adapt the start index with the paragraph offset
        CorrectTextSegment( aResult, aPos.nPara );
        if ( bLineBreak && (nOldIdx-1) >= 0)
        {
            aResult = getTextAtIndex( nOldIdx-1, aTextType );
        }
    }

    return aResult;
}

css::accessibility::TextSegment SAL_CALL AccessibleStaticTextBase::getTextBehindIndex( sal_Int32 nIndex, sal_Int16 aTextType )
{
    SolarMutexGuard aGuard;

    sal_Int32 nTemp = nIndex+1;
    bool bLineBreak = RemoveLineBreakCount(nTemp);
    RemoveLineBreakCount(nIndex);
    EPaM aPos(Range2Internal(nIndex));

    css::accessibility::TextSegment aResult;

    if( AccessibleTextType::PARAGRAPH == aTextType )
    {
        // Special casing one behind the last paragraph is not
        // necessary, this case is invalid here for
        // getTextBehindIndex
        if (aPos.nPara + 1 < GetParagraphCount())
        {
            aResult.SegmentText = GetParagraph( aPos.nPara + 1 ).getText();

            // #112814# Adapt the start index with the paragraph offset
            aResult.SegmentStart = Internal2Index(EPaM(aPos.nPara + 1, 0));
            aResult.SegmentEnd = aResult.SegmentStart + aResult.SegmentText.getLength();
        }
    }
    else
    {
        // No special handling required, forward to wrapped class
        aResult = GetParagraph( aPos.nPara ).getTextBehindIndex( aPos.nIndex, aTextType );

        // #112814# Adapt the start index with the paragraph offset
        CorrectTextSegment( aResult, aPos.nPara );
        if ( bLineBreak )
        {
            aResult.SegmentText = OUStringChar(cNewLine) + aResult.SegmentText;
        }
   }

    return aResult;
}

sal_Bool SAL_CALL AccessibleStaticTextBase::copyText( sal_Int32 nStartIndex, sal_Int32 nEndIndex )
{
    SolarMutexGuard aGuard;

    if( nStartIndex > nEndIndex )
        std::swap(nStartIndex, nEndIndex);

    EPaM aStartIndex(Range2Internal(nStartIndex));
    EPaM aEndIndex(Range2Internal(nEndIndex));

    return CopyText(aStartIndex.nPara, aStartIndex.nIndex, aEndIndex.nPara, aEndIndex.nIndex);
}

sal_Bool SAL_CALL AccessibleStaticTextBase::scrollSubstringTo( sal_Int32, sal_Int32, AccessibleScrollType )
{
    return false;
}

// XAccessibleTextAttributes
uno::Sequence< beans::PropertyValue > AccessibleStaticTextBase::getDefaultAttributes( const uno::Sequence< OUString >& RequestedAttributes )
{
    // get the intersection of the default attributes of all paragraphs

    SolarMutexGuard aGuard;

    PropertyValueVector aDefAttrVec(
            comphelper::sequenceToContainer<PropertyValueVector>(GetParagraph( 0 ).getDefaultAttributes( RequestedAttributes )) );

    const sal_Int32 nParaCount = GetParagraphCount();
    for ( sal_Int32 nPara = 1; nPara < nParaCount; ++nPara )
    {
        uno::Sequence< beans::PropertyValue > aSeq = GetParagraph( nPara ).getDefaultAttributes( RequestedAttributes );
        PropertyValueVector aIntersectionVec;

        for ( const auto& rDefAttr : aDefAttrVec )
        {
            auto it = std::find_if(aSeq.begin(), aSeq.end(), PropertyValueEqualFunctor(rDefAttr));
            if (it != aSeq.end())
                aIntersectionVec.push_back(*it);
        }

        aDefAttrVec.swap( aIntersectionVec );

        if ( aDefAttrVec.empty() )
        {
            break;
        }
    }

    return comphelper::containerToSequence(aDefAttrVec);
}

uno::Sequence< beans::PropertyValue > SAL_CALL AccessibleStaticTextBase::getRunAttributes( sal_Int32 nIndex, const uno::Sequence< OUString >& RequestedAttributes )
{
    // get those default attributes of the paragraph, which are not part
    // of the intersection of all paragraphs and add them to the run attributes

    SolarMutexGuard aGuard;

    EPaM aPos(Index2Internal(nIndex));
    AccessibleEditableTextPara& rPara = GetParagraph( aPos.nPara );
    uno::Sequence< beans::PropertyValue > aDefAttrSeq = rPara.getDefaultAttributes( RequestedAttributes );
    uno::Sequence< beans::PropertyValue > aRunAttrSeq = rPara.getRunAttributes( aPos.nIndex, RequestedAttributes );
    uno::Sequence< beans::PropertyValue > aIntersectionSeq = getDefaultAttributes( RequestedAttributes );
    PropertyValueVector aDiffVec;

    for (auto& defAttr : aDefAttrSeq)
    {
        bool bNone = std::none_of(aIntersectionSeq.begin(), aIntersectionSeq.end(),
                                  PropertyValueEqualFunctor(defAttr));
        if (bNone && defAttr.Handle != 0)
        {
            aDiffVec.push_back(defAttr);
        }
    }

    return comphelper::concatSequences(aRunAttrSeq, aDiffVec);
}

tools::Rectangle AccessibleStaticTextBase::GetParagraphBoundingBox() const
{
    if (!mxTextParagraph.is())
        return tools::Rectangle();

    awt::Rectangle aAwtRect = mxTextParagraph->getBounds();
    return vcl::unohelper::ConvertToVCLRect(aAwtRect);
}

}  // end of namespace accessibility


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
