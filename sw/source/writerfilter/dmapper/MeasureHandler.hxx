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

#include "LoggedResources.hxx"
#include <vector>
#include <com/sun/star/beans/PropertyValue.hpp>

namespace writerfilter::dmapper
{
/** Handler for sprms that contain a measure and a unit
    - Left indent of tables
    - Preferred width of tables
 */
class MeasureHandler : public LoggedProperties
{
    sal_Int32 m_nMeasureValue;
    sal_Int32 m_nUnit;
    sal_Int16 m_nRowHeightSizeType; //table row height type

    OUString m_aInteropGrabBagName;
    std::vector<css::beans::PropertyValue> m_aInteropGrabBag;

    // Properties
    virtual void lcl_attribute(Id Name, const Value& val) override;
    virtual void lcl_sprm(Sprm& sprm) override;

public:
    MeasureHandler();
    virtual ~MeasureHandler() override;

    sal_Int32 getMeasureValue() const;

    sal_Int32 getValue() const { return m_nMeasureValue; }
    sal_Int32 getUnit() const { return m_nUnit; }

    sal_Int16 GetRowHeightSizeType() const { return m_nRowHeightSizeType; }
    void enableInteropGrabBag(const OUString& aName);
    css::beans::PropertyValue getInteropGrabBag();
};
typedef tools::SvRef<MeasureHandler> MeasureHandlerPtr;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
