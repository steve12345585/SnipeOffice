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

#include <com/sun/star/chart2/LegendPosition.hpp>
#include <com/sun/star/chart2/XFormattedString2.hpp>
#include <com/sun/star/awt/Size.hpp>
#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/uno/Sequence.h>
#include <rtl/ref.hxx>
#include <svx/unoshape.hxx>
#include <vector>

namespace chart { class ChartModel; }
namespace com::sun::star::beans { class XPropertySet; }
namespace com::sun::star::drawing { class XShape; }
namespace com::sun::star::drawing { class XShapes; }
namespace com::sun::star::lang { class XMultiServiceFactory; }
namespace com::sun::star::uno { class XComponentContext; }

namespace chart
{
class FormattedString;

enum class LegendSymbolStyle
{
    /** A square box with border.
     */
    Box,

    /** A line like with a symbol.
     */
    Line,

    /** A bordered circle which has the same bounding-box as the
        <member>BOX</member>.
     */
    Circle
};

struct ViewLegendEntry
{
    /** The legend symbol that represents a data series or other
        information contained in the legend
     */
    rtl::Reference< SvxShapeGroup > xSymbol;

    /** The descriptive text for a legend entry.
     */
    rtl::Reference< ::chart::FormattedString > xLabel;
};


struct ViewLegendSymbol
{
    /** The legend symbol that represents a data series or other
        information contained in the legend
     */
    rtl::Reference<SvxShapeGroup> xSymbol;
};

class LegendEntryProvider
{
public:
    virtual css::awt::Size getPreferredLegendKeyAspectRatio()=0;

    virtual std::vector< ViewLegendEntry > createLegendEntries(
            const css::awt::Size& rEntryKeyAspectRatio,
            css::chart2::LegendPosition eLegendPosition,
            const css::uno::Reference< css::beans::XPropertySet >& xTextProperties,
            const rtl::Reference<SvxShapeGroupAnyD>& xTarget,
            const css::uno::Reference< css::uno::XComponentContext >& xContext,
            ChartModel& rModel
                ) = 0;

protected:
    ~LegendEntryProvider() {}
};

} //  namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
