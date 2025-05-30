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
#include <docmodel/theme/ThemeColorType.hxx>
#include <docmodel/color/ComplexColor.hxx>

namespace com::sun::star{
    namespace table {
        struct BorderLine2;
    }
    namespace beans {
        struct PropertyValue;
    }
}

namespace writerfilter::dmapper
{
class PropertyMap;
class TablePropertyMap;
class TDefTableHandler : public LoggedProperties
{
    std::vector<css::table::BorderLine2> m_aLeftBorderLines;
    std::vector<css::table::BorderLine2> m_aRightBorderLines;
    std::vector<css::table::BorderLine2> m_aTopBorderLines;
    std::vector<css::table::BorderLine2> m_aBottomBorderLines;
    std::vector<css::table::BorderLine2> m_aInsideHBorderLines;
    std::vector<css::table::BorderLine2> m_aInsideVBorderLines;

    //values of the current border
    sal_Int32 m_nLineWidth;
    sal_Int32 m_nLineType;
    sal_Int32 m_nLineColor;

    model::ThemeColorType m_eThemeColorType = model::ThemeColorType::Unknown;
    sal_Int32 m_nThemeShade = 0;
    sal_Int32 m_nThemeTint = 0;

    OUString m_aInteropGrabBagName;
    std::vector<css::beans::PropertyValue> m_aInteropGrabBag;
    void appendGrabBag(const OUString& aKey, const OUString& aValue);

    void localResolve(Id Name, const writerfilter::Reference<Properties>::Pointer_t& pProperties);

    // Properties
    virtual void lcl_attribute(Id Name, const Value & val) override;
    virtual void lcl_sprm(Sprm & sprm) override;

public:
    TDefTableHandler();
    virtual ~TDefTableHandler() override;

    void fillCellProperties( const ::tools::SvRef< TablePropertyMap >& pCellProperties) const;
    void enableInteropGrabBag(const OUString& aName);
    css::beans::PropertyValue getInteropGrabBag(const OUString& aName = OUString());

    static OUString getBorderTypeString(sal_Int32 nType);
    static OUString getThemeColorTypeString(sal_Int32 nType);
    static model::ThemeColorType getThemeColorTypeIndex(sal_Int32 nType);
    static model::ThemeColorUsage getThemeColorUsage(sal_Int32 nType);
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
