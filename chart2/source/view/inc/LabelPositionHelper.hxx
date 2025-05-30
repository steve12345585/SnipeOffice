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

#include "LabelAlignment.hxx"
#include "PropertyMapper.hxx"
#include <com/sun/star/awt/Point.hpp>
#include <rtl/ref.hxx>
#include <svx/unoshape.hxx>

namespace com::sun::star::drawing { struct Position3D; }
namespace com::sun::star::awt { struct Size; }

namespace chart
{

class LabelPositionHelper
{
public:
    LabelPositionHelper() = delete;
    LabelPositionHelper(
          sal_Int32 nDimensionCount
        , rtl::Reference<SvxShapeGroupAnyD> xLogicTarget );
    virtual ~LabelPositionHelper();

    css::awt::Point transformSceneToScreenPosition(
            const css::drawing::Position3D& rScenePosition3D ) const;

    static void changeTextAdjustment( tAnySequence& rPropValues, const tNameSequence& rPropNames, LabelAlignment eAlignment);
    static void doDynamicFontResize(  tAnySequence& rPropValues, const tNameSequence& rPropNames
                    , const css::uno::Reference< css::beans::XPropertySet >& xAxisModelProps
                    , const css::awt::Size& rNewReferenceSize );

    static void correctPositionForRotation( const rtl::Reference<SvxShapeText>& xShape2DText
                    , LabelAlignment eLabelAlignment, const double fRotationAngle, bool bRotateAroundCenter );

protected:
    sal_Int32                m_nDimensionCount;

private:
    //these members are only necessary for transformation from 3D to 2D
    rtl::Reference<SvxShapeGroupAnyD>    m_xLogicTarget;
};

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
