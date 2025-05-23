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

#include <svl/itemset.hxx>
#include <svl/intitem.hxx>
#include <svl/eitem.hxx>
#include <svl/languageoptions.hxx>
#include <comphelper/configuration.hxx>
#include <utility>
#include <vcl/outdev.hxx>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>
#include <sal/log.hxx>
#include <osl/diagnose.h>
#include <i18nlangtag/languagetag.hxx>

#include <com/sun/star/beans/PropertyValue.hpp>

#include <cfgitem.hxx>

#include <starmath.hrc>
#include <smmod.hxx>
#include <symbol.hxx>
#include <format.hxx>

using namespace com::sun::star::uno;
using namespace com::sun::star::beans;

constexpr OUString SYMBOL_LIST = u"SymbolList"_ustr;
constexpr OUString FONT_FORMAT_LIST = u"FontFormatList"_ustr;
constexpr OUString USER_DEFINED_LIST = u"User-Defined"_ustr;

static Sequence< OUString > lcl_GetFontPropertyNames()
{
    return Sequence< OUString > {
                        u"Name"_ustr,
                        u"CharSet"_ustr,
                        u"Family"_ustr,
                        u"Pitch"_ustr,
                        u"Weight"_ustr,
                        u"Italic"_ustr
                    };
}

static Sequence< OUString > lcl_GetSymbolPropertyNames()
{
    return Sequence< OUString > {
                        u"Char"_ustr,
                        u"Set"_ustr,
                        u"Predefined"_ustr,
                        u"FontFormatId"_ustr
                    };
}

static Sequence<OUString> lcl_GetOtherPropertyNames()
{
    return Sequence<OUString>{ u"LoadSave/IsSaveOnlyUsedSymbols"_ustr,
                               u"Misc/AutoCloseBrackets"_ustr,
                               u"Misc/DefaultSmSyntaxVersion"_ustr,
                               u"Misc/InlineEditEnable"_ustr,
                               u"Misc/IgnoreSpacesRight"_ustr,
                               u"Misc/SmEditWindowZoomFactor"_ustr,
                               u"Print/FormulaText"_ustr,
                               u"Print/Frame"_ustr,
                               u"Print/Size"_ustr,
                               u"Print/Title"_ustr,
                               u"Print/ZoomFactor"_ustr,
                               u"View/AutoRedraw"_ustr,
                               u"View/FormulaCursor"_ustr,
                               u"View/ToolboxVisible"_ustr };
}

static Sequence< OUString > lcl_GetFormatPropertyNames()
{
    //! Beware of order according to *_BEGIN *_END defines in format.hxx !
    //! see respective load/save routines here
    return Sequence< OUString > {
                        u"StandardFormat/Textmode"_ustr,
                        u"StandardFormat/RightToLeft"_ustr,
                        u"StandardFormat/GreekCharStyle"_ustr,
                        u"StandardFormat/ScaleNormalBracket"_ustr,
                        u"StandardFormat/HorizontalAlignment"_ustr,
                        u"StandardFormat/BaseSize"_ustr,
                        u"StandardFormat/TextSize"_ustr,
                        u"StandardFormat/IndexSize"_ustr,
                        u"StandardFormat/FunctionSize"_ustr,
                        u"StandardFormat/OperatorSize"_ustr,
                        u"StandardFormat/LimitsSize"_ustr,
                        u"StandardFormat/Distance/Horizontal"_ustr,
                        u"StandardFormat/Distance/Vertical"_ustr,
                        u"StandardFormat/Distance/Root"_ustr,
                        u"StandardFormat/Distance/SuperScript"_ustr,
                        u"StandardFormat/Distance/SubScript"_ustr,
                        u"StandardFormat/Distance/Numerator"_ustr,
                        u"StandardFormat/Distance/Denominator"_ustr,
                        u"StandardFormat/Distance/Fraction"_ustr,
                        u"StandardFormat/Distance/StrokeWidth"_ustr,
                        u"StandardFormat/Distance/UpperLimit"_ustr,
                        u"StandardFormat/Distance/LowerLimit"_ustr,
                        u"StandardFormat/Distance/BracketSize"_ustr,
                        u"StandardFormat/Distance/BracketSpace"_ustr,
                        u"StandardFormat/Distance/MatrixRow"_ustr,
                        u"StandardFormat/Distance/MatrixColumn"_ustr,
                        u"StandardFormat/Distance/OrnamentSize"_ustr,
                        u"StandardFormat/Distance/OrnamentSpace"_ustr,
                        u"StandardFormat/Distance/OperatorSize"_ustr,
                        u"StandardFormat/Distance/OperatorSpace"_ustr,
                        u"StandardFormat/Distance/LeftSpace"_ustr,
                        u"StandardFormat/Distance/RightSpace"_ustr,
                        u"StandardFormat/Distance/TopSpace"_ustr,
                        u"StandardFormat/Distance/BottomSpace"_ustr,
                        u"StandardFormat/Distance/NormalBracketSize"_ustr,
                        u"StandardFormat/VariableFont"_ustr,
                        u"StandardFormat/FunctionFont"_ustr,
                        u"StandardFormat/NumberFont"_ustr,
                        u"StandardFormat/TextFont"_ustr,
                        u"StandardFormat/SerifFont"_ustr,
                        u"StandardFormat/SansFont"_ustr,
                        u"StandardFormat/FixedFont"_ustr
                    };
}

struct SmCfgOther
{
    SmPrintSize     ePrintSize;
    sal_uInt16      nPrintZoomFactor;
    sal_uInt16      nSmEditWindowZoomFactor;
    sal_Int16       nSmSyntaxVersion;
    bool            bPrintTitle;
    bool            bPrintFormulaText;
    bool            bPrintFrame;
    bool            bIsSaveOnlyUsedSymbols;
    bool            bIsAutoCloseBrackets;
    bool            bInlineEditEnable;
    bool            bIgnoreSpacesRight;
    bool            bToolboxVisible;
    bool            bAutoRedraw;
    bool            bFormulaCursor;

    SmCfgOther();
};

constexpr sal_Int16 nDefaultSmSyntaxVersion(5);

SmCfgOther::SmCfgOther()
    : ePrintSize(PRINT_SIZE_NORMAL)
    , nPrintZoomFactor(100)
    , nSmEditWindowZoomFactor(100)
    // Defaulted as 5 so I have time to code the parser 6
    , nSmSyntaxVersion(nDefaultSmSyntaxVersion)
    , bPrintTitle(true)
    , bPrintFormulaText(true)
    , bPrintFrame(true)
    , bIsSaveOnlyUsedSymbols(true)
    , bIsAutoCloseBrackets(true)
    , bInlineEditEnable(false)
    , bIgnoreSpacesRight(true)
    , bToolboxVisible(true)
    , bAutoRedraw(true)
    , bFormulaCursor(true)
{
}


SmFontFormat::SmFontFormat()
    : aName(FONTNAME_MATH)
    , nCharSet(RTL_TEXTENCODING_UNICODE)
    , nFamily(FAMILY_DONTKNOW)
    , nPitch(PITCH_DONTKNOW)
    , nWeight(WEIGHT_DONTKNOW)
    , nItalic(ITALIC_NONE)
{
}


SmFontFormat::SmFontFormat( const vcl::Font &rFont )
    : aName(rFont.GetFamilyName())
    , nCharSet(static_cast<sal_Int16>(rFont.GetCharSet()))
    , nFamily(static_cast<sal_Int16>(rFont.GetFamilyType()))
    , nPitch(static_cast<sal_Int16>(rFont.GetPitch()))
    , nWeight(static_cast<sal_Int16>(rFont.GetWeight()))
    , nItalic(static_cast<sal_Int16>(rFont.GetItalic()))
{
}


vcl::Font SmFontFormat::GetFont() const
{
    vcl::Font aRes;
    aRes.SetFamilyName( aName );
    aRes.SetCharSet( static_cast<rtl_TextEncoding>(nCharSet) );
    aRes.SetFamily( static_cast<FontFamily>(nFamily) );
    aRes.SetPitch( static_cast<FontPitch>(nPitch) );
    aRes.SetWeight( static_cast<FontWeight>(nWeight) );
    aRes.SetItalic( static_cast<FontItalic>(nItalic) );
    return aRes;
}


bool SmFontFormat::operator == ( const SmFontFormat &rFntFmt ) const
{
    return  aName    == rFntFmt.aName       &&
            nCharSet == rFntFmt.nCharSet    &&
            nFamily  == rFntFmt.nFamily     &&
            nPitch   == rFntFmt.nPitch      &&
            nWeight  == rFntFmt.nWeight     &&
            nItalic  == rFntFmt.nItalic;
}


SmFntFmtListEntry::SmFntFmtListEntry( OUString _aId, SmFontFormat _aFntFmt ) :
    aId     (std::move(_aId)),
    aFntFmt (std::move(_aFntFmt))
{
}


SmFontFormatList::SmFontFormatList()
    : bModified(false)
{
}


void SmFontFormatList::Clear()
{
    if (!aEntries.empty())
    {
        aEntries.clear();
        SetModified( true );
    }
}


void SmFontFormatList::AddFontFormat( const OUString &rFntFmtId,
        const SmFontFormat &rFntFmt )
{
    const SmFontFormat *pFntFmt = GetFontFormat( rFntFmtId );
    OSL_ENSURE( !pFntFmt, "FontFormatId already exists" );
    if (!pFntFmt)
    {
        SmFntFmtListEntry aEntry( rFntFmtId, rFntFmt );
        aEntries.push_back( aEntry );
        SetModified( true );
    }
}


void SmFontFormatList::RemoveFontFormat( std::u16string_view rFntFmtId )
{

    // search for entry
    for (size_t i = 0;  i < aEntries.size();  ++i)
    {
        if (aEntries[i].aId == rFntFmtId)
        {
            // remove entry if found
            aEntries.erase( aEntries.begin() + i );
            SetModified( true );
            break;
        }
    }
}


const SmFontFormat * SmFontFormatList::GetFontFormat( std::u16string_view rFntFmtId ) const
{
    const SmFontFormat *pRes = nullptr;

    for (const auto & rEntry : aEntries)
    {
        if (rEntry.aId == rFntFmtId)
        {
            pRes = &rEntry.aFntFmt;
            break;
        }
    }

    return pRes;
}


const SmFontFormat * SmFontFormatList::GetFontFormat( size_t nPos ) const
{
    const SmFontFormat *pRes = nullptr;
    if (nPos < aEntries.size())
        pRes = &aEntries[nPos].aFntFmt;
    return pRes;
}


OUString SmFontFormatList::GetFontFormatId( const SmFontFormat &rFntFmt ) const
{
    OUString aRes;

    for (const auto & rEntry : aEntries)
    {
        if (rEntry.aFntFmt == rFntFmt)
        {
            aRes = rEntry.aId;
            break;
        }
    }

    return aRes;
}


OUString SmFontFormatList::GetFontFormatId( const SmFontFormat &rFntFmt, bool bAdd )
{
    OUString aRes( GetFontFormatId( rFntFmt) );
    if (aRes.isEmpty()  &&  bAdd)
    {
        aRes = GetNewFontFormatId();
        AddFontFormat( aRes, rFntFmt );
    }
    return aRes;
}


OUString SmFontFormatList::GetFontFormatId( size_t nPos ) const
{
    OUString aRes;
    if (nPos < aEntries.size())
        aRes = aEntries[nPos].aId;
    return aRes;
}


OUString SmFontFormatList::GetNewFontFormatId() const
{
    // returns first unused FormatId

    sal_Int32 nCnt = GetCount();
    for (sal_Int32 i = 1;  i <= nCnt + 1;  ++i)
    {
        OUString aTmpId = "Id" + OUString::number(i);
        if (!GetFontFormat(aTmpId))
            return aTmpId;
    }
    OSL_ENSURE( false, "failed to create new FontFormatId" );

    return OUString();
}


SmMathConfig::SmMathConfig() :
    ConfigItem(u"Office.Math"_ustr)
    , bIsOtherModified(false)
    , bIsFormatModified(false)
{
    EnableNotification({ {} }); // Listen to everything under the node
}


SmMathConfig::~SmMathConfig()
{
    Save();
}


void SmMathConfig::SetOtherModified( bool bVal )
{
    bIsOtherModified = bVal;
}


void SmMathConfig::SetFormatModified( bool bVal )
{
    bIsFormatModified = bVal;
}


void SmMathConfig::ReadSymbol( SmSym &rSymbol,
                        const OUString &rSymbolName,
                        std::u16string_view rBaseNode ) const
{
    Sequence< OUString > aNames = lcl_GetSymbolPropertyNames();
    sal_Int32 nProps = aNames.getLength();

    OUString aDelim( u"/"_ustr );
    for (auto& rName : asNonConstRange(aNames))
        rName = rBaseNode + aDelim + rSymbolName + aDelim + rName;

    const Sequence< Any > aValues = const_cast<SmMathConfig*>(this)->GetProperties(aNames);

    if (!(nProps  &&  aValues.getLength() == nProps))
        return;

    const Any * pValue = aValues.getConstArray();
    vcl::Font   aFont;
    sal_UCS4    cChar = '\0';
    OUString    aSet;
    bool        bPredefined = false;

    OUString    aTmpStr;
    sal_Int32       nTmp32 = 0;
    bool        bTmp = false;

    bool bOK = true;
    if (pValue->hasValue()  &&  (*pValue >>= nTmp32))
        cChar = static_cast< sal_UCS4 >( nTmp32 );
    else
        bOK = false;
    ++pValue;
    if (pValue->hasValue()  &&  (*pValue >>= aTmpStr))
        aSet = aTmpStr;
    else
        bOK = false;
    ++pValue;
    if (pValue->hasValue()  &&  (*pValue >>= bTmp))
        bPredefined = bTmp;
    else
        bOK = false;
    ++pValue;
    if (pValue->hasValue()  &&  (*pValue >>= aTmpStr))
    {
        const SmFontFormat *pFntFmt = GetFontFormatList().GetFontFormat( aTmpStr );
        OSL_ENSURE( pFntFmt, "unknown FontFormat" );
        if (pFntFmt)
            aFont = pFntFmt->GetFont();
    }
    ++pValue;

    if (bOK)
    {
        OUString aUiName( rSymbolName );
        OUString aUiSetName( aSet );
        if (bPredefined)
        {
            OUString aTmp;
            aTmp = SmLocalizedSymbolData::GetUiSymbolName( rSymbolName );
            OSL_ENSURE( !aTmp.isEmpty(), "localized symbol-name not found" );
            if (!aTmp.isEmpty())
                aUiName = aTmp;
            aTmp = SmLocalizedSymbolData::GetUiSymbolSetName( aSet );
            OSL_ENSURE( !aTmp.isEmpty(), "localized symbolset-name not found" );
            if (!aTmp.isEmpty())
                aUiSetName = aTmp;
        }

        rSymbol = SmSym( aUiName, aFont, cChar, aUiSetName, bPredefined );
        if (aUiName != rSymbolName)
            rSymbol.SetExportName( rSymbolName );
    }
    else
    {
        SAL_WARN("starmath", "symbol read error");
    }
}


SmSymbolManager & SmMathConfig::GetSymbolManager()
{
    if (!pSymbolMgr)
    {
        pSymbolMgr.reset(new SmSymbolManager);
        pSymbolMgr->Load();
    }
    return *pSymbolMgr;
}


void SmMathConfig::ImplCommit()
{
    Save();
}


void SmMathConfig::Save()
{
    SaveOther();
    SaveFormat();
    SaveFontFormatList();
}


void SmMathConfig::UnlockCommit()
{
    if (--m_nCommitLock == 0)
        Commit();
}


void SmMathConfig::Clear()
{
    // Re-read data on next request
    pOther.reset();
    pFormat.reset();
    pFontFormatList.reset();
}


void SmMathConfig::GetSymbols( std::vector< SmSym > &rSymbols ) const
{
    Sequence< OUString > aNodes(const_cast<SmMathConfig*>(this)->GetNodeNames(SYMBOL_LIST));
    const OUString *pNode = aNodes.getConstArray();
    sal_Int32 nNodes = aNodes.getLength();

    rSymbols.resize( nNodes );
    for (auto& rSymbol : rSymbols)
    {
        ReadSymbol( rSymbol, *pNode++, SYMBOL_LIST );
    }
}


void SmMathConfig::SetSymbols( const std::vector< SmSym > &rNewSymbols )
{
    CommitLocker aLock(*this);
    auto nCount = sal::static_int_cast<sal_Int32>(rNewSymbols.size());

    Sequence< OUString > aNames = lcl_GetSymbolPropertyNames();
    const OUString *pNames = aNames.getConstArray();
    sal_Int32 nSymbolProps = aNames.getLength();

    Sequence< PropertyValue > aValues( nCount * nSymbolProps );
    PropertyValue *pValues = aValues.getArray();

    PropertyValue *pVal = pValues;
    OUString aDelim( u"/"_ustr );
    for (const SmSym& rSymbol : rNewSymbols)
    {
        OUString  aNodeNameDelim = SYMBOL_LIST +
            aDelim +
            rSymbol.GetExportName() +
            aDelim;

        const OUString *pName = pNames;

        // Char
        pVal->Name  = aNodeNameDelim;
        pVal->Name += *pName++;
        pVal->Value <<= rSymbol.GetCharacter();
        pVal++;
        // Set
        pVal->Name  = aNodeNameDelim;
        pVal->Name += *pName++;
        OUString aTmp( rSymbol.GetSymbolSetName() );
        if (rSymbol.IsPredefined())
            aTmp = SmLocalizedSymbolData::GetExportSymbolSetName( aTmp );
        pVal->Value <<= aTmp;
        pVal++;
        // Predefined
        pVal->Name  = aNodeNameDelim;
        pVal->Name += *pName++;
        pVal->Value <<= rSymbol.IsPredefined();
        pVal++;
        // FontFormatId
        SmFontFormat aFntFmt( rSymbol.GetFace() );
        OUString aFntFmtId( GetFontFormatList().GetFontFormatId( aFntFmt, true ) );
        OSL_ENSURE( !aFntFmtId.isEmpty(), "FontFormatId not found" );
        pVal->Name  = aNodeNameDelim;
        pVal->Name += *pName++;
        pVal->Value <<= aFntFmtId;
        pVal++;
    }
    OSL_ENSURE( pVal - pValues == sal::static_int_cast< ptrdiff_t >(nCount * nSymbolProps), "properties missing" );
    ReplaceSetProperties( SYMBOL_LIST, aValues );

    StripFontFormatList( rNewSymbols );
}

SmFontFormatList & SmMathConfig::GetFontFormatList()
{
    if (!pFontFormatList)
    {
        LoadFontFormatList();
    }
    return *pFontFormatList;
}

void SmMathConfig::LoadFontFormatList()
{
    if (!pFontFormatList)
        pFontFormatList.reset(new SmFontFormatList);
    else
        pFontFormatList->Clear();

    const Sequence< OUString > aNodes( GetNodeNames( FONT_FORMAT_LIST ) );

    for (const OUString& rNode : aNodes)
    {
        SmFontFormat aFntFmt;
        ReadFontFormat( aFntFmt, rNode, FONT_FORMAT_LIST );
        if (!pFontFormatList->GetFontFormat( rNode ))
            pFontFormatList->AddFontFormat( rNode, aFntFmt );
    }
    pFontFormatList->SetModified( false );
}


void SmMathConfig::ReadFontFormat( SmFontFormat &rFontFormat,
        std::u16string_view rSymbolName, std::u16string_view rBaseNode ) const
{
    Sequence< OUString > aNames = lcl_GetFontPropertyNames();
    sal_Int32 nProps = aNames.getLength();

    OUString aDelim( u"/"_ustr );
    for (auto& rName : asNonConstRange(aNames))
        rName = rBaseNode + aDelim + rSymbolName + aDelim + rName;

    const Sequence< Any > aValues = const_cast<SmMathConfig*>(this)->GetProperties(aNames);

    if (!(nProps  &&  aValues.getLength() == nProps))
        return;

    const Any * pValue = aValues.getConstArray();

    OUString    aTmpStr;
    sal_Int16       nTmp16 = 0;

    bool bOK = true;
    if (pValue->hasValue()  &&  (*pValue >>= aTmpStr))
        rFontFormat.aName = aTmpStr;
    else
        bOK = false;
    ++pValue;
    if (pValue->hasValue()  &&  (*pValue >>= nTmp16))
        rFontFormat.nCharSet = nTmp16; // 6.0 file-format GetSOLoadTextEncoding not needed
    else
        bOK = false;
    ++pValue;
    if (pValue->hasValue()  &&  (*pValue >>= nTmp16))
        rFontFormat.nFamily = nTmp16;
    else
        bOK = false;
    ++pValue;
    if (pValue->hasValue()  &&  (*pValue >>= nTmp16))
        rFontFormat.nPitch = nTmp16;
    else
        bOK = false;
    ++pValue;
    if (pValue->hasValue()  &&  (*pValue >>= nTmp16))
        rFontFormat.nWeight = nTmp16;
    else
        bOK = false;
    ++pValue;
    if (pValue->hasValue()  &&  (*pValue >>= nTmp16))
        rFontFormat.nItalic = nTmp16;
    else
        bOK = false;
    ++pValue;

    OSL_ENSURE( bOK, "read FontFormat failed" );
}

css::uno::Sequence<OUString> SmMathConfig::LoadUserDefinedNames()
{
    m_sUserDefinedNames = GetNodeNames(USER_DEFINED_LIST);
    return m_sUserDefinedNames;
}

void SmMathConfig::GetUserDefinedFormula(std::u16string_view sName, OUString &sFormula)
{
    css::uno::Sequence<OUString> aNames(1);
    OUString* pName = aNames.getArray();
    pName[0] = USER_DEFINED_LIST + "/" + sName + "/FormulaText";
    const Sequence<Any> aValues(GetProperties(aNames));
    const Any* pValues = aValues.getConstArray();
    const Any* pVal = pValues;
    *pVal >>= sFormula;
}

bool SmMathConfig::HasUserDefinedFormula(std::u16string_view sName)
{
    for (int i = 0; i < m_sUserDefinedNames.getLength(); i++)
        if (m_sUserDefinedNames[i] == sName)
            return true;
    return false;
}

void SmMathConfig::SaveUserDefinedFormula(std::u16string_view sName, const OUString& sElement)
{
    Sequence<PropertyValue> pValues(1);
    auto pArgs = pValues.getArray();

    pArgs[0].Name = USER_DEFINED_LIST + "/" + sName + "/FormulaText";
    pArgs[0].Value <<= sElement;

    SetSetProperties( USER_DEFINED_LIST, pValues );
}

void SmMathConfig::DeleteUserDefinedFormula(std::u16string_view sName)
{
    Sequence<OUString> aElements { OUString(sName) };
    ClearNodeElements(USER_DEFINED_LIST, aElements);
}

void SmMathConfig::SaveFontFormatList()
{
    SmFontFormatList &rFntFmtList = GetFontFormatList();

    if (!rFntFmtList.IsModified())
        return;

    Sequence< OUString > aNames = lcl_GetFontPropertyNames();
    sal_Int32 nSymbolProps = aNames.getLength();

    size_t nCount = rFntFmtList.GetCount();

    Sequence< PropertyValue > aValues( nCount * nSymbolProps );
    PropertyValue *pValues = aValues.getArray();

    PropertyValue *pVal = pValues;
    OUString aDelim( u"/"_ustr );
    for (size_t i = 0;  i < nCount;  ++i)
    {
        OUString aFntFmtId(rFntFmtList.GetFontFormatId(i));
        const SmFontFormat *pFntFmt = rFntFmtList.GetFontFormat(i);
        assert(pFntFmt);
        const SmFontFormat aFntFmt(*pFntFmt);

        OUString  aNodeNameDelim = FONT_FORMAT_LIST +
            aDelim +
            aFntFmtId +
            aDelim;

        const OUString *pName = aNames.getConstArray();

        // Name
        pVal->Name  = aNodeNameDelim;
        pVal->Name += *pName++;
        pVal->Value <<= aFntFmt.aName;
        pVal++;
        // CharSet
        pVal->Name  = aNodeNameDelim;
        pVal->Name += *pName++;
        pVal->Value <<= aFntFmt.nCharSet; // 6.0 file-format GetSOStoreTextEncoding not needed
        pVal++;
        // Family
        pVal->Name  = aNodeNameDelim;
        pVal->Name += *pName++;
        pVal->Value <<= aFntFmt.nFamily;
        pVal++;
        // Pitch
        pVal->Name  = aNodeNameDelim;
        pVal->Name += *pName++;
        pVal->Value <<= aFntFmt.nPitch;
        pVal++;
        // Weight
        pVal->Name  = aNodeNameDelim;
        pVal->Name += *pName++;
        pVal->Value <<= aFntFmt.nWeight;
        pVal++;
        // Italic
        pVal->Name  = aNodeNameDelim;
        pVal->Name += *pName++;
        pVal->Value <<= aFntFmt.nItalic;
        pVal++;
    }
    OSL_ENSURE( sal::static_int_cast<size_t>(pVal - pValues) == nCount * nSymbolProps, "properties missing" );
    ReplaceSetProperties( FONT_FORMAT_LIST, aValues );

    rFntFmtList.SetModified( false );
}


void SmMathConfig::StripFontFormatList( const std::vector< SmSym > &rSymbols )
{
    size_t i;

    // build list of used font-formats only
    //!! font-format IDs may be different !!
    SmFontFormatList aUsedList;
    for (i = 0;  i < rSymbols.size();  ++i)
    {
        OSL_ENSURE( !rSymbols[i].GetUiName().isEmpty(), "non named symbol" );
        aUsedList.GetFontFormatId( SmFontFormat( rSymbols[i].GetFace() ) , true );
    }
    const SmFormat & rStdFmt = GetStandardFormat();
    for (i = FNT_BEGIN;  i <= FNT_END;  ++i)
    {
        aUsedList.GetFontFormatId( SmFontFormat( rStdFmt.GetFont( i ) ) , true );
    }

    // remove unused font-formats from list
    SmFontFormatList &rFntFmtList = GetFontFormatList();
    size_t nCnt = rFntFmtList.GetCount();
    std::unique_ptr<SmFontFormat[]> pTmpFormat(new SmFontFormat[ nCnt ]);
    std::unique_ptr<OUString[]> pId(new OUString[ nCnt ]);
    size_t k;
    for (k = 0;  k < nCnt;  ++k)
    {
        pTmpFormat[k] = *rFntFmtList.GetFontFormat( k );
        pId[k]     = rFntFmtList.GetFontFormatId( k );
    }
    for (k = 0;  k < nCnt;  ++k)
    {
        if (aUsedList.GetFontFormatId( pTmpFormat[k] ).isEmpty())
        {
            rFntFmtList.RemoveFontFormat( pId[k] );
        }
    }
}


void SmMathConfig::LoadOther()
{
    if (!pOther)
        pOther.reset(new SmCfgOther);

    const Sequence<OUString> aNames(lcl_GetOtherPropertyNames());
    const Sequence<Any> aValues(GetProperties(aNames));
    if (aNames.getLength() != aValues.getLength())
        return;

    const Any* pValues = aValues.getConstArray();
    const Any* pVal = pValues;

    // LoadSave/IsSaveOnlyUsedSymbols
    if (bool bTmp; pVal->hasValue() && (*pVal >>= bTmp))
        pOther->bIsSaveOnlyUsedSymbols = bTmp;
    ++pVal;
    // Misc/AutoCloseBrackets
    if (bool bTmp; pVal->hasValue() && (*pVal >>= bTmp))
        pOther->bIsAutoCloseBrackets = bTmp;
    ++pVal;
    // Misc/DefaultSmSyntaxVersion
    if (sal_Int16 nTmp; pVal->hasValue() && (*pVal >>= nTmp))
        pOther->nSmSyntaxVersion = nTmp;
    ++pVal;
    // Misc/InlineEditEnable
    if (bool bTmp; pVal->hasValue() && (*pVal >>= bTmp))
        pOther->bInlineEditEnable = bTmp;
    ++pVal;
    // Misc/IgnoreSpacesRight
    if (bool bTmp; pVal->hasValue() && (*pVal >>= bTmp))
        pOther->bIgnoreSpacesRight = bTmp;
    ++pVal;
    // Misc/SmEditWindowZoomFactor
    if (sal_Int16 nTmp; pVal->hasValue() && (*pVal >>= nTmp))
        pOther->nSmEditWindowZoomFactor = nTmp;
    ++pVal;
    // Print/FormulaText
    if (bool bTmp; pVal->hasValue() && (*pVal >>= bTmp))
        pOther->bPrintFormulaText = bTmp;
    ++pVal;
    // Print/Frame
    if (bool bTmp; pVal->hasValue() && (*pVal >>= bTmp))
        pOther->bPrintFrame = bTmp;
    ++pVal;
    // Print/Size:
    if (sal_Int16 nTmp; pVal->hasValue() && (*pVal >>= nTmp))
        pOther->ePrintSize = static_cast<SmPrintSize>(nTmp);
    ++pVal;
    // Print/Title
    if (bool bTmp; pVal->hasValue() && (*pVal >>= bTmp))
        pOther->bPrintTitle = bTmp;
    ++pVal;
    // Print/ZoomFactor
    if (sal_Int16 nTmp; pVal->hasValue() && (*pVal >>= nTmp))
        pOther->nPrintZoomFactor = nTmp;
    ++pVal;
    // View/AutoRedraw
    if (bool bTmp; pVal->hasValue() && (*pVal >>= bTmp))
        pOther->bAutoRedraw = bTmp;
    ++pVal;
    // View/FormulaCursor
    if (bool bTmp; pVal->hasValue() && (*pVal >>= bTmp))
        pOther->bFormulaCursor = bTmp;
    ++pVal;
    // View/ToolboxVisible
    if (bool bTmp; pVal->hasValue() && (*pVal >>= bTmp))
        pOther->bToolboxVisible = bTmp;
    ++pVal;

    OSL_ENSURE(pVal - pValues == aNames.getLength(), "property mismatch");
    SetOtherModified( false );
}


void SmMathConfig::SaveOther()
{
    if (!pOther || !IsOtherModified())
        return;

    const Sequence<OUString> aNames(lcl_GetOtherPropertyNames());
    Sequence<Any> aValues(aNames.getLength());

    Any* pValues = aValues.getArray();
    Any* pVal = pValues;

    // LoadSave/IsSaveOnlyUsedSymbols
    *pVal++ <<= pOther->bIsSaveOnlyUsedSymbols;
    // Misc/AutoCloseBrackets
    *pVal++ <<= pOther->bIsAutoCloseBrackets;
    // Misc/DefaultSmSyntaxVersion
    *pVal++ <<= pOther->nSmSyntaxVersion;
    // Misc/InlineEditEnable
    *pVal++ <<= pOther->bInlineEditEnable;
    // Misc/IgnoreSpacesRight
    *pVal++ <<= pOther->bIgnoreSpacesRight;
    // Misc/SmEditWindowZoomFactor
    *pVal++ <<= pOther->nSmEditWindowZoomFactor;
    // Print/FormulaText
    *pVal++ <<= pOther->bPrintFormulaText;
    // Print/Frame
    *pVal++ <<= pOther->bPrintFrame;
    // Print/Size:
    *pVal++ <<= static_cast<sal_Int16>(pOther->ePrintSize);
    // Print/Title
    *pVal++ <<= pOther->bPrintTitle;
    // Print/ZoomFactor
    *pVal++ <<= pOther->nPrintZoomFactor;
    // View/AutoRedraw
    *pVal++ <<= pOther->bAutoRedraw;
    // View/FormulaCursor
    *pVal++ <<= pOther->bFormulaCursor;
    // View/ToolboxVisible
    *pVal++ <<= pOther->bToolboxVisible;

    OSL_ENSURE(pVal - pValues == aNames.getLength(), "property mismatch");
    PutProperties(aNames, aValues);

    SetOtherModified( false );
}

namespace {

// Latin default-fonts
const DefaultFontType aLatinDefFnts[FNT_END] =
{
    DefaultFontType::SERIF,  // FNT_VARIABLE
    DefaultFontType::SERIF,  // FNT_FUNCTION
    DefaultFontType::SERIF,  // FNT_NUMBER
    DefaultFontType::SERIF,  // FNT_TEXT
    DefaultFontType::SERIF,  // FNT_SERIF
    DefaultFontType::SANS,   // FNT_SANS
    DefaultFontType::FIXED   // FNT_FIXED
    //OpenSymbol,    // FNT_MATH
};

// CJK default-fonts
//! we use non-asian fonts for variables, functions and numbers since they
//! look better and even in asia only latin letters will be used for those.
//! At least that's what I was told...
const DefaultFontType aCJKDefFnts[FNT_END] =
{
    DefaultFontType::SERIF,          // FNT_VARIABLE
    DefaultFontType::SERIF,          // FNT_FUNCTION
    DefaultFontType::SERIF,          // FNT_NUMBER
    DefaultFontType::CJK_TEXT,       // FNT_TEXT
    DefaultFontType::CJK_TEXT,       // FNT_SERIF
    DefaultFontType::CJK_DISPLAY,    // FNT_SANS
    DefaultFontType::CJK_TEXT        // FNT_FIXED
    //OpenSymbol,    // FNT_MATH
};

// CTL default-fonts
const DefaultFontType aCTLDefFnts[FNT_END] =
{
    DefaultFontType::CTL_TEXT,    // FNT_VARIABLE
    DefaultFontType::CTL_TEXT,    // FNT_FUNCTION
    DefaultFontType::CTL_TEXT,    // FNT_NUMBER
    DefaultFontType::CTL_TEXT,    // FNT_TEXT
    DefaultFontType::CTL_TEXT,    // FNT_SERIF
    DefaultFontType::CTL_TEXT,    // FNT_SANS
    DefaultFontType::CTL_TEXT     // FNT_FIXED
    //OpenSymbol,    // FNT_MATH
};


OUString lcl_GetDefaultFontName( LanguageType nLang, sal_uInt16 nIdent )
{
    assert(nIdent < FNT_END);
    const DefaultFontType *pTable;
    switch ( SvtLanguageOptions::GetScriptTypeOfLanguage( nLang ) )
    {
        case SvtScriptType::LATIN :     pTable = aLatinDefFnts; break;
        case SvtScriptType::ASIAN :     pTable = aCJKDefFnts; break;
        case SvtScriptType::COMPLEX :   pTable = aCTLDefFnts; break;
        default :
            pTable = aLatinDefFnts;
            SAL_WARN("starmath", "unknown script-type");
    }

    return OutputDevice::GetDefaultFont(pTable[ nIdent ], nLang,
                                        GetDefaultFontFlags::OnlyOne ).GetFamilyName();
}

}


void SmMathConfig::LoadFormat()
{
    if (!pFormat)
        pFormat.reset(new SmFormat);


    Sequence< OUString > aNames = lcl_GetFormatPropertyNames();

    sal_Int32 nProps = aNames.getLength();

    Sequence< Any > aValues( GetProperties( aNames ) );
    if (!(nProps  &&  aValues.getLength() == nProps))
        return;

    const Any *pValues = aValues.getConstArray();
    const Any *pVal = pValues;

    OUString    aTmpStr;
    sal_Int16       nTmp16 = 0;
    bool        bTmp = false;

    // StandardFormat/Textmode
    if (pVal->hasValue()  &&  (*pVal >>= bTmp))
        pFormat->SetTextmode( bTmp );
    ++pVal;
    // StandardFormat/RightToLeft
    if (pVal->hasValue()  &&  (*pVal >>= bTmp))
        pFormat->SetRightToLeft( bTmp );
    ++pVal;
    // StandardFormat/GreekCharStyle
    if (pVal->hasValue()  &&  (*pVal >>= nTmp16))
        pFormat->SetGreekCharStyle( nTmp16 );
    ++pVal;
    // StandardFormat/ScaleNormalBracket
    if (pVal->hasValue()  &&  (*pVal >>= bTmp))
        pFormat->SetScaleNormalBrackets( bTmp );
    ++pVal;
    // StandardFormat/HorizontalAlignment
    if (pVal->hasValue()  &&  (*pVal >>= nTmp16))
        pFormat->SetHorAlign( static_cast<SmHorAlign>(nTmp16) );
    ++pVal;
    // StandardFormat/BaseSize
    if (pVal->hasValue()  &&  (*pVal >>= nTmp16))
        pFormat->SetBaseSize(Size(0, o3tl::convert(nTmp16, o3tl::Length::pt, SmO3tlLengthUnit())));
    ++pVal;

    sal_uInt16 i;
    for (i = SIZ_BEGIN;  i <= SIZ_END;  ++i)
    {
        if (pVal->hasValue()  &&  (*pVal >>= nTmp16))
            pFormat->SetRelSize( i, nTmp16 );
        ++pVal;
    }

    for (i = DIS_BEGIN;  i <= DIS_END;  ++i)
    {
        if (pVal->hasValue()  &&  (*pVal >>= nTmp16))
            pFormat->SetDistance( i, nTmp16 );
        ++pVal;
    }

    LanguageType nLang = Application::GetSettings().GetUILanguageTag().getLanguageType();
    for (i = FNT_BEGIN;  i < FNT_END;  ++i)
    {
        vcl::Font aFnt;
        bool bUseDefaultFont = true;
        if (pVal->hasValue()  &&  (*pVal >>= aTmpStr))
        {
            bUseDefaultFont = aTmpStr.isEmpty();
            if (bUseDefaultFont)
            {
                aFnt = pFormat->GetFont( i );
                aFnt.SetFamilyName( lcl_GetDefaultFontName( nLang, i ) );
            }
            else
            {
                const SmFontFormat *pFntFmt = GetFontFormatList().GetFontFormat( aTmpStr );
                OSL_ENSURE( pFntFmt, "unknown FontFormat" );
                if (pFntFmt)
                    aFnt = pFntFmt->GetFont();
            }
        }
        ++pVal;

        aFnt.SetFontSize( pFormat->GetBaseSize() );
        pFormat->SetFont( i, SmFace(aFnt), bUseDefaultFont );
    }

    OSL_ENSURE( pVal - pValues == nProps, "property mismatch" );
    SetFormatModified( false );
}


void SmMathConfig::SaveFormat()
{
    if (!pFormat || !IsFormatModified())
        return;

    const Sequence< OUString > aNames = lcl_GetFormatPropertyNames();
    sal_Int32 nProps = aNames.getLength();

    Sequence< Any > aValues( nProps );
    Any *pValues = aValues.getArray();
    Any *pValue  = pValues;

    // StandardFormat/Textmode
    *pValue++ <<= pFormat->IsTextmode();
    // StandardFormat/RightToLeft
    *pValue++ <<= pFormat->IsRightToLeft();
    // StandardFormat/GreekCharStyle
    *pValue++ <<= pFormat->GetGreekCharStyle();
    // StandardFormat/ScaleNormalBracket
    *pValue++ <<= pFormat->IsScaleNormalBrackets();
    // StandardFormat/HorizontalAlignment
    *pValue++ <<= static_cast<sal_Int16>(pFormat->GetHorAlign());
    // StandardFormat/BaseSize
    *pValue++ <<= static_cast<sal_Int16>(
        o3tl::convert(pFormat->GetBaseSize().Height(), SmO3tlLengthUnit(), o3tl::Length::pt));

    sal_uInt16 i;
    for (i = SIZ_BEGIN;  i <= SIZ_END;  ++i)
        *pValue++ <<= static_cast<sal_Int16>(pFormat->GetRelSize( i ));

    for (i = DIS_BEGIN;  i <= DIS_END;  ++i)
        *pValue++ <<= static_cast<sal_Int16>(pFormat->GetDistance( i ));

    for (i = FNT_BEGIN;  i < FNT_END;  ++i)
    {
        OUString aFntFmtId;

        if (!pFormat->IsDefaultFont( i ))
        {
            SmFontFormat aFntFmt( pFormat->GetFont( i ) );
            aFntFmtId = GetFontFormatList().GetFontFormatId( aFntFmt, true );
            OSL_ENSURE( !aFntFmtId.isEmpty(), "FontFormatId not found" );
        }

        *pValue++ <<= aFntFmtId;
    }

    OSL_ENSURE( pValue - pValues == nProps, "property mismatch" );
    PutProperties( aNames , aValues );

    SetFormatModified( false );
}


const SmFormat & SmMathConfig::GetStandardFormat() const
{
    if (!pFormat)
        const_cast<SmMathConfig*>(this)->LoadFormat();
    return *pFormat;
}


void SmMathConfig::SetStandardFormat( const SmFormat &rFormat, bool bSaveFontFormatList )
{
    if (!pFormat)
        LoadFormat();
    if (rFormat == *pFormat)
        return;

    CommitLocker aLock(*this);
    *pFormat = rFormat;
    SetFormatModified( true );

    if (bSaveFontFormatList)
    {
        // needed for SmFontTypeDialog's DefaultButtonClickHdl
        if (pFontFormatList)
            pFontFormatList->SetModified( true );
    }
}


SmPrintSize SmMathConfig::GetPrintSize() const
{
    if (!pOther)
        const_cast<SmMathConfig*>(this)->LoadOther();
    return pOther->ePrintSize;
}


void SmMathConfig::SetPrintSize( SmPrintSize eSize )
{
    if (!pOther)
        LoadOther();
    if (eSize != pOther->ePrintSize)
    {
        CommitLocker aLock(*this);
        pOther->ePrintSize = eSize;
        SetOtherModified( true );
    }
}


sal_uInt16 SmMathConfig::GetPrintZoomFactor() const
{
    if (!pOther)
        const_cast<SmMathConfig*>(this)->LoadOther();
    return pOther->nPrintZoomFactor;
}


void SmMathConfig::SetPrintZoomFactor( sal_uInt16 nVal )
{
    if (!pOther)
        LoadOther();
    if (nVal != pOther->nPrintZoomFactor)
    {
        CommitLocker aLock(*this);
        pOther->nPrintZoomFactor = nVal;
        SetOtherModified( true );
    }
}


sal_uInt16 SmMathConfig::GetSmEditWindowZoomFactor() const
{
    sal_uInt16 smzoomfactor;
    if (!pOther)
        const_cast<SmMathConfig*>(this)->LoadOther();
    smzoomfactor = pOther->nSmEditWindowZoomFactor;
    return smzoomfactor < 10 || smzoomfactor > 1000 ? 100 : smzoomfactor;
}


void SmMathConfig::SetSmEditWindowZoomFactor( sal_uInt16 nVal )
{
    if (!pOther)
        LoadOther();
    if (nVal != pOther->nSmEditWindowZoomFactor)
    {
        CommitLocker aLock(*this);
        pOther->nSmEditWindowZoomFactor = nVal;
        SetOtherModified( true );
    }
}


bool SmMathConfig::SetOtherIfNotEqual( bool &rbItem, bool bNewVal )
{
    if (bNewVal != rbItem)
    {
        CommitLocker aLock(*this);
        rbItem = bNewVal;
        SetOtherModified( true );
        return true;
    }
    return false;
}


bool SmMathConfig::IsPrintTitle() const
{
    if (!pOther)
        const_cast<SmMathConfig*>(this)->LoadOther();
    return pOther->bPrintTitle;
}


void SmMathConfig::SetPrintTitle( bool bVal )
{
    if (!pOther)
        LoadOther();
    SetOtherIfNotEqual( pOther->bPrintTitle, bVal );
}


bool SmMathConfig::IsPrintFormulaText() const
{
    if (!pOther)
        const_cast<SmMathConfig*>(this)->LoadOther();
    return pOther->bPrintFormulaText;
}


void SmMathConfig::SetPrintFormulaText( bool bVal )
{
    if (!pOther)
        LoadOther();
    SetOtherIfNotEqual( pOther->bPrintFormulaText, bVal );
}

bool SmMathConfig::IsSaveOnlyUsedSymbols() const
{
    if (!pOther)
        const_cast<SmMathConfig*>(this)->LoadOther();
    return pOther->bIsSaveOnlyUsedSymbols;
}

bool SmMathConfig::IsAutoCloseBrackets() const
{
    if (!pOther)
        const_cast<SmMathConfig*>(this)->LoadOther();
    return pOther->bIsAutoCloseBrackets;
}

sal_Int16 SmMathConfig::GetDefaultSmSyntaxVersion() const
{
    if (comphelper::IsFuzzing())
        return nDefaultSmSyntaxVersion;
    if (!pOther)
        const_cast<SmMathConfig*>(this)->LoadOther();
    return pOther->nSmSyntaxVersion;
}

bool SmMathConfig::IsPrintFrame() const
{
    if (!pOther)
        const_cast<SmMathConfig*>(this)->LoadOther();
    return pOther->bPrintFrame;
}


void SmMathConfig::SetPrintFrame( bool bVal )
{
    if (!pOther)
        LoadOther();
    SetOtherIfNotEqual( pOther->bPrintFrame, bVal );
}


void SmMathConfig::SetSaveOnlyUsedSymbols( bool bVal )
{
    if (!pOther)
        LoadOther();
    SetOtherIfNotEqual( pOther->bIsSaveOnlyUsedSymbols, bVal );
}


void SmMathConfig::SetAutoCloseBrackets( bool bVal )
{
    if (!pOther)
        LoadOther();
    SetOtherIfNotEqual( pOther->bIsAutoCloseBrackets, bVal );
}

void SmMathConfig::SetDefaultSmSyntaxVersion( sal_Int16 nVal )
{
    if (!pOther)
        LoadOther();
    if (nVal != pOther->nSmSyntaxVersion)
    {
        CommitLocker aLock(*this);
        pOther->nSmSyntaxVersion = nVal;
        SetOtherModified( true );
    }
}

bool SmMathConfig::IsInlineEditEnable() const
{
    if (comphelper::IsFuzzing())
        return false;
    if (!pOther)
        const_cast<SmMathConfig*>(this)->LoadOther();
    return pOther->bInlineEditEnable;
}


void SmMathConfig::SetInlineEditEnable( bool bVal )
{
    if (!pOther)
        LoadOther();
    if (SetOtherIfNotEqual( pOther->bInlineEditEnable, bVal ))
    {
        // reformat (displayed) formulas accordingly
        Broadcast(SfxHint(SfxHintId::MathFormatChanged));
    }
}

bool SmMathConfig::IsIgnoreSpacesRight() const
{
    if (comphelper::IsFuzzing())
        return false;
    if (!pOther)
        const_cast<SmMathConfig*>(this)->LoadOther();
    return pOther->bIgnoreSpacesRight;
}


void SmMathConfig::SetIgnoreSpacesRight( bool bVal )
{
    if (!pOther)
        LoadOther();
    if (SetOtherIfNotEqual( pOther->bIgnoreSpacesRight, bVal ))
    {
        // reformat (displayed) formulas accordingly
        Broadcast(SfxHint(SfxHintId::MathFormatChanged));
    }

}


bool SmMathConfig::IsAutoRedraw() const
{
    if (!pOther)
        const_cast<SmMathConfig*>(this)->LoadOther();
    return pOther->bAutoRedraw;
}


void SmMathConfig::SetAutoRedraw( bool bVal )
{
    if (!pOther)
        LoadOther();
    SetOtherIfNotEqual( pOther->bAutoRedraw, bVal );
}


bool SmMathConfig::IsShowFormulaCursor() const
{
    if (!pOther)
        const_cast<SmMathConfig*>(this)->LoadOther();
    return pOther->bFormulaCursor;
}


void SmMathConfig::SetShowFormulaCursor( bool bVal )
{
    if (!pOther)
        LoadOther();
    SetOtherIfNotEqual( pOther->bFormulaCursor, bVal );
}

void SmMathConfig::Notify( const css::uno::Sequence< OUString >& rNames )
{
    Clear();
    if (std::find(rNames.begin(), rNames.end(), "Misc/IgnoreSpacesRight") != rNames.end())
        Broadcast(SfxHint(SfxHintId::MathFormatChanged));
}


void SmMathConfig::ItemSetToConfig(const SfxItemSet &rSet)
{
    CommitLocker aLock(*this);

    sal_uInt16 nU16;
    bool bVal;
    if (const SfxUInt16Item* pPrintSizeItem = rSet.GetItemIfSet(SID_PRINTSIZE))
    {   nU16 = pPrintSizeItem->GetValue();
        SetPrintSize( static_cast<SmPrintSize>(nU16) );
    }
    if (const SfxUInt16Item* pPrintZoomItem = rSet.GetItemIfSet(SID_PRINTZOOM))
    {   nU16 = pPrintZoomItem->GetValue();
        SetPrintZoomFactor( nU16 );
    }
    if (const SfxUInt16Item* pPrintZoomItem = rSet.GetItemIfSet(SID_SMEDITWINDOWZOOM))
    {   nU16 = pPrintZoomItem->GetValue();
        SetSmEditWindowZoomFactor( nU16 );
    }
    if (const SfxBoolItem* pPrintTitleItem = rSet.GetItemIfSet(SID_PRINTTITLE))
    {   bVal = pPrintTitleItem->GetValue();
        SetPrintTitle( bVal );
    }
    if (const SfxBoolItem* pPrintTextItem = rSet.GetItemIfSet(SID_PRINTTEXT))
    {   bVal = pPrintTextItem->GetValue();
        SetPrintFormulaText( bVal );
    }
    if (const SfxBoolItem* pPrintZoomItem = rSet.GetItemIfSet(SID_PRINTFRAME))
    {   bVal = pPrintZoomItem->GetValue();
        SetPrintFrame( bVal );
    }
    if (const SfxBoolItem* pRedrawItem = rSet.GetItemIfSet(SID_AUTOREDRAW))
    {   bVal = pRedrawItem->GetValue();
        SetAutoRedraw( bVal );
    }
    if (const SfxBoolItem* pInlineEditItem = rSet.GetItemIfSet(SID_INLINE_EDIT_ENABLE))
    {   bVal = pInlineEditItem->GetValue();
        SetInlineEditEnable( bVal );
    }
    if (const SfxBoolItem* pSpacesItem = rSet.GetItemIfSet(SID_NO_RIGHT_SPACES))
    {   bVal = pSpacesItem->GetValue();
        SetIgnoreSpacesRight( bVal );
    }
    if (const SfxBoolItem* pSymbolsItem = rSet.GetItemIfSet(SID_SAVE_ONLY_USED_SYMBOLS))
    {   bVal = pSymbolsItem->GetValue();
        SetSaveOnlyUsedSymbols( bVal );
    }

    if (const SfxBoolItem* pBracketsItem = rSet.GetItemIfSet(SID_AUTO_CLOSE_BRACKETS))
    {
        bVal = pBracketsItem->GetValue();
        SetAutoCloseBrackets( bVal );
    }

    if (const SfxUInt16Item* pSyntaxItem = rSet.GetItemIfSet(SID_DEFAULT_SM_SYNTAX_VERSION))
    {
        nU16 = pSyntaxItem->GetValue();
        SetDefaultSmSyntaxVersion( nU16 );
    }
}


void SmMathConfig::ConfigToItemSet(SfxItemSet &rSet) const
{
    rSet.Put(SfxUInt16Item(SID_PRINTSIZE,
                           sal::static_int_cast<sal_uInt16>(GetPrintSize())));
    rSet.Put(SfxUInt16Item(SID_PRINTZOOM,
                           GetPrintZoomFactor()));
    rSet.Put(SfxUInt16Item(SID_SMEDITWINDOWZOOM,
                           GetSmEditWindowZoomFactor()));

    rSet.Put(SfxBoolItem(SID_PRINTTITLE, IsPrintTitle()));
    rSet.Put(SfxBoolItem(SID_PRINTTEXT,  IsPrintFormulaText()));
    rSet.Put(SfxBoolItem(SID_PRINTFRAME, IsPrintFrame()));
    rSet.Put(SfxBoolItem(SID_AUTOREDRAW, IsAutoRedraw()));
    rSet.Put(SfxBoolItem(SID_INLINE_EDIT_ENABLE, IsInlineEditEnable()));
    rSet.Put(SfxBoolItem(SID_NO_RIGHT_SPACES, IsIgnoreSpacesRight()));
    rSet.Put(SfxBoolItem(SID_SAVE_ONLY_USED_SYMBOLS, IsSaveOnlyUsedSymbols()));
    rSet.Put(SfxBoolItem(SID_AUTO_CLOSE_BRACKETS, IsAutoCloseBrackets()));
    rSet.Put(SfxBoolItem(SID_DEFAULT_SM_SYNTAX_VERSION, GetDefaultSmSyntaxVersion()));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
