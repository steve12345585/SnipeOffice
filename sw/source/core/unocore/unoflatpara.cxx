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

#include <unobaseclass.hxx>
#include <unocrsrhelper.hxx>
#include <unoflatpara.hxx>

#include <o3tl/safeint.hxx>
#include <utility>
#include <vcl/svapp.hxx>
#include <com/sun/star/text/TextMarkupType.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <unotextmarkup.hxx>
#include <ndtxt.hxx>
#include <doc.hxx>
#include <IDocumentLayoutAccess.hxx>
#include <IDocumentStylePoolAccess.hxx>
#include <viewsh.hxx>
#include <viewimp.hxx>
#include <breakit.hxx>
#include <pam.hxx>
#include <unotextrange.hxx>
#include <pagefrm.hxx>
#include <cntfrm.hxx>
#include <txtfrm.hxx>
#include <rootfrm.hxx>
#include <poolfmt.hxx>
#include <pagedesc.hxx>
#include <GrammarContact.hxx>
#include <viewopt.hxx>
#include <comphelper/servicehelper.hxx>
#include <comphelper/propertysetinfo.hxx>
#include <comphelper/sequence.hxx>
#include <sal/log.hxx>

#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <com/sun/star/text/XTextRange.hpp>

using namespace ::com::sun::star;

namespace SwUnoCursorHelper {

uno::Reference<text::XFlatParagraphIterator>
CreateFlatParagraphIterator(SwDoc & rDoc, sal_Int32 const nTextMarkupType,
        bool const bAutomatic)
{
    return new SwXFlatParagraphIterator(rDoc, nTextMarkupType, bAutomatic);
}

}

SwXFlatParagraph::SwXFlatParagraph( SwTextNode& rTextNode, OUString aExpandText, const ModelToViewHelper& rMap )
    : SwXFlatParagraph_Base(& rTextNode, rMap)
    , maExpandText(std::move(aExpandText))
    , maOrigText(rTextNode.GetText())
{
}

SwXFlatParagraph::~SwXFlatParagraph()
{
}


// XPropertySet
uno::Reference< beans::XPropertySetInfo > SAL_CALL
SwXFlatParagraph::getPropertySetInfo()
{
    static const comphelper::PropertyMapEntry s_Entries[] = {
        { u"FieldPositions"_ustr, -1, ::cppu::UnoType<uno::Sequence<sal_Int32>>::get(), beans::PropertyAttribute::READONLY, 0 },
        { u"FootnotePositions"_ustr, -1, ::cppu::UnoType<uno::Sequence<sal_Int32>>::get(), beans::PropertyAttribute::READONLY, 0 },
        { u"SortedTextId"_ustr, -1, ::cppu::UnoType<sal_Int32>::get(), beans::PropertyAttribute::READONLY, 0 },
        { u"DocumentElementsCount"_ustr, -1, ::cppu::UnoType<sal_Int32>::get(), beans::PropertyAttribute::READONLY, 0 },
    };
    return new comphelper::PropertySetInfo(s_Entries);
}

void SAL_CALL
SwXFlatParagraph::setPropertyValue(const OUString&, const uno::Any&)
{
    throw lang::IllegalArgumentException(u"no values can be set"_ustr,
            getXWeak(), 0);
}

uno::Any SAL_CALL
SwXFlatParagraph::getPropertyValue(const OUString& rPropertyName)
{
    SolarMutexGuard g;

    if (rPropertyName == "FieldPositions")
    {
        return uno::Any( comphelper::containerToSequence( GetConversionMap().getFieldPositions() ) );
    }
    else if (rPropertyName == "FootnotePositions")
    {
        return uno::Any( comphelper::containerToSequence( GetConversionMap().getFootnotePositions() ) );
    }
    else if (rPropertyName == "SortedTextId")
    {
        SwTextNode const*const pCurrentNode = GetTextNode();
        sal_Int32 nIndex = -1;
        if ( pCurrentNode )
            nIndex = pCurrentNode->GetIndex().get();
        return uno::Any( nIndex );
    }
    else if (rPropertyName == "DocumentElementsCount")
    {
        SwTextNode const*const pCurrentNode = GetTextNode();
        sal_Int32 nCount = -1;
        if ( pCurrentNode )
            nCount = pCurrentNode->GetDoc().GetNodes().Count().get();
        return uno::Any( nCount );
    }
    return uno::Any();
}

void SAL_CALL
SwXFlatParagraph::addPropertyChangeListener(
        const OUString& /*rPropertyName*/,
        const uno::Reference< beans::XPropertyChangeListener >& /*xListener*/)
{
    SAL_WARN("sw.uno",
        "SwXFlatParagraph::addPropertyChangeListener(): not implemented");
}

void SAL_CALL
SwXFlatParagraph::removePropertyChangeListener(
        const OUString& /*rPropertyName*/,
        const uno::Reference< beans::XPropertyChangeListener >& /*xListener*/)
{
    SAL_WARN("sw.uno",
        "SwXFlatParagraph::removePropertyChangeListener(): not implemented");
}

void SAL_CALL
SwXFlatParagraph::addVetoableChangeListener(
        const OUString& /*rPropertyName*/,
        const uno::Reference< beans::XVetoableChangeListener >& /*xListener*/)
{
    SAL_WARN("sw.uno",
        "SwXFlatParagraph::addVetoableChangeListener(): not implemented");
}

void SAL_CALL
SwXFlatParagraph::removeVetoableChangeListener(
        const OUString& /*rPropertyName*/,
        const uno::Reference< beans::XVetoableChangeListener >& /*xListener*/)
{
    SAL_WARN("sw.uno",
        "SwXFlatParagraph::removeVetoableChangeListener(): not implemented");
}


css::uno::Reference< css::container::XStringKeyMap > SAL_CALL SwXFlatParagraph::getMarkupInfoContainer()
{
    return SwXTextMarkup::getMarkupInfoContainer();
}

void SAL_CALL SwXFlatParagraph::commitTextRangeMarkup(::sal_Int32 nType, const OUString & aIdentifier, const uno::Reference< text::XTextRange> & xRange,
                                                      const css::uno::Reference< css::container::XStringKeyMap > & xMarkupInfoContainer)
{
    SolarMutexGuard aGuard;
    SwXTextMarkup::commitTextRangeMarkup( nType, aIdentifier, xRange,  xMarkupInfoContainer );
}

void SAL_CALL SwXFlatParagraph::commitStringMarkup(::sal_Int32 nType, const OUString & rIdentifier, ::sal_Int32 nStart, ::sal_Int32 nLength, const css::uno::Reference< css::container::XStringKeyMap > & rxMarkupInfoContainer)
{
    SolarMutexGuard aGuard;
    SwXTextMarkup::commitStringMarkup( nType, rIdentifier, nStart, nLength,  rxMarkupInfoContainer );
}

// text::XFlatParagraph:
OUString SAL_CALL SwXFlatParagraph::getText()
{
    return maExpandText;
}

// text::XFlatParagraph:
void SAL_CALL SwXFlatParagraph::setChecked( ::sal_Int32 nType, sal_Bool bVal )
{
    SolarMutexGuard aGuard;

    if (!GetTextNode())
        return;

    if ( text::TextMarkupType::SPELLCHECK == nType )
    {
        GetTextNode()->SetWrongDirty(
            bVal ? sw::WrongState::DONE : sw::WrongState::TODO);
    }
    else if ( text::TextMarkupType::SMARTTAG == nType )
        GetTextNode()->SetSmartTagDirty( !bVal );
    else if( text::TextMarkupType::PROOFREADING == nType )
    {
        GetTextNode()->SetGrammarCheckDirty( !bVal );
        if( bVal )
            sw::finishGrammarCheckFor(*GetTextNode());
    }
}

// text::XFlatParagraph:
sal_Bool SAL_CALL SwXFlatParagraph::isChecked( ::sal_Int32 nType )
{
    SolarMutexGuard aGuard;
    if (GetTextNode())
    {
        if ( text::TextMarkupType::SPELLCHECK == nType )
            return !GetTextNode()->IsWrongDirty();
        else if ( text::TextMarkupType::PROOFREADING == nType )
            return !GetTextNode()->IsGrammarCheckDirty();
        else if ( text::TextMarkupType::SMARTTAG == nType )
            return !GetTextNode()->IsSmartTagDirty();
    }

    return true;
}

// text::XFlatParagraph:
sal_Bool SAL_CALL SwXFlatParagraph::isModified()
{
    SolarMutexGuard aGuard;
    return !GetTextNode() || GetTextNode()->GetText() != maOrigText;
}

// text::XFlatParagraph:
lang::Locale SAL_CALL SwXFlatParagraph::getLanguageOfText(::sal_Int32 nPos, ::sal_Int32 nLen)
{
    SolarMutexGuard aGuard;
    if (!GetTextNode())
        return LanguageTag::convertToLocale( LANGUAGE_NONE );

    const lang::Locale aLocale( SW_BREAKITER()->GetLocale( GetTextNode()->GetLang(nPos, nLen) ) );
    return aLocale;
}

// text::XFlatParagraph:
lang::Locale SAL_CALL SwXFlatParagraph::getPrimaryLanguageOfText(::sal_Int32 nPos, ::sal_Int32 nLen)
{
    SolarMutexGuard aGuard;

    if (!GetTextNode())
        return LanguageTag::convertToLocale( LANGUAGE_NONE );

    const lang::Locale aLocale( SW_BREAKITER()->GetLocale( GetTextNode()->GetLang(nPos, nLen) ) );
    return aLocale;
}

// text::XFlatParagraph:
void SAL_CALL SwXFlatParagraph::changeText(::sal_Int32 nPos, ::sal_Int32 nLen, const OUString & aNewText, const css::uno::Sequence< css::beans::PropertyValue > & aAttributes)
{
    SolarMutexGuard aGuard;

    if (!GetTextNode())
        return;

    SwTextNode *const pOldTextNode = GetTextNode();

    if (nPos < 0 || pOldTextNode->Len() < nPos || nLen < 0 || o3tl::make_unsigned(pOldTextNode->Len()) < static_cast<sal_uInt32>(nPos) + nLen)
    {
        throw lang::IllegalArgumentException();
    }

    SwPaM aPaM( *GetTextNode(), nPos, *GetTextNode(), nPos+nLen );

    UnoActionContext aAction( &GetTextNode()->GetDoc() );

    const rtl::Reference<SwXTextRange> xRange =
        SwXTextRange::CreateXTextRange(
            GetTextNode()->GetDoc(), *aPaM.GetPoint(), aPaM.GetMark() );
    if ( xRange.is() )
    {
        for ( const auto& rAttribute : aAttributes )
            xRange->setPropertyValue( rAttribute.Name, rAttribute.Value );
    }

    IDocumentContentOperations& rIDCO = pOldTextNode->getIDocumentContentOperations();
    rIDCO.ReplaceRange( aPaM, aNewText, false );

    ClearTextNode(); // TODO: is this really needed?
}

// text::XFlatParagraph:
void SAL_CALL SwXFlatParagraph::changeAttributes(::sal_Int32 nPos, ::sal_Int32 nLen, const css::uno::Sequence< css::beans::PropertyValue > & aAttributes)
{
    SolarMutexGuard aGuard;

    if (!GetTextNode())
        return;

    if (nPos < 0 || GetTextNode()->Len() < nPos || nLen < 0 || o3tl::make_unsigned(GetTextNode()->Len()) < static_cast<sal_uInt32>(nPos) + nLen)
    {
        throw lang::IllegalArgumentException();
    }

    SwPaM aPaM( *GetTextNode(), nPos, *GetTextNode(), nPos+nLen );

    UnoActionContext aAction( &GetTextNode()->GetDoc() );

    const rtl::Reference<SwXTextRange> xRange =
        SwXTextRange::CreateXTextRange(
            GetTextNode()->GetDoc(), *aPaM.GetPoint(), aPaM.GetMark() );
    if ( xRange.is() )
    {
        for ( const auto& rAttribute : aAttributes )
            xRange->setPropertyValue( rAttribute.Name, rAttribute.Value );
    }

    ClearTextNode(); // TODO: is this really needed?
}

// text::XFlatParagraph:
css::uno::Sequence< ::sal_Int32 > SAL_CALL SwXFlatParagraph::getLanguagePortions()
{
    return css::uno::Sequence< ::sal_Int32>();
}

SwXFlatParagraphIterator::SwXFlatParagraphIterator( SwDoc& rDoc, sal_Int32 nType, bool bAutomatic )
    : mpDoc( &rDoc ),
      mnType( nType ),
      mbAutomatic( bAutomatic ),
      mnCurrentNode( 0 ),
      mnEndNode( rDoc.GetNodes().Count() )
{
    //mnStartNode = mnCurrentNode = get node from current cursor TODO!

    // register as listener and get notified when document is closed
    StartListening(mpDoc->getIDocumentStylePoolAccess().GetPageDescFromPool( RES_POOLPAGE_STANDARD )->GetNotifier());
}

SwXFlatParagraphIterator::~SwXFlatParagraphIterator()
{
    SolarMutexGuard aGuard;
    EndListeningAll();
}

void SwXFlatParagraphIterator::Notify( const SfxHint& rHint )
{
    if(rHint.GetId() == SfxHintId::Dying)
    {
        SolarMutexGuard aGuard;
        mpDoc = nullptr;
    }
}

uno::Reference< text::XFlatParagraph > SwXFlatParagraphIterator::getFirstPara()
{
    return getNextPara();   // TODO
}

uno::Reference< text::XFlatParagraph > SwXFlatParagraphIterator::getNextPara()
{
    SolarMutexGuard aGuard;

    if (!mpDoc)
        return nullptr;

    SwTextNode* pRet = nullptr;
    if ( mbAutomatic )
    {
        SwViewShell* pViewShell = mpDoc->getIDocumentLayoutAccess().GetCurrentViewShell();

        SwPageFrame* pCurrentPage = pViewShell ? pViewShell->Imp()->GetFirstVisPage(pViewShell->GetOut()) : nullptr;
        SwPageFrame* pStartPage = pCurrentPage;
        SwPageFrame* pStopPage = nullptr;

        while ( pCurrentPage && pCurrentPage != pStopPage )
        {
            if (mnType != text::TextMarkupType::SPELLCHECK || pCurrentPage->IsInvalidSpelling() )
            {
                // this method is supposed to return an empty paragraph in case Online Checking is disabled
                if ( ( mnType == text::TextMarkupType::PROOFREADING || mnType == text::TextMarkupType::SPELLCHECK )
                    && !pViewShell->GetViewOptions()->IsOnlineSpell() )
                    return nullptr;

                // search for invalid content:
                SwContentFrame* pCnt = pCurrentPage->ContainsContent();

                while( pCnt && pCurrentPage->IsAnLower( pCnt ) )
                {
                    if (pCnt->IsTextFrame())
                    {
                        SwTextFrame const*const pText(static_cast<SwTextFrame const*>(pCnt));
                        if (sw::MergedPara const*const pMergedPara = pText->GetMergedPara()
            )
                        {
                            SwTextNode * pTextNode(nullptr);
                            for (auto const& e : pMergedPara->extents)
                            {
                                if (e.pNode != pTextNode)
                                {
                                    pTextNode = e.pNode;
                                    if ((mnType == text::TextMarkupType::SPELLCHECK
                                            && pTextNode->IsWrongDirty()) ||
                                        (mnType == text::TextMarkupType::PROOFREADING
                                             && pTextNode->IsGrammarCheckDirty()))
                                    {
                                        pRet = pTextNode;
                                        break;
                                    }
                                }
                            }
                        }
                        else
                        {
                            SwTextNode const*const pTextNode(pText->GetTextNodeFirst());
                            if ((mnType == text::TextMarkupType::SPELLCHECK
                                    && pTextNode->IsWrongDirty()) ||
                                (mnType == text::TextMarkupType::PROOFREADING
                                     && pTextNode->IsGrammarCheckDirty()))

                            {
                                pRet = const_cast<SwTextNode*>(pTextNode);
                            }
                        }

                        if (pRet)
                        {
                            break;
                        }
                    }

                    pCnt = pCnt->GetNextContentFrame();
                }
            }

            if ( pRet )
                break;

            // if there is no invalid text node on the current page,
            // we validate the page
            pCurrentPage->ValidateSpelling();

            // proceed with next page, wrap at end of document if required:
            pCurrentPage = static_cast<SwPageFrame*>(pCurrentPage->GetNext());

            if ( !pCurrentPage && !pStopPage )
            {
                pStopPage = pStartPage;
                pCurrentPage = static_cast<SwPageFrame*>(pViewShell->GetLayout()->Lower());
            }
        }
    }
    else    // non-automatic checking
    {
        const SwNodes& rNodes = mpDoc->GetNodes();
        const SwNodeOffset nMaxNodes = rNodes.Count();

        while ( mnCurrentNode < mnEndNode && mnCurrentNode < nMaxNodes )
        {
            SwNode* pNd = rNodes[ mnCurrentNode ];

            ++mnCurrentNode;

            pRet = pNd->GetTextNode();
            if ( pRet )
                break;

            if ( mnCurrentNode == mnEndNode )
            {
                mnCurrentNode = SwNodeOffset(0);
                mnEndNode = SwNodeOffset(0);
            }
        }
    }

    if ( !pRet )
        return nullptr;

    // Expand the string:
    const ModelToViewHelper aConversionMap(*pRet, mpDoc->getIDocumentLayoutAccess().GetCurrentLayout());
    const OUString& aExpandText = aConversionMap.getViewText();

    return new SwXFlatParagraph( *pRet, aExpandText, aConversionMap );
}

uno::Reference< text::XFlatParagraph > SwXFlatParagraphIterator::getLastPara()
{
    return getNextPara();
}

uno::Reference< text::XFlatParagraph > SwXFlatParagraphIterator::getParaAfter(const uno::Reference< text::XFlatParagraph > & xPara)
{
    SolarMutexGuard aGuard;

    if (!mpDoc)
        return nullptr;

    SwXFlatParagraph* const pFlatParagraph(dynamic_cast<SwXFlatParagraph*>(xPara.get()));
    SAL_WARN_IF(!pFlatParagraph, "sw.core", "invalid argument");
    if ( !pFlatParagraph )
        return nullptr;

    SwTextNode const*const pCurrentNode = pFlatParagraph->GetTextNode();

    if ( !pCurrentNode )
        return nullptr;

    SwTextNode* pNextTextNode = nullptr;
    const SwNodes& rNodes = pCurrentNode->GetDoc().GetNodes();

    for( SwNodeOffset nCurrentNode = pCurrentNode->GetIndex() + 1; nCurrentNode < rNodes.Count(); ++nCurrentNode )
    {
        SwNode* pNd = rNodes[ nCurrentNode ];
        pNextTextNode = pNd->GetTextNode();
        if ( pNextTextNode )
            break;
    }

    if ( !pNextTextNode )
        return nullptr;

    // Expand the string:
    const ModelToViewHelper aConversionMap(*pNextTextNode, mpDoc->getIDocumentLayoutAccess().GetCurrentLayout());
    const OUString& aExpandText = aConversionMap.getViewText();

    return new SwXFlatParagraph( *pNextTextNode, aExpandText, aConversionMap );
}

uno::Reference< text::XFlatParagraph > SwXFlatParagraphIterator::getParaBefore(const uno::Reference< text::XFlatParagraph > & xPara )
{
    SolarMutexGuard aGuard;

    if (!mpDoc)
        return nullptr;

    SwXFlatParagraph* const pFlatParagraph(dynamic_cast<SwXFlatParagraph*>(xPara.get()));
    SAL_WARN_IF(!pFlatParagraph, "sw.core", "invalid argument");
    if ( !pFlatParagraph )
        return nullptr;

    SwTextNode const*const pCurrentNode = pFlatParagraph->GetTextNode();

    if ( !pCurrentNode )
        return nullptr;

    SwTextNode* pPrevTextNode = nullptr;
    const SwNodes& rNodes = pCurrentNode->GetDoc().GetNodes();

    for( SwNodeOffset nCurrentNode = pCurrentNode->GetIndex() - 1; nCurrentNode > SwNodeOffset(0); --nCurrentNode )
    {
        SwNode* pNd = rNodes[ nCurrentNode ];
        pPrevTextNode = pNd->GetTextNode();
        if ( pPrevTextNode )
            break;
    }

    if ( !pPrevTextNode )
        return nullptr;

    // Expand the string:
    const ModelToViewHelper aConversionMap(*pPrevTextNode, mpDoc->getIDocumentLayoutAccess().GetCurrentLayout());
    const OUString& aExpandText = aConversionMap.getViewText();

    return new SwXFlatParagraph( *pPrevTextNode, aExpandText, aConversionMap );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
