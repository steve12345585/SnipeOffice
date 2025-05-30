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

#include <sal/config.h>

#include <comphelper/propertyvalue.hxx>
#include <utility>
#include <vcl/fieldvalues.hxx>
#include <vcl/settings.hxx>
#include <vcl/svapp.hxx>
#include <vcl/toolbox.hxx>
#include <svl/intitem.hxx>

#include <strings.hrc>

#include <diactrl.hxx>
#include <SlideSorter.hxx>

#include <sdresid.hxx>
#include <app.hrc>

#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/frame/XFrame.hpp>

using namespace ::com::sun::star;

SFX_IMPL_TOOLBOX_CONTROL( SdTbxCtlDiaPages,  SfxUInt16Item )

namespace
{
    OUString format_number(int nSlides)
    {
        OUString aSlides(SdResId(STR_SLIDES, nSlides));
        return aSlides.replaceFirst("%1", OUString::number(nSlides));
    }
}

// SdPagesField
SdPagesField::SdPagesField( vcl::Window* pParent,
                            uno::Reference< frame::XFrame > xFrame )
    : InterimItemWindow(pParent, u"modules/simpress/ui/pagesfieldbox.ui"_ustr, u"PagesFieldBox"_ustr)
    , m_xWidget(m_xBuilder->weld_spin_button(u"pagesfield"_ustr))
    , m_xFrame(std::move(xFrame))
{
    InitControlBase(m_xWidget.get());

    // set parameter of MetricFields
    m_xWidget->set_digits(0);
    m_xWidget->set_range(1, MAX_PAGES_PER_ROW);
    m_xWidget->set_increments(1, 5);
    m_xWidget->connect_value_changed(LINK(this, SdPagesField, ModifyHdl));
    m_xWidget->set_value_formatter(LINK(this, SdPagesField, OutputHdl));
    m_xWidget->set_text_parser(LINK(this, SdPagesField, spin_button_input));
    m_xWidget->connect_key_press(LINK(this, SdPagesField, KeyInputHdl));

    auto width = std::max(m_xWidget->get_pixel_size(format_number(1)).Width(),
                          m_xWidget->get_pixel_size(format_number(15)).Width());
    int chars = ceil(width / m_xWidget->get_approximate_digit_width());
    m_xWidget->set_width_chars(chars);

    SetSizePixel(m_xWidget->get_preferred_size());
}

IMPL_LINK(SdPagesField, KeyInputHdl, const KeyEvent&, rKEvt, bool)
{
    return ChildKeyInput(rKEvt);
}

void SdPagesField::dispose()
{
    m_xWidget.reset();
    InterimItemWindow::dispose();
}

SdPagesField::~SdPagesField()
{
    disposeOnce();
}

void SdPagesField::set_sensitive(bool bSensitive)
{
    Enable(bSensitive);
    m_xWidget->set_sensitive(bSensitive);
    if (!bSensitive)
        m_xWidget->set_text(u""_ustr);
}

void SdPagesField::UpdatePagesField( const SfxUInt16Item* pItem )
{
    if (pItem)
        m_xWidget->set_value(pItem->GetValue());
    else
        m_xWidget->set_text(OUString());
}

IMPL_STATIC_LINK(SdPagesField, OutputHdl, sal_Int64, nValue, OUString)
{
    return format_number(nValue);
}

IMPL_LINK(SdPagesField, spin_button_input, const OUString&, rText, std::optional<int>)
{
    const LocaleDataWrapper& rLocaleData = Application::GetSettings().GetLocaleDataWrapper();
    double fResult(0.0);
    bool bRet = vcl::TextToValue(rText, fResult, 0, m_xWidget->get_digits(), rLocaleData, FieldUnit::NONE);
    if (!bRet)
        return {};

    if (fResult > SAL_MAX_INT32)
        fResult = SAL_MAX_INT32;
    else if (fResult < SAL_MIN_INT32)
        fResult = SAL_MIN_INT32;

    return std::optional<int>(fResult);
}

IMPL_LINK_NOARG(SdPagesField, ModifyHdl, weld::SpinButton&, void)
{
    SfxUInt16Item aItem(SID_PAGES_PER_ROW, m_xWidget->get_value());

    uno::Any a;
    aItem.QueryValue( a );
    uno::Sequence< beans::PropertyValue > aArgs{ comphelper::makePropertyValue(u"PagesPerRow"_ustr, a) };
    SfxToolBoxControl::Dispatch( ::uno::Reference< ::frame::XDispatchProvider >( m_xFrame->getController(), ::uno::UNO_QUERY ),
                                 u".uno:PagesPerRow"_ustr,
                                 aArgs );
}

SdTbxCtlDiaPages::SdTbxCtlDiaPages( sal_uInt16 nSlotId, ToolBoxItemId nId, ToolBox& rTbx ) :
    SfxToolBoxControl( nSlotId, nId, rTbx )
{
}

SdTbxCtlDiaPages::~SdTbxCtlDiaPages()
{
}

void SdTbxCtlDiaPages::StateChangedAtToolBoxControl( sal_uInt16,
                SfxItemState eState, const SfxPoolItem* pState )
{
    SdPagesField* pFld = static_cast<SdPagesField*>( GetToolBox().GetItemWindow( GetId() ) );
    DBG_ASSERT( pFld, "Window not found" );

    if ( eState == SfxItemState::DISABLED )
    {
        pFld->set_sensitive(false);
    }
    else
    {
        pFld->set_sensitive(true);

        const SfxUInt16Item* pItem = nullptr;
        if ( eState == SfxItemState::DEFAULT )
        {
            pItem = dynamic_cast< const SfxUInt16Item* >( pState );
            DBG_ASSERT( pItem, "sd::SdTbxCtlDiaPages::StateChanged(), wrong item type!" );
        }

        pFld->UpdatePagesField( pItem );
    }
}

VclPtr<InterimItemWindow> SdTbxCtlDiaPages::CreateItemWindow( vcl::Window* pParent )
{
    VclPtr<SdPagesField> pWindow = VclPtr<SdPagesField>::Create(pParent, m_xFrame);
    pWindow->Show();

    return pWindow;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
