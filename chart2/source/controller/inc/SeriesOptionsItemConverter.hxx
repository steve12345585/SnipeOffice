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

#include "ItemConverter.hxx"
#include <com/sun/star/uno/Sequence.h>
#include <rtl/ref.hxx>

namespace com::sun::star::uno { class XComponentContext; }
namespace chart { class ChartModel; }
namespace chart { class BaseCoordinateSystem; }
namespace chart { class DataSeries; }

namespace chart::wrapper
{

class SeriesOptionsItemConverter final : public ItemConverter
{
public:
    SeriesOptionsItemConverter(
        const rtl::Reference<::chart::ChartModel> & xChartModel,
        css::uno::Reference< css::uno::XComponentContext > xContext,
        const rtl::Reference<::chart::DataSeries> & rPropertySet,
        SfxItemPool& rItemPool );
    virtual ~SeriesOptionsItemConverter() override;

protected:
    virtual const WhichRangesContainer& GetWhichPairs() const override;
    virtual bool GetItemProperty( tWhichIdType nWhichId, tPropertyNameWithMemberId & rOutProperty ) const override;

    virtual void FillSpecialItem( sal_uInt16 nWhichId, SfxItemSet & rOutItemSet ) const override;
    virtual bool ApplySpecialItem( sal_uInt16 nWhichId, const SfxItemSet & rItemSet ) override;

private:
    rtl::Reference<::chart::ChartModel>  m_xChartModel;
    css::uno::Reference< css::uno::XComponentContext>   m_xCC;

    bool m_bAttachToMainAxis;
    bool m_bSupportingOverlapAndGapWidthProperties;
    bool m_bSupportingBarConnectors;

    sal_Int32 m_nBarOverlap;
    sal_Int32 m_nGapWidth;
    bool  m_bConnectBars;

    bool m_bSupportingAxisSideBySide;
    bool m_bGroupBarsPerAxis;

    bool m_bSupportingStartingAngle;
    sal_Int32 m_nStartingAngle;

    bool m_bClockwise;
    rtl::Reference< ::chart::BaseCoordinateSystem > m_xCooSys;

    css::uno::Sequence< sal_Int32 > m_aSupportedMissingValueTreatments;
    sal_Int32 m_nMissingValueTreatment;

    bool m_bSupportingPlottingOfHiddenCells;
    bool m_bIncludeHiddenCells;

    bool m_bHideLegendEntry;
};

} //  namespace chart::wrapper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
