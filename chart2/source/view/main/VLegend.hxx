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

#include <Legend.hxx>

#include <com/sun/star/uno/Reference.hxx>
#include <rtl/ref.hxx>
#include <svx/unoshape.hxx>
#include <vector>

namespace chart { class ChartModel; }
namespace com::sun::star::awt { struct Rectangle; }
namespace com::sun::star::awt { struct Size; }
namespace com::sun::star::uno { class XComponentContext; }

namespace chart
{

class LegendEntryProvider;

class VLegend
{
public:
    VLegend( rtl::Reference< ::chart::Legend > xLegend,
             const css::uno::Reference< css::uno::XComponentContext > & xContext,
             std::vector< LegendEntryProvider* >&& rLegendEntryProviderList,
             rtl::Reference<SvxShapeGroupAnyD> xTargetPage,
             ChartModel& rModel  );

    void setDefaultWritingMode( sal_Int16 nDefaultWritingMode );

    void createShapes( const css::awt::Size & rAvailableSpace,
                       const css::awt::Size & rPageSize,
                       css::awt::Size & rDefaultLegendSize );

    /** Sets the position according to its internal anchor.

        @param rOutAvailableSpace
            is modified by the method, if the legend is in a standard position,
            such that the space allocated by the legend is removed from it.

        @param rReferenceSize
            is used to calculate the offset (default 2%) from the edge.
     */
    void changePosition(
        css::awt::Rectangle & rOutAvailableSpace,
        const css::awt::Size & rReferenceSize,
        const css::awt::Size & rDefaultLegendSize );

    static bool isVisible(
        const rtl::Reference< ::chart::Legend > & xLegend );

private:
    rtl::Reference<SvxShapeGroupAnyD>            m_xTarget;
    rtl::Reference<::chart::Legend>              m_xLegend;
    rtl::Reference< SvxShapeGroup >                         m_xShape;

    ChartModel& mrModel;

    css::uno::Reference< css::uno::XComponentContext >      m_xContext;

    std::vector< LegendEntryProvider* >         m_aLegendEntryProviderList;

    sal_Int16 m_nDefaultWritingMode;//to be used when writing mode is set to page
};

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
