/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "charttest.hxx"
#include <com/sun/star/chart2/XRegressionCurveContainer.hpp>
#include <com/sun/star/chart2/XRegressionCurveCalculator.hpp>
#include <SpecialCharacters.hxx>


// Define the index of sheets in the test document
constexpr sal_Int32 SHEET_POTENTIAL1 = 0;
constexpr sal_Int32 SHEET_POTENTIAL2 = 1;
constexpr sal_Int32 SHEET_LINEAR1 = 2;
constexpr sal_Int32 SHEET_POLYNOMIAL1 = 3;
constexpr sal_Int32 SHEET_EXPONENTIAL1 = 4;
constexpr sal_Int32 SHEET_EXPONENTIAL2 = 5;

class Chart2TrendCalculators : public ChartTest
{
public:
    Chart2TrendCalculators()
        : ChartTest(u"/chart2/qa/extras/data/"_ustr)
    {}

    void setUp() override;
    void tearDown() override;

    void testPotentialRegression1();
    void testPotentialRegression2();
    void testLinearRegression1();
    void testPolynomialRegression1();
    void testExponentialRegression1();
    void testExponentialRegression2();

    CPPUNIT_TEST_SUITE(Chart2TrendCalculators);
    CPPUNIT_TEST(testPotentialRegression1);
    CPPUNIT_TEST(testPotentialRegression2);
    CPPUNIT_TEST(testLinearRegression1);
    CPPUNIT_TEST(testPolynomialRegression1);
    CPPUNIT_TEST(testExponentialRegression1);
    CPPUNIT_TEST(testExponentialRegression2);
    CPPUNIT_TEST_SUITE_END();

private:
    Reference<chart2::XRegressionCurve> m_xCurve;
    Reference< chart2::XRegressionCurveCalculator > m_xRegressionCurveCalculator;

    void loadCalculatorFromSheet(sal_Int32 nSheet);
    void checkCalculator(
        const Sequence< double >& xValues, const Sequence< double >& yValues,
        const OUString& sExpectedFormula );
};

void Chart2TrendCalculators::setUp()
{
    ChartTest::setUp();
    loadFromFile(u"ods/trend_calculators.ods");
}

void Chart2TrendCalculators::tearDown()
{
    m_xRegressionCurveCalculator.clear();
    m_xCurve.clear();
    ChartTest::tearDown();
}

void Chart2TrendCalculators::loadCalculatorFromSheet(sal_Int32 nSheet)
{
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(nSheet);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference<chart2::XDataSeries> xDataSeries = getDataSeriesFromDoc(xChartDoc, 0);
    CPPUNIT_ASSERT(xDataSeries.is());

    Reference<chart2::XRegressionCurveContainer> xRegressionCurveContainer(xDataSeries, UNO_QUERY_THROW);

    Sequence< Reference< chart2::XRegressionCurve > > xRegressionCurveSequence = xRegressionCurveContainer->getRegressionCurves();
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1), xRegressionCurveSequence.getLength());

    m_xCurve = xRegressionCurveSequence[0];
    CPPUNIT_ASSERT(m_xCurve.is());

    m_xRegressionCurveCalculator = m_xCurve->getCalculator();
    CPPUNIT_ASSERT(m_xRegressionCurveCalculator.is());
}

void Chart2TrendCalculators::checkCalculator(
    const Sequence< double >& xValues, const Sequence< double >& yValues,
    const OUString& sExpectedFormula )
{
    m_xRegressionCurveCalculator->recalculateRegression( xValues, yValues );
    OUString aRepresentation = m_xRegressionCurveCalculator->getRepresentation ();
    CPPUNIT_ASSERT_EQUAL( sExpectedFormula, aRepresentation );
    double r2 = m_xRegressionCurveCalculator->getCorrelationCoefficient();
    CPPUNIT_ASSERT_DOUBLES_EQUAL( 1.0, r2, 1e-8 );
}

// test y = A x ^ B
void Chart2TrendCalculators::testPotentialRegression1()
{
    loadCalculatorFromSheet( SHEET_POTENTIAL1 );
    m_xRegressionCurveCalculator->setRegressionProperties( 0, false, 0, 0, 0 );
    Sequence< double > xValues( 7 );
    auto pxValues = xValues.getArray();
    Sequence< double > yValues( 7 );
    auto pyValues = yValues.getArray();
    for (int i=0; i<7; i++)
    {
        const double d = static_cast<double>(i);
        pxValues[i] = d;
        pyValues[i] = 2.0 * pow ( d, 3 );
    }
    checkCalculator( xValues, yValues, u"f(x) = 2 x^3"_ustr);
}

// test y = A x ^ B
void Chart2TrendCalculators::testPotentialRegression2()
{
    loadCalculatorFromSheet( SHEET_POTENTIAL2 );
    m_xRegressionCurveCalculator->setRegressionProperties( 0, false, 0, 0, 0 );
    Sequence< double > xValues( 7 );
    auto pxValues = xValues.getArray();
    Sequence< double > yValues( 7 );
    auto pyValues = yValues.getArray();
    for (int i=0; i<7; i++)
    {
        const double d = static_cast<double>(i);
        pxValues[i] = d;
        pyValues[i] = -2.0 * pow ( d, 3 );
    }
    checkCalculator( xValues, yValues, "f(x) = "+ OUStringChar(aMinusSign) +" 2 x^3");
}

// test y = - 2 X - 5
void Chart2TrendCalculators::testLinearRegression1()
{
    loadCalculatorFromSheet( SHEET_LINEAR1 );
    m_xRegressionCurveCalculator->setRegressionProperties( 1, false, 0, 0, 0 );
    Sequence< double > xValues( 7 );
    auto pxValues = xValues.getArray();
    Sequence< double > yValues( 7 );
    auto pyValues = yValues.getArray();
    for (int i=0; i<7; i++)
    {
        const double d = static_cast<double>(i);
        pxValues[i] = d;
        pyValues[i] = - 2.0 * d - 5.0 ;
    }
    checkCalculator( xValues, yValues, "f(x) = "+ OUStringChar(aMinusSign) +" 2 x "+ OUStringChar(aMinusSign) +" 5");
}

// test y = A x ^ B
void Chart2TrendCalculators::testPolynomialRegression1()
{
    loadCalculatorFromSheet( SHEET_POLYNOMIAL1 );
    m_xRegressionCurveCalculator->setRegressionProperties( 2, false, 0, 0, 0 );
    Sequence< double > xValues( 7 );
    auto pxValues = xValues.getArray();
    Sequence< double > yValues( 7 );
    auto pyValues = yValues.getArray();
    for (int i=0; i<7; i++)
    {
        const double d = static_cast<double>(i);
        pxValues[i] = d;
        pyValues[i] =  - 2.0 * d * d + 4 * d - 5;
    }
    OUString sExpectedFormula( "f(x) = "+ OUStringChar(aMinusSign) +" 2 x" + OUStringChar( aSuperscriptFigures[2] ) + " + 4 x "+ OUStringChar(aMinusSign) +" 5" );
    checkCalculator( xValues, yValues, sExpectedFormula );
}

void Chart2TrendCalculators::testExponentialRegression1()
{
    loadCalculatorFromSheet( SHEET_EXPONENTIAL1 );
    m_xRegressionCurveCalculator->setRegressionProperties( 0, false, 0, 0, 0 );
    Sequence< double > xValues( 7 );
    auto pxValues = xValues.getArray();
    Sequence< double > yValues( 7 );
    auto pyValues = yValues.getArray();
    for (int i=0; i<7; i++)
    {
        const double d = static_cast<double>(i);
        pxValues[i] = d;
        pyValues[i] = 2.0 * exp ( 0.3 * d );
    }
    checkCalculator( xValues, yValues, u"f(x) = 2 exp( 0.3 x )"_ustr);
}

void Chart2TrendCalculators::testExponentialRegression2()
{
    loadCalculatorFromSheet( SHEET_EXPONENTIAL2 );
    m_xRegressionCurveCalculator->setRegressionProperties( 0, false, 0, 0, 0 );
    Sequence< double > xValues( 7 );
    auto pxValues = xValues.getArray();
    Sequence< double > yValues( 7 );
    auto pyValues = yValues.getArray();
    for (int i=0; i<7; i++)
    {
        const double d = static_cast<double>(i);
        pxValues[i] = d;
        pyValues[i] = -2.0 * exp ( 0.3 * d );
    }
    checkCalculator( xValues, yValues, "f(x) = "+ OUStringChar(aMinusSign) + " 2 exp( 0.3 x )");
}


CPPUNIT_TEST_SUITE_REGISTRATION(Chart2TrendCalculators);

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
