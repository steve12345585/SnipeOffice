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

#include <config_options.h>
#include "StackMode.hxx"
#include <com/sun/star/uno/Reference.h>
#include <rtl/ustring.hxx>
#include <rtl/ref.hxx>

#include <vector>

namespace com::sun::star::beans { class XPropertySet; }
namespace com::sun::star::chart2::data { class XDataSequence; }
namespace com::sun::star::chart2::data { class XDataSource; }
namespace com::sun::star::chart2::data { class XLabeledDataSequence; }
namespace com::sun::star::uno { class Any; }
namespace com::sun::star::uno { template <class E> class Sequence; }
namespace chart { class BaseCoordinateSystem; }
namespace chart { class ChartType; }
namespace chart { class DataSource; }
namespace chart { class Diagram; }
namespace chart { class DataSeries; }
namespace chart { class ChartModel; }

namespace chart::DataSeriesHelper
{

OUString getRole( const css::uno::Reference<css::chart2::data::XLabeledDataSequence>& xLabeledDataSequence );

/** Retrieves the data sequence in the given data source that matches the
    given role.  If more than one sequences match the role, the first match
    is returned.  If no sequence matches, an empty reference is returned.

    @param aSource
        The data source containing all data sequences to be searched through.

    @param aRole
        The role that is to be filtered out.
*/
css::uno::Reference< css::chart2::data::XLabeledDataSequence >
    getDataSequenceByRole( const css::uno::Reference< css::chart2::data::XDataSource > & xSource,
                           const OUString& aRole,
                           bool bMatchPrefix = false );

/** Retrieves all data sequences in the given data source that match the given
    role prefix.

    @param aSource
        The data source containing all data sequences to be searched through.

    @param aRole
        The role that is to be filtered out.
*/
std::vector<
  css::uno::Reference< css::chart2::data::XLabeledDataSequence > >
    getAllDataSequencesByRole( const css::uno::Sequence< css::uno::Reference< css::chart2::data::XLabeledDataSequence > > & aDataSequences,
                               const OUString& aRole );
std::vector<
  css::uno::Reference< css::chart2::data::XLabeledDataSequence > >
    getAllDataSequencesByRole( const std::vector< css::uno::Reference< css::chart2::data::XLabeledDataSequence > > & aDataSequences,
                               const OUString& aRole );

std::vector< css::uno::Reference< css::chart2::data::XLabeledDataSequence > >
getAllDataSequences(
    const std::vector<rtl::Reference<::chart::DataSeries> >& aSeries );

/** Retrieves all data sequences found in the given data series and puts them
    into a data source.  The order of sequences will match the order of the data
    series.
 */
rtl::Reference< ::chart::DataSource >
    getDataSource( const std::vector< rtl::Reference< ::chart::DataSeries > > & aSeries );

void setStackModeAtSeries(
    const std::vector< rtl::Reference< ::chart::DataSeries > > & aSeries,
    const rtl::Reference< ::chart::BaseCoordinateSystem > & xCorrespondingCoordinateSystem,
    StackMode eStackMode );

/// @param nAxisIndex, if -1 it is determined by the given data series via getAttachedAxisIndex
sal_Int32 getNumberFormatKeyFromAxis(
    const rtl::Reference< ::chart::DataSeries > & xSeries,
    const rtl::Reference< ::chart::BaseCoordinateSystem > & xCorrespondingCoordinateSystem,
    sal_Int32 nDimensionIndex,
    sal_Int32 nAxisIndex = -1 );

rtl::Reference< ::chart::BaseCoordinateSystem >
    getCoordinateSystemOfSeries(
        const rtl::Reference< ::chart::DataSeries > & xSeries,
        const rtl::Reference< ::chart::Diagram > & xDiagram );

rtl::Reference< ::chart::ChartType >
    getChartTypeOfSeries(
        const rtl::Reference< ::chart::DataSeries > & xSeries,
        const rtl::Reference< ::chart::Diagram > & xDiagram );

sal_Int32 translateIndexFromHiddenToFullSequence( sal_Int32 nClippedIndex, const css::uno::Reference<
        css::chart2::data::XDataSequence >& xDataSequence, bool bTranslate );

void insertDataLabelToPoint( const css::uno::Reference< css::beans::XPropertySet >& xPointPropertySet );

void deleteDataLabelsFromPoint( const css::uno::Reference< css::beans::XPropertySet >& xPointPropertySet );

} //  namespace chart::DataSeriesHelper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
