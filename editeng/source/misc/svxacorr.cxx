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

#include <memory>
#include <utility>
#include <algorithm>
#include <string_view>
#include <sal/config.h>

#include <com/sun/star/linguistic2/XSpellChecker1.hpp>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/io/XStream.hpp>
#include <tools/urlobj.hxx>
#include <i18nlangtag/mslangid.hxx>
#include <i18nutil/transliteration.hxx>
#include <sal/log.hxx>
#include <osl/diagnose.h>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>
#include <svl/fstathelper.hxx>
#include <svl/urihelper.hxx>
#include <unotools/charclass.hxx>
#include <com/sun/star/i18n/UnicodeType.hpp>
#include <unotools/collatorwrapper.hxx>
#include <com/sun/star/i18n/UnicodeScript.hpp>
#include <com/sun/star/i18n/OrdinalSuffix.hpp>
#include <unotools/localedatawrapper.hxx>
#include <unotools/transliterationwrapper.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/sequence.hxx>
#include <comphelper/storagehelper.hxx>
#include <o3tl/string_view.hxx>
#include <editeng/editids.hrc>
#include <sot/storage.hxx>
#include <editeng/udlnitem.hxx>
#include <editeng/wghtitem.hxx>
#include <editeng/postitem.hxx>
#include <editeng/crossedoutitem.hxx>
#include <editeng/escapementitem.hxx>
#include <editeng/svxacorr.hxx>
#include <editeng/unolingu.hxx>
#include <vcl/window.hxx>
#include <com/sun/star/xml/sax/InputSource.hpp>
#include <com/sun/star/xml/sax/FastParser.hpp>
#include <com/sun/star/xml/sax/Writer.hpp>
#include <com/sun/star/xml/sax/SAXParseException.hpp>
#include <unotools/streamwrap.hxx>
#include "SvXMLAutoCorrectImport.hxx"
#include "SvXMLAutoCorrectExport.hxx"
#include "SvXMLAutoCorrectTokenHandler.hxx"
#include <ucbhelper/content.hxx>
#include <com/sun/star/ucb/ContentCreationException.hpp>
#include <com/sun/star/ucb/XCommandEnvironment.hpp>
#include <com/sun/star/ucb/TransferInfo.hpp>
#include <com/sun/star/ucb/NameClash.hpp>
#include <comphelper/diagnose_ex.hxx>
#include <xmloff/xmltoken.hxx>
#include <unordered_map>
#include <rtl/character.hxx>

using namespace ::com::sun::star::ucb;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::xml::sax;
using namespace ::com::sun::star;
using namespace ::xmloff::token;
using namespace ::utl;

namespace {

enum class Flags {
    NONE            = 0x00,
    FullStop        = 0x01,
    ExclamationMark = 0x02,
    QuestionMark    = 0x04,
};

}

namespace o3tl {
    template<> struct typed_flags<Flags> : is_typed_flags<Flags, 0x07> {};
}
const sal_Unicode cNonBreakingSpace = 0xA0; // UNICODE code for no break space

constexpr OUString pXMLImplWordStart_ExcptLstStr = u"WordExceptList.xml"_ustr;
constexpr OUString pXMLImplCplStt_ExcptLstStr = u"SentenceExceptList.xml"_ustr;
constexpr OUString pXMLImplAutocorr_ListStr = u"DocumentList.xml"_ustr;

// tdf#54409 check also typographical quotation marks in the case of skipped ASCII quotation marks
// Curious, why these \u0083\u0084\u0089\u0091\u0092\u0093\u0094 are handled as "begin characters"?
constexpr std::u16string_view
    /* also at these beginnings - Brackets and all kinds of begin characters */
    sImplSttSkipChars = u"\"'([{\u2018\u2019\u201a\u201b\u201c\u201d\u201e\u201f\u0083\u0084\u0089\u0091\u0092\u0093\u0094",
    /* also at these ends - Brackets and all kinds of begin characters */
    sImplEndSkipChars = u"\"')]}\u2018\u2019\u201a\u201b\u201c\u201d\u201e\u201f\u0083\u0084\u0089\u0091\u0092\u0093\u0094";

static OUString EncryptBlockName_Imp(std::u16string_view rName);

static bool NonFieldWordDelim( const sal_Unicode c )
{
    return ' ' == c || '\t' == c || 0x0a == c ||
            cNonBreakingSpace == c || 0x2011 == c;
}

static bool IsWordDelim( const sal_Unicode c )
{
    return c == 0x1 || NonFieldWordDelim(c);
}


static bool IsLowerLetter( sal_Int32 nCharType )
{
    return CharClass::isLetterType( nCharType ) &&
           ( css::i18n::KCharacterType::LOWER & nCharType);
}

static bool IsUpperLetter( sal_Int32 nCharType )
{
    return CharClass::isLetterType( nCharType ) &&
            ( css::i18n::KCharacterType::UPPER & nCharType);
}

static bool lcl_IsUnsupportedUnicodeChar( CharClass const & rCC, const OUString& rTxt,
                                   sal_Int32 nStt, sal_Int32 nEnd )
{
    for( ; nStt < nEnd; ++nStt )
    {
        css::i18n::UnicodeScript nScript = rCC.getScript( rTxt, nStt );
        switch( nScript )
        {
            case css::i18n::UnicodeScript_kCJKRadicalsSupplement:
            case css::i18n::UnicodeScript_kHangulJamo:
            case css::i18n::UnicodeScript_kCJKSymbolPunctuation:
            case css::i18n::UnicodeScript_kHiragana:
            case css::i18n::UnicodeScript_kKatakana:
            case css::i18n::UnicodeScript_kHangulCompatibilityJamo:
            case css::i18n::UnicodeScript_kEnclosedCJKLetterMonth:
            case css::i18n::UnicodeScript_kCJKCompatibility:
            case css::i18n::UnicodeScript_kCJKUnifiedIdeographsExtensionA:
            case css::i18n::UnicodeScript_kCJKUnifiedIdeograph:
            case css::i18n::UnicodeScript_kHangulSyllable:
            case css::i18n::UnicodeScript_kCJKCompatibilityIdeograph:
            case css::i18n::UnicodeScript_kHalfwidthFullwidthForm:
                return true;
            default: ; //do nothing
        }
    }
    return false;
}

static bool lcl_IsSymbolChar( CharClass const & rCC, const OUString& rTxt,
                                  sal_Int32 nStt, sal_Int32 nEnd )
{
    for( ; nStt < nEnd; ++nStt )
    {
        if( css::i18n::UnicodeType::PRIVATE_USE == rCC.getType( rTxt, nStt ))
            return true;
    }
    return false;
}

static bool lcl_IsInArr(std::u16string_view arr, const sal_uInt32 c)
{
    return std::any_of(arr.begin(), arr.end(), [c](const auto c1) { return c1 == c; });
}

SvxAutoCorrDoc::~SvxAutoCorrDoc()
{
}

// Called by the functions:
//  - FnCapitalStartWord
//  - FnCapitalStartSentence
// after the exchange of characters. Then the words, if necessary, can be inserted
// into the exception list.
void SvxAutoCorrDoc::SaveCpltSttWord( ACFlags, sal_Int32, const OUString&,
                                        sal_Unicode )
{
}

LanguageType SvxAutoCorrDoc::GetLanguage( sal_Int32 ) const
{
    return LANGUAGE_SYSTEM;
}

static const LanguageTag& GetAppLang()
{
    return Application::GetSettings().GetLanguageTag();
}

/// Never use an unresolved LANGUAGE_SYSTEM.
static LanguageType GetDocLanguage( const SvxAutoCorrDoc& rDoc, sal_Int32 nPos )
{
    LanguageType eLang = rDoc.GetLanguage( nPos );
    if (eLang == LANGUAGE_SYSTEM)
        eLang = GetAppLang().getLanguageType();     // the current work locale
    return eLang;
}

static LocaleDataWrapper& GetLocaleDataWrapper( LanguageType nLang )
{
    static std::unique_ptr<LocaleDataWrapper> xLclDtWrp;
    LanguageTag aLcl( nLang );
    if (!xLclDtWrp || xLclDtWrp->getLoadedLanguageTag() != aLcl)
        xLclDtWrp.reset(new LocaleDataWrapper(std::move(aLcl)));
    return *xLclDtWrp;
}
static TransliterationWrapper& GetIgnoreTranslWrapper()
{
    static int bIsInit = 0;
    static TransliterationWrapper aWrp( ::comphelper::getProcessComponentContext(),
                TransliterationFlags::IGNORE_KANA |
                TransliterationFlags::IGNORE_WIDTH );
    if( !bIsInit )
    {
        aWrp.loadModuleIfNeeded( GetAppLang().getLanguageType() );
        bIsInit = 1;
    }
    return aWrp;
}
static CollatorWrapper& GetCollatorWrapper()
{
    static CollatorWrapper aCollWrp = []()
    {
        CollatorWrapper tmp( ::comphelper::getProcessComponentContext() );
        tmp.loadDefaultCollator( GetAppLang().getLocale(), 0 );
        return tmp;
    }();
    return aCollWrp;
}

bool SvxAutoCorrect::IsAutoCorrectChar( sal_Unicode cChar )
{
    return  cChar == '\0' || cChar == '\t' || cChar == 0x0a ||
            cChar == ' '  || cChar == '\'' || cChar == '\"' ||
            cChar == '*'  || cChar == '_'  || cChar == '%' ||
            cChar == '.'  || cChar == ','  || cChar == ';' ||
            cChar == ':'  || cChar == '?' || cChar == '!' ||
            cChar == '<'  || cChar == '>' ||
            cChar == '/'  || cChar == '-';
}

namespace
{
    bool IsCompoundWordDelimChar(sal_Unicode cChar)
    {
        return  cChar == '-' || SvxAutoCorrect::IsAutoCorrectChar(cChar);
    }
}

bool SvxAutoCorrect::NeedsHardspaceAutocorr( sal_Unicode cChar )
{
    return cChar == '%' || cChar == ';' || cChar == ':'  || cChar == '?' || cChar == '!' ||
        cChar == '/' /*case for the urls exception*/;
}

ACFlags SvxAutoCorrect::GetDefaultFlags()
{
    ACFlags nRet = ACFlags::Autocorrect
                    | ACFlags::CapitalStartSentence
                    | ACFlags::CapitalStartWord
                    | ACFlags::ChgOrdinalNumber
                    | ACFlags::ChgToEnEmDash
                    | ACFlags::AddNonBrkSpace
                    | ACFlags::TransliterateRTL
                    | ACFlags::ChgAngleQuotes
                    | ACFlags::ChgWeightUnderl
                    | ACFlags::SetINetAttr
                    | ACFlags::SetDOIAttr
                    | ACFlags::ChgQuotes
                    | ACFlags::SaveWordCplSttLst
                    | ACFlags::SaveWordWordStartLst
                    | ACFlags::CorrectCapsLock;
    LanguageType eLang = GetAppLang().getLanguageType();
    if( eLang.anyOf(
        LANGUAGE_ENGLISH,
        LANGUAGE_ENGLISH_US,
        LANGUAGE_ENGLISH_UK,
        LANGUAGE_ENGLISH_AUS,
        LANGUAGE_ENGLISH_CAN,
        LANGUAGE_ENGLISH_NZ,
        LANGUAGE_ENGLISH_EIRE,
        LANGUAGE_ENGLISH_SAFRICA,
        LANGUAGE_ENGLISH_JAMAICA,
        LANGUAGE_ENGLISH_CARIBBEAN))
        nRet &= ~ACFlags(ACFlags::ChgQuotes|ACFlags::ChgSglQuotes);
    return nRet;
}

constexpr sal_Unicode cEmDash = 0x2014;
constexpr sal_Unicode cEnDash = 0x2013;
constexpr OUString sEmDash(u"\u2014"_ustr);
constexpr OUString sEnDash(u"\u2013"_ustr);
constexpr sal_Unicode cApostrophe = 0x2019;
constexpr sal_Unicode cLeftDoubleAngleQuote = 0xAB;
constexpr sal_Unicode cRightDoubleAngleQuote = 0xBB;
constexpr sal_Unicode cLeftSingleAngleQuote = 0x2039;
constexpr sal_Unicode cRightSingleAngleQuote = 0x203A;
// stop characters for searching preceding quotes
// (the first character is also the opening quote we are looking for)
const sal_Unicode aStopDoubleAngleQuoteStart[] = { 0x201E, 0x201D, 0x201C, 0 }; // preceding ,,
const sal_Unicode aStopDoubleAngleQuoteEnd[] = { cRightDoubleAngleQuote, cLeftDoubleAngleQuote, 0x201D, 0x201E, 0 }; // preceding >>
// preceding << for Romanian, handle also alternative primary closing quotation mark U+201C
const sal_Unicode aStopDoubleAngleQuoteEndRo[] = { cLeftDoubleAngleQuote, cRightDoubleAngleQuote, 0x201D, 0x201E, 0x201C, 0 };
const sal_Unicode aStopSingleQuoteEnd[] = { 0x201A, 0x2018, 0x201C, 0x201E, 0 };
const sal_Unicode aStopSingleQuoteEndRuUa[] = { 0x201E, 0x201C, cRightDoubleAngleQuote, cLeftDoubleAngleQuote, 0 };

SvxAutoCorrect::SvxAutoCorrect( OUString aShareAutocorrFile,
                                OUString aUserAutocorrFile )
    : sShareAutoCorrFile(std::move( aShareAutocorrFile ))
    , sUserAutoCorrFile(std::move( aUserAutocorrFile ))
    , eCharClassLang( LANGUAGE_DONTKNOW )
    , nFlags(SvxAutoCorrect::GetDefaultFlags())
    , cStartDQuote( 0 )
    , cEndDQuote( 0 )
    , cStartSQuote( 0 )
    , cEndSQuote( 0 )
{
}

SvxAutoCorrect::SvxAutoCorrect( const SvxAutoCorrect& rCpy )
    : sShareAutoCorrFile( rCpy.sShareAutoCorrFile )
    , sUserAutoCorrFile( rCpy.sUserAutoCorrFile )
    , aSwFlags( rCpy.aSwFlags )
    , eCharClassLang(rCpy.eCharClassLang)
    , nFlags( rCpy.nFlags & ~ACFlags(ACFlags::ChgWordLstLoad|ACFlags::CplSttLstLoad|ACFlags::WordStartLstLoad))
    , cStartDQuote( rCpy.cStartDQuote )
    , cEndDQuote( rCpy.cEndDQuote )
    , cStartSQuote( rCpy.cStartSQuote )
    , cEndSQuote( rCpy.cEndSQuote )
{
}


SvxAutoCorrect::~SvxAutoCorrect()
{
}

void SvxAutoCorrect::GetCharClass_( LanguageType eLang )
{
    moCharClass.emplace( LanguageTag( eLang) );
    eCharClassLang = eLang;
}

void SvxAutoCorrect::SetAutoCorrFlag( ACFlags nFlag, bool bOn )
{
    ACFlags nOld = nFlags;
    nFlags = bOn ? nFlags | nFlag
                 : nFlags & ~nFlag;

    if( !bOn )
    {
        if( (nOld & ACFlags::CapitalStartSentence) != (nFlags & ACFlags::CapitalStartSentence) )
            nFlags &= ~ACFlags::CplSttLstLoad;
        if( (nOld & ACFlags::CapitalStartWord) != (nFlags & ACFlags::CapitalStartWord) )
            nFlags &= ~ACFlags::WordStartLstLoad;
        if( (nOld & ACFlags::Autocorrect) != (nFlags & ACFlags::Autocorrect) )
            nFlags &= ~ACFlags::ChgWordLstLoad;
    }
}


// Correct TWo INitial CApitals
void SvxAutoCorrect::FnCapitalStartWord( SvxAutoCorrDoc& rDoc, const OUString& rTxt,
                                    sal_Int32 nSttPos, sal_Int32 nEndPos,
                                    LanguageType eLang )
{
    CharClass& rCC = GetCharClass( eLang );

    // Delete all non alphanumeric. Test the characters at the beginning/end of
    // the word ( recognizes: "(min.", "/min.", and so on.)
    for( ; nSttPos < nEndPos; ++nSttPos )
        if( rCC.isLetterNumeric( rTxt, nSttPos ))
            break;
    for( ; nSttPos < nEndPos; --nEndPos )
        if( rCC.isLetterNumeric( rTxt, nEndPos - 1 ))
            break;

    // Is the word a compounded word separated by delimiters?
    // If so, keep track of all delimiters so each constituent
    // word can be checked for two initial capital letters.
    std::deque<sal_Int32> aDelimiters;

    // Always check for two capitals at the beginning
    // of the entire word, so start at nSttPos.
    aDelimiters.push_back(nSttPos);

    // Find all compound word delimiters
    for (sal_Int32 n = nSttPos; n < nEndPos; ++n)
    {
        if (IsCompoundWordDelimChar(rTxt[ n ]))
        {
            aDelimiters.push_back( n + 1 ); // Get position of char after delimiter
        }
    }

    // Decide where to put the terminating delimiter.
    // If the last AutoCorrect char was a newline, then the AutoCorrect
    // char will not be included in rTxt.
    // If the last AutoCorrect char was not a newline, then the AutoCorrect
    // character will be the last character in rTxt.
    if (!IsCompoundWordDelimChar(rTxt[nEndPos-1]))
        aDelimiters.push_back(nEndPos);

    // Iterate through the word and all words that compose it.
    // Two capital letters at the beginning of word?
    for (size_t nI = 0; nI < aDelimiters.size() - 1; ++nI)
    {
        nSttPos = aDelimiters[nI];
        nEndPos = aDelimiters[nI + 1];

        if( nSttPos+2 < nEndPos &&
            IsUpperLetter( rCC.getCharacterType( rTxt, nSttPos )) &&
            IsUpperLetter( rCC.getCharacterType( rTxt, ++nSttPos )) &&
            // Is the third character a lower case
            IsLowerLetter( rCC.getCharacterType( rTxt, nSttPos +1 )) &&
            // Do not replace special attributes
            0x1 != rTxt[ nSttPos ] && 0x2 != rTxt[ nSttPos ])
        {
            // test if the word is in an exception list
            OUString sWord( rTxt.copy( nSttPos - 1, nEndPos - nSttPos + 1 ));
            if( !FindInWordStartExceptList(eLang, sWord) )
            {
                // Check that word isn't correctly spelt before correcting:
                css::uno::Reference< css::linguistic2::XSpellChecker1 > xSpeller =
                    LinguMgr::GetSpellChecker();
                if( xSpeller->hasLanguage(static_cast<sal_uInt16>(eLang)) )
                {
                    Sequence< css::beans::PropertyValue > aEmptySeq;
                    if (xSpeller->isValid(sWord, static_cast<sal_uInt16>(eLang), aEmptySeq))
                    {
                        return;
                    }
                }
                sal_Unicode cSave = rTxt[ nSttPos ];
                OUString sChar = rCC.lowercase( OUString(cSave) );
                if( sChar[0] != cSave && rDoc.ReplaceRange( nSttPos, 1, sChar ))
                {
                    if( ACFlags::SaveWordWordStartLst & nFlags )
                        rDoc.SaveCpltSttWord( ACFlags::CapitalStartWord, nSttPos, sWord, cSave );
                }
            }
        }
    }
}

// Format ordinal numbers suffixes (1st -> 1^st)
bool SvxAutoCorrect::FnChgOrdinalNumber(
    SvxAutoCorrDoc& rDoc, const OUString& rTxt,
    sal_Int32 nSttPos, sal_Int32 nEndPos,
    LanguageType eLang)
{
    // 1st, 2nd, 3rd, 4 - 0th
    // 201th or 201st
    // 12th or 12nd
    bool bChg = false;

    // In some languages ordinal suffixes should never be
    // changed to superscript. Let's break for those languages.
    if (!eLang.anyOf(
         LANGUAGE_CATALAN,              // tdf#156792
         LANGUAGE_CATALAN_VALENCIAN,
         LANGUAGE_SWEDISH,
         LANGUAGE_SWEDISH_FINLAND))
    {
        CharClass& rCC = GetCharClass(eLang);

        for (; nSttPos < nEndPos; ++nSttPos)
            if (!lcl_IsInArr(sImplSttSkipChars, rTxt[nSttPos]))
                break;
        for (; nSttPos < nEndPos; --nEndPos)
            if (!lcl_IsInArr(sImplEndSkipChars, rTxt[nEndPos - 1]))
                break;


        // Get the last number in the string to check
        sal_Int32 nNumEnd = nEndPos;
        bool bFoundEnd = false;
        bool isValidNumber = true;
        sal_Int32 i = nEndPos;
        while (i > nSttPos)
        {
            i--;
            bool isDigit = rCC.isDigit(rTxt, i);
            if (bFoundEnd)
                isValidNumber &= (isDigit || !rCC.isLetter(rTxt, i));

            if (isDigit && !bFoundEnd)
            {
                bFoundEnd = true;
                nNumEnd = i;
            }
        }

        if (bFoundEnd && isValidNumber) {
            sal_Int32 nNum = o3tl::toInt32(rTxt.subView(nSttPos, nNumEnd - nSttPos + 1));
            std::u16string_view sEnd = rTxt.subView(nNumEnd + 1, nEndPos - nNumEnd - 1);

            // Check if the characters after that number correspond to the ordinal suffix
            uno::Reference< i18n::XOrdinalSuffix > xOrdSuffix
                = i18n::OrdinalSuffix::create(comphelper::getProcessComponentContext());

            uno::Sequence< OUString > aSuffixes = xOrdSuffix->getOrdinalSuffix(nNum, rCC.getLanguageTag().getLocale());

            // add extra suffixes for languages not handled by i18npool/ICU
            if ( primary(eLang) == primary(LANGUAGE_PORTUGUESE) &&
                            ( nEndPos == nNumEnd + 3 || nEndPos == nNumEnd + 4 ) &&
                            ( sEnd[0] == 'a' || sEnd[0] == 'o' || sEnd[0] == 'r' ) )
            {
               auto aExtendedSuffixes = comphelper::sequenceToContainer< std::vector<OUString> >(aSuffixes);
               aExtendedSuffixes.push_back(u"as"_ustr); // plural form of 'a'
               aExtendedSuffixes.push_back(u"os"_ustr); // plural form of 'o'
               aExtendedSuffixes.push_back(u"ra"_ustr); // alternative form of 'a'
               aExtendedSuffixes.push_back(u"ro"_ustr); // alternative form of 'o'
               aExtendedSuffixes.push_back(u"ras"_ustr); // alternative form of "as"
               aExtendedSuffixes.push_back(u"ros"_ustr); // alternative form of "os"
               aSuffixes = comphelper::containerToSequence(aExtendedSuffixes);
            }

            for (OUString const & sSuffix : aSuffixes)
            {
                if (sSuffix == sEnd)
                {
                    // Check if the ordinal suffix has to be set as super script
                    if (rCC.isLetter(sSuffix))
                    {
                        sal_Int32 nNumberChanged = 0;
                        sal_Int32 nSuffixChanged = 0;
                        // exceptions for Portuguese
                        // add missing dot: 1a -> 1.ª
                        // and remove optional 'r': 1ro -> 1.º
                        if ( primary(eLang) == primary(LANGUAGE_PORTUGUESE) )
                        {
                            if ( sSuffix.startsWith("r") )
                            {
                                rDoc.Delete( nNumEnd + 1, nNumEnd + 2 );
                                nSuffixChanged = -1;
                            }
                            rDoc.Insert( nNumEnd + 1, u"."_ustr );
                            nNumberChanged = 1;
                        }

                        // Do the change
                        SvxEscapementItem aSvxEscapementItem(DFLT_ESC_AUTO_SUPER,
                            DFLT_ESC_PROP, SID_ATTR_CHAR_ESCAPEMENT);
                        rDoc.SetAttr(nNumEnd + 1 + nNumberChanged,
                            nEndPos + nNumberChanged + nSuffixChanged,
                            SID_ATTR_CHAR_ESCAPEMENT,
                            aSvxEscapementItem);
                        bChg = true;
                        break;
                    }
                }
            }
        }
    }
    return bChg;
}

// Replace dashes
bool SvxAutoCorrect::FnChgToEnEmDash(
                                SvxAutoCorrDoc& rDoc, const OUString& rTxt,
                                sal_Int32 nSttPos, sal_Int32 nEndPos,
                                LanguageType eLang )
{
    bool bRet = false;
    CharClass& rCC = GetCharClass( eLang );
    if (eLang == LANGUAGE_SYSTEM)
        eLang = GetAppLang().getLanguageType();
    bool bAlwaysUseEmDash = (eLang == LANGUAGE_RUSSIAN || eLang == LANGUAGE_UKRAINIAN);

    // rTxt may refer to the frame text that will change in the calls to rDoc.Delete / rDoc.Insert;
    // keep a local copy for later use
    OUString aOrigTxt = rTxt;
    sal_Int32 nFirstReplacementTextLengthChange = 0;

    // replace " - " or " --" with "enDash"
    if( 1 < nSttPos && 1 <= nEndPos - nSttPos )
    {
        sal_Unicode cCh = rTxt[ nSttPos ];
        if( '-' == cCh )
        {
            if( 1 < nEndPos - nSttPos &&
                ' ' == rTxt[ nSttPos-1 ] &&
                '-' == rTxt[ nSttPos+1 ])
            {
                sal_Int32 n;
                for( n = nSttPos+2; n < nEndPos && lcl_IsInArr(
                            sImplSttSkipChars,(cCh = rTxt[ n ]));
                        ++n )
                    ;

                // found: " --[<AnySttChars>][A-z0-9]
                if( rCC.isLetterNumeric( OUString(cCh) ) )
                {
                    for( n = nSttPos-1; n && lcl_IsInArr(
                            sImplEndSkipChars,(cCh = rTxt[ --n ])); )
                        ;

                    // found: "[A-z0-9][<AnyEndChars>] --[<AnySttChars>][A-z0-9]
                    if( rCC.isLetterNumeric( OUString(cCh) ))
                    {
                        rDoc.Delete( nSttPos, nSttPos + 2 );
                        rDoc.Insert( nSttPos, bAlwaysUseEmDash ? sEmDash : sEnDash );
                        nFirstReplacementTextLengthChange = -1; // 2 ch -> 1 ch
                        bRet = true;
                    }
                }
            }
        }
        else if( 3 < nSttPos &&
                 ' ' == rTxt[ nSttPos-1 ] &&
                 '-' == rTxt[ nSttPos-2 ])
        {
            sal_Int32 n, nLen = 1, nTmpPos = nSttPos - 2;
            if( '-' == ( cCh = rTxt[ nTmpPos-1 ]) )
            {
                --nTmpPos;
                ++nLen;
                cCh = rTxt[ nTmpPos-1 ];
            }
            if( ' ' == cCh )
            {
                for( n = nSttPos; n < nEndPos && lcl_IsInArr(
                            sImplSttSkipChars,(cCh = rTxt[ n ]));
                        ++n )
                    ;

                // found: " - [<AnySttChars>][A-z0-9]
                if( rCC.isLetterNumeric( OUString(cCh) ) )
                {
                    cCh = ' ';
                    for( n = nTmpPos-1; n && lcl_IsInArr(
                            sImplEndSkipChars,(cCh = rTxt[ --n ])); )
                            ;
                    // found: "[A-z0-9][<AnyEndChars>] - [<AnySttChars>][A-z0-9]
                    if (rCC.isLetterNumeric(OUString(cCh)) || lcl_IsInArr(u".!?", cCh))
                    {
                        rDoc.Delete( nTmpPos, nTmpPos + nLen );
                        rDoc.Insert( nTmpPos, bAlwaysUseEmDash ? sEmDash : sEnDash );
                        nFirstReplacementTextLengthChange = 1 - nLen; // nLen ch -> 1 ch
                        bRet = true;
                    }
                }
            }
        }
    }

    // Replace [A-z0-9]--[A-z0-9] double dash with "emDash" or "enDash"
    // [0-9]--[0-9] double dash always replaced with "enDash"
    // Finnish and Hungarian use enDash instead of emDash.
    bool bEnDash = (eLang == LANGUAGE_HUNGARIAN || eLang == LANGUAGE_FINNISH);
    if( 4 <= nEndPos - nSttPos )
    {
        std::u16string_view sTmpView( aOrigTxt.subView( nSttPos, nEndPos - nSttPos ) );
        size_t nFndPos = sTmpView.find(u"--");
        if (nFndPos > 0 && nFndPos < sTmpView.size() - 2)
        {
            // Use proper codepoints. Currently, CharClass::isLetterNumeric is broken, it
            // uses the index *both* as code unit index (when checking it as ASCII), *and*
            // as code point index (when passes to css::i18n::XCharacterClassification).
            // Oh well... Anyway, single-codepoint strings will workaround it.
            sal_Int32 nStart = nSttPos + nFndPos;
            sal_uInt32 chStart = aOrigTxt.iterateCodePoints(&nStart, -1);
            OUString sStart(&chStart, 1);
            // No idea why sImplEndSkipChars is checked at start
            if (rCC.isLetterNumeric(sStart, 0) || lcl_IsInArr(sImplEndSkipChars, chStart))
            {
                sal_Int32 nEnd = nSttPos + nFndPos + 2;
                sal_uInt32 chEnd = aOrigTxt.iterateCodePoints(&nEnd, 1);
                OUString sEnd(&chEnd, 1);
                // No idea why sImplSttSkipChars is checked at end
                if (rCC.isLetterNumeric(sEnd, 0) || lcl_IsInArr(sImplSttSkipChars, chEnd))
                {
                    nSttPos = nSttPos + nFndPos + nFirstReplacementTextLengthChange;
                    rDoc.Delete(nSttPos, nSttPos + 2);
                    rDoc.Insert(nSttPos,
                                (bEnDash || (rCC.isDigit(sStart, 0) && rCC.isDigit(sEnd, 0))
                                     ? sEnDash
                                     : sEmDash));
                    bRet = true;
                }
            }
        }
    }
    return bRet;
}

// Add non-breaking space before specific punctuation marks in French text
sal_Int32 SvxAutoCorrect::FnAddNonBrkSpace(
                                SvxAutoCorrDoc& rDoc, std::u16string_view rTxt,
                                sal_Int32 nEndPos,
                                LanguageType eLang, bool& io_bNbspRunNext )
{
    sal_Int32 nRet = -1;

    CharClass& rCC = GetCharClass( eLang );

    if ( rCC.getLanguageTag().getLanguage() == "fr" )
    {
        bool bFrCA = (rCC.getLanguageTag().getCountry() == "CA");
        OUString allChars = u":;?!%"_ustr;
        OUString chars( allChars );
        if ( bFrCA )
            chars = ":";

        sal_Unicode cChar = rTxt[ nEndPos ];
        bool bHasSpace = chars.indexOf( cChar ) != -1;
        bool bIsSpecial = allChars.indexOf( cChar ) != -1;
        if ( bIsSpecial )
        {
            // Get the last word delimiter position
            sal_Int32 nSttWdPos = nEndPos;
            bool bWasWordDelim = false;
            while( nSttWdPos )
            {
                bWasWordDelim = IsWordDelim( rTxt[ --nSttWdPos ]);
                if (bWasWordDelim)
                    break;
            }

            //See if the text is the start of a protocol string, e.g. have text of
            //"http" see if it is the start of "http:" and if so leave it alone
            size_t nIndex = nSttWdPos + (bWasWordDelim ? 1 : 0);
            size_t nProtocolLen = nEndPos - nSttWdPos + 1;
            if (nIndex + nProtocolLen <= rTxt.size())
            {
                if (INetURLObject::CompareProtocolScheme(rTxt.substr(nIndex, nProtocolLen)) != INetProtocol::NotValid)
                    return -1;
            }

            // Check the presence of "://" in the word
            size_t nStrPos = rTxt.find( u"://", nSttWdPos + 1 );
            if ( nStrPos == std::u16string_view::npos && nEndPos > 0 )
            {
                // Check the previous char
                sal_Unicode cPrevChar = rTxt[ nEndPos - 1 ];
                if ( ( chars.indexOf( cPrevChar ) == -1 ) && cPrevChar != '\t' )
                {
                    // Remove any previous normal space
                    sal_Int32 nPos = nEndPos - 1;
                    while ( cPrevChar == ' ' || cPrevChar == cNonBreakingSpace )
                    {
                        if ( nPos == 0 ) break;
                        nPos--;
                        cPrevChar = rTxt[ nPos ];
                    }

                    nPos++;
                    if ( nEndPos - nPos > 0 )
                        rDoc.Delete( nPos, nEndPos );

                    // Add the non-breaking space at the end pos
                    if ( bHasSpace )
                        rDoc.Insert( nPos, OUString(cNonBreakingSpace) );
                    io_bNbspRunNext = true;
                    nRet = nPos;
                }
                else if ( chars.indexOf( cPrevChar ) != -1 )
                    io_bNbspRunNext = true;
            }
        }
        else if ( cChar == '/' && nEndPos > 1 && static_cast<sal_Int32>(rTxt.size()) > (nEndPos - 1) )
        {
            // Remove the hardspace right before to avoid formatting URLs
            sal_Unicode cPrevChar = rTxt[ nEndPos - 1 ];
            sal_Unicode cMaybeSpaceChar = rTxt[ nEndPos - 2 ];
            if ( cPrevChar == ':' && cMaybeSpaceChar == cNonBreakingSpace )
            {
                rDoc.Delete( nEndPos - 2, nEndPos - 1 );
                nRet = nEndPos - 1;
            }
        }
    }

    return nRet;
}

// URL recognition
bool SvxAutoCorrect::FnSetINetAttr( SvxAutoCorrDoc& rDoc, const OUString& rTxt,
                                    sal_Int32 nSttPos, sal_Int32 nEndPos,
                                    LanguageType eLang )
{
    OUString sURL( URIHelper::FindFirstURLInText( rTxt, nSttPos, nEndPos,
                                                GetCharClass( eLang ) ));
    bool bRet = !sURL.isEmpty();
    if( bRet )          // so, set attribute:
        rDoc.SetINetAttr( nSttPos, nEndPos, sURL );
    return bRet;
}

// DOI citation recognition
bool SvxAutoCorrect::FnSetDOIAttr( SvxAutoCorrDoc& rDoc, std::u16string_view rTxt,
                                    sal_Int32 nSttPos, sal_Int32 nEndPos,
                                    LanguageType eLang )
{
    OUString sURL( URIHelper::FindFirstDOIInText( rTxt, nSttPos, nEndPos, GetCharClass( eLang ) ));
    bool bRet = !sURL.isEmpty();
    if( bRet )          // so, set attribute:
        rDoc.SetINetAttr( nSttPos, nEndPos, sURL );
    return bRet;
}

// Automatic *bold*, /italic/, -strikeout- and _underline_
bool SvxAutoCorrect::FnChgWeightUnderl( SvxAutoCorrDoc& rDoc, const OUString& rTxt,
                                        sal_Int32 nEndPos )
{
    // Condition:
    //  at the beginning:   _, *, / or ~ after Space with the following !Space
    //  at the end:         _, *, / or ~ before Space (word delimiter?)

    sal_Unicode cInsChar = rTxt[ nEndPos ];  // underline, bold, italic or strikeout
    if( ++nEndPos != rTxt.getLength() &&
        !IsWordDelim( rTxt[ nEndPos ] ) )
        return false;

    --nEndPos;

    bool bAlphaNum = false;
    sal_Int32 nPos = nEndPos;
    sal_Int32  nFndPos = -1;
    CharClass& rCC = GetCharClass( LANGUAGE_SYSTEM );

    while( nPos )
    {
        switch( sal_Unicode c = rTxt[ --nPos ] )
        {
        case '_':
        case '-':
        case '/':
        case '*':
            if( c == cInsChar )
            {
                if( bAlphaNum && nPos+1 < nEndPos && ( !nPos ||
                    IsWordDelim( rTxt[ nPos-1 ])) &&
                    !IsWordDelim( rTxt[ nPos+1 ]))
                        nFndPos = nPos;
                else
                    // Condition is not satisfied, so cancel
                    nFndPos = -1;
                nPos = 0;
            }
            break;
        default:
            if( !bAlphaNum )
                bAlphaNum = rCC.isLetterNumeric( rTxt, nPos );
        }
    }

    if( -1 != nFndPos )
    {
        // first delete the Character at the end - this allows insertion
        // of an empty hint in SetAttr which would be removed by Delete
        // (fdo#62536, AUTOFMT in Writer)
        rDoc.Delete( nEndPos, nEndPos + 1 );

        // Span the Attribute over the area
        // the end.
        if( '*' == cInsChar )           // Bold
        {
            SvxWeightItem aSvxWeightItem( WEIGHT_BOLD, SID_ATTR_CHAR_WEIGHT );
            rDoc.SetAttr( nFndPos + 1, nEndPos,
                          SID_ATTR_CHAR_WEIGHT,
                          aSvxWeightItem);
        }
        else if( '/' == cInsChar )           // Italic
        {
            SvxPostureItem aSvxPostureItem( ITALIC_NORMAL, SID_ATTR_CHAR_POSTURE );
            rDoc.SetAttr( nFndPos + 1, nEndPos,
                          SID_ATTR_CHAR_POSTURE,
                          aSvxPostureItem);
        }
        else if( '-' == cInsChar )           // Strikeout
        {
            SvxCrossedOutItem aSvxCrossedOutItem( STRIKEOUT_SINGLE, SID_ATTR_CHAR_STRIKEOUT );
            rDoc.SetAttr( nFndPos + 1, nEndPos,
                          SID_ATTR_CHAR_STRIKEOUT,
                          aSvxCrossedOutItem);
        }
        else                            // Underline
        {
            SvxUnderlineItem aSvxUnderlineItem( LINESTYLE_SINGLE, SID_ATTR_CHAR_UNDERLINE );
            rDoc.SetAttr( nFndPos + 1, nEndPos,
                          SID_ATTR_CHAR_UNDERLINE,
                          aSvxUnderlineItem);
        }
        rDoc.Delete( nFndPos, nFndPos + 1 );
    }

    return -1 != nFndPos;
}

// Capitalize first letter of every sentence
void SvxAutoCorrect::FnCapitalStartSentence( SvxAutoCorrDoc& rDoc,
                                    const OUString& rTxt, bool bNormalPos,
                                    sal_Int32 nSttPos, sal_Int32 nEndPos,
                                    LanguageType eLang )
{

    if( rTxt.isEmpty() || nEndPos <= nSttPos )
        return;

    CharClass& rCC = GetCharClass( eLang );
    OUString aText( rTxt );
    const sal_Unicode *pStart = aText.getStr(),
                      *pStr = pStart + nEndPos,
                      *pWordStt = nullptr,
                      *pDelim = nullptr;

    bool bAtStart = false;
    do {
        --pStr;
        if (rCC.isLetter(aText, pStr - pStart))
        {
            if( !pWordStt )
                pDelim = pStr+1;
            pWordStt = pStr;
        }
        else if (pWordStt && !rCC.isDigit(aText, pStr - pStart))
        {
            if( (lcl_IsInArr( u"-'", *pStr ) || *pStr == cApostrophe) && // These characters are allowed in words
                pWordStt - 1 == pStr &&
                // Installation at beginning of paragraph. Replaced < by <= (#i38971#)
                (pStart + 1) <= pStr &&
                rCC.isLetter(aText, pStr-1 - pStart))
                pWordStt = --pStr;
            else
                break;
        }
        bAtStart = (pStart == pStr);
    } while( !bAtStart );

    if (!pWordStt)
        return;    // no character to be replaced


    if (rCC.isDigit(aText, pStr - pStart))
        return; // already ok

    if (IsUpperLetter(rCC.getCharacterType(aText, pWordStt - pStart)))
        return; // already ok

    //See if the text is the start of a protocol string, e.g. have text of
    //"http" see if it is the start of "http:" and if so leave it alone
    sal_Int32 nIndex = pWordStt - pStart;
    sal_Int32 nProtocolLen = pDelim - pWordStt + 1;
    if (nIndex + nProtocolLen <= rTxt.getLength())
    {
        if (INetURLObject::CompareProtocolScheme(rTxt.subView(nIndex, nProtocolLen)) != INetProtocol::NotValid)
            return; // already ok
    }

    if (0x1 == *pWordStt || 0x2 == *pWordStt)
        return; // already ok

    // Only capitalize, if string before specified characters is long enough
    if( *pDelim && 2 >= pDelim - pWordStt &&
        lcl_IsInArr( u".-)>", *pDelim ) )
        return;

    // tdf#59666 don't capitalize single Greek letters (except in Greek texts)
    if ( 1 == pDelim - pWordStt && 0x03B1 <= *pWordStt && *pWordStt <= 0x03C9 && eLang != LANGUAGE_GREEK )
        return;

    if( !bAtStart ) // Still no beginning of a paragraph?
    {
        if (NonFieldWordDelim(*pStr))
        {
            for (;;)
            {
                bAtStart = (pStart == pStr--);
                if (bAtStart || !NonFieldWordDelim(*pStr))
                    break;
            }
        }
        // Asian full stop, full width full stop, full width exclamation mark
        // and full width question marks are treated as word delimiters
        else if ( 0x3002 != *pStr && 0xFF0E != *pStr && 0xFF01 != *pStr &&
                  0xFF1F != *pStr )
            return; // no valid separator -> no replacement
    }

    // No replacement for words in TWo INitial CApitals or sMALL iNITIAL list
    if (FindInWordStartExceptList(eLang, OUString(pWordStt, pDelim - pWordStt)))
        return;

    if( bAtStart )  // at the beginning of a paragraph?
    {
        // Check out the previous paragraph, if it exists.
        // If so, then check to paragraph separator at the end.
        OUString const*const pPrevPara = rDoc.GetPrevPara(bNormalPos);
        if (!pPrevPara)
        {
            // valid separator -> replace
            OUString sChar( *pWordStt );
            sChar = rCC.titlecase(sChar); //see fdo#56740
            if (sChar != OUStringChar(*pWordStt))
               rDoc.ReplaceRange( pWordStt - pStart, 1, sChar );
            return;
        }

        aText = *pPrevPara;
        bAtStart = false;
        pStart = aText.getStr();
        pStr = pStart + aText.getLength();

        do {            // overwrite all blanks
            --pStr;
            if (!NonFieldWordDelim(*pStr))
                break;
            bAtStart = (pStart == pStr);
        } while( !bAtStart );

        if( bAtStart )
            return;  // no valid separator -> no replacement
    }

    // Found [ \t]+[A-Z0-9]+ until here. Test now on the paragraph separator.
    // all three can happen, but not more than once!
    const sal_Unicode* pExceptStt = nullptr;
    bool bContinue = true;
    Flags nFlag = Flags::NONE;
    do
    {
        switch (*pStr)
        {
            // Western and Asian full stop
            case '.':
            case 0x3002:
            case 0xFF0E:
            {
                if (pStr >= pStart + 2 && *(pStr - 2) == '.')
                {
                    //e.g. text "f.o.o. word": Now currently considering
                    //capitalizing word but second last character of
                    //previous word is a .  So probably last word is an
                    //anagram that ends in . and not truly the end of a
                    //previous sentence, so don't autocapitalize this word
                    return;
                }
                if (nFlag & Flags::FullStop)
                    return; // no valid separator -> no replacement
                nFlag |= Flags::FullStop;
                pExceptStt = pStr;
            }
            break;
            case '!':
            case 0xFF01:
            {
                if (nFlag & Flags::ExclamationMark)
                    return; // no valid separator -> no replacement
                nFlag |= Flags::ExclamationMark;
            }
            break;
            case '?':
            case 0xFF1F:
            {
                if (nFlag & Flags::QuestionMark)
                    return; // no valid separator -> no replacement
                nFlag |= Flags::QuestionMark;
            }
            break;
            default:
                if (nFlag == Flags::NONE)
                    return; // no valid separator -> no replacement
                else
                    bContinue = false;
                break;
        }

        if (bContinue && pStr-- == pStart)
        {
            return; // no valid separator -> no replacement
        }
    } while (bContinue);
    if (Flags::FullStop != nFlag)
        pExceptStt = nullptr;

    // Only capitalize, if string is long enough
    if( 2 > ( pStr - pStart ) )
        return;

    if (!rCC.isLetterNumeric(aText, pStr-- - pStart))
    {
        bool bValid = false, bAlphaFnd = false;
        const sal_Unicode* pTmpStr = pStr;
        while( !bValid )
        {
            if( rCC.isDigit( aText, pTmpStr - pStart ) )
            {
                bValid = true;
                pStr = pTmpStr - 1;
            }
            else if( rCC.isLetter( aText, pTmpStr - pStart ) )
            {
                if( bAlphaFnd )
                {
                    bValid = true;
                    pStr = pTmpStr;
                }
                else
                    bAlphaFnd = true;
            }
            else if (bAlphaFnd || NonFieldWordDelim(*pTmpStr))
                break;

            if( pTmpStr == pStart )
                break;

            --pTmpStr;
        }

        if( !bValid )
            return;       // no valid separator -> no replacement
    }

    bool bNumericOnly = '0' <= *(pStr+1) && *(pStr+1) <= '9';

    // Search for the beginning of the word
    while (!NonFieldWordDelim(*pStr))
    {
        if( bNumericOnly && rCC.isLetter( aText, pStr - pStart ) )
            bNumericOnly = false;

        if( pStart == pStr )
            break;

        --pStr;
    }

    if( bNumericOnly )      // consists of only numbers, then not
        return;

    if (NonFieldWordDelim(*pStr))
        ++pStr;

    OUString sWord;

    // check on the basis of the exception list
    if( pExceptStt )
    {
        sWord = OUString(pStr, pExceptStt - pStr + 1);
        if( FindInCplSttExceptList(eLang, sWord) )
            return;

        // Delete all non alphanumeric. Test the characters at the
        // beginning/end of the word ( recognizes: "(min.", "/min.", and so on.)
        OUString sTmp( sWord );
        while( !sTmp.isEmpty() &&
                !rCC.isLetterNumeric( sTmp, 0 ) )
            sTmp = sTmp.copy(1);

        // Remove all non alphanumeric characters towards the end up until
        // the last one.
        sal_Int32 nLen = sTmp.getLength();
        while( nLen && !rCC.isLetterNumeric( sTmp, nLen-1 ) )
            --nLen;
        if( nLen + 1 < sTmp.getLength() )
            sTmp = sTmp.copy( 0, nLen + 1 );

        if( !sTmp.isEmpty() && sTmp.getLength() != sWord.getLength() &&
            FindInCplSttExceptList(eLang, sTmp))
            return;

        if(FindInCplSttExceptList(eLang, sWord, true))
            return;
    }

    // Ok, then replace
    sal_Unicode cSave = *pWordStt;
    nSttPos = pWordStt - rTxt.getStr();
    OUString sChar = rCC.titlecase(OUString(cSave)); //see fdo#56740
    bool bRet = sChar[0] != cSave && rDoc.ReplaceRange( nSttPos, 1, sChar );

    // Perhaps someone wants to have the word
    if( bRet && ACFlags::SaveWordCplSttLst & nFlags )
        rDoc.SaveCpltSttWord( ACFlags::CapitalStartSentence, nSttPos, sWord, cSave );
}

// Correct accidental use of cAPS LOCK key
bool SvxAutoCorrect::FnCorrectCapsLock( SvxAutoCorrDoc& rDoc, const OUString& rTxt,
                                        sal_Int32 nSttPos, sal_Int32 nEndPos,
                                        LanguageType eLang )
{
    if (nEndPos - nSttPos < 2)
        // string must be at least 2-character long.
        return false;

    CharClass& rCC = GetCharClass( eLang );

    // Check the first 2 letters.
    if ( !IsLowerLetter(rCC.getCharacterType(rTxt, nSttPos)) )
        return false;

    if ( !IsUpperLetter(rCC.getCharacterType(rTxt, nSttPos+1)) )
        return false;

    OUStringBuffer aConverted;
    aConverted.append( rCC.uppercase(OUString(rTxt[nSttPos])) );
    aConverted.append( rCC.lowercase(OUString(rTxt[nSttPos+1])) );

    // No replacement for words in TWo INitial CApitals or sMALL iNITIAL list
    if (FindInWordStartExceptList(eLang, rTxt.copy(nSttPos, nEndPos - nSttPos)))
        return false;

    for( sal_Int32 i = nSttPos+2; i < nEndPos; ++i )
    {
        if ( IsLowerLetter(rCC.getCharacterType(rTxt, i)) )
            // A lowercase letter disqualifies the whole text.
            return false;

        if ( IsUpperLetter(rCC.getCharacterType(rTxt, i)) )
            // Another uppercase letter.  Convert it.
            aConverted.append( rCC.lowercase(OUString(rTxt[i])) );
        else
            // This is not an alphabetic letter.  Leave it as-is.
            aConverted.append( rTxt[i] );
    }

    // Replace the word.
    rDoc.Delete(nSttPos, nEndPos);
    rDoc.Insert(nSttPos, aConverted.makeStringAndClear());

    return true;
}


sal_Unicode SvxAutoCorrect::GetQuote( sal_Unicode cInsChar, bool bSttQuote,
                                        LanguageType eLang ) const
{
    sal_Unicode cRet = bSttQuote ? ( '\"' == cInsChar
                                    ? GetStartDoubleQuote()
                                    : GetStartSingleQuote() )
                                   : ( '\"' == cInsChar
                                    ? GetEndDoubleQuote()
                                    : GetEndSingleQuote() );
    if( !cRet )
    {
        // then through the Language find the right character
        if( LANGUAGE_NONE == eLang )
            cRet = cInsChar;
        else
        {
            LocaleDataWrapper& rLcl = GetLocaleDataWrapper( eLang );
            OUString sRet( bSttQuote
                            ? ( '\"' == cInsChar
                                ? rLcl.getDoubleQuotationMarkStart()
                                : rLcl.getQuotationMarkStart() )
                            : ( '\"' == cInsChar
                                ? rLcl.getDoubleQuotationMarkEnd()
                                : rLcl.getQuotationMarkEnd() ));
            cRet = !sRet.isEmpty() ? sRet[0] : cInsChar;
        }
    }
    return cRet;
}

void SvxAutoCorrect::InsertQuote( SvxAutoCorrDoc& rDoc, sal_Int32 nInsPos,
                                    sal_Unicode cInsChar, bool bSttQuote,
                                    bool bIns, LanguageType eLang, ACQuotes eType ) const
{
    sal_Unicode cRet;

    if ( eType == ACQuotes::DoubleAngleQuote )
    {
        bool bSwiss = eLang == LANGUAGE_FRENCH_SWISS;
        // pressing " inside a quotation -> use second level angle quotes
        bool bLeftQuote = '\"' == cInsChar &&
                // start position and Romanian OR
                // not start position and Hungarian
                bSttQuote == (eLang != LANGUAGE_HUNGARIAN);
        cRet = ( '<' == cInsChar || bLeftQuote )
                ? ( bSwiss ? cLeftSingleAngleQuote : cLeftDoubleAngleQuote )
                : ( bSwiss ? cRightSingleAngleQuote : cRightDoubleAngleQuote );
    }
    else if ( eType == ACQuotes::UseApostrophe )
        cRet = cApostrophe;
    else
        cRet = GetQuote( cInsChar, bSttQuote, eLang );

    OUString sChg( cInsChar );
    if( bIns )
        rDoc.Insert( nInsPos, sChg );
    else
        rDoc.Replace( nInsPos, sChg );

    sChg = OUString(cRet);

    if( eType == ACQuotes::NonBreakingSpace )
    {
        if( rDoc.Insert( bSttQuote ? nInsPos+1 : nInsPos, OUStringChar(cNonBreakingSpace) ))
        {
            if( !bSttQuote )
                ++nInsPos;
        }
    }
    else if( eType == ACQuotes::DoubleAngleQuote && cInsChar != '\"' )
    {
        rDoc.Delete( nInsPos-1, nInsPos);
        --nInsPos;
    }

    rDoc.Replace( nInsPos, sChg );

    // i' -> I' in English (last step for the Undo)
    if( eType == ACQuotes::CapitalizeIAm )
        rDoc.Replace( nInsPos-1, u"I"_ustr );
}

OUString SvxAutoCorrect::GetQuote( SvxAutoCorrDoc const & rDoc, sal_Int32 nInsPos,
                                sal_Unicode cInsChar, bool bSttQuote )
{
    const LanguageType eLang = GetDocLanguage( rDoc, nInsPos );
    sal_Unicode cRet = GetQuote( cInsChar, bSttQuote, eLang );

    OUString sRet(cRet);

    if( '\"' == cInsChar )
    {
        if (primary(eLang) == primary(LANGUAGE_FRENCH) && eLang != LANGUAGE_FRENCH_SWISS)
        {
            if( bSttQuote )
                sRet += " ";
            else
                sRet = " " + sRet;
        }
    }
    return sRet;
}

// search preceding opening quote in the paragraph before the insert position
static bool lcl_HasPrecedingChar( std::u16string_view rTxt, sal_Int32 nPos,
                const sal_Unicode sPrecedingChar, const sal_Unicode sStopChar, const sal_Unicode* aStopChars )
{
    sal_Unicode cTmpChar;

    do {
        cTmpChar = rTxt[ --nPos ];
        if ( cTmpChar == sPrecedingChar )
            return true;

        if ( cTmpChar == sStopChar )
            return false;

        for ( const sal_Unicode* pCh = aStopChars; *pCh; ++pCh )
            if ( cTmpChar == *pCh )
                return false;

    } while ( nPos > 0 );

    return false;
}

// WARNING: rText may become invalid, see comment below
void SvxAutoCorrect::DoAutoCorrect( SvxAutoCorrDoc& rDoc, const OUString& rTxt,
                                    sal_Int32 nInsPos, sal_Unicode cChar,
                                    bool bInsert, bool& io_bNbspRunNext, vcl::Window const * pFrameWin )
{
    bool bIsNextRun = io_bNbspRunNext;
    io_bNbspRunNext = false;  // if it was set, then it has to be turned off

    do{                                 // only for middle check loop !!
        if( cChar )
        {
            // Prevent double space
            if( nInsPos && ' ' == cChar &&
                IsAutoCorrFlag( ACFlags::IgnoreDoubleSpace ) &&
                ' ' == rTxt[ nInsPos - 1 ])
            {
                break;
            }

            bool bSingle = '\'' == cChar;
            bool bIsReplaceQuote =
                        (IsAutoCorrFlag( ACFlags::ChgQuotes ) && ('\"' == cChar )) ||
                        (IsAutoCorrFlag( ACFlags::ChgSglQuotes ) && bSingle );
            if( bIsReplaceQuote )
            {
                bool bSttQuote = !nInsPos;
                ACQuotes eType = ACQuotes::NONE;
                const LanguageType eLang = GetDocLanguage( rDoc, nInsPos );
                if (!bSttQuote)
                {
                    sal_Unicode cPrev = rTxt[ nInsPos-1 ];
                    bSttQuote = NonFieldWordDelim(cPrev) ||
                        lcl_IsInArr( u"([{", cPrev ) ||
                        ( cEmDash == cPrev ) ||
                        ( cEnDash == cPrev );
                    // tdf#38394 use opening quotation mark << in French l'<<word>>
                    if ( !bSingle && !bSttQuote && cPrev == cApostrophe &&
                        primary(eLang) == primary(LANGUAGE_FRENCH) &&
                        ( ( ( nInsPos == 2 || ( nInsPos > 2 && IsWordDelim( rTxt[ nInsPos-3 ] ) ) ) &&
                               // abbreviated form of ce, de, je, la, le, ne, me, te, se or si
                               u"cdjlnmtsCDJLNMTS"_ustr.indexOf( rTxt[ nInsPos-2 ] ) > -1 ) ||
                          ( ( nInsPos == 3 || (nInsPos > 3 && IsWordDelim( rTxt[ nInsPos-4 ] ) ) ) &&
                               // abbreviated form of que
                               ( rTxt[ nInsPos-2 ] == 'u' || rTxt[ nInsPos-2 ] == 'U' ) &&
                               ( rTxt[ nInsPos-3 ] == 'q' || rTxt[ nInsPos-3 ] == 'Q' ) ) ) )
                    {
                        bSttQuote = true;
                    }
                    // tdf#108423 for capitalization of English i'm
                    else if ( bSingle && ( cPrev == 'i' ) &&
                        primary(eLang) == primary(LANGUAGE_ENGLISH) &&
                        ( nInsPos == 1 || IsWordDelim( rTxt[ nInsPos-2 ] ) ) )
                    {
                        eType = ACQuotes::CapitalizeIAm;
                    }
                    // tdf#133524 support >>Hungarian<< and <<Romanian>> secondary level quotations
                    else if ( !bSingle && nInsPos &&
                        ( ( eLang == LANGUAGE_HUNGARIAN &&
                            lcl_HasPrecedingChar( rTxt, nInsPos,
                                bSttQuote ? aStopDoubleAngleQuoteStart[0] : aStopDoubleAngleQuoteEnd[0],
                                bSttQuote ? aStopDoubleAngleQuoteStart[1] : aStopDoubleAngleQuoteEnd[1],
                                bSttQuote ? aStopDoubleAngleQuoteStart + 1 : aStopDoubleAngleQuoteEnd + 2 ) ) ||
                          ( eLang.anyOf(
                                LANGUAGE_ROMANIAN,
                                LANGUAGE_ROMANIAN_MOLDOVA ) &&
                            lcl_HasPrecedingChar( rTxt, nInsPos,
                                bSttQuote ? aStopDoubleAngleQuoteStart[0] : aStopDoubleAngleQuoteEndRo[0],
                                bSttQuote ? aStopDoubleAngleQuoteStart[1] : aStopDoubleAngleQuoteEndRo[1],
                                bSttQuote ? aStopDoubleAngleQuoteStart + 1 : aStopDoubleAngleQuoteEndRo + 2 ) ) ) )
                    {
                        LocaleDataWrapper& rLcl = GetLocaleDataWrapper( eLang );
                        // only if the opening double quotation mark is the default one
                        if ( rLcl.getDoubleQuotationMarkStart() == OUStringChar(aStopDoubleAngleQuoteStart[0]) )
                            eType = ACQuotes::DoubleAngleQuote;
                    }
                    else if ( bSingle && nInsPos && !bSttQuote &&
                        // tdf#128860 use apostrophe outside of second level quotation in Czech, German, Icelandic,
                        // Slovak and Slovenian instead of the – in this case, bad – closing quotation mark U+2018.
                        // tdf#123786 the same for Russian and Ukrainian
                        ( eLang.anyOf (
                                 LANGUAGE_CZECH,
                                 LANGUAGE_GERMAN,
                                 LANGUAGE_GERMAN_SWISS,
                                 LANGUAGE_GERMAN_AUSTRIAN,
                                 LANGUAGE_GERMAN_LUXEMBOURG,
                                 LANGUAGE_GERMAN_LIECHTENSTEIN,
                                 LANGUAGE_ICELANDIC,
                                 LANGUAGE_SLOVAK,
                                 LANGUAGE_SLOVENIAN ) ) )
                    {
                        sal_Unicode sStartChar = GetStartSingleQuote();
                        sal_Unicode sEndChar = GetEndSingleQuote();
                        if ( !sStartChar || !sEndChar ) {
                            LocaleDataWrapper& rLcl = GetLocaleDataWrapper( eLang );
                            if ( !sStartChar ) sStartChar = rLcl.getQuotationMarkStart()[0];
                            if ( !sEndChar ) sEndChar = rLcl.getQuotationMarkStart()[0];
                        }
                        if ( !lcl_HasPrecedingChar( rTxt, nInsPos, sStartChar, sEndChar, aStopSingleQuoteEnd + 1 ) )
                        {
                            CharClass& rCC = GetCharClass( eLang );
                            if ( rCC.isLetter(rTxt, nInsPos-1) )
                            {
                                eType = ACQuotes::UseApostrophe;
                            }
                        }
                    }
                    else if ( bSingle && nInsPos && !bSttQuote &&
                          ( eLang.anyOf (
                                 LANGUAGE_RUSSIAN,
                                 LANGUAGE_UKRAINIAN ) &&
                            !lcl_HasPrecedingChar( rTxt, nInsPos, aStopSingleQuoteEndRuUa[0], aStopSingleQuoteEndRuUa[1],  aStopSingleQuoteEndRuUa + 2 ) ) )
                    {
                        LocaleDataWrapper& rLcl = GetLocaleDataWrapper( eLang );
                        CharClass& rCC = GetCharClass( eLang );
                        if ( rLcl.getQuotationMarkStart() == OUStringChar(aStopSingleQuoteEndRuUa[0]) &&
                             // use apostrophe only after letters, not after digits or punctuation
                             rCC.isLetter(rTxt, nInsPos-1) )
                        {
                            eType = ACQuotes::UseApostrophe;
                        }
                    }
                }

                if ( eType == ACQuotes::NONE && !bSingle &&
                    ( primary(eLang) == primary(LANGUAGE_FRENCH) && eLang != LANGUAGE_FRENCH_SWISS ) )
                    eType = ACQuotes::NonBreakingSpace;

                InsertQuote( rDoc, nInsPos, cChar, bSttQuote, bInsert, eLang, eType );
                break;
            }
            // tdf#133524 change "<<" and ">>" to double angle quotation marks
            else if ( IsAutoCorrFlag( ACFlags::ChgQuotes ) &&
                IsAutoCorrFlag( ACFlags::ChgAngleQuotes ) &&
                ('<' == cChar || '>' == cChar) &&
                nInsPos > 0 && cChar == rTxt[ nInsPos-1 ] )
            {
                const LanguageType eLang = GetDocLanguage( rDoc, nInsPos );
                if ( eLang.anyOf(
                        LANGUAGE_CATALAN,              // primary level
                        LANGUAGE_CATALAN_VALENCIAN,    // primary level
                        LANGUAGE_FINNISH,              // alternative primary level
                        LANGUAGE_FRENCH_SWISS,         // second level
                        LANGUAGE_GALICIAN,             // primary level
                        LANGUAGE_HUNGARIAN,            // second level
                        LANGUAGE_POLISH,               // second level
                        LANGUAGE_PORTUGUESE,           // primary level
                        LANGUAGE_PORTUGUESE_BRAZILIAN, // primary level
                        LANGUAGE_ROMANIAN,             // second level
                        LANGUAGE_ROMANIAN_MOLDOVA,     // second level
                        LANGUAGE_SWEDISH,              // alternative primary level
                        LANGUAGE_SWEDISH_FINLAND,      // alternative primary level
                        LANGUAGE_UKRAINIAN,            // primary level
                        LANGUAGE_USER_ARAGONESE,       // primary level
                        LANGUAGE_USER_ASTURIAN ) ||    // primary level
                    primary(eLang) == primary(LANGUAGE_GERMAN) ||  // alternative primary level
                    primary(eLang) == primary(LANGUAGE_SPANISH) )  // primary level
                {
                    InsertQuote( rDoc, nInsPos, cChar, false, bInsert, eLang, ACQuotes::DoubleAngleQuote );
                    break;
                }
            }

            if( bInsert )
                rDoc.Insert( nInsPos, OUString(cChar) );
            else
                rDoc.Replace( nInsPos, OUString(cChar) );

            // Hardspaces autocorrection
            if ( IsAutoCorrFlag( ACFlags::AddNonBrkSpace ) )
            {
                // WARNING ATTENTION: rTxt is an alias of the text node's OUString
                // and its length may change (even become shorter) if FnAddNonBrkSpace succeeds!
                sal_Int32 nUpdatedPos = -1;
                if (NeedsHardspaceAutocorr(cChar))
                    nUpdatedPos = FnAddNonBrkSpace( rDoc, rTxt, nInsPos, GetDocLanguage( rDoc, nInsPos ), io_bNbspRunNext );
                if (nUpdatedPos >= 0)
                {
                    nInsPos = nUpdatedPos;
                }
                else if ( bIsNextRun && !IsAutoCorrectChar( cChar ) )
                {
                    // Remove the NBSP if it wasn't an autocorrection
                    if ( nInsPos != 0 && NeedsHardspaceAutocorr( rTxt[ nInsPos - 1 ] ) &&
                            cChar != ' ' && cChar != '\t' && cChar != cNonBreakingSpace )
                    {
                        // Look for the last HARD_SPACE
                        sal_Int32 nPos = nInsPos - 1;
                        bool bContinue = true;
                        while ( bContinue )
                        {
                            const sal_Unicode cTmpChar = rTxt[ nPos ];
                            if ( cTmpChar == cNonBreakingSpace )
                            {
                                rDoc.Delete( nPos, nPos + 1 );
                                bContinue = false;
                            }
                            else if ( !NeedsHardspaceAutocorr( cTmpChar ) || nPos == 0 )
                                bContinue = false;
                            nPos--;
                        }
                    }
                }
            }
        }

        if( !nInsPos )
            break;

        sal_Int32 nPos = nInsPos - 1;

        if( IsWordDelim( rTxt[ nPos ]))
            break;

        // Set bold or underline automatically?
        if (('*' == cChar || '_' == cChar || '/' == cChar || '-' == cChar) && (nPos+1 < rTxt.getLength()))
        {
            if( IsAutoCorrFlag( ACFlags::ChgWeightUnderl ) )
            {
                FnChgWeightUnderl( rDoc, rTxt, nPos+1 );
            }
            break;
        }

        while( nPos && !IsWordDelim( rTxt[ --nPos ]))
            ;

        // Found a Paragraph-start or a Blank, search for the word shortcut in
        // auto.
        sal_Int32 nCapLttrPos = nPos+1;        // on the 1st Character
        if( !nPos && !IsWordDelim( rTxt[ 0 ]))
            --nCapLttrPos;          // begin of paragraph and no blank

        const LanguageType eLang = GetDocLanguage( rDoc, nCapLttrPos );
        CharClass& rCC = GetCharClass( eLang );

        // no symbol characters
        if( lcl_IsSymbolChar( rCC, rTxt, nCapLttrPos, nInsPos ))
            break;

        if( IsAutoCorrFlag( ACFlags::Autocorrect ) &&
            // tdf#134940 fix regression of arrow "-->" resulted by premature
            // replacement of "--" since '>' was added to IsAutoCorrectChar()
            '>' != cChar )
        {
            // WARNING ATTENTION: rTxt is an alias of the text node's OUString
            // and becomes INVALID if ChgAutoCorrWord returns true!
            // => use aPara/pPara to create a valid copy of the string!
            OUString aPara;
            OUString* pPara = IsAutoCorrFlag(ACFlags::CapitalStartSentence) ? &aPara : nullptr;

            bool bChgWord = rDoc.ChgAutoCorrWord( nCapLttrPos, nInsPos,
                                                    *this, pPara );
            if( !bChgWord )
            {
                sal_Int32 nCapLttrPos1 = nCapLttrPos, nInsPos1 = nInsPos;
                while( nCapLttrPos1 < nInsPos &&
                        lcl_IsInArr( sImplSttSkipChars, rTxt[ nCapLttrPos1 ] )
                        )
                        ++nCapLttrPos1;
                while( nCapLttrPos1 < nInsPos1 && nInsPos1 &&
                        lcl_IsInArr( sImplEndSkipChars, rTxt[ nInsPos1-1 ] )
                        )
                        --nInsPos1;

                if( (nCapLttrPos1 != nCapLttrPos || nInsPos1 != nInsPos ) &&
                    nCapLttrPos1 < nInsPos1 &&
                    rDoc.ChgAutoCorrWord( nCapLttrPos1, nInsPos1, *this, pPara ))
                {
                    bChgWord = true;
                    nCapLttrPos = nCapLttrPos1;
                }
            }

            if( bChgWord )
            {
                if( !aPara.isEmpty() )
                {
                    sal_Int32 nEnd = nCapLttrPos;
                    while( nEnd < aPara.getLength() &&
                            !IsWordDelim( aPara[ nEnd ]))
                        ++nEnd;

                    // Capital letter at beginning of paragraph?
                    if( IsAutoCorrFlag( ACFlags::CapitalStartSentence ) )
                    {
                        FnCapitalStartSentence( rDoc, aPara, false,
                                                nCapLttrPos, nEnd, eLang );
                    }

                    if( IsAutoCorrFlag( ACFlags::ChgToEnEmDash ) )
                    {
                        FnChgToEnEmDash( rDoc, aPara, nCapLttrPos, nEnd, eLang );
                    }
                }
                break;
            }
        }

        if( IsAutoCorrFlag( ACFlags::TransliterateRTL ) && GetDocLanguage( rDoc, nInsPos ) == LANGUAGE_HUNGARIAN )
        {
            // WARNING ATTENTION: rTxt is an alias of the text node's OUString
            // and becomes INVALID if TransliterateRTLWord returns true!
            if ( rDoc.TransliterateRTLWord( nCapLttrPos, nInsPos ) )
                break;
        }

        if( ( IsAutoCorrFlag( ACFlags::ChgOrdinalNumber ) &&
                (nInsPos >= 2 ) &&       // fdo#69762 avoid autocorrect for 2e-3
                ( '-' != cChar || 'E' != rtl::toAsciiUpperCase(rTxt[nInsPos-1]) || '0' > rTxt[nInsPos-2] || '9' < rTxt[nInsPos-2] ) &&
                FnChgOrdinalNumber( rDoc, rTxt, nCapLttrPos, nInsPos, eLang ) ) ||
            ( IsAutoCorrFlag( ACFlags::SetINetAttr ) &&
                ( ' ' == cChar || '\t' == cChar || 0x0a == cChar || !cChar ) &&
                FnSetINetAttr( rDoc, rTxt, nCapLttrPos, nInsPos, eLang ) ) ||
            ( IsAutoCorrFlag( ACFlags::SetDOIAttr ) &&
                ( ' ' == cChar || '\t' == cChar || 0x0a == cChar || !cChar ) &&
                FnSetDOIAttr( rDoc, rTxt, nCapLttrPos, nInsPos, eLang ) ) )
            ;
        else
        {
            bool bLockKeyOn = pFrameWin && (pFrameWin->GetIndicatorState() & KeyIndicatorState::CAPSLOCK);
            bool bUnsupported = lcl_IsUnsupportedUnicodeChar( rCC, rTxt, nCapLttrPos, nInsPos );

            if ( bLockKeyOn && IsAutoCorrFlag( ACFlags::CorrectCapsLock ) &&
                 FnCorrectCapsLock( rDoc, rTxt, nCapLttrPos, nInsPos, eLang ) )
            {
                // Correct accidental use of cAPS LOCK key (do this only when
                // the caps or shift lock key is pressed). Turn off the caps
                // lock afterwards.
                pFrameWin->SimulateKeyPress( KEY_CAPSLOCK );
            }

            // Capital letter at beginning of paragraph ?
            if( !bUnsupported &&
                IsAutoCorrFlag( ACFlags::CapitalStartSentence ) )
            {
                FnCapitalStartSentence( rDoc, rTxt, true, nCapLttrPos, nInsPos, eLang );
            }

            // Two capital letters at beginning of word ??
            if( !bUnsupported &&
                IsAutoCorrFlag( ACFlags::CapitalStartWord ) )
            {
                FnCapitalStartWord( rDoc, rTxt, nCapLttrPos, nInsPos, eLang );
            }

            if( IsAutoCorrFlag( ACFlags::ChgToEnEmDash ) )
            {
                FnChgToEnEmDash( rDoc, rTxt, nCapLttrPos, nInsPos, eLang );
            }
        }

    } while( false );
}

SvxAutoCorrectLanguageLists& SvxAutoCorrect::GetLanguageList_(
                                                        LanguageType eLang )
{
    LanguageTag aLanguageTag( eLang);
    if (m_aLangTable.find(aLanguageTag) == m_aLangTable.end())
        (void)CreateLanguageFile(aLanguageTag);
    const auto iter = m_aLangTable.find(aLanguageTag);
    assert(iter != m_aLangTable.end());
    return iter->second;
}

void SvxAutoCorrect::SaveCplSttExceptList( LanguageType eLang )
{
    auto const iter = m_aLangTable.find(LanguageTag(eLang));
    if (iter != m_aLangTable.end())
        iter->second.SaveCplSttExceptList();
    else
    {
        SAL_WARN("editeng", "Save an empty list? ");
    }
}

void SvxAutoCorrect::SaveWordStartExceptList(LanguageType eLang)
{
    auto const iter = m_aLangTable.find(LanguageTag(eLang));
    if (iter != m_aLangTable.end())
        iter->second.SaveWordStartExceptList();
    else
    {
        SAL_WARN("editeng", "Save an empty list? ");
    }
}

// Adds a single word. The list will immediately be written to the file!
bool SvxAutoCorrect::AddCplSttException( const OUString& rNew,
                                        LanguageType eLang )
{
    SvxAutoCorrectLanguageLists* pLists = nullptr;
    // either the right language is present or it will be this in the general list
    auto iter = m_aLangTable.find(LanguageTag(eLang));
    if (iter != m_aLangTable.end())
        pLists = &iter->second;
    else
    {
        LanguageTag aLangTagUndetermined( LANGUAGE_UNDETERMINED);
        iter = m_aLangTable.find(aLangTagUndetermined);
        if (iter != m_aLangTable.end())
            pLists = &iter->second;
        else if(CreateLanguageFile(aLangTagUndetermined))
        {
            iter = m_aLangTable.find(aLangTagUndetermined);
            assert(iter != m_aLangTable.end());
            pLists = &iter->second;
        }
    }
    OSL_ENSURE(pLists, "No auto correction data");
    return pLists && pLists->AddToCplSttExceptList(rNew);
}

// Adds a single word. The list will immediately be written to the file!
bool SvxAutoCorrect::AddWordStartException( const OUString& rNew,
                                         LanguageType eLang )
{
    SvxAutoCorrectLanguageLists* pLists = nullptr;
    //either the right language is present or it is set in the general list
    auto iter = m_aLangTable.find(LanguageTag(eLang));
    if (iter != m_aLangTable.end())
        pLists = &iter->second;
    else
    {
        LanguageTag aLangTagUndetermined( LANGUAGE_UNDETERMINED);
        iter = m_aLangTable.find(aLangTagUndetermined);
        if (iter != m_aLangTable.end())
            pLists = &iter->second;
        else if(CreateLanguageFile(aLangTagUndetermined))
        {
            iter = m_aLangTable.find(aLangTagUndetermined);
            assert(iter != m_aLangTable.end());
            pLists = &iter->second;
        }
    }
    OSL_ENSURE(pLists, "No auto correction file!");
    return pLists && pLists->AddToWordStartExceptList(rNew);
}

OUString SvxAutoCorrect::GetPrevAutoCorrWord(SvxAutoCorrDoc const& rDoc, const OUString& rTxt,
                                             sal_Int32 nPos)
{
    OUString sRet;
    if( !nPos )
        return sRet;

    sal_Int32 nEnd = nPos;

    // it must be followed by a blank or tab!
    if( ( nPos < rTxt.getLength() &&
        !IsWordDelim( rTxt[ nPos ])) ||
        IsWordDelim( rTxt[ --nPos ]))
        return sRet;

    while( nPos && !IsWordDelim( rTxt[ --nPos ]))
        ;

    // Found a Paragraph-start or a Blank, search for the word shortcut in
    // auto.
    sal_Int32 nCapLttrPos = nPos+1;        // on the 1st Character
    if( !nPos && !IsWordDelim( rTxt[ 0 ]))
        --nCapLttrPos;          // Beginning of paragraph and no Blank!

    while( lcl_IsInArr( sImplSttSkipChars, rTxt[ nCapLttrPos ]) )
        if( ++nCapLttrPos >= nEnd )
            return sRet;

    if( 3 > nEnd - nCapLttrPos )
        return sRet;

    const LanguageType eLang = GetDocLanguage( rDoc, nCapLttrPos );

    CharClass& rCC = GetCharClass(eLang);

    if( lcl_IsSymbolChar( rCC, rTxt, nCapLttrPos, nEnd ))
        return sRet;

    sRet = rTxt.copy( nCapLttrPos, nEnd - nCapLttrPos );
    return sRet;
}

// static
std::vector<OUString> SvxAutoCorrect::GetChunkForAutoText(std::u16string_view rTxt,
                                                          const sal_Int32 nPos)
{
    constexpr sal_Int32 nMinLen = 3;
    constexpr sal_Int32 nMaxLen = 9;
    std::vector<OUString> aRes;
    if (nPos >= nMinLen)
    {
        sal_Int32 nBegin = std::max<sal_Int32>(nPos - nMaxLen, 0);
        // TODO: better detect word boundaries (not only whitespaces, but also e.g. punctuation)
        if (nBegin > 0 && !IsWordDelim(rTxt[nBegin-1]))
        {
            while (nBegin + nMinLen <= nPos && !IsWordDelim(rTxt[nBegin]))
                ++nBegin;
        }
        if (nBegin + nMinLen <= nPos)
        {
            OUString sRes( rTxt.substr(nBegin, nPos - nBegin) );
            aRes.push_back(sRes);
            bool bLastStartedWithDelim = IsWordDelim(sRes[0]);
            for (sal_Int32 i = 1; i <= sRes.getLength() - nMinLen; ++i)
            {
                bool bAdd = bLastStartedWithDelim;
                bLastStartedWithDelim = IsWordDelim(sRes[i]);
                bAdd = bAdd || bLastStartedWithDelim;
                if (bAdd)
                    aRes.push_back(sRes.copy(i));
            }
        }
    }
    return aRes;
}

bool SvxAutoCorrect::CreateLanguageFile( const LanguageTag& rLanguageTag, bool bNewFile )
{
    OSL_ENSURE(m_aLangTable.find(rLanguageTag) == m_aLangTable.end(), "Language already exists ");

    OUString sUserDirFile( GetAutoCorrFileName( rLanguageTag, true ));
    OUString sShareDirFile( sUserDirFile );

    SvxAutoCorrectLanguageLists* pLists = nullptr;

    tools::Time nAktTime(tools::Time::SYSTEM);

    auto nFndPos = aLastFileTable.find(rLanguageTag);
    bool lastCheckLessThan2MinutesAgo = nFndPos != aLastFileTable.end();
    if (lastCheckLessThan2MinutesAgo)
    {
        const tools::Time nLastCheckTime(tools::Time::fromEncodedTime(nFndPos->second));
        lastCheckLessThan2MinutesAgo
            = nLastCheckTime < nAktTime && nAktTime - nLastCheckTime < tools::Time(0, 2);
    }
    if (lastCheckLessThan2MinutesAgo)
    {
        // no need to test the file, because the last check is not older then
        // 2 minutes.
        if( bNewFile )
        {
            sShareDirFile = sUserDirFile;
            auto itBool = m_aLangTable.emplace(std::piecewise_construct,
                            std::forward_as_tuple(rLanguageTag),
                            std::forward_as_tuple(*this, sShareDirFile, sUserDirFile));
            pLists = &itBool.first->second;
            aLastFileTable.erase(nFndPos);
        }
    }
    else if(
             ( FStatHelper::IsDocument( sUserDirFile ) ||
               FStatHelper::IsDocument( sShareDirFile =
                   GetAutoCorrFileName( rLanguageTag ) ) ||
               FStatHelper::IsDocument( sShareDirFile =
                   GetAutoCorrFileName( rLanguageTag, false, false, true) )
             ) ||
        ( sShareDirFile = sUserDirFile, bNewFile )
          )
    {
        auto itBool = m_aLangTable.emplace(std::piecewise_construct,
                        std::forward_as_tuple(rLanguageTag),
                        std::forward_as_tuple(*this, sShareDirFile, sUserDirFile));
        pLists = &itBool.first->second;
        if (nFndPos != aLastFileTable.end())
            aLastFileTable.erase(nFndPos);
    }
    else if( !bNewFile )
    {
        aLastFileTable[rLanguageTag] = nAktTime.GetTime();
    }
    return pLists != nullptr;
}

bool SvxAutoCorrect::PutText( const OUString& rShort, const OUString& rLong,
                                LanguageType eLang )
{
    LanguageTag aLanguageTag( eLang);
    if (auto const iter = m_aLangTable.find(aLanguageTag); iter != m_aLangTable.end())
        return iter->second.PutText(rShort, rLong);
    if (CreateLanguageFile(aLanguageTag))
    {
        auto const iter = m_aLangTable.find(aLanguageTag);
        assert (iter != m_aLangTable.end());
        return iter->second.PutText(rShort, rLong);
    }
    return false;
}

void SvxAutoCorrect::MakeCombinedChanges( std::vector<SvxAutocorrWord>& aNewEntries,
                                              std::vector<SvxAutocorrWord>& aDeleteEntries,
                                              LanguageType eLang )
{
    LanguageTag aLanguageTag( eLang);
    auto iter = m_aLangTable.find(aLanguageTag);
    if (iter != m_aLangTable.end())
    {
        iter->second.MakeCombinedChanges( aNewEntries, aDeleteEntries );
    }
    else if(CreateLanguageFile( aLanguageTag ))
    {
        iter = m_aLangTable.find(aLanguageTag);
        assert(iter != m_aLangTable.end());
        iter->second.MakeCombinedChanges( aNewEntries, aDeleteEntries );
    }
}

//  - return the replacement text (only for SWG-Format, all other
//    can be taken from the word list!)
bool SvxAutoCorrect::GetLongText( const OUString&, OUString& )
{
    return false;
}

void SvxAutoCorrect::refreshBlockList( const uno::Reference< embed::XStorage >& )
{
}

// Text with attribution (only the SWG - SWG format!)
bool SvxAutoCorrect::PutText( const css::uno::Reference < css::embed::XStorage >&,
                              const OUString&, const OUString&, SfxObjectShell&, OUString& )
{
    return false;
}

OUString EncryptBlockName_Imp(std::u16string_view rName)
{
    OUStringBuffer aName;
    aName.append('#').append(rName);
    for (size_t nLen = rName.size(), nPos = 1; nPos < nLen; ++nPos)
    {
        if (lcl_IsInArr( u"!/:.\\", aName[nPos]))
            aName[nPos] &= 0x0f;
    }
    return aName.makeStringAndClear();
}

/* This code is copied from SwXMLTextBlocks::GeneratePackageName */
static void GeneratePackageName ( std::u16string_view rShort, OUString& rPackageName )
{
    OString sByte(OUStringToOString(rShort, RTL_TEXTENCODING_UTF7));
    OUStringBuffer aBuf(OStringToOUString(sByte, RTL_TEXTENCODING_ASCII_US));

    for (sal_Int32 nPos = 0; nPos < aBuf.getLength(); ++nPos)
    {
        switch (aBuf[nPos])
        {
            case '!':
            case '/':
            case ':':
            case '.':
            case '\\':
            // tdf#156769 - escape the question mark in the storage name
            case '?':
                aBuf[nPos] = '_';
                break;
            default:
                break;
        }
    }

    rPackageName = aBuf.makeStringAndClear();
}

static std::optional<SvxAutocorrWordList::WordSearchStatus>
lcl_SearchWordsInList( SvxAutoCorrectLanguageLists* pList,
                       std::u16string_view rTxt,
                       sal_Int32& rStt, sal_Int32 nEndPos )
{
    const SvxAutocorrWordList* pAutoCorrWordList = pList->GetAutocorrWordList();
    return pAutoCorrWordList->SearchWordsInList( rTxt, rStt, nEndPos );
}

// the search for the words in the substitution table
std::optional<SvxAutocorrWordList::WordSearchStatus>
SvxAutoCorrect::SearchWordsInList(
    std::u16string_view rTxt, sal_Int32& rStt, sal_Int32 nEndPos,
    SvxAutoCorrDoc&, LanguageTag& rLang )
{
    LanguageTag aLanguageTag( rLang);
    if( aLanguageTag.isSystemLocale() )
        aLanguageTag.reset( MsLangId::getConfiguredSystemLanguage());

    /* TODO-BCP47: this is so ugly, should all maybe be a proper fallback
     * list instead? */

    // First search for eLang, then US-English -> English
    // and last in LANGUAGE_UNDETERMINED
    if (m_aLangTable.find(aLanguageTag) != m_aLangTable.end() || CreateLanguageFile(aLanguageTag, false))
    {
        //the language is available - so bring it on
        const auto iter = m_aLangTable.find(aLanguageTag);
        assert(iter != m_aLangTable.end());
        SvxAutoCorrectLanguageLists & rList = iter->second;
        auto pRet = lcl_SearchWordsInList( &rList, rTxt, rStt, nEndPos );
        if( pRet )
        {
            rLang = aLanguageTag;
            return pRet;
        }
    }

    // If it still could not be found here, then keep on searching
    LanguageType eLang = aLanguageTag.getLanguageType();
    // the primary language for example EN
    aLanguageTag.reset(aLanguageTag.getLanguage());
    LanguageType nTmpKey = aLanguageTag.getLanguageType(false);
    if (nTmpKey != eLang && nTmpKey != LANGUAGE_UNDETERMINED &&
                (m_aLangTable.find(aLanguageTag) != m_aLangTable.end() ||
                 CreateLanguageFile(aLanguageTag, false)))
    {
        //the language is available - so bring it on
        const auto iter = m_aLangTable.find(aLanguageTag);
        assert(iter != m_aLangTable.end());
        SvxAutoCorrectLanguageLists& rList = iter->second;
        auto pRet = lcl_SearchWordsInList( &rList, rTxt, rStt, nEndPos );
        if( pRet )
        {
            rLang = aLanguageTag;
            return pRet;
        }
    }

    if (m_aLangTable.find(aLanguageTag.reset(LANGUAGE_UNDETERMINED)) != m_aLangTable.end() ||
            CreateLanguageFile(aLanguageTag, false))
    {
        //the language is available - so bring it on
        const auto iter = m_aLangTable.find(aLanguageTag);
        assert(iter != m_aLangTable.end());
        SvxAutoCorrectLanguageLists& rList = iter->second;
        auto pRet = lcl_SearchWordsInList( &rList, rTxt, rStt, nEndPos );
        if( pRet )
        {
            rLang = std::move(aLanguageTag);
            return pRet;
        }
    }
    return std::nullopt;
}

bool SvxAutoCorrect::SearchWordsNext(
    std::u16string_view rTxt, sal_Int32& rStt, sal_Int32 nEndPos,
    SvxAutocorrWordList::WordSearchStatus& rStatus )
{
    const SvxAutocorrWordList* pWordList = rStatus.GetAutocorrWordList();
    return pWordList->SearchWordsNext( rTxt, rStt, nEndPos, rStatus );
}

bool SvxAutoCorrect::FindInWordStartExceptList( LanguageType eLang,
                                             const OUString& sWord )
{
    LanguageTag aLanguageTag( eLang);

    /* TODO-BCP47: again horrible ugliness */

    // First search for eLang, then primary language of eLang
    // and last in LANGUAGE_UNDETERMINED

    if (m_aLangTable.find(aLanguageTag) != m_aLangTable.end() || CreateLanguageFile(aLanguageTag, false))
    {
        //the language is available - so bring it on
        const auto iter = m_aLangTable.find(aLanguageTag);
        assert(iter != m_aLangTable.end() && "CreateLanguageFile can't fail");
        auto& rList = iter->second;
        if(rList.GetWordStartExceptList()->find(sWord) != rList.GetWordStartExceptList()->end() )
            return true;
    }

    // If it still could not be found here, then keep on searching
    // the primary language for example EN
    aLanguageTag.reset(aLanguageTag.getLanguage());
    LanguageType nTmpKey = aLanguageTag.getLanguageType(false);
    if (nTmpKey != eLang && nTmpKey != LANGUAGE_UNDETERMINED &&
                (m_aLangTable.find(aLanguageTag) != m_aLangTable.end() ||
                 CreateLanguageFile(aLanguageTag, false)))
    {
        //the language is available - so bring it on
        const auto iter = m_aLangTable.find(aLanguageTag);
        assert(iter != m_aLangTable.end() && "CreateLanguageFile can't fail");
        auto& rList = iter->second;
        if(rList.GetWordStartExceptList()->find(sWord) != rList.GetWordStartExceptList()->end() )
            return true;
    }

    if (m_aLangTable.find(aLanguageTag.reset(LANGUAGE_UNDETERMINED)) != m_aLangTable.end() ||
            CreateLanguageFile(aLanguageTag, false))
    {
        //the language is available - so bring it on
        const auto iter = m_aLangTable.find(aLanguageTag);
        assert(iter != m_aLangTable.end());
        auto& rList = iter->second;
        if(rList.GetWordStartExceptList()->find(sWord) != rList.GetWordStartExceptList()->end() )
            return true;
    }
    return false;
}

static bool lcl_FindAbbreviation(const SvStringsISortDtor* pList, const OUString& sWord)
{
    SvStringsISortDtor::const_iterator it = pList->find(u"~"_ustr);
    SvStringsISortDtor::size_type nPos = it - pList->begin();
    if( nPos < pList->size() )
    {
        OUString sLowerWord(sWord.toAsciiLowerCase());
        OUString sAbr;
        for( SvStringsISortDtor::size_type n = nPos; n < pList->size(); ++n )
        {
            sAbr = (*pList)[ n ];
            if (sAbr[0] != '~')
                break;
            // ~ and ~. are not allowed!
            if( 2 < sAbr.getLength() && sAbr.getLength() - 1 <= sWord.getLength() )
            {
                OUString sLowerAbk(sAbr.toAsciiLowerCase());
                for (sal_Int32 i = sLowerAbk.getLength(), ii = sLowerWord.getLength(); i;)
                {
                    if( !--i )      // agrees
                        return true;

                    if( sLowerAbk[i] != sLowerWord[--ii])
                        break;
                }
            }
        }
    }
    OSL_ENSURE( !(nPos && '~' == (*pList)[ --nPos ][ 0 ] ),
            "Wrongly sorted exception list?" );
    return false;
}

bool SvxAutoCorrect::FindInCplSttExceptList(LanguageType eLang,
                                const OUString& sWord, bool bAbbreviation)
{
    LanguageTag aLanguageTag( eLang);

    /* TODO-BCP47: did I mention terrible horrible ugliness? */

    // First search for eLang, then primary language of eLang
    // and last in LANGUAGE_UNDETERMINED

    if (m_aLangTable.find(aLanguageTag) != m_aLangTable.end() || CreateLanguageFile(aLanguageTag, false))
    {
        //the language is available - so bring it on
        const auto iter = m_aLangTable.find(aLanguageTag);
        assert(iter != m_aLangTable.end() && "CreateLanguageFile can't fail");
        const SvStringsISortDtor* pList = iter->second.GetCplSttExceptList();
        if(bAbbreviation ? lcl_FindAbbreviation(pList, sWord) : pList->find(sWord) != pList->end() )
            return true;
    }

    // If it still could not be found here, then keep on searching
    // the primary language for example EN
    aLanguageTag.reset(aLanguageTag.getLanguage());
    LanguageType nTmpKey = aLanguageTag.getLanguageType(false);
    if (nTmpKey != eLang && nTmpKey != LANGUAGE_UNDETERMINED &&
                (m_aLangTable.find(aLanguageTag) != m_aLangTable.end() ||
                 CreateLanguageFile(aLanguageTag, false)))
    {
        //the language is available - so bring it on
        const auto iter = m_aLangTable.find(aLanguageTag);
        assert(iter != m_aLangTable.end() && "CreateLanguageFile can't fail");
        const SvStringsISortDtor* pList = iter->second.GetCplSttExceptList();
        if(bAbbreviation ? lcl_FindAbbreviation(pList, sWord) : pList->find(sWord) != pList->end() )
            return true;
    }

    if (m_aLangTable.find(aLanguageTag.reset(LANGUAGE_UNDETERMINED)) != m_aLangTable.end() ||
            CreateLanguageFile(aLanguageTag, false))
    {
        //the language is available - so bring it on
        const auto iter = m_aLangTable.find(aLanguageTag);
        assert(iter != m_aLangTable.end() && "CreateLanguageFile can't fail");
        const SvStringsISortDtor* pList = iter->second.GetCplSttExceptList();
        if(bAbbreviation ? lcl_FindAbbreviation(pList, sWord) : pList->find(sWord) != pList->end() )
            return true;
    }
    return false;
}

OUString SvxAutoCorrect::GetAutoCorrFileName( const LanguageTag& rLanguageTag,
                                            bool bNewFile, bool bTst, bool bUnlocalized ) const
{
    OUString sRet, sExt( rLanguageTag.getBcp47() );
    if (bUnlocalized)
    {
        // we don't want variant, so we'll take "fr" instead of "fr-CA" for example
        std::vector< OUString > vecFallBackStrings = rLanguageTag.getFallbackStrings(false);
        if (!vecFallBackStrings.empty())
           sExt = vecFallBackStrings[0];
    }

    sExt = "_" + sExt + ".dat";
    if( bNewFile )
        sRet = sUserAutoCorrFile + sExt;
    else if( !bTst )
        sRet = sShareAutoCorrFile + sExt;
    else
    {
        // test first in the user directory - if not exist, then
        sRet = sUserAutoCorrFile + sExt;
        if( !FStatHelper::IsDocument( sRet ))
            sRet = sShareAutoCorrFile + sExt;
    }
    return sRet;
}

SvxAutoCorrectLanguageLists::SvxAutoCorrectLanguageLists(
                SvxAutoCorrect& rParent,
                OUString aShareAutoCorrectFile,
                OUString aUserAutoCorrectFile)
:   sShareAutoCorrFile(std::move( aShareAutoCorrectFile )),
    sUserAutoCorrFile(std::move( aUserAutoCorrectFile )),
    aModifiedDate( Date::EMPTY ),
    aModifiedTime( tools::Time::EMPTY ),
    aLastCheckTime( tools::Time::EMPTY ),
    rAutoCorrect(rParent),
    nFlags(ACFlags::NONE)
{
}

SvxAutoCorrectLanguageLists::~SvxAutoCorrectLanguageLists()
{
}

bool SvxAutoCorrectLanguageLists::IsFileChanged_Imp()
{
    // Access the file system only every 2 minutes to check the date stamp
    bool bRet = false;

    tools::Time nMinTime( 0, 2 );
    tools::Time nAktTime( tools::Time::SYSTEM );
    if( aLastCheckTime <= nAktTime) // overflow?
        return false;
    nAktTime -= aLastCheckTime;
    if( nAktTime > nMinTime )     // min time past
    {
        Date aTstDate( Date::EMPTY ); tools::Time aTstTime( tools::Time::EMPTY );
        if( FStatHelper::GetModifiedDateTimeOfFile( sShareAutoCorrFile,
                                            &aTstDate, &aTstTime ) &&
            ( aModifiedDate != aTstDate || aModifiedTime != aTstTime ))
        {
            bRet = true;
            // then remove all the lists fast!
            if( (ACFlags::CplSttLstLoad & nFlags) && pCplStt_ExcptLst )
            {
                pCplStt_ExcptLst.reset();
            }
            if( (ACFlags::WordStartLstLoad & nFlags) && pWordStart_ExcptLst )
            {
                pWordStart_ExcptLst.reset();
            }
            if( (ACFlags::ChgWordLstLoad & nFlags) && pAutocorr_List )
            {
                pAutocorr_List.reset();
            }
            nFlags &= ~ACFlags(ACFlags::CplSttLstLoad | ACFlags::WordStartLstLoad | ACFlags::ChgWordLstLoad );
        }
        aLastCheckTime = tools::Time( tools::Time::SYSTEM );
    }
    return bRet;
}

void SvxAutoCorrectLanguageLists::LoadXMLExceptList_Imp(
                                        std::unique_ptr<SvStringsISortDtor>& rpLst,
                                        const OUString& sStrmName,
                                        rtl::Reference<SotStorage>& rStg)
{
    if( rpLst )
        rpLst->clear();
    else
        rpLst.reset( new SvStringsISortDtor );

    {
        if( rStg.is() && rStg->IsStream( sStrmName ) )
        {
            rtl::Reference<SotStorageStream> xStrm = rStg->OpenSotStream( sStrmName,
                ( StreamMode::READ | StreamMode::SHARE_DENYWRITE | StreamMode::NOCREATE ) );
            if( ERRCODE_NONE != xStrm->GetError())
            {
                xStrm.clear();
                rStg.clear();
                RemoveStream_Imp( sStrmName );
            }
            else
            {
                const uno::Reference< uno::XComponentContext >& xContext =
                    comphelper::getProcessComponentContext();

                xml::sax::InputSource aParserInput;
                aParserInput.sSystemId = sStrmName;

                xStrm->Seek( 0 );
                xStrm->SetBufferSize( 8 * 1024 );
                aParserInput.aInputStream = new utl::OInputStreamWrapper( *xStrm );

                // get filter
                uno::Reference< xml::sax::XFastDocumentHandler > xFilter = new SvXMLExceptionListImport ( xContext, *rpLst );

                // connect parser and filter
                uno::Reference< xml::sax::XFastParser > xParser = xml::sax::FastParser::create( xContext );
                uno::Reference<xml::sax::XFastTokenHandler> xTokenHandler = new SvXMLAutoCorrectTokenHandler;
                xParser->setFastDocumentHandler( xFilter );
                xParser->registerNamespace( u"http://openoffice.org/2001/block-list"_ustr, SvXMLAutoCorrectToken::NAMESPACE );
                xParser->setTokenHandler( xTokenHandler );

                // parse
                try
                {
                    xParser->parseStream( aParserInput );
                }
                catch( const xml::sax::SAXParseException& )
                {
                    // re throw ?
                }
                catch( const xml::sax::SAXException& )
                {
                    // re throw ?
                }
                catch( const io::IOException& )
                {
                    // re throw ?
                }
            }
        }

        // Set time stamp
        FStatHelper::GetModifiedDateTimeOfFile( sShareAutoCorrFile,
                                        &aModifiedDate, &aModifiedTime );
        aLastCheckTime = tools::Time( tools::Time::SYSTEM );
    }

}

void SvxAutoCorrectLanguageLists::SaveExceptList_Imp(
                            const SvStringsISortDtor& rLst,
                            const OUString& sStrmName,
                            rtl::Reference<SotStorage> const &rStg,
                            bool bConvert )
{
    if( !rStg.is() )
        return;

    if( rLst.empty() )
    {
        rStg->Remove( sStrmName );
        rStg->Commit();
    }
    else
    {
        rtl::Reference<SotStorageStream> xStrm = rStg->OpenSotStream( sStrmName,
                ( StreamMode::READ | StreamMode::WRITE | StreamMode::SHARE_DENYWRITE ) );
        if( xStrm.is() )
        {
            xStrm->SetSize( 0 );
            xStrm->SetBufferSize( 8192 );
            xStrm->SetProperty( u"MediaType"_ustr, Any(u"text/xml"_ustr) );


            const uno::Reference< uno::XComponentContext >& xContext =
                comphelper::getProcessComponentContext();

            uno::Reference < xml::sax::XWriter > xWriter  = xml::sax::Writer::create(xContext);
            uno::Reference < io::XOutputStream> xOut = new utl::OOutputStreamWrapper( *xStrm );
            xWriter->setOutputStream(xOut);

            uno::Reference < xml::sax::XDocumentHandler > xHandler(xWriter, UNO_QUERY_THROW);
            rtl::Reference< SvXMLExceptionListExport > xExp( new SvXMLExceptionListExport( xContext, rLst, sStrmName, xHandler ) );

            xExp->exportDoc( XML_BLOCK_LIST );

            xStrm->Commit();
            if( xStrm->GetError() == ERRCODE_NONE )
            {
                xStrm.clear();
                if (!bConvert)
                {
                    rStg->Commit();
                    if( ERRCODE_NONE != rStg->GetError() )
                    {
                        rStg->Remove( sStrmName );
                        rStg->Commit();
                    }
                }
            }
        }
    }
}

SvxAutocorrWordList* SvxAutoCorrectLanguageLists::LoadAutocorrWordList()
{
    if( pAutocorr_List )
        pAutocorr_List->DeleteAndDestroyAll();
    else
        pAutocorr_List.reset( new SvxAutocorrWordList() );

    try
    {
        uno::Reference < embed::XStorage > xStg = comphelper::OStorageHelper::GetStorageFromURL( sShareAutoCorrFile, embed::ElementModes::READ );
        uno::Reference < io::XStream > xStrm = xStg->openStreamElement( pXMLImplAutocorr_ListStr, embed::ElementModes::READ );
        const uno::Reference< uno::XComponentContext >& xContext = comphelper::getProcessComponentContext();

        xml::sax::InputSource aParserInput;
        aParserInput.sSystemId = pXMLImplAutocorr_ListStr;
        aParserInput.aInputStream = xStrm->getInputStream();

        // get parser
        uno::Reference< xml::sax::XFastParser > xParser = xml::sax::FastParser::create(xContext);
        SAL_INFO("editeng", "AutoCorrect Import" );
        uno::Reference< xml::sax::XFastDocumentHandler > xFilter = new SvXMLAutoCorrectImport( xContext, pAutocorr_List.get(), rAutoCorrect, xStg );
        uno::Reference<xml::sax::XFastTokenHandler> xTokenHandler = new SvXMLAutoCorrectTokenHandler;

        // connect parser and filter
        xParser->setFastDocumentHandler( xFilter );
        xParser->registerNamespace( u"http://openoffice.org/2001/block-list"_ustr, SvXMLAutoCorrectToken::NAMESPACE );
        xParser->setTokenHandler(xTokenHandler);

        // parse
        xParser->parseStream( aParserInput );
    }
    catch ( const uno::Exception& )
    {
        TOOLS_WARN_EXCEPTION("editeng", "when loading " << sShareAutoCorrFile);
    }

    // Set time stamp
    FStatHelper::GetModifiedDateTimeOfFile( sShareAutoCorrFile,
                                    &aModifiedDate, &aModifiedTime );
    aLastCheckTime = tools::Time( tools::Time::SYSTEM );

    return pAutocorr_List.get();
}

const SvxAutocorrWordList* SvxAutoCorrectLanguageLists::GetAutocorrWordList()
{
    if( !( ACFlags::ChgWordLstLoad & nFlags ) || IsFileChanged_Imp() )
    {
        LoadAutocorrWordList();
        if( !pAutocorr_List )
        {
            OSL_ENSURE( false, "No valid list" );
            pAutocorr_List.reset( new SvxAutocorrWordList() );
        }
        nFlags |= ACFlags::ChgWordLstLoad;
    }
    return pAutocorr_List.get();
}

SvStringsISortDtor* SvxAutoCorrectLanguageLists::GetCplSttExceptList()
{
    if( !( ACFlags::CplSttLstLoad & nFlags ) || IsFileChanged_Imp() )
    {
        LoadCplSttExceptList();
        if( !pCplStt_ExcptLst )
        {
            OSL_ENSURE( false, "No valid list" );
            pCplStt_ExcptLst.reset( new SvStringsISortDtor );
        }
        nFlags |= ACFlags::CplSttLstLoad;
    }
    return pCplStt_ExcptLst.get();
}

bool SvxAutoCorrectLanguageLists::AddToCplSttExceptList(const OUString& rNew)
{
    bool bRet = false;
    if( !rNew.isEmpty() && GetCplSttExceptList()->insert( rNew ).second )
    {
        MakeUserStorage_Impl();
        rtl::Reference<SotStorage> xStg = new SotStorage(sUserAutoCorrFile, StreamMode::READWRITE);

        SaveExceptList_Imp( *pCplStt_ExcptLst, pXMLImplCplStt_ExcptLstStr, xStg );

        xStg = nullptr;
        // Set time stamp
        FStatHelper::GetModifiedDateTimeOfFile( sUserAutoCorrFile,
                                            &aModifiedDate, &aModifiedTime );
        aLastCheckTime = tools::Time( tools::Time::SYSTEM );
        bRet = true;
    }
    return bRet;
}

bool SvxAutoCorrectLanguageLists::AddToWordStartExceptList(const OUString& rNew)
{
    bool bRet = false;
    if( !rNew.isEmpty() && GetWordStartExceptList()->insert( rNew ).second )
    {
        MakeUserStorage_Impl();
        rtl::Reference<SotStorage> xStg = new SotStorage(sUserAutoCorrFile, StreamMode::READWRITE);

        SaveExceptList_Imp( *pWordStart_ExcptLst, pXMLImplWordStart_ExcptLstStr, xStg );

        xStg = nullptr;
        // Set time stamp
        FStatHelper::GetModifiedDateTimeOfFile( sUserAutoCorrFile,
                                            &aModifiedDate, &aModifiedTime );
        aLastCheckTime = tools::Time( tools::Time::SYSTEM );
        bRet = true;
    }
    return bRet;
}

SvStringsISortDtor* SvxAutoCorrectLanguageLists::LoadCplSttExceptList()
{
    try
    {
        rtl::Reference<SotStorage> xStg = new SotStorage( sShareAutoCorrFile, StreamMode::READ | StreamMode::SHARE_DENYNONE );
        if( xStg.is() && xStg->IsContained( pXMLImplCplStt_ExcptLstStr ) )
            LoadXMLExceptList_Imp( pCplStt_ExcptLst, pXMLImplCplStt_ExcptLstStr, xStg );
    }
    catch (const css::ucb::ContentCreationException&)
    {
    }
    return pCplStt_ExcptLst.get();
}

void SvxAutoCorrectLanguageLists::SaveCplSttExceptList()
{
    MakeUserStorage_Impl();
    rtl::Reference<SotStorage> xStg = new SotStorage(sUserAutoCorrFile, StreamMode::READWRITE);

    SaveExceptList_Imp( *pCplStt_ExcptLst, pXMLImplCplStt_ExcptLstStr, xStg );

    xStg = nullptr;

    // Set time stamp
    FStatHelper::GetModifiedDateTimeOfFile( sUserAutoCorrFile,
                                            &aModifiedDate, &aModifiedTime );
    aLastCheckTime = tools::Time( tools::Time::SYSTEM );
}

SvStringsISortDtor* SvxAutoCorrectLanguageLists::LoadWordStartExceptList()
{
    try
    {
        rtl::Reference<SotStorage> xStg = new SotStorage( sShareAutoCorrFile, StreamMode::READ | StreamMode::SHARE_DENYNONE );
        if( xStg.is() && xStg->IsContained( pXMLImplWordStart_ExcptLstStr ) )
            LoadXMLExceptList_Imp( pWordStart_ExcptLst, pXMLImplWordStart_ExcptLstStr, xStg );
    }
    catch (const css::ucb::ContentCreationException &)
    {
        TOOLS_WARN_EXCEPTION("editeng", "SvxAutoCorrectLanguageLists::LoadWordStartExceptList");
    }
    return pWordStart_ExcptLst.get();
}

void SvxAutoCorrectLanguageLists::SaveWordStartExceptList()
{
    MakeUserStorage_Impl();
    rtl::Reference<SotStorage> xStg = new SotStorage(sUserAutoCorrFile, StreamMode::READWRITE);

    SaveExceptList_Imp( *pWordStart_ExcptLst, pXMLImplWordStart_ExcptLstStr, xStg );

    xStg = nullptr;
    // Set time stamp
    FStatHelper::GetModifiedDateTimeOfFile( sUserAutoCorrFile,
                                            &aModifiedDate, &aModifiedTime );
    aLastCheckTime = tools::Time( tools::Time::SYSTEM );
}

SvStringsISortDtor* SvxAutoCorrectLanguageLists::GetWordStartExceptList()
{
    if( !( ACFlags::WordStartLstLoad & nFlags ) || IsFileChanged_Imp() )
    {
        LoadWordStartExceptList();
        if( !pWordStart_ExcptLst )
        {
            OSL_ENSURE( false, "No valid list" );
            pWordStart_ExcptLst.reset( new SvStringsISortDtor );
        }
        nFlags |= ACFlags::WordStartLstLoad;
    }
    return pWordStart_ExcptLst.get();
}

void SvxAutoCorrectLanguageLists::RemoveStream_Imp( const OUString& rName )
{
    if( sShareAutoCorrFile != sUserAutoCorrFile )
    {
        rtl::Reference<SotStorage> xStg = new SotStorage(sUserAutoCorrFile, StreamMode::READWRITE);
        if( xStg.is() && ERRCODE_NONE == xStg->GetError() &&
            xStg->IsStream( rName ) )
        {
            xStg->Remove( rName );
            xStg->Commit();

            xStg = nullptr;
        }
    }
}

void SvxAutoCorrectLanguageLists::MakeUserStorage_Impl()
{
    // The conversion needs to happen if the file is already in the user
    // directory and is in the old format. Additionally it needs to
    // happen when the file is being copied from share to user.

    bool bError = false, bConvert = false, bCopy = false;
    INetURLObject aDest;
    INetURLObject aSource;

    if (sUserAutoCorrFile != sShareAutoCorrFile )
    {
        aSource = INetURLObject ( sShareAutoCorrFile );
        aDest = INetURLObject ( sUserAutoCorrFile );
        if ( SotStorage::IsOLEStorage ( sShareAutoCorrFile ) )
        {
            aDest.SetExtension ( u"bak" );
            bConvert = true;
        }
        bCopy = true;
    }
    else if ( SotStorage::IsOLEStorage ( sUserAutoCorrFile ) )
    {
        aSource = INetURLObject ( sUserAutoCorrFile );
        aDest = INetURLObject ( sUserAutoCorrFile );
        aDest.SetExtension ( u"bak" );
        bCopy = bConvert = true;
    }
    if (bCopy)
    {
        try
        {
            OUString sMain(aDest.GetMainURL( INetURLObject::DecodeMechanism::ToIUri ));
            sal_Int32 nSlashPos = sMain.lastIndexOf('/');
            sMain = sMain.copy(0, nSlashPos);
            ::ucbhelper::Content aNewContent( sMain, uno::Reference< XCommandEnvironment >(), comphelper::getProcessComponentContext() );
            TransferInfo aInfo;
            aInfo.NameClash = NameClash::OVERWRITE;
            aInfo.NewTitle = aDest.GetLastName();
            aInfo.SourceURL = aSource.GetMainURL( INetURLObject::DecodeMechanism::ToIUri );
            aInfo.MoveData  = false;
            aNewContent.executeCommand( u"transfer"_ustr, Any(aInfo));
        }
        catch (...)
        {
            bError = true;
        }
    }
    if (bConvert && !bError)
    {
        rtl::Reference<SotStorage> xSrcStg = new SotStorage( aDest.GetMainURL( INetURLObject::DecodeMechanism::ToIUri ), StreamMode::READ );
        rtl::Reference<SotStorage> xDstStg = new SotStorage(sUserAutoCorrFile, StreamMode::WRITE);

        if( xSrcStg.is() && xDstStg.is() )
        {
            std::unique_ptr<SvStringsISortDtor> pTmpWordList;

            if (xSrcStg->IsContained( pXMLImplWordStart_ExcptLstStr ) )
                LoadXMLExceptList_Imp( pTmpWordList, pXMLImplWordStart_ExcptLstStr, xSrcStg );

            if (pTmpWordList)
            {
                SaveExceptList_Imp( *pTmpWordList, pXMLImplWordStart_ExcptLstStr, xDstStg, true );
                pTmpWordList.reset();
            }


            if (xSrcStg->IsContained( pXMLImplCplStt_ExcptLstStr ) )
                LoadXMLExceptList_Imp( pTmpWordList, pXMLImplCplStt_ExcptLstStr, xSrcStg );

            if (pTmpWordList)
            {
                SaveExceptList_Imp( *pTmpWordList, pXMLImplCplStt_ExcptLstStr, xDstStg, true );
                pTmpWordList->clear();
            }

            GetAutocorrWordList();
            MakeBlocklist_Imp( *xDstStg );
            sShareAutoCorrFile = sUserAutoCorrFile;
            xDstStg = nullptr;
            try
            {
                ::ucbhelper::Content aContent ( aDest.GetMainURL( INetURLObject::DecodeMechanism::ToIUri ), uno::Reference < XCommandEnvironment >(), comphelper::getProcessComponentContext() );
                aContent.executeCommand ( u"delete"_ustr, Any ( true ) );
            }
            catch (...)
            {
            }
        }
    }
    else if( bCopy && !bError )
        sShareAutoCorrFile = sUserAutoCorrFile;
}

bool SvxAutoCorrectLanguageLists::MakeBlocklist_Imp( SotStorage& rStg )
{
    bool bRet = true, bRemove = !pAutocorr_List || pAutocorr_List->empty();
    if( !bRemove )
    {
        rtl::Reference<SotStorageStream> refList = rStg.OpenSotStream( pXMLImplAutocorr_ListStr,
                    ( StreamMode::READ | StreamMode::WRITE | StreamMode::SHARE_DENYWRITE ) );
        if( refList.is() )
        {
            refList->SetSize( 0 );
            refList->SetBufferSize( 8192 );
            refList->SetProperty( u"MediaType"_ustr, Any(u"text/xml"_ustr) );

            const uno::Reference< uno::XComponentContext >& xContext =
                comphelper::getProcessComponentContext();

            uno::Reference < xml::sax::XWriter > xWriter = xml::sax::Writer::create(xContext);
            uno::Reference < io::XOutputStream> xOut = new utl::OOutputStreamWrapper( *refList );
            xWriter->setOutputStream(xOut);

            rtl::Reference< SvXMLAutoCorrectExport > xExp( new SvXMLAutoCorrectExport( xContext, pAutocorr_List.get(), pXMLImplAutocorr_ListStr, xWriter ) );

            xExp->exportDoc( XML_BLOCK_LIST );

            refList->Commit();
            bRet = ERRCODE_NONE == refList->GetError();
            if( bRet )
            {
                refList.clear();
                rStg.Commit();
                if( ERRCODE_NONE != rStg.GetError() )
                {
                    bRemove = true;
                    bRet = false;
                }
            }
        }
        else
            bRet = false;
    }

    if( bRemove )
    {
        rStg.Remove( pXMLImplAutocorr_ListStr );
        rStg.Commit();
    }

    return bRet;
}

bool SvxAutoCorrectLanguageLists::MakeCombinedChanges( std::vector<SvxAutocorrWord>& aNewEntries, std::vector<SvxAutocorrWord>& aDeleteEntries )
{
    // First get the current list!
    GetAutocorrWordList();

    MakeUserStorage_Impl();
    rtl::Reference<SotStorage> xStorage = new SotStorage(sUserAutoCorrFile, StreamMode::READWRITE);

    bool bRet = xStorage.is() && ERRCODE_NONE == xStorage->GetError();

    if( bRet )
    {
        for (SvxAutocorrWord & aWordToDelete : aDeleteEntries)
        {
            std::optional<SvxAutocorrWord> xFoundEntry = pAutocorr_List->FindAndRemove( &aWordToDelete );
            if( xFoundEntry )
            {
                if( !xFoundEntry->IsTextOnly() )
                {
                    OUString aName( aWordToDelete.GetShort() );
                    if (xStorage->IsOLEStorage())
                        aName = EncryptBlockName_Imp(aName);
                    else
                        GeneratePackageName ( aWordToDelete.GetShort(), aName );

                    if( xStorage->IsContained( aName ) )
                    {
                        xStorage->Remove( aName );
                        bRet = xStorage->Commit();
                    }
                }
            }
        }

        for (const SvxAutocorrWord & aNewEntrie : aNewEntries)
        {
            SvxAutocorrWord aWordToAdd(aNewEntrie.GetShort(), aNewEntrie.GetLong(), true );
            std::optional<SvxAutocorrWord> xRemoved = pAutocorr_List->FindAndRemove( &aWordToAdd );
            if( xRemoved )
            {
                if( !xRemoved->IsTextOnly() )
                {
                    // Still have to remove the Storage
                    OUString sStorageName( aWordToAdd.GetShort() );
                    if (xStorage->IsOLEStorage())
                        sStorageName = EncryptBlockName_Imp(sStorageName);
                    else
                        GeneratePackageName ( aWordToAdd.GetShort(), sStorageName);

                    if( xStorage->IsContained( sStorageName ) )
                        xStorage->Remove( sStorageName );
                }
            }
            bRet = pAutocorr_List->Insert( std::move(aWordToAdd) );

            if ( !bRet )
            {
                break;
            }
        }

        if ( bRet )
        {
            bRet = MakeBlocklist_Imp( *xStorage );
        }
    }
    return bRet;
}

bool SvxAutoCorrectLanguageLists::PutText( const OUString& rShort, const OUString& rLong )
{
    // First get the current list!
    GetAutocorrWordList();

    MakeUserStorage_Impl();
    rtl::Reference<SotStorage> xStg = new SotStorage(sUserAutoCorrFile, StreamMode::READWRITE);

    bool bRet = xStg.is() && ERRCODE_NONE == xStg->GetError();

    // Update the word list
    if( bRet )
    {
        SvxAutocorrWord aNew(rShort, rLong, true );
        std::optional<SvxAutocorrWord> xRemove = pAutocorr_List->FindAndRemove( &aNew );
        if( xRemove )
        {
            if( !xRemove->IsTextOnly() )
            {
                // Still have to remove the Storage
                OUString sStgNm( rShort );
                if (xStg->IsOLEStorage())
                    sStgNm = EncryptBlockName_Imp(sStgNm);
                else
                    GeneratePackageName ( rShort, sStgNm);

                if( xStg->IsContained( sStgNm ) )
                    xStg->Remove( sStgNm );
            }
        }

        if( pAutocorr_List->Insert( std::move(aNew) ) )
        {
            bRet = MakeBlocklist_Imp( *xStg );
            xStg = nullptr;
        }
        else
        {
            bRet = false;
        }
    }
    return bRet;
}

void SvxAutoCorrectLanguageLists::PutText( const OUString& rShort,
                                               SfxObjectShell& rShell )
{
    // First get the current list!
    GetAutocorrWordList();

    MakeUserStorage_Impl();

    try
    {
        uno::Reference < embed::XStorage > xStg = comphelper::OStorageHelper::GetStorageFromURL( sUserAutoCorrFile, embed::ElementModes::READWRITE );
        OUString sLong;
        bool bRet = rAutoCorrect.PutText( xStg, sUserAutoCorrFile, rShort, rShell, sLong );
        xStg = nullptr;

        // Update the word list
        if( bRet )
        {
            if( pAutocorr_List->Insert( SvxAutocorrWord(rShort, sLong, false) ) )
            {
                rtl::Reference<SotStorage> xStor = new SotStorage( sUserAutoCorrFile, StreamMode::READWRITE );
                MakeBlocklist_Imp( *xStor );
            }
        }
    }
    catch ( const uno::Exception& )
    {
    }
}

// Keep the list sorted ...
struct SvxAutocorrWordList::CompareSvxAutocorrWordList
{
    bool operator()( SvxAutocorrWord const & lhs, SvxAutocorrWord const & rhs ) const
    {
        CollatorWrapper& rCmp = ::GetCollatorWrapper();
        return rCmp.compareString( lhs.GetShort(), rhs.GetShort() ) < 0;
    }
};

namespace {

typedef std::unordered_map<OUString, SvxAutocorrWord> AutocorrWordHashType;

}

struct SvxAutocorrWordList::Impl
{

    // only one of these contains the data
    // maSortedVector is manually sorted so we can optimise data movement
    mutable AutocorrWordSetType maSortedVector;
    mutable AutocorrWordHashType maHash; // key is 'Short'

    void DeleteAndDestroyAll()
    {
        maHash.clear();
        maSortedVector.clear();
    }
};

SvxAutocorrWordList::SvxAutocorrWordList() : mpImpl(new Impl) {}

SvxAutocorrWordList::~SvxAutocorrWordList()
{
}

void SvxAutocorrWordList::DeleteAndDestroyAll()
{
    mpImpl->DeleteAndDestroyAll();
}

struct SvxAutocorrWordList::Iterator::Impl {
    typedef SvxAutocorrWordList::AutocorrWordSetType::const_iterator VecIterType;
    typedef AutocorrWordHashType::const_iterator HashIterType;

    HashIterType  mHashIter, mHashEnd;
    VecIterType   mSortedVectorIter, mSortedVectorEnd;

    Impl(const HashIterType& hashIter, const HashIterType& hashEnd,
         const VecIterType& vecIter, const VecIterType& vecEnd)
        : mHashIter(hashIter), mHashEnd(hashEnd),
          mSortedVectorIter(vecIter), mSortedVectorEnd(vecEnd) {}

    bool Step() {
        // Iterate hash table, followed by sorted vector
        if (mHashIter != mHashEnd) {
            return ++mHashIter != mHashEnd
                   || mSortedVectorIter != mSortedVectorEnd;
        }
        return ++mSortedVectorIter != mSortedVectorEnd;
    }

    const SvxAutocorrWord& operator*() {
        return (mHashIter == mHashEnd) ? *mSortedVectorIter : mHashIter->second;
    }
    const SvxAutocorrWord* operator->() {
        return (mHashIter == mHashEnd) ? &*mSortedVectorIter : &mHashIter->second;
    }
};

SvxAutocorrWordList::Iterator::Iterator(
    std::unique_ptr<SvxAutocorrWordList::Iterator::Impl> pImpl
) : mpImpl(std::move(pImpl))
{
}

SvxAutocorrWordList::Iterator::Iterator(
    const SvxAutocorrWordList::Iterator& it
) : mpImpl(new Impl(*(it.mpImpl)))
{
}

SvxAutocorrWordList::Iterator::~Iterator()
{
}

bool SvxAutocorrWordList::Iterator::Step()
{
    return mpImpl->Step();
}

const SvxAutocorrWord& SvxAutocorrWordList::Iterator::operator*() const
{
    return **mpImpl;
}

const SvxAutocorrWord* SvxAutocorrWordList::Iterator::operator->() const
{
    return mpImpl->operator->();
}

bool SvxAutocorrWordList::ContainsPattern(const OUString& aShort) const
{
    // check hash table first
    if (mpImpl->maHash.contains(aShort)) {
        return true;
    }

    // then do binary search on sorted vector
    CollatorWrapper& rCmp = ::GetCollatorWrapper();
    auto it = std::lower_bound(mpImpl->maSortedVector.begin(),
                               mpImpl->maSortedVector.end(),
                               aShort,
                               [&](const SvxAutocorrWord& elm,
                                   const OUString& val) {
                                       return rCmp.compareString(elm.GetShort(),
                                                                 val) < 0;
                                   } );
    if (it != mpImpl->maSortedVector.end()
        && rCmp.compareString(aShort, it->GetShort()) == 0)
    {
        return true;
    }

    return false;
}

// returns true if inserted
const SvxAutocorrWord* SvxAutocorrWordList::Insert(SvxAutocorrWord aWord) const
{
    if ( mpImpl->maSortedVector.empty() ) // use the hash
    {
        OUString aShort = aWord.GetShort();
        auto [it,inserted] = mpImpl->maHash.emplace( std::move(aShort), std::move(aWord) );
        if (inserted)
            return &(it->second);
        return nullptr;
    }
    else
    {
        auto it = std::lower_bound(mpImpl->maSortedVector.begin(), mpImpl->maSortedVector.end(), aWord, CompareSvxAutocorrWordList());
        CollatorWrapper& rCmp = ::GetCollatorWrapper();
        if (it == mpImpl->maSortedVector.end() || rCmp.compareString( aWord.GetShort(), it->GetShort() ) != 0)
        {
            it = mpImpl->maSortedVector.insert(it, std::move(aWord));
            return &*it;
        }
        return nullptr;
    }
}

void SvxAutocorrWordList::LoadEntry(const OUString& sWrong, const OUString& sRight, bool bOnlyTxt)
{
    (void)Insert(SvxAutocorrWord( sWrong, sRight, bOnlyTxt ));
}

bool SvxAutocorrWordList::empty() const
{
    return mpImpl->maHash.empty() && mpImpl->maSortedVector.empty();
}

std::optional<SvxAutocorrWord> SvxAutocorrWordList::FindAndRemove(const SvxAutocorrWord *pWord)
{

    if ( mpImpl->maSortedVector.empty() ) // use the hash
    {
        AutocorrWordHashType::iterator it = mpImpl->maHash.find( pWord->GetShort() );
        if( it != mpImpl->maHash.end() )
        {
            SvxAutocorrWord pMatch = std::move(it->second);
            mpImpl->maHash.erase (it);
            return pMatch;
        }
    }
    else
    {
        auto it = std::lower_bound(mpImpl->maSortedVector.begin(), mpImpl->maSortedVector.end(), *pWord, CompareSvxAutocorrWordList());
        if (it != mpImpl->maSortedVector.end() && !CompareSvxAutocorrWordList()(*pWord, *it))
        {
            SvxAutocorrWord pMatch = std::move(*it);
            mpImpl->maSortedVector.erase (it);
            return pMatch;
        }
    }
    return std::optional<SvxAutocorrWord>();
}

// return the sorted contents - defer sorting until we have to.
const SvxAutocorrWordList::AutocorrWordSetType& SvxAutocorrWordList::getSortedContent() const
{
    // convert from hash to set permanently
    if ( mpImpl->maSortedVector.empty() )
    {
        std::vector<SvxAutocorrWord> tmp;
        tmp.reserve(mpImpl->maHash.size());
        for (auto & rPair : mpImpl->maHash)
            tmp.emplace_back(std::move(rPair.second));
        mpImpl->maHash.clear();
        // sort twice - this gets the list into mostly-sorted order, which
        // reduces the number of times we need to invoke the expensive ICU collate fn.
        std::sort(tmp.begin(), tmp.end(),
            [] ( SvxAutocorrWord const & lhs, SvxAutocorrWord const & rhs )
            {
                return lhs.GetShort() < rhs.GetShort();
            });
        // This beast has some O(N log(N)) in a terribly slow ICU collate fn.
        // stable_sort is twice as fast as sort in this situation because it does
        // fewer comparison operations.
        std::stable_sort(tmp.begin(), tmp.end(), CompareSvxAutocorrWordList());
        mpImpl->maSortedVector = std::move(tmp);
    }
    return mpImpl->maSortedVector;
}

std::optional<SvxAutocorrWord>
SvxAutocorrWordList::WordMatches(const SvxAutocorrWord *pFnd,
                                 std::u16string_view rTxt,
                                 sal_Int32 &rStt,
                                 sal_Int32 nEndPos) const
{
    const OUString& rChk = pFnd->GetShort();

    sal_Int32 left_wildcard = rChk.startsWith( ".*" ) ? 2 : 0; // ".*word" pattern?
    sal_Int32 right_wildcard = rChk.endsWith( ".*" ) ? 2 : 0; // "word.*" pattern?
    assert(nEndPos >= 0);
    size_t nSttWdPos = nEndPos;

    // direct replacement of keywords surrounded by colons (for example, ":name:")
    bool bColonNameColon = static_cast<sal_Int32>(rTxt.size()) > nEndPos &&
        rTxt[nEndPos] == ':' && rChk[0] == ':' && rChk.endsWith(":");
    if ( nEndPos + (bColonNameColon ? 1 : 0) < rChk.getLength() - left_wildcard - right_wildcard )
    {
        return std::nullopt;
    }

    bool bWasWordDelim = false;
    sal_Int32 nCalcStt = nEndPos - rChk.getLength() + left_wildcard;
    if (bColonNameColon)
        nCalcStt++;
    if( !right_wildcard && ( !nCalcStt || nCalcStt == rStt || left_wildcard || bColonNameColon ||
          ( nCalcStt < rStt &&
            IsWordDelim( rTxt[ nCalcStt - 1 ] ))) )
    {
        TransliterationWrapper& rCmp = GetIgnoreTranslWrapper();
        OUString sWord( rTxt.substr(nCalcStt, rChk.getLength() - left_wildcard) );
        if( (!left_wildcard && rCmp.isEqual( rChk, sWord )) || (left_wildcard && rCmp.isEqual( rChk.copy(left_wildcard), sWord) ))
        {
            rStt = nCalcStt;
            if (!left_wildcard)
            {
                // fdo#33899 avoid "1/2", "1/3".. to be replaced by fractions in dates, eg. 1/2/14
                if (static_cast<sal_Int32>(rTxt.size()) > nEndPos && rTxt[nEndPos] == '/' && rChk.indexOf('/') != -1)
                {
                    return std::nullopt;
                }
                return *pFnd;
            }
            // get the first word delimiter position before the matching ".*word" pattern
            while( rStt && !(bWasWordDelim = IsWordDelim( rTxt[ --rStt ])))
                ;
            if (bWasWordDelim) rStt++;

            // don't let wildcard pattern override non-wildcard one
            OUString aShort(rTxt.substr(rStt, nEndPos - rStt));
            if (ContainsPattern(aShort)) {
                return std::nullopt;
            }

            OUString left_pattern( rTxt.substr(rStt, nEndPos - rStt - rChk.getLength() + left_wildcard) );
            // avoid double spaces before simple "word" replacement
            left_pattern += (left_pattern.getLength() == 0 && pFnd->GetLong()[0] == 0x20) ? pFnd->GetLong().subView(1) : pFnd->GetLong();
            return SvxAutocorrWord(aShort, left_pattern);
        }
    } else
    // match "word.*" or ".*word.*" patterns, eg. "i18n.*", ".*---.*", TODO: add transliteration support
    if ( right_wildcard )
    {

        OUString sTmp( rChk.copy( left_wildcard, rChk.getLength() - left_wildcard - right_wildcard ) );
        // Get the last word delimiter position
        bool not_suffix;

        while( nSttWdPos && !(bWasWordDelim = IsWordDelim( rTxt[ --nSttWdPos ])))
            ;
        // search the first occurrence (with a left word delimitation, if needed)
        size_t nFndPos = rStt - 1;
        do {
            nFndPos = rTxt.find( sTmp, nFndPos + 1);
            if (nFndPos == std::u16string_view::npos)
                break;
            not_suffix = bWasWordDelim && (nSttWdPos >= (nFndPos + sTmp.getLength()));
        } while ( (!left_wildcard && nFndPos && !IsWordDelim( rTxt[ nFndPos - 1 ])) || not_suffix );

        if ( nFndPos != std::u16string_view::npos )
        {
            sal_Int32 extra_repl = static_cast<sal_Int32>(nFndPos) + sTmp.getLength() > nEndPos ? 1: 0; // for patterns with terminating characters, eg. "a:"

            if ( left_wildcard )
            {
                // get the first word delimiter position before the matching ".*word.*" pattern
                while( nFndPos && !(bWasWordDelim = IsWordDelim( rTxt[ --nFndPos ])))
                    ;
                if (bWasWordDelim) nFndPos++;
            }
            if (nEndPos + extra_repl <= static_cast<sal_Int32>(nFndPos))
            {
                return std::nullopt;
            }
            // return matching pattern and its replacement as a new list item, eg. "i18ns" -> "internationalizations"
            OUString aShort( rTxt.substr(nFndPos, nEndPos - nFndPos + extra_repl) );
            // don't let wildcard pattern override non-wildcard one
            if (ContainsPattern(aShort)) {
                return std::nullopt;
            }

            OUString aLong;
            rStt = nFndPos;
            if ( !left_wildcard )
            {
                sal_Int32 siz = nEndPos - nFndPos - sTmp.getLength();
                aLong = pFnd->GetLong() + (siz > 0 ? rTxt.substr(nFndPos + sTmp.getLength(), siz) : u"");
            } else {
                OUStringBuffer buf;
                do {
                    nSttWdPos = rTxt.find( sTmp, nFndPos);
                    if (nSttWdPos != std::u16string_view::npos)
                    {
                        sal_Int32 nTmp(nFndPos);
                        while (nTmp < static_cast<sal_Int32>(nSttWdPos) && !IsWordDelim(rTxt[nTmp]))
                        {
                            nTmp++;
                        }
                        if (nTmp < static_cast<sal_Int32>(nSttWdPos)) {
                            break; // word delimiter found
                        }
                        buf.append(rTxt.substr(nFndPos, nSttWdPos - nFndPos)).append(pFnd->GetLong());
                        nFndPos = nSttWdPos + sTmp.getLength();
                    }
                } while (nSttWdPos != std::u16string_view::npos);
                if (static_cast<sal_Int32>(nEndPos - nFndPos) > extra_repl) {
                    buf.append(rTxt.substr(nFndPos, nEndPos - nFndPos));
                }
                aLong = buf.makeStringAndClear();
            }
            if ( (static_cast<sal_Int32>(rTxt.size()) > nEndPos && IsWordDelim(rTxt[nEndPos])) || static_cast<sal_Int32>(rTxt.size()) == nEndPos )
            {
                return SvxAutocorrWord(aShort, aLong);
            }
        }
    }
    return std::nullopt;
}

std::optional<SvxAutocorrWordList::WordSearchStatus>
SvxAutocorrWordList::SearchWordsInList(std::u16string_view rTxt,
                                       sal_Int32& rStt,
                                       sal_Int32 nEndPos) const
{
    for (auto it = mpImpl->maHash.begin(); it != mpImpl->maHash.end(); ++it)
    {
        if(auto pTmp = WordMatches(&it->second, rTxt, rStt, nEndPos))
        {
            return WordSearchStatus(
                *pTmp, this,
                Iterator(std::make_unique<Iterator::Impl>(
                    it, mpImpl->maHash.end(),
                    mpImpl->maSortedVector.begin(), mpImpl->maSortedVector.end()
                ))
            );
        }
    }

    for (auto it = mpImpl->maSortedVector.begin();
         it != mpImpl->maSortedVector.end(); ++it)
    {
        if(auto pTmp = WordMatches(&*it, rTxt, rStt, nEndPos))
        {
            return WordSearchStatus(
                *pTmp, this,
                Iterator(std::make_unique<Iterator::Impl>(
                    mpImpl->maHash.end(), mpImpl->maHash.end(),
                    it, mpImpl->maSortedVector.end()
                ))
            );
        }
    }

    return std::nullopt;
}

bool
SvxAutocorrWordList::SearchWordsNext(std::u16string_view rTxt,
                                     sal_Int32& rStt,
                                     sal_Int32 nEndPos,
                                     SvxAutocorrWordList::WordSearchStatus& rStatus) const
{
    while(rStatus.StepIter())
    {
        if(auto pTmp = WordMatches(rStatus.GetWordAtIter(),
                                   rTxt, rStt, nEndPos))
        {
            rStatus.mFnd = *pTmp;
            return true;
        }
    }

    return false;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
