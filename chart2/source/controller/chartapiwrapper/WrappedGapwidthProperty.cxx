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

#include <cstddef>

#include "WrappedGapwidthProperty.hxx"
#include "Chart2ModelContact.hxx"
#include <ChartType.hxx>
#include <tools/long.hxx>
#include <utility>

using namespace ::com::sun::star;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;
using ::com::sun::star::uno::Any;

namespace chart::wrapper
{

const sal_Int32 DEFAULT_GAPWIDTH = 100;
const sal_Int32 DEFAULT_OVERLAP = 0;

WrappedBarPositionProperty_Base::WrappedBarPositionProperty_Base(
                  const OUString& rOuterName
                , OUString aInnerSequencePropertyName
                , sal_Int32 nDefaultValue
                , std::shared_ptr<Chart2ModelContact> spChart2ModelContact )
            : WrappedDefaultProperty( rOuterName, OUString(), uno::Any( nDefaultValue ) )
            , m_nDimensionIndex(0)
            , m_nAxisIndex(0)
            , m_spChart2ModelContact(std::move( spChart2ModelContact ))
            , m_nDefaultValue( nDefaultValue )
            , m_InnerSequencePropertyName(std::move( aInnerSequencePropertyName ))
{
}

void WrappedBarPositionProperty_Base::setDimensionAndAxisIndex( sal_Int32 nDimensionIndex, sal_Int32 nAxisIndex )
{
    m_nDimensionIndex = nDimensionIndex;
    m_nAxisIndex = nAxisIndex;
}

WrappedBarPositionProperty_Base::~WrappedBarPositionProperty_Base()
{
}

void WrappedBarPositionProperty_Base::setPropertyValue( const Any& rOuterValue, const Reference< beans::XPropertySet >& /*xInnerPropertySet*/ ) const
{
    sal_Int32 nNewValue = 0;
    if( ! (rOuterValue >>= nNewValue) )
        throw lang::IllegalArgumentException( u"GapWidth and Overlap property require value of type sal_Int32"_ustr, nullptr, 0 );

    m_aOuterValue = rOuterValue;

    rtl::Reference< ::chart::Diagram > xDiagram( m_spChart2ModelContact->getDiagram() );
    if( !xDiagram.is() )
        return;

    if( m_nDimensionIndex!=1 )
        return;

    const std::vector< rtl::Reference< ChartType > > aChartTypeList( xDiagram->getChartTypes() );
    for( rtl::Reference< ChartType > const & chartType : aChartTypeList )
    {
        try
        {
            Sequence< sal_Int32 > aBarPositionSequence;
            chartType->getPropertyValue( m_InnerSequencePropertyName ) >>= aBarPositionSequence;

            tools::Long nOldLength = aBarPositionSequence.getLength();
            if( nOldLength <= m_nAxisIndex  )
                aBarPositionSequence.realloc( m_nAxisIndex+1 );
            auto pBarPositionSequence = aBarPositionSequence.getArray();
            for( sal_Int32 i=nOldLength; i<m_nAxisIndex; i++ )
            {
                pBarPositionSequence[i] = m_nDefaultValue;
            }
            pBarPositionSequence[m_nAxisIndex] = nNewValue;

            chartType->setPropertyValue( m_InnerSequencePropertyName, uno::Any( aBarPositionSequence ) );
        }
        catch( uno::Exception& e )
        {
            //the above properties are not supported by all charttypes (only by column and bar)
            //in that cases this exception is ok
            e.Context.is();//to have debug information without compilation warnings
        }
    }
}

Any WrappedBarPositionProperty_Base::getPropertyValue( const Reference< beans::XPropertySet >& /*xInnerPropertySet*/ ) const
{
    rtl::Reference< ::chart::Diagram > xDiagram( m_spChart2ModelContact->getDiagram() );
    if( xDiagram.is() )
    {
        bool bInnerValueDetected = false;
        sal_Int32 nInnerValue = m_nDefaultValue;

        if( m_nDimensionIndex==1 )
        {
            std::vector< rtl::Reference< ChartType > > aChartTypeList = xDiagram->getChartTypes();
            for( std::size_t nN = 0; nN < aChartTypeList.size() && !bInnerValueDetected; nN++ )
            {
                try
                {
                    Sequence< sal_Int32 > aBarPositionSequence;
                    aChartTypeList[nN]->getPropertyValue( m_InnerSequencePropertyName ) >>= aBarPositionSequence;
                    if( m_nAxisIndex < aBarPositionSequence.getLength() )
                    {
                        nInnerValue = aBarPositionSequence[m_nAxisIndex];
                        bInnerValueDetected = true;
                    }
                }
                catch( uno::Exception& e )
                {
                    //the above properties are not supported by all charttypes (only by column and bar)
                    //in that cases this exception is ok
                    e.Context.is();//to have debug information without compilation warnings
                }
            }
        }
        if( bInnerValueDetected )
        {
            m_aOuterValue <<= nInnerValue;
        }
    }
    return m_aOuterValue;
}

WrappedGapwidthProperty::WrappedGapwidthProperty(
        const std::shared_ptr<Chart2ModelContact>& spChart2ModelContact)
    : WrappedBarPositionProperty_Base( u"GapWidth"_ustr, u"GapwidthSequence"_ustr, DEFAULT_GAPWIDTH, spChart2ModelContact )
{
}
WrappedGapwidthProperty::~WrappedGapwidthProperty()
{
}

WrappedBarOverlapProperty::WrappedBarOverlapProperty(
        const std::shared_ptr<Chart2ModelContact>& spChart2ModelContact )
    : WrappedBarPositionProperty_Base( u"Overlap"_ustr, u"OverlapSequence"_ustr, DEFAULT_OVERLAP, spChart2ModelContact )
{
}
WrappedBarOverlapProperty::~WrappedBarOverlapProperty()
{
}

} //  namespace chart::wrapper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
