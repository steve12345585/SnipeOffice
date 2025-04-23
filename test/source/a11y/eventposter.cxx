/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/a11y/eventposter.hxx>

#include <com/sun/star/accessibility/XAccessible.hpp>
#include <com/sun/star/accessibility/XAccessibleContext.hpp>
#include <com/sun/star/uno/Reference.hxx>

#include <sfx2/lokhelper.hxx>
#include <test/a11y/AccessibilityTools.hxx>
#include <toolkit/awt/vclxwindow.hxx>

void test::EventPosterHelper::postKeyEventAsync(int nType, int nCharCode, int nKeyCode) const
{
    SfxLokHelper::postKeyEventAsync(mxWindow, nType, nCharCode, nKeyCode);
}

void test::EventPosterHelper::postExtTextEventAsync(int nType, const OUString& rText) const
{
    SfxLokHelper::postExtTextEventAsync(mxWindow, nType, rText);
}

void test::AccessibleEventPosterHelper::setWindow(
    css::uno::Reference<css::accessibility::XAccessible> xAcc)
{
    while (auto xParent = xAcc->getAccessibleContext()->getAccessibleParent())
        xAcc = xParent;
    auto vclXWindow = dynamic_cast<VCLXWindow*>(xAcc.get());
    if (!vclXWindow)
    {
        std::cerr << "WARNING: AccessibleEventPosterHelper::setWindow() called on "
                     "unsupported object "
                  << AccessibilityTools::debugString(xAcc) << ". Event delivery will not work."
                  << std::endl;
    }
    mxWindow = vclXWindow ? vclXWindow->GetWindow() : nullptr;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
