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

#include <WrappedProperty.hxx>

#include <memory>
#include <vector>

namespace chart::wrapper { class Chart2ModelContact; }

namespace chart::wrapper
{

class WrappedSceneProperty
{
public:
    static void addWrappedProperties( std::vector< std::unique_ptr<WrappedProperty> >& rList
        , const std::shared_ptr< Chart2ModelContact >& spChart2ModelContact );
};

class WrappedD3DTransformMatrixProperty : public WrappedProperty
{
public:
    explicit WrappedD3DTransformMatrixProperty(
        std::shared_ptr<Chart2ModelContact> spChart2ModelContact);
    virtual ~WrappedD3DTransformMatrixProperty() override;

    virtual void setPropertyValue( const css::uno::Any& rOuterValue, const css::uno::Reference< css::beans::XPropertySet >& xInnerPropertySet ) const override;

    virtual css::uno::Any getPropertyValue( const css::uno::Reference< css::beans::XPropertySet >& xInnerPropertySet ) const override;

private:
    std::shared_ptr< Chart2ModelContact >   m_spChart2ModelContact;
};

} //namespace chart::wrapper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
