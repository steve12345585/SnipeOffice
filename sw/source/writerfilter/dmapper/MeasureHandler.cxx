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
#include "MeasureHandler.hxx"
#include "ConversionHelper.hxx"
#include <ooxml/resourceids.hxx>
#include <osl/diagnose.h>
#include <com/sun/star/text/SizeType.hpp>
#include <comphelper/sequence.hxx>

namespace writerfilter::dmapper {

using namespace ::com::sun::star;


MeasureHandler::MeasureHandler() :
LoggedProperties("MeasureHandler"),
m_nMeasureValue( 0 ),
m_nUnit( -1 ),
m_nRowHeightSizeType( text::SizeType::MIN )
{
}


MeasureHandler::~MeasureHandler()
{
}


void MeasureHandler::lcl_attribute(Id rName, const Value & rVal)
{
    sal_Int32 nIntValue = rVal.getInt();
    switch( rName )
    {
        case NS_ooxml::LN_CT_TblWidth_type:
        {
            //can be: NS_ooxml::LN_Value_ST_TblWidth_nil, NS_ooxml::LN_Value_ST_TblWidth_pct,
            //        NS_ooxml::LN_Value_ST_TblWidth_dxa, NS_ooxml::LN_Value_ST_TblWidth_auto;
            m_nUnit = nIntValue;

            if (!m_aInteropGrabBagName.isEmpty())
            {
                beans::PropertyValue aValue;
                aValue.Name = "type";
                switch (nIntValue)
                {
                    case NS_ooxml::LN_Value_ST_TblWidth_nil: aValue.Value <<= u"nil"_ustr; break;
                    case NS_ooxml::LN_Value_ST_TblWidth_pct: aValue.Value <<= u"pct"_ustr; break;
                    case NS_ooxml::LN_Value_ST_TblWidth_dxa: aValue.Value <<= u"dxa"_ustr; break;
                    case NS_ooxml::LN_Value_ST_TblWidth_auto: aValue.Value <<= u"auto"_ustr; break;
                }
                m_aInteropGrabBag.push_back(aValue);
            }
        }
        break;
        case NS_ooxml::LN_CT_Height_hRule:
        {
            OUString sHeightType = rVal.getString();
            if ( sHeightType == "exact" )
                m_nRowHeightSizeType = text::SizeType::FIX;
        }
        break;
        case NS_ooxml::LN_CT_TblWidth_w:
            m_nMeasureValue = nIntValue;
            if (!m_aInteropGrabBagName.isEmpty())
            {
                beans::PropertyValue aValue;
                aValue.Name = "w";
                aValue.Value <<= nIntValue;
                m_aInteropGrabBag.push_back(aValue);
            }
        break;
        case NS_ooxml::LN_CT_Height_val: // a string value
        {
            m_nUnit = NS_ooxml::LN_Value_ST_TblWidth_dxa;
            OUString sHeight = rVal.getString();
            m_nMeasureValue = sHeight.toInt32();
        }
        break;
        default:
            OSL_FAIL( "unknown attribute");
    }
}


void MeasureHandler::lcl_sprm(Sprm &) {}


sal_Int32 MeasureHandler::getMeasureValue() const
{
    sal_Int32 nRet = 0;
    if( m_nMeasureValue != 0 && m_nUnit >= 0 )
    {
        // TODO m_nUnit 3 - twip, other values unknown :-(
        if( m_nUnit == 3 || sal::static_int_cast<Id>(m_nUnit) == NS_ooxml::LN_Value_ST_TblWidth_dxa)
        {
            nRet = ConversionHelper::convertTwipToMm100_Limited(m_nMeasureValue);
        }
        //todo: handle additional width types:
        //NS_ooxml::LN_Value_ST_TblWidth_nil, NS_ooxml::LN_Value_ST_TblWidth_pct,
        //NS_ooxml::LN_Value_ST_TblWidth_dxa, NS_ooxml::LN_Value_ST_TblWidth_auto;
    }
    return nRet;
}

void MeasureHandler::enableInteropGrabBag(const OUString& aName)
{
    m_aInteropGrabBagName = aName;
}

beans::PropertyValue MeasureHandler::getInteropGrabBag()
{
    beans::PropertyValue aRet;
    aRet.Name = m_aInteropGrabBagName;
    aRet.Value <<= comphelper::containerToSequence(m_aInteropGrabBag);
    return aRet;
}

} //namespace writerfilter::dmapper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
