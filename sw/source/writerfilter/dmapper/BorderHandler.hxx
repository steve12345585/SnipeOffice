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

#include <vector>
#include "LoggedResources.hxx"
#include "PropertyMap.hxx"
#include <com/sun/star/table/BorderLine2.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <o3tl/enumarray.hxx>
#include <docmodel/theme/ThemeColorType.hxx>
#include <docmodel/color/ComplexColor.hxx>

namespace writerfilter::dmapper
{
class PropertyMap;
class BorderHandler : public LoggedProperties
{
private:
    //todo: order is a guess
    enum class BorderPosition
    {
        Top,
        Left,
        Bottom,
        Right,
        Horizontal,
        Vertical,
        LAST = Vertical
    };

    //values of the current border
    sal_Int32       m_nLineWidth;
    sal_Int32       m_nLineType;
    sal_Int32       m_nLineColor;
    sal_Int32       m_nLineDistance;
    bool            m_bShadow;
    bool            m_bOOXML;
    model::ThemeColorType m_eThemeColorType = model::ThemeColorType::Unknown;
    sal_Int32 m_nThemeShade = 0;
    sal_Int32 m_nThemeTint = 0;

    o3tl::enumarray<BorderPosition, bool> m_aFilledLines;
    o3tl::enumarray<BorderPosition, css::table::BorderLine2> m_aBorderLines;
    OUString m_aInteropGrabBagName;
    std::vector<css::beans::PropertyValue> m_aInteropGrabBag;
    void appendGrabBag(const OUString& aKey, const OUString& aValue);

    // Properties
    virtual void lcl_attribute(Id Name, const Value & val) override;
    virtual void lcl_sprm(Sprm & sprm) override;

public:
    explicit BorderHandler( bool bOOXML );
    virtual ~BorderHandler() override;

    PropertyMapPtr          getProperties();
    css::table::BorderLine2 getBorderLine();
    sal_Int32                                   getLineDistance() const { return m_nLineDistance;}
    sal_Int32                                   getLineType() const { return m_nLineType;}
    bool                                        getShadow() const { return m_bShadow;}
    void enableInteropGrabBag(const OUString& aName);
    css::beans::PropertyValue getInteropGrabBag(const OUString& aName = OUString());

    model::ComplexColor getComplexColor() const;

};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
