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

#include "textmarkuphelper.hxx"
#include "accportions.hxx"

#include <vector>

#include <com/sun/star/text/TextMarkupType.hpp>
#include <com/sun/star/accessibility/TextSegment.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>

#include <comphelper/sequence.hxx>
#include <osl/diagnose.h>
#include <ndtxt.hxx>
#include <wrong.hxx>

using namespace com::sun::star;

// helper functions
namespace {
    /// @throws css::lang::IllegalArgumentException
    /// @throws css::uno::RuntimeException
    SwWrongList const* (SwTextNode::*
        getTextMarkupFunc(const sal_Int32 nTextMarkupType))() const
    {
        switch ( nTextMarkupType )
        {
            case text::TextMarkupType::SPELLCHECK:
            {
                return &SwTextNode::GetWrong;
            }
            break;
            case text::TextMarkupType::PROOFREADING:
            {
                // support not implemented yet
                return nullptr;
            }
            break;
            case text::TextMarkupType::SMARTTAG:
            {
                // support not implemented yet
                return nullptr;
            }
            break;
            default:
            {
                throw lang::IllegalArgumentException();
            }
        }
    }
}

// implementation of class <SwTextMarkupoHelper>
SwTextMarkupHelper::SwTextMarkupHelper( const SwAccessiblePortionData& rPortionData,
                                        const SwTextFrame& rTextFrame)
    : mrPortionData( rPortionData )
    , m_pTextFrame(&rTextFrame)
    , mpTextMarkupList( nullptr )
{
}

// #i108125#
SwTextMarkupHelper::SwTextMarkupHelper( const SwAccessiblePortionData& rPortionData,
                                        const SwWrongList& rTextMarkupList )
    : mrPortionData( rPortionData )
    , m_pTextFrame( nullptr )
    , mpTextMarkupList( &rTextMarkupList )
{
}


std::unique_ptr<sw::WrongListIteratorCounter> SwTextMarkupHelper::getIterator(sal_Int32 nTextMarkupType)
{
    std::unique_ptr<sw::WrongListIteratorCounter> pIter;
    if (mpTextMarkupList)
    {
        pIter.reset(new sw::WrongListIteratorCounter(*mpTextMarkupList));
    }
    else
    {
        assert(m_pTextFrame);
        SwWrongList const* (SwTextNode::*const pGetWrongList)() const = getTextMarkupFunc(nTextMarkupType);
        if (pGetWrongList)
        {
            pIter.reset(new sw::WrongListIteratorCounter(*m_pTextFrame, pGetWrongList));
        }
    }

    return pIter;
}


sal_Int32 SwTextMarkupHelper::getTextMarkupCount( const sal_Int32 nTextMarkupType )
{
    sal_Int32 nTextMarkupCount( 0 );

    std::unique_ptr<sw::WrongListIteratorCounter> pIter = getIterator(nTextMarkupType);
    // iterator may handle all items in the underlying text node in the model, which may be more
    // than what is in the portion data (e.g. if a paragraph is split across multiple pages),
    // only take into account those that are in the portion data
    for (sal_uInt16 i = 0; i < pIter->GetElementCount(); i++)
    {
        std::optional<std::pair<TextFrameIndex, TextFrameIndex>> oIndices = pIter->GetElementAt(i);
        if (oIndices && mrPortionData.IsValidCorePosition(oIndices->first) && mrPortionData.IsValidCorePosition(oIndices->second))
            nTextMarkupCount++;
    }

    return nTextMarkupCount;
}

css::accessibility::TextSegment
        SwTextMarkupHelper::getTextMarkup( const sal_Int32 nTextMarkupIndex,
                                           const sal_Int32 nTextMarkupType )
{
    if ( nTextMarkupIndex >= getTextMarkupCount( nTextMarkupType ) ||
         nTextMarkupIndex < 0 )
    {
        throw lang::IndexOutOfBoundsException();
    }

    css::accessibility::TextSegment aTextMarkupSegment;
    aTextMarkupSegment.SegmentStart = -1;
    aTextMarkupSegment.SegmentEnd = -1;

    std::unique_ptr<sw::WrongListIteratorCounter> pIter = getIterator(nTextMarkupType);
    if (pIter)
    {
        std::optional<std::pair<TextFrameIndex, TextFrameIndex>> oElement;
        const sal_uInt16 nIterElementCount = pIter->GetElementCount();
        sal_Int32 nIndexInPortion = 0;
        sal_uInt16 nIterIndex = 0;
        while (!oElement && nIterIndex < nIterElementCount)
        {
            // iterator may handle all items in the underlying text node in the model, which may be more
            // than what is in the portion data (e.g. if a paragraph is split across multiple pages),
            // only take into account those that are in the portion data
            std::optional<std::pair<TextFrameIndex, TextFrameIndex>> oIndices = pIter->GetElementAt(nIterIndex);
            if (oIndices && mrPortionData.IsValidCorePosition(oIndices->first) && mrPortionData.IsValidCorePosition(oIndices->second))
            {
                if (nIndexInPortion == nTextMarkupIndex)
                    oElement = std::move(oIndices);

                nIndexInPortion++;
            }

            nIterIndex++;
        }

        if (oElement)
        {
            const OUString& rText = mrPortionData.GetAccessibleString();
            const sal_Int32 nStartPos =
                            mrPortionData.GetAccessiblePosition(oElement->first);
            const sal_Int32 nEndPos =
                            mrPortionData.GetAccessiblePosition(oElement->second);
            aTextMarkupSegment.SegmentText = rText.copy( nStartPos, nEndPos - nStartPos );
            aTextMarkupSegment.SegmentStart = nStartPos;
            aTextMarkupSegment.SegmentEnd = nEndPos;
        }
        else
        {
            OSL_FAIL( "<SwTextMarkupHelper::getTextMarkup(..)> - missing <SwWrongArea> instance" );
        }
    }

    return aTextMarkupSegment;
}

css::uno::Sequence< css::accessibility::TextSegment >
        SwTextMarkupHelper::getTextMarkupAtIndex( const sal_Int32 nCharIndex,
                                                  const sal_Int32 nTextMarkupType )
{
    // assumption:
    // value of <nCharIndex> is in range [0..length of accessible text)

    const TextFrameIndex nCoreCharIndex = mrPortionData.GetCoreViewPosition(nCharIndex);
    // Handling of portions with core length == 0 at the beginning of the
    // paragraph - e.g. numbering portion.
    if ( mrPortionData.GetAccessiblePosition( nCoreCharIndex ) > nCharIndex )
    {
        return uno::Sequence< css::accessibility::TextSegment >();
    }

    std::unique_ptr<sw::WrongListIteratorCounter> pIter = getIterator(nTextMarkupType);
    std::vector< css::accessibility::TextSegment > aTmpTextMarkups;
    if (pIter)
    {
        const OUString& rText = mrPortionData.GetAccessibleString();
        sal_uInt16 count(pIter->GetElementCount());
        for (sal_uInt16 i = 0; i < count; ++i)
        {
            auto const oElement(pIter->GetElementAt(i));
            if (oElement &&
                oElement->first <= nCoreCharIndex &&
                nCoreCharIndex < oElement->second)
            {
                const sal_Int32 nStartPos =
                    mrPortionData.GetAccessiblePosition(oElement->first);
                const sal_Int32 nEndPos =
                    mrPortionData.GetAccessiblePosition(oElement->second);
                css::accessibility::TextSegment aTextMarkupSegment;
                aTextMarkupSegment.SegmentText = rText.copy( nStartPos, nEndPos - nStartPos );
                aTextMarkupSegment.SegmentStart = nStartPos;
                aTextMarkupSegment.SegmentEnd = nEndPos;
                aTmpTextMarkups.push_back( aTextMarkupSegment );
            }
        }
    }

    return comphelper::containerToSequence(aTmpTextMarkups  );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
