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

#include "VAxisBase.hxx"
#include <PlottingPositionHelper.hxx>
#include <memory>

class SvNumberFormatsSupplierObj;

namespace chart
{

class VPolarAxis : public VAxisBase
{
public:
    static std::shared_ptr<VPolarAxis> createAxis( const AxisProperties& rAxisProperties
           , const rtl::Reference< SvNumberFormatsSupplierObj >& xNumberFormatsSupplier
           , sal_Int32 nDimensionIndex, sal_Int32 nDimensionCount );

    void setIncrements( std::vector< ExplicitIncrementData >&& rIncrements );

    virtual bool isAnythingToDraw() override;

    virtual ~VPolarAxis() override;

protected:
    VPolarAxis( const AxisProperties& rAxisProperties
           , const rtl::Reference< SvNumberFormatsSupplierObj >& xNumberFormatsSupplier
           , sal_Int32 nDimensionIndex, sal_Int32 nDimensionCount );

protected: //member
    PolarPlottingPositionHelper m_aPosHelper;
    std::vector< ExplicitIncrementData >   m_aIncrements;
};

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
