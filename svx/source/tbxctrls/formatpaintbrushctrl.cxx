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


#include <svx/formatpaintbrushctrl.hxx>

#include <comphelper/propertyvalue.hxx>
#include <svl/eitem.hxx>
#include <vcl/toolbox.hxx>
#include <vcl/settings.hxx>


namespace svx
{


using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;

SFX_IMPL_TOOLBOX_CONTROL( FormatPaintBrushToolBoxControl, SfxBoolItem );

FormatPaintBrushToolBoxControl::FormatPaintBrushToolBoxControl( sal_uInt16 nSlotId, ToolBoxItemId nId, ToolBox& rTbx )
    : SfxToolBoxControl( nSlotId, nId, rTbx )
    , m_bPersistentCopy(false)
    , m_aDoubleClickTimer("FormatPaintBrushToolBoxControl m_aDoubleClickTimer")
{
    sal_uInt64 nDblClkTime = rTbx.GetSettings().GetMouseSettings().GetDoubleClickTime();

    m_aDoubleClickTimer.SetInvokeHandler( LINK(this, FormatPaintBrushToolBoxControl, WaitDoubleClickHdl) );
    m_aDoubleClickTimer.SetTimeout(nDblClkTime);
}


FormatPaintBrushToolBoxControl::~FormatPaintBrushToolBoxControl()
{
    m_aDoubleClickTimer.Stop();
}


void FormatPaintBrushToolBoxControl::impl_executePaintBrush()
{
    Sequence< PropertyValue > aArgs{ comphelper::makePropertyValue(u"PersistentCopy"_ustr,
                                                                   m_bPersistentCopy) };
    Dispatch( u".uno:FormatPaintbrush"_ustr
        , aArgs );
}


void FormatPaintBrushToolBoxControl::DoubleClick()
{
    m_aDoubleClickTimer.Stop();

    m_bPersistentCopy = true;
    impl_executePaintBrush();
}


void FormatPaintBrushToolBoxControl::Click()
{
    m_bPersistentCopy = false;
    m_aDoubleClickTimer.Start();
}


IMPL_LINK_NOARG(FormatPaintBrushToolBoxControl, WaitDoubleClickHdl, Timer *, void)
{
    //there was no second click during waiting
    impl_executePaintBrush();
}


void FormatPaintBrushToolBoxControl::Select(sal_uInt16 /*nSelectModifier*/)
{
}


void FormatPaintBrushToolBoxControl::StateChangedAtToolBoxControl( sal_uInt16 nSID, SfxItemState eState,
                const SfxPoolItem* pState )
{
    if( eState != SfxItemState::DEFAULT && eState != SfxItemState::SET )
        m_bPersistentCopy = false;
    SfxToolBoxControl::StateChangedAtToolBoxControl( nSID, eState, pState );
}


} //namespace svx


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
