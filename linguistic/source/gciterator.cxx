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

#include <sal/macros.h>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/ElementExistException.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/configuration/theDefaultProvider.hpp>
#include <com/sun/star/i18n/BreakIterator.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/linguistic2/XDictionary.hpp>
#include <com/sun/star/linguistic2/XSupportedLocales.hpp>
#include <com/sun/star/linguistic2/XProofreader.hpp>
#include <com/sun/star/linguistic2/XProofreadingIterator.hpp>
#include <com/sun/star/linguistic2/SingleProofreadingError.hpp>
#include <com/sun/star/linguistic2/ProofreadingResult.hpp>
#include <com/sun/star/linguistic2/LinguServiceEvent.hpp>
#include <com/sun/star/linguistic2/LinguServiceEventFlags.hpp>
#include <com/sun/star/text/TextMarkupType.hpp>
#include <com/sun/star/text/TextMarkupDescriptor.hpp>
#include <com/sun/star/text/XMultiTextMarkup.hpp>
#include <com/sun/star/text/XFlatParagraph.hpp>
#include <com/sun/star/text/XFlatParagraphIterator.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <sal/config.h>
#include <sal/log.hxx>
#include <o3tl/safeint.hxx>
#include <osl/conditn.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/weak.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/propertysequence.hxx>
#include <tools/debug.hxx>
#include <comphelper/diagnose_ex.hxx>

#include <map>
#include <algorithm>

#include <linguistic/misc.hxx>

#include "gciterator.hxx"

using namespace linguistic;
using namespace ::com::sun::star;

// white space list: obtained from the fonts.config.txt of a Linux system.
const sal_Unicode aWhiteSpaces[] =
{
    0x0020,   /* SPACE */
    0x00a0,   /* NO-BREAK SPACE */
    0x00ad,   /* SOFT HYPHEN */
    0x115f,   /* HANGUL CHOSEONG FILLER */
    0x1160,   /* HANGUL JUNGSEONG FILLER */
    0x1680,   /* OGHAM SPACE MARK */
    0x2000,   /* EN QUAD */
    0x2001,   /* EM QUAD */
    0x2002,   /* EN SPACE */
    0x2003,   /* EM SPACE */
    0x2004,   /* THREE-PER-EM SPACE */
    0x2005,   /* FOUR-PER-EM SPACE */
    0x2006,   /* SIX-PER-EM SPACE */
    0x2007,   /* FIGURE SPACE */
    0x2008,   /* PUNCTUATION SPACE */
    0x2009,   /* THIN SPACE */
    0x200a,   /* HAIR SPACE */
    0x200b,   /* ZERO WIDTH SPACE */
    0x200c,   /* ZERO WIDTH NON-JOINER */
    0x200d,   /* ZERO WIDTH JOINER */
    0x200e,   /* LEFT-TO-RIGHT MARK */
    0x200f,   /* RIGHT-TO-LEFT MARK */
    0x2028,   /* LINE SEPARATOR */
    0x2029,   /* PARAGRAPH SEPARATOR */
    0x202a,   /* LEFT-TO-RIGHT EMBEDDING */
    0x202b,   /* RIGHT-TO-LEFT EMBEDDING */
    0x202c,   /* POP DIRECTIONAL FORMATTING */
    0x202d,   /* LEFT-TO-RIGHT OVERRIDE */
    0x202e,   /* RIGHT-TO-LEFT OVERRIDE */
    0x202f,   /* NARROW NO-BREAK SPACE */
    0x205f,   /* MEDIUM MATHEMATICAL SPACE */
    0x2060,   /* WORD JOINER */
    0x2061,   /* FUNCTION APPLICATION */
    0x2062,   /* INVISIBLE TIMES */
    0x2063,   /* INVISIBLE SEPARATOR */
    0x206A,   /* INHIBIT SYMMETRIC SWAPPING */
    0x206B,   /* ACTIVATE SYMMETRIC SWAPPING */
    0x206C,   /* INHIBIT ARABIC FORM SHAPING */
    0x206D,   /* ACTIVATE ARABIC FORM SHAPING */
    0x206E,   /* NATIONAL DIGIT SHAPES */
    0x206F,   /* NOMINAL DIGIT SHAPES */
    0x3000,   /* IDEOGRAPHIC SPACE */
    0x3164,   /* HANGUL FILLER */
    0xfeff,   /* ZERO WIDTH NO-BREAK SPACE */
    0xffa0,   /* HALFWIDTH HANGUL FILLER */
    0xfff9,   /* INTERLINEAR ANNOTATION ANCHOR */
    0xfffa,   /* INTERLINEAR ANNOTATION SEPARATOR */
    0xfffb    /* INTERLINEAR ANNOTATION TERMINATOR */
};

//  Information about reason for proofreading (ProofInfo)
   const sal_Int32 PROOFINFO_GET_PROOFRESULT = 1;
   const sal_Int32 PROOFINFO_MARK_PARAGRAPH = 2;

static bool lcl_IsWhiteSpace( sal_Unicode cChar )
{
    return std::any_of(std::begin(aWhiteSpaces), std::end(aWhiteSpaces),
        [&cChar](const sal_Unicode c) { return c == cChar; });
}

static sal_Int32 lcl_SkipWhiteSpaces( const OUString &rText, sal_Int32 nStartPos )
{
    // note having nStartPos point right behind the string is OK since that one
    // is a correct end-of-sentence position to be returned from a grammar checker...

    const sal_Int32 nLen = rText.getLength();
    bool bIllegalArgument = false;
    if (nStartPos < 0)
    {
        bIllegalArgument = true;
        nStartPos = 0;
    }
    if (nStartPos > nLen)
    {
        bIllegalArgument = true;
        nStartPos = nLen;
    }
    if (bIllegalArgument)
    {
        SAL_WARN( "linguistic", "lcl_SkipWhiteSpaces: illegal arguments" );
    }

    sal_Int32 nRes = nStartPos;
    if (0 <= nStartPos && nStartPos < nLen)
    {
        const sal_Unicode* const pEnd = rText.getStr() + nLen;
        const sal_Unicode *pText = rText.getStr() + nStartPos;
        while (pText != pEnd && lcl_IsWhiteSpace(*pText))
            ++pText;
        nRes = pText - rText.getStr();
    }

    DBG_ASSERT( 0 <= nRes && nRes <= nLen, "lcl_SkipWhiteSpaces return value out of range" );
    return nRes;
}

static sal_Int32 lcl_BacktraceWhiteSpaces( const OUString &rText, sal_Int32 nStartPos )
{
    // note: having nStartPos point right behind the string is OK since that one
    // is a correct end-of-sentence position to be returned from a grammar checker...

    const sal_Int32 nLen = rText.getLength();
    bool bIllegalArgument = false;
    if (nStartPos < 0)
    {
        bIllegalArgument = true;
        nStartPos = 0;
    }
    if (nStartPos > nLen)
    {
        bIllegalArgument = true;
        nStartPos = nLen;
    }
    if (bIllegalArgument)
    {
        SAL_WARN( "linguistic", "lcl_BacktraceWhiteSpaces: illegal arguments" );
    }

    sal_Int32 nRes = nStartPos;
    sal_Int32 nPosBefore = nStartPos - 1;
    const sal_Unicode *pStart = rText.getStr();
    if (0 <= nPosBefore && nPosBefore < nLen && lcl_IsWhiteSpace( pStart[ nPosBefore ] ))
    {
        nStartPos = nPosBefore;
        const sal_Unicode *pText = rText.getStr() + nStartPos;
        while (pText > pStart && lcl_IsWhiteSpace( *pText ))
            --pText;
        // now add 1 since we want to point to the first char after the last char in the sentence...
        nRes = pText - pStart + 1;
    }

    DBG_ASSERT( 0 <= nRes && nRes <= nLen, "lcl_BacktraceWhiteSpaces return value out of range" );
    return nRes;
}


extern "C" {

static void lcl_workerfunc (void * gci)
{
    osl_setThreadName("GrammarCheckingIterator");

    static_cast<GrammarCheckingIterator*>(gci)->DequeueAndCheck();
}

}

static lang::Locale lcl_GetPrimaryLanguageOfSentence(
    const uno::Reference< text::XFlatParagraph >& xFlatPara,
    sal_Int32 nStartIndex )
{
    //get the language of the first word
    return xFlatPara->getLanguageOfText( nStartIndex, 1 );
}


LngXStringKeyMap::LngXStringKeyMap() {}

void SAL_CALL LngXStringKeyMap::insertValue(const OUString& aKey, const css::uno::Any& aValue)
{
    std::map<OUString, css::uno::Any>::const_iterator aIter = maMap.find(aKey);
    if (aIter != maMap.end())
        throw css::container::ElementExistException();

    maMap[aKey] = aValue;
}

css::uno::Any SAL_CALL LngXStringKeyMap::getValue(const OUString& aKey)
{
    std::map<OUString, css::uno::Any>::const_iterator aIter = maMap.find(aKey);
    if (aIter == maMap.end())
        throw css::container::NoSuchElementException();

    return (*aIter).second;
}

sal_Bool SAL_CALL LngXStringKeyMap::hasValue(const OUString& aKey)
{
    return maMap.contains(aKey);
}

::sal_Int32 SAL_CALL LngXStringKeyMap::getCount() { return maMap.size(); }

OUString SAL_CALL LngXStringKeyMap::getKeyByIndex(::sal_Int32 nIndex)
{
    if (nIndex < 0 || o3tl::make_unsigned(nIndex) >= maMap.size())
        throw css::lang::IndexOutOfBoundsException();

    return OUString();
}

css::uno::Any SAL_CALL LngXStringKeyMap::getValueByIndex(::sal_Int32 nIndex)
{
    if (nIndex < 0 || o3tl::make_unsigned(nIndex) >= maMap.size())
        throw css::lang::IndexOutOfBoundsException();

    return css::uno::Any();
}


osl::Mutex& GrammarCheckingIterator::MyMutex()
{
    static osl::Mutex SINGLETON;
    return SINGLETON;
}

GrammarCheckingIterator::GrammarCheckingIterator() :
    m_bEnd( false ),
    m_bGCServicesChecked( false ),
    m_nDocIdCounter( 0 ),
    m_thread(nullptr),
    m_aEventListeners( MyMutex() ),
    m_aNotifyListeners( MyMutex() )
{
}


GrammarCheckingIterator::~GrammarCheckingIterator()
{
    TerminateThread();
}

void GrammarCheckingIterator::TerminateThread()
{
    oslThread t;
    {
        ::osl::Guard< ::osl::Mutex > aGuard( MyMutex() );
        t = m_thread;
        m_thread = nullptr;
        m_bEnd = true;
        m_aWakeUpThread.set();
    }
    if (t != nullptr)
    {
        osl_joinWithThread(t);
        osl_destroyThread(t);
    }
    // After m_bEnd was used to flag lcl_workerfunc to quit, now
    // reset it so lcl_workerfunc could be relaunched later.
    {
        ::osl::Guard< ::osl::Mutex > aGuard( MyMutex() );
        m_bEnd = false;
    }
}

bool GrammarCheckingIterator::joinThreads()
{
    TerminateThread();
    return true;
}


sal_Int32 GrammarCheckingIterator::NextDocId()
{
    ::osl::Guard< ::osl::Mutex > aGuard( MyMutex() );
    m_nDocIdCounter += 1;
    return m_nDocIdCounter;
}


OUString GrammarCheckingIterator::GetOrCreateDocId(
    const uno::Reference< lang::XComponent > &xComponent )
{
    // internal method; will always be called with locked mutex

    OUString aRes;
    if (xComponent.is())
    {
        if (m_aDocIdMap.contains( xComponent.get() ))
        {
            // return already existing entry
            aRes = m_aDocIdMap[ xComponent.get() ];
        }
        else // add new entry
        {
            sal_Int32 nRes = NextDocId();
            aRes = OUString::number( nRes );
            m_aDocIdMap[ xComponent.get() ] = aRes;
            xComponent->addEventListener( this );
        }
    }
    return aRes;
}


void GrammarCheckingIterator::AddEntry(
    const uno::Reference< text::XFlatParagraphIterator >& xFlatParaIterator,
    const uno::Reference< text::XFlatParagraph >& xFlatPara,
    const OUString & rDocId,
    sal_Int32 nStartIndex,
    bool bAutomatic )
{
    // we may not need/have a xFlatParaIterator (e.g. if checkGrammarAtPos was called)
    // but we always need a xFlatPara...
    if (!xFlatPara.is())
        return;

    FPEntry aNewFPEntry;
    aNewFPEntry.m_xParaIterator = xFlatParaIterator;
    aNewFPEntry.m_xPara         = xFlatPara;
    aNewFPEntry.m_aDocId        = rDocId;
    aNewFPEntry.m_nStartIndex   = nStartIndex;
    aNewFPEntry.m_bAutomatic    = bAutomatic;

    // add new entry to the end of this queue
    ::osl::Guard< ::osl::Mutex > aGuard( MyMutex() );
    if (!m_thread)
        m_thread = osl_createThread( lcl_workerfunc, this );
    m_aFPEntriesQueue.push_back( aNewFPEntry );

    // wake up the thread in order to do grammar checking
    m_aWakeUpThread.set();
}


void GrammarCheckingIterator::ProcessResult(
    const linguistic2::ProofreadingResult &rRes,
    const uno::Reference< text::XFlatParagraphIterator > &rxFlatParagraphIterator,
    bool bIsAutomaticChecking )
{
    DBG_ASSERT( rRes.xFlatParagraph.is(), "xFlatParagraph is missing" );
     //no guard necessary as no members are used
    bool bContinueWithNextPara = false;
    if (!rRes.xFlatParagraph.is() || rRes.xFlatParagraph->isModified())
    {
        // if paragraph was modified/deleted meanwhile continue with the next one...
        bContinueWithNextPara = true;
    }
    else    // paragraph is still unchanged...
    {
        // mark found errors...

        sal_Int32 nTextLen = rRes.aText.getLength();
        bool bBoundariesOk = 0 <= rRes.nStartOfSentencePosition     && rRes.nStartOfSentencePosition <= nTextLen &&
                             0 <= rRes.nBehindEndOfSentencePosition && rRes.nBehindEndOfSentencePosition <= nTextLen &&
                             0 <= rRes.nStartOfNextSentencePosition && rRes.nStartOfNextSentencePosition <= nTextLen &&
                             rRes.nStartOfSentencePosition      <= rRes.nBehindEndOfSentencePosition &&
                             rRes.nBehindEndOfSentencePosition  <= rRes.nStartOfNextSentencePosition;
        DBG_ASSERT( bBoundariesOk, "inconsistent sentence boundaries" );

        uno::Reference< text::XMultiTextMarkup > xMulti( rRes.xFlatParagraph, uno::UNO_QUERY );
        if (xMulti.is())    // use new API for markups
        {
            try
            {
                // length = number of found errors + 1 sentence markup
                sal_Int32 nErrors = rRes.aErrors.getLength();
                uno::Sequence< text::TextMarkupDescriptor > aDescriptors( nErrors + 1 );
                text::TextMarkupDescriptor * pDescriptors = aDescriptors.getArray();

                uno::Reference< linguistic2::XDictionary > xIgnoreAll = ::GetIgnoreAllList();
                sal_Int32 ignoredCount = 0;

                // at pos 0 .. nErrors-1 -> all grammar errors
                for (const linguistic2::SingleProofreadingError &rError : rRes.aErrors)
                {
                    OUString word(rRes.aText.subView(rError.nErrorStart, rError.nErrorLength));
                    bool ignored = xIgnoreAll.is() && xIgnoreAll->getEntry(word).is();

                    if (!ignored)
                    {
                        text::TextMarkupDescriptor &rDesc = *pDescriptors++;

                        rDesc.nType   = rError.nErrorType;
                        rDesc.nOffset = rError.nErrorStart;
                        rDesc.nLength = rError.nErrorLength;

                        // the proofreader may return SPELLING but right now our core
                        // does only handle PROOFREADING if the result is from the proofreader...
                        // (later on we may wish to color spelling errors found by the proofreader
                        // differently for example. But no special handling right now.
                        if (rDesc.nType == text::TextMarkupType::SPELLCHECK)
                            rDesc.nType = text::TextMarkupType::PROOFREADING;

                        uno::Reference< container::XStringKeyMap > xKeyMap(new LngXStringKeyMap());
                        for( const beans::PropertyValue& rProperty : rError.aProperties )
                        {
                            if ( rProperty.Name == "LineColor" )
                            {
                                xKeyMap->insertValue(rProperty.Name, rProperty.Value);
                                rDesc.xMarkupInfoContainer = xKeyMap;
                            }
                            else if ( rProperty.Name == "LineType" )
                            {
                                xKeyMap->insertValue(rProperty.Name, rProperty.Value);
                                rDesc.xMarkupInfoContainer = xKeyMap;
                            }
                        }
                    }
                    else
                        ignoredCount++;
                }

                if (ignoredCount != 0)
                {
                    aDescriptors.realloc(aDescriptors.getLength() - ignoredCount);
                    pDescriptors = aDescriptors.getArray();
                    pDescriptors += aDescriptors.getLength() - 1;
                }

                // at pos nErrors -> sentence markup
                // nSentenceLength: includes the white-spaces following the sentence end...
                const sal_Int32 nSentenceLength = rRes.nStartOfNextSentencePosition - rRes.nStartOfSentencePosition;
                pDescriptors->nType   = text::TextMarkupType::SENTENCE;
                pDescriptors->nOffset = rRes.nStartOfSentencePosition;
                pDescriptors->nLength = nSentenceLength;

                xMulti->commitMultiTextMarkup( aDescriptors ) ;
            }
            catch (lang::IllegalArgumentException &)
            {
                TOOLS_WARN_EXCEPTION( "linguistic", "commitMultiTextMarkup" );
            }
        }

        // other sentences left to be checked in this paragraph?
        if (rRes.nStartOfNextSentencePosition < rRes.aText.getLength())
        {
            AddEntry( rxFlatParagraphIterator, rRes.xFlatParagraph, rRes.aDocumentIdentifier, rRes.nStartOfNextSentencePosition, bIsAutomaticChecking );
        }
        else    // current paragraph finished
        {
            // set "already checked" flag for the current flat paragraph
            if (rRes.xFlatParagraph.is())
                rRes.xFlatParagraph->setChecked( text::TextMarkupType::PROOFREADING, true );

            bContinueWithNextPara = true;
        }
    }

    if (bContinueWithNextPara)
    {
        // we need to continue with the next paragraph
        if (rxFlatParagraphIterator.is())
            AddEntry(rxFlatParagraphIterator, rxFlatParagraphIterator->getNextPara(),
                     rRes.aDocumentIdentifier, 0, bIsAutomaticChecking);
    }
}


std::pair<OUString, std::optional<OUString>>
GrammarCheckingIterator::getServiceForLocale(const lang::Locale& rLocale) const
{
    if (!rLocale.Language.isEmpty())
    {
        const OUString sBcp47 = LanguageTag::convertToBcp47(rLocale, false);
        GCImplNames_t::const_iterator aLangIt(m_aGCImplNamesByLang.find(sBcp47));
        if (aLangIt != m_aGCImplNamesByLang.end())
            return { aLangIt->second, {} };

        for (const auto& sFallbackBcp47 : LanguageTag(rLocale).getFallbackStrings(false))
        {
            aLangIt = m_aGCImplNamesByLang.find(sFallbackBcp47);
            if (aLangIt != m_aGCImplNamesByLang.end())
                return { aLangIt->second, sFallbackBcp47 };
        }
    }

    return {};
}


uno::Reference< linguistic2::XProofreader > GrammarCheckingIterator::GetGrammarChecker(
    lang::Locale &rLocale )
{
    uno::Reference< linguistic2::XProofreader > xRes;

    // ---- THREAD SAFE START ----
    ::osl::Guard< ::osl::Mutex > aGuard( MyMutex() );

    // check supported locales for each grammarchecker if not already done
    if (!m_bGCServicesChecked)
    {
        GetConfiguredGCSvcs_Impl();
        m_bGCServicesChecked = true;
    }

    if (const auto [aSvcImplName, oFallbackBcp47] = getServiceForLocale(rLocale);
        !aSvcImplName.isEmpty()) // matching configured language found?
    {
        if (oFallbackBcp47)
            rLocale = LanguageTag::convertToLocale(*oFallbackBcp47, false);
        GCReferences_t::const_iterator aImplNameIt( m_aGCReferencesByService.find( aSvcImplName ) );
        if (aImplNameIt != m_aGCReferencesByService.end())  // matching impl name found?
        {
            xRes = aImplNameIt->second;
        }
        else    // the service is to be instantiated here for the first time...
        {
            try
            {
                const uno::Reference< uno::XComponentContext >& xContext( comphelper::getProcessComponentContext() );
                uno::Reference< linguistic2::XProofreader > xGC(
                        xContext->getServiceManager()->createInstanceWithContext(aSvcImplName, xContext),
                        uno::UNO_QUERY_THROW );
                uno::Reference< linguistic2::XSupportedLocales > xSuppLoc( xGC, uno::UNO_QUERY_THROW );

                if (xSuppLoc->hasLocale( rLocale ))
                {
                    m_aGCReferencesByService[ aSvcImplName ] = xGC;
                    xRes = xGC;

                    uno::Reference< linguistic2::XLinguServiceEventBroadcaster > xBC( xGC, uno::UNO_QUERY );
                    if (xBC.is())
                        xBC->addLinguServiceEventListener( this );
                }
                else
                {
                    SAL_WARN( "linguistic", "grammar checker does not support required locale" );
                }
            }
            catch (uno::Exception &)
            {
                SAL_WARN( "linguistic", "instantiating grammar checker failed" );
            }
        }
    }
    else // not found - quite normal
    {
        SAL_INFO("linguistic", "No grammar checker found for \""
                                   << LanguageTag::convertToBcp47(rLocale, false) << "\"");
    }
    // ---- THREAD SAFE END ----

    return xRes;
}

static uno::Sequence<beans::PropertyValue>
lcl_makeProperties(uno::Reference<text::XFlatParagraph> const& xFlatPara, sal_Int32 nProofInfo)
{
    uno::Reference<beans::XPropertySet> const xProps(
            xFlatPara, uno::UNO_QUERY_THROW);
    css::uno::Any a (nProofInfo);
    return comphelper::InitPropertySequence({
        { "FieldPositions", xProps->getPropertyValue(u"FieldPositions"_ustr) },
        { "FootnotePositions", xProps->getPropertyValue(u"FootnotePositions"_ustr) },
        { "SortedTextId", xProps->getPropertyValue(u"SortedTextId"_ustr) },
        { "DocumentElementsCount", xProps->getPropertyValue(u"DocumentElementsCount"_ustr) },
        { "ProofInfo", a }
    });
}

void GrammarCheckingIterator::DequeueAndCheck()
{
    for (;;)
    {
        // ---- THREAD SAFE START ----
        bool bQueueEmpty = false;
        {
            ::osl::Guard< ::osl::Mutex > aGuard( MyMutex() );
            if (m_bEnd)
            {
                break;
            }
            bQueueEmpty = m_aFPEntriesQueue.empty();
        }
        // ---- THREAD SAFE END ----

        if (!bQueueEmpty)
        {
            uno::Reference< text::XFlatParagraphIterator > xFPIterator;
            uno::Reference< text::XFlatParagraph > xFlatPara;
            FPEntry aFPEntryItem;
            OUString aCurDocId;
            // ---- THREAD SAFE START ----
            {
                ::osl::Guard< ::osl::Mutex > aGuard( MyMutex() );
                aFPEntryItem        = m_aFPEntriesQueue.front();
                xFPIterator         = aFPEntryItem.m_xParaIterator;
                xFlatPara           = aFPEntryItem.m_xPara;
                m_aCurCheckedDocId  = aFPEntryItem.m_aDocId;
                aCurDocId = m_aCurCheckedDocId;

                m_aFPEntriesQueue.pop_front();
            }
            // ---- THREAD SAFE END ----

            if (xFlatPara.is() && xFPIterator.is())
            {
                try
                {
                    OUString aCurTxt( xFlatPara->getText() );
                    lang::Locale aCurLocale = lcl_GetPrimaryLanguageOfSentence( xFlatPara, aFPEntryItem.m_nStartIndex );

                    const bool bModified = xFlatPara->isModified();
                    if (!bModified)
                    {
                        linguistic2::ProofreadingResult aRes;

                        // ---- THREAD SAFE START ----
                        {
                            osl::ClearableMutexGuard aGuard(MyMutex());

                            sal_Int32 nStartPos = aFPEntryItem.m_nStartIndex;
                            sal_Int32 nSuggestedEnd
                                = GetSuggestedEndOfSentence(aCurTxt, nStartPos, aCurLocale);
                            DBG_ASSERT((nSuggestedEnd == 0 && aCurTxt.isEmpty())
                                           || nSuggestedEnd > nStartPos,
                                       "nSuggestedEndOfSentencePos calculation failed?");

                            uno::Reference<linguistic2::XProofreader> xGC =
                                GetGrammarChecker(aCurLocale);
                            if (xGC.is())
                            {
                                aGuard.clear();
                                uno::Sequence<beans::PropertyValue> const aProps(
                                    lcl_makeProperties(xFlatPara, PROOFINFO_MARK_PARAGRAPH));
                                aRes = xGC->doProofreading(aCurDocId, aCurTxt, aCurLocale,
                                                           nStartPos, nSuggestedEnd, aProps);

                                //!! work-around to prevent looping if the grammar checker
                                //!! failed to properly identify the sentence end
                                if (aRes.nBehindEndOfSentencePosition <= nStartPos
                                    && aRes.nBehindEndOfSentencePosition != nSuggestedEnd)
                                {
                                    SAL_WARN(
                                        "linguistic",
                                        "!! Grammarchecker failed to provide end of sentence !!");
                                    aRes.nBehindEndOfSentencePosition = nSuggestedEnd;
                                }

                                aRes.xFlatParagraph = std::move(xFlatPara);
                                aRes.nStartOfSentencePosition = nStartPos;
                            }
                            else
                            {
                                // no grammar checker -> no error
                                // but we need to provide the data below in order to continue with the next sentence
                                aRes.aDocumentIdentifier = aCurDocId;
                                aRes.xFlatParagraph = std::move(xFlatPara);
                                aRes.aText = aCurTxt;
                                aRes.aLocale = std::move(aCurLocale);
                                aRes.nStartOfSentencePosition = nStartPos;
                                aRes.nBehindEndOfSentencePosition = nSuggestedEnd;
                            }
                            aRes.nStartOfNextSentencePosition
                                = lcl_SkipWhiteSpaces(aCurTxt, aRes.nBehindEndOfSentencePosition);
                            aRes.nBehindEndOfSentencePosition = lcl_BacktraceWhiteSpaces(
                                aCurTxt, aRes.nStartOfNextSentencePosition);

                            //guard has to be cleared as ProcessResult calls out of this class
                        }
                        // ---- THREAD SAFE END ----
                        ProcessResult( aRes, xFPIterator, aFPEntryItem.m_bAutomatic );
                    }
                    else
                    {
                        // the paragraph changed meanwhile... (and maybe is still edited)
                        // thus we simply continue to ask for the next to be checked.
                        uno::Reference< text::XFlatParagraph > xFlatParaNext( xFPIterator->getNextPara() );
                        AddEntry( xFPIterator, xFlatParaNext, aCurDocId, 0, aFPEntryItem.m_bAutomatic );
                    }
                }
                catch (css::uno::Exception &)
                {
                    TOOLS_WARN_EXCEPTION("linguistic", "GrammarCheckingIterator::DequeueAndCheck ignoring");
                }
            }

            // ---- THREAD SAFE START ----
            {
                ::osl::Guard< ::osl::Mutex > aGuard( MyMutex() );
                m_aCurCheckedDocId.clear();
            }
            // ---- THREAD SAFE END ----
        }
        else
        {
            // ---- THREAD SAFE START ----
            {
                ::osl::Guard< ::osl::Mutex > aGuard( MyMutex() );
                if (m_bEnd)
                {
                    break;
                }
                // Check queue state again
                if (m_aFPEntriesQueue.empty())
                    m_aWakeUpThread.reset();
            }
            // ---- THREAD SAFE END ----

            //if the queue is empty
            // IMPORTANT: Don't call condition.wait() with locked
            // mutex. Otherwise you would keep out other threads
            // to add entries to the queue! A condition is thread-
            // safe implemented.
            m_aWakeUpThread.wait();
        }
    }
}


void SAL_CALL GrammarCheckingIterator::startProofreading(
    const uno::Reference< ::uno::XInterface > & xDoc,
    const uno::Reference< text::XFlatParagraphIteratorProvider > & xIteratorProvider )
{
    // get paragraph to start checking with
    const bool bAutomatic = true;
    uno::Reference<text::XFlatParagraphIterator> xFPIterator = xIteratorProvider->getFlatParagraphIterator(
            text::TextMarkupType::PROOFREADING, bAutomatic );
    uno::Reference< text::XFlatParagraph > xPara( xFPIterator.is()? xFPIterator->getFirstPara() : nullptr );
    uno::Reference< lang::XComponent > xComponent( xDoc, uno::UNO_QUERY );

    // ---- THREAD SAFE START ----
    ::osl::Guard< ::osl::Mutex > aGuard( MyMutex() );
    if (xPara.is() && xComponent.is())
    {
        OUString aDocId = GetOrCreateDocId( xComponent );

        // create new entry and add it to queue
        AddEntry( xFPIterator, xPara, aDocId, 0, bAutomatic );
    }
    // ---- THREAD SAFE END ----
}


linguistic2::ProofreadingResult SAL_CALL GrammarCheckingIterator::checkSentenceAtPosition(
    const uno::Reference< uno::XInterface >& xDoc,
    const uno::Reference< text::XFlatParagraph >& xFlatPara,
    const OUString& rText,
    const lang::Locale&,
    sal_Int32 nStartOfSentencePos,
    sal_Int32 nSuggestedEndOfSentencePos,
    sal_Int32 nErrorPosInPara )
{
    // for the context menu...

    uno::Reference< lang::XComponent > xComponent( xDoc, uno::UNO_QUERY );
    const bool bDoCheck = (xFlatPara.is() && xComponent.is() &&
        ( nErrorPosInPara < 0 || nErrorPosInPara < rText.getLength()));

    if (!bDoCheck)
        return linguistic2::ProofreadingResult();

    // iterate through paragraph until we find the sentence we are interested in
    linguistic2::ProofreadingResult aTmpRes;
    sal_Int32 nStartPos = nStartOfSentencePos >= 0 ? nStartOfSentencePos : 0;

    bool bFound = false;
    do
    {
        lang::Locale aCurLocale = lcl_GetPrimaryLanguageOfSentence( xFlatPara, nStartPos );
        sal_Int32 nOldStartOfSentencePos = nStartPos;
        uno::Reference< linguistic2::XProofreader > xGC;
        OUString aDocId;

        // ---- THREAD SAFE START ----
        {
            ::osl::Guard< ::osl::Mutex > aGuard( MyMutex() );
            aDocId = GetOrCreateDocId( xComponent );
            nSuggestedEndOfSentencePos = GetSuggestedEndOfSentence( rText, nStartPos, aCurLocale );
            DBG_ASSERT( nSuggestedEndOfSentencePos > nStartPos, "nSuggestedEndOfSentencePos calculation failed?" );

            xGC = GetGrammarChecker( aCurLocale );
        }
        // ---- THREAD SAFE START ----
        sal_Int32 nEndPos = -1;
        if (xGC.is())
        {
            uno::Sequence<beans::PropertyValue> const aProps(
                    lcl_makeProperties(xFlatPara, PROOFINFO_GET_PROOFRESULT));
            aTmpRes = xGC->doProofreading( aDocId, rText,
                aCurLocale, nStartPos, nSuggestedEndOfSentencePos, aProps );

            //!! work-around to prevent looping if the grammar checker
            //!! failed to properly identify the sentence end
            if (aTmpRes.nBehindEndOfSentencePosition <= nStartPos)
            {
                SAL_WARN( "linguistic", "!! Grammarchecker failed to provide end of sentence !!" );
                aTmpRes.nBehindEndOfSentencePosition = nSuggestedEndOfSentencePos;
            }

            aTmpRes.xFlatParagraph           = xFlatPara;
            aTmpRes.nStartOfSentencePosition = nStartPos;
            nEndPos = aTmpRes.nBehindEndOfSentencePosition;

            if ((nErrorPosInPara< 0 || nStartPos <= nErrorPosInPara) && nErrorPosInPara < nEndPos)
                bFound = true;
        }
        if (nEndPos == -1) // no result from grammar checker
            nEndPos = nSuggestedEndOfSentencePos;
        nStartPos = lcl_SkipWhiteSpaces( rText, nEndPos );
        aTmpRes.nBehindEndOfSentencePosition = nEndPos;
        aTmpRes.nStartOfNextSentencePosition = nStartPos;
        aTmpRes.nBehindEndOfSentencePosition = lcl_BacktraceWhiteSpaces( rText, aTmpRes.nStartOfNextSentencePosition );

        // prevent endless loop by forcefully advancing if needs be...
        if (nStartPos <= nOldStartOfSentencePos)
        {
            SAL_WARN( "linguistic", "end-of-sentence detection failed?" );
            nStartPos = nOldStartOfSentencePos + 1;
        }
    }
    while (!bFound && nStartPos < rText.getLength());

    if (bFound && !xFlatPara->isModified())
        return aTmpRes;

    return linguistic2::ProofreadingResult();
}

sal_Int32 GrammarCheckingIterator::GetSuggestedEndOfSentence(
    const OUString &rText,
    sal_Int32 nSentenceStartPos,
    const lang::Locale &rLocale )
{
    // internal method; will always be called with locked mutex

    if (!m_xBreakIterator.is())
    {
        const uno::Reference< uno::XComponentContext >& xContext = ::comphelper::getProcessComponentContext();
        m_xBreakIterator = i18n::BreakIterator::create(xContext);
    }
    sal_Int32 nTextLen = rText.getLength();
    sal_Int32 nEndPosition(0);
    sal_Int32 nTmpStartPos = nSentenceStartPos;
    do
    {
        sal_Int32 const nPrevEndPosition(nEndPosition);
        nEndPosition = nTextLen;
        if (nTmpStartPos < nTextLen)
        {
            nEndPosition = m_xBreakIterator->endOfSentence( rText, nTmpStartPos, rLocale );
            if (nEndPosition <= nPrevEndPosition)
            {
                // fdo#68750 if there's no progress at all then presumably
                // there's no end of sentence in this paragraph so just
                // set the end position to end of paragraph
                nEndPosition = nTextLen;
            }
        }
        if (nEndPosition < 0)
            nEndPosition = nTextLen;

        ++nTmpStartPos;
    }
    while (nEndPosition <= nSentenceStartPos && nEndPosition < nTextLen);
    if (nEndPosition > nTextLen)
        nEndPosition = nTextLen;
    return nEndPosition;
}


void SAL_CALL GrammarCheckingIterator::resetIgnoreRules(  )
{
    for (auto const& elem : m_aGCReferencesByService)
    {
        uno::Reference< linguistic2::XProofreader > xGC(elem.second);
        if (xGC.is())
            xGC->resetIgnoreRules();
    }
}


sal_Bool SAL_CALL GrammarCheckingIterator::isProofreading(
    const uno::Reference< uno::XInterface >& xDoc )
{
    // ---- THREAD SAFE START ----
    ::osl::Guard< ::osl::Mutex > aGuard( MyMutex() );

    bool bRes = false;

    uno::Reference< lang::XComponent > xComponent( xDoc, uno::UNO_QUERY );
    if (xComponent.is())
    {
        // if the component was already used in one of the two calls to check text
        // i.e. in startGrammarChecking or checkGrammarAtPos it will be found in the
        // m_aDocIdMap unless the document already disposed.
        // If it is not found then it is not yet being checked (or requested to being checked)
        const DocMap_t::const_iterator aIt( m_aDocIdMap.find( xComponent.get() ) );
        if (aIt != m_aDocIdMap.end())
        {
            // check in document is checked automatically in the background...
            OUString aDocId = aIt->second;
            if (!m_aCurCheckedDocId.isEmpty() && m_aCurCheckedDocId == aDocId)
            {
                // an entry for that document was dequeued and is currently being checked.
                bRes = true;
            }
            else
            {
                // we need to check if there is an entry for that document in the queue...
                // That is the document is going to be checked sooner or later.

                sal_Int32 nSize = m_aFPEntriesQueue.size();
                for (sal_Int32 i = 0; i < nSize && !bRes; ++i)
                {
                    if (aDocId == m_aFPEntriesQueue[i].m_aDocId)
                        bRes = true;
                }
            }
        }
    }
    // ---- THREAD SAFE END ----

    return bRes;
}


void SAL_CALL GrammarCheckingIterator::processLinguServiceEvent(
    const linguistic2::LinguServiceEvent& rLngSvcEvent )
{
    if (rLngSvcEvent.nEvent != linguistic2::LinguServiceEventFlags::PROOFREAD_AGAIN)
        return;

    try
    {
         uno::Reference< uno::XInterface > xThis( getXWeak() );
         linguistic2::LinguServiceEvent aEvent( xThis, linguistic2::LinguServiceEventFlags::PROOFREAD_AGAIN );
         m_aNotifyListeners.notifyEach(
                &linguistic2::XLinguServiceEventListener::processLinguServiceEvent,
                aEvent);
    }
    catch (uno::RuntimeException &)
    {
         throw;
    }
    catch (const ::uno::Exception &)
    {
        // ignore
        TOOLS_WARN_EXCEPTION("linguistic", "processLinguServiceEvent");
    }
}


sal_Bool SAL_CALL GrammarCheckingIterator::addLinguServiceEventListener(
    const uno::Reference< linguistic2::XLinguServiceEventListener >& xListener )
{
    if (xListener.is())
    {
        m_aNotifyListeners.addInterface( xListener );
    }
    return true;
}


sal_Bool SAL_CALL GrammarCheckingIterator::removeLinguServiceEventListener(
    const uno::Reference< linguistic2::XLinguServiceEventListener >& xListener )
{
    if (xListener.is())
    {
        m_aNotifyListeners.removeInterface( xListener );
    }
    return true;
}


void SAL_CALL GrammarCheckingIterator::dispose()
{
    lang::EventObject aEvt( static_cast<linguistic2::XProofreadingIterator *>(this) );
    m_aEventListeners.disposeAndClear( aEvt );

    TerminateThread();

    // ---- THREAD SAFE START ----
    {
        ::osl::Guard< ::osl::Mutex > aGuard( MyMutex() );

        // release all UNO references

        m_xBreakIterator.clear();

        // clear containers with UNO references AND have those references released
        GCReferences_t  aTmpEmpty1;
        DocMap_t        aTmpEmpty2;
        FPQueue_t       aTmpEmpty3;
        m_aGCReferencesByService.swap( aTmpEmpty1 );
        m_aDocIdMap.swap( aTmpEmpty2 );
        m_aFPEntriesQueue.swap( aTmpEmpty3 );
    }
    // ---- THREAD SAFE END ----
}


void SAL_CALL GrammarCheckingIterator::addEventListener(
    const uno::Reference< lang::XEventListener >& xListener )
{
    if (xListener.is())
    {
        m_aEventListeners.addInterface( xListener );
    }
}


void SAL_CALL GrammarCheckingIterator::removeEventListener(
    const uno::Reference< lang::XEventListener >& xListener )
{
    if (xListener.is())
    {
        m_aEventListeners.removeInterface( xListener );
    }
}


void SAL_CALL GrammarCheckingIterator::disposing( const lang::EventObject &rSource )
{
    // if the component (document) is disposing release all references
    //!! There is no need to remove entries from the queue that are from this document
    //!! since the respectives xFlatParagraphs should become invalid (isModified() == true)
    //!! and the call to xFlatParagraphIterator->getNextPara() will result in an empty reference.
    //!! And if an entry is currently checked by a grammar checker upon return the results
    //!! should be ignored.
    //!! Also GetOrCreateDocId will not use that very same Id again...
    //!! All of the above resulting in that we only have to get rid of the implementation pointer here.
    uno::Reference< lang::XComponent > xDoc( rSource.Source, uno::UNO_QUERY );
    if (xDoc.is())
    {
        // ---- THREAD SAFE START ----
        ::osl::Guard< ::osl::Mutex > aGuard( MyMutex() );
        m_aDocIdMap.erase( xDoc.get() );
        // ---- THREAD SAFE END ----
    }
}


uno::Reference< util::XChangesBatch > const & GrammarCheckingIterator::GetUpdateAccess() const
{
    if (!m_xUpdateAccess.is())
    {
        try
        {
            // get configuration provider
            const uno::Reference< uno::XComponentContext >& xContext = comphelper::getProcessComponentContext();
            uno::Reference< lang::XMultiServiceFactory > xConfigurationProvider =
                    configuration::theDefaultProvider::get( xContext );

            // get configuration update access
            beans::PropertyValue aValue;
            aValue.Name  = "nodepath";
            aValue.Value <<= u"org.openoffice.Office.Linguistic/ServiceManager"_ustr;
            uno::Sequence< uno::Any > aProps{ uno::Any(aValue) };
            m_xUpdateAccess.set(
                    xConfigurationProvider->createInstanceWithArguments(
                        u"com.sun.star.configuration.ConfigurationUpdateAccess"_ustr, aProps ),
                        uno::UNO_QUERY_THROW );
        }
        catch (uno::Exception &)
        {
        }
    }

    return m_xUpdateAccess;
}


void GrammarCheckingIterator::GetConfiguredGCSvcs_Impl()
{
    GCImplNames_t   aTmpGCImplNamesByLang;

    try
    {
        // get node names (locale iso strings) for configured grammar checkers
        uno::Reference< container::XNameAccess > xNA( GetUpdateAccess(), uno::UNO_QUERY_THROW );
        xNA.set( xNA->getByName( u"GrammarCheckerList"_ustr ), uno::UNO_QUERY_THROW );
        const uno::Sequence< OUString > aElementNames( xNA->getElementNames() );

        for (const OUString& rElementName : aElementNames)
        {
            uno::Sequence< OUString > aImplNames;
            uno::Any aTmp( xNA->getByName( rElementName ) );
            if (aTmp >>= aImplNames)
            {
                if (aImplNames.hasElements())
                {
                    // only the first entry is used, there should be only one grammar checker per language
                    aTmpGCImplNamesByLang[rElementName] = aImplNames[0];
                }
            }
            else
            {
                SAL_WARN( "linguistic", "failed to get aImplNames. Wrong type?" );
            }
        }
    }
    catch (uno::Exception const &)
    {
        TOOLS_WARN_EXCEPTION( "linguistic", "exception caught. Failed to get configured services" );
    }

    {
        // ---- THREAD SAFE START ----
        ::osl::Guard< ::osl::Mutex > aGuard( MyMutex() );
        m_aGCImplNamesByLang.swap(aTmpGCImplNamesByLang);
        // ---- THREAD SAFE END ----
    }
}


sal_Bool SAL_CALL GrammarCheckingIterator::supportsService(
    const OUString & rServiceName )
{
    return cppu::supportsService(this, rServiceName);
}


OUString SAL_CALL GrammarCheckingIterator::getImplementationName(  )
{
    return u"com.sun.star.lingu2.ProofreadingIterator"_ustr;
}


uno::Sequence< OUString > SAL_CALL GrammarCheckingIterator::getSupportedServiceNames(  )
{
    return  { u"com.sun.star.linguistic2.ProofreadingIterator"_ustr };
}


void GrammarCheckingIterator::SetServiceList(
    const lang::Locale &rLocale,
    const uno::Sequence< OUString > &rSvcImplNames )
{
    ::osl::Guard< ::osl::Mutex > aGuard( MyMutex() );

    OUString sBcp47 = LanguageTag::convertToBcp47(rLocale, false);
    OUString aImplName;
    if (rSvcImplNames.hasElements())
        aImplName = rSvcImplNames[0];   // there is only one grammar checker per language

    if (!LinguIsUnspecified(sBcp47) && !sBcp47.isEmpty())
    {
        if (!aImplName.isEmpty())
            m_aGCImplNamesByLang[sBcp47] = aImplName;
        else
            m_aGCImplNamesByLang.erase(sBcp47);
    }
}


uno::Sequence< OUString > GrammarCheckingIterator::GetServiceList(
    const lang::Locale &rLocale ) const
{
    ::osl::Guard< ::osl::Mutex > aGuard( MyMutex() );

    const OUString aImplName = getServiceForLocale(rLocale).first;     // there is only one grammar checker per language

    if (!aImplName.isEmpty())
        return { aImplName };
    return {};
}


extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
linguistic_GrammarCheckingIterator_get_implementation(
    css::uno::XComponentContext* , css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new GrammarCheckingIterator());
}



/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
