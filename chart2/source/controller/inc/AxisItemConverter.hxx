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
#include <com/sun/star/awt/Size.hpp>
#include <rtl/ref.hxx>

#include <optional>
#include <vector>

namespace com::sun::star::beans { class XPropertySet; }
namespace com::sun::star::chart2 { class XAxis; }
namespace com::sun::star::chart2 { class XChartDocument; }
namespace chart { class Axis; }
namespace chart { struct ExplicitIncrementData; }
namespace chart { struct ExplicitScaleData; }
namespace chart { class ChartModel; }

class SdrModel;

namespace chart::wrapper {

class AxisItemConverter final : public ItemConverter
{
public:
    AxisItemConverter(
        const css::uno::Reference<css::beans::XPropertySet>& rPropertySet,
        SfxItemPool& rItemPool, SdrModel& rDrawModel,
        const rtl::Reference<::chart::ChartModel> & xChartDoc,
        ExplicitScaleData const * pScale,
        ExplicitIncrementData const * pIncrement,
        const std::optional<css::awt::Size>& pRefSize );

    virtual ~AxisItemConverter() override;

    virtual void FillItemSet( SfxItemSet & rOutItemSet ) const override;
    virtual bool ApplyItemSet( const SfxItemSet & rItemSet ) override;

protected:
    virtual const WhichRangesContainer& GetWhichPairs() const override;
    virtual bool GetItemProperty( tWhichIdType nWhichId, tPropertyNameWithMemberId & rOutProperty ) const override;

    virtual void FillSpecialItem( sal_uInt16 nWhichId, SfxItemSet & rOutItemSet ) const override;
    virtual bool ApplySpecialItem( sal_uInt16 nWhichId, const SfxItemSet & rItemSet ) override;

private:
    std::vector< std::unique_ptr<ItemConverter> >  m_aConverters;
    rtl::Reference<::chart::Axis>  m_xAxis;

    rtl::Reference<::chart::ChartModel>m_xChartDoc;

    std::unique_ptr<ExplicitScaleData>  m_pExplicitScale;
    std::unique_ptr<ExplicitIncrementData>  m_pExplicitIncrement;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
