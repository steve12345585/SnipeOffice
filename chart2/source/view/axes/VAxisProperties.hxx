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
#pragma once

#include "TickmarkProperties.hxx"
#include <Axis.hxx>
#include <LabelAlignment.hxx>
#include <DataTable.hxx>

#include <com/sun/star/chart/ChartAxisLabelPosition.hpp>
#include <com/sun/star/chart/ChartAxisMarkPosition.hpp>
#include <com/sun/star/chart/ChartAxisPosition.hpp>
#include <com/sun/star/awt/Rectangle.hpp>
#include <com/sun/star/awt/Size.hpp>
#include <com/sun/star/uno/Any.hxx>
#include <rtl/ref.hxx>

#include <vector>
#include <optional>

namespace chart { class ExplicitCategoriesProvider; }
namespace com::sun::star::beans { class XPropertySet; }
namespace com::sun::star::chart2 { class XAxis; }
namespace com::sun::star::chart2::data { class XTextualDataSequence; }

namespace chart
{

//These properties describe how a couple of labels are arranged one to another.
//The couple can contain all labels for all tickmark depth or just the labels for one single depth or
//the labels from a coherent range of tick depths (e.g. the major and first minor tickmarks should be handled together).
//... only allow side by side for different tick depth
enum class AxisLabelStaggering
{
      SideBySide
    , StaggerEven
    , StaggerOdd
    , StaggerAuto
};

struct AxisLabelProperties final
{
    AxisLabelProperties();

    css::awt::Size         m_aFontReferenceSize;//reference size to calculate the font height
    css::awt::Rectangle    m_aMaximumSpaceForLabels;//Labels need to be clipped in order to fit into this rectangle

    sal_Int32            m_nNumberFormatKey;

    AxisLabelStaggering  m_eStaggering;

    bool                 m_bLineBreakAllowed;
    bool                 m_bOverlapAllowed;

    bool                 m_bStackCharacters;
    double               m_fRotationAngleDegree;

    sal_Int32   m_nRhythm; //show only each nth label with n==nRhythm

    //methods:
    void init( const rtl::Reference< ::chart::Axis >&  xAxisModel );

    bool isStaggered() const;

    void autoRotate45();
};

struct AxisLabelAlignment
{
    double mfLabelDirection; /// which direction the labels are to be drawn.
    double mfInnerTickDirection; /// which direction the inner tickmarks are to be drawn.

    LabelAlignment meAlignment;

    AxisLabelAlignment();
};

struct AxisProperties final
{
    rtl::Reference<::chart::Axis> m_xAxisModel;

    sal_Int32   m_nDimensionIndex;
    bool        m_bIsMainAxis;//not secondary axis
    bool        m_bSwapXAndY;

    css::chart::ChartAxisPosition      m_eCrossoverType;
    css::chart::ChartAxisLabelPosition m_eLabelPos;
    css::chart::ChartAxisMarkPosition  m_eTickmarkPos;

    std::optional<double> m_pfMainLinePositionAtOtherAxis;
    std::optional<double> m_pfExrtaLinePositionAtOtherAxis;

    bool        m_bCrossingAxisHasReverseDirection;
    bool        m_bCrossingAxisIsCategoryAxes;

    AxisLabelAlignment maLabelAlignment;

    // Data table
    bool m_bDisplayDataTable;
    bool m_bDataTableAlignAxisValuesWithColumns;

    bool m_bDisplayLabels;

    // Compatibility option: starting from LibreOffice 5.1 the rotated
    // layout is preferred to staggering for axis labels.
    // So the default value of this flag for new documents is `false`.
    bool            m_bTryStaggeringFirst;

    sal_Int32       m_nNumberFormatKey;

    /*
    0: no tickmarks         1: inner tickmarks
    2: outer tickmarks      3: inner and outer tickmarks
    */
    sal_Int32                           m_nMajorTickmarks;
    sal_Int32                           m_nMinorTickmarks;
    std::vector<TickmarkProperties>   m_aTickmarkPropertiesList;

    VLineProperties                     m_aLineProperties;

    //for category axes ->
    sal_Int32                           m_nAxisType;//REALNUMBER, CATEGORY etc. type css::chart2::AxisType
    bool                                m_bComplexCategories;
    ExplicitCategoriesProvider* m_pExplicitCategoriesProvider;/*no ownership here*/
    css::uno::Reference<css::chart2::data::XTextualDataSequence> m_xAxisTextProvider; //for categories or series names
    //<- category axes

    bool                                m_bLimitSpaceForLabels;

    rtl::Reference<::chart::DataTable> m_xDataTableModel;

    //methods:

    AxisProperties(rtl::Reference<::chart::Axis> xAxisModel,
                   ExplicitCategoriesProvider* pExplicitCategoriesProvider,
                   rtl::Reference<::chart::DataTable> const& xDataTableModel);

    void init(bool bCartesian=false);//init from model data (m_xAxisModel)

    void initAxisPositioning( const css::uno::Reference< css::beans::XPropertySet >& xAxisProp );

    static TickmarkProperties getBiggestTickmarkProperties();
    TickmarkProperties makeTickmarkPropertiesForComplexCategories( sal_Int32 nTickLength, sal_Int32 nTickStartDistanceToAxis ) const;

private:
    AxisProperties() = delete;

    TickmarkProperties  makeTickmarkProperties( sal_Int32 nDepth ) const;
    //@todo get this from somewhere; maybe for each subincrement
    //so far the model does not offer different settings for each tick depth
    const VLineProperties&  makeLinePropertiesForDepth() const { return m_aLineProperties; }
};

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
