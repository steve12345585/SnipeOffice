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

#include "weighhdl.hxx"

#include <sax/tools/converter.hxx>

#include <xmloff/xmltoken.hxx>

#include <rtl/ustring.hxx>

#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/awt/FontWeight.hpp>

using namespace ::com::sun::star::uno;
using namespace ::xmloff::token;

namespace {

struct FontWeightMapper
{
    float fWeight;
    sal_uInt16 nValue;
};

}

FontWeightMapper const aFontWeightMap[] =
{
    { css::awt::FontWeight::DONTKNOW,              0 },
    { css::awt::FontWeight::THIN,                  100 },
    { css::awt::FontWeight::ULTRALIGHT,            150 },
    { css::awt::FontWeight::LIGHT,                 250 },
    { css::awt::FontWeight::SEMILIGHT,             350 },
    { css::awt::FontWeight::NORMAL,                400 },
    { css::awt::FontWeight::NORMAL,                450 },
    { css::awt::FontWeight::SEMIBOLD,              600 },
    { css::awt::FontWeight::BOLD,                  700 },
    { css::awt::FontWeight::ULTRABOLD,             800 },
    { css::awt::FontWeight::BLACK,                 900 },
    { css::awt::FontWeight::DONTKNOW,             1000 }
};


XMLFontWeightPropHdl::~XMLFontWeightPropHdl()
{
    // Nothing to do
}

bool XMLFontWeightPropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;
    sal_uInt16 nWeight = 0;

    if( IsXMLToken( rStrImpValue, XML_NORMAL ) )
    {
        nWeight = 400;
        bRet = true;
    }
    else if( IsXMLToken( rStrImpValue, XML_BOLD ) )
    {
        nWeight = 700;
        bRet = true;
    }
    else
    {
        sal_Int32 nTemp;
        bRet = ::sax::Converter::convertNumber(nTemp, rStrImpValue, 100, 900);
        if( bRet )
            nWeight = sal::static_int_cast< sal_uInt16 >(nTemp);
    }

    if( bRet )
    {
        bRet = false;
        int const nCount = SAL_N_ELEMENTS(aFontWeightMap);
        for (int i = 0; i < (nCount-1); ++i)
        {
            if( (nWeight >= aFontWeightMap[i].nValue) && (nWeight <= aFontWeightMap[i+1].nValue) )
            {
                sal_uInt16 nDiff1 = nWeight - aFontWeightMap[i].nValue;
                sal_uInt16 nDiff2 = aFontWeightMap[i+1].nValue - nWeight;

                if( nDiff1 < nDiff2 )
                    rValue <<= aFontWeightMap[i].fWeight;
                else
                    rValue <<= aFontWeightMap[i+1].fWeight;

                bRet = true;
                break;
            }
        }
    }

    return bRet;
}

bool XMLFontWeightPropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;

    float fValue = float();
    if( !( rValue >>= fValue ) )
    {
        sal_Int32 nValue = 0;
        if( rValue >>= nValue )
        {
            fValue = static_cast<float>(nValue);
            bRet = true;
        }
    }
    else
        bRet = true;

    if( bRet )
    {
        sal_uInt16 nWeight = 0;
        for( auto const & pair : aFontWeightMap )
        {
            if( fValue <= pair.fWeight )
            {
                 nWeight = pair.nValue;
                 break;
            }
        }

        if( 400 == nWeight )
            rStrExpValue = GetXMLToken(XML_NORMAL);
        else if( 700 == nWeight )
            rStrExpValue = GetXMLToken(XML_BOLD);
        else
            rStrExpValue = OUString::number( nWeight );
    }

    return bRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
