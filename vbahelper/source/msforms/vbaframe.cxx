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

#include "vbaframe.hxx"
#include "vbanewfont.hxx"
#include "vbacontrols.hxx"
#include <ooo/vba/msforms/fmBorderStyle.hpp>
#include <ooo/vba/msforms/fmSpecialEffect.hpp>
#include <utility>

using namespace com::sun::star;
using namespace ooo::vba;

ScVbaFrame::ScVbaFrame(
        const uno::Reference< XHelperInterface >& xParent,
        const uno::Reference< uno::XComponentContext >& xContext,
        const uno::Reference< uno::XInterface >& xControl,
        const uno::Reference< frame::XModel >& xModel,
        std::unique_ptr<ov::AbstractGeometryAttributes> pGeomHelper,
        css::uno::Reference< css::awt::XControl > xDialog ) :
    FrameImpl_BASE( xParent, xContext, xControl, xModel, std::move(pGeomHelper) ),
    mxDialog(std::move( xDialog ))
{
}

// XFrame attributes

OUString SAL_CALL ScVbaFrame::getCaption()
{
    OUString Label;
    m_xProps->getPropertyValue( u"Label"_ustr ) >>= Label;
    return Label;
}

void SAL_CALL ScVbaFrame::setCaption( const OUString& _caption )
{
    m_xProps->setPropertyValue( u"Label"_ustr, uno::Any( _caption ) );
}

sal_Int32 SAL_CALL ScVbaFrame::getSpecialEffect()
{
    return msforms::fmSpecialEffect::fmSpecialEffectEtched;
}


void SAL_CALL ScVbaFrame::setSpecialEffect( sal_Int32 /*nSpecialEffect*/ )
{
    // #STUB
}

sal_Int32 SAL_CALL ScVbaFrame::getBorderStyle()
{
    return msforms::fmBorderStyle::fmBorderStyleNone;
}

void SAL_CALL ScVbaFrame::setBorderStyle( sal_Int32 /*nBorderStyle*/ )
{
    // #STUB
}

uno::Reference< msforms::XNewFont > SAL_CALL ScVbaFrame::getFont()
{
    return new VbaNewFont( m_xProps );
}

// XFrame methods

uno::Any SAL_CALL ScVbaFrame::Controls( const uno::Any& rIndex )
{
    // horizontal anchor of frame children is inside border line (add one unit to compensate border line width)
    double fOffsetX = mpGeometryHelper->getOffsetX() + getLeft() + 1.0;
    // vertical anchor of frame children is inside border line (add half of text height and one unit to compensate border line width)
    double fOffsetY = mpGeometryHelper->getOffsetY() + getTop() + (getFont()->getSize() / 2.0) + 1.0;

    uno::Reference< XCollection > xControls( new ScVbaControls( this, mxContext, mxDialog, m_xModel, fOffsetX, fOffsetY ) );
    if( rIndex.hasValue() )
        return xControls->Item( rIndex, uno::Any() );
    return uno::Any( xControls );
}

// XHelperInterface

VBAHELPER_IMPL_XHELPERINTERFACE( ScVbaFrame, u"ooo.vba.msforms.Frame"_ustr )
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
