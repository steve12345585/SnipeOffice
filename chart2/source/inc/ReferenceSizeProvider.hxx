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

#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/awt/Size.hpp>
#include <rtl/ref.hxx>
#include <ChartModel.hxx>

namespace com::sun::star {
    namespace chart2 {
        class XTitle;
        class XTitled;
    }
    namespace beans {
        class XPropertySet;
    }
}

namespace chart
{

class ReferenceSizeProvider
{
public:

    enum AutoResizeState
    {
        AUTO_RESIZE_YES,
        AUTO_RESIZE_NO,
        AUTO_RESIZE_AMBIGUOUS,
        AUTO_RESIZE_UNKNOWN
    };

    ReferenceSizeProvider(
        css::awt::Size aPageSize,
        const rtl::Reference<::chart::ChartModel> & xChartDoc );

    const css::awt::Size& getPageSize() const { return m_aPageSize;}

    /** Retrieves the state auto-resize from all objects that support this
        feature.  If all objects return the same state, AUTO_RESIZE_YES or
        AUTO_RESIZE_NO is returned.

        If no object supporting the feature is found, AUTO_RESIZE_UNKNOWN is
        returned.  If there are multiple objects, some with state YES and some
        with state NO, AUTO_RESIZE_AMBIGUOUS is returned.
    */
    static AutoResizeState getAutoResizeState(
        const rtl::Reference<::chart::ChartModel> & xChartDoc );

    /** sets or resets the auto-resize at all objects that support this feature
        for text to the opposite of the current setting.  If the current state
        is ambiguous, it is turned on.  If the current state is unknown it stays
        unknown.
    */
    void toggleAutoResizeState();

    /** Sets the ReferencePageSize according to the internal settings of this
        class at the XPropertySet, and the adapted font sizes if bAdaptFontSizes
        is </sal_True>.
     */
    void setValuesAtPropertySet(
        const css::uno::Reference< css::beans::XPropertySet > & xProp,
        bool bAdaptFontSizes = true );

    /** Sets the ReferencePageSize according to the internal settings of this
        class at the XTitle, and the adapted font sizes at the contained
        XFormattedStrings
     */
    void setValuesAtTitle(
        const css::uno::Reference< css::chart2::XTitle > & xTitle );

    /** Sets the internal value at all data series in the currently set model.
        This is useful, if you have changed a chart-type and thus probably added
        some new data series via model functionality.
     */
    void setValuesAtAllDataSeries();

private:
    bool useAutoScale() const { return m_bUseAutoScale;}

    /** sets the auto-resize at all objects that support this feature for text.
        eNewState must be either AUTO_RESIZE_YES or AUTO_RESIZE_NO
    */
    void setAutoResizeState( AutoResizeState eNewState );

    /** Retrieves the auto-resize state from the given propertyset.  The result
        will be put into eInOutState.  If you initialize eInOutState with
        AUTO_RESIZE_UNKNOWN, you will get the actual state.  If you pass any
        other state, the result will be the accumulated state,
        esp. AUTO_RESIZE_AMBIGUOUS, if the value was NO before, and is YES for
        the current property set, or the other way round.
     */
    static void getAutoResizeFromPropSet(
        const css::uno::Reference< css::beans::XPropertySet > & xProp,
        AutoResizeState & rInOutState );

    void impl_setValuesAtTitled(
        const css::uno::Reference< css::chart2::XTitled > & xTitled );
    static void impl_getAutoResizeFromTitled(
        const css::uno::Reference< css::chart2::XTitled > & xTitled,
        AutoResizeState & rInOutState );

    css::awt::Size m_aPageSize;
    rtl::Reference<::chart::ChartModel> m_xChartDoc;
    bool      m_bUseAutoScale;
};

} //  namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
