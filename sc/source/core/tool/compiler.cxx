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

#include <config_features.h>

#include <compiler.hxx>

#include <mutex>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>
#include <sfx2/app.hxx>
#include <sfx2/objsh.hxx>
#include <basic/sbmeth.hxx>
#include <basic/sbstar.hxx>
#include <svl/numformat.hxx>
#include <svl/zforlist.hxx>
#include <svl/sharedstringpool.hxx>
#include <sal/log.hxx>
#include <o3tl/safeint.hxx>
#include <o3tl/string_view.hxx>
#include <osl/diagnose.h>
#include <rtl/character.hxx>
#include <unotools/charclass.hxx>
#include <unotools/configmgr.hxx>
#include <com/sun/star/lang/Locale.hpp>
#include <com/sun/star/sheet/FormulaOpCodeMapEntry.hpp>
#include <com/sun/star/sheet/FormulaLanguage.hpp>
#include <com/sun/star/i18n/KParseTokens.hpp>
#include <com/sun/star/i18n/KParseType.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/string.hxx>
#include <unotools/transliterationwrapper.hxx>
#include <tools/urlobj.hxx>
#include <rtl/math.hxx>
#include <rtl/ustring.hxx>
#include <stdlib.h>
#include <rangenam.hxx>
#include <dbdata.hxx>
#include <document.hxx>
#include <docsh.hxx>
#include <callform.hxx>
#include <addincol.hxx>
#include <refupdat.hxx>
#include <globstr.hrc>
#include <scresid.hxx>
#include <formulacell.hxx>
#include <dociter.hxx>
#include <docoptio.hxx>
#include <formula/errorcodes.hxx>
#include <parclass.hxx>
#include <autonamecache.hxx>
#include <externalrefmgr.hxx>
#include <rangeutl.hxx>
#include <convuno.hxx>
#include <tokenuno.hxx>
#include <formulaparserpool.hxx>
#include <tokenarray.hxx>
#include <scmatrix.hxx>
#include <tokenstringcontext.hxx>
#include <officecfg/Office/Common.hxx>
#include <sfx2/linkmgr.hxx>
#include <interpre.hxx>

using namespace formula;
using namespace ::com::sun::star;
using ::std::vector;

const CharClass*                    ScCompiler::pCharClassEnglish = nullptr;
const CharClass*                    ScCompiler::pCharClassLocalized = nullptr;
const ScCompiler::Convention*       ScCompiler::pConventions[ ]   = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

namespace {

enum ScanState
{
    ssGetChar,
    ssGetBool,
    ssGetValue,
    ssGetString,
    ssSkipString,
    ssGetIdent,
    ssGetReference,
    ssSkipReference,
    ssGetErrorConstant,
    ssGetTableRefItem,
    ssGetTableRefColumn,
    ssStop
};

constexpr std::array<ScCharFlags, 128> makeCommonCharTable()
{
    std::array<ScCharFlags, 128> a;
    a.fill(ScCharFlags::Illegal);

    // Allow tabs/newlines.
    // Allow saving whitespace as is (as per OpenFormula specification v.1.2, clause 5.14 "Whitespace").
    a['\t'] = ScCharFlags::CharDontCare | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['\n'] = ScCharFlags::CharDontCare | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['\r'] = ScCharFlags::CharDontCare | ScCharFlags::WordSep | ScCharFlags::ValueSep;

    a[' '] = ScCharFlags::CharDontCare | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['!'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['"'] = ScCharFlags::CharString | ScCharFlags::StringSep;
    a['#'] = ScCharFlags::WordSep | ScCharFlags::CharErrConst;
    a['$'] = ScCharFlags::CharWord | ScCharFlags::Word | ScCharFlags::CharIdent | ScCharFlags::Ident;
    a['%'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['&'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['\''] = ScCharFlags::NameSep;
    a['('] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a[')'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['*'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['+'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueExp | ScCharFlags::ValueSign;
    a[','] = ScCharFlags::CharValue | ScCharFlags::Value;
    a['-'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueExp | ScCharFlags::ValueSign;
    a['.'] = ScCharFlags::Word | ScCharFlags::CharValue | ScCharFlags::Value | ScCharFlags::Ident | ScCharFlags::Name;
    a['/'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;

    for (int i = '0'; i <= '9'; i++)
        a[i] = ScCharFlags::CharValue | ScCharFlags::Word | ScCharFlags::Value | ScCharFlags::ValueExp | ScCharFlags::ValueValue | ScCharFlags::Ident | ScCharFlags::Name;

    a[':'] = ScCharFlags::Char | ScCharFlags::Word;
    a[';'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['<'] = ScCharFlags::CharBool | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['='] = ScCharFlags::Char | ScCharFlags::Bool | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['>'] = ScCharFlags::CharBool | ScCharFlags::Bool | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['?'] = ScCharFlags::CharWord | ScCharFlags::Word | ScCharFlags::Name;
    /* @ */ // FREE

    for (int i = 'A'; i <= 'Z'; i++)
        a[i] = ScCharFlags::CharWord | ScCharFlags::Word | ScCharFlags::CharIdent | ScCharFlags::Ident | ScCharFlags::CharName | ScCharFlags::Name;

    /* [ */ // FREE
    /* \ */ // FREE
    /* ] */ // FREE

    a['^'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['_'] = ScCharFlags::CharWord | ScCharFlags::Word | ScCharFlags::CharIdent | ScCharFlags::Ident | ScCharFlags::CharName | ScCharFlags::Name;
    /* ` */ // FREE

    for (int i = 'a'; i <= 'z'; i++)
        a[i] = ScCharFlags::CharWord | ScCharFlags::Word | ScCharFlags::CharIdent | ScCharFlags::Ident | ScCharFlags::CharName | ScCharFlags::Name;

    a['{'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep; // array open
    a['|'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep; // array row sep (Should be OOo specific)
    a['}'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep; // array close
    a['~'] = ScCharFlags::Char;        // OOo specific
    /* 127 */ // FREE

    return a;
}

constexpr std::array<ScCharFlags, 128> makeCharTable_OOO_A1()
{
    auto a = makeCommonCharTable();
    a['['] = ScCharFlags::Char;
    a[']'] = ScCharFlags::Char;
    return a;
}

constexpr std::array<ScCharFlags, 128> makeCharTable_OOO_A1_ODF()
{
    auto a = makeCommonCharTable();
    a['!'] |= ScCharFlags::OdfLabelOp;
    a['$'] |= ScCharFlags::OdfNameMarker;
    a['['] = ScCharFlags::OdfLBracket;
    a[']'] = ScCharFlags::OdfRBracket;
    return a;
}

constexpr std::array<ScCharFlags, 128> makeCharTable_XL()
{
    auto a = makeCommonCharTable();
    a[' '] |= ScCharFlags::Word;
    a['!'] |= ScCharFlags::Ident | ScCharFlags::Word;
    a['"'] |= ScCharFlags::Word;
    a['#'] &= ~ScCharFlags::WordSep;
    a['#'] |= ScCharFlags::Word;
    a['%'] |= ScCharFlags::Word;
    a['&'] |= ScCharFlags::Word;
    a['\''] |= ScCharFlags::Word;
    a['('] |= ScCharFlags::Word;
    a[')'] |= ScCharFlags::Word;
    a['*'] |= ScCharFlags::Word;
    a['+'] |= ScCharFlags::Word;
#if 0 /* this really needs to be locale specific. */
    a[','] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
#else
    a[','] |= ScCharFlags::Word;
#endif
    a['-'] |= ScCharFlags::Word;

    a[';'] |= ScCharFlags::Word;
    a['<'] |= ScCharFlags::Word;
    a['='] |= ScCharFlags::Word;
    a['>'] |= ScCharFlags::Word;
    /* ? */ // question really is not permitted in sheet name
    a['@'] |= ScCharFlags::Word;
    a['['] = ScCharFlags::Word;
    a[']'] = ScCharFlags::Word;
    a['{'] |= ScCharFlags::Word;
    a['|'] |= ScCharFlags::Word;
    a['}'] |= ScCharFlags::Word;
    a['~'] |= ScCharFlags::Word;
    return a;
}

constexpr std::array<ScCharFlags, 128> makeCharTable_XL_A1()
{
    auto a = makeCharTable_XL();
    a['['] |= ScCharFlags::Char;
    a[']'] |= ScCharFlags::Char;
    return a;
}

constexpr std::array<ScCharFlags, 128> makeCharTable_XL_OOX()
{
    auto a = makeCharTable_XL_A1();
    a['['] |= ScCharFlags::CharIdent;
    a[']'] |= ScCharFlags::Ident;
    return a;
}

constexpr std::array<ScCharFlags, 128> makeCharTable_XL_R1C1()
{
    auto a = makeCharTable_XL();
    a['['] |= ScCharFlags::Ident;
    a[']'] |= ScCharFlags::Ident;
    return a;
}

const std::array<ScCharFlags, 128>& getCharTable(FormulaGrammar::AddressConvention eConv)
{
    switch (eConv)
    {
        case FormulaGrammar::CONV_OOO:
        {
            static constexpr auto table_OOO_A1(makeCharTable_OOO_A1());
            return table_OOO_A1;
        }
        case FormulaGrammar::CONV_ODF:
        {
            static constexpr auto table_OOO_A1_ODF(makeCharTable_OOO_A1_ODF());
            return table_OOO_A1_ODF;
        }
        case FormulaGrammar::CONV_XL_A1:
        {
            static constexpr auto table_XL_A1(makeCharTable_XL_A1());
            return table_XL_A1;
        }
        case FormulaGrammar::CONV_XL_R1C1:
        {
            static constexpr auto table_XL_R1C1(makeCharTable_XL_R1C1());
            return table_XL_R1C1;
        }
        case FormulaGrammar::CONV_XL_OOX:
        {
            static constexpr auto table_XL_OOX(makeCharTable_XL_OOX());
            return table_XL_OOX;
        }
        case FormulaGrammar::CONV_UNSPECIFIED:
        default:
        {
            assert(!"Unimplemented convention");
            std::abort();
        }
    }
}

}

using namespace ::com::sun::star::i18n;

void ScCompiler::fillFromAddInMap( const NonConstOpCodeMapPtr& xMap,FormulaGrammar::Grammar _eGrammar  ) const
{
    size_t nSymbolOffset;
    switch( _eGrammar )
    {
        // XFunctionAccess and XCell::setFormula()/getFormula() API always used
        // PODF grammar symbols, keep it.
        case FormulaGrammar::GRAM_API:
        case FormulaGrammar::GRAM_PODF:
            nSymbolOffset = offsetof( AddInMap, pUpper);
            break;
        default:
        case FormulaGrammar::GRAM_ODFF:
            nSymbolOffset = offsetof( AddInMap, pODFF);
            break;
        case FormulaGrammar::GRAM_ENGLISH:
            nSymbolOffset = offsetof( AddInMap, pEnglish);
            break;
    }
    const AddInMap* const pStop = g_aAddInMap + GetAddInMapCount();
    for (const AddInMap* pMap = g_aAddInMap; pMap < pStop; ++pMap)
    {
        char const * const * ppSymbol =
            reinterpret_cast< char const * const * >(
                    reinterpret_cast< char const * >(pMap) + nSymbolOffset);
        xMap->putExternal( OUString::createFromAscii( *ppSymbol),
                OUString::createFromAscii( pMap->pOriginal));
    }
    if (_eGrammar == FormulaGrammar::GRAM_API)
    {
        // Add English names additionally to programmatic names, so they
        // can be used in XCell::setFormula() non-localized API calls.
        // Note the reverse map will still deliver programmatic names for
        // XCell::getFormula().
        nSymbolOffset = offsetof( AddInMap, pEnglish);
        for (const AddInMap* pMap = g_aAddInMap; pMap < pStop; ++pMap)
        {
            char const * const * ppSymbol =
                reinterpret_cast< char const * const * >(
                        reinterpret_cast< char const * >(pMap) + nSymbolOffset);
            xMap->putExternal( OUString::createFromAscii( *ppSymbol),
                    OUString::createFromAscii( pMap->pOriginal));
        }
    }
}

void ScCompiler::fillFromAddInCollectionUpperName( const NonConstOpCodeMapPtr& xMap ) const
{
    ScUnoAddInCollection* pColl = ScGlobal::GetAddInCollection();
    tools::Long nCount = pColl->GetFuncCount();
    for (tools::Long i=0; i < nCount; ++i)
    {
        const ScUnoAddInFuncData* pFuncData = pColl->GetFuncData(i);
        if (pFuncData)
            xMap->putExternalSoftly( pFuncData->GetUpperName(),
                    pFuncData->GetOriginalName());
    }
}

void ScCompiler::fillFromAddInCollectionEnglishName( const NonConstOpCodeMapPtr& xMap ) const
{
    ScUnoAddInCollection* pColl = ScGlobal::GetAddInCollection();
    tools::Long nCount = pColl->GetFuncCount();
    for (tools::Long i=0; i < nCount; ++i)
    {
        const ScUnoAddInFuncData* pFuncData = pColl->GetFuncData(i);
        if (pFuncData)
        {
            const OUString aName( pFuncData->GetUpperEnglish());
            if (!aName.isEmpty())
                xMap->putExternalSoftly( aName, pFuncData->GetOriginalName());
            else
                xMap->putExternalSoftly( pFuncData->GetUpperName(),
                        pFuncData->GetOriginalName());
        }
    }
}

void ScCompiler::fillFromAddInCollectionExcelName( const NonConstOpCodeMapPtr& xMap ) const
{
    const LanguageTag aDestLang(LANGUAGE_ENGLISH_US);
    ScUnoAddInCollection* pColl = ScGlobal::GetAddInCollection();
    tools::Long nCount = pColl->GetFuncCount();
    for (tools::Long i=0; i < nCount; ++i)
    {
        OUString aExcelName;
        const ScUnoAddInFuncData* pFuncData = pColl->GetFuncData(i);
        if (pFuncData && pFuncData->GetExcelName( aDestLang, aExcelName, true))
        {
            // Note that function names not defined in OOXML but implemented by
            // Excel should have the "_xlfn." prefix. We have no way to check
            // though what an Add-In actually implements.
            xMap->putExternalSoftly( GetCharClassEnglish()->uppercase(aExcelName), pFuncData->GetOriginalName());
        }
    }
}

static std::mutex gCharClassMutex;

void ScCompiler::DeInit()
{
    std::scoped_lock aGuard(gCharClassMutex);
    if (pCharClassEnglish)
    {
        delete pCharClassEnglish;
        pCharClassEnglish = nullptr;
    }
    if (pCharClassLocalized)
    {
        delete pCharClassLocalized;
        pCharClassLocalized = nullptr;
    }
}

bool ScCompiler::IsEnglishSymbol( const OUString& rName )
{
    // function names are always case-insensitive
    OUString aUpper = GetCharClassEnglish()->uppercase(rName);

    // 1. built-in function name
    formula::FormulaCompiler aCompiler;
    OpCode eOp = aCompiler.GetEnglishOpCode( aUpper );
    if ( eOp != ocNone )
    {
        return true;
    }
    // 2. old add in functions
    if (ScGlobal::GetLegacyFuncCollection()->findByName(aUpper))
    {
        return true;
    }

    // 3. new (uno) add in functions
    OUString aIntName = ScGlobal::GetAddInCollection()->FindFunction(aUpper, false);
    return !aIntName.isEmpty();       // no valid function name
}

const CharClass* ScCompiler::GetCharClassEnglish()
{
    std::scoped_lock aGuard(gCharClassMutex);
    if (!pCharClassEnglish)
    {
        pCharClassEnglish = new CharClass( ::comphelper::getProcessComponentContext(),
                LanguageTag( LANGUAGE_ENGLISH_US));
    }
    return pCharClassEnglish;
}

const CharClass* ScCompiler::GetCharClassLocalized()
{
    // Switching UI language requires restart; if not, we would have to
    // keep track of that.
    std::scoped_lock aGuard(gCharClassMutex);
    if (!pCharClassLocalized)
    {
        pCharClassLocalized = new CharClass( ::comphelper::getProcessComponentContext(),
                Application::GetSettings().GetUILanguageTag());
    }
    return pCharClassLocalized;
}

void ScCompiler::SetGrammar( const FormulaGrammar::Grammar eGrammar )
{
    assert( eGrammar != FormulaGrammar::GRAM_UNSPECIFIED && "ScCompiler::SetGrammar: don't pass FormulaGrammar::GRAM_UNSPECIFIED");
    if (eGrammar == GetGrammar())
        return;     // nothing to be done

    if( eGrammar == FormulaGrammar::GRAM_EXTERNAL )
    {
        meGrammar = eGrammar;
        mxSymbols = GetFinalOpCodeMap( css::sheet::FormulaLanguage::NATIVE);
    }
    else
    {
        FormulaGrammar::Grammar eMyGrammar = eGrammar;
        const sal_Int32 nFormulaLanguage = FormulaGrammar::extractFormulaLanguage( eMyGrammar);
        OpCodeMapPtr xMap = GetFinalOpCodeMap( nFormulaLanguage);
        OSL_ENSURE( xMap, "ScCompiler::SetGrammar: unknown formula language");
        if (!xMap)
        {
            xMap = GetFinalOpCodeMap( css::sheet::FormulaLanguage::NATIVE);
            eMyGrammar = xMap->getGrammar();
        }

        // Save old grammar for call to SetGrammarAndRefConvention().
        FormulaGrammar::Grammar eOldGrammar = GetGrammar();
        // This also sets the grammar associated with the map!
        SetFormulaLanguage( xMap);

        // Override if necessary.
        if (eMyGrammar != GetGrammar())
            SetGrammarAndRefConvention( eMyGrammar, eOldGrammar);
    }
}

// Unclear how this was intended to be refreshed when the
// grammar or sheet count is changed ? Ideally this would be
// a method on Document that would globally cache these.
std::vector<OUString> &ScCompiler::GetSetupTabNames() const
{
    std::vector<OUString> &rTabNames = const_cast<ScCompiler *>(this)->maTabNames;

    if (rTabNames.empty())
    {
        rTabNames = rDoc.GetAllTableNames();
        for (auto& rTabName : rTabNames)
            ScCompiler::CheckTabQuotes(rTabName, formula::FormulaGrammar::extractRefConvention(meGrammar));
    }

    return rTabNames;
}

void ScCompiler::SetFormulaLanguage( const ScCompiler::OpCodeMapPtr & xMap )
{
    if (!xMap)
        return;

    mxSymbols = xMap;
    if (mxSymbols->isEnglish())
        pCharClass = GetCharClassEnglish();
    else
        pCharClass = GetCharClassLocalized();

    // The difference is needed for an uppercase() call that usually does not
    // result in different strings but for a few languages like Turkish;
    // though even de-DE and de-CH may differ in ß/SS handling..
    // At least don't care if both are English.
    // The current locale is more likely to not be "en" so check first.
    const LanguageTag& rLT1 = ScGlobal::getCharClass().getLanguageTag();
    const LanguageTag& rLT2 = pCharClass->getLanguageTag();
    mbCharClassesDiffer = (rLT1 != rLT2 && (rLT1.getLanguage() != "en" || rLT2.getLanguage() != "en"));

    SetGrammarAndRefConvention( mxSymbols->getGrammar(), GetGrammar());
}

void ScCompiler::SetGrammarAndRefConvention(
        const FormulaGrammar::Grammar eNewGrammar, const FormulaGrammar::Grammar eOldGrammar )
{
    meGrammar = eNewGrammar;    // SetRefConvention needs the new grammar set!
    FormulaGrammar::AddressConvention eConv = FormulaGrammar::extractRefConvention( meGrammar);
    if (eConv == FormulaGrammar::CONV_UNSPECIFIED && eOldGrammar == FormulaGrammar::GRAM_UNSPECIFIED)
        SetRefConvention( rDoc.GetAddressConvention());
    else
        SetRefConvention( eConv );
}

OUString ScCompiler::FindAddInFunction( const OUString& rUpperName, bool bLocalFirst ) const
{
    return ScGlobal::GetAddInCollection()->FindFunction(rUpperName, bLocalFirst);    // bLocalFirst=false for english
}

ScCompiler::Convention::~Convention()
{
}

ScCompiler::Convention::Convention( FormulaGrammar::AddressConvention eConv )
    : meConv(eConv)
    , mrCharTable(getCharTable(eConv))
{
    ScCompiler::pConventions[ meConv ] = this;
}

static bool lcl_isValidQuotedText( std::u16string_view rFormula, size_t nSrcPos, ParseResult& rRes )
{
    // Tokens that start at ' can have anything in them until a final '
    // but '' marks an escaped '
    // We've earlier guaranteed that a string containing '' will be
    // surrounded by '
    if (nSrcPos < rFormula.size() && rFormula[nSrcPos] == '\'')
    {
        size_t nPos = nSrcPos+1;
        while (nPos < rFormula.size())
        {
            if (rFormula[nPos] == '\'')
            {
                if ( (nPos+1 == rFormula.size()) || (rFormula[nPos+1] != '\'') )
                {
                    rRes.TokenType = KParseType::SINGLE_QUOTE_NAME;
                    rRes.EndPos = nPos+1;
                    return true;
                }
                ++nPos;
            }
            ++nPos;
        }
    }

    return false;
}

static bool lcl_parseExternalName(
        const OUString& rSymbol,
        OUString& rFile,
        OUString& rName,
        const sal_Unicode cSep,
        const ScDocument& rDoc,
        const uno::Sequence<sheet::ExternalLinkInfo>* pExternalLinks )
{
    /* TODO: future versions will have to support sheet-local names too, thus
     * return a possible sheet name as well. */
    const sal_Unicode* const pStart = rSymbol.getStr();
    const sal_Unicode* p = pStart;
    sal_Int32 nLen = rSymbol.getLength();
    OUString aTmpFile;
    OUStringBuffer aTmpName;
    sal_Int32 i = 0;
    bool bInName = false;
    if (cSep == '!')
    {
        // For XL use existing parser that resolves bracketed and quoted and
        // indexed external document names.
        ScRange aRange;
        OUString aStartTabName, aEndTabName;
        ScRefFlags nFlags = ScRefFlags::ZERO;
        p = aRange.Parse_XL_Header( p, rDoc, aTmpFile, aStartTabName,
                aEndTabName, nFlags, true, pExternalLinks );
        if (!p || p == pStart)
            return false;
        i = sal_Int32(p - pStart);
    }
    for ( ; i < nLen; ++i, ++p)
    {
        sal_Unicode c = *p;
        if (i == 0)
        {
            if (c == '.' || c == cSep)
                return false;

            if (c == '\'')
            {
                // Move to the next char and loop until the second single
                // quote.
                sal_Unicode cPrev = c;
                ++i; ++p;
                for (sal_Int32 j = i; j < nLen; ++j, ++p)
                {
                    c = *p;
                    if (c == '\'')
                    {
                        if (j == i)
                        {
                            // empty quote e.g. (=''!Name)
                            return false;
                        }

                        if (cPrev == '\'')
                        {
                            // two consecutive quotes equal a single quote in
                            // the file name.
                            aTmpFile += OUStringChar(c);
                            cPrev = 'a';
                        }
                        else
                            cPrev = c;

                        continue;
                    }

                    if (cPrev == '\'' && j != i)
                    {
                        // this is not a quote but the previous one is.  This
                        // ends the parsing of the quoted segment.  At this
                        // point, the current char must equal the separator
                        // char.

                        i = j;
                        bInName = true;
                        aTmpName.append(c); // Keep the separator as part of the name.
                        break;
                    }
                    aTmpFile += OUStringChar(c);
                    cPrev = c;
                }

                if (!bInName)
                {
                    // premature ending of the quoted segment.
                    return false;
                }

                if (c != cSep)
                {
                    // only the separator is allowed after the closing quote.
                    return false;
                }

                continue;
            }
        }

        if (bInName)
        {
            if (c == cSep)
            {
                // A second separator ?  Not a valid external name.
                return false;
            }
            aTmpName.append(c);
        }
        else
        {
            if (c == cSep)
            {
                bInName = true;
                aTmpName.append(c); // Keep the separator as part of the name.
            }
            else
            {
                do
                {
                    if (rtl::isAsciiAlphanumeric(c))
                        // allowed.
                        break;

                    if (c > 128)
                        // non-ASCII character is allowed.
                        break;

                    bool bValid = false;
                    switch (c)
                    {
                        case '_':
                        case '-':
                        case '.':
                            // these special characters are allowed.
                            bValid = true;
                            break;
                    }
                    if (bValid)
                        break;

                    return false;
                }
                while (false);
                aTmpFile += OUStringChar(c);
            }
        }
    }

    if (!bInName)
    {
        // No name found - most likely the symbol has no '!'s.
        return false;
    }

    sal_Int32 nNameLen = aTmpName.getLength();
    if (nNameLen < 2)
    {
        // Name must be at least 2-char long (separator plus name).
        return false;
    }

    if (aTmpName[0] != cSep)
    {
        // 1st char of the name must equal the separator.
        return false;
    }

    if (aTmpName[nNameLen-1] == '!')
    {
        // Check against #REF!.
        if (OUString::unacquired(aTmpName).equalsIgnoreAsciiCase("#REF!"))
            return false;
    }

    rFile = aTmpFile;
    rName = aTmpName.makeStringAndClear().copy(1); // Skip the first char as it is always the separator.
    return true;
}

static OUString lcl_makeExternalNameStr(const OUString& rFile, const OUString& rName,
        const sal_Unicode cSep, bool bODF )
{
    OUString aEscQuote(u"''"_ustr);
    OUString aFile(rFile.replaceAll("'", aEscQuote));
    OUString aName(rName);
    if (bODF)
        aName = aName.replaceAll("'", aEscQuote);
    OUStringBuffer aBuf(aFile.getLength() + aName.getLength() + 9);
    if (bODF)
        aBuf.append( '[');
    aBuf.append( "'" + aFile + "'" + OUStringChar(cSep) );
    if (bODF)
        aBuf.append( "$$'" );
    aBuf.append( aName);
    if (bODF)
        aBuf.append( "']" );
    return aBuf.makeStringAndClear();
}

static bool lcl_getLastTabName( OUString& rTabName2, const OUString& rTabName1,
                                const vector<OUString>& rTabNames, const ScRange& rRef )
{
    SCTAB nTabSpan = rRef.aEnd.Tab() - rRef.aStart.Tab();
    if (nTabSpan > 0)
    {
        size_t nCount = rTabNames.size();
        vector<OUString>::const_iterator itrBeg = rTabNames.begin(), itrEnd = rTabNames.end();
        vector<OUString>::const_iterator itr = ::std::find(itrBeg, itrEnd, rTabName1);
        if (itr == rTabNames.end())
        {
            rTabName2 = ScResId(STR_NO_REF_TABLE);
            return false;
        }

        size_t nDist = ::std::distance(itrBeg, itr);
        if (nDist + static_cast<size_t>(nTabSpan) >= nCount)
        {
            rTabName2 = ScResId(STR_NO_REF_TABLE);
            return false;
        }

        rTabName2 = rTabNames[nDist+nTabSpan];
    }
    else
        rTabName2 = rTabName1;

    return true;
}

namespace {

struct Convention_A1 : public ScCompiler::Convention
{
    explicit Convention_A1( FormulaGrammar::AddressConvention eConv ) : ScCompiler::Convention( eConv ) { }
    static void MakeColStr( const ScSheetLimits& rLimits, OUStringBuffer& rBuffer, SCCOL nCol );
    static void MakeRowStr( const ScSheetLimits& rLimits, OUStringBuffer& rBuffer, SCROW nRow );

    ParseResult parseAnyToken( const OUString& rFormula,
                               sal_Int32 nSrcPos,
                               const CharClass* pCharClass,
                               bool bGroupSeparator) const override
    {
        ParseResult aRet;
        if ( lcl_isValidQuotedText(rFormula, nSrcPos, aRet) )
            return aRet;

        constexpr sal_Int32 nStartFlags = KParseTokens::ANY_LETTER_OR_NUMBER |
            KParseTokens::ASC_UNDERSCORE | KParseTokens::ASC_DOLLAR;
        constexpr sal_Int32 nContFlags = nStartFlags | KParseTokens::ASC_DOT;
        // '?' allowed in range names because of Xcl :-/
        static constexpr OUString aAddAllowed(u"?#"_ustr);
        return pCharClass->parseAnyToken( rFormula,
                nSrcPos, nStartFlags, aAddAllowed,
                (bGroupSeparator ? nContFlags | KParseTokens::GROUP_SEPARATOR_IN_NUMBER_3 : nContFlags),
                aAddAllowed );
    }

    virtual ScCharFlags getCharTableFlags( sal_Unicode c, sal_Unicode /*cLast*/ ) const override
    {
        return mrCharTable[static_cast<sal_uInt8>(c)];
    }
};

}

void Convention_A1::MakeColStr( const ScSheetLimits& rLimits, OUStringBuffer& rBuffer, SCCOL nCol )
{
    if ( !rLimits.ValidCol(nCol) )
        rBuffer.append(ScResId(STR_NO_REF_TABLE));
    else
        ::ScColToAlpha( rBuffer, nCol);
}

void Convention_A1::MakeRowStr( const ScSheetLimits& rLimits, OUStringBuffer& rBuffer, SCROW nRow )
{
    if ( !rLimits.ValidRow(nRow) )
        rBuffer.append(ScResId(STR_NO_REF_TABLE));
    else
        rBuffer.append(sal_Int32(nRow + 1));
}

namespace {

struct ConventionOOO_A1 : public Convention_A1
{
    ConventionOOO_A1() : Convention_A1 (FormulaGrammar::CONV_OOO) { }
    explicit ConventionOOO_A1( FormulaGrammar::AddressConvention eConv ) : Convention_A1 (eConv) { }

    static void MakeTabStr( OUStringBuffer &rBuf, const std::vector<OUString>& rTabNames, SCTAB nTab )
    {
        if (o3tl::make_unsigned(nTab) >= rTabNames.size())
            rBuf.append(ScResId(STR_NO_REF_TABLE));
        else
            rBuf.append(rTabNames[nTab]);
        rBuf.append('.');
    }

    enum SingletonDisplay
    {
        SINGLETON_NONE,
        SINGLETON_COL,
        SINGLETON_ROW
    };

    static void MakeOneRefStrImpl(
        const ScSheetLimits& rLimits, OUStringBuffer& rBuffer,
        std::u16string_view rErrRef, const std::vector<OUString>& rTabNames,
        const ScSingleRefData& rRef, const ScAddress& rAbsRef,
        bool bForceTab, bool bODF, SingletonDisplay eSingletonDisplay )
    {
        if( rRef.IsFlag3D() || bForceTab )
        {
            if (!ValidTab(rAbsRef.Tab()) || rRef.IsTabDeleted())
            {
                if (!rRef.IsTabRel())
                    rBuffer.append('$');
                rBuffer.append(rErrRef);
                rBuffer.append('.');
            }
            else
            {
                if (!rRef.IsTabRel())
                    rBuffer.append('$');
                MakeTabStr(rBuffer, rTabNames, rAbsRef.Tab());
            }
        }
        else if (bODF)
            rBuffer.append('.');

        if (eSingletonDisplay != SINGLETON_ROW)
        {
            if (!rRef.IsColRel())
                rBuffer.append('$');
            if (!rLimits.ValidCol(rAbsRef.Col()) || rRef.IsColDeleted())
                rBuffer.append(rErrRef);
            else
                MakeColStr(rLimits, rBuffer, rAbsRef.Col());
        }

        if (eSingletonDisplay != SINGLETON_COL)
        {
            if (!rRef.IsRowRel())
                rBuffer.append('$');
            if (!rLimits.ValidRow(rAbsRef.Row()) || rRef.IsRowDeleted())
                rBuffer.append(rErrRef);
            else
                MakeRowStr(rLimits, rBuffer, rAbsRef.Row());
        }
    }

    static SingletonDisplay getSingletonDisplay( const ScSheetLimits& rLimits, const ScAddress& rAbs1, const ScAddress& rAbs2,
            const ScComplexRefData& rRef, bool bFromRangeName )
    {
        // If any part is error, display as such.
        if (!rLimits.ValidCol(rAbs1.Col()) || rRef.Ref1.IsColDeleted() || !rLimits.ValidRow(rAbs1.Row()) || rRef.Ref1.IsRowDeleted() ||
            !rLimits.ValidCol(rAbs2.Col()) || rRef.Ref2.IsColDeleted() || !rLimits.ValidRow(rAbs2.Row()) || rRef.Ref2.IsRowDeleted())
            return SINGLETON_NONE;

        // A:A or $A:$A or A:$A or $A:A
        if (rRef.IsEntireCol(rLimits))
            return SINGLETON_COL;

        // Same if not in named expression and both rows of entire columns are
        // relative references.
        if (!bFromRangeName && rAbs1.Row() == 0 && rAbs2.Row() == rLimits.mnMaxRow &&
                rRef.Ref1.IsRowRel() && rRef.Ref2.IsRowRel())
            return SINGLETON_COL;

        // 1:1 or $1:$1 or 1:$1 or $1:1
        if (rRef.IsEntireRow(rLimits))
            return SINGLETON_ROW;

        // Same if not in named expression and both columns of entire rows are
        // relative references.
        if (!bFromRangeName && rAbs1.Col() == 0 && rAbs2.Col() == rLimits.mnMaxCol &&
                rRef.Ref1.IsColRel() && rRef.Ref2.IsColRel())
            return SINGLETON_ROW;

        return SINGLETON_NONE;
    }

    virtual void makeRefStr(
                     ScSheetLimits& rLimits,
                     OUStringBuffer&   rBuffer,
                     formula::FormulaGrammar::Grammar /*eGram*/,
                     const ScAddress& rPos,
                     const OUString& rErrRef, const std::vector<OUString>& rTabNames,
                     const ScComplexRefData& rRef,
                     bool bSingleRef,
                     bool bFromRangeName ) const override
    {
        // In case absolute/relative positions weren't separately available:
        // transform relative to absolute!
        ScAddress aAbs1 = rRef.Ref1.toAbs(rLimits, rPos), aAbs2;
        if( !bSingleRef )
            aAbs2 = rRef.Ref2.toAbs(rLimits, rPos);

        SingletonDisplay eSingleton = bSingleRef ? SINGLETON_NONE :
            getSingletonDisplay( rLimits, aAbs1, aAbs2, rRef, bFromRangeName);
        MakeOneRefStrImpl(rLimits, rBuffer, rErrRef, rTabNames, rRef.Ref1, aAbs1, false, false, eSingleton);
        if (!bSingleRef)
        {
            rBuffer.append(':');
            MakeOneRefStrImpl(rLimits, rBuffer, rErrRef, rTabNames, rRef.Ref2, aAbs2, aAbs1.Tab() != aAbs2.Tab(), false,
                    eSingleton);
        }
    }

    virtual sal_Unicode getSpecialSymbol( SpecialSymbolType eSymType ) const override
    {
        switch (eSymType)
        {
            case ScCompiler::Convention::ABS_SHEET_PREFIX:
                return '$';
            case ScCompiler::Convention::SHEET_SEPARATOR:
                return '.';
        }

        return u'\0';
    }

    virtual bool parseExternalName( const OUString& rSymbol, OUString& rFile, OUString& rName,
            const ScDocument& rDoc,
            const uno::Sequence<sheet::ExternalLinkInfo>* pExternalLinks ) const override
    {
        return lcl_parseExternalName(rSymbol, rFile, rName, '#', rDoc, pExternalLinks);
    }

    virtual OUString makeExternalNameStr( sal_uInt16 /*nFileId*/, const OUString& rFile,
            const OUString& rName ) const override
    {
        return lcl_makeExternalNameStr( rFile, rName, '#', false);
    }

    static bool makeExternalSingleRefStr(
        const ScSheetLimits& rLimits,
        OUStringBuffer& rBuffer, const OUString& rFileName, const OUString& rTabName,
        const ScSingleRefData& rRef, const ScAddress& rPos, bool bDisplayTabName, bool bEncodeUrl )
    {
        ScAddress aAbsRef = rRef.toAbs(rLimits, rPos);
        if (bDisplayTabName)
        {
            OUString aFile;
            if (bEncodeUrl)
                aFile = rFileName;
            else
                aFile = INetURLObject::decode(rFileName, INetURLObject::DecodeMechanism::Unambiguous);

            rBuffer.append("'" + aFile.replaceAll("'", "''") + "'#");

            if (!rRef.IsTabRel())
                rBuffer.append('$');
            ScRangeStringConverter::AppendTableName(rBuffer, rTabName);

            rBuffer.append('.');
        }

        if (!rRef.IsColRel())
            rBuffer.append('$');
        MakeColStr( rLimits, rBuffer, aAbsRef.Col());
        if (!rRef.IsRowRel())
            rBuffer.append('$');
        MakeRowStr( rLimits, rBuffer, aAbsRef.Row());

        return true;
    }

    static void makeExternalRefStrImpl(
        const ScSheetLimits& rLimits,
        OUStringBuffer& rBuffer, const ScAddress& rPos, const OUString& rFileName,
        const OUString& rTabName, const ScSingleRefData& rRef, bool bODF )
    {
        if (bODF)
            rBuffer.append( '[');

        bool bEncodeUrl = bODF;
        makeExternalSingleRefStr(rLimits, rBuffer, rFileName, rTabName, rRef, rPos, true, bEncodeUrl);
        if (bODF)
            rBuffer.append( ']');
    }

    virtual void makeExternalRefStr(
        ScSheetLimits& rLimits,
        OUStringBuffer& rBuffer, const ScAddress& rPos, sal_uInt16 /*nFileId*/, const OUString& rFileName,
        const OUString& rTabName, const ScSingleRefData& rRef ) const override
    {
        makeExternalRefStrImpl(rLimits, rBuffer, rPos, rFileName, rTabName, rRef, false);
    }

    static void makeExternalRefStrImpl(
        const ScSheetLimits& rLimits,
        OUStringBuffer& rBuffer, const ScAddress& rPos, const OUString& rFileName,
        const std::vector<OUString>& rTabNames, const OUString& rTabName,
        const ScComplexRefData& rRef, bool bODF )
    {
        ScRange aAbsRange = rRef.toAbs(rLimits, rPos);

        if (bODF)
            rBuffer.append( '[');
        // Ensure that there's always a closing bracket, no premature returns.
        bool bEncodeUrl = bODF;

        do
        {
            if (!makeExternalSingleRefStr(rLimits, rBuffer, rFileName, rTabName, rRef.Ref1, rPos, true, bEncodeUrl))
                break;

            rBuffer.append(':');

            OUString aLastTabName;
            bool bDisplayTabName = (aAbsRange.aStart.Tab() != aAbsRange.aEnd.Tab());
            if (bDisplayTabName)
            {
                // Get the name of the last table.
                if (!lcl_getLastTabName(aLastTabName, rTabName, rTabNames, aAbsRange))
                {
                    OSL_FAIL( "ConventionOOO_A1::makeExternalRefStrImpl: sheet name not found");
                    // aLastTabName contains #REF!, proceed.
                }
            }
            else if (bODF)
                rBuffer.append( '.');      // need at least the sheet separator in ODF
            makeExternalSingleRefStr(rLimits,
                rBuffer, rFileName, aLastTabName, rRef.Ref2, rPos, bDisplayTabName, bEncodeUrl);
        } while (false);

        if (bODF)
            rBuffer.append( ']');
    }

    virtual void makeExternalRefStr(
        ScSheetLimits& rLimits,
        OUStringBuffer& rBuffer, const ScAddress& rPos, sal_uInt16 /*nFileId*/, const OUString& rFileName,
        const std::vector<OUString>& rTabNames, const OUString& rTabName,
        const ScComplexRefData& rRef ) const override
    {
        makeExternalRefStrImpl(rLimits, rBuffer, rPos, rFileName, rTabNames, rTabName, rRef, false);
    }
};

struct ConventionOOO_A1_ODF : public ConventionOOO_A1
{
    ConventionOOO_A1_ODF() : ConventionOOO_A1 (FormulaGrammar::CONV_ODF) { }

    virtual void makeRefStr(
                     ScSheetLimits& rLimits,
                     OUStringBuffer&   rBuffer,
                     formula::FormulaGrammar::Grammar eGram,
                     const ScAddress& rPos,
                     const OUString& rErrRef, const std::vector<OUString>& rTabNames,
                     const ScComplexRefData& rRef,
                     bool bSingleRef,
                     bool bFromRangeName ) const override
    {
        rBuffer.append('[');
        // In case absolute/relative positions weren't separately available:
        // transform relative to absolute!
        ScAddress aAbs1 = rRef.Ref1.toAbs(rLimits, rPos), aAbs2;
        if( !bSingleRef )
            aAbs2 = rRef.Ref2.toAbs(rLimits, rPos);

        if (FormulaGrammar::isODFF(eGram) && (rRef.Ref1.IsDeleted() || !rLimits.ValidAddress(aAbs1) ||
                    (!bSingleRef && (rRef.Ref2.IsDeleted() || !rLimits.ValidAddress(aAbs2)))))
        {
            rBuffer.append(rErrRef);
            // For ODFF write [#REF!], but not for PODF so apps reading ODF
            // 1.0/1.1 may have a better chance if they implemented the old
            // form.
        }
        else
        {
            SingletonDisplay eSingleton = bSingleRef ? SINGLETON_NONE :
                getSingletonDisplay( rLimits, aAbs1, aAbs2, rRef, bFromRangeName);
            MakeOneRefStrImpl(rLimits, rBuffer, rErrRef, rTabNames, rRef.Ref1, aAbs1, false, true, eSingleton);
            if (!bSingleRef)
            {
                rBuffer.append(':');
                MakeOneRefStrImpl(rLimits, rBuffer, rErrRef, rTabNames, rRef.Ref2, aAbs2, aAbs1.Tab() != aAbs2.Tab(), true,
                        eSingleton);
            }
        }
        rBuffer.append(']');
    }

    virtual OUString makeExternalNameStr( sal_uInt16 /*nFileId*/, const OUString& rFile,
            const OUString& rName ) const override
    {
        return lcl_makeExternalNameStr( rFile, rName, '#', true);
    }

    virtual void makeExternalRefStr(
        ScSheetLimits& rLimits,
        OUStringBuffer& rBuffer, const ScAddress& rPos, sal_uInt16 /*nFileId*/, const OUString& rFileName,
        const OUString& rTabName, const ScSingleRefData& rRef ) const override
    {
        makeExternalRefStrImpl(rLimits, rBuffer, rPos, rFileName, rTabName, rRef, true);
    }

    virtual void makeExternalRefStr(
        ScSheetLimits& rLimits,
        OUStringBuffer& rBuffer, const ScAddress& rPos, sal_uInt16 /*nFileId*/, const OUString& rFileName,
        const std::vector<OUString>& rTabNames,
        const OUString& rTabName, const ScComplexRefData& rRef ) const override
    {
        makeExternalRefStrImpl(rLimits, rBuffer, rPos, rFileName, rTabNames, rTabName, rRef, true);
    }
};

struct ConventionXL
{
    virtual ~ConventionXL()
    {
    }

    static void GetTab(
        const ScSheetLimits& rLimits,
        const ScAddress& rPos, const std::vector<OUString>& rTabNames,
        const ScSingleRefData& rRef, OUString& rTabName )
    {
        ScAddress aAbs = rRef.toAbs(rLimits, rPos);
        if (rRef.IsTabDeleted() || o3tl::make_unsigned(aAbs.Tab()) >= rTabNames.size())
        {
            rTabName = ScResId( STR_NO_REF_TABLE );
            return;
        }
        rTabName = rTabNames[aAbs.Tab()];
    }

    static void MakeTabStr( const ScSheetLimits& rLimits, OUStringBuffer& rBuf,
                            const ScAddress& rPos,
                            const std::vector<OUString>& rTabNames,
                            const ScComplexRefData& rRef,
                            bool bSingleRef )
    {
        if( !rRef.Ref1.IsFlag3D() )
            return;

        OUString aStartTabName, aEndTabName;

        GetTab(rLimits, rPos, rTabNames, rRef.Ref1, aStartTabName);

        if( !bSingleRef && rRef.Ref2.IsFlag3D() )
        {
            GetTab(rLimits, rPos, rTabNames, rRef.Ref2, aEndTabName);
        }

        const sal_Int32 nQuotePos = rBuf.getLength();
        rBuf.append( aStartTabName );
        if( !bSingleRef && rRef.Ref2.IsFlag3D() && aStartTabName != aEndTabName )
        {
            ScCompiler::FormExcelSheetRange( rBuf, nQuotePos, aEndTabName);
        }

        rBuf.append( '!' );
    }

    static sal_Unicode getSpecialSymbol( ScCompiler::Convention::SpecialSymbolType eSymType )
    {
        switch (eSymType)
        {
            case ScCompiler::Convention::ABS_SHEET_PREFIX:
                return u'\0';
            case ScCompiler::Convention::SHEET_SEPARATOR:
                return '!';
        }
        return u'\0';
    }

    static bool parseExternalName( const OUString& rSymbol, OUString& rFile, OUString& rName,
            const ScDocument& rDoc,
            const uno::Sequence<sheet::ExternalLinkInfo>* pExternalLinks )
    {
        return lcl_parseExternalName( rSymbol, rFile, rName, '!', rDoc, pExternalLinks);
    }

    static OUString makeExternalNameStr( const OUString& rFile, const OUString& rName )
    {
        return lcl_makeExternalNameStr( rFile, rName, '!', false);
    }

    static void makeExternalDocStr( OUStringBuffer& rBuffer, std::u16string_view rFullName )
    {
        // Format that is easier to deal with inside OOo, because we use file
        // URL, and all characters are allowed.  Check if it makes sense to do
        // it the way Gnumeric does it.  Gnumeric doesn't use the URL form
        // and allows relative file path.
        //
        //   ['file:///path/to/source/filename.xls']

        rBuffer.append('[');
        rBuffer.append('\'');
        OUString aFullName = INetURLObject::decode(rFullName, INetURLObject::DecodeMechanism::Unambiguous);

        const sal_Unicode* pBuf = aFullName.getStr();
        sal_Int32 nLen = aFullName.getLength();
        for (sal_Int32 i = 0; i < nLen; ++i)
        {
            const sal_Unicode c = pBuf[i];
            if (c == '\'')
                rBuffer.append(c);
            rBuffer.append(c);
        }
        rBuffer.append('\'');
        rBuffer.append(']');
    }

    static void makeExternalTabNameRange( OUStringBuffer& rBuf, const OUString& rTabName,
                                          const vector<OUString>& rTabNames,
                                          const ScRange& rRef )
    {
        OUString aLastTabName;
        if (!lcl_getLastTabName(aLastTabName, rTabName, rTabNames, rRef))
        {
            ScRangeStringConverter::AppendTableName(rBuf, aLastTabName);
            return;
        }

        ScRangeStringConverter::AppendTableName(rBuf, rTabName);
        if (rTabName != aLastTabName)
        {
            rBuf.append(':');
            ScRangeStringConverter::AppendTableName(rBuf, aLastTabName);
        }
    }

    virtual void parseExternalDocName( const OUString& rFormula, sal_Int32& rSrcPos ) const
    {
        sal_Int32 nLen = rFormula.getLength();
        const sal_Unicode* p = rFormula.getStr();
        sal_Unicode cPrev = 0;
        for (sal_Int32 i = rSrcPos; i < nLen; ++i)
        {
            sal_Unicode c = p[i];
            if (i == rSrcPos)
            {
                // first character must be '['.
                if (c != '[')
                    return;
            }
            else if (i == rSrcPos + 1)
            {
                // second character must be a single quote.
                if (c != '\'')
                    return;
            }
            else if (c == '\'')
            {
                if (cPrev == '\'')
                    // two successive single quote is treated as a single
                    // valid character.
                    c = 'a';
            }
            else if (c == ']')
            {
                if (cPrev == '\'')
                {
                    // valid source document path found.  Increment the
                    // current position to skip the source path.
                    rSrcPos = i + 1;
                    if (rSrcPos >= nLen)
                        rSrcPos = nLen - 1;
                    return;
                }
                else
                    return;
            }
            else
            {
                // any other character
                if (i > rSrcPos + 2 && cPrev == '\'')
                    // unless it's the 3rd character, a normal character
                    // following immediately a single quote is invalid.
                    return;
            }
            cPrev = c;
        }
    }
};

struct ConventionXL_A1 : public Convention_A1, public ConventionXL
{
    ConventionXL_A1() : Convention_A1( FormulaGrammar::CONV_XL_A1 ) { }
    explicit ConventionXL_A1( FormulaGrammar::AddressConvention eConv ) : Convention_A1( eConv ) { }

    static void makeSingleCellStr( const ScSheetLimits& rLimits, OUStringBuffer& rBuf, const ScSingleRefData& rRef, const ScAddress& rAbs )
    {
        if (!rRef.IsColRel())
            rBuf.append('$');
        MakeColStr(rLimits, rBuf, rAbs.Col());
        if (!rRef.IsRowRel())
            rBuf.append('$');
        MakeRowStr(rLimits, rBuf, rAbs.Row());
    }

    virtual void makeRefStr(
                     ScSheetLimits& rLimits,
                     OUStringBuffer&   rBuf,
                     formula::FormulaGrammar::Grammar /*eGram*/,
                     const ScAddress& rPos,
                     const OUString& rErrRef, const std::vector<OUString>& rTabNames,
                     const ScComplexRefData& rRef,
                     bool bSingleRef,
                     bool /*bFromRangeName*/ ) const override
    {
        ScComplexRefData aRef( rRef );

        // Play fast and loose with invalid refs.  There is not much point in producing
        // Foo!A1:#REF! versus #REF! at this point
        ScAddress aAbs1 = aRef.Ref1.toAbs(rLimits, rPos), aAbs2;

        MakeTabStr(rLimits, rBuf, rPos, rTabNames, aRef, bSingleRef);

        if (!rLimits.ValidAddress(aAbs1))
        {
            rBuf.append(rErrRef);
            return;
        }

        if( !bSingleRef )
        {
            aAbs2 = aRef.Ref2.toAbs(rLimits, rPos);
            if (!rLimits.ValidAddress(aAbs2))
            {
                rBuf.append(rErrRef);
                return;
            }

            if (aAbs1.Col() == 0 && aAbs2.Col() >= rLimits.mnMaxCol)
            {
                if (!aRef.Ref1.IsRowRel())
                    rBuf.append( '$' );
                MakeRowStr(rLimits, rBuf, aAbs1.Row());
                rBuf.append( ':' );
                if (!aRef.Ref2.IsRowRel())
                    rBuf.append( '$' );
                MakeRowStr(rLimits, rBuf, aAbs2.Row());
                return;
            }

            if (aAbs1.Row() == 0 && aAbs2.Row() >= rLimits.mnMaxRow)
            {
                if (!aRef.Ref1.IsColRel())
                    rBuf.append( '$' );
                MakeColStr(rLimits, rBuf, aAbs1.Col());
                rBuf.append( ':' );
                if (!aRef.Ref2.IsColRel())
                    rBuf.append( '$' );
                MakeColStr(rLimits, rBuf, aAbs2.Col());
                return;
            }
        }

        makeSingleCellStr(rLimits, rBuf, aRef.Ref1, aAbs1);
        if (!bSingleRef &&
                (aAbs1.Row() != aAbs2.Row() || aRef.Ref1.IsRowRel() != aRef.Ref2.IsRowRel() ||
                 aAbs1.Col() != aAbs2.Col() || aRef.Ref1.IsColRel() != aRef.Ref2.IsColRel()))
        {
            rBuf.append( ':' );
            makeSingleCellStr(rLimits, rBuf, aRef.Ref2, aAbs2);
        }
    }

    virtual ParseResult parseAnyToken( const OUString& rFormula,
                                       sal_Int32 nSrcPos,
                                       const CharClass* pCharClass,
                                       bool bGroupSeparator) const override
    {
        parseExternalDocName(rFormula, nSrcPos);

        ParseResult aRet;
        if ( lcl_isValidQuotedText(rFormula, nSrcPos, aRet) )
            return aRet;

        constexpr sal_Int32 nStartFlags = KParseTokens::ANY_LETTER_OR_NUMBER |
            KParseTokens::ASC_UNDERSCORE | KParseTokens::ASC_DOLLAR;
        constexpr sal_Int32 nContFlags = nStartFlags | KParseTokens::ASC_DOT;
        // '?' allowed in range names
        static constexpr OUString aAddAllowed(u"?!"_ustr);
        return pCharClass->parseAnyToken( rFormula,
                nSrcPos, nStartFlags, aAddAllowed,
                (bGroupSeparator ? nContFlags | KParseTokens::GROUP_SEPARATOR_IN_NUMBER_3 : nContFlags),
                aAddAllowed );
    }

    virtual sal_Unicode getSpecialSymbol( SpecialSymbolType eSymType ) const override
    {
        return ConventionXL::getSpecialSymbol(eSymType);
    }

    virtual bool parseExternalName( const OUString& rSymbol, OUString& rFile, OUString& rName,
            const ScDocument& rDoc,
            const uno::Sequence<sheet::ExternalLinkInfo>* pExternalLinks ) const override
    {
        return ConventionXL::parseExternalName( rSymbol, rFile, rName, rDoc, pExternalLinks);
    }

    virtual OUString makeExternalNameStr( sal_uInt16 /*nFileId*/, const OUString& rFile,
            const OUString& rName ) const override
    {
        return ConventionXL::makeExternalNameStr(rFile, rName);
    }

    virtual void makeExternalRefStr(
        ScSheetLimits& rLimits,
        OUStringBuffer& rBuffer, const ScAddress& rPos, sal_uInt16 /*nFileId*/, const OUString& rFileName,
        const OUString& rTabName, const ScSingleRefData& rRef ) const override
    {
        // ['file:///path/to/file/filename.xls']'Sheet Name'!$A$1
        // This is a little different from the format Excel uses, as Excel
        // puts [] only around the file name.  But we need to enclose the
        // whole file path with [] because the file name can contain any
        // characters.

        ConventionXL::makeExternalDocStr(rBuffer, rFileName);
        ScRangeStringConverter::AppendTableName(rBuffer, rTabName);
        rBuffer.append('!');

        makeSingleCellStr(rLimits, rBuffer, rRef, rRef.toAbs(rLimits, rPos));
    }

    virtual void makeExternalRefStr(
        ScSheetLimits& rLimits,
        OUStringBuffer& rBuffer, const ScAddress& rPos, sal_uInt16 /*nFileId*/, const OUString& rFileName,
        const std::vector<OUString>& rTabNames, const OUString& rTabName,
        const ScComplexRefData& rRef ) const override
    {
        ScRange aAbsRef = rRef.toAbs(rLimits, rPos);

        ConventionXL::makeExternalDocStr(rBuffer, rFileName);
        ConventionXL::makeExternalTabNameRange(rBuffer, rTabName, rTabNames, aAbsRef);
        rBuffer.append('!');

        makeSingleCellStr(rLimits, rBuffer, rRef.Ref1, aAbsRef.aStart);
        if (aAbsRef.aStart != aAbsRef.aEnd)
        {
            rBuffer.append(':');
            makeSingleCellStr(rLimits, rBuffer, rRef.Ref2, aAbsRef.aEnd);
        }
    }
};

struct ConventionXL_OOX : public ConventionXL_A1
{
    ConventionXL_OOX() : ConventionXL_A1( FormulaGrammar::CONV_XL_OOX ) { }

    virtual void makeRefStr( ScSheetLimits& rLimits,
                     OUStringBuffer&   rBuf,
                     formula::FormulaGrammar::Grammar eGram,
                     const ScAddress& rPos,
                     const OUString& rErrRef, const std::vector<OUString>& rTabNames,
                     const ScComplexRefData& rRef,
                     bool bSingleRef,
                     bool bFromRangeName ) const override
    {
        // In OOXML relative references in named expressions are relative to
        // column 0 and row 0. Relative sheet references don't exist.
        ScAddress aPos( rPos );
        if (bFromRangeName)
        {
            // XXX NOTE: by decrementing the reference position we may end up
            // with resolved references with negative values. There's no proper
            // way to solve that or wrap them around without sheet dimensions
            // that are stored along. That, or blindly assume fixed dimensions
            // here and in import.
            /* TODO: maybe do that blind fixed dimensions wrap? */
            aPos.SetCol(0);
            aPos.SetRow(0);
        }

        if (rRef.Ref1.IsDeleted() || (!bSingleRef && rRef.Ref2.IsDeleted()))
        {
            // For OOXML write plain "#REF!" instead of detailed sheet/col/row
            // information.
            rBuf.append(rErrRef);
            return;
        }

        {
            ScAddress aAbs1 = rRef.Ref1.toAbs(rLimits, rPos);
            if (!rLimits.ValidAddress(aAbs1)
                || o3tl::make_unsigned(aAbs1.Tab()) >= rTabNames.size())
            {
                rBuf.append(rErrRef);
                return;
            }
        }

        if (!bSingleRef)
        {
            ScAddress aAbs2 = rRef.Ref2.toAbs(rLimits, rPos);
            if (!rLimits.ValidAddress(aAbs2)
                || o3tl::make_unsigned(aAbs2.Tab()) >= rTabNames.size())
            {
                rBuf.append(rErrRef);
                return;
            }
        }

        ConventionXL_A1::makeRefStr( rLimits, rBuf, eGram, aPos, rErrRef, rTabNames, rRef, bSingleRef, bFromRangeName);
    }

    virtual OUString makeExternalNameStr( sal_uInt16 nFileId, const OUString& /*rFile*/,
            const OUString& rName ) const override
    {
        // [N]!DefinedName is a workbook global name.
        return OUString( "[" + OUString::number(nFileId+1) + "]!" + rName );

        /* TODO: add support for sheet local names, would be
         * [N]'Sheet Name'!DefinedName
         * Similar to makeExternalRefStr() but with DefinedName instead of
         * CellStr. */
    }

    virtual void parseExternalDocName(const OUString& rFormula, sal_Int32& rSrcPos) const override
    {
        sal_Int32 nLen = rFormula.getLength();
        const sal_Unicode* p = rFormula.getStr();
        for (sal_Int32 i = rSrcPos; i < nLen; ++i)
        {
            sal_Unicode c = p[i];
            if (i == rSrcPos)
            {
                // first character must be '['.
                if (c != '[')
                    return;
            }
            else if (c == ']')
            {
                rSrcPos = i + 1;
                return;
            }
        }
    }

    virtual void makeExternalRefStr(
        ScSheetLimits& rLimits,
        OUStringBuffer& rBuffer, const ScAddress& rPos, sal_uInt16 nFileId, const OUString& /*rFileName*/,
        const OUString& rTabName, const ScSingleRefData& rRef ) const override
    {
        // '[N]Sheet Name'!$A$1 or [N]SheetName!$A$1
        // Where N is a 1-based positive integer number of a file name in OOXML
        // xl/externalLinks/externalLinkN.xml

        OUString aQuotedTab( rTabName);
        ScCompiler::CheckTabQuotes( aQuotedTab);
        if (!aQuotedTab.isEmpty() && aQuotedTab[0] == '\'')
        {
            rBuffer.append('\'');
            ConventionXL_OOX::makeExternalDocStr( rBuffer, nFileId);
            rBuffer.append( aQuotedTab.subView(1));
        }
        else
        {
            ConventionXL_OOX::makeExternalDocStr( rBuffer, nFileId);
            rBuffer.append( aQuotedTab);
        }
        rBuffer.append('!');

        makeSingleCellStr(rLimits, rBuffer, rRef, rRef.toAbs(rLimits, rPos));
    }

    virtual void makeExternalRefStr(
        ScSheetLimits& rLimits,
        OUStringBuffer& rBuffer, const ScAddress& rPos, sal_uInt16 nFileId, const OUString& /*rFileName*/,
        const std::vector<OUString>& rTabNames, const OUString& rTabName,
        const ScComplexRefData& rRef ) const override
    {
        // '[N]Sheet One':'Sheet Two'!A1:B2 or [N]SheetOne!A1:B2
        // Actually Excel writes '[N]Sheet One:Sheet Two'!A1:B2 but reads the
        // simpler to produce and more logical form with independently quoted
        // sheet names as well. The [N] having to be within the quoted sheet
        // name is ugly enough...

        ScRange aAbsRef = rRef.toAbs(rLimits, rPos);

        OUStringBuffer aBuf;
        ConventionXL::makeExternalTabNameRange( aBuf, rTabName, rTabNames, aAbsRef);
        if (!aBuf.isEmpty() && aBuf[0] == '\'')
        {
            rBuffer.append('\'');
            ConventionXL_OOX::makeExternalDocStr( rBuffer, nFileId);
            rBuffer.append( aBuf.subView(1));
        }
        else
        {
            ConventionXL_OOX::makeExternalDocStr( rBuffer, nFileId);
            rBuffer.append( aBuf);
        }
        rBuffer.append('!');

        makeSingleCellStr(rLimits, rBuffer, rRef.Ref1, aAbsRef.aStart);
        if (aAbsRef.aStart != aAbsRef.aEnd)
        {
            rBuffer.append(':');
            makeSingleCellStr(rLimits, rBuffer, rRef.Ref2, aAbsRef.aEnd);
        }
    }

    static void makeExternalDocStr( OUStringBuffer& rBuffer, sal_uInt16 nFileId )
    {
        rBuffer.append("[" + OUString::number( static_cast<sal_Int32>(nFileId+1) ) + "]");
    }
};

}

static void
r1c1_add_col( OUStringBuffer &rBuf, const ScSingleRefData& rRef, const ScAddress& rAbsRef )
{
    rBuf.append( 'C' );
    if( rRef.IsColRel() )
    {
        SCCOL nCol = rRef.Col();
        if (nCol != 0)
            rBuf.append("[" + OUString::number(nCol) + "]");
    }
    else
        rBuf.append( static_cast<sal_Int32>(rAbsRef.Col() + 1) );
}
static void
r1c1_add_row( OUStringBuffer &rBuf, const ScSingleRefData& rRef, const ScAddress& rAbsRef )
{
    rBuf.append( 'R' );
    if( rRef.IsRowRel() )
    {
        if (rRef.Row() != 0)
        {
            rBuf.append("[" + OUString::number(rRef.Row()) + "]");
        }
    }
    else
        rBuf.append( rAbsRef.Row() + 1 );
}

namespace {

struct ConventionXL_R1C1 : public ScCompiler::Convention, public ConventionXL
{
    ConventionXL_R1C1() : ScCompiler::Convention( FormulaGrammar::CONV_XL_R1C1 ) { }

    virtual void makeRefStr( ScSheetLimits& rLimits,
                     OUStringBuffer&   rBuf,
                     formula::FormulaGrammar::Grammar /*eGram*/,
                     const ScAddress& rPos,
                     const OUString& rErrRef, const std::vector<OUString>& rTabNames,
                     const ScComplexRefData& rRef,
                     bool bSingleRef,
                     bool /*bFromRangeName*/ ) const override
    {
        ScRange aAbsRef = rRef.toAbs(rLimits, rPos);
        ScComplexRefData aRef( rRef );

        MakeTabStr(rLimits, rBuf, rPos, rTabNames, aRef, bSingleRef);

        // Play fast and loose with invalid refs.  There is not much point in producing
        // Foo!A1:#REF! versus #REF! at this point
        if (!rLimits.ValidCol(aAbsRef.aStart.Col()) || !rLimits.ValidRow(aAbsRef.aStart.Row()))
        {
            rBuf.append(rErrRef);
            return;
        }

        if( !bSingleRef )
        {
            if (!rLimits.ValidCol(aAbsRef.aEnd.Col()) || !rLimits.ValidRow(aAbsRef.aEnd.Row()))
            {
                rBuf.append(rErrRef);
                return;
            }

            if (aAbsRef.aStart.Col() == 0 && aAbsRef.aEnd.Col() >= rLimits.mnMaxCol)
            {
                r1c1_add_row(rBuf,  rRef.Ref1, aAbsRef.aStart);
                if (aAbsRef.aStart.Row() != aAbsRef.aEnd.Row() ||
                    rRef.Ref1.IsRowRel() != rRef.Ref2.IsRowRel() )
                {
                    rBuf.append( ':' );
                    r1c1_add_row(rBuf,  rRef.Ref2, aAbsRef.aEnd);
                }
                return;

            }

            if (aAbsRef.aStart.Row() == 0 && aAbsRef.aEnd.Row() >= rLimits.mnMaxRow)
            {
                r1c1_add_col(rBuf, rRef.Ref1, aAbsRef.aStart);
                if (aAbsRef.aStart.Col() != aAbsRef.aEnd.Col() ||
                    rRef.Ref1.IsColRel() != rRef.Ref2.IsColRel())
                {
                    rBuf.append( ':' );
                    r1c1_add_col(rBuf, rRef.Ref2, aAbsRef.aEnd);
                }
                return;
            }
        }

        r1c1_add_row(rBuf, rRef.Ref1, aAbsRef.aStart);
        r1c1_add_col(rBuf, rRef.Ref1, aAbsRef.aStart);
        // We can't parse a single col/row reference in the context of a R1C1
        // 3D reference back yet, otherwise (if Excel understands it) an
        // additional condition similar to ConventionXL_A1::makeRefStr() could
        // be
        //
        //   && (aAbsRef.aStart.Row() != aAbsRef.aEnd.Row() || rRef.Ref1.IsRowRel() != rRef.Ref2.IsRowRel() ||
        //       aAbsRef.aStart.Col() != aAbsRef.aEnd.Col() || rRef.Ref1.IsColRel() != rRef.Ref2.IsColRel())
        if (!bSingleRef)
        {
            rBuf.append( ':' );
            r1c1_add_row(rBuf, rRef.Ref2, aAbsRef.aEnd);
            r1c1_add_col(rBuf, rRef.Ref2, aAbsRef.aEnd);
        }
    }

    ParseResult parseAnyToken( const OUString& rFormula,
                               sal_Int32 nSrcPos,
                               const CharClass* pCharClass,
                               bool bGroupSeparator) const override
    {
        parseExternalDocName(rFormula, nSrcPos);

        ParseResult aRet;
        if ( lcl_isValidQuotedText(rFormula, nSrcPos, aRet) )
            return aRet;

        constexpr sal_Int32 nStartFlags = KParseTokens::ANY_LETTER_OR_NUMBER |
            KParseTokens::ASC_UNDERSCORE ;
        constexpr sal_Int32 nContFlags = nStartFlags | KParseTokens::ASC_DOT;
        // '?' allowed in range names
        static constexpr OUString aAddAllowed(u"?-[]!"_ustr);

        return pCharClass->parseAnyToken( rFormula,
                nSrcPos, nStartFlags, aAddAllowed,
                (bGroupSeparator ? nContFlags | KParseTokens::GROUP_SEPARATOR_IN_NUMBER_3 : nContFlags),
                aAddAllowed );
    }

    virtual sal_Unicode getSpecialSymbol( SpecialSymbolType eSymType ) const override
    {
        return ConventionXL::getSpecialSymbol(eSymType);
    }

    virtual bool parseExternalName( const OUString& rSymbol, OUString& rFile, OUString& rName,
            const ScDocument& rDoc,
            const uno::Sequence<sheet::ExternalLinkInfo>* pExternalLinks ) const override
    {
        return ConventionXL::parseExternalName( rSymbol, rFile, rName, rDoc, pExternalLinks);
    }

    virtual OUString makeExternalNameStr( sal_uInt16 /*nFileId*/, const OUString& rFile,
            const OUString& rName ) const override
    {
        return ConventionXL::makeExternalNameStr(rFile, rName);
    }

    virtual void makeExternalRefStr(
        ScSheetLimits& rLimits,
        OUStringBuffer& rBuffer, const ScAddress& rPos, sal_uInt16 /*nFileId*/, const OUString& rFileName,
        const OUString& rTabName, const ScSingleRefData& rRef ) const override
    {
        // ['file:///path/to/file/filename.xls']'Sheet Name'!$A$1
        // This is a little different from the format Excel uses, as Excel
        // puts [] only around the file name.  But we need to enclose the
        // whole file path with [] because the file name can contain any
        // characters.

        ScAddress aAbsRef = rRef.toAbs(rLimits, rPos);
        ConventionXL::makeExternalDocStr(rBuffer, rFileName);
        ScRangeStringConverter::AppendTableName(rBuffer, rTabName);
        rBuffer.append('!');

        r1c1_add_row(rBuffer, rRef, aAbsRef);
        r1c1_add_col(rBuffer, rRef, aAbsRef);
    }

    virtual void makeExternalRefStr(
        ScSheetLimits& rLimits,
        OUStringBuffer& rBuffer, const ScAddress& rPos, sal_uInt16 /*nFileId*/, const OUString& rFileName,
        const std::vector<OUString>& rTabNames, const OUString& rTabName,
        const ScComplexRefData& rRef ) const override
    {
        ScRange aAbsRef = rRef.toAbs(rLimits, rPos);

        ConventionXL::makeExternalDocStr(rBuffer, rFileName);
        ConventionXL::makeExternalTabNameRange(rBuffer, rTabName, rTabNames, aAbsRef);
        rBuffer.append('!');

        if (!rLimits.ValidCol(aAbsRef.aEnd.Col()) || !rLimits.ValidRow(aAbsRef.aEnd.Row()))
        {
            rBuffer.append(ScResId(STR_NO_REF_TABLE));
            return;
        }

        if (aAbsRef.aStart.Col() == 0 && aAbsRef.aEnd.Col() >= rLimits.mnMaxCol)
        {
            r1c1_add_row(rBuffer, rRef.Ref1, aAbsRef.aStart);
            if (aAbsRef.aStart.Row() != aAbsRef.aEnd.Row() || rRef.Ref1.IsRowRel() != rRef.Ref2.IsRowRel())
            {
                rBuffer.append(':');
                r1c1_add_row(rBuffer, rRef.Ref2, aAbsRef.aEnd);
            }
            return;
        }

        if (aAbsRef.aStart.Row() == 0 && aAbsRef.aEnd.Row() >= rLimits.mnMaxRow)
        {
            r1c1_add_col(rBuffer, rRef.Ref1, aAbsRef.aStart);
            if (aAbsRef.aStart.Col() != aAbsRef.aEnd.Col() || rRef.Ref1.IsColRel() != rRef.Ref2.IsColRel())
            {
                rBuffer.append(':');
                r1c1_add_col(rBuffer, rRef.Ref2, aAbsRef.aEnd);
            }
            return;
        }

        r1c1_add_row(rBuffer, rRef.Ref1, aAbsRef.aStart);
        r1c1_add_col(rBuffer, rRef.Ref1, aAbsRef.aStart);
        rBuffer.append(':');
        r1c1_add_row(rBuffer, rRef.Ref2, aAbsRef.aEnd);
        r1c1_add_col(rBuffer, rRef.Ref2, aAbsRef.aEnd);
    }

    virtual ScCharFlags getCharTableFlags( sal_Unicode c, sal_Unicode cLast ) const override
    {
        ScCharFlags nFlags = mrCharTable[static_cast<sal_uInt8>(c)];
        if (c == '-' && cLast == '[')
            // '-' can occur within a reference string only after '[' e.g. R[-1]C.
            nFlags |= ScCharFlags::Ident;
        return nFlags;
    }
};

}

ScCompiler::ScCompiler( sc::CompileFormulaContext& rCxt, const ScAddress& rPos, ScTokenArray& rArr,
                        bool bComputeII, bool bMatrixFlag, ScInterpreterContext* pContext )
    : FormulaCompiler(rArr, bComputeII, bMatrixFlag),
    rDoc(rCxt.getDoc()),
    aPos(rPos),
    mrInterpreterContext(pContext ? *pContext : rDoc.GetNonThreadedContext()),
    mnCurrentSheetTab(-1),
    mnCurrentSheetEndPos(0),
    pCharClass(&ScGlobal::getCharClass()),
    mbCharClassesDiffer(false),
    mnPredetectedReference(0),
    mnRangeOpPosInSymbol(-1),
    pConv(GetRefConvention(FormulaGrammar::CONV_OOO)),
    meExtendedErrorDetection(EXTENDED_ERROR_DETECTION_NONE),
    mbCloseBrackets(true),
    mbRewind(false),
    mbRefConventionChartOOXML(false),
    maTabNames(rCxt.getTabNames())
{
    SetGrammar(rCxt.getGrammar());
    m_oODFSavingVersion = rCxt.getODFSavingVersion();
}

ScCompiler::ScCompiler( ScDocument& rDocument, const ScAddress& rPos, ScTokenArray& rArr,
                        formula::FormulaGrammar::Grammar eGrammar,
                        bool bComputeII, bool bMatrixFlag, ScInterpreterContext* pContext )
    : FormulaCompiler(rArr, bComputeII, bMatrixFlag),
        rDoc( rDocument ),
        aPos( rPos ),
        mrInterpreterContext(pContext ? *pContext : rDoc.GetNonThreadedContext()),
        mnCurrentSheetTab(-1),
        mnCurrentSheetEndPos(0),
        nSrcPos(0),
        pCharClass( &ScGlobal::getCharClass() ),
        mbCharClassesDiffer(false),
        mnPredetectedReference(0),
        mnRangeOpPosInSymbol(-1),
        pConv( GetRefConvention( FormulaGrammar::CONV_OOO ) ),
        meExtendedErrorDetection( EXTENDED_ERROR_DETECTION_NONE ),
        mbCloseBrackets( true ),
        mbRewind( false ),
        mbRefConventionChartOOXML( false )
{
    SetGrammar( (eGrammar == formula::FormulaGrammar::GRAM_UNSPECIFIED) ?
                rDocument.GetGrammar() :
                eGrammar );
}

ScCompiler::ScCompiler( sc::CompileFormulaContext& rCxt, const ScAddress& rPos,
                        bool bComputeII, bool bMatrixFlag, ScInterpreterContext* pContext )
    : FormulaCompiler(bComputeII, bMatrixFlag),
    rDoc(rCxt.getDoc()),
    aPos(rPos),
    mrInterpreterContext(pContext ? *pContext : rDoc.GetNonThreadedContext()),
    mnCurrentSheetTab(-1),
    mnCurrentSheetEndPos(0),
    pCharClass(&ScGlobal::getCharClass()),
    mbCharClassesDiffer(false),
    mnPredetectedReference(0),
    mnRangeOpPosInSymbol(-1),
    pConv(GetRefConvention(FormulaGrammar::CONV_OOO)),
    meExtendedErrorDetection(EXTENDED_ERROR_DETECTION_NONE),
    mbCloseBrackets(true),
    mbRewind(false),
    mbRefConventionChartOOXML(false),
    maTabNames(rCxt.getTabNames())
{
    SetGrammar(rCxt.getGrammar());
}

ScCompiler::ScCompiler( ScDocument& rDocument, const ScAddress& rPos,
                        formula::FormulaGrammar::Grammar eGrammar,
                        bool bComputeII, bool bMatrixFlag, ScInterpreterContext* pContext )
        : FormulaCompiler(bComputeII, bMatrixFlag),
        rDoc( rDocument ),
        aPos( rPos ),
        mrInterpreterContext(pContext ? *pContext : rDoc.GetNonThreadedContext()),
        mnCurrentSheetTab(-1),
        mnCurrentSheetEndPos(0),
        nSrcPos(0),
        pCharClass( &ScGlobal::getCharClass() ),
        mbCharClassesDiffer(false),
        mnPredetectedReference(0),
        mnRangeOpPosInSymbol(-1),
        pConv( GetRefConvention( FormulaGrammar::CONV_OOO ) ),
        meExtendedErrorDetection( EXTENDED_ERROR_DETECTION_NONE ),
        mbCloseBrackets( true ),
        mbRewind( false ),
        mbRefConventionChartOOXML( false )
{
    SetGrammar( (eGrammar == formula::FormulaGrammar::GRAM_UNSPECIFIED) ?
                rDocument.GetGrammar() :
                eGrammar );
}

ScCompiler::~ScCompiler()
{
}

void ScCompiler::CheckTabQuotes( OUString& rString,
                                 const FormulaGrammar::AddressConvention eConv )
{
    sal_Int32 nStartFlags = KParseTokens::ANY_LETTER_OR_NUMBER | KParseTokens::ASC_UNDERSCORE;
    sal_Int32 nContFlags = nStartFlags;
    ParseResult aRes = ScGlobal::getCharClass().parsePredefinedToken(
        KParseType::IDENTNAME, rString, 0, nStartFlags, OUString(), nContFlags, OUString());
    bool bNeedsQuote = !((aRes.TokenType & KParseType::IDENTNAME) && aRes.EndPos == rString.getLength());

    switch ( eConv )
    {
        default :
        case FormulaGrammar::CONV_UNSPECIFIED :
            break;
        case FormulaGrammar::CONV_OOO :
        case FormulaGrammar::CONV_XL_A1 :
        case FormulaGrammar::CONV_XL_R1C1 :
        case FormulaGrammar::CONV_XL_OOX :
        case FormulaGrammar::CONV_ODF :
            if( bNeedsQuote )
            {
                // escape embedded quotes
                rString = rString.replaceAll( "'", "''" );
            }
            break;
    }

    if ( !bNeedsQuote && CharClass::isAsciiNumeric( rString ) )
    {
        // Prevent any possible confusion resulting from pure numeric sheet names.
        bNeedsQuote = true;
    }

    if( bNeedsQuote )
    {
        rString = "'" + rString + "'";
    }
}

void ScCompiler::FormExcelSheetRange( OUStringBuffer& rBuf, sal_Int32 nQuotePos, const OUString& rEndTabName )
{
    OUString aEndTabName(rEndTabName);
    if (nQuotePos < rBuf.getLength())
    {
        const bool bQuoted2 = (!aEndTabName.isEmpty() && aEndTabName[0] == '\'');
        if (bQuoted2)
            aEndTabName = aEndTabName.subView(1);   //  Sheet2'
        if (rBuf[nQuotePos] == '\'')                // 'Sheet1'
        {
            const sal_Int32 nLast = rBuf.getLength() - 1;
            if (rBuf[nLast] == '\'')
                rBuf.remove(nLast, 1);              // 'Sheet1
        }
        else if (bQuoted2)                          //  Sheet1
        {
            rBuf.insert(nQuotePos, '\'');           // 'Sheet1
        }
    }
    rBuf.append( ':' );
    rBuf.append( aEndTabName );
}

sal_Int32 ScCompiler::GetDocTabPos( const OUString& rString )
{
    if (rString[0] != '\'')
        return -1;
    sal_Int32 nPos = ScGlobal::FindUnquoted( rString, SC_COMPILER_FILE_TAB_SEP);
    // it must be 'Doc'#
    if (nPos != -1 && rString[nPos-1] != '\'')
        nPos = -1;
    return nPos;
}

void ScCompiler::SetRefConvention( FormulaGrammar::AddressConvention eConv )
{
    const Convention* p = GetRefConvention(eConv);
    if (p)
        SetRefConvention(p);
}

const ScCompiler::Convention* ScCompiler::GetRefConvention( FormulaGrammar::AddressConvention eConv )
{

    switch (eConv)
    {
        case FormulaGrammar::CONV_OOO:
        {
            static const ConventionOOO_A1 ConvOOO_A1;
            return &ConvOOO_A1;
        }
        case FormulaGrammar::CONV_ODF:
        {
            static const ConventionOOO_A1_ODF ConvOOO_A1_ODF;
            return &ConvOOO_A1_ODF;
        }
        case FormulaGrammar::CONV_XL_A1:
        {
            static const ConventionXL_A1 ConvXL_A1;
            return &ConvXL_A1;
        }
        case FormulaGrammar::CONV_XL_R1C1:
        {
            static const ConventionXL_R1C1 ConvXL_R1C1;
            return &ConvXL_R1C1;
        }
        case FormulaGrammar::CONV_XL_OOX:
        {
            static const ConventionXL_OOX ConvXL_OOX;
            return &ConvXL_OOX;
        }
        case FormulaGrammar::CONV_UNSPECIFIED:
        default:
            ;
    }

    return nullptr;
}

void ScCompiler::SetRefConvention( const ScCompiler::Convention *pConvP )
{
    pConv = pConvP;
    meGrammar = FormulaGrammar::mergeToGrammar( meGrammar, pConv->meConv);
    assert( FormulaGrammar::isSupported( meGrammar));
}

void ScCompiler::SetError(FormulaError nError)
{
    if( pArr->GetCodeError() == FormulaError::NONE)
        pArr->SetCodeError( nError);
}

static sal_Unicode* lcl_UnicodeStrNCpy( sal_Unicode* pDst, const sal_Unicode* pSrc, sal_Int32 nMax )
{
    const sal_Unicode* const pStop = pDst + nMax;
    while ( pDst < pStop )
    {
        *pDst++ = *pSrc++;
    }
    *pDst = 0;
    return pDst;
}

// p1 MUST contain at least n characters, or terminate with NIL.
// p2 MUST pass upper case letters, if any.
// n  MUST not be greater than length of p2
static bool lcl_isUnicodeIgnoreAscii( const sal_Unicode* p1, const char* p2, size_t n )
{
    for (size_t i=0; i<n; ++i)
    {
        if (!p1[i])
            return false;
        if (p1[i] != p2[i])
        {
            if (p1[i] < 'a' || 'z' < p1[i])
                return false;   // not a lower case letter
            if (p2[i] < 'A' || 'Z' < p2[i])
                return false;   // not a letter to match
            if (p1[i] != p2[i] + 0x20)
                return false;   // lower case doesn't match either
        }
    }
    return true;
}

// static
void ScCompiler::addWhitespace( std::vector<ScCompiler::Whitespace> & rvSpaces,
        ScCompiler::Whitespace & rSpace, sal_Unicode c, sal_Int32 n )
{
    if (rSpace.cChar != c)
    {
        if (rSpace.cChar && rSpace.nCount > 0)
            rvSpaces.emplace_back(rSpace);
        rSpace.reset(c);
    }
    rSpace.nCount += n;
}

// NextSymbol

// Parses the formula into separate symbols for further processing.
// XXX NOTE: this is a rough sketch of the original idea, there are other
// states that were added and didn't make it into this table and things are
// more complicated. Use the source, Luke.

// initial state = GetChar

// old state     | read character    | action                | new state
//---------------+-------------------+-----------------------+---------------
// GetChar       | ;()+-*/^=&        | Symbol=char           | Stop
//               | <>                | Symbol=char           | GetBool
//               | $ letter          | Symbol=char           | GetWord
//               | number            | Symbol=char           | GetValue
//               | "                 | none                  | GetString
//               | other             | none                  | GetChar
//---------------+-------------------+-----------------------+---------------
// GetBool       | =>                | Symbol=Symbol+char    | Stop
//               | other             | Dec(CharPos)          | Stop
//---------------+-------------------+-----------------------+---------------
// GetWord       | SepSymbol         | Dec(CharPos)          | Stop
//               | ()+-*/^=<>&~      |                       |
//               | space             | Dec(CharPos)          | Stop
//               | $_:.              |                       |
//               | letter, number    | Symbol=Symbol+char    | GetWord
//               | other             | error                 | Stop
//---------------+-------------------+-----------------------+---------------
// GetValue      | ;()*/^=<>&        |                       |
//               | space             | Dec(CharPos)          | Stop
//               | number E+-%,.     | Symbol=Symbol+char    | GetValue
//               | other             | error                 | Stop
//---------------+-------------------+-----------------------+---------------
// GetString     | "                 | none                  | Stop
//               | other             | Symbol=Symbol+char    | GetString
//---------------+-------------------+-----------------------+---------------

std::vector<ScCompiler::Whitespace> ScCompiler::NextSymbol(bool bInArray)
{
    std::vector<Whitespace> vSpaces;
    cSymbol[MAXSTRLEN] = 0;       // end
    sal_Unicode* pSym = cSymbol;
    const sal_Unicode* const pStart = aFormula.getStr();
    const sal_Unicode* pSrc = pStart + nSrcPos;
    bool bi18n = false;
    sal_Unicode c = *pSrc;
    sal_Unicode cLast = 0;
    bool bQuote = false;
    mnRangeOpPosInSymbol = -1;
    ScanState eState = ssGetChar;
    Whitespace aSpace;
    sal_Unicode cSep = mxSymbols->getSymbolChar( ocSep);
    sal_Unicode cArrayColSep = mxSymbols->getSymbolChar( ocArrayColSep);
    sal_Unicode cArrayRowSep = mxSymbols->getSymbolChar( ocArrayRowSep);
    sal_Unicode cDecSep = (mxSymbols->isEnglishLocale() ? '.' : ScGlobal::getLocaleData().getNumDecimalSep()[0]);
    sal_Unicode cDecSepAlt = (mxSymbols->isEnglishLocale() ? 0 : ScGlobal::getLocaleData().getNumDecimalSepAlt().toChar());

    // special symbols specific to address convention used
    sal_Unicode cSheetPrefix = pConv->getSpecialSymbol(ScCompiler::Convention::ABS_SHEET_PREFIX);
    sal_Unicode cSheetSep    = pConv->getSpecialSymbol(ScCompiler::Convention::SHEET_SEPARATOR);

    int nDecSeps = 0;
    bool bAutoIntersection = false;
    size_t nAutoIntersectionSpacesPos = 0;
    int nRefInName = 0;
    bool bErrorConstantHadSlash = false;
    mnPredetectedReference = 0;
    // try to parse simple tokens before calling i18n parser
    while ((c != 0) && (eState != ssStop) )
    {
        pSrc++;
        ScCharFlags nMask = GetCharTableFlags( c, cLast );

        // The parameter separator and the array column and row separators end
        // things unconditionally if not in string or reference.
        if (c == cSep || (bInArray && (c == cArrayColSep || c == cArrayRowSep)))
        {
            switch (eState)
            {
                // these are to be continued
                case ssGetString:
                case ssSkipString:
                case ssGetReference:
                case ssSkipReference:
                case ssGetTableRefItem:
                case ssGetTableRefColumn:
                    break;
                default:
                    if (eState == ssGetChar)
                        *pSym++ = c;
                    else
                        pSrc--;
                    eState = ssStop;
            }
        }
Label_MaskStateMachine:
        switch (eState)
        {
            case ssGetChar :
            {
                // Order is important!
                if (eLastOp == ocTableRefOpen && c != '[' && c != '#' && c != ']')
                {
                    *pSym++ = c;
                    eState = ssGetTableRefColumn;
                }
                else if( nMask & ScCharFlags::OdfLabelOp )
                {
                    // '!!' automatic intersection
                    if (GetCharTableFlags( pSrc[0], 0 ) & ScCharFlags::OdfLabelOp)
                    {
                        /* TODO: For now the UI "space operator" is used, this
                         * could be enhanced using a specialized OpCode to get
                         * rid of the space ambiguity, which would need some
                         * places to be adapted though. And we would still need
                         * to support the ambiguous space operator for UI
                         * purposes anyway. However, we then could check for
                         * invalid usage of '!!', which currently isn't
                         * possible. */
                        if (!bAutoIntersection)
                        {
                            ++pSrc;
                            // Add 2 because it must match the character count
                            // for bi18n.
                            addWhitespace( vSpaces, aSpace, 0x20, 2);
                            // Position of Whitespace where it will be added to
                            // vector.
                            nAutoIntersectionSpacesPos = vSpaces.size();
                            bAutoIntersection = true;
                        }
                        else
                        {
                            pSrc--;
                            eState = ssStop;
                        }
                    }
                    else
                    {
                        nMask &= ~ScCharFlags::OdfLabelOp;
                        goto Label_MaskStateMachine;
                    }
                }
                else if( nMask & ScCharFlags::OdfNameMarker )
                {
                    // '$$' defined name marker
                    if (GetCharTableFlags( pSrc[0], 0 ) & ScCharFlags::OdfNameMarker)
                    {
                        // both eaten, not added to pSym
                        ++pSrc;
                    }
                    else
                    {
                        nMask &= ~ScCharFlags::OdfNameMarker;
                        goto Label_MaskStateMachine;
                    }
                }
                else if( nMask & ScCharFlags::Char )
                {
                    // '[' is a special case in Excel syntax, it can start an
                    // external reference, ID in OOXML like [1]Sheet1!A1 or
                    // Excel_A1 [filename]Sheet!A1 or Excel_R1C1
                    // [filename]Sheet!R1C1 that needs to be scanned
                    // entirely, or can be ocTableRefOpen, of which the first
                    // transforms an ocDBArea into an ocTableRef.
                    if (c == '[' && FormulaGrammar::isExcelSyntax( meGrammar)
                            && eLastOp != ocDBArea && maTableRefs.empty())
                    {
                        // [0]!Global_Range_Name, is a special case in OOXML
                        // syntax, where the '0' is referencing to self and we
                        // do not need it, so we should skip it, in order to
                        // later it will be more recognisable for IsNamedRange.
                        if (FormulaGrammar::isRefConventionOOXML(meGrammar) &&
                                pSrc[0] == '0' && pSrc[1] == ']' && pSrc[2] == '!')
                        {
                            pSrc += 3;
                            c = *pSrc;
                            continue;
                        }

                        nMask &= ~ScCharFlags::Char;
                        goto Label_MaskStateMachine;
                    }
                    else
                    {
                        *pSym++ = c;
                        eState = ssStop;
                    }
                }
                else if( nMask & ScCharFlags::OdfLBracket )
                {
                    // eaten, not added to pSym
                    eState = ssGetReference;
                    mnPredetectedReference = 1;
                }
                else if( nMask & ScCharFlags::CharBool )
                {
                    *pSym++ = c;
                    eState = ssGetBool;
                }
                else if( nMask & ScCharFlags::CharValue )
                {
                    *pSym++ = c;
                    eState = ssGetValue;
                }
                else if( nMask & ScCharFlags::CharString )
                {
                    *pSym++ = c;
                    eState = ssGetString;
                }
                else if( nMask & ScCharFlags::CharErrConst )
                {
                    *pSym++ = c;
                    sal_uInt16 nLevel;
                    if (!maTableRefs.empty() && ((nLevel = maTableRefs.back().mnLevel) == 2 || nLevel == 1))
                        eState = ssGetTableRefItem;
                    else
                        eState = ssGetErrorConstant;
                }
                else if( nMask & ScCharFlags::CharDontCare )
                {
                    addWhitespace( vSpaces, aSpace, c);
                }
                else if( nMask & ScCharFlags::CharIdent )
                {   // try to get a simple ASCII identifier before calling
                    // i18n, to gain performance during import
                    *pSym++ = c;
                    eState = ssGetIdent;
                }
                else
                {
                    bi18n = true;
                    eState = ssStop;
                }
            }
            break;
            case ssGetIdent:
            {
                if ( nMask & ScCharFlags::Ident )
                {   // This catches also $Sheet1.A$1, for example.
                    if( pSym == &cSymbol[ MAXSTRLEN ] )
                    {
                        SetError(FormulaError::StringOverflow);
                        eState = ssStop;
                    }
                    else
                        *pSym++ = c;
                }
                else if (c == '#' && lcl_isUnicodeIgnoreAscii( pSrc, "REF!", 4))
                {
                    // Completely ugly means to catch broken
                    // [$]#REF!.[$]#REF![$]#REF! (one or multiple parts)
                    // references that were written in ODF named ranges
                    // (without embracing [] hence no predetected reference)
                    // and to OOXML and handle them as one symbol.
                    // Also catches these in UI, so we can process them
                    // further.
                    int i = 0;
                    for ( ; i<5; ++i)
                    {
                        if( pSym == &cSymbol[ MAXSTRLEN ] )
                        {
                            SetError(FormulaError::StringOverflow);
                            eState = ssStop;
                            break;  // for
                        }
                        else
                        {
                            *pSym++ = c;
                            c = *pSrc++;
                        }
                    }
                    if (i == 5)
                        c = *((--pSrc)-1);  // position last/next character correctly
                }
                else if (c == ':' && mnRangeOpPosInSymbol < 0)
                {
                    // One range operator may form Sheet1.A:A, which we need to
                    // pass as one entity to IsReference().
                    if( pSym == &cSymbol[ MAXSTRLEN ] )
                    {
                        SetError(FormulaError::StringOverflow);
                        eState = ssStop;
                    }
                    else
                    {
                        mnRangeOpPosInSymbol = pSym - &cSymbol[0];
                        *pSym++ = c;
                    }
                }
                else if ( 128 <= c || '\'' == c )
                {   // High values need reparsing with i18n,
                    // single quoted $'sheet' names too (otherwise we'd had to
                    // implement everything twice).
                    bi18n = true;
                    eState = ssStop;
                }
                else
                {
                    pSrc--;
                    eState = ssStop;
                }
            }
            break;
            case ssGetBool :
            {
                if( nMask & ScCharFlags::Bool )
                {
                    *pSym++ = c;
                    eState = ssStop;
                }
                else
                {
                    pSrc--;
                    eState = ssStop;
                }
            }
            break;
            case ssGetValue :
            {
                if( pSym == &cSymbol[ MAXSTRLEN ] )
                {
                    SetError(FormulaError::StringOverflow);
                    eState = ssStop;
                }
                else if (c == cDecSep || (cDecSepAlt && c == cDecSepAlt))
                {
                    if (++nDecSeps > 1)
                    {
                        // reparse with i18n, may be numeric sheet name as well
                        bi18n = true;
                        eState = ssStop;
                    }
                    else
                        *pSym++ = c;
                }
                else if( nMask & ScCharFlags::Value )
                    *pSym++ = c;
                else if( nMask & ScCharFlags::ValueSep )
                {
                    pSrc--;
                    eState = ssStop;
                }
                else if (c == 'E' || c == 'e')
                {
                    if (GetCharTableFlags( pSrc[0], 0 ) & ScCharFlags::ValueExp)
                        *pSym++ = c;
                    else
                    {
                        // reparse with i18n
                        bi18n = true;
                        eState = ssStop;
                    }
                }
                else if( nMask & ScCharFlags::ValueSign )
                {
                    if (((cLast == 'E') || (cLast == 'e')) &&
                            (GetCharTableFlags( pSrc[0], 0 ) & ScCharFlags::ValueValue))
                    {
                        *pSym++ = c;
                    }
                    else
                    {
                        pSrc--;
                        eState = ssStop;
                    }
                }
                else
                {
                    // reparse with i18n
                    bi18n = true;
                    eState = ssStop;
                }
            }
            break;
            case ssGetString :
            {
                if( nMask & ScCharFlags::StringSep )
                {
                    if ( !bQuote )
                    {
                        if ( *pSrc == '"' )
                            bQuote = true;      // "" => literal "
                        else
                            eState = ssStop;
                    }
                    else
                        bQuote = false;
                }
                if ( !bQuote )
                {
                    if( pSym == &cSymbol[ MAXSTRLEN ] )
                    {
                        SetError(FormulaError::StringOverflow);
                        eState = ssSkipString;
                    }
                    else
                        *pSym++ = c;
                }
            }
            break;
            case ssSkipString:
                if( nMask & ScCharFlags::StringSep )
                    eState = ssStop;
                break;
            case ssGetErrorConstant:
                {
                    // ODFF Error ::= '#' [A-Z0-9]+ ([!?] | ('/' ([A-Z] | ([0-9] [!?]))))
                    // BUT, in UI these may have been translated! So don't
                    // check for ASCII alnum. Note that this construct can't be
                    // parsed with i18n.
                    /* TODO: be strict when reading ODFF, check for ASCII alnum
                     * and proper continuation after '/'. However, even with
                     * the lax parsing only the error constants we have defined
                     * as opcode symbols will be recognized and others result
                     * in ocBad, so the result is actually conformant. */
                    bool bAdd = true;
                    if ('?' == c)
                        eState = ssStop;
                    else if ('!' == c)
                    {
                        // Check if this is #REF! that starts an invalid reference.
                        // Note we have an implicit '!' here at the end.
                        if (pSym - &cSymbol[0] == 4 && lcl_isUnicodeIgnoreAscii( cSymbol, "#REF", 4) &&
                                (GetCharTableFlags( *pSrc, c) & ScCharFlags::Ident))
                            eState = ssGetIdent;
                        else
                            eState = ssStop;
                    }
                    else if ('/' == c)
                    {
                        if (!bErrorConstantHadSlash)
                            bErrorConstantHadSlash = true;
                        else
                        {
                            bAdd = false;
                            eState = ssStop;
                        }
                    }
                    else if ((nMask & ScCharFlags::WordSep) ||
                            (c < 128 && !rtl::isAsciiAlphanumeric( c)))
                    {
                        bAdd = false;
                        eState = ssStop;
                    }
                    if (!bAdd)
                        --pSrc;
                    else
                    {
                        if (pSym == &cSymbol[ MAXSTRLEN ])
                        {
                            SetError( FormulaError::StringOverflow);
                            eState = ssStop;
                        }
                        else
                            *pSym++ = c;
                    }
                }
                break;
            case ssGetTableRefItem:
                {
                    // Scan whatever up to the next ']' closer.
                    if (c != ']')
                    {
                        if( pSym == &cSymbol[ MAXSTRLEN ] )
                        {
                            SetError( FormulaError::StringOverflow);
                            eState = ssStop;
                        }
                        else
                            *pSym++ = c;
                    }
                    else
                    {
                        --pSrc;
                        eState = ssStop;
                    }
                }
                break;
            case ssGetTableRefColumn:
                {
                    // Scan whatever up to the next unescaped ']' closer.
                    if (c != ']' || cLast == '\'')
                    {
                        if( pSym == &cSymbol[ MAXSTRLEN ] )
                        {
                            SetError( FormulaError::StringOverflow);
                            eState = ssStop;
                        }
                        else
                            *pSym++ = c;
                    }
                    else
                    {
                        --pSrc;
                        eState = ssStop;
                    }
                }
                break;
            case ssGetReference:
                if( pSym == &cSymbol[ MAXSTRLEN ] )
                {
                    SetError( FormulaError::StringOverflow);
                    eState = ssSkipReference;
                }
                [[fallthrough]];
            case ssSkipReference:
                // ODF reference: ['External'#$'Sheet'.A1:.B2] with dots being
                // mandatory also if no sheet name. 'External'# is optional,
                // sheet name is optional, quotes around sheet name are
                // optional if no quote contained. [#REF!] is valid.
                // 2nd usage: ['Sheet'.$$'DefinedName']
                // 3rd usage: ['External'#$$'DefinedName']
                // 4th usage: ['External'#$'Sheet'.$$'DefinedName']
                // Also for all these names quotes are optional if no quote
                // contained.
                {

                    // nRefInName: 0 := not in sheet name yet. 'External'
                    // is parsed as if it was a sheet name and nRefInName
                    // is reset when # is encountered immediately after closing
                    // quote. Same with 'DefinedName', nRefInName is cleared
                    // when : is encountered.

                    // Encountered leading $ before sheet name.
                    constexpr int kDollar    = (1 << 1);
                    // Encountered ' opening quote, which may be after $ or
                    // not.
                    constexpr int kOpen      = (1 << 2);
                    // Somewhere in name.
                    constexpr int kName      = (1 << 3);
                    // Encountered ' in name, will be cleared if double or
                    // transformed to kClose if not, in which case kOpen is
                    // cleared.
                    constexpr int kQuote     = (1 << 4);
                    // Past ' closing quote.
                    constexpr int kClose     = (1 << 5);
                    // Encountered # file/sheet separator.
                    constexpr int kFileSep   = (1 << 6);
                    // Past . sheet name separator.
                    constexpr int kPast      = (1 << 7);
                    // Marked name $$ follows sheet name separator, detected
                    // while we're still on the separator. Will be cleared when
                    // entering the name.
                    constexpr int kMarkAhead = (1 << 8);
                    // In marked defined name.
                    constexpr int kDefName   = (1 << 9);
                    // Encountered # of #REF!
                    constexpr int kRefErr    = (1 << 10);

                    bool bAddToSymbol = true;
                    if ((nMask & ScCharFlags::OdfRBracket) && !(nRefInName & kOpen))
                    {
                        OSL_ENSURE( nRefInName & (kPast | kDefName | kRefErr),
                                "ScCompiler::NextSymbol: reference: "
                                "closing bracket ']' without prior sheet name separator '.' violates ODF spec");
                        // eaten, not added to pSym
                        bAddToSymbol = false;
                        eState = ssStop;
                    }
                    else if (cSheetSep == c && nRefInName == 0)
                    {
                        // eat it, no sheet name [.A1]
                        bAddToSymbol = false;
                        nRefInName |= kPast;
                        if ('$' == pSrc[0] && '$' == pSrc[1])
                            nRefInName |= kMarkAhead;
                    }
                    else if (!(nRefInName & kPast) || (nRefInName & (kMarkAhead | kDefName)))
                    {
                        // Not in col/row yet.

                        if (SC_COMPILER_FILE_TAB_SEP == c && (nRefInName & kFileSep))
                            nRefInName = 0;
                        else if ('$' == c && '$' == pSrc[0] && !(nRefInName & kOpen))
                        {
                            nRefInName &= ~kMarkAhead;
                            if (!(nRefInName & kDefName))
                            {
                                // eaten, not added to pSym (2 chars)
                                bAddToSymbol = false;
                                ++pSrc;
                                nRefInName &= kPast;
                                nRefInName |= kDefName;
                            }
                            else
                            {
                                // ScAddress::Parse() will recognize this as
                                // invalid later.
                                if (eState != ssSkipReference)
                                {
                                    *pSym++ = c;

                                    if( pSym == &cSymbol[ MAXSTRLEN ] )
                                    {
                                        SetError( FormulaError::StringOverflow);
                                        eState = ssStop;
                                    }
                                    else
                                        *pSym++ = *pSrc++;
                                }
                                bAddToSymbol = false;
                            }
                        }
                        else if (cSheetPrefix == c && nRefInName == 0)
                            nRefInName |= kDollar;
                        else if ('\'' == c)
                        {
                            // TODO: The conventions' parseExternalName()
                            // should handle quoted names, but as long as they
                            // don't remove non-embedded quotes here.
                            if (!(nRefInName & kName))
                            {
                                nRefInName |= (kOpen | kName);
                                bAddToSymbol = !(nRefInName & kDefName);
                            }
                            else if (!(nRefInName & kOpen))
                            {
                                OSL_FAIL("ScCompiler::NextSymbol: reference: "
                                        "a ''' without the name being enclosed in '...' violates ODF spec");
                            }
                            else if (nRefInName & kQuote)
                            {
                                // escaped embedded quote
                                nRefInName &= ~kQuote;
                            }
                            else
                            {
                                switch (pSrc[0])
                                {
                                    case '\'':
                                        // escapes embedded quote
                                        nRefInName |= kQuote;
                                        break;
                                    case SC_COMPILER_FILE_TAB_SEP:
                                        // sheet name should follow
                                        nRefInName |= kFileSep;
                                        [[fallthrough]];
                                    default:
                                        // quote not followed by quote => close
                                        nRefInName |= kClose;
                                        nRefInName &= ~kOpen;
                                }
                                bAddToSymbol = !(nRefInName & kDefName);
                            }
                        }
                        else if ('#' == c && nRefInName == 0)
                            nRefInName |= kRefErr;
                        else if (cSheetSep == c && !(nRefInName & kOpen))
                        {
                            // unquoted sheet name separator
                            nRefInName |= kPast;
                            if ('$' == pSrc[0] && '$' == pSrc[1])
                                nRefInName |= kMarkAhead;
                        }
                        else if (':' == c && !(nRefInName & kOpen))
                        {
                            OSL_FAIL("ScCompiler::NextSymbol: reference: "
                                    "range operator ':' without prior sheet name separator '.' violates ODF spec");
                            nRefInName = 0;
                            ++mnPredetectedReference;
                        }
                        else if (!(nRefInName & kName))
                        {
                            // start unquoted name
                            nRefInName |= kName;
                        }
                    }
                    else if (':' == c)
                    {
                        // range operator
                        nRefInName = 0;
                        ++mnPredetectedReference;
                    }
                    if (bAddToSymbol && eState != ssSkipReference)
                        *pSym++ = c;    // everything is part of reference
                }
                break;
            case ssStop:
                ;   // nothing, prevent warning
                break;
        }
        cLast = c;
        c = *pSrc;
    }

    if (aSpace.nCount && aSpace.cChar)
        vSpaces.emplace_back(aSpace);

    if ( bi18n )
    {
        const sal_Int32 nOldSrcPos = nSrcPos;
        for (const auto& r : vSpaces)
            nSrcPos += r.nCount;
        // If group separator is not a possible operator and not one of any
        // separators then it may be parsed away in numbers. This is
        // specifically the case with NO-BREAK SPACE, which actually triggers
        // the bi18n case (which we don't want to include as yet another
        // special case above as it is rare enough and doesn't generally occur
        // in formulas).
        const sal_Unicode cGroupSep = ScGlobal::getLocaleData().getNumThousandSep()[0];
        const bool bGroupSeparator = (128 <= cGroupSep && cGroupSep != cSep &&
                cGroupSep != cArrayColSep && cGroupSep != cArrayRowSep &&
                cGroupSep != cDecSep && cGroupSep != cDecSepAlt &&
                cGroupSep != cSheetPrefix && cGroupSep != cSheetSep);
        // If a numeric context triggered bi18n then use the default locale's
        // CharClass, this may accept group separator as well.
        const CharClass* pMyCharClass = (ScGlobal::getCharClass().isDigit( OUString(pStart[nSrcPos]), 0) ?
                &ScGlobal::getCharClass() : pCharClass);
        OUStringBuffer aSymbol;
        mnRangeOpPosInSymbol = -1;
        FormulaError nErr = FormulaError::NONE;
        do
        {
            bi18n = false;
            // special case  (e.g. $'sheetname' in OOO A1)
            if ( pStart[nSrcPos] == cSheetPrefix && pStart[nSrcPos+1] == '\'' )
                aSymbol.append(pStart[nSrcPos++]);

            ParseResult aRes = pConv->parseAnyToken( aFormula, nSrcPos, pMyCharClass, bGroupSeparator);

            if ( !aRes.TokenType )
            {
                nErr = FormulaError::IllegalChar;
                SetError( nErr );      // parsed chars as string
            }
            if ( aRes.EndPos <= nSrcPos )
            {
                // Could not parse anything meaningful.
                assert(!aRes.TokenType);
                nErr = FormulaError::IllegalChar;
                SetError( nErr );
                // Caller has to act on an empty symbol for
                // nSrcPos < aFormula.getLength()
                nSrcPos = nOldSrcPos;
                aSymbol.setLength(0);
            }
            else
            {
                // When having parsed a second reference part, ensure that the
                // i18n parser did not mistakenly parse a number that included
                // a separator which happened to be meant as a parameter
                // separator instead.
                if (mnRangeOpPosInSymbol >= 0 && (aRes.TokenType & KParseType::ASC_NUMBER))
                {
                    for (sal_Int32 i = nSrcPos; i < aRes.EndPos; ++i)
                    {
                        if (pStart[i] == cSep)
                            aRes.EndPos = i;    // also ends for
                    }
                }
                aSymbol.append( pStart + nSrcPos, aRes.EndPos - nSrcPos);
                nSrcPos = aRes.EndPos;
                c = pStart[nSrcPos];
                if ( aRes.TokenType & KParseType::SINGLE_QUOTE_NAME )
                {   // special cases (e.g. 'sheetname'. or 'filename'# in OOO A1)
                    bi18n = (c == cSheetSep || c == SC_COMPILER_FILE_TAB_SEP);
                }
                // One range operator restarts parsing for second reference.
                if (c == ':' && mnRangeOpPosInSymbol < 0)
                {
                    mnRangeOpPosInSymbol = aSymbol.getLength();
                    bi18n = true;
                }
                if ( bi18n )
                    aSymbol.append(pStart[nSrcPos++]);
            }
        } while ( bi18n && nErr == FormulaError::NONE );
        sal_Int32 nLen = aSymbol.getLength();
        if ( nLen > MAXSTRLEN )
        {
            SetError( FormulaError::StringOverflow );
            nLen = MAXSTRLEN;
        }
        if (mnRangeOpPosInSymbol >= nLen)
            mnRangeOpPosInSymbol = -1;
        lcl_UnicodeStrNCpy( cSymbol, aSymbol.getStr(), nLen );
        pSym = &cSymbol[nLen];
    }
    else
    {
        nSrcPos = pSrc - pStart;
        *pSym = 0;
    }
    if (mnRangeOpPosInSymbol >= 0 && mnRangeOpPosInSymbol == (pSym-1) - &cSymbol[0])
    {
        // This is a trailing range operator, which is nonsense. Will be caught
        // in next round.
        mnRangeOpPosInSymbol = -1;
        *--pSym = 0;
        --nSrcPos;
    }
    if ( bAutoCorrect )
        aCorrectedSymbol = OUString(cSymbol, pSym - cSymbol);
    if (bAutoIntersection && vSpaces[nAutoIntersectionSpacesPos].nCount > 1)
        --vSpaces[nAutoIntersectionSpacesPos].nCount;   // replace '!!' with only one space
    return vSpaces;
}

// Convert symbol to token

bool ScCompiler::ParseOpCode( const OUString& rName, bool bInArray )
{
    OpCodeHashMap::const_iterator iLook( mxSymbols->getHashMap().find( rName));
    bool bFound = (iLook != mxSymbols->getHashMap().end());
    if (bFound)
    {
        OpCode eOp = iLook->second;
        if (bInArray)
        {
            if (rName == mxSymbols->getSymbol(ocArrayColSep))
                eOp = ocArrayColSep;
            else if (rName == mxSymbols->getSymbol(ocArrayRowSep))
                eOp = ocArrayRowSep;
        }
        else if (eOp == ocArrayColSep || eOp == ocArrayRowSep)
        {
            if (rName == mxSymbols->getSymbol(ocSep))
                eOp = ocSep;
            else if (rName == ";")
            {
                switch (FormulaGrammar::extractFormulaLanguage( meGrammar))
                {
                    // Only for languages/grammars that actually use ';'
                    // parameter separator.
                    case css::sheet::FormulaLanguage::NATIVE:
                    case css::sheet::FormulaLanguage::ENGLISH:
                    case css::sheet::FormulaLanguage::ODFF:
                    case css::sheet::FormulaLanguage::ODF_11:
                        eOp = ocSep;
                }
            }
        }
        else if (eOp == ocCeil && mxSymbols->isOOXML())
        {
            // Ensure that _xlfn.CEILING.MATH maps to ocCeil_Math. ocCeil is
            // unassigned for import.
            eOp = ocCeil_Math;
        }
        else if (eOp == ocFloor && mxSymbols->isOOXML())
        {
            // Ensure that _xlfn.FLOOR.MATH maps to ocFloor_Math. ocFloor is
            // unassigned for import.
            eOp = ocFloor_Math;
        }
        maRawToken.SetOpCode(eOp);
    }
    else if (mxSymbols->isODFF())
    {
        // ODFF names that are not written in the current mapping but to be
        // recognized. New names will be written in a future release, then
        // exchange (!) with the names in
        // formula/source/core/resource/core_resource.src to be able to still
        // read the old names as well.
        struct FunctionName
        {
            const char* pName;
            OpCode          eOp;
        };
        static const FunctionName aOdffAliases[] = {
            // Renamed old names, still accept them:
            { "B",              ocB },              // B -> BINOM.DIST.RANGE
            { "TDIST",          ocTDist },          // TDIST -> LEGACY.TDIST
            { "ORG.OPENOFFICE.EASTERSUNDAY",   ocEasterSunday },   // ORG.OPENOFFICE.EASTERSUNDAY -> EASTERSUNDAY
            { "ZGZ",            ocRRI },            // ZGZ -> RRI
            { "COLOR",          ocColor },          // COLOR -> ORG.LIBREOFFICE.COLOR
            { "GOALSEEK",       ocBackSolver },     // GOALSEEK -> ORG.OPENOFFICE.GOALSEEK
            { "COM.MICROSOFT.F.DIST", ocFDist_LT }, // fdo#40835, -> FDIST -> COM.MICROSOFT.F.DIST
            { "COM.MICROSOFT.F.INV",  ocFInv_LT }   // tdf#94214, COM.MICROSOFT.F.INV -> FINV (ODF)
            // Renamed new names, prepare to read future names:
            //{ "ORG.OPENOFFICE.XXX", ocXXX }         // XXX -> ORG.OPENOFFICE.XXX
        };
        for (const FunctionName& rOdffAlias : aOdffAliases)
        {
            if (rName.equalsIgnoreAsciiCaseAscii( rOdffAlias.pName))
            {
                maRawToken.SetOpCode( rOdffAlias.eOp);
                bFound = true;
                break;  // for
            }
        }
    }
    else if (mxSymbols->isOOXML())
    {
        // OOXML names that are not written in the current mapping but to be
        // recognized as old versions wrote them.
        struct FunctionName
        {
            const char* pName;
            OpCode      eOp;
        };
        static const FunctionName aOoxmlAliases[] = {
            { "EFFECTIVE",      ocEffect },         // EFFECTIVE -> EFFECT
            { "ERRORTYPE",      ocErrorType },      // ERRORTYPE -> _xlfn.ORG.OPENOFFICE.ERRORTYPE
            { "MULTIRANGE",     ocMultiArea },      // MULTIRANGE -> _xlfn.ORG.OPENOFFICE.MULTIRANGE
            { "GOALSEEK",       ocBackSolver },     // GOALSEEK -> _xlfn.ORG.OPENOFFICE.GOALSEEK
            { "EASTERSUNDAY",   ocEasterSunday },   // EASTERSUNDAY -> _xlfn.ORG.OPENOFFICE.EASTERSUNDAY
            { "CURRENT",        ocCurrent },        // CURRENT -> _xlfn.ORG.OPENOFFICE.CURRENT
            { "STYLE",          ocStyle }           // STYLE -> _xlfn.ORG.OPENOFFICE.STYLE
        };
        for (const FunctionName& rOoxmlAlias : aOoxmlAliases)
        {
            if (rName.equalsIgnoreAsciiCaseAscii( rOoxmlAlias.pName))
            {
                maRawToken.SetOpCode( rOoxmlAlias.eOp);
                bFound = true;
                break;  // for
            }
        }
    }
    else if (mxSymbols->isPODF())
    {
        // PODF names are ODF 1.0/1.1 and also used in API XFunctionAccess.
        // We can't rename them in
        // formula/source/core/resource/core_resource.src but can add
        // additional names to be recognized here so they match the UI names if
        // those are renamed.
        struct FunctionName
        {
            const char* pName;
            OpCode      eOp;
        };
        static const FunctionName aPodfAliases[] = {
            { "EFFECT",  ocEffect }      // EFFECTIVE -> EFFECT
        };
        for (const FunctionName& rPodfAlias : aPodfAliases)
        {
            if (rName.equalsIgnoreAsciiCaseAscii( rPodfAlias.pName))
            {
                maRawToken.SetOpCode( rPodfAlias.eOp);
                bFound = true;
                break;  // for
            }
        }
    }

    if (!bFound)
    {
        OUString aIntName;
        if (mxSymbols->hasExternals())
        {
            // If symbols are set by filters get mapping to exact name.
            ExternalHashMap::const_iterator iExt(
                    mxSymbols->getExternalHashMap().find( rName));
            if (iExt != mxSymbols->getExternalHashMap().end())
            {
                if (ScGlobal::GetAddInCollection()->GetFuncData( (*iExt).second))
                    aIntName = (*iExt).second;
            }
        }
        else
        {
            // Old (deprecated) addins first for legacy.
            if (ScGlobal::GetLegacyFuncCollection()->findByName(OUString(cSymbol)))
            {
                aIntName = cSymbol;
            }
            else
                // bLocalFirst=false for (English) upper full original name
                // (service.function)
                aIntName = ScGlobal::GetAddInCollection()->FindFunction(
                        rName, !mxSymbols->isEnglish());
        }
        if (!aIntName.isEmpty())
        {
            maRawToken.SetExternal( aIntName );     // international name
            bFound = true;
        }
    }
    if (!bFound)
        return false;
    OpCode eOp = maRawToken.GetOpCode();
    if (eOp == ocSub || eOp == ocNegSub)
    {
        bool bShouldBeNegSub =
            (eLastOp == ocOpen || eLastOp == ocSep || eLastOp == ocNegSub ||
             (SC_OPCODE_START_BIN_OP <= eLastOp && eLastOp < SC_OPCODE_STOP_BIN_OP) ||
             eLastOp == ocArrayOpen ||
             eLastOp == ocArrayColSep || eLastOp == ocArrayRowSep);
        if (bShouldBeNegSub && eOp == ocSub)
            maRawToken.NewOpCode( ocNegSub );
            //TODO: if ocNegSub had ForceArray we'd have to set it here
        else if (!bShouldBeNegSub && eOp == ocNegSub)
            maRawToken.NewOpCode( ocSub );
    }
    return bFound;
}

bool ScCompiler::ParseOpCode2( std::u16string_view rName )
{
    if (rName == u"TTT")
    {
        maRawToken.SetOpCode(ocTTT);
        return true;
    }
    if (rName == u"__DEBUG_VAR")
    {
        maRawToken.SetOpCode(ocDebugVar);
        return true;
    }

    return false;
}

static bool lcl_ParenthesisFollows( const sal_Unicode* p )
{
    while (*p == ' ')
        p++;
    return *p == '(';
}

bool ScCompiler::ParseValue( const OUString& rSym )
{
    const sal_Int32 nFormulaLanguage = FormulaGrammar::extractFormulaLanguage( GetGrammar());
    if (nFormulaLanguage == css::sheet::FormulaLanguage::ODFF || nFormulaLanguage == css::sheet::FormulaLanguage::OOXML)
    {
        // Speedup things for ODFF, only well-formed numbers, not locale
        // dependent nor user input.
        rtl_math_ConversionStatus eStatus;
        sal_Int32 nParseEnd;
        double fVal = rtl::math::stringToDouble( rSym, '.', 0, &eStatus, &nParseEnd);
        if (nParseEnd != rSym.getLength())
        {
            // Not (only) a number.

            if (nParseEnd > 0)
                return false;   // partially a number => no such thing

            if (lcl_ParenthesisFollows( aFormula.getStr() + nSrcPos))
                return false;   // some function name, not a constant

            // Could be TRUE or FALSE constant.
            OpCode eOpFunc = ocNone;
            if (rSym.equalsIgnoreAsciiCase("TRUE"))
                eOpFunc = ocTrue;
            else if (rSym.equalsIgnoreAsciiCase("FALSE"))
                eOpFunc = ocFalse;
            if (eOpFunc != ocNone)
            {
                maRawToken.SetOpCode(eOpFunc);
                // add missing trailing parentheses
                maPendingOpCodes.push(ocOpen);
                maPendingOpCodes.push(ocClose);
                return true;
            }
            return false;
        }
        if (eStatus == rtl_math_ConversionStatus_OutOfRange)
        {
            // rtl::math::stringToDouble() recognizes XMLSchema-2 "INF" and
            // "NaN" (case sensitive) that could be named expressions or DB
            // areas as well.
            // rSym is already upper so "NaN" is not possible here.
            if (!std::isfinite(fVal) && rSym == "INF")
            {
                SCTAB nSheet = -1;
                if (GetRangeData( nSheet, rSym))
                    return false;
                if (rDoc.GetDBCollection()->getNamedDBs().findByUpperName(rSym))
                    return false;
            }
            /* TODO: is there a specific reason why we don't accept an infinity
             * value that would raise an error in the interpreter, instead of
             * setting the hard error at the token array already? */
            SetError( FormulaError::IllegalArgument );
        }
        maRawToken.SetDouble( fVal );
        return true;
    }

    double fVal;
    sal_uInt32 nIndex = mxSymbols->isEnglishLocale() ? mrInterpreterContext.NFGetStandardIndex(LANGUAGE_ENGLISH_US) : 0;

    if (!mrInterpreterContext.NFIsNumberFormat(rSym, nIndex, fVal))
        return false;

    SvNumFormatType nType = mrInterpreterContext.NFGetType(nIndex);

    // Don't accept 3:3 as time, it is a reference to entire row 3 instead.
    // Dates should never be entered directly and automatically converted
    // to serial, because the serial would be wrong if null-date changed.
    // Usually it wouldn't be accepted anyway because the date separator
    // clashed with other separators or operators.
    if (nType & (SvNumFormatType::TIME | SvNumFormatType::DATE))
        return false;

    if (nType == SvNumFormatType::LOGICAL)
    {
        if (lcl_ParenthesisFollows( aFormula.getStr() + nSrcPos))
            return false;   // Boolean function instead.
    }

    if( nType == SvNumFormatType::TEXT )
        // HACK: number too big!
        SetError( FormulaError::IllegalArgument );
    maRawToken.SetDouble( fVal );
    return true;
}

bool ScCompiler::ParseString()
{
    if ( cSymbol[0] != '"' )
        return false;
    const sal_Unicode* p = cSymbol+1;
    while ( *p )
        p++;
    sal_Int32 nLen = sal::static_int_cast<sal_Int32>( p - cSymbol - 1 );
    if (!nLen || cSymbol[nLen] != '"')
        return false;
    svl::SharedString aSS = rDoc.GetSharedStringPool().intern(OUString(cSymbol+1, nLen-1));
    maRawToken.SetString(aSS.getData(), aSS.getDataIgnoreCase());
    return true;
}

bool ScCompiler::ParsePredetectedErrRefReference( const OUString& rName, const OUString* pErrRef )
{
    switch (mnPredetectedReference)
    {
        case 1:
            return ParseSingleReference( rName, pErrRef);
        case 2:
            return ParseDoubleReference( rName, pErrRef);
        default:
            return false;
    }
}

bool ScCompiler::ParsePredetectedReference( const OUString& rName )
{
    // Speedup documents with lots of broken references, e.g. sheet deleted.
    // It could also be a broken invalidated reference that contains #REF!
    // (but is not equal to), which we wrote prior to ODFF and also to ODFF
    // between 2013 and 2016 until 5.1.4
    static constexpr OUString aErrRef(u"#REF!"_ustr);    // not localized in ODFF
    sal_Int32 nPos = rName.indexOf( aErrRef);
    if (nPos != -1)
    {
        /* TODO: this may be enhanced by reusing scan information from
         * NextSymbol(), the positions of quotes and special characters found
         * there for $'sheet'.A1:... could be stored in a vector. We don't
         * fully rescan here whether found positions are within single quotes
         * for performance reasons. This code does not check for possible
         * occurrences of insane "valid" sheet names like
         * 'haha.#REF!1fooledyou' and will generate an error on such. */
        if (nPos == 0)
        {
            // Per ODFF the correct string for a reference error is just #REF!,
            // so pass it on.
            if (rName.getLength() == 5)
                return ParseErrorConstant( rName);
            // #REF!.AB42 or #REF!42 or #REF!#REF!
            return ParsePredetectedErrRefReference( rName, &aErrRef);
        }
        sal_Unicode c = rName[nPos-1];      // before #REF!
        if ('$' == c)
        {
            if (nPos == 1)
            {
                // $#REF!.AB42 or $#REF!42 or $#REF!#REF!
                return ParsePredetectedErrRefReference( rName, &aErrRef);
            }
            c = rName[nPos-2];              // before $#REF!
        }
        sal_Unicode c2 = nPos+5 < rName.getLength() ? rName[nPos+5] : 0;     // after #REF!
        switch (c)
        {
            case '.':
                if ('$' == c2 || '#' == c2 || ('0' <= c2 && c2 <= '9'))
                {
                    // sheet.#REF!42 or sheet.#REF!#REF!
                    return ParsePredetectedErrRefReference( rName, &aErrRef);
                }
                break;
            case ':':
                if (mnPredetectedReference > 1 &&
                        ('.' == c2 || '$' == c2 || '#' == c2 ||
                         ('0' <= c2 && c2 <= '9')))
                {
                    // :#REF!.AB42 or :#REF!42 or :#REF!#REF!
                    return ParsePredetectedErrRefReference( rName, &aErrRef);
                }
                break;
            default:
                if (rtl::isAsciiAlpha(c) &&
                        ((mnPredetectedReference > 1 && ':' == c2) || 0 == c2))
                {
                    // AB#REF!: or AB#REF!
                    return ParsePredetectedErrRefReference( rName, &aErrRef);
                }
        }
    }
    switch (mnPredetectedReference)
    {
        case 1:
            return ParseSingleReference( rName);
        case 2:
            return ParseDoubleReference( rName);
    }
    return false;
}

bool ScCompiler::ParseDoubleReference( const OUString& rName, const OUString* pErrRef )
{
    ScRange aRange( aPos, aPos );
    const ScAddress::Details aDetails( pConv->meConv, aPos );
    ScAddress::ExternalInfo aExtInfo;
    ScRefFlags nFlags = aRange.Parse( rName, rDoc, aDetails, &aExtInfo, &maExternalLinks, pErrRef );
    if( nFlags & ScRefFlags::VALID )
    {
        ScComplexRefData aRef;
        aRef.InitRange( aRange );
        aRef.Ref1.SetColRel( (nFlags & ScRefFlags::COL_ABS) == ScRefFlags::ZERO );
        aRef.Ref1.SetRowRel( (nFlags & ScRefFlags::ROW_ABS) == ScRefFlags::ZERO );
        aRef.Ref1.SetTabRel( (nFlags & ScRefFlags::TAB_ABS) == ScRefFlags::ZERO );
        if ( !(nFlags & ScRefFlags::TAB_VALID) )
            aRef.Ref1.SetTabDeleted( true );        // #REF!
        aRef.Ref1.SetFlag3D( ( nFlags & ScRefFlags::TAB_3D ) != ScRefFlags::ZERO );
        aRef.Ref2.SetColRel( (nFlags & ScRefFlags::COL2_ABS) == ScRefFlags::ZERO );
        aRef.Ref2.SetRowRel( (nFlags & ScRefFlags::ROW2_ABS) == ScRefFlags::ZERO );
        aRef.Ref2.SetTabRel( (nFlags & ScRefFlags::TAB2_ABS) == ScRefFlags::ZERO );
        if ( !(nFlags & ScRefFlags::TAB2_VALID) )
            aRef.Ref2.SetTabDeleted( true );        // #REF!
        aRef.Ref2.SetFlag3D( ( nFlags & ScRefFlags::TAB2_3D ) != ScRefFlags::ZERO );
        aRef.SetRange(rDoc.GetSheetLimits(), aRange, aPos);
        if (aExtInfo.mbExternal)
        {
            ScExternalRefManager* pRefMgr = rDoc.GetExternalRefManager();
            const OUString* pRealTab = pRefMgr->getRealTableName(aExtInfo.mnFileId, aExtInfo.maTabName);
            maRawToken.SetExternalDoubleRef(
                aExtInfo.mnFileId, pRealTab ? *pRealTab : aExtInfo.maTabName, aRef);
            maExternalFiles.push_back(aExtInfo.mnFileId);
        }
        else
        {
            maRawToken.SetDoubleReference(aRef);
        }
    }

    return ( nFlags & ScRefFlags::VALID ) != ScRefFlags::ZERO;
}

bool ScCompiler::ParseSingleReference( const OUString& rName, const OUString* pErrRef )
{
    mnCurrentSheetEndPos = 0;
    mnCurrentSheetTab = -1;
    ScAddress aAddr( aPos );
    const ScAddress::Details aDetails( pConv->meConv, aPos );
    ScAddress::ExternalInfo aExtInfo;
    ScRefFlags nFlags = aAddr.Parse( rName, rDoc, aDetails,
            &aExtInfo, &maExternalLinks, &mnCurrentSheetEndPos, pErrRef);
    // Something must be valid in order to recognize Sheet1.blah or blah.a1
    // as a (wrong) reference.
    if( nFlags & ( ScRefFlags::COL_VALID|ScRefFlags::ROW_VALID|ScRefFlags::TAB_VALID ) )
    {
        // Valid given tab and invalid col or row may indicate a sheet-local
        // named expression, bail out early and don't create a reference token.
        if (!(nFlags & ScRefFlags::VALID) && mnCurrentSheetEndPos > 0 &&
                (nFlags & ScRefFlags::TAB_VALID) && (nFlags & ScRefFlags::TAB_3D))
        {
            if (aExtInfo.mbExternal)
            {
                // External names are handled separately.
                mnCurrentSheetEndPos = 0;
                mnCurrentSheetTab = -1;
            }
            else
            {
                mnCurrentSheetTab = aAddr.Tab();
            }
            return false;
        }

        if( HasPossibleNamedRangeConflict( aAddr.Tab()))
        {
            // A named range named e.g. 'num1' is valid with 1k columns, but would become a reference
            // when the document is opened later with 16k columns. Resolve the conflict by not
            // considering it a reference.
            OUString aUpper( ScGlobal::getCharClass().uppercase( rName ));
            mnCurrentSheetTab = aAddr.Tab(); // temporarily set for ParseNamedRange()
            if(ParseNamedRange( aUpper, true )) // only check
                return false;
            mnCurrentSheetTab = -1;
        }

        ScSingleRefData aRef;
        aRef.InitAddress( aAddr );
        aRef.SetColRel( (nFlags & ScRefFlags::COL_ABS) == ScRefFlags::ZERO );
        aRef.SetRowRel( (nFlags & ScRefFlags::ROW_ABS) == ScRefFlags::ZERO );
        aRef.SetTabRel( (nFlags & ScRefFlags::TAB_ABS) == ScRefFlags::ZERO );
        aRef.SetFlag3D( ( nFlags & ScRefFlags::TAB_3D ) != ScRefFlags::ZERO );
        // the reference is really invalid
        if( !( nFlags & ScRefFlags::VALID ) )
        {
            if( !( nFlags & ScRefFlags::COL_VALID ) )
                aRef.SetColDeleted(true);
            if( !( nFlags & ScRefFlags::ROW_VALID ) )
                aRef.SetRowDeleted(true);
            if( !( nFlags & ScRefFlags::TAB_VALID ) )
                aRef.SetTabDeleted(true);
            nFlags |= ScRefFlags::VALID;
        }
        aRef.SetAddress(rDoc.GetSheetLimits(), aAddr, aPos);

        if (aExtInfo.mbExternal)
        {
            ScExternalRefManager* pRefMgr = rDoc.GetExternalRefManager();
            const OUString* pRealTab = pRefMgr->getRealTableName(aExtInfo.mnFileId, aExtInfo.maTabName);
            maRawToken.SetExternalSingleRef(
                aExtInfo.mnFileId, pRealTab ? *pRealTab : aExtInfo.maTabName, aRef);
            maExternalFiles.push_back(aExtInfo.mnFileId);
        }
        else
            maRawToken.SetSingleReference(aRef);
    }

    return ( nFlags & ScRefFlags::VALID ) != ScRefFlags::ZERO;
}

bool ScCompiler::ParseReference( const OUString& rName, const OUString* pErrRef )
{
    // Has to be called before ParseValue

    // A later ParseNamedRange() relies on these, being set in ParseSingleReference()
    // if so, reset in all cases.
    mnCurrentSheetEndPos = 0;
    mnCurrentSheetTab = -1;

    sal_Unicode ch1 = rName[0];
    sal_Unicode cDecSep = ( mxSymbols->isEnglishLocale() ? '.' : ScGlobal::getLocaleData().getNumDecimalSep()[0] );
    if ( ch1 == cDecSep )
        return false;
    // Code further down checks only if cDecSep=='.' so simply obtaining the
    // alternative decimal separator if it's not is sufficient.
    if (cDecSep != '.')
    {
        cDecSep = ScGlobal::getLocaleData().getNumDecimalSepAlt().toChar();
        if ( ch1 == cDecSep )
            return false;
    }
    // Who was that imbecile introducing '.' as the sheet name separator!?!
    if ( rtl::isAsciiDigit( ch1 ) && pConv->getSpecialSymbol( Convention::SHEET_SEPARATOR) == '.' )
    {
        // Numerical sheet name is valid.
        // But English 1.E2 or 1.E+2 is value 100, 1.E-2 is 0.01
        // Don't create a #REF! of values. But also do not bail out on
        // something like 3:3, meaning entire row 3.
        do
        {
            const sal_Int32 nPos = ScGlobal::FindUnquoted( rName, '.');
            if ( nPos == -1 )
            {
                if (ScGlobal::FindUnquoted( rName, ':') != -1)
                    break;      // may be 3:3, continue as usual
                return false;
            }
            sal_Unicode const * const pTabSep = rName.getStr() + nPos;
            sal_Unicode ch2 = pTabSep[1];   // maybe a column identifier
            if ( !(ch2 == '$' || rtl::isAsciiAlpha( ch2 )) )
                return false;
            if ( cDecSep == '.' && (ch2 == 'E' || ch2 == 'e')   // E + - digit
                    && (GetCharTableFlags( pTabSep[2], pTabSep[1] ) & ScCharFlags::ValueExp) )
            {
                // If it is an 1.E2 expression check if "1" is an existent sheet
                // name. If so, a desired value 1.E2 would have to be entered as
                // 1E2 or 1.0E2 or 1.E+2, sorry. Another possibility would be to
                // require numerical sheet names always being entered quoted, which
                // is not desirable (too many 1999, 2000, 2001 sheets in use).
                // Furthermore, XML files created with versions prior to SRC640e
                // wouldn't contain the quotes added by MakeTabStr()/CheckTabQuotes()
                // and would produce wrong formulas if the conditions here are met.
                // If you can live with these restrictions you may remove the
                // check and return an unconditional FALSE.
                OUString aTabName( rName.copy( 0, nPos ) );
                SCTAB nTab;
                if ( !rDoc.GetTable( aTabName, nTab ) )
                    return false;
                // If sheet "1" exists and the expression is 1.E+2 continue as
                // usual, the ScRange/ScAddress parser will take care of it.
            }
        } while(false);
    }

    if (ParseSingleReference( rName, pErrRef))
        return true;

    // Though the range operator is handled explicitly, when encountering
    // something like Sheet1.A:A we will have to treat it as one entity if it
    // doesn't pass as single cell reference.
    if (mnRangeOpPosInSymbol > 0)   // ":foo" would be nonsense
    {
        if (ParseDoubleReference( rName, pErrRef))
            return true;
        // Now try with a symbol up to the range operator, rewind source
        // position.
        assert(mnRangeOpPosInSymbol < MAXSTRLEN);   // We should have caught the maldoers.
        if (mnRangeOpPosInSymbol >= MAXSTRLEN)      // TODO: this check and return
            return false;                           // can be removed when sure.
        sal_Int32 nLen = mnRangeOpPosInSymbol;
        while (cSymbol[++nLen])
            ;
        cSymbol[mnRangeOpPosInSymbol] = 0;
        nSrcPos -= (nLen - mnRangeOpPosInSymbol);
        mnRangeOpPosInSymbol = -1;
        mbRewind = true;
        return true;    // end all checks
    }
    else
    {
        switch (pConv->meConv)
        {
            case FormulaGrammar::CONV_XL_A1:
            case FormulaGrammar::CONV_XL_OOX:
                // Special treatment for the 'E:\[doc]Sheet1:Sheet3'!D5 Excel
                // sickness, mnRangeOpPosInSymbol did not catch the range
                // operator as it is within a quoted name.
                if (rName[0] != '\'')
                    return false;   // Document name has to be single quoted.
                [[fallthrough]];
            case FormulaGrammar::CONV_XL_R1C1:
                // C2 or C[1] are valid entire column references.
                if (ParseDoubleReference( rName, pErrRef))
                    return true;
                break;
            default:
                ;   // nothing
        }
    }
    return false;
}

bool ScCompiler::ParseMacro( const OUString& rName )
{
#if !HAVE_FEATURE_SCRIPTING
    (void) rName;

    return false;
#else

    // Calling SfxObjectShell::GetBasic() may result in all sort of things
    // including obtaining the model and deep down in
    // SfxBaseModel::getDocumentStorage() acquiring the SolarMutex, which when
    // formulas are compiled from a threaded import may result in a deadlock.
    // Check first if we actually could acquire it and if not bail out.
    /* FIXME: yes, but how ... */
    vcl::SolarMutexTryAndBuyGuard g;
    if (!g.isAcquired())
    {
        SAL_WARN( "sc.core", "ScCompiler::ParseMacro - SolarMutex would deadlock, not obtaining Basic");
        return false;   // bad luck
    }

    OUString aName( rName);
    StarBASIC* pObj = nullptr;
    ScDocShell* pDocSh = rDoc.GetDocumentShell();

    try
    {
        if( pDocSh )//XXX
            pObj = pDocSh->GetBasic();
        else
            pObj = SfxApplication::GetBasic();
    }
    catch (...)
    {
        return false;
    }

    if (!pObj)
        return false;

    // ODFF recommends to store user-defined functions prefixed with "USER.",
    // use only unprefixed name if encountered. BASIC doesn't allow '.' in a
    // function name so a function "USER.FOO" could not exist, and macro check
    // is assigned the lowest priority in function name check.
    if (FormulaGrammar::isODFF( GetGrammar()) && aName.startsWithIgnoreAsciiCase("USER."))
        aName = aName.copy(5);

    SbxMethod* pMeth = static_cast<SbxMethod*>(pObj->Find( aName, SbxClassType::Method ));
    if( !pMeth )
    {
        return false;
    }
    // It really should be a BASIC function!
    if( pMeth->GetType() == SbxVOID
     || ( pMeth->IsFixed() && pMeth->GetType() == SbxEMPTY )
     || dynamic_cast<const SbMethod*>( pMeth) ==  nullptr )
    {
        return false;
    }
    maRawToken.SetExternal( aName );
    maRawToken.eOp = ocMacro;
    return true;
#endif
}

const ScRangeData* ScCompiler::GetRangeData( SCTAB& rSheet, const OUString& rUpperName ) const
{
    // try local names first
    rSheet = aPos.Tab();
    const ScRangeName* pRangeName = rDoc.GetRangeName(rSheet);
    const ScRangeData* pData = nullptr;
    if (pRangeName)
        pData = pRangeName->findByUpperName(rUpperName);
    if (!pData)
    {
        pRangeName = rDoc.GetRangeName();
        if (pRangeName)
            pData = pRangeName->findByUpperName(rUpperName);
        if (pData)
            rSheet = -1;
    }
    return pData;
}

bool ScCompiler::HasPossibleNamedRangeConflict( SCTAB nTab ) const
{
    const ScRangeName* pRangeName = rDoc.GetRangeName();
    if (pRangeName && pRangeName->hasPossibleAddressConflict())
        return true;
    pRangeName = rDoc.GetRangeName(nTab);
    if (pRangeName && pRangeName->hasPossibleAddressConflict())
        return true;
    return false;
}

bool ScCompiler::ParseNamedRange( const OUString& rUpperName, bool onlyCheck )
{
    // ParseNamedRange is called only from NextNewToken, with an upper-case string

    SCTAB nSheet = -1;
    const ScRangeData* pData = GetRangeData( nSheet, rUpperName);
    if (pData)
    {
        if (!onlyCheck)
            maRawToken.SetName( nSheet, pData->GetIndex());
        return true;
    }

    // Sheet-local name with sheet specified.
    if (mnCurrentSheetEndPos > 0 && mnCurrentSheetTab >= 0)
    {
        OUString aName( rUpperName.copy( mnCurrentSheetEndPos));
        const ScRangeName* pRangeName = rDoc.GetRangeName( mnCurrentSheetTab);
        if (pRangeName)
        {
            pData = pRangeName->findByUpperName(aName);
            if (pData)
            {
                if (!onlyCheck)
                    maRawToken.SetName( mnCurrentSheetTab, pData->GetIndex());
                return true;
            }
        }
    }

    return false;
}

bool ScCompiler::ParseLambdaFuncName( const OUString& aOrg )
{
    if (m_aLambda.bInLambdaFunction && !aOrg.isEmpty())
    {
        OUString aName = aOrg;
        if (aOrg.startsWithIgnoreAsciiCase(u"_xlpm."))
            aName = aName.copy(6);

        if (m_aLambda.nParaPos % 2 == 1 && m_aLambda.nParaCount > m_aLambda.nParaPos)
            m_aLambda.aNameSet.insert(aName);
        else
        {
            // should already exist the name
            if (m_aLambda.aNameSet.find(aName) == m_aLambda.aNameSet.end())
                return false;
        }
        svl::SharedString aSS = rDoc.GetSharedStringPool().intern(aName);
        maRawToken.SetStringName(aSS.getData(), aSS.getDataIgnoreCase());
        return true;
    }
    return false;
}

bool ScCompiler::ParseExternalNamedRange( const OUString& rSymbol, bool& rbInvalidExternalNameRange )
{
    /* FIXME: This code currently (2008-12-02T15:41+0100 in CWS mooxlsc)
     * correctly parses external named references in OOo, as required per RFE
     * #i3740#, just that we can't store them in ODF yet. We will need an OASIS
     * spec first. Until then don't pretend to support external names that
     * wouldn't survive a save and reload cycle, return false instead. */

    rbInvalidExternalNameRange = false;

    if (!pConv)
        return false;

    OUString aFile, aName;
    if (!pConv->parseExternalName( rSymbol, aFile, aName, rDoc, &maExternalLinks))
        return false;

    if (aFile.getLength() > MAXSTRLEN || aName.getLength() > MAXSTRLEN)
        return false;

    ScExternalRefManager* pRefMgr = rDoc.GetExternalRefManager();
    OUString aTmp = aFile;
    pRefMgr->convertToAbsName(aTmp);
    aFile = aTmp;
    sal_uInt16 nFileId = pRefMgr->getExternalFileId(aFile);
    if (!pRefMgr->isValidRangeName(nFileId, aName))
    {
        rbInvalidExternalNameRange = true;
        // range name doesn't exist in the source document.
        return false;
    }

    const OUString* pRealName = pRefMgr->getRealRangeName(nFileId, aName);
    maRawToken.SetExternalName(nFileId, pRealName ? *pRealName : aTmp);
    maExternalFiles.push_back(nFileId);
    return true;
}

bool ScCompiler::ParseDBRange( const OUString& rName )
{
    ScDBCollection::NamedDBs& rDBs = rDoc.GetDBCollection()->getNamedDBs();
    const ScDBData* p = rDBs.findByUpperName(rName);
    if (!p)
        return false;

    maRawToken.SetName( -1, p->GetIndex()); // DB range is always global.
    maRawToken.eOp = ocDBArea;
    return true;
}

bool ScCompiler::ParseColRowName( const OUString& rName )
{
    bool bInList = false;
    bool bFound = false;
    ScSingleRefData aRef;
    OUString aName( rName );
    DeQuote( aName );
    SCTAB nThisTab = aPos.Tab();
    for ( short jThisTab = 1; jThisTab >= 0 && !bInList; jThisTab-- )
    {   // first check ranges on this sheet, in case of duplicated names
        for ( short jRow=0; jRow<2 && !bInList; jRow++ )
        {
            ScRangePairList* pRL;
            if ( !jRow )
                pRL = rDoc.GetColNameRanges();
            else
                pRL = rDoc.GetRowNameRanges();
            for ( size_t iPair = 0, nPairs = pRL->size(); iPair < nPairs && !bInList; ++iPair )
            {
                const ScRangePair & rR = (*pRL)[iPair];
                const ScRange& rNameRange = rR.GetRange(0);
                if ( jThisTab && (rNameRange.aStart.Tab() > nThisTab ||
                        nThisTab > rNameRange.aEnd.Tab()) )
                    continue;   // for
                ScCellIterator aIter( rDoc, rNameRange );
                for (bool bHas = aIter.first(); bHas && !bInList; bHas = aIter.next())
                {
                    // Don't crash if cell (via CompileNameFormula) encounters
                    // a formula cell without code and
                    // HasStringData/Interpret/Compile is executed and all that
                    // recursively...
                    // Furthermore, *this* cell won't be touched, since no RPN exists yet.
                    CellType eType = aIter.getType();
                    bool bOk = false;
                    if (eType == CELLTYPE_FORMULA)
                    {
                        ScFormulaCell* pFC = aIter.getFormulaCell();
                        bOk = (pFC->GetCode()->GetCodeLen() > 0) && (pFC->aPos != aPos);
                    }
                    else
                        bOk = true;

                    if (bOk && aIter.hasString())
                    {
                        OUString aStr = aIter.getString();
                        if ( ScGlobal::GetTransliteration().isEqual( aStr, aName ) )
                        {
                            aRef.InitFlags();
                            if ( !jRow )
                                aRef.SetColRel( true );     // ColName
                            else
                                aRef.SetRowRel( true );     // RowName
                            aRef.SetAddress(rDoc.GetSheetLimits(), aIter.GetPos(), aPos);
                            bInList = bFound = true;
                        }
                    }
                }
            }
        }
    }
    if ( !bInList && rDoc.GetDocOptions().IsLookUpColRowNames() )
    {   // search in current sheet
        tools::Long nDistance = 0, nMax = 0;
        tools::Long nMyCol = static_cast<tools::Long>(aPos.Col());
        tools::Long nMyRow = static_cast<tools::Long>(aPos.Row());
        bool bTwo = false;
        ScAddress aOne( 0, 0, aPos.Tab() );
        ScAddress aTwo( rDoc.MaxCol(), rDoc.MaxRow(), aPos.Tab() );

        ScAutoNameCache* pNameCache = rDoc.GetAutoNameCache();
        if ( pNameCache )
        {
            //  use GetNameOccurrences to collect all positions of aName on the sheet
            //  (only once), similar to the outer part of the loop in the "else" branch.

            const ScAutoNameAddresses& rAddresses = pNameCache->GetNameOccurrences( aName, aPos.Tab() );

            //  Loop through the found positions, similar to the inner part of the loop in the "else" branch.
            //  The order of addresses in the vector is the same as from ScCellIterator.

            for ( const ScAddress& aAddress : rAddresses )
            {
                if ( bFound )
                {   // stop if everything else is further away
                    if ( nMax < static_cast<tools::Long>(aAddress.Col()) )
                        break;      // aIter
                }
                if ( aAddress != aPos )
                {
                    // same treatment as in isEqual case below

                    SCCOL nCol = aAddress.Col();
                    SCROW nRow = aAddress.Row();
                    tools::Long nC = nMyCol - nCol;
                    tools::Long nR = nMyRow - nRow;
                    if ( bFound )
                    {
                        tools::Long nD = nC * nC + nR * nR;
                        if ( nD < nDistance )
                        {
                            if ( nC < 0 || nR < 0 )
                            {   // right or below
                                bTwo = true;
                                aTwo.Set( nCol, nRow, aAddress.Tab() );
                                nMax = std::max( nMyCol + std::abs( nC ), nMyRow + std::abs( nR ) );
                                nDistance = nD;
                            }
                            else if ( nRow >= aOne.Row() || nMyRow < static_cast<tools::Long>(aOne.Row()) )
                            {
                                // upper left, only if not further up than the
                                // current entry and nMyRow is below (CellIter
                                // runs column-wise)
                                bTwo = false;
                                aOne.Set( nCol, nRow, aAddress.Tab() );
                                nMax = std::max( nMyCol + nC, nMyRow + nR );
                                nDistance = nD;
                            }
                        }
                    }
                    else
                    {
                        aOne.Set( nCol, nRow, aAddress.Tab() );
                        nDistance = nC * nC + nR * nR;
                        nMax = std::max( nMyCol + std::abs( nC ), nMyRow + std::abs( nR ) );

                    }
                    bFound = true;
                }
            }
        }
        else
        {
            ScCellIterator aIter( rDoc, ScRange( aOne, aTwo ) );
            for (bool bHas = aIter.first(); bHas; bHas = aIter.next())
            {
                if ( bFound )
                {   // stop if everything else is further away
                    if ( nMax < static_cast<tools::Long>(aIter.GetPos().Col()) )
                        break;      // aIter
                }
                CellType eType = aIter.getType();
                bool bOk = false;
                if (eType == CELLTYPE_FORMULA)
                {
                    ScFormulaCell* pFC = aIter.getFormulaCell();
                    bOk = (pFC->GetCode()->GetCodeLen() > 0) && (pFC->aPos != aPos);
                }
                else
                    bOk = true;

                if (bOk && aIter.hasString())
                {
                    OUString aStr = aIter.getString();
                    if ( ScGlobal::GetTransliteration().isEqual( aStr, aName ) )
                    {
                        SCCOL nCol = aIter.GetPos().Col();
                        SCROW nRow = aIter.GetPos().Row();
                        tools::Long nC = nMyCol - nCol;
                        tools::Long nR = nMyRow - nRow;
                        if ( bFound )
                        {
                            tools::Long nD = nC * nC + nR * nR;
                            if ( nD < nDistance )
                            {
                                if ( nC < 0 || nR < 0 )
                                {   // right or below
                                    bTwo = true;
                                    aTwo.Set( nCol, nRow, aIter.GetPos().Tab() );
                                    nMax = std::max( nMyCol + std::abs( nC ), nMyRow + std::abs( nR ) );
                                    nDistance = nD;
                                }
                                else if ( nRow >= aOne.Row() || nMyRow < static_cast<tools::Long>(aOne.Row()) )
                                {
                                    // upper left, only if not further up than the
                                    // current entry and nMyRow is below (CellIter
                                    // runs column-wise)
                                    bTwo = false;
                                    aOne.Set( nCol, nRow, aIter.GetPos().Tab() );
                                    nMax = std::max( nMyCol + nC, nMyRow + nR );
                                    nDistance = nD;
                                }
                            }
                        }
                        else
                        {
                            aOne.Set( nCol, nRow, aIter.GetPos().Tab() );
                            nDistance = nC * nC + nR * nR;
                            nMax = std::max( nMyCol + std::abs( nC ), nMyRow + std::abs( nR ) );
                        }
                        bFound = true;
                    }
                }
            }
        }

        if ( bFound )
        {
            ScAddress aAdr;
            if ( bTwo )
            {
                if ( nMyCol >= static_cast<tools::Long>(aOne.Col()) && nMyRow >= static_cast<tools::Long>(aOne.Row()) )
                    aAdr = aOne;        // upper left takes precedence
                else
                {
                    if ( nMyCol < static_cast<tools::Long>(aOne.Col()) )
                    {   // two to the right
                        if ( nMyRow >= static_cast<tools::Long>(aTwo.Row()) )
                            aAdr = aTwo;        // directly right
                        else
                            aAdr = aOne;
                    }
                    else
                    {   // two below or below and right, take the nearest
                        tools::Long nC1 = nMyCol - aOne.Col();
                        tools::Long nR1 = nMyRow - aOne.Row();
                        tools::Long nC2 = nMyCol - aTwo.Col();
                        tools::Long nR2 = nMyRow - aTwo.Row();
                        if ( nC1 * nC1 + nR1 * nR1 <= nC2 * nC2 + nR2 * nR2 )
                            aAdr = aOne;
                        else
                            aAdr = aTwo;
                    }
                }
            }
            else
                aAdr = aOne;
            aRef.InitAddress( aAdr );
            // Prioritize on column label; row label only if the next cell
            // above/below the found label cell is text, or if both are not and
            // the cell below is empty and the next cell to the right is
            // numeric.
            if ((aAdr.Row() < rDoc.MaxRow() && rDoc.HasStringData(
                            aAdr.Col(), aAdr.Row() + 1, aAdr.Tab()))
                    || (aAdr.Row() > 0 && rDoc.HasStringData(
                            aAdr.Col(), aAdr.Row() - 1, aAdr.Tab()))
                    || (aAdr.Row() < rDoc.MaxRow() && rDoc.GetRefCellValue(
                            ScAddress( aAdr.Col(), aAdr.Row() + 1, aAdr.Tab())).isEmpty()
                        && aAdr.Col() < rDoc.MaxCol() && rDoc.GetRefCellValue(
                            ScAddress( aAdr.Col() + 1, aAdr.Row(), aAdr.Tab())).hasNumeric()))
                aRef.SetRowRel( true );     // RowName
            else
                aRef.SetColRel( true );     // ColName
            aRef.SetAddress(rDoc.GetSheetLimits(), aAdr, aPos);
        }
    }
    if ( bFound )
    {
        maRawToken.SetSingleReference( aRef );
        maRawToken.eOp = ocColRowName;
        return true;
    }
    else
        return false;
}

bool ScCompiler::ParseBoolean( const OUString& rName )
{
    OpCodeHashMap::const_iterator iLook( mxSymbols->getHashMap().find( rName ) );
    if( iLook != mxSymbols->getHashMap().end() &&
        ((*iLook).second == ocTrue ||
         (*iLook).second == ocFalse) )
    {
        maRawToken.SetOpCode( (*iLook).second );
        return true;
    }
    else
        return false;
}

bool ScCompiler::ParseErrorConstant( const OUString& rName )
{
    FormulaError nError = GetErrorConstant( rName);
    if (nError != FormulaError::NONE)
    {
        maRawToken.SetErrorConstant( nError);
        return true;
    }
    else
        return false;
}

bool ScCompiler::ParseTableRefItem( const OUString& rName )
{
    bool bItem = false;
    OpCodeHashMap::const_iterator iLook( mxSymbols->getHashMap().find( rName));
    if (iLook != mxSymbols->getHashMap().end())
    {
        // Only called when there actually is a current TableRef, hence
        // accessing maTableRefs.back() is safe.
        ScTableRefToken* p = maTableRefs.back().mxToken.get();
        assert(p);  // not a ScTableRefToken can't be

        switch ((*iLook).second)
        {
            case ocTableRefItemAll:
                bItem = true;
                p->AddItem( ScTableRefToken::ALL);
                break;
            case ocTableRefItemHeaders:
                bItem = true;
                p->AddItem( ScTableRefToken::HEADERS);
                break;
            case ocTableRefItemData:
                bItem = true;
                p->AddItem( ScTableRefToken::DATA);
                break;
            case ocTableRefItemTotals:
                bItem = true;
                p->AddItem( ScTableRefToken::TOTALS);
                break;
            case ocTableRefItemThisRow:
                bItem = true;
                p->AddItem( ScTableRefToken::THIS_ROW);
                break;
            default:
                ;
        }
        if (bItem)
            maRawToken.SetOpCode( (*iLook).second );
    }
    return bItem;
}

namespace {
OUString unescapeTableRefColumnSpecifier( const OUString& rStr )
{
    // '#', '[', ']' and '\'' are escaped with '\''

    if (rStr.indexOf( '\'' ) < 0)
        return rStr;

    const sal_Int32 n = rStr.getLength();
    OUStringBuffer aBuf( n );
    const sal_Unicode* p = rStr.getStr();
    const sal_Unicode* const pStop = p + n;
    bool bEscaped = false;
    for ( ; p < pStop; ++p)
    {
        const sal_Unicode c = *p;
        if (bEscaped)
        {
            aBuf.append( c );
            bEscaped = false;
        }
        else if (c == '\'')
            bEscaped = true;    // unescaped escaping '\''
        else
            aBuf.append( c );
    }
    return aBuf.makeStringAndClear();
}
}

bool ScCompiler::ParseTableRefColumn( const OUString& rName )
{
    // Only called when there actually is a current TableRef, hence
    // accessing maTableRefs.back() is safe.
    ScTableRefToken* p = maTableRefs.back().mxToken.get();
    assert(p);  // not a ScTableRefToken can't be

    ScDBData* pDBData = rDoc.GetDBCollection()->getNamedDBs().findByIndex( p->GetIndex());
    if (!pDBData)
        return false;

    OUString aName( unescapeTableRefColumnSpecifier( rName));

    ScRange aRange;
    pDBData->GetArea( aRange);
    aRange.aEnd.SetTab( aRange.aStart.Tab());
    aRange.aEnd.SetRow( aRange.aStart.Row());

    // Prefer the stored internal table column name, which is also needed for
    // named expressions during document load time when cell content isn't
    // available yet. Also, avoiding a possible calculation step in case the
    // header cell is a formula cell is "a good thing".
    sal_Int32 nOffset = pDBData->GetColumnNameOffset( aName);
    if (nOffset >= 0)
    {
        // This is sneaky... we always use the top row of the database range,
        // regardless of whether it is a header row or not. Code evaluating
        // this reference must take that into account and may have to act
        // differently if it is a header-less table. Which are two places,
        // HandleTableRef() (no change necessary there) and
        // CreateStringFromSingleRef() (must not fallback to cell lookup).
        ScSingleRefData aRef;
        ScAddress aAdr( aRange.aStart);
        aAdr.IncCol( nOffset);
        aRef.InitAddress( aAdr);
        maRawToken.SetSingleReference( aRef );
        return true;
    }

    if (pDBData->HasHeader())
    {
        // Quite similar to IsColRowName() but limited to one row of headers.
        ScCellIterator aIter( rDoc, aRange);
        for (bool bHas = aIter.first(); bHas; bHas = aIter.next())
        {
            CellType eType = aIter.getType();
            bool bOk = false;
            if (eType == CELLTYPE_FORMULA)
            {
                ScFormulaCell* pFC = aIter.getFormulaCell();
                bOk = (pFC->GetCode()->GetCodeLen() > 0) && (pFC->aPos != aPos);
            }
            else
                bOk = true;

            if (bOk && aIter.hasString())
            {
                OUString aStr = aIter.getString();
                if (ScGlobal::GetTransliteration().isEqual( aStr, aName))
                {
                    // If this is successful and the internal column name
                    // lookup was not, it may be worth a warning.
                    SAL_WARN("sc.core", "ScCompiler::IsTableRefColumn - falling back to cell lookup");

                    /* XXX NOTE: we could init the column as relative so copying a
                     * formula across columns would point to the relative column,
                     * but do it absolute because:
                     * a) it makes the reference work in named expressions without
                     * having to distinguish
                     * b) Excel does it the same. */
                    ScSingleRefData aRef;
                    aRef.InitAddress( aIter.GetPos());
                    maRawToken.SetSingleReference( aRef );
                    return true;
                }
            }
        }
    }

    return false;
}

void ScCompiler::SetAutoCorrection( bool bVal )
{
    assert(mbJumpCommandReorder);
    bAutoCorrect = bVal;
    mbStopOnError = !bVal;
}

void ScCompiler::AutoCorrectParsedSymbol()
{
    sal_Int32 nPos = aCorrectedSymbol.getLength();
    if ( !nPos )
        return;

    nPos--;
    const sal_Unicode cQuote = '\"';
    const sal_Unicode cx = 'x';
    const sal_Unicode cX = 'X';
    sal_Unicode c1 = aCorrectedSymbol[0];
    sal_Unicode c2 = aCorrectedSymbol[nPos];
    sal_Unicode c2p = nPos > 0 ? aCorrectedSymbol[nPos-1] : 0;
    if ( c1 == cQuote && c2 != cQuote  )
    {   // "...
        // What's not a word doesn't belong to it.
        // Don't be pedantic: c < 128 should be sufficient here.
        while ( nPos && ((aCorrectedSymbol[nPos] < 128) &&
                ((GetCharTableFlags(aCorrectedSymbol[nPos], aCorrectedSymbol[nPos-1]) &
                (ScCharFlags::Word | ScCharFlags::CharDontCare)) == ScCharFlags::NONE)) )
            nPos--;
        if ( nPos == MAXSTRLEN - 1 )
            aCorrectedSymbol = aCorrectedSymbol.replaceAt( nPos, 1, rtl::OUStringChar(cQuote) );   // '"' the MAXSTRLENth character
        else
            aCorrectedSymbol = aCorrectedSymbol.replaceAt( nPos + 1, 0, rtl::OUStringChar(cQuote) );
        bCorrected = true;
    }
    else if ( c1 != cQuote && c2 == cQuote )
    {   // ..."
        aCorrectedSymbol = OUStringChar(cQuote) + aCorrectedSymbol;
        bCorrected = true;
    }
    else if ( nPos == 0 && (c1 == cx || c1 == cX) )
    {   // x => *
        aCorrectedSymbol = mxSymbols->getSymbol(ocMul);
        bCorrected = true;
    }
    else if ( (GetCharTableFlags( c1, 0 ) & ScCharFlags::CharValue)
           && (GetCharTableFlags( c2, c2p ) & ScCharFlags::CharValue) )
    {
        if ( aCorrectedSymbol.indexOf(cx) >= 0 ) // At least two tokens separated by cx
        {   // x => *
            sal_Unicode c = mxSymbols->getSymbolChar(ocMul);
            aCorrectedSymbol = aCorrectedSymbol.replaceAll(OUStringChar(cx), OUStringChar(c));
            bCorrected = true;
        }
        if ( aCorrectedSymbol.indexOf(cX) >= 0 ) // At least two tokens separated by cX
        {   // X => *
            sal_Unicode c = mxSymbols->getSymbolChar(ocMul);
            aCorrectedSymbol = aCorrectedSymbol.replaceAll(OUStringChar(cX), OUStringChar(c));
            bCorrected = true;
        }
    }
    else
    {
        OUString aSymbol( aCorrectedSymbol );
        OUString aDoc;
        if ( aSymbol[0] == '\'' )
        {
            sal_Int32 nPosition = aSymbol.indexOf( "'#" );
            if (nPosition != -1)
            {   // Split off 'Doc'#, may be d:\... or whatever
                aDoc = aSymbol.copy(0, nPosition + 2);
                aSymbol = aSymbol.copy(nPosition + 2);
            }
        }
        sal_Int32 nRefs = comphelper::string::getTokenCount(aSymbol, ':');
        bool bColons;
        if ( nRefs > 2 )
        {   // duplicated or too many ':'? B:2::C10 => B2:C10
            bColons = true;
            sal_Int32 nIndex = 0;
            OUString aTmp1( aSymbol.getToken( 0, ':', nIndex ) );
            sal_Int32 nLen1 = aTmp1.getLength();
            OUStringBuffer aSym;
            OUString aTmp2;
            bool bLastAlp = true;
            sal_Int32 nStrip = 0;
            sal_Int32 nCount = nRefs;
            for ( sal_Int32 j=1; j<nCount; j++ )
            {
                aTmp2 = aSymbol.getToken( 0, ':', nIndex );
                sal_Int32 nLen2 = aTmp2.getLength();
                if ( nLen1 || nLen2 )
                {
                    if ( nLen1 )
                    {
                        aSym.append(aTmp1);
                        bLastAlp = CharClass::isAsciiAlpha( aTmp1 );
                    }
                    if ( nLen2 )
                    {
                        bool bNextNum = CharClass::isAsciiNumeric( aTmp2 );
                        if ( bLastAlp == bNextNum && nStrip < 1 )
                        {
                            // Must be alternating number/string, only
                            // strip within a reference.
                            nRefs--;
                            nStrip++;
                        }
                        else
                        {
                            if ( !aSym.isEmpty() && aSym[aSym.getLength()-1] != ':')
                                aSym.append(":");
                            nStrip = 0;
                        }
                        bLastAlp = !bNextNum;
                    }
                    else
                    {   // ::
                        nRefs--;
                        if ( nLen1 )
                        {   // B10::C10 ? append ':' on next round
                            if ( !bLastAlp && !CharClass::isAsciiNumeric( aTmp1 ) )
                                nStrip++;
                        }
                    }
                    aTmp1 = aTmp2;
                    nLen1 = nLen2;
                }
                else
                    nRefs--;
            }
            aSymbol = aSym + aTmp1;
            aSym.setLength(0);
        }
        else
            bColons = false;
        if ( nRefs && nRefs <= 2 )
        {   // reference twisted? 4A => A4 etc.
            OUString aTab[2], aRef[2];
            const ScAddress::Details aDetails( pConv->meConv, aPos );
            if ( nRefs == 2 )
            {
                sal_Int32 nIdx{ 0 };
                aRef[0] = aSymbol.getToken( 0, ':', nIdx );
                aRef[1] = aSymbol.getToken( 0, ':', nIdx );
            }
            else
                aRef[0] = aSymbol;

            bool bChanged = false;
            bool bOk = true;
            ScRefFlags nMask = ScRefFlags::VALID | ScRefFlags::COL_VALID | ScRefFlags::ROW_VALID;
            for ( int j=0; j<nRefs; j++ )
            {
                sal_Int32 nTmp = 0;
                sal_Int32 nDotPos = -1;
                while ( (nTmp = aRef[j].indexOf( '.', nTmp )) != -1 )
                    nDotPos = nTmp++;      // the last one counts
                if ( nDotPos != -1 )
                {
                    aTab[j] = aRef[j].copy( 0, nDotPos + 1 );  // with '.'
                    aRef[j] = aRef[j].copy( nDotPos + 1 );
                }
                OUString aOld( aRef[j] );
                OUStringBuffer aStr2;
                const sal_Unicode* p = aRef[j].getStr();
                while ( *p && rtl::isAsciiDigit( *p ) )
                    aStr2.append(*p++);
                aRef[j] = OUString( p );
                aRef[j] += aStr2;
                if ( bColons || aRef[j] != aOld )
                {
                    bChanged = true;
                    ScAddress aAdr;
                    bOk &= ((aAdr.Parse( aRef[j], rDoc, aDetails ) & nMask) == nMask);
                }
            }
            if ( bChanged && bOk )
            {
                aCorrectedSymbol = aDoc;
                aCorrectedSymbol += aTab[0];
                aCorrectedSymbol += aRef[0];
                if ( nRefs == 2 )
                {
                    aCorrectedSymbol += ":";
                    aCorrectedSymbol += aTab[1];
                    aCorrectedSymbol += aRef[1];
                }
                bCorrected = true;
            }
        }
    }
}

bool ScCompiler::ToUpperAsciiOrI18nIsAscii( OUString& rUpper, const OUString& rOrg ) const
{
    if (FormulaGrammar::isODFF( meGrammar) || FormulaGrammar::isOOXML( meGrammar))
    {
        // ODFF and OOXML have defined sets of English function names, avoid
        // i18n overhead.
        rUpper = rOrg.toAsciiUpperCase();
        return true;
    }
    else
    {
        // One of localized or English.
        rUpper = pCharClass->uppercase(rOrg);
        return false;
    }
}

short ScCompiler::GetPossibleParaCount( std::u16string_view rLambdaFormula ) const
{
    sal_Unicode cSep = mxSymbols->getSymbolChar(ocSep);
    sal_Unicode cOpen = mxSymbols->getSymbolChar(ocOpen);
    sal_Unicode cClose = mxSymbols->getSymbolChar(ocClose);
    sal_Unicode cArrayOpen = mxSymbols->getSymbolChar(ocArrayOpen);
    sal_Unicode cArrayClose = mxSymbols->getSymbolChar(ocArrayClose);
    short nBrackets = 0;

    short nCount = std::count_if(rLambdaFormula.begin(), rLambdaFormula.end(),
        [&](sal_Unicode c) {
            if (c == cOpen || c == cArrayOpen || c == '[') {
                nBrackets++;
                return false;
            }
            else if (c == cClose || c == cArrayClose || c == ']') {
                nBrackets--;
                return false;
            }
            else {
                if (nBrackets == 1)
                    return c == cSep;
                else
                    return false;
            }
        });

    return static_cast<short>(nCount + 1);
}

bool ScCompiler::NextNewToken( bool bInArray )
{
    if (!maPendingOpCodes.empty())
    {
        maRawToken.SetOpCode(maPendingOpCodes.front());
        maPendingOpCodes.pop();
        return true;
    }

    bool bAllowBooleans = bInArray;
    const std::vector<Whitespace> vSpaces = NextSymbol(bInArray);

    if (!cSymbol[0])
    {
        if (nSrcPos < aFormula.getLength())
        {
            // Nothing could be parsed, remainder as bad string.
            // NextSymbol() must had set an error for this.
            assert( pArr->GetCodeError() != FormulaError::NONE);
            const OUString aBad( aFormula.copy( nSrcPos));
            svl::SharedString aSS = rDoc.GetSharedStringPool().intern( aBad);
            maRawToken.SetString( aSS.getData(), aSS.getDataIgnoreCase());
            maRawToken.NewOpCode( ocBad);
            nSrcPos = aFormula.getLength();
            // Add bad string as last token.
            return true;
        }
        return false;
    }

    if (!vSpaces.empty())
    {
        ScRawToken aToken;
        for (const auto& rSpace : vSpaces)
        {
            if (rSpace.cChar == 0x20)
            {
                // For now keep this a FormulaByteToken for the nasty
                // significant whitespace intersection. This probably can be
                // changed to a FormulaSpaceToken but then other places may
                // need to be adapted.
                aToken.SetOpCode( ocSpaces );
                aToken.sbyte.cByte = static_cast<sal_uInt8>( std::min<sal_Int32>(rSpace.nCount, 255) );
            }
            else
            {
                aToken.SetOpCode( ocWhitespace );
                aToken.whitespace.nCount = static_cast<sal_uInt8>( std::min<sal_Int32>(rSpace.nCount, 255) );
                aToken.whitespace.cChar = rSpace.cChar;
            }
            if (!static_cast<ScTokenArray*>(pArr)->AddRawToken( aToken ))
            {
                SetError(FormulaError::CodeOverflow);
                return false;
            }
        }
    }

    // Short cut for references when reading ODF to speedup things.
    if (mnPredetectedReference)
    {
        OUString aStr( cSymbol);
        bool bInvalidExternalNameRange;
        if (!ParsePredetectedReference( aStr) && !ParseExternalNamedRange( aStr, bInvalidExternalNameRange ))
        {
            svl::SharedString aSS = rDoc.GetSharedStringPool().intern(aStr);
            maRawToken.SetString(aSS.getData(), aSS.getDataIgnoreCase());
            maRawToken.NewOpCode( ocBad );
        }
        return true;
    }

    if ( (cSymbol[0] == '#' || cSymbol[0] == '$') && cSymbol[1] == 0 &&
            !bAutoCorrect )
    {   // special case to speed up broken [$]#REF documents
        /* FIXME: ISERROR(#REF!) would be valid and true and the formula to
         * be processed as usual. That would need some special treatment,
         * also in NextSymbol() because of possible combinations of
         * #REF!.#REF!#REF! parts. In case of reading ODF that is all
         * handled by IsPredetectedReference(), this case here remains for
         * manual/API input. */
        OUString aBad( aFormula.copy( nSrcPos-1 ) );
        const FormulaToken* pBadToken = pArr->AddBad(aBad);
        eLastOp = pBadToken ? pBadToken->GetOpCode() : ocNone;
        return false;
    }

    if( ParseString() )
        return true;

    bool bMayBeFuncName;
    bool bAsciiNonAlnum;    // operators, separators, ...
    if ( cSymbol[0] < 128 )
    {
        bMayBeFuncName = rtl::isAsciiAlpha(cSymbol[0])
            || (cSymbol[0] == '_' && mxSymbols->isOOXML() && rtl::isAsciiAlpha(cSymbol[1]));
        if (!bMayBeFuncName && (cSymbol[0] == '_' && cSymbol[1] == '_') && !comphelper::IsFuzzing())
        {
            bMayBeFuncName = officecfg::Office::Common::Misc::ExperimentalMode::get();
        }

        bAsciiNonAlnum = !bMayBeFuncName && !rtl::isAsciiDigit( cSymbol[0] );
    }
    else
    {
        OUString aTmpStr( cSymbol[0] );
        bMayBeFuncName = pCharClass->isLetter( aTmpStr, 0 );
        bAsciiNonAlnum = false;
    }

    // Within a TableRef anything except an unescaped '[' or ']' is an item
    // or a column specifier, do not attempt to recognize any other single
    // operator there so even [,] or [+] for a single character column
    // specifier works. Note that space between two ocTableRefOpen is not
    // supported (Table[ [ColumnSpec]]), not only here. Note also that Table[]
    // without any item or column specifier is valid.
    if (bAsciiNonAlnum && cSymbol[1] == 0 && (eLastOp != ocTableRefOpen || cSymbol[0] == '[' || cSymbol[0] == ']'))
    {
        // Shortcut for operators and separators that need no further checks or upper.
        if (ParseOpCode( OUString( cSymbol), bInArray ))
            return true;
    }

    if ( bMayBeFuncName )
    {
        // a function name must be followed by a parenthesis
        const sal_Unicode* p = aFormula.getStr() + nSrcPos;
        while( *p == ' ' )
            p++;
        bMayBeFuncName = ( *p == '(' );
    }

    // Italian ARCTAN.2 resulted in #REF! => ParseOpcode() before
    // ParseReference().

    OUString aUpper;
    bool bAsciiUpper = false;

Label_Rewind:

    do
    {
        const OUString aOrg( cSymbol );

        // Check for TableRef column specifier first, it may be anything.
        if (cSymbol[0] != '#' && !maTableRefs.empty() && maTableRefs.back().mnLevel)
        {
            if (ParseTableRefColumn( aOrg ))
                return true;
            // Do not attempt to resolve as any other name.
            aUpper = aOrg;  // for ocBad
            break;          // do; create ocBad token or set error.
        }

        mbRewind = false;
        aUpper.clear();
        bAsciiUpper = false;

        if (bAsciiNonAlnum)
        {
            bAsciiUpper = ToUpperAsciiOrI18nIsAscii( aUpper, aOrg);
            if (cSymbol[0] == '#')
            {
                // Check for TableRef item specifiers first.
                sal_uInt16 nLevel;
                if (!maTableRefs.empty() && ((nLevel = maTableRefs.back().mnLevel) == 2 || nLevel == 1))
                {
                    if (ParseTableRefItem( aUpper ))
                        return true;
                }

                // This can be either an error constant ...
                if (ParseErrorConstant( aUpper))
                    return true;

                // ... or some invalidated reference starting with #REF!
                // which is handled after the do loop.

                break;  // do; create ocBad token or set error.
            }
            if (ParseOpCode( aUpper, bInArray ))
                return true;
        }

        if (bMayBeFuncName)
        {
            if (aUpper.isEmpty())
                bAsciiUpper = ToUpperAsciiOrI18nIsAscii( aUpper, aOrg);
            if (ParseOpCode( aUpper, bInArray ))
                return true;
        }

        // Column 'DM' ("Deutsche Mark", German currency) couldn't be
        // referred => ParseReference() before ParseValue().
        // Preserve case of file names in external references.
        if (ParseReference( aOrg ))
        {
            if (mbRewind)   // Range operator, but no direct reference.
                continue;   // do; up to range operator.
            // If a syntactically correct reference was recognized but invalid
            // e.g. because of non-existing sheet name => entire reference
            // ocBad to preserve input instead of #REF!.A1
            if (!maRawToken.IsValidReference(rDoc))
            {
                aUpper = aOrg;          // ensure for ocBad
                break;                  // do; create ocBad token or set error.
            }
            return true;
        }

        if (aUpper.isEmpty())
            bAsciiUpper = ToUpperAsciiOrI18nIsAscii( aUpper, aOrg);

        // ParseBoolean() before ParseValue() to catch inline bools without the kludge
        //    for inline arrays.
        if (bAllowBooleans && ParseBoolean( aUpper ))
            return true;

        if (ParseValue( aUpper ))
            return true;

        // User defined names and such do need i18n upper also in ODF.
        if (bAsciiUpper || mbCharClassesDiffer)
        {
            // Use current system locale here because user defined symbols are
            // more likely in that localized language than in the formula
            // language. This in corner cases needs to continue to work for
            // existing documents and environments.
            // Do not change bAsciiUpper from here on for the lowercase() call
            // below in the ocBad case to use the correct CharClass.
            aUpper = ScGlobal::getCharClass().uppercase( aOrg );
        }

        if (ParseNamedRange( aUpper ))
            return true;

        // Compiling a named expression during collecting them in import shall
        // not match arbitrary names that otherwise if all named expressions
        // were present would be recognized as named expression. Such name will
        // flag an error below and will be recompiled in a second step later
        // with ScRangeData::CompileUnresolvedXML()
        if (meExtendedErrorDetection == EXTENDED_ERROR_DETECTION_NAME_NO_BREAK && rDoc.IsImportingXML())
            break;  // while

        // Preserve case of file names in external references.
        bool bInvalidExternalNameRange;
        if (ParseExternalNamedRange( aOrg, bInvalidExternalNameRange ))
            return true;
        // Preserve case of file names in external references even when range
        // is not valid and previous check failed tdf#89330
        if (bInvalidExternalNameRange)
        {
            // add ocBad but do not lowercase
            svl::SharedString aSS = rDoc.GetSharedStringPool().intern(aOrg);
            maRawToken.SetString(aSS.getData(), aSS.getDataIgnoreCase());
            maRawToken.NewOpCode( ocBad );
            return true;
        }
        if (ParseDBRange( aUpper ))
            return true;
        // If followed by '(' (with or without space inbetween) it can not be a
        // column/row label. Prevent arbitrary content detection.
        if (!bMayBeFuncName && ParseColRowName( aUpper ))
            return true;
        if (bMayBeFuncName && ParseMacro( aUpper ))
            return true;
        if (bMayBeFuncName && ParseOpCode2( aUpper ))
            return true;

        if (ParseLambdaFuncName( aOrg ))
            return true;

    } while (mbRewind);

    // Last chance: it could be a broken invalidated reference that contains
    // #REF! (but is not equal to), which we also wrote to ODFF between 2013
    // and 2016 until 5.1.4
    OUString aErrRef( mxSymbols->getSymbol( ocErrRef));
    if (aUpper.indexOf( aErrRef) >= 0 && ParseReference( aUpper, &aErrRef))
    {
        if (mbRewind)
            goto Label_Rewind;
        return true;
    }

    if ( meExtendedErrorDetection != EXTENDED_ERROR_DETECTION_NONE )
    {
        // set an error
        SetError( FormulaError::NoName );
        if (meExtendedErrorDetection == EXTENDED_ERROR_DETECTION_NAME_BREAK)
            return false;   // end compilation
    }

    // Provide single token information and continue. Do not set an error, that
    // would prematurely end compilation. Simple unknown names are handled by
    // the interpreter.
    // Use the same CharClass that was used for uppercase.
    aUpper = ((bAsciiUpper || mbCharClassesDiffer) ? ScGlobal::getCharClass() : *pCharClass).lowercase( aUpper );
    svl::SharedString aSS = rDoc.GetSharedStringPool().intern(aUpper);
    maRawToken.SetString(aSS.getData(), aSS.getDataIgnoreCase());
    maRawToken.NewOpCode( ocBad );
    if ( bAutoCorrect )
        AutoCorrectParsedSymbol();
    return true;
}

void ScCompiler::CreateStringFromXMLTokenArray( OUString& rFormula, OUString& rFormulaNmsp )
{
    bool bExternal = GetGrammar() == FormulaGrammar::GRAM_EXTERNAL;
    sal_uInt16 nExpectedCount = bExternal ? 2 : 1;
    OSL_ENSURE( pArr->GetLen() == nExpectedCount, "ScCompiler::CreateStringFromXMLTokenArray - wrong number of tokens" );
    if( pArr->GetLen() == nExpectedCount )
    {
        FormulaToken** ppTokens = pArr->GetArray();
        // string tokens expected, GetString() will assert if token type is wrong
        rFormula = ppTokens[0]->GetString().getString();
        if( bExternal )
            rFormulaNmsp = ppTokens[1]->GetString().getString();
    }
}

namespace {

class ExternalFileInserter
{
    ScAddress maPos;
    ScExternalRefManager& mrRefMgr;
public:
    ExternalFileInserter(const ScAddress& rPos, ScExternalRefManager& rRefMgr) :
        maPos(rPos), mrRefMgr(rRefMgr) {}

    void operator() (sal_uInt16 nFileId) const
    {
        mrRefMgr.insertRefCell(nFileId, maPos);
    }
};

}

std::unique_ptr<ScTokenArray> ScCompiler::CompileString( const OUString& rFormula )
{
    OSL_ENSURE( meGrammar != FormulaGrammar::GRAM_EXTERNAL, "ScCompiler::CompileString - unexpected grammar GRAM_EXTERNAL" );
    if( meGrammar == FormulaGrammar::GRAM_EXTERNAL )
        SetGrammar( FormulaGrammar::GRAM_PODF );

    ScTokenArray aArr(rDoc);
    pArr = &aArr;
    maArrIterator = FormulaTokenArrayPlainIterator(*pArr);
    aFormula = comphelper::string::strip(rFormula, ' ');

    nSrcPos = 0;
    bCorrected = false;
    if ( bAutoCorrect )
    {
        aCorrectedFormula.clear();
        aCorrectedSymbol.clear();
    }
    sal_uInt8 nForced = 0;   // ==formula forces recalc even if cell is not visible
    if( nSrcPos < aFormula.getLength() && aFormula[nSrcPos] == '=' )
    {
        nSrcPos++;
        nForced++;
        if ( bAutoCorrect )
            aCorrectedFormula += "=";
    }
    if( nSrcPos < aFormula.getLength() && aFormula[nSrcPos] == '=' )
    {
        nSrcPos++;
        nForced++;
        if ( bAutoCorrect )
            aCorrectedFormula += "=";
    }
    struct FunctionStack
    {
        OpCode  eOp;
        short   nSep;
    };
    // FunctionStack only used if PODF or OOXML!
    bool bPODF = FormulaGrammar::isPODF( meGrammar);
    bool bOOXML = FormulaGrammar::isOOXML( meGrammar);
    bool bUseFunctionStack = (bPODF || bOOXML);
    const size_t nAlloc = 512;
    FunctionStack aFuncs[ nAlloc ];
    FunctionStack* pFunctionStack = (bUseFunctionStack && o3tl::make_unsigned(rFormula.getLength()) > nAlloc ?
         new FunctionStack[rFormula.getLength()] : &aFuncs[0]);
    pFunctionStack[0].eOp = ocNone;
    pFunctionStack[0].nSep = 0;
    size_t nFunction = 0;
    size_t nHighWatermark = 0;
    short nBrackets = 0;
    bool bInArray = false;
    eLastOp = ocOpen;
    while( NextNewToken( bInArray ) )
    {
        const OpCode eOp = maRawToken.GetOpCode();
        if (eOp == ocSkip)
            continue;

        switch (eOp)
        {
            case ocOpen:
            {
                if (eLastOp == ocLet)
                {
                    m_aLambda.bInLambdaFunction = true;
                    m_aLambda.nBracketPos = nBrackets;
                    m_aLambda.nParaPos++;
                    m_aLambda.nParaCount = GetPossibleParaCount(rFormula.subView(nSrcPos - 1));
                }

                ++nBrackets;
                if (bUseFunctionStack)
                {
                    ++nFunction;
                    pFunctionStack[ nFunction ].eOp = eLastOp;
                    pFunctionStack[ nFunction ].nSep = 0;
                    nHighWatermark = nFunction;
                }
            }
            break;
            case ocClose:
            {
                if( !nBrackets )
                {
                    SetError( FormulaError::PairExpected );
                    if ( bAutoCorrect )
                    {
                        bCorrected = true;
                        aCorrectedSymbol.clear();
                    }
                }
                else
                {
                    nBrackets--;
                    if (m_aLambda.bInLambdaFunction && m_aLambda.nBracketPos == nBrackets)
                    {
                        m_aLambda.bInLambdaFunction = false;
                        m_aLambda.nBracketPos = nBrackets;
                    }
                }
                if (bUseFunctionStack && nFunction)
                    --nFunction;
            }
            break;
            case ocSep:
            {
                if (bUseFunctionStack)
                    ++pFunctionStack[ nFunction ].nSep;

                if (m_aLambda.bInLambdaFunction && m_aLambda.nBracketPos + 1 == nBrackets)
                    m_aLambda.nParaPos++;
            }
            break;
            case ocArrayOpen:
            {
                if( bInArray )
                    SetError( FormulaError::NestedArray );
                else
                    bInArray = true;
                // Don't count following column separator as parameter separator.
                if (bUseFunctionStack)
                {
                    ++nFunction;
                    pFunctionStack[ nFunction ].eOp = eOp;
                    pFunctionStack[ nFunction ].nSep = 0;
                    nHighWatermark = nFunction;
                }
            }
            break;
            case ocArrayClose:
            {
                if( bInArray )
                {
                    bInArray = false;
                }
                else
                {
                    SetError( FormulaError::PairExpected );
                    if ( bAutoCorrect )
                    {
                        bCorrected = true;
                        aCorrectedSymbol.clear();
                    }
                }
                if (bUseFunctionStack && nFunction)
                    --nFunction;
            }
            break;
            case ocTableRefOpen:
            {
                // Don't count following item separator as parameter separator.
                if (bUseFunctionStack)
                {
                    ++nFunction;
                    pFunctionStack[ nFunction ].eOp = eOp;
                    pFunctionStack[ nFunction ].nSep = 0;
                    nHighWatermark = nFunction;
                }
            }
            break;
            case ocTableRefClose:
            {
                if (bUseFunctionStack && nFunction)
                    --nFunction;
            }
            break;
            case ocColRowName:
            case ocColRowNameAuto:
                // The current implementation of column / row labels doesn't
                // function correctly in grouped cells.
                aArr.SetShareable(false);
            break;
            default:
            break;
        }
        if ((eLastOp != ocOpen || eOp != ocClose) &&
                (eLastOp == ocOpen ||
                 eLastOp == ocSep ||
                 eLastOp == ocArrayRowSep ||
                 eLastOp == ocArrayColSep ||
                 eLastOp == ocArrayOpen) &&
                (eOp == ocSep ||
                 eOp == ocClose ||
                 eOp == ocArrayRowSep ||
                 eOp == ocArrayColSep ||
                 eOp == ocArrayClose))
        {
            // TODO: should we check for known functions with optional empty
            // args so the correction dialog can do better?
            if ( !static_cast<ScTokenArray*>(pArr)->Add( new FormulaMissingToken ) )
            {
                SetError(FormulaError::CodeOverflow); break;
            }
        }
        if (bOOXML)
        {
            // Append a parameter for WEEKNUM, all 1.0
            // Function is already closed, parameter count is nSep+1
            size_t nFunc = nFunction + 1;
            if (eOp == ocClose && nFunc <= nHighWatermark &&
                     pFunctionStack[ nFunc ].nSep == 0 &&
                     pFunctionStack[ nFunc ].eOp == ocWeek)   // 2nd week start
            {
                if (    !static_cast<ScTokenArray*>(pArr)->Add( new FormulaToken( svSep, ocSep)) ||
                        !static_cast<ScTokenArray*>(pArr)->Add( new FormulaDoubleToken( 1.0)))
                {
                    SetError(FormulaError::CodeOverflow); break;
                }
            }
        }
        else if (bPODF)
        {
            /* TODO: for now this is the only PODF adapter. If there were more,
             * factor this out. */
            // Insert ADDRESS() new empty parameter 4 if there is a 4th, now to be 5th.
            if (eOp == ocSep &&
                    pFunctionStack[ nFunction ].eOp == ocAddress &&
                    pFunctionStack[ nFunction ].nSep == 3)
            {
                if (    !static_cast<ScTokenArray*>(pArr)->Add( new FormulaToken( svSep, ocSep)) ||
                        !static_cast<ScTokenArray*>(pArr)->Add( new FormulaDoubleToken( 1.0)))
                {
                    SetError(FormulaError::CodeOverflow); break;
                }
                ++pFunctionStack[ nFunction ].nSep;
            }
        }
        FormulaToken* pNewToken = static_cast<ScTokenArray*>(pArr)->Add( maRawToken.CreateToken(rDoc.GetSheetLimits()));
        if (!pNewToken && eOp == ocArrayClose && pArr->OpCodeBefore( pArr->GetLen()) == ocArrayClose)
        {
            // Nested inline array or non-value/non-string in array. The
            // original tokens are still in the ScTokenArray and not merged
            // into an ScMatrixToken. Set error but keep on tokenizing.
            SetError( FormulaError::BadArrayContent);
        }
        else if (!pNewToken)
        {
            SetError(FormulaError::CodeOverflow);
            break;
        }
        else if (eLastOp == ocRange && pNewToken->GetOpCode() == ocPush && pNewToken->GetType() == svSingleRef)
        {
            static_cast<ScTokenArray*>(pArr)->MergeRangeReference( aPos);
        }
        else if (eLastOp == ocDBArea && pNewToken->GetOpCode() == ocTableRefOpen)
        {
            sal_uInt16 nIdx = pArr->GetLen() - 1;
            const FormulaToken* pPrev = pArr->PeekPrev( nIdx);
            if (pPrev && pPrev->GetOpCode() == ocDBArea)
            {
                ScTableRefToken* pTableRefToken = new ScTableRefToken( pPrev->GetIndex(), ScTableRefToken::TABLE);
                maTableRefs.emplace_back( pTableRefToken);
                // pPrev may be dead hereafter.
                static_cast<ScTokenArray*>(pArr)->ReplaceToken( nIdx, pTableRefToken,
                        FormulaTokenArray::ReplaceMode::CODE_ONLY);
            }
        }
        switch (eOp)
        {
            case ocTableRefOpen:
                SAL_WARN_IF( maTableRefs.empty(), "sc.core", "ocTableRefOpen without TableRefEntry");
                if (maTableRefs.empty())
                    SetError(FormulaError::Pair);
                else
                    ++maTableRefs.back().mnLevel;
                break;
            case ocTableRefClose:
                SAL_WARN_IF( maTableRefs.empty(), "sc.core", "ocTableRefClose without TableRefEntry");
                if (maTableRefs.empty())
                    SetError(FormulaError::Pair);
                else
                {
                    if (--maTableRefs.back().mnLevel == 0)
                        maTableRefs.pop_back();
                }
                break;
            default:
                break;
        }
        eLastOp = maRawToken.GetOpCode();
        if ( bAutoCorrect )
            aCorrectedFormula += aCorrectedSymbol;
    }
    if ( mbCloseBrackets )
    {
        if( bInArray )
        {
            FormulaByteToken aToken( ocArrayClose );
            if( !pArr->AddToken( aToken ) )
            {
                SetError(FormulaError::CodeOverflow);
            }
            else if ( bAutoCorrect )
                aCorrectedFormula += mxSymbols->getSymbol(ocArrayClose);
        }

        if (nBrackets)
        {
            FormulaToken aToken( svSep, ocClose );
            while( nBrackets-- )
            {
                if( !pArr->AddToken( aToken ) )
                {
                    SetError(FormulaError::CodeOverflow);
                    break;  // while
                }
                if ( bAutoCorrect )
                    aCorrectedFormula += mxSymbols->getSymbol(ocClose);
            }
        }
    }
    if ( nForced >= 2 )
        pArr->SetRecalcModeForced();

    if (pFunctionStack != &aFuncs[0])
        delete [] pFunctionStack;

    // remember pArr, in case a subsequent CompileTokenArray() is executed.
    std::unique_ptr<ScTokenArray> pNew(new ScTokenArray( std::move(aArr) ));
    pNew->GenHash();
    // coverity[escape : FALSE] - ownership of pNew is retained by caller, so pArr remains valid
    pArr = pNew.get();
    maArrIterator = FormulaTokenArrayPlainIterator(*pArr);

    if (!maExternalFiles.empty())
    {
        // Remove duplicates, and register all external files found in this cell.
        std::sort(maExternalFiles.begin(), maExternalFiles.end());
        std::vector<sal_uInt16>::iterator itEnd = std::unique(maExternalFiles.begin(), maExternalFiles.end());
        std::for_each(maExternalFiles.begin(), itEnd, ExternalFileInserter(aPos, *rDoc.GetExternalRefManager()));
        maExternalFiles.erase(itEnd, maExternalFiles.end());
    }

    return pNew;
}

std::unique_ptr<ScTokenArray> ScCompiler::CompileString( const OUString& rFormula, const OUString& rFormulaNmsp )
{
    OSL_ENSURE( (GetGrammar() == FormulaGrammar::GRAM_EXTERNAL) || rFormulaNmsp.isEmpty(),
        "ScCompiler::CompileString - unexpected formula namespace for internal grammar" );
    if( GetGrammar() == FormulaGrammar::GRAM_EXTERNAL ) try
    {
        ScFormulaParserPool& rParserPool = rDoc.GetFormulaParserPool();
        uno::Reference< sheet::XFormulaParser > xParser( rParserPool.getFormulaParser( rFormulaNmsp ), uno::UNO_SET_THROW );
        table::CellAddress aReferencePos;
        ScUnoConversion::FillApiAddress( aReferencePos, aPos );
        uno::Sequence< sheet::FormulaToken > aTokenSeq = xParser->parseFormula( rFormula, aReferencePos );
        ScTokenArray aTokenArray(rDoc);
        if( ScTokenConversion::ConvertToTokenArray( rDoc, aTokenArray, aTokenSeq ) )
        {
            // remember pArr, in case a subsequent CompileTokenArray() is executed.
            std::unique_ptr<ScTokenArray> pNew(new ScTokenArray( std::move(aTokenArray) ));
            // coverity[escape : FALSE] - ownership of pNew is retained by caller, so pArr remains valid
            pArr = pNew.get();
            maArrIterator = FormulaTokenArrayPlainIterator(*pArr);
            return pNew;
        }
    }
    catch( uno::Exception& )
    {
    }
    // no success - fallback to some internal grammar and hope the best
    return CompileString( rFormula );
}

ScRangeData* ScCompiler::GetRangeData( const FormulaToken& rToken ) const
{
    return rDoc.FindRangeNameBySheetAndIndex( rToken.GetSheet(), rToken.GetIndex());
}

bool ScCompiler::HandleStringName()
{
    ScTokenArray* pNew = new ScTokenArray(rDoc);
    pNew->AddStringName(mpToken->GetString());
    PushTokenArray(pNew, true);
    return GetToken();
}

bool ScCompiler::HandleRange()
{
    ScTokenArray* pNew;
    const ScRangeData* pRangeData = GetRangeData( *mpToken);
    if (pRangeData)
    {
        FormulaError nErr = pRangeData->GetErrCode();
        if( nErr != FormulaError::NONE )
            SetError( nErr );
        else if (mbJumpCommandReorder)
        {
            // put named formula into parentheses.
            // But only if there aren't any yet, parenthetical
            // ocSep doesn't work, e.g. SUM((...;...))
            // or if not directly between ocSep/parenthesis,
            // e.g. SUM(...;(...;...)) no, SUM(...;(...)*3) yes,
            // in short: if it isn't a self-contained expression.
            FormulaToken* p1 = maArrIterator.PeekPrevNoSpaces();
            FormulaToken* p2 = maArrIterator.PeekNextNoSpaces();
            OpCode eOp1 = (p1 ? p1->GetOpCode() : ocSep);
            OpCode eOp2 = (p2 ? p2->GetOpCode() : ocSep);
            bool bBorder1 = (eOp1 == ocSep || eOp1 == ocOpen);
            bool bBorder2 = (eOp2 == ocSep || eOp2 == ocClose);
            bool bAddPair = !(bBorder1 && bBorder2);
            if ( bAddPair )
            {
                pNew = new ScTokenArray(rDoc);
                pNew->AddOpCode( ocClose );
                PushTokenArray( pNew, true );
            }
            pNew = pRangeData->GetCode()->Clone().release();
            pNew->SetFromRangeName( true );
            PushTokenArray( pNew, true );
            if( pRangeData->HasReferences() )
            {
                // Relative sheet references in sheet-local named expressions
                // shall still point to the same sheet as if used on the
                // original sheet, not shifted to the current position where
                // they are used.
                SCTAB nSheetTab = mpToken->GetSheet();
                if (nSheetTab >= 0 && nSheetTab != aPos.Tab())
                    AdjustSheetLocalNameRelReferences( nSheetTab - aPos.Tab());

                SetRelNameReference();
                MoveRelWrap();
            }
            maArrIterator.Reset();
            if ( bAddPair )
            {
                pNew = new ScTokenArray(rDoc);
                pNew->AddOpCode( ocOpen );
                PushTokenArray( pNew, true );
            }
            return GetToken();
        }
    }
    else
    {
        // No ScRangeData for an already compiled token can happen in BIFF .xls
        // import if the original range is not present in the document.
        pNew = new ScTokenArray(rDoc);
        pNew->Add( new FormulaErrorToken( FormulaError::NoName));
        PushTokenArray( pNew, true );
        return GetToken();
    }
    return true;
}

bool ScCompiler::HandleExternalReference(const FormulaToken& _aToken)
{
    // Handle external range names.
    switch (_aToken.GetType())
    {
        case svExternalSingleRef:
        case svExternalDoubleRef:
            break;
        case svExternalName:
        {
            ScExternalRefManager* pRefMgr = rDoc.GetExternalRefManager();
            const OUString* pFile = pRefMgr->getExternalFileName(_aToken.GetIndex());
            if (!pFile)
            {
                SetError(FormulaError::NoName);
                return true;
            }

            OUString aName = _aToken.GetString().getString();
            ScExternalRefCache::TokenArrayRef xNew = pRefMgr->getRangeNameTokens(
                _aToken.GetIndex(), aName, &aPos);

            if (!xNew)
            {
                SetError(FormulaError::NoName);
                return true;
            }

            ScTokenArray* pNew = xNew->Clone().release();
            PushTokenArray( pNew, true);
            if (FormulaTokenArrayPlainIterator(*pNew).GetNextReference() != nullptr)
            {
                SetRelNameReference();
                MoveRelWrap();
            }
            maArrIterator.Reset();
            return GetToken();
        }
        default:
            OSL_FAIL("Wrong type for external reference!");
            return false;
    }
    return true;
}

void ScCompiler::AdjustSheetLocalNameRelReferences( SCTAB nDelta )
{
    for ( auto t: pArr->References() )
    {
        ScSingleRefData& rRef1 = *t->GetSingleRef();
        if (rRef1.IsTabRel())
            rRef1.IncTab( nDelta);
        if ( t->GetType() == svDoubleRef )
        {
            ScSingleRefData& rRef2 = t->GetDoubleRef()->Ref2;
            if (rRef2.IsTabRel())
                rRef2.IncTab( nDelta);
        }
    }
}

// reference of named range with relative references

void ScCompiler::SetRelNameReference()
{
    for ( auto t: pArr->References() )
    {
        ScSingleRefData& rRef1 = *t->GetSingleRef();
        if ( rRef1.IsColRel() || rRef1.IsRowRel() || rRef1.IsTabRel() )
            rRef1.SetRelName( true );
        if ( t->GetType() == svDoubleRef )
        {
            ScSingleRefData& rRef2 = t->GetDoubleRef()->Ref2;
            if ( rRef2.IsColRel() || rRef2.IsRowRel() || rRef2.IsTabRel() )
                rRef2.SetRelName( true );
        }
    }
}

// Wrap-adjust relative references of a RangeName to current position,
// don't call for other token arrays!
void ScCompiler::MoveRelWrap()
{
    for ( auto t: pArr->References() )
    {
        if ( t->GetType() == svSingleRef || t->GetType() == svExternalSingleRef )
            ScRefUpdate::MoveRelWrap( rDoc, aPos, rDoc.MaxCol(), rDoc.MaxRow(), SingleDoubleRefModifier( *t->GetSingleRef() ).Ref() );
        else
            ScRefUpdate::MoveRelWrap( rDoc, aPos, rDoc.MaxCol(), rDoc.MaxRow(), *t->GetDoubleRef() );
    }
}

// Wrap-adjust relative references of a RangeName to current position,
// don't call for other token arrays!
void ScCompiler::MoveRelWrap( const ScTokenArray& rArr, const ScDocument& rDoc, const ScAddress& rPos,
                              SCCOL nMaxCol, SCROW nMaxRow )
{
    for ( auto t: rArr.References() )
    {
        if ( t->GetType() == svSingleRef || t->GetType() == svExternalSingleRef )
            ScRefUpdate::MoveRelWrap( rDoc, rPos, nMaxCol, nMaxRow, SingleDoubleRefModifier( *t->GetSingleRef() ).Ref() );
        else
            ScRefUpdate::MoveRelWrap( rDoc, rPos, nMaxCol, nMaxRow, *t->GetDoubleRef() );
    }
}

bool ScCompiler::IsCharFlagAllConventions(
    OUString const & rStr, sal_Int32 nPos, ScCharFlags nFlags )
{
    sal_Unicode c = rStr[ nPos ];
    sal_Unicode cLast = nPos > 0 ? rStr[ nPos-1 ] : 0;
    if (c < 128)
    {
        for ( int nConv = formula::FormulaGrammar::CONV_UNSPECIFIED;
                ++nConv < formula::FormulaGrammar::CONV_LAST; )
        {
            if (pConventions[nConv] &&
                    ((pConventions[nConv]->getCharTableFlags(c, cLast) & nFlags) != nFlags))
                return false;
            // convention not known => assume valid
        }
        return true;
    }
    else
        return ScGlobal::getCharClass().isLetterNumeric( rStr, nPos );
}

void ScCompiler::CreateStringFromExternal( OUStringBuffer& rBuffer, const FormulaToken* pTokenP ) const
{
    const FormulaToken* t = pTokenP;
    sal_uInt16 nFileId = t->GetIndex();
    ScExternalRefManager* pRefMgr = rDoc.GetExternalRefManager();
    sal_uInt16 nUsedFileId = pRefMgr->convertFileIdToUsedFileId(nFileId);
    const OUString* pFileName = pRefMgr->getExternalFileName(nFileId);
    if (!pFileName)
        return;

    switch (t->GetType())
    {
        case svExternalName:
            rBuffer.append(pConv->makeExternalNameStr( nFileId, *pFileName, t->GetString().getString()));
        break;
        case svExternalSingleRef:
            pConv->makeExternalRefStr(rDoc.GetSheetLimits(),
                   rBuffer, GetPos(), nUsedFileId, *pFileName, t->GetString().getString(),
                   *t->GetSingleRef());
        break;
        case svExternalDoubleRef:
        {
            vector<OUString> aTabNames;
            pRefMgr->getAllCachedTableNames(nFileId, aTabNames);
            // No sheet names is a valid case if external sheets were not
            // cached in this document and external document is not reachable,
            // else not and worth to be investigated.
            SAL_WARN_IF( aTabNames.empty(), "sc.core", "wrecked cache of external document? '" <<
                    *pFileName << "' '" << t->GetString().getString() << "'");

            pConv->makeExternalRefStr(
                rDoc.GetSheetLimits(), rBuffer, GetPos(), nUsedFileId, *pFileName, aTabNames, t->GetString().getString(),
                *t->GetDoubleRef());
        }
        break;
        default:
            // warning, not error, otherwise we may end up with a never
            // ending message box loop if this was the cursor cell to be redrawn.
            OSL_FAIL("ScCompiler::CreateStringFromToken: unknown type of ocExternalRef");
    }
}

void ScCompiler::CreateStringFromMatrix( OUStringBuffer& rBuffer, const FormulaToken* pTokenP ) const
{
    const ScMatrix* pMatrix = pTokenP->GetMatrix();
    SCSIZE nC, nMaxC, nR, nMaxR;

    pMatrix->GetDimensions( nMaxC, nMaxR);

    rBuffer.append( mxSymbols->getSymbol(ocArrayOpen) );
    for( nR = 0 ; nR < nMaxR ; nR++)
    {
        if( nR > 0)
        {
            rBuffer.append( mxSymbols->getSymbol(ocArrayRowSep) );
        }

        for( nC = 0 ; nC < nMaxC ; nC++)
        {
            if( nC > 0)
            {
                rBuffer.append( mxSymbols->getSymbol(ocArrayColSep) );
            }

            if( pMatrix->IsValue( nC, nR ) )
            {
                if (pMatrix->IsBoolean(nC, nR))
                    AppendBoolean(rBuffer, pMatrix->GetDouble(nC, nR) != 0.0);
                else
                {
                    FormulaError nErr = pMatrix->GetError(nC, nR);
                    if (nErr != FormulaError::NONE)
                        rBuffer.append(ScGlobal::GetErrorString(nErr));
                    else
                        AppendDouble(rBuffer, pMatrix->GetDouble(nC, nR));
                }
            }
            else if( pMatrix->IsEmpty( nC, nR ) )
                ;
            else if( pMatrix->IsStringOrEmpty( nC, nR ) )
                AppendString( rBuffer, pMatrix->GetString(nC, nR).getString() );
        }
    }
    rBuffer.append( mxSymbols->getSymbol(ocArrayClose) );
}

namespace {
void escapeTableRefColumnSpecifier( OUString& rStr )
{
    const sal_Int32 n = rStr.getLength();
    OUStringBuffer aBuf( n * 2 );
    const sal_Unicode* p = rStr.getStr();
    const sal_Unicode* const pStop = p + n;
    for ( ; p < pStop; ++p)
    {
        const sal_Unicode c = *p;
        switch (c)
        {
            case '\'':
            case '[':
            case '#':
            case ']':
                aBuf.append( '\'' );
                break;
            default:
                ;   // nothing
        }
        aBuf.append( c );
    }
    rStr = aBuf.makeStringAndClear();
}
}

void ScCompiler::CreateStringFromSingleRef( OUStringBuffer& rBuffer, const FormulaToken* _pTokenP ) const
{
    const FormulaToken* p;
    OUString aErrRef = GetCurrentOpCodeMap()->getSymbol(ocErrRef);
    const OpCode eOp = _pTokenP->GetOpCode();
    const ScSingleRefData& rRef = *_pTokenP->GetSingleRef();
    ScComplexRefData aRef;
    aRef.Ref1 = aRef.Ref2 = rRef;
    if ( eOp == ocColRowName )
    {
        ScAddress aAbs = rRef.toAbs(rDoc, aPos);
        if (rDoc.HasStringData(aAbs.Col(), aAbs.Row(), aAbs.Tab()))
        {
            OUString aStr = rDoc.GetString(aAbs, &mrInterpreterContext);
            // Enquote to SingleQuoted.
            aStr = aStr.replaceAll(u"'", u"''");
            rBuffer.append('\'');
            rBuffer.append(aStr);
            rBuffer.append('\'');
        }
        else
        {
            rBuffer.append(ScCompiler::GetNativeSymbol(ocErrName));
            pConv->makeRefStr(rDoc.GetSheetLimits(), rBuffer, meGrammar, aPos, aErrRef,
                              GetSetupTabNames(), aRef, true, (pArr && pArr->IsFromRangeName()));
        }
    }
    else if (pArr && (p = maArrIterator.PeekPrevNoSpaces()) && p->GetOpCode() == ocTableRefOpen)
    {
        OUString aStr;
        ScAddress aAbs = rRef.toAbs(rDoc, aPos);
        const ScDBData* pData = rDoc.GetDBAtCursor( aAbs.Col(), aAbs.Row(), aAbs.Tab(), ScDBDataPortion::AREA);
        SAL_WARN_IF( !pData, "sc.core", "ScCompiler::CreateStringFromSingleRef - TableRef without ScDBData: " <<
                aAbs.Format( ScRefFlags::VALID | ScRefFlags::TAB_3D, &rDoc));
        if (pData)
            aStr = pData->GetTableColumnName( aAbs.Col());
        if (aStr.isEmpty())
        {
            if (pData && pData->HasHeader())
            {
                SAL_WARN("sc.core", "ScCompiler::CreateStringFromSingleRef - TableRef falling back to cell: " <<
                        aAbs.Format( ScRefFlags::VALID | ScRefFlags::TAB_3D, &rDoc));
                aStr = rDoc.GetString(aAbs, &mrInterpreterContext);
            }
            else
            {
                SAL_WARN("sc.core", "ScCompiler::CreateStringFromSingleRef - TableRef of empty header-less: " <<
                        aAbs.Format( ScRefFlags::VALID | ScRefFlags::TAB_3D, &rDoc));
                aStr = aErrRef;
            }
        }
        escapeTableRefColumnSpecifier( aStr);
        rBuffer.append(aStr);
    }
    else
        pConv->makeRefStr(rDoc.GetSheetLimits(), rBuffer, meGrammar, aPos, aErrRef,
                          GetSetupTabNames(), aRef, true, (pArr && pArr->IsFromRangeName()));
}

void ScCompiler::CreateStringFromDoubleRef( OUStringBuffer& rBuffer, const FormulaToken* _pTokenP ) const
{
    OUString aErrRef = GetCurrentOpCodeMap()->getSymbol(ocErrRef);
    pConv->makeRefStr(rDoc.GetSheetLimits(), rBuffer, meGrammar, aPos, aErrRef, GetSetupTabNames(),
                      *_pTokenP->GetDoubleRef(), false, (pArr && pArr->IsFromRangeName()));
}

void ScCompiler::CreateStringFromIndex( OUStringBuffer& rBuffer, const FormulaToken* _pTokenP ) const
{
    const OpCode eOp = _pTokenP->GetOpCode();
    OUStringBuffer aBuffer;
    switch ( eOp )
    {
        case ocName:
        {
            const ScRangeData* pData = GetRangeData( *_pTokenP);
            if (pData)
            {
                SCTAB nTab = _pTokenP->GetSheet();
                if (nTab >= 0 && (nTab != aPos.Tab() || mbRefConventionChartOOXML))
                {
                    // Sheet-local on other sheet.
                    OUString aName;
                    if (rDoc.GetName( nTab, aName))
                    {
                        ScCompiler::CheckTabQuotes( aName, pConv->meConv);
                        aBuffer.append( aName);
                    }
                    else
                        aBuffer.append(ScCompiler::GetNativeSymbol(ocErrName));
                    aBuffer.append( pConv->getSpecialSymbol( ScCompiler::Convention::SHEET_SEPARATOR));
                }
                else if (mbRefConventionChartOOXML)
                {
                    aBuffer.append("[0]"
                        + OUStringChar(pConv->getSpecialSymbol(ScCompiler::Convention::SHEET_SEPARATOR)));
                }
                aBuffer.append(pData->GetName());
            }
        }
        break;
        case ocDBArea:
        {
            const ScDBData* pDBData = rDoc.GetDBCollection()->getNamedDBs().findByIndex(_pTokenP->GetIndex());
            if (pDBData)
                aBuffer.append(pDBData->GetName());
        }
        break;
        case ocTableRef:
        {
            if (NeedsTableRefTransformation())
            {
                // Write the resulting reference if TableRef is not supported.
                const ScTableRefToken* pTR = dynamic_cast<const ScTableRefToken*>(_pTokenP);
                if (!pTR)
                    AppendErrorConstant( aBuffer, FormulaError::NoCode);
                else
                {
                    const FormulaToken* pRef = pTR->GetAreaRefRPN();
                    if (!pRef)
                        AppendErrorConstant( aBuffer, FormulaError::NoCode);
                    else
                    {
                        switch (pRef->GetType())
                        {
                            case svSingleRef:
                                CreateStringFromSingleRef( aBuffer, pRef);
                                break;
                            case svDoubleRef:
                                CreateStringFromDoubleRef( aBuffer, pRef);
                                break;
                            case svError:
                                AppendErrorConstant( aBuffer, pRef->GetError());
                                break;
                            default:
                                AppendErrorConstant( aBuffer, FormulaError::NoCode);
                        }
                    }
                }
            }
            else
            {
                const ScDBData* pDBData = rDoc.GetDBCollection()->getNamedDBs().findByIndex(_pTokenP->GetIndex());
                if (pDBData)
                    aBuffer.append(pDBData->GetName());
            }
        }
        break;
        default:
            ;   // nothing
    }
    if ( !aBuffer.isEmpty() )
        rBuffer.append(aBuffer);
    else
        rBuffer.append(ScCompiler::GetNativeSymbol(ocErrName));
}

void ScCompiler::LocalizeString( OUString& rName ) const
{
    ScGlobal::GetAddInCollection()->LocalizeString( rName );
}

bool ScCompiler::GetExcelName( OUString& rName ) const
{
    return ScGlobal::GetAddInCollection()->GetExcelName( rName, LANGUAGE_ENGLISH_US, rName);
}

FormulaTokenRef ScCompiler::ExtendRangeReference( FormulaToken & rTok1, FormulaToken & rTok2 )
{
    return extendRangeReference( rDoc.GetSheetLimits(), rTok1, rTok2, aPos, true/*bReuseDoubleRef*/ );
}

void ScCompiler::fillAddInToken(::std::vector< css::sheet::FormulaOpCodeMapEntry >& _rVec,bool _bIsEnglish) const
{
    // All known AddIn functions.
    sheet::FormulaOpCodeMapEntry aEntry;
    aEntry.Token.OpCode = ocExternal;

    const LanguageTag aEnglishLanguageTag(LANGUAGE_ENGLISH_US);
    ScUnoAddInCollection* pColl = ScGlobal::GetAddInCollection();
    const tools::Long nCount = pColl->GetFuncCount();
    for (tools::Long i=0; i < nCount; ++i)
    {
        const ScUnoAddInFuncData* pFuncData = pColl->GetFuncData(i);
        if (pFuncData)
        {
            if ( _bIsEnglish )
            {
                // This is used with OOXML import, so GetExcelName() is really
                // wanted here until we'll have a parameter to differentiate
                // from the general css::sheet::XFormulaOpCodeMapper case and
                // use pFuncData->GetUpperEnglish().
                OUString aName;
                if (pFuncData->GetExcelName( aEnglishLanguageTag, aName))
                    aEntry.Name = aName;
                else
                    aEntry.Name = pFuncData->GetUpperName();
            }
            else
                aEntry.Name = pFuncData->GetUpperLocal();
            aEntry.Token.Data <<= pFuncData->GetOriginalName();
            _rVec.push_back( aEntry);
        }
    }
    // FIXME: what about those old non-UNO AddIns?
}

bool ScCompiler::HandleColRowName()
{
    ScSingleRefData& rRef = *mpToken->GetSingleRef();
    const ScAddress aAbs = rRef.toAbs(rDoc, aPos);
    if (!rDoc.ValidAddress(aAbs))
    {
        SetError( FormulaError::NoRef );
        return true;
    }
    SCCOL nCol = aAbs.Col();
    SCROW nRow = aAbs.Row();
    SCTAB nTab = aAbs.Tab();
    bool bColName = rRef.IsColRel();
    SCCOL nMyCol = aPos.Col();
    SCROW nMyRow = aPos.Row();
    bool bInList = false;
    bool bValidName = false;
    ScRangePairList* pRL = (bColName ?
        rDoc.GetColNameRanges() : rDoc.GetRowNameRanges());
    ScRange aRange;
    for ( size_t i = 0, nPairs = pRL->size(); i < nPairs; ++i )
    {
        const ScRangePair & rR = (*pRL)[i];
        if ( rR.GetRange(0).Contains( aAbs ) )
        {
            bInList = bValidName = true;
            aRange = rR.GetRange(1);
            if ( bColName )
            {
                aRange.aStart.SetCol( nCol );
                aRange.aEnd.SetCol( nCol );
            }
            else
            {
                aRange.aStart.SetRow( nRow );
                aRange.aEnd.SetRow( nRow );
            }
            break;  // for
        }
    }
    if ( !bInList && rDoc.GetDocOptions().IsLookUpColRowNames() )
    {   // automagically or created by copying and NamePos isn't in list
        ScRefCellValue aCell(rDoc, aAbs);
        bool bString = aCell.hasString();
        if (!bString && aCell.isEmpty())
            bString = true;     // empty cell is ok
        if ( bString )
        {   // corresponds with ScInterpreter::ScColRowNameAuto()
            bValidName = true;
            if ( bColName )
            {   // ColName
                SCROW nStartRow = nRow + 1;
                if ( nStartRow > rDoc.MaxRow() )
                    nStartRow = rDoc.MaxRow();
                SCROW nMaxRow = rDoc.MaxRow();
                if ( nMyCol == nCol )
                {   // formula cell in same column
                    if ( nMyRow == nStartRow )
                    {   // take remainder under name cell
                        nStartRow++;
                        if ( nStartRow > rDoc.MaxRow() )
                            nStartRow = rDoc.MaxRow();
                    }
                    else if ( nMyRow > nStartRow )
                    {   // from name cell down to formula cell
                        nMaxRow = nMyRow - 1;
                    }
                }
                for ( size_t i = 0, nPairs = pRL->size(); i < nPairs; ++i )
                {   // next defined ColNameRange below limits row
                    const ScRangePair & rR = (*pRL)[i];
                    const ScRange& rRange = rR.GetRange(1);
                    if ( rRange.aStart.Col() <= nCol && nCol <= rRange.aEnd.Col() )
                    {   // identical column range
                        SCROW nTmp = rRange.aStart.Row();
                        if ( nStartRow < nTmp && nTmp <= nMaxRow )
                            nMaxRow = nTmp - 1;
                    }
                }
                aRange.aStart.Set( nCol, nStartRow, nTab );
                aRange.aEnd.Set( nCol, nMaxRow, nTab );
            }
            else
            {   // RowName
                SCCOL nStartCol = nCol + 1;
                if ( nStartCol > rDoc.MaxCol() )
                    nStartCol = rDoc.MaxCol();
                SCCOL nMaxCol = rDoc.MaxCol();
                if ( nMyRow == nRow )
                {   // formula cell in same row
                    if ( nMyCol == nStartCol )
                    {   // take remainder right from name cell
                        nStartCol++;
                        if ( nStartCol > rDoc.MaxCol() )
                            nStartCol = rDoc.MaxCol();
                    }
                    else if ( nMyCol > nStartCol )
                    {   // from name cell right to formula cell
                        nMaxCol = nMyCol - 1;
                    }
                }
                for ( size_t i = 0, nPairs = pRL->size(); i < nPairs; ++i )
                {   // next defined RowNameRange to the right limits column
                    const ScRangePair & rR = (*pRL)[i];
                    const ScRange& rRange = rR.GetRange(1);
                    if ( rRange.aStart.Row() <= nRow && nRow <= rRange.aEnd.Row() )
                    {   // identical row range
                        SCCOL nTmp = rRange.aStart.Col();
                        if ( nStartCol < nTmp && nTmp <= nMaxCol )
                            nMaxCol = nTmp - 1;
                    }
                }
                aRange.aStart.Set( nStartCol, nRow, nTab );
                aRange.aEnd.Set( nMaxCol, nRow, nTab );
            }
        }
    }
    if ( bValidName )
    {
        // And now the magic to distinguish between a range and a single
        // cell thereof, which is picked position-dependent of the formula
        // cell. If a direct neighbor is a binary operator (ocAdd, ...) a
        // SingleRef matching the column/row of the formula cell is
        // generated. A ocColRowName or ocIntersect as a neighbor results
        // in a range. Special case: if label is valid for a single cell, a
        // position independent SingleRef is generated.
        bool bSingle = (aRange.aStart == aRange.aEnd);
        bool bFound;
        if ( bSingle )
            bFound = true;
        else
        {
            FormulaToken* p1 = maArrIterator.PeekPrevNoSpaces();
            FormulaToken* p2 = maArrIterator.PeekNextNoSpaces();
            // begin/end of a formula => single
            OpCode eOp1 = p1 ? p1->GetOpCode() : ocAdd;
            OpCode eOp2 = p2 ? p2->GetOpCode() : ocAdd;
            if ( eOp1 != ocColRowName && eOp1 != ocIntersect
                && eOp2 != ocColRowName && eOp2 != ocIntersect )
            {
                if (    (SC_OPCODE_START_BIN_OP <= eOp1 && eOp1 < SC_OPCODE_STOP_BIN_OP) ||
                        (SC_OPCODE_START_BIN_OP <= eOp2 && eOp2 < SC_OPCODE_STOP_BIN_OP))
                    bSingle = true;
            }
            if ( bSingle )
            {   // column and/or row must match range
                if ( bColName )
                {
                    bFound = (aRange.aStart.Row() <= nMyRow
                        && nMyRow <= aRange.aEnd.Row());
                    if ( bFound )
                        aRange.aStart.SetRow( nMyRow );
                }
                else
                {
                    bFound = (aRange.aStart.Col() <= nMyCol
                        && nMyCol <= aRange.aEnd.Col());
                    if ( bFound )
                        aRange.aStart.SetCol( nMyCol );
                }
            }
            else
                bFound = true;
        }
        if ( !bFound )
            SetError(FormulaError::NoRef);
        else if (mbJumpCommandReorder)
        {
            ScTokenArray* pNew = new ScTokenArray(rDoc);
            if ( bSingle )
            {
                ScSingleRefData aRefData;
                aRefData.InitAddress( aRange.aStart );
                if ( bColName )
                    aRefData.SetColRel( true );
                else
                    aRefData.SetRowRel( true );
                aRefData.SetAddress(rDoc.GetSheetLimits(), aRange.aStart, aPos);
                pNew->AddSingleReference( aRefData );
            }
            else
            {
                ScComplexRefData aRefData;
                aRefData.InitRange( aRange );
                if ( bColName )
                {
                    aRefData.Ref1.SetColRel( true );
                    aRefData.Ref2.SetColRel( true );
                }
                else
                {
                    aRefData.Ref1.SetRowRel( true );
                    aRefData.Ref2.SetRowRel( true );
                }
                aRefData.SetRange(rDoc.GetSheetLimits(), aRange, aPos);
                if ( bInList )
                    pNew->AddDoubleReference( aRefData );
                else
                {   // automagically
                    pNew->Add( new ScDoubleRefToken( rDoc.GetSheetLimits(), aRefData, ocColRowNameAuto ) );
                }
            }
            PushTokenArray( pNew, true );
            return GetToken();
        }
    }
    else
        SetError(FormulaError::NoName);
    return true;
}

bool ScCompiler::HandleDbData()
{
    ScDBData* pDBData = rDoc.GetDBCollection()->getNamedDBs().findByIndex(mpToken->GetIndex());
    if ( !pDBData )
        SetError(FormulaError::NoName);
    else if (mbJumpCommandReorder)
    {
        ScComplexRefData aRefData;
        aRefData.InitFlags();
        ScRange aRange;
        pDBData->GetArea(aRange);
        aRange.aEnd.SetTab(aRange.aStart.Tab());
        aRefData.SetRange(rDoc.GetSheetLimits(), aRange, aPos);
        ScTokenArray* pNew = new ScTokenArray(rDoc);
        pNew->AddDoubleReference( aRefData );
        PushTokenArray( pNew, true );
        return GetToken();
    }
    return true;
}

bool ScCompiler::GetTokenIfOpCode( OpCode eOp )
{
    const formula::FormulaToken* p = maArrIterator.PeekNextNoSpaces();
    if (p && p->GetOpCode() == eOp)
        return GetToken();
    return false;
}


/* Documentation on MS-Excel Table structured references:
 * https://support.office.com/en-us/article/Use-structured-references-in-Excel-table-formulas-75fb07d3-826a-449c-b76f-363057e3d16f
 * * as of Excel 2013
 * [MS-XLSX]: Formulas https://msdn.microsoft.com/en-us/library/dd906358.aspx
 * * look for structure-reference
 */

bool ScCompiler::HandleTableRef()
{
    ScTableRefToken* pTR = dynamic_cast<ScTableRefToken*>(mpToken.get());
    if (!pTR)
    {
        SetError(FormulaError::UnknownToken);
        return true;
    }

    ScDBData* pDBData = rDoc.GetDBCollection()->getNamedDBs().findByIndex( pTR->GetIndex());
    if ( !pDBData )
        SetError(FormulaError::NoName);
    else if (mbJumpCommandReorder)
    {
        ScRange aDBRange;
        pDBData->GetArea(aDBRange);
        aDBRange.aEnd.SetTab(aDBRange.aStart.Tab());
        ScRange aRange( aDBRange);
        FormulaError nError = FormulaError::NONE;
        bool bForwardToClose = false;
        ScTableRefToken::Item eItem = pTR->GetItem();
        switch (eItem)
        {
            case ScTableRefToken::TABLE:
                {
                    // The table name without items references the table data,
                    // without headers or totals.
                    if (pDBData->HasHeader())
                        aRange.aStart.IncRow();
                    if (pDBData->HasTotals())
                        aRange.aEnd.IncRow(-1);
                    if (aRange.aEnd.Row() < aRange.aStart.Row())
                        nError = FormulaError::NoRef;
                    bForwardToClose = true;
                }
                break;
            case ScTableRefToken::ALL:
                {
                    bForwardToClose = true;
                }
                break;
            case ScTableRefToken::HEADERS:
                {
                    if (pDBData->HasHeader())
                        aRange.aEnd.SetRow( aRange.aStart.Row());
                    else
                        nError = FormulaError::NoRef;
                    bForwardToClose = true;
                }
                break;
            case ScTableRefToken::DATA:
                {
                    if (pDBData->HasHeader())
                        aRange.aStart.IncRow();
                }
                [[fallthrough]];
            case ScTableRefToken::HEADERS_DATA:
                {
                    if (pDBData->HasTotals())
                        aRange.aEnd.IncRow(-1);
                    if (aRange.aEnd.Row() < aRange.aStart.Row())
                        nError = FormulaError::NoRef;
                    bForwardToClose = true;
                }
                break;
            case ScTableRefToken::TOTALS:
                {
                    if (pDBData->HasTotals())
                        aRange.aStart.SetRow( aRange.aEnd.Row());
                    else
                        nError = FormulaError::NoRef;
                    bForwardToClose = true;
                }
                break;
            case ScTableRefToken::DATA_TOTALS:
                {
                    if (pDBData->HasHeader())
                        aRange.aStart.IncRow();
                    if (aRange.aEnd.Row() < aRange.aStart.Row())
                        nError = FormulaError::NoRef;
                    bForwardToClose = true;
                }
                break;
            case ScTableRefToken::THIS_ROW:
                {
                    if (aRange.aStart.Row() <= aPos.Row() && aPos.Row() <= aRange.aEnd.Row())
                    {
                        aRange.aStart.SetRow( aPos.Row());
                        aRange.aEnd.SetRow( aPos.Row());
                    }
                    else
                    {
                        nError = FormulaError::NoValue;
                        // For *some* relative row reference in named
                        // expressions' thisrow special handling below.
                        aRange.aEnd.SetRow( aRange.aStart.Row());
                    }
                    bForwardToClose = true;
                }
                break;
        }
        bool bColumnRange = false;
        bool bCol1Rel = false;
        bool bCol1RelName = false;
        int nLevel = 0;
        if (bForwardToClose && GetTokenIfOpCode( ocTableRefOpen))
        {
            ++nLevel;
            enum
            {
                sOpen,
                sItem,
                sClose,
                sSep,
                sLast,
                sStop
            } eState = sOpen;
            do
            {
                const formula::FormulaToken* p = maArrIterator.PeekNextNoSpaces();
                if (!p)
                    eState = sStop;
                else
                {
                    switch (p->GetOpCode())
                    {
                        case ocTableRefOpen:
                            eState = ((eState == sOpen || eState == sSep) ? sOpen : sStop);
                            if (++nLevel > 2)
                            {
                                SetError( FormulaError::Pair);
                                eState = sStop;
                            }
                            break;
                        case ocTableRefItemAll:
                        case ocTableRefItemHeaders:
                        case ocTableRefItemData:
                        case ocTableRefItemTotals:
                        case ocTableRefItemThisRow:
                            eState = ((eState == sOpen) ? sItem : sStop);
                            break;
                        case ocTableRefClose:
                            eState = ((eState == sItem || eState == sClose) ? sClose : sStop);
                            if (eState != sStop && --nLevel == 0)
                                eState = sLast;
                            break;
                        case ocSep:
                            eState = ((eState == sClose) ? sSep : sStop);
                            break;
                        case ocPush:
                            if (eState == sOpen && p->GetType() == svSingleRef)
                            {
                                bColumnRange = true;
                                bCol1Rel = p->GetSingleRef()->IsColRel();
                                bCol1RelName = p->GetSingleRef()->IsRelName();
                                eState = sLast;
                            }
                            else
                            {
                                eState = sStop;
                            }
                            break;
                        case ocBad:
                            eState = sLast;
                            if (nError == FormulaError::NONE)
                                nError = FormulaError::NoName;
                            break;
                        default:
                            eState = sStop;
                    }
                    if (eState != sStop)
                        GetToken();
                    if (eState == sLast)
                        eState = sStop;
                }
            } while (eState != sStop);
        }
        ScTokenArray* pNew = new ScTokenArray(rDoc);
        if (nError == FormulaError::NONE || nError == FormulaError::NoValue)
        {
            bool bCol2Rel = false;
            bool bCol2RelName = false;
            // The FormulaError::NoValue case generates a thisrow reference that can be
            // used to save named expressions in A1 syntax notation.
            if (bColumnRange)
            {
                // Limit range to specified columns.
                ScRange aColRange( ScAddress::INITIALIZE_INVALID );
                switch (mpToken->GetType())
                {
                    case svSingleRef:
                        {
                            aColRange.aStart = aColRange.aEnd = mpToken->GetSingleRef()->toAbs(rDoc, aPos);
                            if (    GetTokenIfOpCode( ocTableRefClose) && (nLevel--) &&
                                    GetTokenIfOpCode( ocRange) &&
                                    GetTokenIfOpCode( ocTableRefOpen) && (++nLevel) &&
                                    GetTokenIfOpCode( ocPush))
                            {
                                if (mpToken->GetType() != svSingleRef)
                                    aColRange = ScRange( ScAddress::INITIALIZE_INVALID);
                                else
                                {
                                    aColRange.aEnd = mpToken->GetSingleRef()->toAbs(rDoc, aPos);
                                    aColRange.PutInOrder();
                                    bCol2Rel = mpToken->GetSingleRef()->IsColRel();
                                    bCol2RelName = mpToken->GetSingleRef()->IsRelName();
                                }
                            }
                        }
                        break;
                    default:
                        ;   // nothing
                }
                // coverity[copy_paste_error : FALSE] - this is correct, aStart in both aDBRange uses
                if (aColRange.aStart.Row() != aDBRange.aStart.Row() || aColRange.aEnd.Row() != aDBRange.aStart.Row())
                    aRange = ScRange( ScAddress::INITIALIZE_INVALID);
                else
                {
                    aColRange.aEnd.SetRow( aRange.aEnd.Row());
                    aRange = aRange.Intersection( aColRange);
                }
            }
            if (aRange.IsValid())
            {
                if (aRange.aStart == aRange.aEnd)
                {
                    ScSingleRefData aRefData;
                    aRefData.InitFlags();
                    aRefData.SetColRel( bCol1Rel);
                    if (eItem == ScTableRefToken::THIS_ROW)
                    {
                        aRefData.SetRowRel( true);
                        if (!bCol1RelName)
                            bCol1RelName = pArr->IsFromRangeName();
                    }
                    aRefData.SetRelName( bCol1RelName);
                    aRefData.SetFlag3D( true);
                    if (nError != FormulaError::NONE)
                    {
                        aRefData.SetAddress( rDoc.GetSheetLimits(), aRange.aStart, aRange.aStart);
                        pTR->SetAreaRefRPN( new ScSingleRefToken(rDoc.GetSheetLimits(), aRefData));   // set reference at TableRef
                        pNew->Add( new FormulaErrorToken( nError));             // set error in RPN
                    }
                    else
                    {
                        aRefData.SetAddress( rDoc.GetSheetLimits(), aRange.aStart, aPos);
                        pTR->SetAreaRefRPN( pNew->AddSingleReference( aRefData));
                    }
                }
                else
                {
                    ScComplexRefData aRefData;
                    aRefData.InitFlags();
                    aRefData.Ref1.SetColRel( bCol1Rel);
                    aRefData.Ref2.SetColRel( bCol2Rel);
                    bool bRelName = bCol1RelName || bCol2RelName;
                    if (eItem == ScTableRefToken::THIS_ROW)
                    {
                        aRefData.Ref1.SetRowRel( true);
                        aRefData.Ref2.SetRowRel( true);
                        if (!bRelName)
                            bRelName = pArr->IsFromRangeName();
                    }
                    aRefData.Ref1.SetRelName( bRelName);
                    aRefData.Ref2.SetRelName( bRelName);
                    aRefData.Ref1.SetFlag3D( true);
                    if (nError != FormulaError::NONE)
                    {
                        aRefData.SetRange( rDoc.GetSheetLimits(), aRange, aRange.aStart);
                        pTR->SetAreaRefRPN( new ScDoubleRefToken(rDoc.GetSheetLimits(), aRefData));   // set reference at TableRef
                        pNew->Add( new FormulaErrorToken( nError));             // set error in RPN
                    }
                    else
                    {
                        aRefData.SetRange( rDoc.GetSheetLimits(), aRange, aPos);
                        pTR->SetAreaRefRPN( pNew->AddDoubleReference( aRefData));
                    }
                }
            }
            else
            {
                pTR->SetAreaRefRPN( pNew->Add( new FormulaErrorToken( FormulaError::NoRef)));
            }
        }
        else
        {
            pTR->SetAreaRefRPN( pNew->Add( new FormulaErrorToken( nError)));
        }
        while (nLevel-- > 0)
        {
            if (!GetTokenIfOpCode( ocTableRefClose))
                SetError( FormulaError::Pair);
        }
        PushTokenArray( pNew, true );
        return GetToken();
    }
    return true;
}

formula::ParamClass ScCompiler::GetForceArrayParameter( const formula::FormulaToken* pToken, sal_uInt16 nParam ) const
{
    return ScParameterClassification::GetParameterType( pToken, nParam);
}

bool ScCompiler::ParameterMayBeImplicitIntersection(const FormulaToken* token, int parameter)
{
    formula::ParamClass param = ScParameterClassification::GetParameterType( token, parameter );
    return param == Value || param == Array;
}

bool ScCompiler::SkipImplicitIntersectionOptimization(const FormulaToken* token) const
{
    if (mbMatrixFlag)
        return true;
    formula::ParamClass paramClass = token->GetInForceArray();
    if (paramClass == formula::ForceArray
        || paramClass == formula::ReferenceOrForceArray
        || paramClass == formula::SuppressedReferenceOrForceArray
        || paramClass == formula::ReferenceOrRefArray)
    {
        return true;
    }
    formula::ParamClass returnType = ScParameterClassification::GetParameterType( token, SAL_MAX_UINT16 );
    return returnType == formula::Reference;
}

void ScCompiler::HandleIIOpCode(FormulaToken* token, FormulaToken*** pppToken, sal_uInt8 nNumParams)
{
    if (!mbComputeII)
        return;
#ifdef DBG_UTIL
    if(!HandleIIOpCodeInternal(token, pppToken, nNumParams))
        mUnhandledPossibleImplicitIntersectionsOpCodes.insert(token->GetOpCode());
#else
    HandleIIOpCodeInternal(token, pppToken, nNumParams);
#endif
}

// return true if opcode is handled
bool ScCompiler::HandleIIOpCodeInternal(FormulaToken* token, FormulaToken*** pppToken, sal_uInt8 nNumParams)
{
    if (nNumParams > 0 && *pppToken[0] == nullptr)
        return false; // Bad expression (see the dummy creation in FormulaCompiler::CompileTokenArray())

    const OpCode nOpCode = token->GetOpCode();

    if (nOpCode == ocPush)
    {
        if(token->GetType() == svDoubleRef)
            mUnhandledPossibleImplicitIntersections.insert( token );
        return true;
    }
    else if (nOpCode == ocSumIf || nOpCode == ocAverageIf)
    {
        if (nNumParams != 3)
            return false;

        if (!(pppToken[0] && pppToken[2] && *pppToken[0] && *pppToken[2]))
            return false;

        if ((*pppToken[0])->GetType() != svDoubleRef)
            return false;

        const StackVar eSumRangeType = (*pppToken[2])->GetType();

        if ( eSumRangeType != svSingleRef && eSumRangeType != svDoubleRef )
            return false;

        const ScComplexRefData& rBaseRange = *(*pppToken[0])->GetDoubleRef();

        ScComplexRefData aSumRange;
        if (eSumRangeType == svSingleRef)
        {
            aSumRange.Ref1 = *(*pppToken[2])->GetSingleRef();
            aSumRange.Ref2 = aSumRange.Ref1;
        }
        else
            aSumRange = *(*pppToken[2])->GetDoubleRef();

        CorrectSumRange(rBaseRange, aSumRange, pppToken[2]);
        // TODO mark parameters as handled
        return true;
    }
    else if (nOpCode >= SC_OPCODE_START_1_PAR && nOpCode < SC_OPCODE_STOP_1_PAR)
    {
        if (nNumParams != 1)
            return false;

        if( !ParameterMayBeImplicitIntersection( token, 0 ))
            return false;
        if (SkipImplicitIntersectionOptimization(token))
            return false;

        if ((*pppToken[0])->GetType() != svDoubleRef)
            return false;

        mPendingImplicitIntersectionOptimizations.emplace_back( pppToken[0], token );
        return true;
    }
    else if ((nOpCode >= SC_OPCODE_START_BIN_OP && nOpCode < SC_OPCODE_STOP_BIN_OP
                && nOpCode != ocAnd && nOpCode != ocOr)
              || nOpCode == ocRound || nOpCode == ocRoundUp || nOpCode == ocRoundDown)
    {
        if (nNumParams != 2)
            return false;

        if( !ParameterMayBeImplicitIntersection( token, 0 ) || !ParameterMayBeImplicitIntersection( token, 1 ))
            return false;
        if (SkipImplicitIntersectionOptimization(token))
            return false;

        // Convert only if the other parameter is not a matrix (which would force the result to be a matrix).
        if ((*pppToken[0])->GetType() == svDoubleRef && (*pppToken[1])->GetType() != svMatrix)
            mPendingImplicitIntersectionOptimizations.emplace_back( pppToken[0], token );
        if ((*pppToken[1])->GetType() == svDoubleRef && (*pppToken[0])->GetType() != svMatrix)
            mPendingImplicitIntersectionOptimizations.emplace_back( pppToken[1], token );
        return true;
    }
    else if ((nOpCode >= SC_OPCODE_START_UN_OP && nOpCode < SC_OPCODE_STOP_UN_OP)
              || nOpCode == ocPercentSign)
    {
        if (nNumParams != 1)
            return false;

        if( !ParameterMayBeImplicitIntersection( token, 0 ))
            return false;
        if (SkipImplicitIntersectionOptimization(token))
            return false;

        if ((*pppToken[0])->GetType() == svDoubleRef)
            mPendingImplicitIntersectionOptimizations.emplace_back( pppToken[0], token );
        return true;
    }
    else if (nOpCode == ocVLookup)
    {
        if (nNumParams != 3 && nNumParams != 4)
            return false;

        if (SkipImplicitIntersectionOptimization(token))
            return false;

        assert( ParameterMayBeImplicitIntersection( token, 0 ));
        assert( !ParameterMayBeImplicitIntersection( token, 1 ));
        assert( ParameterMayBeImplicitIntersection( token, 2 ));
        assert( ParameterMayBeImplicitIntersection( token, 3 ));
        if ((*pppToken[2])->GetType() == svDoubleRef)
            mPendingImplicitIntersectionOptimizations.emplace_back( pppToken[2], token );
        if ((*pppToken[0])->GetType() == svDoubleRef)
            mPendingImplicitIntersectionOptimizations.emplace_back( pppToken[0], token );
        if (nNumParams == 4 && (*pppToken[3])->GetType() == svDoubleRef)
            mPendingImplicitIntersectionOptimizations.emplace_back( pppToken[3], token );
        // a range for the second parameters is not an implicit intersection
        mUnhandledPossibleImplicitIntersections.erase( *pppToken[ 1 ] );
        return true;
    }
    else
    {
        bool possibleII = false;
        for( int i = 0; i < nNumParams; ++i )
        {
            if( ParameterMayBeImplicitIntersection( token, i )
                && (*pppToken[i])->GetType() == svDoubleRef)
            {
                possibleII = true;
                break;
            }
        }
        if( !possibleII )
        {
            // all parameters have been handled, they are not implicit intersections
            for( int i = 0; i < nNumParams; ++i )
                mUnhandledPossibleImplicitIntersections.erase( *pppToken[ i ] );
            return true;
        }
    }

    return false;
}

void ScCompiler::PostProcessCode()
{
    for( const PendingImplicitIntersectionOptimization& item : mPendingImplicitIntersectionOptimizations )
    {
        if( *item.parameterLocation != item.parameter ) // the parameter has been changed somehow
            continue;
        if( item.parameterLocation >= pCode ) // the location is not inside the code (pCode points after the end)
            continue;
        // E.g. "SUMPRODUCT(I5:I6+1)" shouldn't do implicit intersection.
        if( item.operation->IsInForceArray())
            continue;
        ReplaceDoubleRefII( item.parameterLocation );
    }
    mPendingImplicitIntersectionOptimizations.clear();
}

void ScCompiler::AnnotateOperands()
{
    AnnotateTrimOnDoubleRefs();
}

void ScCompiler::ReplaceDoubleRefII(FormulaToken** ppDoubleRefTok)
{
    const ScComplexRefData* pRange = (*ppDoubleRefTok)->GetDoubleRef();
    if (!pRange)
        return;

    const ScComplexRefData& rRange = *pRange;

    // Can't do optimization reliably in this case (when row references are absolute).
    // Example : =SIN(A$1:A$10) filled in a formula group starting at B5 and of length 100.
    // If we just optimize the argument $A$1:$A$10 to singleref "A5" for the top cell in the fg, then
    // the results in cells B11:B104 will be incorrect (sin(0) = 0, assuming empty cells in A11:A104)
    // instead of the #VALUE! errors we would expect. We need to know the formula-group length to
    // fix this, but that is unknown at this stage, so skip such cases.
    if (!rRange.Ref1.IsRowRel() && !rRange.Ref2.IsRowRel())
        return;

    ScRange aAbsRange = rRange.toAbs(rDoc, aPos);
    if (aAbsRange.aStart == aAbsRange.aEnd)
        return; // Nothing to do (trivial case).

    ScAddress aAddr;

    if (!DoubleRefToPosSingleRefScalarCase(aAbsRange, aAddr, aPos))
        return;

    ScSingleRefData aSingleRef;
    aSingleRef.InitFlags();
    aSingleRef.SetColRel(rRange.Ref1.IsColRel());
    aSingleRef.SetRowRel(true);
    aSingleRef.SetTabRel(rRange.Ref1.IsTabRel());
    aSingleRef.SetAddress(rDoc.GetSheetLimits(), aAddr, aPos);

    // Replace the original doubleref token with computed singleref token
    FormulaToken* pNewSingleRefTok = new ScSingleRefToken(rDoc.GetSheetLimits(), aSingleRef);
    (*ppDoubleRefTok)->DecRef();
    *ppDoubleRefTok = pNewSingleRefTok;
    pNewSingleRefTok->IncRef();
}

bool ScCompiler::DoubleRefToPosSingleRefScalarCase(const ScRange& rRange, ScAddress& rAdr, const ScAddress& rFormulaPos)
{
    assert(rRange.aStart != rRange.aEnd);

    bool bOk = false;
    SCCOL nMyCol = rFormulaPos.Col();
    SCROW nMyRow = rFormulaPos.Row();
    SCTAB nMyTab = rFormulaPos.Tab();
    SCCOL nCol = 0;
    SCROW nRow = 0;
    SCTAB nTab;
    nTab = rRange.aStart.Tab();
    if ( rRange.aStart.Col() <= nMyCol && nMyCol <= rRange.aEnd.Col() )
    {
        nRow = rRange.aStart.Row();
        if ( nRow == rRange.aEnd.Row() )
        {
            bOk = true;
            nCol = nMyCol;
        }
        else if ( nTab != nMyTab && nTab == rRange.aEnd.Tab()
                && rRange.aStart.Row() <= nMyRow && nMyRow <= rRange.aEnd.Row() )
        {
            bOk = true;
            nCol = nMyCol;
            nRow = nMyRow;
        }
    }
    else if ( rRange.aStart.Row() <= nMyRow && nMyRow <= rRange.aEnd.Row() )
    {
        nCol = rRange.aStart.Col();
        if ( nCol == rRange.aEnd.Col() )
        {
            bOk = true;
            nRow = nMyRow;
        }
        else if ( nTab != nMyTab && nTab == rRange.aEnd.Tab()
                && rRange.aStart.Col() <= nMyCol && nMyCol <= rRange.aEnd.Col() )
        {
            bOk = true;
            nCol = nMyCol;
            nRow = nMyRow;
        }
    }
    if ( bOk )
    {
        if ( nTab == rRange.aEnd.Tab() )
            ;   // all done
        else if ( nTab <= nMyTab && nMyTab <= rRange.aEnd.Tab() )
            nTab = nMyTab;
        else
            bOk = false;
        if ( bOk )
            rAdr.Set( nCol, nRow, nTab );
    }

    return bOk;
}

static void lcl_GetColRowDeltas(const ScRange& rRange, SCCOL& rXDelta, SCROW& rYDelta)
{
    rXDelta = rRange.aEnd.Col() - rRange.aStart.Col();
    rYDelta = rRange.aEnd.Row() - rRange.aStart.Row();
}

bool ScCompiler::AdjustSumRangeShape(const ScComplexRefData& rBaseRange, ScComplexRefData& rSumRange)
{
    ScRange aAbs = rSumRange.toAbs(rDoc, aPos);

    // Current sum-range end col/row
    SCCOL nEndCol = aAbs.aEnd.Col();
    SCROW nEndRow = aAbs.aEnd.Row();

    // Current behaviour is, we will get a #NAME? for the below case, so bail out.
    // Note that sum-range's End[Col,Row] are same as Start[Col,Row] if the original formula
    // has a single-ref as the sum-range.
    if (!rDoc.ValidCol(nEndCol) || !rDoc.ValidRow(nEndRow))
        return false;

    SCCOL nXDeltaSum = 0;
    SCROW nYDeltaSum = 0;

    lcl_GetColRowDeltas(aAbs, nXDeltaSum, nYDeltaSum);

    aAbs = rBaseRange.toAbs(rDoc, aPos);
    SCCOL nXDelta = 0;
    SCROW nYDelta = 0;

    lcl_GetColRowDeltas(aAbs, nXDelta, nYDelta);

    if (nXDelta == nXDeltaSum &&
        nYDelta == nYDeltaSum)
        return false;  // shapes of base-range match current sum-range

    // Try to make the sum-range to take the same shape as base-range,
    // by adjusting Ref2 member of rSumRange if the resultant sum-range don't
    // go out-of-bounds.

    SCCOL nXInc = nXDelta - nXDeltaSum;
    SCROW nYInc = nYDelta - nYDeltaSum;

    // Don't let a valid End[Col,Row] go beyond (rDoc.MaxCol(),rDoc.MaxRow()) to match
    // what happens in ScInterpreter::IterateParametersIf(), but there it also shrinks
    // the base-range by the (out-of-bound)amount clipped off the sum-range.
    // TODO: Probably we can optimize (from threading perspective) rBaseRange
    //       by shrinking it here correspondingly (?)
    if (nEndCol + nXInc > rDoc.MaxCol())
        nXInc = rDoc.MaxCol() - nEndCol;
    if (nEndRow + nYInc > rDoc.MaxRow())
        nYInc = rDoc.MaxRow() - nEndRow;

    rSumRange.Ref2.IncCol(nXInc);
    rSumRange.Ref2.IncRow(nYInc);

    return true;
}

void ScCompiler::CorrectSumRange(const ScComplexRefData& rBaseRange,
                                 ScComplexRefData& rSumRange,
                                 FormulaToken** ppSumRangeToken)
{
    if (!AdjustSumRangeShape(rBaseRange, rSumRange))
        return;

    // Replace sum-range token
    FormulaToken* pNewSumRangeTok = new ScDoubleRefToken(rDoc.GetSheetLimits(), rSumRange);
    (*ppSumRangeToken)->DecRef();
    *ppSumRangeToken = pNewSumRangeTok;
    pNewSumRangeTok->IncRef();
}

void ScCompiler::AnnotateTrimOnDoubleRefs()
{
    if (!pCode || !(*(pCode - 1)))
        return;

    // OpCode of the "root" operator (which is already in RPN array).
    OpCode eOpCode = (*(pCode - 1))->GetOpCode();
    // Param number of the "root" operator (which is already in RPN array).
    sal_uInt8 nRootParam = (*(pCode - 1))->GetByte();
    // eOpCode can be some operator which does not change with operands with or contains zero values.
    if (eOpCode == ocSum)
    {
        FormulaToken** ppTok = pCode - 2; // exclude the root operator.
        // The following loop runs till a "pattern" is found or there is a mismatch
        // and marks the push DoubleRef arguments as trimmable when there is a match.
        // The pattern is
        // SUM(IF(<reference|double>=<reference|double>, <then-clause>)<a some operands with operators / or *>)
        // such that one of the operands of ocEqual is a double-ref.
        // Examples of formula that matches this are:
        //   SUM(IF(D:D=$A$1,F:F)*$H$1*2.3/$G$2)
        //   SUM((IF(D:D=$A$1,F:F)*$H$1*2.3/$G$2)*$H$2*5/$G$3)
        //   SUM(IF(E:E=16,F:F)*$H$1*100)
        bool bTillClose = true;
        bool bCloseTillIf = false;
        sal_Int16 nToksTillIf = 0;
        constexpr sal_Int16 MAXDIST_IF = 15;
        while (*ppTok)
        {
            FormulaToken* pTok = *ppTok;
            OpCode eCurrOp = pTok->GetOpCode();
            ++nToksTillIf;

            // TODO : Is there a better way to handle this ?
            // ocIf is too far off from the sum opcode.
            if (nToksTillIf > MAXDIST_IF)
                return;

            switch (eCurrOp)
            {
                case ocDiv:
                case ocMul:
                    if (!bTillClose)
                        return;
                    break;
                case ocPush:

                    break;
                case ocClose:
                    if (bTillClose)
                    {
                        bTillClose = false;
                        bCloseTillIf = true;
                    }
                    else
                        return;
                    break;
                case ocIf:
                    {
                        if (!bCloseTillIf)
                            return;

                        if (!pTok->IsInForceArray())
                            return;

                        const short nJumpCount = pTok->GetJump()[0];
                        if (nJumpCount != 2) // Should have THEN but no ELSE.
                            return;

                        OpCode eCompOp = (*(ppTok - 1))->GetOpCode();
                        if (eCompOp != ocEqual)
                            return;

                        FormulaToken* pLHS = *(ppTok - 2);
                        FormulaToken* pRHS = *(ppTok - 3);
                        if (((pLHS->GetType() == svSingleRef || pLHS->GetType() == svDouble) && pRHS->GetType() == svDoubleRef) ||
                            ((pRHS->GetType() == svSingleRef || pRHS->GetType() == svDouble) && pLHS->GetType() == svDoubleRef))
                        {
                            if (pLHS->GetType() == svDoubleRef)
                                pLHS->GetDoubleRef()->SetTrimToData(true);
                            else
                                pRHS->GetDoubleRef()->SetTrimToData(true);
                            return;
                        }
                    }
                    break;
                default:
                    return;
            }
            --ppTok;
        }
    }
    else if (eOpCode == ocSumProduct)
    {
        FormulaToken** ppTok = pCode - 2; // exclude the root operator.
        // The following loop runs till a "pattern" is found or there is a mismatch
        // and marks the push DoubleRef arguments as trimmable when there is a match.
        // The pattern is
        // SUMPRODUCT(IF(<reference|double>=<reference|double>, <then-clause>)<a some operands with operators / or *>)
        // such that one of the operands of ocEqual is a double-ref.
        // Examples of formula that matches this are:
        //   SUMPRODUCT(IF($A:$A=$L12;$D:$D*G:G))
        // Also in case of DoubleRef arguments around other Binary operators can be trimmable inside one parameter
        // of the root operator:
        //   SUMPRODUCT(($D:$D>M47:M47)*($D:$D<M48:M48)*($I:$I=N$41))
        bool bTillClose = true;
        bool bCloseTillIf = false;
        sal_Int16 nToksTillIf = 0;
        constexpr sal_Int16 MAXDIST_IF = 15;
        while (*ppTok)
        {
            FormulaToken* pTok = *ppTok;
            OpCode eCurrOp = pTok->GetOpCode();
            ++nToksTillIf;

            // TODO : Is there a better way to handle this ?
            // ocIf is too far off from the sum opcode.
            if (nToksTillIf > MAXDIST_IF)
                return;

            switch (eCurrOp)
            {
                case ocDiv:
                case ocMul:
                    {
                        if (!pTok->IsInForceArray())
                            break;
                        FormulaToken* pLHS = *(ppTok - 1);
                        FormulaToken* pRHS = *(ppTok - 2);
                        if (pLHS && pRHS)
                        {
                            StackVar lhsType = pLHS->GetType();
                            StackVar rhsType = pRHS->GetType();
                            if (lhsType == svDoubleRef && rhsType == svDoubleRef)
                            {
                                pLHS->GetDoubleRef()->SetTrimToData(true);
                                pRHS->GetDoubleRef()->SetTrimToData(true);
                            }
                        }
                    }
                    break;
                case ocEqual:
                case ocAdd:
                case ocSub:
                case ocAmpersand:
                case ocPow:
                case ocNotEqual:
                case ocLess:
                case ocGreater:
                case ocLessEqual:
                case ocGreaterEqual:
                case ocAnd:
                case ocOr:
                case ocXor:
                case ocIntersect:
                    {
                        // tdf#160616: Double refs with these operators only
                        // trimmable in case of one parameter
                        if (!pTok->IsInForceArray() || nRootParam > 1)
                            break;
                        FormulaToken* pLHS = *(ppTok - 1);
                        FormulaToken* pRHS = *(ppTok - 2);
                        if (pLHS && pRHS)
                        {
                            StackVar lhsType = pLHS->GetType();
                            StackVar rhsType = pRHS->GetType();
                            if (lhsType == svDoubleRef && (rhsType == svSingleRef || rhsType == svDoubleRef))
                            {
                                pLHS->GetDoubleRef()->SetTrimToData(true);
                            }
                            if (rhsType == svDoubleRef && (lhsType == svSingleRef || lhsType == svDoubleRef))
                            {
                                pRHS->GetDoubleRef()->SetTrimToData(true);
                            }
                        }
                    }
                    break;
                case ocPush:
                    break;
                case ocClose:
                    if (bTillClose)
                    {
                        bTillClose = false;
                        bCloseTillIf = true;
                    }
                    else
                        return;
                    break;
                case ocIf:
                    {
                        if (!bCloseTillIf)
                            return;

                        if (!pTok->IsInForceArray())
                            return;

                        const short nJumpCount = pTok->GetJump()[0];
                        if (nJumpCount != 2) // Should have THEN but no ELSE.
                            return;

                        OpCode eCompOp = (*(ppTok - 1))->GetOpCode();
                        if (eCompOp != ocEqual)
                            return;

                        FormulaToken* pLHS = *(ppTok - 2);
                        FormulaToken* pRHS = *(ppTok - 3);
                        StackVar lhsType = pLHS->GetType();
                        StackVar rhsType = pRHS->GetType();
                        if (lhsType == svDoubleRef && (rhsType == svSingleRef || rhsType == svDouble))
                        {
                            pLHS->GetDoubleRef()->SetTrimToData(true);
                        }
                        if ((lhsType == svSingleRef || lhsType == svDouble) && rhsType == svDoubleRef)
                        {
                            pRHS->GetDoubleRef()->SetTrimToData(true);
                        }
                        return;
                    }
                    break;
                default:
                    return;
            }
            --ppTok;
        }
    }
    else if (eOpCode == ocSubTotal)
    {
        // tdf#164843: Double references from relative named ranges can point to large
        // ranges (MAXCOL/MAXROW) and because of that some function evaluation
        // like SubTotal can be extremely slow when we call ScTable::CompileHybridFormula
        // with these large ranges. Since all the SubTotal functions ignore empty cells
        // its worth to optimize and trim the double references in SubTotal functions.
        FormulaToken** ppTok = pCode - 2;
        while (*ppTok)
        {
            FormulaToken* pTok = *ppTok;
            if (pTok->GetType() == svDoubleRef)
            {
                ScComplexRefData* pRefData = pTok->GetDoubleRef();
                // no need to set pRefData->SetTrimToData(true); because we already trim here
                ScRange rRange = pRefData->toAbs(rDoc, aPos);
                SCCOL nTempStartCol = rRange.aStart.Col();
                SCROW nTempStartRow = rRange.aStart.Row();
                SCCOL nTempEndCol = rRange.aEnd.Col();
                SCROW nTempEndRow = rRange.aEnd.Row();
                rDoc.ShrinkToDataArea(rRange.aStart.Tab(), nTempStartCol, nTempStartRow, nTempEndCol, nTempEndRow);
                rRange.aStart.Set(nTempStartCol, nTempStartRow, rRange.aStart.Tab());
                rRange.aEnd.Set(nTempEndCol, nTempEndRow, rRange.aEnd.Tab());
                rRange.PutInOrder();
                pRefData->SetRange(rDoc.GetSheetLimits(), rRange, aPos);
            }
            --ppTok;
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
