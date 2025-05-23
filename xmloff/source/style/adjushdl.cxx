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

#include "adjushdl.hxx"
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmluconv.hxx>
#include <xmloff/xmlement.hxx>
#include <rtl/ustrbuf.hxx>
#include <com/sun/star/style/ParagraphAdjust.hpp>
#include <com/sun/star/uno/Any.hxx>

using namespace ::com::sun::star;

using namespace ::xmloff::token;

SvXMLEnumMapEntry<style::ParagraphAdjust> const pXML_Para_Adjust_Enum[] =
{
    { XML_START,        style::ParagraphAdjust_LEFT },
    { XML_END,          style::ParagraphAdjust_RIGHT },
    { XML_CENTER,       style::ParagraphAdjust_CENTER },
    { XML_JUSTIFY,      style::ParagraphAdjust_BLOCK },
    { XML_JUSTIFIED,    style::ParagraphAdjust_BLOCK }, // obsolete
    { XML_LEFT,         style::ParagraphAdjust_LEFT },
    { XML_RIGHT,        style::ParagraphAdjust_RIGHT },
    { XML_TOKEN_INVALID, style::ParagraphAdjust(0) }
};

SvXMLEnumMapEntry<style::ParagraphAdjust> const pXML_Para_Align_Last_Enum[] =
{
    { XML_START,        style::ParagraphAdjust_LEFT },
    { XML_CENTER,       style::ParagraphAdjust_CENTER },
    { XML_JUSTIFY,      style::ParagraphAdjust_BLOCK },
    { XML_JUSTIFIED,    style::ParagraphAdjust_BLOCK }, // obsolete
    { XML_TOKEN_INVALID, style::ParagraphAdjust(0) }
};




XMLParaAdjustPropHdl::~XMLParaAdjustPropHdl()
{
    // nothing to do
}

bool XMLParaAdjustPropHdl::importXML( const OUString& rStrImpValue, uno::Any& rValue, const SvXMLUnitConverter& ) const
{
    style::ParagraphAdjust eAdjust;
    bool bRet = SvXMLUnitConverter::convertEnum( eAdjust, rStrImpValue, pXML_Para_Adjust_Enum );
    if( bRet )
        rValue <<= static_cast<sal_Int16>(eAdjust);

    return bRet;
}

bool XMLParaAdjustPropHdl::exportXML( OUString& rStrExpValue, const uno::Any& rValue, const SvXMLUnitConverter& ) const
{
    if(!rValue.hasValue())
        return false;
    OUStringBuffer aOut;
    sal_Int16 nVal = 0;

    rValue >>= nVal;

    bool bRet = SvXMLUnitConverter::convertEnum( aOut, static_cast<style::ParagraphAdjust>(nVal), pXML_Para_Adjust_Enum, XML_START );

    rStrExpValue = aOut.makeStringAndClear();

    return bRet;
}




XMLLastLineAdjustPropHdl::~XMLLastLineAdjustPropHdl()
{
    // nothing to do
}

bool XMLLastLineAdjustPropHdl::importXML( const OUString& rStrImpValue, uno::Any& rValue, const SvXMLUnitConverter& ) const
{
    style::ParagraphAdjust eAdjust;
    bool bRet = SvXMLUnitConverter::convertEnum( eAdjust, rStrImpValue, pXML_Para_Align_Last_Enum );
    if( bRet )
        rValue <<= static_cast<sal_Int16>(eAdjust);

    return bRet;
}

bool XMLLastLineAdjustPropHdl::exportXML( OUString& rStrExpValue, const uno::Any& rValue, const SvXMLUnitConverter& ) const
{
    OUStringBuffer aOut;
    sal_Int16 nVal = 0;
    bool bRet = false;

    rValue >>= nVal;

    if( static_cast<style::ParagraphAdjust>(nVal) != style::ParagraphAdjust_LEFT )
        bRet = SvXMLUnitConverter::convertEnum( aOut, static_cast<style::ParagraphAdjust>(nVal), pXML_Para_Align_Last_Enum, XML_START );

    rStrExpValue = aOut.makeStringAndClear();

    return bRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
