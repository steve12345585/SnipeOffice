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

#include <RelativeSizeHelper.hxx>
#include <com/sun/star/awt/Size.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <comphelper/diagnose_ex.hxx>
#include <svx/unoshape.hxx>
#include <vector>
#include <algorithm>

using namespace ::com::sun::star;
using namespace ::com::sun::star::beans;

using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Any;
using ::com::sun::star::uno::Exception;

namespace chart
{

double RelativeSizeHelper::calculate(
    double fValue,
    const awt::Size & rOldReferenceSize,
    const awt::Size & rNewReferenceSize )
{
    if( rOldReferenceSize.Width <= 0 ||
        rOldReferenceSize.Height <= 0 )
        return fValue;

    return std::min(
        static_cast< double >( rNewReferenceSize.Width )  / static_cast< double >( rOldReferenceSize.Width ),
        static_cast< double >( rNewReferenceSize.Height ) / static_cast< double >( rOldReferenceSize.Height ))
        * fValue;
}

void RelativeSizeHelper::adaptFontSizes(
    SvxShapeText& xTargetProperties,
    const awt::Size & rOldReferenceSize,
    const awt::Size & rNewReferenceSize )
{
    float fFontHeight = 0;

    std::vector< OUString > aProperties;
    aProperties.emplace_back("CharHeight" );
    aProperties.emplace_back("CharHeightAsian" );
    aProperties.emplace_back("CharHeightComplex" );

    for (auto const& property : aProperties)
    {
        try
        {
            if( xTargetProperties.SvxShape::getPropertyValue(property) >>= fFontHeight )
            {
                xTargetProperties.SvxShape::setPropertyValue(
                    property,
                    Any( static_cast< float >(
                                 calculate( fFontHeight, rOldReferenceSize, rNewReferenceSize ))));
            }
        }
        catch( const Exception & )
        {
            DBG_UNHANDLED_EXCEPTION("chart2");
        }
    }
}

void RelativeSizeHelper::adaptFontSizes(
    const Reference< XPropertySet > & xTargetProperties,
    const awt::Size & rOldReferenceSize,
    const awt::Size & rNewReferenceSize )
{
    if( ! xTargetProperties.is())
        return;

    float fFontHeight = 0;

    std::vector< OUString > aProperties;
    aProperties.emplace_back("CharHeight" );
    aProperties.emplace_back("CharHeightAsian" );
    aProperties.emplace_back("CharHeightComplex" );

    for (auto const& property : aProperties)
    {
        try
        {
            if( xTargetProperties->getPropertyValue(property) >>= fFontHeight )
            {
                xTargetProperties->setPropertyValue(
                    property,
                    Any( static_cast< float >(
                                 calculate( fFontHeight, rOldReferenceSize, rNewReferenceSize ))));
            }
        }
        catch( const Exception & )
        {
            DBG_UNHANDLED_EXCEPTION("chart2");
        }
    }
}

} //  namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
