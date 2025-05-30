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

#include <svl/numformat.hxx>
#include <osl/diagnose.h>

#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Sequence.hxx>

#include <docoptio.hxx>
#include <miscuno.hxx>
#include <global.hxx>

using namespace utl;
using namespace com::sun::star::uno;


using sc::TwipsToEvenHMM;

static sal_uInt16 lcl_GetDefaultTabDist()
{
    if ( ScOptionsUtil::IsMetricSystem() )
        return 709;                 // 1,25 cm
    else
        return 720;                 // 1/2"
}

//      ScDocOptions - document options

ScDocOptions::ScDocOptions()
{
    ResetDocOptions();
}

void ScDocOptions::ResetDocOptions()
{
    bIsIgnoreCase       = false;
    bIsIter             = false;
    nIterCount          = 100;
    fIterEps            = 1.0E-3;
    nPrecStandardFormat = SvNumberFormatter::UNLIMITED_PRECISION;
    nDay                = 30;
    nMonth              = 12;
    nYear               = 1899;
    nYear2000           = SvNumberFormatter::GetYear2000Default();
    nTabDistance        = lcl_GetDefaultTabDist();
    bCalcAsShown        = false;
    bMatchWholeCell     = true;
    bLookUpColRowNames  = true;
    bFormulaRegexEnabled= false;
    bFormulaWildcardsEnabled= true;
    eFormulaSearchType  = utl::SearchParam::SearchType::Wildcard;
    bWriteCalcConfig    = true;
}

void ScDocOptions::SetFormulaRegexEnabled( bool bVal )
{
    if (bVal)
    {
        bFormulaRegexEnabled = true;
        bFormulaWildcardsEnabled = false;
        eFormulaSearchType = utl::SearchParam::SearchType::Regexp;
    }
    else if (!bFormulaRegexEnabled)
        ;   // nothing changes for setting false to false
    else
    {
        bFormulaRegexEnabled = false;
        eFormulaSearchType = utl::SearchParam::SearchType::Unknown;
    }
}

void ScDocOptions::SetFormulaWildcardsEnabled( bool bVal )
{
    if (bVal)
    {
        bFormulaRegexEnabled = false;
        bFormulaWildcardsEnabled = true;
        eFormulaSearchType = utl::SearchParam::SearchType::Wildcard;
    }
    else if (!bFormulaWildcardsEnabled)
        ;   // nothing changes for setting false to false
    else
    {
        bFormulaWildcardsEnabled = false;
        eFormulaSearchType = utl::SearchParam::SearchType::Unknown;
    }
}

//      ScTpCalcItem - data for the CalcOptions TabPage

ScTpCalcItem::ScTpCalcItem( sal_uInt16 nWhichP, const ScDocOptions& rOpt )
    :   SfxPoolItem ( nWhichP ),
        theOptions  ( rOpt )
{
}

ScTpCalcItem::~ScTpCalcItem()
{
}

bool ScTpCalcItem::operator==( const SfxPoolItem& rItem ) const
{
    assert(SfxPoolItem::operator==(rItem));

    const ScTpCalcItem& rPItem = static_cast<const ScTpCalcItem&>(rItem);

    return ( theOptions == rPItem.theOptions );
}

ScTpCalcItem* ScTpCalcItem::Clone( SfxItemPool * ) const
{
    return new ScTpCalcItem( *this );
}

//  Config Item containing document options

constexpr OUStringLiteral CFGPATH_CALC = u"Office.Calc/Calculate";

#define SCCALCOPT_ITER_ITER         0
#define SCCALCOPT_ITER_STEPS        1
#define SCCALCOPT_ITER_MINCHG       2
#define SCCALCOPT_DATE_DAY          3
#define SCCALCOPT_DATE_MONTH        4
#define SCCALCOPT_DATE_YEAR         5
#define SCCALCOPT_DECIMALS          6
#define SCCALCOPT_CASESENSITIVE     7
#define SCCALCOPT_PRECISION         8
#define SCCALCOPT_SEARCHCRIT        9
#define SCCALCOPT_FINDLABEL         10
#define SCCALCOPT_REGEX             11
#define SCCALCOPT_WILDCARDS         12

constexpr OUStringLiteral CFGPATH_DOCLAYOUT = u"Office.Calc/Layout/Other";

#define SCDOCLAYOUTOPT_TABSTOP      0

Sequence<OUString> ScDocCfg::GetCalcPropertyNames()
{
    return {u"IterativeReference/Iteration"_ustr,     // SCCALCOPT_ITER_ITER
            u"IterativeReference/Steps"_ustr,         // SCCALCOPT_ITER_STEPS
            u"IterativeReference/MinimumChange"_ustr, // SCCALCOPT_ITER_MINCHG
            u"Other/Date/DD"_ustr,                    // SCCALCOPT_DATE_DAY
            u"Other/Date/MM"_ustr,                    // SCCALCOPT_DATE_MONTH
            u"Other/Date/YY"_ustr,                    // SCCALCOPT_DATE_YEAR
            u"Other/DecimalPlaces"_ustr,              // SCCALCOPT_DECIMALS
            u"Other/CaseSensitive"_ustr,              // SCCALCOPT_CASESENSITIVE
            u"Other/Precision"_ustr,                  // SCCALCOPT_PRECISION
            u"Other/SearchCriteria"_ustr,             // SCCALCOPT_SEARCHCRIT
            u"Other/FindLabel"_ustr,                  // SCCALCOPT_FINDLABEL
            u"Other/RegularExpressions"_ustr,         // SCCALCOPT_REGEX
            u"Other/Wildcards"_ustr};                 // SCCALCOPT_WILDCARDS
}

Sequence<OUString> ScDocCfg::GetLayoutPropertyNames()
{
    if (ScOptionsUtil::IsMetricSystem())
        return {u"TabStop/Metric"_ustr};    // SCDOCLAYOUTOPT_TABSTOP
    else
        return {u"TabStop/NonMetric"_ustr}; // SCDOCLAYOUTOPT_TABSTOP
}

ScDocCfg::ScDocCfg() :
    aCalcItem( CFGPATH_CALC ),
    aLayoutItem(CFGPATH_DOCLAYOUT)
{
    sal_Int32 nIntVal = 0;

    Sequence<OUString> aNames;
    Sequence<Any> aValues;
    const Any* pValues = nullptr;

    sal_uInt16 nDateDay, nDateMonth;
    sal_Int16 nDateYear;
    GetDate( nDateDay, nDateMonth, nDateYear );

    aNames = GetCalcPropertyNames();
    aValues = aCalcItem.GetProperties(aNames);
    aCalcItem.EnableNotification(aNames);
    pValues = aValues.getConstArray();
    OSL_ENSURE(aValues.getLength() == aNames.getLength(), "GetProperties failed");
    if(aValues.getLength() == aNames.getLength())
    {
        double fDoubleVal = 0;
        for(int nProp = 0; nProp < aNames.getLength(); nProp++)
        {
            OSL_ENSURE(pValues[nProp].hasValue(), "property value missing");
            if(pValues[nProp].hasValue())
            {
                switch(nProp)
                {
                    case SCCALCOPT_ITER_ITER:
                        SetIter( ScUnoHelpFunctions::GetBoolFromAny( pValues[nProp] ) );
                        break;
                    case SCCALCOPT_ITER_STEPS:
                        if (pValues[nProp] >>= nIntVal) SetIterCount( static_cast<sal_uInt16>(nIntVal) );
                        break;
                    case SCCALCOPT_ITER_MINCHG:
                        if (pValues[nProp] >>= fDoubleVal) SetIterEps( fDoubleVal );
                        break;
                    case SCCALCOPT_DATE_DAY:
                        if (pValues[nProp] >>= nIntVal) nDateDay = static_cast<sal_uInt16>(nIntVal);
                        break;
                    case SCCALCOPT_DATE_MONTH:
                        if (pValues[nProp] >>= nIntVal) nDateMonth = static_cast<sal_uInt16>(nIntVal);
                        break;
                    case SCCALCOPT_DATE_YEAR:
                        if (pValues[nProp] >>= nIntVal) nDateYear = static_cast<sal_Int16>(nIntVal);
                        break;
                    case SCCALCOPT_DECIMALS:
                        if (pValues[nProp] >>= nIntVal) SetStdPrecision( static_cast<sal_uInt16>(nIntVal) );
                        break;
                    case SCCALCOPT_CASESENSITIVE:
                        // content is reversed
                        SetIgnoreCase( !ScUnoHelpFunctions::GetBoolFromAny( pValues[nProp] ) );
                        break;
                    case SCCALCOPT_PRECISION:
                        SetCalcAsShown( ScUnoHelpFunctions::GetBoolFromAny( pValues[nProp] ) );
                        break;
                    case SCCALCOPT_SEARCHCRIT:
                        SetMatchWholeCell( ScUnoHelpFunctions::GetBoolFromAny( pValues[nProp] ) );
                        break;
                    case SCCALCOPT_FINDLABEL:
                        SetLookUpColRowNames( ScUnoHelpFunctions::GetBoolFromAny( pValues[nProp] ) );
                        break;
                    case SCCALCOPT_REGEX :
                        SetFormulaRegexEnabled( ScUnoHelpFunctions::GetBoolFromAny( pValues[nProp] ) );
                        break;
                    case SCCALCOPT_WILDCARDS :
                        SetFormulaWildcardsEnabled( ScUnoHelpFunctions::GetBoolFromAny( pValues[nProp] ) );
                        break;
                }
            }
        }
    }
    aCalcItem.SetCommitLink( LINK( this, ScDocCfg, CalcCommitHdl ) );

    SetDate( nDateDay, nDateMonth, nDateYear );

    aNames = GetLayoutPropertyNames();
    aValues = aLayoutItem.GetProperties(aNames);
    aLayoutItem.EnableNotification(aNames);
    pValues = aValues.getConstArray();
    OSL_ENSURE(aValues.getLength() == aNames.getLength(), "GetProperties failed");
    if(aValues.getLength() == aNames.getLength())
    {
        for(int nProp = 0; nProp < aNames.getLength(); nProp++)
        {
            OSL_ENSURE(pValues[nProp].hasValue(), "property value missing");
            if(pValues[nProp].hasValue())
            {
                switch(nProp)
                {
                    case SCDOCLAYOUTOPT_TABSTOP:
                        // TabDistance in ScDocOptions is in twips
                        if (pValues[nProp] >>= nIntVal)
                            SetTabDistance(o3tl::toTwips(nIntVal, o3tl::Length::mm100));
                        break;
                }
            }
        }
    }
    aLayoutItem.SetCommitLink( LINK( this, ScDocCfg, LayoutCommitHdl ) );
}

IMPL_LINK_NOARG(ScDocCfg, CalcCommitHdl, ScLinkConfigItem&, void)
{
    Sequence<OUString> aNames = GetCalcPropertyNames();
    Sequence<Any> aValues(aNames.getLength());
    Any* pValues = aValues.getArray();

    sal_uInt16 nDateDay, nDateMonth;
    sal_Int16 nDateYear;
    GetDate( nDateDay, nDateMonth, nDateYear );

    for(int nProp = 0; nProp < aNames.getLength(); nProp++)
    {
        switch(nProp)
        {
            case SCCALCOPT_ITER_ITER:
                pValues[nProp] <<= IsIter();
                break;
            case SCCALCOPT_ITER_STEPS:
                pValues[nProp] <<= static_cast<sal_Int32>(GetIterCount());
                break;
            case SCCALCOPT_ITER_MINCHG:
                pValues[nProp] <<= GetIterEps();
                break;
            case SCCALCOPT_DATE_DAY:
                pValues[nProp] <<= static_cast<sal_Int32>(nDateDay);
                break;
            case SCCALCOPT_DATE_MONTH:
                pValues[nProp] <<= static_cast<sal_Int32>(nDateMonth);
                break;
            case SCCALCOPT_DATE_YEAR:
                pValues[nProp] <<= static_cast<sal_Int32>(nDateYear);
                break;
            case SCCALCOPT_DECIMALS:
                pValues[nProp] <<= static_cast<sal_Int32>(GetStdPrecision());
                break;
            case SCCALCOPT_CASESENSITIVE:
                // content is reversed
                pValues[nProp] <<= !IsIgnoreCase();
                break;
            case SCCALCOPT_PRECISION:
                pValues[nProp] <<= IsCalcAsShown();
                break;
            case SCCALCOPT_SEARCHCRIT:
                pValues[nProp] <<= IsMatchWholeCell();
                break;
            case SCCALCOPT_FINDLABEL:
                pValues[nProp] <<= IsLookUpColRowNames();
                break;
            case SCCALCOPT_REGEX :
                pValues[nProp] <<= IsFormulaRegexEnabled();
                break;
            case SCCALCOPT_WILDCARDS :
                pValues[nProp] <<= IsFormulaWildcardsEnabled();
                break;
        }
    }
    aCalcItem.PutProperties(aNames, aValues);
}

IMPL_LINK_NOARG(ScDocCfg, LayoutCommitHdl, ScLinkConfigItem&, void)
{
    Sequence<OUString> aNames = GetLayoutPropertyNames();
    Sequence<Any> aValues(aNames.getLength());
    Any* pValues = aValues.getArray();

    for(int nProp = 0; nProp < aNames.getLength(); nProp++)
    {
        switch(nProp)
        {
            case SCDOCLAYOUTOPT_TABSTOP:
                //  TabDistance in ScDocOptions is in twips
                //  use only even numbers, so defaults don't get changed
                //  by modifying other settings in the same config item
                pValues[nProp] <<= static_cast<sal_Int32>(TwipsToEvenHMM( GetTabDistance() ));
                break;
        }
    }
    aLayoutItem.PutProperties(aNames, aValues);
}

void ScDocCfg::SetOptions( const ScDocOptions& rNew )
{
    *static_cast<ScDocOptions*>(this) = rNew;

    aCalcItem.SetModified();
    aLayoutItem.SetModified();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
