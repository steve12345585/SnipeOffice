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

#include "AccessibleViewForwarder.hxx"
#include <AccessibleChartView.hxx>

#include <vcl/window.hxx>

using namespace ::com::sun::star;

namespace chart
{

AccessibleViewForwarder::AccessibleViewForwarder( AccessibleChartView* pAccChartView, vcl::Window* pWindow )
    :m_pAccChartView( pAccChartView )
    ,m_pWindow( pWindow )
    ,m_aMapMode( MapUnit::Map100thMM )
{
}

AccessibleViewForwarder::~AccessibleViewForwarder()
{
}

// ________ IAccessibleViewforwarder ________

tools::Rectangle AccessibleViewForwarder::GetVisibleArea() const
{
    tools::Rectangle aVisibleArea;
    if ( m_pWindow )
    {
        aVisibleArea = m_pWindow->PixelToLogic(
            tools::Rectangle( Point( 0, 0 ), m_pWindow->GetOutputSizePixel() ),
            m_aMapMode );
    }
    return aVisibleArea;
}

Point AccessibleViewForwarder::LogicToPixel( const Point& rPoint ) const
{
    Point aPoint;
    if ( m_pAccChartView && m_pWindow )
    {
        awt::Point aLocation = m_pAccChartView->getLocationOnScreen();
        Point aTopLeft( aLocation.X, aLocation.Y );
        aPoint = m_pWindow->LogicToPixel( rPoint, m_aMapMode ) + aTopLeft;
    }
    return aPoint;
}

Size AccessibleViewForwarder::LogicToPixel( const Size& rSize ) const
{
    Size aSize;
    if ( m_pWindow )
    {
        aSize = m_pWindow->LogicToPixel( rSize, m_aMapMode );
    }
    return aSize;
}

} // namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
