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

#include "browserview.hxx"
#include "propertyeditor.hxx"
#include <helpids.h>
#include <memory>

namespace pcr
{
    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::lang;

    OPropertyBrowserView::OPropertyBrowserView(const css::uno::Reference<css::uno::XComponentContext>& rContext, weld::Builder& rBuilder)
        : m_xPropBox(new OPropertyEditor(rContext, rBuilder))
        , m_nActivePage(0)
    {
        m_xPropBox->SetHelpId(HID_FM_PROPDLG_TABCTR);
        m_xPropBox->setPageActivationHandler(LINK(this, OPropertyBrowserView, OnPageActivation));
    }

    IMPL_LINK(OPropertyBrowserView, OnPageActivation, const OUString&, rNewPage, void)
    {
        m_nActivePage = rNewPage.toUInt32();
        m_aPageActivationHandler.Call(nullptr);
    }

    OPropertyBrowserView::~OPropertyBrowserView()
    {
        sal_uInt16 nTmpPage = m_xPropBox->GetCurPage();
        if (nTmpPage)
            m_nActivePage = nTmpPage;
    }

    void OPropertyBrowserView::activatePage(sal_uInt16 _nPage)
    {
        m_nActivePage = _nPage;
        getPropertyBox().SetPage(m_nActivePage);
    }

    css::awt::Size OPropertyBrowserView::getMinimumSize() const
    {
        ::Size aSize = m_xPropBox->get_preferred_size();
        return css::awt::Size(aSize.Width(), aSize.Height());
    }
} // namespace pcr

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
