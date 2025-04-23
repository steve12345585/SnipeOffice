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

#include <com/sun/star/accessibility/AccessibleRole.hpp>
#include <com/sun/star/accessibility/AccessibleStateType.hpp>
#include <com/sun/star/accessibility/AccessibleTextType.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <unotools/accessiblerelationsethelper.hxx>

#include <com/sun/star/datatransfer/clipboard/XClipboard.hpp>
#include <com/sun/star/datatransfer/clipboard/XFlushableClipboard.hpp>
#include <comphelper/accessibleeventnotifier.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <osl/diagnose.h>
#include <comphelper/diagnose_ex.hxx>
#include <vcl/kernarray.hxx>
#include <vcl/svapp.hxx>
#include <vcl/unohelp2.hxx>
#include <vcl/settings.hxx>

#include <tools/gen.hxx>

#include <editeng/editobj.hxx>


#include "accessibility.hxx"
#include <document.hxx>
#include <view.hxx>
#include <strings.hrc>
#include <smmod.hxx>

using namespace com::sun::star;
using namespace com::sun::star::lang;
using namespace com::sun::star::uno;
using namespace com::sun::star::accessibility;

SmGraphicAccessible::SmGraphicAccessible(SmGraphicWidget* pGraphicWin)
    : aAccName(SmResId(RID_DOCUMENTSTR))
    , pWin(pGraphicWin)
{
    assert(pWin && "SmGraphicAccessible: window missing");
}

SmGraphicAccessible::~SmGraphicAccessible()
{
}

SmDocShell * SmGraphicAccessible::GetDoc_Impl()
{
    SmViewShell *pView = pWin ? &pWin->GetView() : nullptr;
    return pView ? pView->GetDoc() : nullptr;
}

OUString SmGraphicAccessible::GetAccessibleText_Impl()
{
    OUString aTxt;
    SmDocShell *pDoc = GetDoc_Impl();
    if (pDoc)
        aTxt = pDoc->GetAccessibleText();
    return aTxt;
}

void SAL_CALL SmGraphicAccessible::disposing()
{
    pWin = nullptr;   // implicitly results in AccessibleStateType::DEFUNC set

    comphelper::OAccessibleComponentHelper::disposing();
}

void SmGraphicAccessible::LaunchEvent(
        const sal_Int16 nAccessibleEventId,
        const uno::Any &rOldVal,
        const uno::Any &rNewVal)
{
    NotifyAccessibleEvent(nAccessibleEventId, rOldVal, rNewVal);
}

uno::Reference< XAccessibleContext > SAL_CALL SmGraphicAccessible::getAccessibleContext()
{
    return this;
}

uno::Reference<XAccessible> SAL_CALL SmGraphicAccessible::getAccessibleAtPoint(const awt::Point&)
{
    SolarMutexGuard aGuard;
    return nullptr;
}

awt::Rectangle SmGraphicAccessible::implGetBounds()
{
    assert(pWin);

    const Size aOutSize(pWin->GetOutputSizePixel());

    return css::awt::Rectangle(0, 0, aOutSize.Width(), aOutSize.Height());
}

void SAL_CALL SmGraphicAccessible::grabFocus()
{
    SolarMutexGuard aGuard;
    if (!pWin)
        throw RuntimeException();

    pWin->GrabFocus();
}

sal_Int32 SAL_CALL SmGraphicAccessible::getForeground()
{
    SolarMutexGuard aGuard;
    if (!pWin)
        throw RuntimeException();

    weld::DrawingArea* pDrawingArea = pWin->GetDrawingArea();
    OutputDevice& rDevice = pDrawingArea->get_ref_device();

    return static_cast<sal_Int32>(rDevice.GetTextColor());
}

sal_Int32 SAL_CALL SmGraphicAccessible::getBackground()
{
    SolarMutexGuard aGuard;
    if (!pWin)
        throw RuntimeException();

    weld::DrawingArea* pDrawingArea = pWin->GetDrawingArea();
    OutputDevice& rDevice = pDrawingArea->get_ref_device();

    Wallpaper aWall(rDevice.GetBackground());
    Color nCol;
    if (aWall.IsBitmap() || aWall.IsGradient())
        nCol = Application::GetSettings().GetStyleSettings().GetWindowColor();
    else
        nCol = aWall.GetColor();
    return static_cast<sal_Int32>(nCol);
}

sal_Int64 SAL_CALL SmGraphicAccessible::getAccessibleChildCount()
{
    return 0;
}

Reference< XAccessible > SAL_CALL SmGraphicAccessible::getAccessibleChild(
        sal_Int64 /*i*/ )
{
    throw IndexOutOfBoundsException();  // there is no child...
}

Reference< XAccessible > SAL_CALL SmGraphicAccessible::getAccessibleParent()
{
    SolarMutexGuard aGuard;
    if (!pWin)
        throw RuntimeException();

    return pWin->GetDrawingArea()->get_accessible_parent();
}

sal_Int16 SAL_CALL SmGraphicAccessible::getAccessibleRole()
{
    return AccessibleRole::DOCUMENT;
}

OUString SAL_CALL SmGraphicAccessible::getAccessibleDescription()
{
    SolarMutexGuard aGuard;
    SmDocShell *pDoc = GetDoc_Impl();
    return pDoc ? pDoc->GetText() : OUString();
}

OUString SAL_CALL SmGraphicAccessible::getAccessibleName()
{
    SolarMutexGuard aGuard;
    return aAccName;
}

Reference< XAccessibleRelationSet > SAL_CALL SmGraphicAccessible::getAccessibleRelationSet()
{
    return new utl::AccessibleRelationSetHelper(); // empty relation set
}

sal_Int64 SAL_CALL SmGraphicAccessible::getAccessibleStateSet()
{
    SolarMutexGuard aGuard;
    sal_Int64 nStateSet = 0;

    if (!pWin)
        nStateSet |= AccessibleStateType::DEFUNC;
    else
    {
        nStateSet |= AccessibleStateType::ENABLED;
        nStateSet |= AccessibleStateType::FOCUSABLE;
        if (pWin->HasFocus())
            nStateSet |= AccessibleStateType::FOCUSED;
        if (pWin->IsActive())
            nStateSet |= AccessibleStateType::ACTIVE;
        if (pWin->IsVisible())
            nStateSet |= AccessibleStateType::SHOWING;
        if (pWin->IsReallyVisible())
            nStateSet |= AccessibleStateType::VISIBLE;
        weld::DrawingArea* pDrawingArea = pWin->GetDrawingArea();
        OutputDevice& rDevice = pDrawingArea->get_ref_device();
        if (COL_TRANSPARENT != rDevice.GetBackground().GetColor())
            nStateSet |= AccessibleStateType::OPAQUE;
    }

    return nStateSet;
}

Locale SAL_CALL SmGraphicAccessible::getLocale()
{
    SolarMutexGuard aGuard;
    // should be the document language...
    // We use the language of the localized symbol names here.
    return Application::GetSettings().GetUILanguageTag().getLocale();
}

sal_Int32 SAL_CALL SmGraphicAccessible::getCaretPosition()
{
    return 0;
}

sal_Bool SAL_CALL SmGraphicAccessible::setCaretPosition( sal_Int32 nIndex )
{
    SolarMutexGuard aGuard;
    OUString aTxt( GetAccessibleText_Impl() );
    if (nIndex >= aTxt.getLength())
        throw IndexOutOfBoundsException();
    return false;
}

sal_Unicode SAL_CALL SmGraphicAccessible::getCharacter( sal_Int32 nIndex )
{
    SolarMutexGuard aGuard;
    OUString aTxt( GetAccessibleText_Impl() );
    if (nIndex >= aTxt.getLength())
        throw IndexOutOfBoundsException();
    return aTxt[nIndex];
}

Sequence< beans::PropertyValue > SAL_CALL SmGraphicAccessible::getCharacterAttributes(
        sal_Int32 nIndex,
        const uno::Sequence< OUString > & /*rRequestedAttributes*/ )
{
    SolarMutexGuard aGuard;
    sal_Int32 nLen = GetAccessibleText_Impl().getLength();
    if (0 > nIndex  ||  nIndex >= nLen)
        throw IndexOutOfBoundsException();
    return Sequence< beans::PropertyValue >();
}

awt::Rectangle SAL_CALL SmGraphicAccessible::getCharacterBounds( sal_Int32 nIndex )
{
    SolarMutexGuard aGuard;

    awt::Rectangle aRes;

    if (!pWin)
        throw RuntimeException();

    // get accessible text
    SmDocShell* pDoc  = pWin->GetView().GetDoc();
    if (!pDoc)
        throw RuntimeException();
    OUString aTxt( GetAccessibleText_Impl() );
    if (0 > nIndex  ||  nIndex > aTxt.getLength())   // aTxt.getLength() is valid
        throw IndexOutOfBoundsException();

    // find a reasonable rectangle for position aTxt.getLength().
    bool bWasBehindText = (nIndex == aTxt.getLength());
    if (bWasBehindText && nIndex)
        --nIndex;

    const SmNode *pTree = pDoc->GetFormulaTree();
    const SmNode *pNode = pTree->FindNodeWithAccessibleIndex( nIndex );
    //! pNode may be 0 if the index belongs to a char that was inserted
    //! only for the accessible text!
    if (pNode)
    {
        sal_Int32 nAccIndex = pNode->GetAccessibleIndex();
        OSL_ENSURE( nAccIndex >= 0, "invalid accessible index" );
        OSL_ENSURE( nIndex >= nAccIndex, "index out of range" );

        OUStringBuffer aBuf;
        pNode->GetAccessibleText(aBuf);
        OUString aNodeText = aBuf.makeStringAndClear();
        sal_Int32 nNodeIndex = nIndex - nAccIndex;
        if (0 <= nNodeIndex  &&  nNodeIndex < aNodeText.getLength())
        {
            // get appropriate rectangle
            Point aOffset(pNode->GetTopLeft() - pTree->GetTopLeft());
            Point aTLPos (pWin->GetFormulaDrawPos() + aOffset);
            Size  aSize (pNode->GetSize());

            weld::DrawingArea* pDrawingArea = pWin->GetDrawingArea();
            OutputDevice& rDevice = pDrawingArea->get_ref_device();

            KernArray aXAry;
            rDevice.SetFont( pNode->GetFont() );
            rDevice.GetTextArray( aNodeText, &aXAry, 0, aNodeText.getLength() );
            aTLPos.AdjustX(nNodeIndex > 0 ? aXAry[nNodeIndex - 1] : 0 );
            aSize.setWidth( nNodeIndex > 0 ? aXAry[nNodeIndex] - aXAry[nNodeIndex - 1] : aXAry[nNodeIndex] );

            aTLPos = rDevice.LogicToPixel( aTLPos );
            aSize  = rDevice.LogicToPixel( aSize );
            aRes.X = aTLPos.X();
            aRes.Y = aTLPos.Y();
            aRes.Width  = aSize.Width();
            aRes.Height = aSize.Height();
        }
    }

    // take rectangle from last character and move it to the right
    if (bWasBehindText)
        aRes.X += aRes.Width;

    return aRes;
}

sal_Int32 SAL_CALL SmGraphicAccessible::getCharacterCount()
{
    SolarMutexGuard aGuard;
    return GetAccessibleText_Impl().getLength();
}

sal_Int32 SAL_CALL SmGraphicAccessible::getIndexAtPoint( const awt::Point& aPoint )
{
    SolarMutexGuard aGuard;

    sal_Int32 nRes = -1;
    if (pWin)
    {
        const SmNode *pTree = pWin->GetView().GetDoc()->GetFormulaTree();
        // can be NULL! e.g. if one clicks within the window already during loading of the
        // document (before the parser even started)
        if (!pTree)
            return nRes;

        weld::DrawingArea* pDrawingArea = pWin->GetDrawingArea();
        OutputDevice& rDevice = pDrawingArea->get_ref_device();

        // get position relative to formula draw position
        Point  aPos( aPoint.X, aPoint.Y );
        aPos = rDevice.PixelToLogic( aPos );
        aPos -= pWin->GetFormulaDrawPos();

        // if it was inside the formula then get the appropriate node
        const SmNode *pNode = nullptr;
        if (pTree->OrientedDist(aPos) <= 0)
            pNode = pTree->FindRectClosestTo(aPos);

        if (pNode)
        {
            // get appropriate rectangle
            Point   aOffset( pNode->GetTopLeft() - pTree->GetTopLeft() );
            Point   aTLPos ( aOffset );
            Size  aSize( pNode->GetSize() );

            tools::Rectangle aRect( aTLPos, aSize );
            if (aRect.Contains( aPos ))
            {
                OSL_ENSURE( pNode->IsVisible(), "node is not a leaf" );
                OUStringBuffer aBuf;
                pNode->GetAccessibleText(aBuf);
                OUString aTxt = aBuf.makeStringAndClear();
                OSL_ENSURE( !aTxt.isEmpty(), "no accessible text available" );

                tools::Long nNodeX = pNode->GetLeft();

                KernArray aXAry;
                rDevice.SetFont( pNode->GetFont() );
                rDevice.GetTextArray( aTxt, &aXAry, 0, aTxt.getLength() );
                for (sal_Int32 i = 0;  i < aTxt.getLength()  &&  nRes == -1;  ++i)
                {
                    if (aXAry[i] + nNodeX > aPos.X())
                        nRes = i;
                }
                OSL_ENSURE( nRes >= 0  &&  nRes < aTxt.getLength(), "index out of range" );
                OSL_ENSURE( pNode->GetAccessibleIndex() >= 0,
                        "invalid accessible index" );

                nRes = pNode->GetAccessibleIndex() + nRes;
            }
        }
    }
    return nRes;
}

OUString SAL_CALL SmGraphicAccessible::getSelectedText()
{
    return OUString();
}

sal_Int32 SAL_CALL SmGraphicAccessible::getSelectionStart()
{
    return -1;
}

sal_Int32 SAL_CALL SmGraphicAccessible::getSelectionEnd()
{
    return -1;
}

sal_Bool SAL_CALL SmGraphicAccessible::setSelection(
        sal_Int32 nStartIndex,
        sal_Int32 nEndIndex )
{
    SolarMutexGuard aGuard;
    sal_Int32 nLen = GetAccessibleText_Impl().getLength();
    if (0 > nStartIndex  ||  nStartIndex >= nLen ||
        0 > nEndIndex    ||  nEndIndex   >= nLen)
        throw IndexOutOfBoundsException();
    return false;
}

OUString SAL_CALL SmGraphicAccessible::getText()
{
    SolarMutexGuard aGuard;
    return GetAccessibleText_Impl();
}

OUString SAL_CALL SmGraphicAccessible::getTextRange(
        sal_Int32 nStartIndex,
        sal_Int32 nEndIndex )
{
    //!! nEndIndex may be the string length per definition of the interface !!
    //!! text should be copied exclusive that end index though. And arguments
    //!! may be switched.

    SolarMutexGuard aGuard;
    OUString aTxt( GetAccessibleText_Impl() );
    sal_Int32 nStart = std::min(nStartIndex, nEndIndex);
    sal_Int32 nEnd   = std::max(nStartIndex, nEndIndex);
    if ((nStart > aTxt.getLength()) ||
        (nEnd   > aTxt.getLength()))
        throw IndexOutOfBoundsException();
    return aTxt.copy( nStart, nEnd - nStart );
}

css::accessibility::TextSegment SAL_CALL SmGraphicAccessible::getTextAtIndex( sal_Int32 nIndex, sal_Int16 aTextType )
{
    SolarMutexGuard aGuard;
    OUString aTxt( GetAccessibleText_Impl() );
    //!! nIndex is allowed to be the string length
    if (nIndex > aTxt.getLength())
        throw IndexOutOfBoundsException();

    css::accessibility::TextSegment aResult;
    aResult.SegmentStart = -1;
    aResult.SegmentEnd = -1;
    if ( (AccessibleTextType::CHARACTER == aTextType)  &&  (nIndex < aTxt.getLength()) )
    {
        auto nIndexEnd = nIndex;
        aTxt.iterateCodePoints(&nIndexEnd);

        aResult.SegmentText = aTxt.copy(nIndex, nIndexEnd - nIndex);
        aResult.SegmentStart = nIndex;
        aResult.SegmentEnd = nIndexEnd;
    }
    return aResult;
}

css::accessibility::TextSegment SAL_CALL SmGraphicAccessible::getTextBeforeIndex( sal_Int32 nIndex, sal_Int16 aTextType )
{
    SolarMutexGuard aGuard;
    OUString aTxt( GetAccessibleText_Impl() );
    //!! nIndex is allowed to be the string length
    if (nIndex > aTxt.getLength())
        throw IndexOutOfBoundsException();

    css::accessibility::TextSegment aResult;
    aResult.SegmentStart = -1;
    aResult.SegmentEnd = -1;

    if ( (AccessibleTextType::CHARACTER == aTextType)  && nIndex > 0 )
    {
        aTxt.iterateCodePoints(&nIndex, -1);
        auto nIndexEnd = nIndex;
        aTxt.iterateCodePoints(&nIndexEnd);
        aResult.SegmentText = aTxt.copy(nIndex, nIndexEnd - nIndex);
        aResult.SegmentStart = nIndex;
        aResult.SegmentEnd = nIndexEnd;
    }
    return aResult;
}

css::accessibility::TextSegment SAL_CALL SmGraphicAccessible::getTextBehindIndex( sal_Int32 nIndex, sal_Int16 aTextType )
{
    SolarMutexGuard aGuard;
    OUString aTxt( GetAccessibleText_Impl() );
    //!! nIndex is allowed to be the string length
    if (nIndex > aTxt.getLength())
        throw IndexOutOfBoundsException();

    css::accessibility::TextSegment aResult;
    aResult.SegmentStart = -1;
    aResult.SegmentEnd = -1;

    if ( (AccessibleTextType::CHARACTER == aTextType)  &&  (nIndex + 1 < aTxt.getLength()) )
    {
        aTxt.iterateCodePoints(&nIndex);
        auto nIndexEnd = nIndex;
        aTxt.iterateCodePoints(&nIndexEnd);
        aResult.SegmentText = aTxt.copy(nIndex, nIndexEnd - nIndex);
        aResult.SegmentStart = nIndex;
        aResult.SegmentEnd = nIndexEnd;
    }
    return aResult;
}

sal_Bool SAL_CALL SmGraphicAccessible::copyText(
        sal_Int32 nStartIndex,
        sal_Int32 nEndIndex )
{
    SolarMutexGuard aGuard;
    bool bReturn = false;

    if (!pWin)
        throw RuntimeException();

    Reference< datatransfer::clipboard::XClipboard > xClipboard = pWin->GetClipboard();
    if ( xClipboard.is() )
    {
        OUString sText( getTextRange(nStartIndex, nEndIndex) );

        rtl::Reference<vcl::unohelper::TextDataObject> pDataObj = new vcl::unohelper::TextDataObject( sText );
        SolarMutexReleaser aReleaser;
        xClipboard->setContents( pDataObj, nullptr );

        Reference< datatransfer::clipboard::XFlushableClipboard > xFlushableClipboard( xClipboard, uno::UNO_QUERY );
        if( xFlushableClipboard.is() )
            xFlushableClipboard->flushClipboard();

        bReturn = true;
    }


    return bReturn;
}

sal_Bool SAL_CALL SmGraphicAccessible::scrollSubstringTo( sal_Int32, sal_Int32, AccessibleScrollType )
{
    return false;
}

OUString SAL_CALL SmGraphicAccessible::getImplementationName()
{
    return u"SmGraphicAccessible"_ustr;
}

sal_Bool SAL_CALL SmGraphicAccessible::supportsService(
        const OUString& rServiceName )
{
    return  cppu::supportsService(this, rServiceName);
}

Sequence< OUString > SAL_CALL SmGraphicAccessible::getSupportedServiceNames()
{
    return {
        u"css::accessibility::Accessible"_ustr,
        u"css::accessibility::AccessibleComponent"_ustr,
        u"css::accessibility::AccessibleContext"_ustr,
        u"css::accessibility::AccessibleText"_ustr
    };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
