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

#include <cstdlib>
#include <string_view>

#include <i18nlangtag/languagetag.hxx>
#include <i18nutil/searchopt.hxx>
#include <i18nutil/transliteration.hxx>
#include <com/sun/star/util/TextSearch2.hpp>
#include <com/sun/star/util/SearchAlgorithms2.hpp>
#include <com/sun/star/util/SearchFlags.hpp>
#include <unotools/charclass.hxx>
#include <comphelper/processfactory.hxx>
#include <unotools/textsearch.hxx>
#include <rtl/ustrbuf.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <mutex>

using namespace ::com::sun::star::util;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;

namespace utl
{

SearchParam::SearchParam( const OUString &rText,
                                SearchType eType,
                                bool bCaseSensitive,
                                sal_uInt32 cWildEscChar,
                                bool bWildMatchSel )
{
    sSrchStr        = rText;
    m_eSrchType     = eType;

    m_cWildEscChar  = cWildEscChar;

    m_bCaseSense    = bCaseSensitive;
    m_bWildMatchSel = bWildMatchSel;
}

SearchParam::SearchParam( const SearchParam& rParam )
{
    sSrchStr        = rParam.sSrchStr;
    m_eSrchType     = rParam.m_eSrchType;

    m_cWildEscChar  = rParam.m_cWildEscChar;

    m_bCaseSense    = rParam.m_bCaseSense;
    m_bWildMatchSel = rParam.m_bWildMatchSel;
}

SearchParam::~SearchParam() {}

static bool lcl_Equals( const i18nutil::SearchOptions2& rSO1, const i18nutil::SearchOptions2& rSO2 )
{
    return
        rSO1.AlgorithmType2 == rSO2.AlgorithmType2 &&
        rSO1.WildcardEscapeCharacter == rSO2.WildcardEscapeCharacter &&
        rSO1.searchFlag == rSO2.searchFlag &&
        rSO1.searchString == rSO2.searchString &&
        rSO1.replaceString == rSO2.replaceString &&
        rSO1.changedChars == rSO2.changedChars &&
        rSO1.deletedChars == rSO2.deletedChars &&
        rSO1.insertedChars == rSO2.insertedChars &&
        rSO1.Locale.Language == rSO2.Locale.Language &&
        rSO1.Locale.Country == rSO2.Locale.Country &&
        rSO1.Locale.Variant == rSO2.Locale.Variant &&
        rSO1.transliterateFlags == rSO2.transliterateFlags;
}

namespace
{
    struct CachedTextSearch
    {
        std::mutex mutex;
        i18nutil::SearchOptions2 Options;
        css::uno::Reference< css::util::XTextSearch2 > xTextSearch;
    };
}

Reference<XTextSearch2> TextSearch::getXTextSearch( const i18nutil::SearchOptions2& rPara )
{
    static CachedTextSearch theCachedTextSearch;

    std::scoped_lock aGuard(theCachedTextSearch.mutex);

    if ( lcl_Equals(theCachedTextSearch.Options, rPara) )
        return theCachedTextSearch.xTextSearch;

    const Reference< XComponentContext >& xContext = ::comphelper::getProcessComponentContext();
    theCachedTextSearch.xTextSearch.set( ::TextSearch2::create(xContext) );
    theCachedTextSearch.xTextSearch->setOptions2( rPara.toUnoSearchOptions2() );
    theCachedTextSearch.Options = rPara;

    return theCachedTextSearch.xTextSearch;
}

TextSearch::TextSearch(const SearchParam & rParam, LanguageType eLang )
{
    if( LANGUAGE_NONE == eLang )
        eLang = LANGUAGE_SYSTEM;
    css::lang::Locale aLocale( LanguageTag::convertToLocale( eLang ) );

    Init( rParam, aLocale);
}

TextSearch::TextSearch(const SearchParam & rParam, const CharClass& rCClass )
{
    Init( rParam, rCClass.getLanguageTag().getLocale() );
}

TextSearch::TextSearch( const i18nutil::SearchOptions2& rPara )
{
    xTextSearch = getXTextSearch( rPara );
}

void TextSearch::Init( const SearchParam & rParam,
                        const css::lang::Locale& rLocale )
{
    // convert SearchParam to the UNO SearchOptions2
    i18nutil::SearchOptions2 aSOpt;

    switch( rParam.GetSrchType() )
    {
    case SearchParam::SearchType::Wildcard:
        aSOpt.AlgorithmType2 = SearchAlgorithms2::WILDCARD;
        aSOpt.WildcardEscapeCharacter = rParam.GetWildEscChar();
        if (rParam.IsWildMatchSel())
            aSOpt.searchFlag |= SearchFlags::WILD_MATCH_SELECTION;
        break;

    case SearchParam::SearchType::Regexp:
        aSOpt.AlgorithmType2 = SearchAlgorithms2::REGEXP;
        break;

    case SearchParam::SearchType::Normal:
        aSOpt.AlgorithmType2 = SearchAlgorithms2::ABSOLUTE;
        break;

    default:
        for (;;) std::abort();
    }
    aSOpt.searchString = rParam.GetSrchStr();
    aSOpt.replaceString = "";
    aSOpt.Locale = rLocale;
    aSOpt.transliterateFlags = TransliterationFlags::NONE;
    if( !rParam.IsCaseSensitive() )
    {
        aSOpt.searchFlag |= SearchFlags::ALL_IGNORE_CASE;
        aSOpt.transliterateFlags |= TransliterationFlags::IGNORE_CASE;
    }

    xTextSearch = getXTextSearch( aSOpt );
}

void TextSearch::SetLocale( const i18nutil::SearchOptions2& rOptions,
                            const css::lang::Locale& rLocale )
{
    i18nutil::SearchOptions2 aSOpt( rOptions );
    aSOpt.Locale = rLocale;

    xTextSearch = getXTextSearch( aSOpt );
}

TextSearch::~TextSearch()
{
}

/*
 * General search methods. These methods will call the respective
 * methods, such as ordinary string searching or regular expression
 * matching, using the method pointer.
 */
bool TextSearch::SearchForward( const OUString &rStr,
                    sal_Int32* pStart, sal_Int32* pEnd,
                    css::util::SearchResult* pRes)
{
    bool bRet = false;
    try
    {
        if( xTextSearch.is() )
        {
            SearchResult aRet( xTextSearch->searchForward( rStr, *pStart, *pEnd ));
            if( aRet.subRegExpressions > 0 )
            {
                bRet = true;
                // the XTextsearch returns in startOffset the higher position
                // and the endposition is always exclusive.
                // The caller of this function will have in startPos the
                // lower pos. and end
                *pStart = aRet.startOffset[ 0 ];
                *pEnd = aRet.endOffset[ 0 ];
                if( pRes )
                    *pRes = std::move(aRet);
            }
        }
    }
    catch ( Exception& )
    {
        TOOLS_WARN_EXCEPTION("unotools.i18n", "" );
    }
    return bRet;
}

bool TextSearch::searchForward( const OUString &rStr )
{
    sal_Int32 pStart = 0;
    sal_Int32 pEnd = rStr.getLength();

    bool bResult = SearchForward(rStr, &pStart, &pEnd);

    return bResult;
}

bool TextSearch::SearchBackward( const OUString & rStr, sal_Int32* pStart,
                                sal_Int32* pEnd, SearchResult* pRes )
{
    bool bRet = false;
    try
    {
        if( xTextSearch.is() )
        {
            SearchResult aRet( xTextSearch->searchBackward( rStr, *pStart, *pEnd ));
            if( aRet.subRegExpressions )
            {
                bRet = true;
                // the XTextsearch returns in startOffset the higher position
                // and the endposition is always exclusive.
                // The caller of this function will have in startPos the
                // lower pos. and end
                *pEnd = aRet.startOffset[ 0 ];
                *pStart = aRet.endOffset[ 0 ];
                if( pRes )
                    *pRes = std::move(aRet);
            }
        }
    }
    catch ( Exception& )
    {
        TOOLS_WARN_EXCEPTION("unotools.i18n", "" );
    }
    return bRet;
}

// static
void TextSearch::ReplaceBackReferences( OUString& rReplaceStr, std::u16string_view rStr, const SearchResult& rResult )
{
    if( rResult.subRegExpressions <= 0 )
        return;

    sal_Unicode sFndChar;
    sal_Int32 i;
    OUStringBuffer sBuff(rReplaceStr.getLength()*4);
    for(i = 0; i < rReplaceStr.getLength(); i++)
    {
        if( rReplaceStr[i] == '&')
        {
            sal_Int32 nStart = rResult.startOffset[0];
            sal_Int32 nLength = rResult.endOffset[0] - rResult.startOffset[0];
            sBuff.append(rStr.substr(nStart, nLength));
        }
        else if((i < rReplaceStr.getLength() - 1) && rReplaceStr[i] == '$')
        {
            sFndChar = rReplaceStr[ i + 1 ];
            switch(sFndChar)
            {   // placeholder for a backward reference?
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                {
                    int j = sFndChar - '0'; // index
                    if(j < rResult.subRegExpressions)
                    {
                        sal_Int32 nSttReg = rResult.startOffset[j];
                        sal_Int32 nRegLen = rResult.endOffset[j];
                        if (nSttReg < 0 || nRegLen < 0) // A "not found" optional capture
                        {
                            nSttReg = nRegLen = 0; // Copy empty string
                        }
                        else if (nRegLen >= nSttReg)
                        {
                            nRegLen = nRegLen - nSttReg;
                        }
                        else
                        {
                            nRegLen = nSttReg - nRegLen;
                            nSttReg = rResult.endOffset[j];
                        }
                        // Copy reference from found string
                        sBuff.append(rStr.substr(nSttReg, nRegLen));
                    }
                    i += 1;
                }
                break;
            default:
                sBuff.append(OUStringChar(rReplaceStr[i]) + OUStringChar(rReplaceStr[i+1]));
                i += 1;
                break;
            }
        }
        else if((i < rReplaceStr.getLength() - 1) && rReplaceStr[i] == '\\')
        {
            sFndChar = rReplaceStr[ i+1 ];
            switch(sFndChar)
            {
            case '\\':
            case '&':
            case '$':
                sBuff.append(sFndChar);
                i+=1;
                break;
            case 't':
                sBuff.append('\t');
                i += 1;
                break;
            default:
                sBuff.append(OUStringChar(rReplaceStr[i]) + OUStringChar(rReplaceStr[i+1]));
                i += 1;
                break;
            }
        }
        else
        {
            sBuff.append(rReplaceStr[i]);
        }
    }
    rReplaceStr = sBuff.makeStringAndClear();
}

bool TextSearch::SimilaritySearch(const OUString& rString, const OUString& rSearchString,
                                  ::std::pair<sal_Int32, sal_Int32>& rSimilarityScore)
{
    sal_Int32 nScore = 0;
    sal_Int32 nFirstScore = GetSubstringSimilarity(rString, rSearchString, nScore, true);
    if (nFirstScore == -1)
        nFirstScore = GetSubstringSimilarity(rString, rSearchString, nScore, false);
    if (nFirstScore == -1)
    {
        if (rSearchString.getLength() == 1)
        {
            if (rString.startsWith(rSearchString))
                nFirstScore = nScore;
            else if (rString.endsWith(rSearchString))
                nFirstScore = nScore + 1;
            nScore += 2;
        }
        else if (rString.getLength() == 1 && rSearchString.getLength() < SMALL_STRING_THRESHOLD)
        {
            if (rSearchString.startsWith(rString))
                nFirstScore = nScore;
            else if (rSearchString.endsWith(rString))
                nFirstScore = nScore + 1;
            nScore += 2;
        }
    }
    sal_Int32 nSecondScore = GetWeightedLevenshteinDistance(rString, rSearchString);

    if (nFirstScore == -1 && nSecondScore >= WLD_THRESHOLD)
        return false;

    rSimilarityScore.first = (nFirstScore == -1) ? nScore : nFirstScore;
    rSimilarityScore.second = nSecondScore;
    return true;
}

sal_Int32 TextSearch::GetSubstringSimilarity(std::u16string_view rString,
                                             std::u16string_view rSearchString,
                                             sal_Int32& nInitialScore, const bool bFromStart)
{
    sal_Int32 nScore = -1;
    for (sal_Int32 length = rSearchString.length(); length > 1; length--)
    {
        sal_Int32 nStartPos = bFromStart ? 0 : rSearchString.length() - length;
        std::u16string_view rSearchSubString = rSearchString.substr(nStartPos, length);
        if (rString.starts_with(rSearchSubString))
        {
            nScore = nInitialScore;
            break;
        }
        else if (rString.ends_with(rSearchSubString))
        {
            nScore = nInitialScore + 1;
            break;
        }
        else if (rString.find(rSearchSubString) != std::u16string_view::npos)
        {
            nScore = nInitialScore + 2;
            break;
        }
        nInitialScore += 3;
    }
    return nScore;
}

sal_Int32 TextSearch::GetWeightedLevenshteinDistance(const OUString& rString,
                                                     const OUString& rSearchString)
{
    sal_Int32 n = rString.getLength();
    sal_Int32 m = rSearchString.getLength();
    std::vector<std::vector<sal_Int32>> ScoreDP(n + 1, std::vector<sal_Int32>(m + 1));

    for (sal_Int32 i = 0; i <= n; i++)
    {
        ScoreDP[i][0] = i;
    }
    for (sal_Int32 j = 0; j <= m; j++)
    {
        ScoreDP[0][j] = j;
    }

    for (sal_Int32 i = 1; i <= n; i++)
    {
        for (sal_Int32 j = 1; j <= m; j++)
        {
            sal_Int32& minE = ScoreDP[i][j];
            minE = ScoreDP[i - 1][j] + 1;
            minE = std::min(minE, ScoreDP[i][j - 1] + 1);
            if (rString[i - 1] != rSearchString[j - 1])
                minE = std::min(minE, ScoreDP[i - 1][j - 1] + 2);
            else
                minE = std::min(minE, ScoreDP[i - 1][j - 1]);
        }
    }
    return ScoreDP[n][m];
}

}   // namespace utl

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
