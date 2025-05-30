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

#include <xmlbahdl.hxx>

#include <XMLNumberWithAutoForVoidPropHdl.hxx>
#include <sal/log.hxx>
#include <o3tl/any.hxx>
#include <o3tl/safeint.hxx>
#include <o3tl/string_view.hxx>
#include <sax/tools/converter.hxx>
#include <xmloff/xmluconv.hxx>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/beans/Pair.hpp>
#include <xmloff/xmltoken.hxx>

#include <limits.h>

using namespace ::com::sun::star::uno;
using namespace ::xmloff::token;

static void lcl_xmloff_setAny( Any& rValue, sal_Int32 nValue, sal_Int8 nBytes )
{
    switch( nBytes )
    {
    case 1:
        if( nValue < SCHAR_MIN )
            nValue = SCHAR_MIN;
        else if( nValue > SCHAR_MAX )
            nValue = SCHAR_MAX;
        rValue <<= static_cast<sal_Int8>(nValue);
        break;
    case 2:
        if( nValue < SHRT_MIN )
            nValue = SHRT_MIN;
        else if( nValue > SHRT_MAX )
            nValue = SHRT_MAX;
        rValue <<= static_cast<sal_Int16>(nValue);
        break;
    case 4:
        rValue <<= nValue;
        break;
    }
}

static bool lcl_xmloff_getAny( const Any& rValue, sal_Int32& nValue,
                            sal_Int8 nBytes )
{
    bool bRet = false;

    switch( nBytes )
    {
    case 1:
        {
            sal_Int8 nValue8 = 0;
            bRet = rValue >>= nValue8;
            nValue = nValue8;
        }
        break;
    case 2:
        {
            sal_Int16 nValue16 = 0;
            bRet = rValue >>= nValue16;
            nValue = nValue16;
        }
        break;
    case 4:
        bRet = rValue >>= nValue;
        break;
    }

    return bRet;
}


XMLNumberPropHdl::~XMLNumberPropHdl()
{
    // nothing to do
}

bool XMLNumberPropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    sal_Int32 nValue = 0;
    bool bRet = ::sax::Converter::convertNumber( nValue, rStrImpValue );
    lcl_xmloff_setAny( rValue, nValue, nBytes );

    return bRet;
}

bool XMLNumberPropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;
    sal_Int32 nValue;

    if( lcl_xmloff_getAny( rValue, nValue, nBytes ) )
    {
        rStrExpValue = OUString::number( nValue );

        bRet = true;
    }

    return bRet;
}


XMLNumberNonePropHdl::XMLNumberNonePropHdl( sal_Int8 nB ) :
    sZeroStr( GetXMLToken(XML_NO_LIMIT) ),
    nBytes( nB )
{
}

XMLNumberNonePropHdl::XMLNumberNonePropHdl( enum XMLTokenEnum eZeroString, sal_Int8 nB ) :
    sZeroStr( GetXMLToken( eZeroString ) ),
    nBytes( nB )
{
}

XMLNumberNonePropHdl::~XMLNumberNonePropHdl()
{
    // nothing to do
}

bool XMLNumberNonePropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;

    sal_Int32 nValue = 0;
    if( rStrImpValue == sZeroStr )
    {
        bRet = true;
    }
    else
    {
        bRet = ::sax::Converter::convertNumber( nValue, rStrImpValue );
    }
    lcl_xmloff_setAny( rValue, nValue, nBytes );

    return bRet;
}

bool XMLNumberNonePropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;
    sal_Int32 nValue;

    if( lcl_xmloff_getAny( rValue, nValue, nBytes ) )
    {
        if( nValue == 0 )
        {
            rStrExpValue = sZeroStr;
        }
        else
        {
            rStrExpValue = OUString::number( nValue );
        }

        bRet = true;
    }

    return bRet;
}


XMLMeasurePropHdl::~XMLMeasurePropHdl()
{
    // nothing to do
}

bool XMLMeasurePropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& rUnitConverter ) const
{
    sal_Int32 nValue = 0;
    bool bRet = rUnitConverter.convertMeasureToCore( nValue, rStrImpValue );
    lcl_xmloff_setAny( rValue, nValue, nBytes );
    return bRet;
}

bool XMLMeasurePropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& rUnitConverter ) const
{
    bool bRet = false;
    sal_Int32 nValue;

    if( lcl_xmloff_getAny( rValue, nValue, nBytes ) )
    {
        OUStringBuffer aOut;
        rUnitConverter.convertMeasureToXML( aOut, nValue );
        rStrExpValue = aOut.makeStringAndClear();

        bRet = true;
    }

    return bRet;
}


bool XMLUnitMeasurePropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    double fValue = 0.0;
    std::optional<sal_Int16> nValueUnit;

    auto bRet = ::sax::Converter::convertMeasureUnit( fValue, nValueUnit, rStrImpValue );

    if(bRet)
    {
        // This importer may only accept font-relative units.
        // Discard all other units to allow fall-through to other attributes.
        if (css::util::MeasureUnit::FONT_EM != nValueUnit
            && css::util::MeasureUnit::FONT_CJK_ADVANCE != nValueUnit)
        {
            return false;
        }

        css::beans::Pair<double, sal_Int16> stValue{fValue, nValueUnit.value()};
        rValue <<= stValue;
    }

    return bRet;
}

bool XMLUnitMeasurePropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;
    css::beans::Pair<double, sal_Int16> stValue{0.0, css::util::MeasureUnit::MM_100TH};

    if( rValue >>= stValue )
    {
        auto [fValue, nValueUnit] = stValue;

        // This exporter may only produce font-relative units.
        // Discard all other units to allow fall-through to other attributes.
        if (css::util::MeasureUnit::FONT_EM != nValueUnit
            && css::util::MeasureUnit::FONT_CJK_ADVANCE != nValueUnit)
        {
            return false;
        }

        OUStringBuffer aOut;
        ::sax::Converter::convertMeasureUnit( aOut, fValue, nValueUnit );
        rStrExpValue = aOut.makeStringAndClear();

        bRet = true;
    }

    return bRet;
}


XMLBoolFalsePropHdl::~XMLBoolFalsePropHdl()
{
    // nothing to do
}

bool XMLBoolFalsePropHdl::importXML( const OUString&, Any&, const SvXMLUnitConverter& ) const
{
    return false;
}

bool XMLBoolFalsePropHdl::exportXML( OUString& rStrExpValue, const Any& /*rValue*/, const SvXMLUnitConverter& rCnv) const
{
    return XMLBoolPropHdl::exportXML( rStrExpValue, Any( false ), rCnv );
}


XMLBoolPropHdl::~XMLBoolPropHdl()
{
    // nothing to do
}

bool XMLBoolPropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bValue(false);
    bool const bRet = ::sax::Converter::convertBool( bValue, rStrImpValue );
    rValue <<= bValue;

    return bRet;
}

bool XMLBoolPropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;
    bool bValue;

    if (rValue >>= bValue)
    {
        OUStringBuffer aOut;
        ::sax::Converter::convertBool( aOut, bValue );
        rStrExpValue = aOut.makeStringAndClear();

        bRet = true;
    }

    return bRet;
}


XMLNBoolPropHdl::~XMLNBoolPropHdl()
{
    // nothing to do
}

bool XMLNBoolPropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bValue(false);
    bool const bRet = ::sax::Converter::convertBool( bValue, rStrImpValue );
    rValue <<= !bValue;

    return bRet;
}

bool XMLNBoolPropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;
    bool bValue;

    if (rValue >>= bValue)
    {
        OUStringBuffer aOut;
        ::sax::Converter::convertBool( aOut, !bValue );
        rStrExpValue = aOut.makeStringAndClear();

        bRet = true;
    }

    return bRet;
}


XMLPercentPropHdl::~XMLPercentPropHdl()
{
    // nothing to do
}

bool XMLPercentPropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    sal_Int32 nValue = 0;
    bool const bRet = ::sax::Converter::convertPercent( nValue, rStrImpValue );
    lcl_xmloff_setAny( rValue, nValue, nBytes );

    return bRet;
}

bool XMLPercentPropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;
    sal_Int32 nValue;

    if( lcl_xmloff_getAny( rValue, nValue, nBytes ) )
    {
        OUStringBuffer aOut;
        ::sax::Converter::convertPercent( aOut, nValue );
        rStrExpValue = aOut.makeStringAndClear();

        bRet = true;
    }

    return bRet;
}


bool XMLDoublePercentPropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;

    double fValue = 1.0;

    if( rStrImpValue.indexOf( '%' ) == -1 )
    {
        fValue = rStrImpValue.toDouble();
    }
    else
    {
        sal_Int32 nValue = 0;
        bRet = ::sax::Converter::convertPercent( nValue, rStrImpValue );
        fValue = static_cast<double>(nValue) / 100.0;
    }
    rValue <<= fValue;

    return bRet;
}

bool XMLDoublePercentPropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;
    double fValue = 0;

    if( rValue >>= fValue )
    {
        fValue *= 100.0;
        if( fValue > 0 ) fValue += 0.5; else    fValue -= 0.5;

        sal_Int32 nValue = static_cast<sal_Int32>(fValue);

        OUStringBuffer aOut;
        ::sax::Converter::convertPercent( aOut, nValue );
        rStrExpValue = aOut.makeStringAndClear();

        bRet = true;
    }

    return bRet;
}

bool XML100thPercentPropHdl::importXML(const OUString& rStrImpValue, Any& rValue,
                                       const SvXMLUnitConverter&) const
{
    bool bRet = false;

    sal_Int32 nValue = 0;
    bRet = sax::Converter::convertPercent(nValue, rStrImpValue);
    rValue <<= static_cast<sal_Int16>(nValue * 100);

    return bRet;
}

bool XML100thPercentPropHdl::exportXML(OUString& rStrExpValue, const Any& rValue,
                                       const SvXMLUnitConverter&) const
{
    bool bRet = false;
    sal_Int16 nValue = 0;

    if (rValue >>= nValue)
    {
        nValue = std::round(static_cast<double>(nValue) / 100);
        OUStringBuffer aOut;
        sax::Converter::convertPercent(aOut, nValue);
        rStrExpValue = aOut.makeStringAndClear();
        bRet = true;
    }

    return bRet;
}


XMLNegPercentPropHdl::~XMLNegPercentPropHdl()
{
    // nothing to do
}

bool XMLNegPercentPropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    sal_Int32 nValue = 0;
    bool bRet = ::sax::Converter::convertPercent( nValue, rStrImpValue );
    if (bRet)
        bRet = !o3tl::checked_sub<sal_Int32>(100, nValue, nValue);
    if (bRet)
        lcl_xmloff_setAny( rValue, nValue, nBytes );
    return bRet;
}

bool XMLNegPercentPropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;
    sal_Int32 nValue;

    if( lcl_xmloff_getAny( rValue, nValue, nBytes ) )
    {
        OUStringBuffer aOut;
        ::sax::Converter::convertPercent( aOut, 100-nValue );
        rStrExpValue = aOut.makeStringAndClear();

        bRet = true;
    }

    return bRet;
}

XMLMeasurePxPropHdl::~XMLMeasurePxPropHdl()
{
    // nothing to do
}

bool XMLMeasurePxPropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    sal_Int32 nValue = 0;
    bool bRet = ::sax::Converter::convertMeasurePx( nValue, rStrImpValue );
    lcl_xmloff_setAny( rValue, nValue, nBytes );
    return bRet;
}

bool XMLMeasurePxPropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;
    sal_Int32 nValue;

    if( lcl_xmloff_getAny( rValue, nValue, nBytes ) )
    {
        OUStringBuffer aOut;
        ::sax::Converter::convertMeasurePx( aOut, nValue );
        rStrExpValue = aOut.makeStringAndClear();

        bRet = true;
    }

    return bRet;
}


XMLColorPropHdl::~XMLColorPropHdl()
{
    // Nothing to do
}

bool XMLColorPropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;

    if( rStrImpValue.matchIgnoreAsciiCase( "hsl" ) )
    {
        sal_Int32 nOpen = rStrImpValue.indexOf( '(' );
        sal_Int32 nClose = rStrImpValue.lastIndexOf( ')' );

        if( (nOpen != -1) && (nClose > nOpen) )
        {
            const std::u16string_view aTmp( rStrImpValue.subView( nOpen+1, nClose - nOpen-1) );

            sal_Int32 nIndex = 0;

            Sequence< double > aHSL
            {
                o3tl::toDouble(o3tl::getToken(aTmp, 0, ',', nIndex )),
                o3tl::toDouble(o3tl::getToken(aTmp, 0, ',', nIndex )) / 100.0,
                o3tl::toDouble(o3tl::getToken(aTmp, 0, ',', nIndex )) / 100.0
            };
            rValue <<= aHSL;
            bRet = true;
        }
    }
    else
    {
        sal_Int32 nColor(0);
        bRet = ::sax::Converter::convertColor( nColor, rStrImpValue );
        rValue <<= nColor;
    }

    return bRet;
}

bool XMLColorPropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;
    sal_Int32 nColor = 0;

    OUStringBuffer aOut;
    if( rValue >>= nColor )
    {
        ::sax::Converter::convertColor( aOut, nColor );
        rStrExpValue = aOut.makeStringAndClear();

        bRet = true;
    }
    else
    {
        Sequence< double > aHSL;
        if( (rValue >>= aHSL) && (aHSL.getLength() == 3) )
        {
            rStrExpValue = "hsl(" + OUString::number(aHSL[0]) + "," +
                    OUString::number(aHSL[1] * 100.0) +  "%," +
                    OUString::number(aHSL[2] * 100.0) + "%)";

            bRet = true;
        }
    }

    return bRet;
}


XMLHexPropHdl::~XMLHexPropHdl()
{
    // Nothing to do
}

bool XMLHexPropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    sal_uInt32 nRsid;
    bool bRet = SvXMLUnitConverter::convertHex( nRsid, rStrImpValue );
    rValue <<= nRsid;

    return bRet;
}

bool XMLHexPropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;
    sal_uInt32 nRsid = 0;

    if( rValue >>= nRsid )
    {
        OUStringBuffer aOut;
        SvXMLUnitConverter::convertHex( aOut, nRsid );
        rStrExpValue = aOut.makeStringAndClear();

        bRet = true;
    }
    else
    {
        bRet = false;
    }

    return bRet;
}


XMLStringPropHdl::~XMLStringPropHdl()
{
    // Nothing to do
}

bool XMLStringPropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    rValue <<= rStrImpValue;
    return true;
}

bool XMLStringPropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;

    if( rValue >>= rStrExpValue )
        bRet = true;

    return bRet;
}


XMLStyleNamePropHdl::~XMLStyleNamePropHdl()
{
    // Nothing to do
}

bool XMLStyleNamePropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& rUnitConverter ) const
{
    bool bRet = false;

    if( rValue >>= rStrExpValue )
    {
        rStrExpValue = rUnitConverter.encodeStyleName( rStrExpValue );
        bRet = true;
    }

    return bRet;
}


XMLDoublePropHdl::~XMLDoublePropHdl()
{
    // Nothing to do
}

bool XMLDoublePropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    double fDblValue(0.0);
    bool const bRet = ::sax::Converter::convertDouble(fDblValue, rStrImpValue);
    rValue <<= fDblValue;
    return bRet;
}

bool XMLDoublePropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;

    double fValue = 0;

    if( rValue >>= fValue )
    {
        OUStringBuffer aOut;
        ::sax::Converter::convertDouble( aOut, fValue );
        rStrExpValue = aOut.makeStringAndClear();
        bRet = true;
    }

    return bRet;
}


XMLColorTransparentPropHdl::XMLColorTransparentPropHdl(
    enum XMLTokenEnum eTransparent ) :
    sTransparent( GetXMLToken(
        eTransparent != XML_TOKEN_INVALID ? eTransparent : XML_TRANSPARENT ) )
{
    // Nothing to do
}

XMLColorTransparentPropHdl::~XMLColorTransparentPropHdl()
{
    // Nothing to do
}

bool XMLColorTransparentPropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;

    if( rStrImpValue != sTransparent )
    {
        sal_Int32 nColor(0);
        bRet = ::sax::Converter::convertColor( nColor, rStrImpValue );
        rValue <<= nColor;
    }

    return bRet;
}

bool XMLColorTransparentPropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;
    sal_Int32 nColor = 0;

    if( rStrExpValue == sTransparent )
        bRet = false;
    else if( rValue >>= nColor )
    {
        OUStringBuffer aOut;
        ::sax::Converter::convertColor( aOut, nColor );
        rStrExpValue = aOut.makeStringAndClear();

        bRet = true;
    }

    return bRet;
}


XMLIsTransparentPropHdl::XMLIsTransparentPropHdl(
    enum XMLTokenEnum eTransparent, bool bTransPropVal ) :
    sTransparent( GetXMLToken(
        eTransparent != XML_TOKEN_INVALID ? eTransparent : XML_TRANSPARENT ) ),
    bTransPropValue( bTransPropVal )
{
}

XMLIsTransparentPropHdl::~XMLIsTransparentPropHdl()
{
    // Nothing to do
}

bool XMLIsTransparentPropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bValue = ( (rStrImpValue == sTransparent) == bTransPropValue);
    rValue <<= bValue;

    return true;
}

bool XMLIsTransparentPropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;

    // MIB: This looks a bit strange, because bTransPropValue == bValue should
    // do the same, but this only applies if 'true' is represented by the same
    // 8 bit value in bValue and bTransPropValue. Who will ensure this?
    bool bValue = *o3tl::doAccess<bool>(rValue);
    bool bIsTrans = bTransPropValue ? bValue : !bValue;

    if( bIsTrans )
    {
        rStrExpValue = sTransparent;
        bRet = true;
    }

    return bRet;
}


XMLColorAutoPropHdl::XMLColorAutoPropHdl()
{
    // Nothing to do
}

XMLColorAutoPropHdl::~XMLColorAutoPropHdl()
{
    // Nothing to do
}

bool XMLColorAutoPropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;

    // This is a multi property: the value might be set to AUTO_COLOR
    // already by the XMLIsAutoColorPropHdl!
    sal_Int32 nColor = 0;
    if( !(rValue >>= nColor) || -1 != nColor )
    {
        bRet = ::sax::Converter::convertColor( nColor, rStrImpValue );
        if( bRet )
            rValue <<= nColor;
    }

    return bRet;
}

bool XMLColorAutoPropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;

    sal_Int32 nColor = 0;
    if( (rValue >>= nColor) && -1 != nColor )
    {
        OUStringBuffer aOut;
        ::sax::Converter::convertColor( aOut, nColor );
        rStrExpValue = aOut.makeStringAndClear();

        bRet = true;
    }

    return bRet;
}


XMLIsAutoColorPropHdl::XMLIsAutoColorPropHdl()
{
}

XMLIsAutoColorPropHdl::~XMLIsAutoColorPropHdl()
{
    // Nothing to do
}

bool XMLIsAutoColorPropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& ) const
{
    // An auto color overrides any other color set!
    bool bValue;
    bool const bRet = ::sax::Converter::convertBool( bValue, rStrImpValue );
    if( bRet && bValue )
        rValue <<= sal_Int32(-1);

    return true;
}

bool XMLIsAutoColorPropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{
    bool bRet = false;
    sal_Int32 nColor = 0;

    if( (rValue >>= nColor) && -1 == nColor )
    {
        OUStringBuffer aOut;
        ::sax::Converter::convertBool( aOut, true );
        rStrExpValue = aOut.makeStringAndClear();

        bRet = true;
    }

    return bRet;
}


XMLCompareOnlyPropHdl::~XMLCompareOnlyPropHdl()
{
    // Nothing to do
}

bool XMLCompareOnlyPropHdl::importXML( const OUString&, Any&, const SvXMLUnitConverter& ) const
{
    SAL_WARN( "xmloff", "importXML called for compare-only-property" );
    return false;
}

bool XMLCompareOnlyPropHdl::exportXML( OUString&, const Any&, const SvXMLUnitConverter& ) const
{
    SAL_WARN( "xmloff", "exportXML called for compare-only-property" );
    return false;
}


XMLNumberWithoutZeroPropHdl::XMLNumberWithoutZeroPropHdl( sal_Int8 nB ) :
    nBytes( nB )
{
}

XMLNumberWithoutZeroPropHdl::~XMLNumberWithoutZeroPropHdl()
{
}

bool XMLNumberWithoutZeroPropHdl::importXML(
    const OUString& rStrImpValue,
    Any& rValue,
    const SvXMLUnitConverter& ) const
{
    sal_Int32 nValue = 0;
    bool const bRet = ::sax::Converter::convertNumber( nValue, rStrImpValue );
    if( bRet )
        lcl_xmloff_setAny( rValue, nValue, nBytes );
    return bRet;
}

bool XMLNumberWithoutZeroPropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& ) const
{

    sal_Int32 nValue = 0;
    bool bRet = lcl_xmloff_getAny( rValue, nValue, nBytes );
    bRet &= nValue != 0;

    if( bRet )
    {
        rStrExpValue = OUString::number(nValue);
    }

    return bRet;
}


XMLNumberWithAutoForVoidPropHdl::~XMLNumberWithAutoForVoidPropHdl()
{
}

bool XMLNumberWithAutoForVoidPropHdl::importXML(
    const OUString& rStrImpValue,
    Any& rValue,
    const SvXMLUnitConverter& ) const
{
    sal_Int32 nValue = 0;
    bool bRet = ::sax::Converter::convertNumber( nValue, rStrImpValue );
    if( bRet )
        lcl_xmloff_setAny( rValue, nValue, 2 );
    else if( rStrImpValue == GetXMLToken( XML_AUTO ) )
    {
        rValue.clear(); // void
        bRet = true;
    }
    return bRet;
}

bool XMLNumberWithAutoForVoidPropHdl::exportXML(
        OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter&) const
{

    sal_Int32 nValue = 0;
    bool bRet = lcl_xmloff_getAny( rValue, nValue, 2 );

    // note: 0 is a valid value here, see CTF_PAGENUMBEROFFSET for when it isn't

    if (!bRet)
        rStrExpValue = GetXMLToken( XML_AUTO );
    else
    {
        rStrExpValue = OUString::number(nValue);
    }

    return true;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
