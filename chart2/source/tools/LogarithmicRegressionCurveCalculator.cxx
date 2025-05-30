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

#include <LogarithmicRegressionCurveCalculator.hxx>
#include <RegressionCalculationHelper.hxx>
#include <SpecialCharacters.hxx>

#include <rtl/math.hxx>
#include <rtl/ustrbuf.hxx>

using namespace ::com::sun::star;

namespace chart
{

LogarithmicRegressionCurveCalculator::LogarithmicRegressionCurveCalculator() :
        m_fSlope( std::numeric_limits<double>::quiet_NaN() ),
        m_fIntercept( std::numeric_limits<double>::quiet_NaN() )
{
}

LogarithmicRegressionCurveCalculator::~LogarithmicRegressionCurveCalculator()
{}

// ____ XRegressionCurve ____
void SAL_CALL LogarithmicRegressionCurveCalculator::recalculateRegression(
    const uno::Sequence< double >& aXValues,
    const uno::Sequence< double >& aYValues )
{
    RegressionCalculationHelper::tDoubleVectorPair aValues(
        RegressionCalculationHelper::cleanup(
            aXValues, aYValues,
            RegressionCalculationHelper::isValidAndXPositive()));

    const size_t nMax = aValues.first.size();
    if( nMax <= 1 )  // at least 2 points
    {
        m_fSlope = std::numeric_limits<double>::quiet_NaN();
        m_fIntercept = std::numeric_limits<double>::quiet_NaN();
        m_fCorrelationCoefficient = std::numeric_limits<double>::quiet_NaN();
        return;
    }

    double fAverageX = 0.0, fAverageY = 0.0;
    size_t i = 0;
    for( i = 0; i < nMax; ++i )
    {
        fAverageX += log( aValues.first[i] );
        fAverageY += aValues.second[i];
    }

    const double fN = static_cast< double >( nMax );
    fAverageX /= fN;
    fAverageY /= fN;

    double fQx = 0.0, fQy = 0.0, fQxy = 0.0;
    for( i = 0; i < nMax; ++i )
    {
        double fDeltaX = log( aValues.first[i] ) - fAverageX;
        double fDeltaY = aValues.second[i] - fAverageY;

        fQx  += fDeltaX * fDeltaX;
        fQy  += fDeltaY * fDeltaY;
        fQxy += fDeltaX * fDeltaY;
    }

    m_fSlope = fQxy / fQx;
    m_fIntercept = fAverageY - m_fSlope * fAverageX;
    m_fCorrelationCoefficient = fQxy / sqrt( fQx * fQy );
}

double SAL_CALL LogarithmicRegressionCurveCalculator::getCurveValue( double x )
{
    if( ! ( std::isnan( m_fSlope ) ||
            std::isnan( m_fIntercept )))
    {
        return m_fSlope * log( x ) + m_fIntercept;
    }

    return std::numeric_limits<double>::quiet_NaN();
}

uno::Sequence< geometry::RealPoint2D > SAL_CALL LogarithmicRegressionCurveCalculator::getCurveValues(
    double min, double max, ::sal_Int32 nPointCount,
    const uno::Reference< chart2::XScaling >& xScalingX,
    const uno::Reference< chart2::XScaling >& xScalingY,
    sal_Bool bMaySkipPointsInCalculation )
{
    if( bMaySkipPointsInCalculation &&
        isLogarithmicScaling( xScalingX ) &&
        isLinearScaling( xScalingY ))
    {
        // optimize result
        uno::Sequence< geometry::RealPoint2D > aResult{ { min, getCurveValue( min ) },
                                                        { max, getCurveValue( max ) } };

        return aResult;
    }
    return RegressionCurveCalculator::getCurveValues( min, max, nPointCount, xScalingX, xScalingY, bMaySkipPointsInCalculation );
}

OUString LogarithmicRegressionCurveCalculator::ImplGetRepresentation(
    const uno::Reference< util::XNumberFormatter >& xNumFormatter,
    sal_Int32 nNumberFormatKey, sal_Int32* pFormulaMaxWidth /* = nullptr */ ) const
{
    bool bHasSlope = !rtl::math::approxEqual( fabs( m_fSlope ), 1.0 );
    OUStringBuffer aBuf( mYName + " = " );
    sal_Int32 nLineLength = aBuf.getLength();
    sal_Int32 nValueLength=0;
    if ( pFormulaMaxWidth && *pFormulaMaxWidth > 0 ) // count nValueLength
    {
        sal_Int32 nCharMin = nLineLength + 6 + mXName.getLength();  // 6 = "ln(x)" + 2 extra characters
        if( m_fSlope < 0.0 )
            nCharMin += 2;  // "- "
        if( m_fSlope != 0.0 && m_fIntercept != 0.0 )
        {
            nCharMin += 3; // " + "
            if ( bHasSlope )
                nValueLength = (*pFormulaMaxWidth - nCharMin) / 2;
        }
        if ( nValueLength == 0 ) // not yet calculated
            nValueLength = *pFormulaMaxWidth - nCharMin;
        if ( nValueLength <= 0 )
            nValueLength = 1;
    }

    // temporary buffer
    OUStringBuffer aTmpBuf("");
    // if nValueLength not calculated then nullptr
    sal_Int32* pValueLength = nValueLength ? &nValueLength : nullptr;
    if( m_fSlope != 0.0 )  // add slope value
    {
        if( m_fSlope < 0.0 )
        {
            aTmpBuf.append( OUStringChar(aMinusSign) + " " );
        }
        if( bHasSlope )
        {
            OUString aValueString = getFormattedString( xNumFormatter, nNumberFormatKey, fabs(m_fSlope), pValueLength );
            if ( aValueString != "1" )  // aValueString may be rounded to 1 if nValueLength is small
            {
                aTmpBuf.append( aValueString + " " );
            }
        }
        aTmpBuf.append( "ln(" + mXName + ") " );
        addStringToEquation( aBuf, nLineLength, aTmpBuf, pFormulaMaxWidth );
        aTmpBuf.truncate();

        if( m_fIntercept > 0.0 )
            aTmpBuf.append( "+ " );
    }
             // add intercept value
    if( m_fIntercept < 0.0 )
        aTmpBuf.append( OUStringChar(aMinusSign) + " " );
    OUString aValueString = getFormattedString( xNumFormatter, nNumberFormatKey, fabs(m_fIntercept), pValueLength );
    if ( aValueString != "0" )  // aValueString may be rounded to 0 if nValueLength is small
    {
        aTmpBuf.append( aValueString );
        addStringToEquation( aBuf, nLineLength, aTmpBuf, pFormulaMaxWidth );
    }

    if ( std::u16string_view(aBuf) == Concat2View(mYName + " = ") )
        aBuf.append( "0" );

    return aBuf.makeStringAndClear();
}

} //  namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
